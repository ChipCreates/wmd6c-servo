/**
 * usb_cdc.c — DSR-1 USB CDC Virtual COM Port
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 * Hardware: STM32G0B1KBU6, bare-metal, no HAL, no RTOS
 *
 * Implements a minimal USB CDC ACM (Abstract Control Model) device that
 * enumerates as a virtual COM port on Windows, macOS, and Linux without
 * driver installation. Provides:
 *
 *   - Live servo tuning via single-character commands (p+/p-/i+/i-/f+/f-/t/T/s/r)
 *   - Telemetry snapshot and continuous streaming
 *   - Crystal-less USB via HSI48 + CRS (SOF-trimmed)
 *
 * Architecture:
 *   USB interrupt handler (USB_IRQHandler) processes endpoint events and
 *   fills/drains the packet memory area (PMA). Received bytes are placed
 *   in rx_buf[]. The main loop calls usb_cdc_poll() which drains rx_buf[],
 *   dispatches commands, and sends any queued telemetry.
 *
 *   The USB interrupt runs at priority 1 (below TIM2 servo at priority 0).
 *   USB activity cannot delay servo execution. See why-bare-metal.md §5.1.
 *
 * USB descriptors:
 *   Device descriptor:    bcdUSB 2.0, CDC class, VID 0x0483 (ST demo), PID 0x5740
 *   Configuration:        1 interface (CDC ACM), 2 endpoints (EP1 IN bulk, EP2 OUT bulk)
 *   CDC class descriptors: Header, Call Management, ACM, Union
 *
 * Packet Memory Area (PMA) layout (2KB total, 256 bytes used):
 *   0x000–0x03F  EP0 TX  (64 bytes)
 *   0x040–0x07F  EP0 RX  (64 bytes)
 *   0x080–0x0BF  EP1 TX  (64 bytes) — CDC IN (device→host, telemetry)
 *   0x0C0–0x0FF  EP2 RX  (64 bytes) — CDC OUT (host→device, commands)
 *
 * Crystal-less operation:
 *   USB clock: HSI48 trimmed by CRS using USB SOF packets as reference.
 *   System clock (64 MHz) is completely independent of HSI48.
 *   See usb-crystalless-operation.md for full derivation.
 *
 * NOTE: This file implements only the USB CDC data path. The STM32G0B1
 * factory DFU bootloader handles firmware update — no DFU code here.
 */

#include "stm32g0xx.h"
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "servo.h"
#include "adc.h"
#include "flash.h"
#include "usb_cdc.h"

/* =========================================================================
 * USB PMA (Packet Memory Area) helpers
 *
 * The STM32 USB peripheral uses a dedicated 2KB SRAM for USB packets,
 * accessed through a 16-bit wide bus at base address 0x40009800.
 * Each 16-bit PMA word occupies 32 bits of address space (16-bit bus
 * mapped into 32-bit address space with a gap word between each real word).
 * Write/read via 16-bit pointer with 32-bit stride.
 * ========================================================================= */

#define USB_PMA_BASE        0x40009800UL
#define EP0_TX_ADDR         0x00
#define EP0_RX_ADDR         0x40
#define EP1_TX_ADDR         0x80    /* CDC IN  — telemetry to host  */
#define EP2_RX_ADDR         0xC0    /* CDC OUT — commands from host */
#define PMA_EP_BUF_SIZE     64

/* Buffer descriptor table occupies first 64 bytes of PMA (8 entries × 8 bytes) */
#define USB_BTABLE_ADDR     0x00

typedef struct {
    volatile uint16_t ADDR_TX;  uint16_t _pad0;
    volatile uint16_t COUNT_TX; uint16_t _pad1;
    volatile uint16_t ADDR_RX;  uint16_t _pad2;
    volatile uint16_t COUNT_RX; uint16_t _pad3;
} PMA_BufDesc;

#define USB_BTABLE  ((PMA_BufDesc *)(USB_PMA_BASE + USB_BTABLE_ADDR))

/* Write a 16-bit value into PMA at byte offset 'pma_off' */
static inline void pma_write16(uint32_t pma_off, uint16_t val)
{
    *(__IO uint16_t *)(USB_PMA_BASE + pma_off * 2) = val;
}

/* Read a 16-bit value from PMA at byte offset 'pma_off' */
static inline uint16_t pma_read16(uint32_t pma_off)
{
    return *(__IO uint16_t *)(USB_PMA_BASE + pma_off * 2);
}

/* Copy bytes from SRAM to PMA */
static void pma_write_buf(uint32_t pma_off, const uint8_t *src, uint16_t len)
{
    for (uint16_t i = 0; i < (len + 1) / 2; i++) {
        uint16_t word = src[i*2];
        if (i*2 + 1 < len) word |= (uint16_t)src[i*2 + 1] << 8;
        pma_write16(pma_off + i*2, word);
    }
}

/* Copy bytes from PMA to SRAM */
static void pma_read_buf(uint32_t pma_off, uint8_t *dst, uint16_t len)
{
    for (uint16_t i = 0; i < (len + 1) / 2; i++) {
        uint16_t word = pma_read16(pma_off + i*2);
        dst[i*2] = (uint8_t)(word & 0xFF);
        if (i*2 + 1 < len) dst[i*2 + 1] = (uint8_t)(word >> 8);
    }
}

/* =========================================================================
 * USB descriptor tables
 * ========================================================================= */

/* Device descriptor */
static const uint8_t dev_desc[] = {
    18,         /* bLength */
    0x01,       /* bDescriptorType: Device */
    0x00, 0x02, /* bcdUSB: 2.00 */
    0x02,       /* bDeviceClass: CDC */
    0x00,       /* bDeviceSubClass */
    0x00,       /* bDeviceProtocol */
    64,         /* bMaxPacketSize0: EP0 64 bytes */
    0x83, 0x04, /* idVendor: 0x0483 (ST Microelectronics demo VID) */
    0x40, 0x57, /* idProduct: 0x5740 (ST CDC demo PID) */
    0x00, 0x02, /* bcdDevice: 2.00 */
    0x01,       /* iManufacturer (string index 1) */
    0x02,       /* iProduct (string index 2) */
    0x03,       /* iSerialNumber (string index 3) */
    0x01,       /* bNumConfigurations */
};

/* Configuration descriptor (total 67 bytes) */
static const uint8_t cfg_desc[] = {
    /* Configuration descriptor */
    9,          /* bLength */
    0x02,       /* bDescriptorType: Configuration */
    67, 0x00,   /* wTotalLength: 67 */
    0x02,       /* bNumInterfaces: 2 (CDC control + CDC data) */
    0x01,       /* bConfigurationValue */
    0x00,       /* iConfiguration */
    0x80,       /* bmAttributes: bus-powered, no remote wakeup */
    0x32,       /* bMaxPower: 100mA */

    /* Interface 0: CDC Control */
    9, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,

    /* CDC Header functional descriptor */
    5, 0x24, 0x00, 0x10, 0x01,

    /* CDC Call Management functional descriptor */
    5, 0x24, 0x01, 0x00, 0x01,

    /* CDC ACM functional descriptor */
    4, 0x24, 0x02, 0x02,

    /* CDC Union functional descriptor */
    5, 0x24, 0x06, 0x00, 0x01,

    /* EP1 IN (interrupt, CDC notifications — not used but required by spec) */
    7, 0x05, 0x81, 0x03, 0x08, 0x00, 0xFF,

    /* Interface 1: CDC Data */
    9, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,

    /* EP1 IN bulk — telemetry / responses to host */
    7, 0x05, 0x81, 0x02, 64, 0x00, 0x00,

    /* EP2 OUT bulk — commands from host */
    7, 0x05, 0x02, 0x02, 64, 0x00, 0x00,
};

/* String descriptor 0: language ID (English US) */
static const uint8_t str0_desc[] = { 4, 0x03, 0x09, 0x04 };

/* String helpers — UTF-16LE inline */
#define STR_DESC(s) { sizeof(s), 0x03, s }

static const uint8_t str1_desc[] = {
    20, 0x03,
    'D',0, 'S',0, 'R',0, '-',0, '1',0, ' ',0,
    'P',0, 'r',0, 'o',0
};
static const uint8_t str2_desc[] = {
    28, 0x03,
    'W',0, 'M',0, '-',0, 'D',0, '6',0, 'C',0, ' ',0,
    'S',0, 'e',0, 'r',0, 'v',0, 'o',0, ' ',0
};
static const uint8_t str3_desc[] = {
    10, 0x03,
    'v',0, '0',0, '.',0, '1',0
};

/* =========================================================================
 * Module state
 * ========================================================================= */

/* Receive ring buffer — filled by USB ISR, drained by usb_cdc_poll() */
#define RX_BUF_SIZE     256
static uint8_t  rx_buf[RX_BUF_SIZE];
static uint16_t rx_head = 0;   /* Written by ISR  */
static uint16_t rx_tail = 0;   /* Read by main loop */

/* Transmit buffer — filled by main loop, sent by usb_cdc_tx() */
#define TX_BUF_SIZE     256
static uint8_t  tx_buf[TX_BUF_SIZE];
static uint16_t tx_len  = 0;
static uint8_t  tx_busy = 0;   /* 1 while USB IN transfer is in progress */

/* Continuous telemetry mode flag ('T' command) */
static uint8_t telem_continuous = 0;

/* Boot status — set by usb_cdc_set_boot_status() before interrupts enabled,
 * read by usb_cdc_poll() to send a one-time banner on first connection. */
static FlashResult boot_flash_status = FLASH_OK;
static uint8_t     banner_pending    = 0;  /* Set to 1 when host connects */

/* USB device state */
static uint8_t usb_address   = 0;
static uint8_t usb_configured = 0;

/* =========================================================================
 * USB endpoint register helpers (STM32 USB EPnR registers are unusual)
 *
 * EPnR registers have toggle bits (DTOG, STAT) that are set by writing 1 —
 * writing 0 has no effect. To SET a STAT field to a target value, XOR the
 * current value with the target. Read-modify-write with care.
 * ========================================================================= */

#define EP_STAT_DISABLED    0x00
#define EP_STAT_STALL       0x01
#define EP_STAT_NAK         0x02
#define EP_STAT_VALID       0x03

/* Set TX status bits for endpoint n */
static void ep_set_tx_status(uint8_t ep, uint8_t status)
{
    uint16_t reg = USB->EP0R + ep;  /* EP0R, EP1R... are contiguous */
    /* Cast to EPnR register pointer */
    volatile uint16_t *epr = (volatile uint16_t *)(&USB->EP0R) + ep;
    uint16_t current = *epr;
    /* Toggle bits to reach target status; preserve non-toggle bits */
    uint16_t toggle = ((current >> 4) & 0x3) ^ status;
    *epr = (current & 0x8F8F) | (toggle << 4);
    (void)reg;
}

/* Set RX status bits for endpoint n */
static void ep_set_rx_status(uint8_t ep, uint8_t status)
{
    volatile uint16_t *epr = (volatile uint16_t *)(&USB->EP0R) + ep;
    uint16_t current = *epr;
    uint16_t toggle = ((current >> 12) & 0x3) ^ status;
    *epr = (current & 0x8F8F) | (toggle << 12);
}

/* =========================================================================
 * usb_cdc_init()
 *
 * Initialises HSI48 + CRS, USB clock, GPIO, and USB peripheral.
 * Call from main() after system clock init, before enabling interrupts.
 * ========================================================================= */
void usb_cdc_init(void)
{
    /* ---- HSI48 oscillator ---- */
    RCC->CR |= RCC_CR_HSI48ON;
    while (!(RCC->CR & RCC_CR_HSI48RDY)) { }

    /* ---- CRS: trim HSI48 using USB SOF packets as reference ---- */
    RCC->APBENR1 |= RCC_APBENR1_CRSEN;
    CRS->CFGR = (47999UL << CRS_CFGR_RELOAD_Pos)  /* 48000-1 ticks per SOF  */
              | (34UL    << CRS_CFGR_FELIM_Pos)    /* ~70ppm error limit     */
              | CRS_CFGR_SYNCSRC_1;                /* SYNCSRC=10: USB SOF    */
    CRS->CR = CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

    /* ---- USB clock source: HSI48 ---- */
    RCC->CCIPR = (RCC->CCIPR & ~RCC_CCIPR_CLK48SEL) | (3U << RCC_CCIPR_CLK48SEL_Pos);

    /* ---- GPIO: PA11 (USB D-), PA12 (USB D+) — USB alternate function ---- */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    GPIOA->MODER = (GPIOA->MODER & ~((3U << (11*2)) | (3U << (12*2))))
                 | (2U << (11*2)) | (2U << (12*2));   /* AF mode */
    /* AF10 for PA11/PA12 on STM32G0B1 (USB FS) — see DS13560 Table 12 */
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((0xFU << ((11-8)*4)) | (0xFU << ((12-8)*4))))
                  | (10U << ((11-8)*4)) | (10U << ((12-8)*4));

    /* ---- USB peripheral ---- */
    RCC->APBENR1 |= RCC_APBENR1_USBEN;

    /* Exit power-down, wait for reset sequence */
    USB->CNTR = USB_CNTR_FRES;           /* Force USB reset (FRES=1, PDWN=0)  */
    for (volatile int i = 0; i < 1000; i++) { }  /* Tspec ≥ 1µs */
    USB->CNTR = 0;                        /* Release reset */
    USB->ISTR = 0;                        /* Clear any pending flags */

    /* Buffer descriptor table at PMA offset 0 */
    USB->BTABLE = USB_BTABLE_ADDR;

    /* Enable USB reset and CTR (correct transfer) interrupts */
    USB->CNTR = USB_CNTR_RESETM | USB_CNTR_CTRM;

    /* Enable DP pull-up: signals to host that a full-speed device is connected */
    USB->BCDR |= USB_BCDR_DPPU;

    /* ---- NVIC: USB interrupt at priority 1 (below TIM2 servo at 0) ---- */
    NVIC_SetPriority(USB_UCPD1_2_IRQn, 1);
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);
}

/* =========================================================================
 * usb_ep0_setup() — handle SETUP packets on EP0 (enumeration)
 * ========================================================================= */

static void ep0_send(const uint8_t *data, uint16_t len, uint16_t requested_len)
{
    if (len > requested_len) len = requested_len;
    if (len > 64) len = 64;
    pma_write_buf(EP0_TX_ADDR, data, len);
    USB_BTABLE[0].COUNT_TX = len;
    ep_set_tx_status(0, EP_STAT_VALID);
}

static void ep0_stall(void)
{
    ep_set_tx_status(0, EP_STAT_STALL);
    ep_set_rx_status(0, EP_STAT_STALL);
}

static void usb_handle_setup(void)
{
    uint8_t setup[8];
    pma_read_buf(EP0_RX_ADDR, setup, 8);

    uint8_t  bmRequest = setup[0];
    uint8_t  bRequest  = setup[1];
    uint16_t wValue    = (uint16_t)(setup[2] | (setup[3] << 8));
    uint16_t wLength   = (uint16_t)(setup[6] | (setup[7] << 8));

    if ((bmRequest & 0x60) == 0x00) {
        /* Standard requests */
        switch (bRequest) {
        case 0x05: /* SET_ADDRESS */
            usb_address = wValue & 0x7F;
            USB_BTABLE[0].COUNT_TX = 0;
            ep_set_tx_status(0, EP_STAT_VALID);
            break;

        case 0x06: /* GET_DESCRIPTOR */
        {
            uint8_t dtype = (wValue >> 8) & 0xFF;
            uint8_t didx  = wValue & 0xFF;
            switch (dtype) {
            case 0x01: ep0_send(dev_desc,  sizeof(dev_desc),  wLength); break;
            case 0x02: ep0_send(cfg_desc,  sizeof(cfg_desc),  wLength); break;
            case 0x03:
                switch (didx) {
                case 0: ep0_send(str0_desc, sizeof(str0_desc), wLength); break;
                case 1: ep0_send(str1_desc, sizeof(str1_desc), wLength); break;
                case 2: ep0_send(str2_desc, sizeof(str2_desc), wLength); break;
                case 3: ep0_send(str3_desc, sizeof(str3_desc), wLength); break;
                default: ep0_stall(); break;
                }
                break;
            default: ep0_stall(); break;
            }
            break;
        }

        case 0x08: /* GET_CONFIGURATION */
        {
            uint8_t cfg = usb_configured ? 1 : 0;
            ep0_send(&cfg, 1, wLength);
            break;
        }

        case 0x09: /* SET_CONFIGURATION */
            usb_configured = (wValue == 1) ? 1 : 0;
            if (usb_configured) {
                banner_pending = 1;   /* Send boot status banner on next poll */

                /* Configure EP1 IN (bulk, CDC data in) */
                volatile uint16_t *ep1r = (volatile uint16_t *)(&USB->EP0R) + 1;
                *ep1r = (1U) | USB_EP_BULK;   /* EP number 1, bulk */
                USB_BTABLE[1].ADDR_TX  = EP1_TX_ADDR;
                USB_BTABLE[1].COUNT_TX = 0;
                ep_set_tx_status(1, EP_STAT_NAK);

                /* Configure EP2 OUT (bulk, CDC data out) */
                volatile uint16_t *ep2r = (volatile uint16_t *)(&USB->EP0R) + 2;
                *ep2r = (2U) | USB_EP_BULK;
                USB_BTABLE[2].ADDR_RX  = EP2_RX_ADDR;
                /* COUNT_RX: BL_SIZE=1 (32-byte blocks), NUM_BLOCK=1 → 64 bytes */
                USB_BTABLE[2].COUNT_RX = (1U << 15) | (1U << 10);
                ep_set_rx_status(2, EP_STAT_VALID);
            }
            USB_BTABLE[0].COUNT_TX = 0;
            ep_set_tx_status(0, EP_STAT_VALID);
            break;

        default:
            ep0_stall();
            break;
        }
    } else if ((bmRequest & 0x60) == 0x20) {
        /* Class requests (CDC ACM) — acknowledge but ignore line coding etc. */
        switch (bRequest) {
        case 0x20: /* SET_LINE_CODING — accept, ignore */
            ep_set_rx_status(0, EP_STAT_VALID);
            USB_BTABLE[0].COUNT_TX = 0;
            ep_set_tx_status(0, EP_STAT_VALID);
            break;
        case 0x21: /* GET_LINE_CODING — return 115200 8N1 */
        {
            static const uint8_t lc[] = {
                0x00, 0xC2, 0x01, 0x00,  /* dwDTERate: 115200 */
                0x00, 0x00, 0x08          /* bCharFormat=0, bParityType=0, bDataBits=8 */
            };
            ep0_send(lc, sizeof(lc), wLength);
            break;
        }
        case 0x22: /* SET_CONTROL_LINE_STATE — acknowledge */
            USB_BTABLE[0].COUNT_TX = 0;
            ep_set_tx_status(0, EP_STAT_VALID);
            break;
        default:
            ep0_stall();
            break;
        }
    } else {
        ep0_stall();
    }
}

/* =========================================================================
 * USB_IRQHandler — USB interrupt service routine
 *
 * Handles USB reset, and correct-transfer events on EP0, EP1, EP2.
 * Runs at NVIC priority 1 — preempted only by TIM2 servo ISR (priority 0).
 * ========================================================================= */
void USB_UCPD1_2_IRQHandler(void)
{
    uint16_t istr = USB->ISTR;

    /* ---- USB bus reset ---- */
    if (istr & USB_ISTR_RESET) {
        USB->ISTR = ~USB_ISTR_RESET;
        usb_address   = 0;
        usb_configured = 0;
        tx_busy = 0;

        /* Configure EP0 for control transfers */
        volatile uint16_t *ep0r = (volatile uint16_t *)(&USB->EP0R);
        *ep0r = USB_EP_CONTROL;   /* EP0, control type */

        USB_BTABLE[0].ADDR_TX  = EP0_TX_ADDR;
        USB_BTABLE[0].COUNT_TX = 0;
        USB_BTABLE[0].ADDR_RX  = EP0_RX_ADDR;
        USB_BTABLE[0].COUNT_RX = (1U << 15) | (1U << 10);  /* 64-byte RX buffer */

        ep_set_tx_status(0, EP_STAT_NAK);
        ep_set_rx_status(0, EP_STAT_VALID);

        USB->DADDR = USB_DADDR_EF;  /* Enable function, address 0 */
    }

    /* ---- Correct transfer ---- */
    while (USB->ISTR & USB_ISTR_CTR) {
        uint8_t ep_id  = USB->ISTR & USB_ISTR_EP_ID;
        uint8_t is_out = (USB->ISTR & USB_ISTR_DIR) ? 1 : 0;

        volatile uint16_t *epr = (volatile uint16_t *)(&USB->EP0R) + ep_id;

        if (ep_id == 0) {
            if (is_out) {
                /* EP0 OUT: SETUP or DATA OUT */
                if (*epr & USB_EP_SETUP) {
                    usb_handle_setup();
                } else {
                    /* DATA OUT stage (e.g. SET_LINE_CODING body) — ignore */
                    USB_BTABLE[0].COUNT_TX = 0;
                    ep_set_tx_status(0, EP_STAT_VALID);
                }
                /* Clear RX flag, re-arm */
                *epr &= ~USB_EP_CTR_RX & 0x8F8F;
                ep_set_rx_status(0, EP_STAT_VALID);
            } else {
                /* EP0 IN: STATUS stage or descriptor sent */
                *epr &= ~USB_EP_CTR_TX & 0x8F8F;
                /* Apply deferred address after STATUS IN */
                if (usb_address) {
                    USB->DADDR = USB_DADDR_EF | usb_address;
                    usb_address = 0;
                }
                ep_set_rx_status(0, EP_STAT_VALID);
            }
        } else if (ep_id == 1 && !is_out) {
            /* EP1 IN: bulk transfer to host complete */
            *epr &= ~USB_EP_CTR_TX & 0x8F8F;
            tx_busy = 0;
        } else if (ep_id == 2 && is_out) {
            /* EP2 OUT: received data from host */
            uint16_t cnt = USB_BTABLE[2].COUNT_RX & 0x3FF;
            uint8_t  tmp[64];
            if (cnt > 64) cnt = 64;
            pma_read_buf(EP2_RX_ADDR, tmp, cnt);

            /* Push received bytes into rx_buf ring buffer */
            for (uint16_t i = 0; i < cnt; i++) {
                uint16_t next = (rx_head + 1) & (RX_BUF_SIZE - 1);
                if (next != rx_tail) {
                    rx_buf[rx_head] = tmp[i];
                    rx_head = next;
                }
            }

            *epr &= ~USB_EP_CTR_RX & 0x8F8F;
            ep_set_rx_status(2, EP_STAT_VALID);
        }
    }

    USB->ISTR = 0;
}

/* =========================================================================
 * usb_cdc_tx() — send bytes to host
 *
 * Copies data to TX buffer and starts an IN transfer on EP1.
 * Returns immediately if a transfer is already in progress.
 * Main loop only — not safe from ISR.
 * ========================================================================= */
static void usb_cdc_tx(const uint8_t *data, uint16_t len)
{
    if (!usb_configured || tx_busy) return;
    if (len > 64) len = 64;
    pma_write_buf(EP1_TX_ADDR, data, len);
    USB_BTABLE[1].COUNT_TX = len;
    tx_busy = 1;
    ep_set_tx_status(1, EP_STAT_VALID);
}

/* =========================================================================
 * Telemetry formatter
 *
 * Formats a telemetry line into a static buffer and sends it.
 * Uses integer-only arithmetic (no printf, no float) to keep the
 * telemetry path fast and flash-efficient.
 *
 * Output format (all on one line, newline terminated):
 *   FG:25612 Hz:2499 Err:+12 Int:-847 DAC:2034 Kp:9830(0.150) Ki:524(0.008)
 *   RV1:2048 RV2:1923 RV3:2100
 * ========================================================================= */

/* Minimal integer-to-decimal formatter — no stdlib required */
static uint8_t *fmt_u32(uint8_t *p, uint32_t v)
{
    uint8_t tmp[10];
    int i = 0;
    if (v == 0) { *p++ = '0'; return p; }
    while (v) { tmp[i++] = '0' + (v % 10); v /= 10; }
    while (i--) *p++ = tmp[i];
    return p;
}

static uint8_t *fmt_i32(uint8_t *p, int32_t v)
{
    if (v < 0) { *p++ = '-'; v = -v; }
    else        { *p++ = '+'; }
    return fmt_u32(p, (uint32_t)v);
}

/* Format Q16 value as "NNNNN(d.ddd)" */
static uint8_t *fmt_q16(uint8_t *p, int32_t q16)
{
    p = fmt_u32(p, (uint32_t)q16);
    *p++ = '(';
    uint32_t whole = (uint32_t)q16 >> 16;        /* integer part */
    uint32_t frac  = ((uint32_t)q16 & 0xFFFF) * 1000 / 65536; /* 3 decimal places */
    p = fmt_u32(p, whole);
    *p++ = '.';
    /* Pad fraction to 3 digits */
    if (frac < 100) *p++ = '0';
    if (frac < 10)  *p++ = '0';
    p = fmt_u32(p, frac);
    *p++ = ')';
    return p;
}

/* Compute FG frequency in Hz from period ticks: Hz = 64000000 / ticks */
static uint32_t ticks_to_hz(uint32_t ticks)
{
    if (ticks == 0) return 0;
    return 64000000UL / ticks;
}

static void send_telemetry(void)
{
    static uint8_t buf[128];
    uint8_t *p = buf;

    /* Snapshot telemetry — volatile reads are atomic (32-bit aligned) */
    uint32_t fg    = g_telemetry.fg_period_ticks;
    uint32_t tgt   = g_telemetry.target_period;
    int32_t  err   = g_telemetry.error;
    int32_t  integ = g_telemetry.integral;
    uint16_t dac   = g_telemetry.dac_value;
    int32_t  kp    = g_kp_q16;
    int32_t  ki    = g_ki_q16;
    uint16_t rv1   = g_adc_raw[ADC_IDX_RV601];
    uint16_t rv2   = g_adc_raw[ADC_IDX_RV602];
    uint16_t rv3   = g_adc_raw[ADC_IDX_RV603];

    /* Line 1: servo state */
    #define APPEND(s) do { const char *_s = s; while (*_s) *p++ = (uint8_t)*_s++; } while(0)
    APPEND("FG:"); p = fmt_u32(p, fg);
    APPEND(" Hz:"); p = fmt_u32(p, ticks_to_hz(fg));
    APPEND(" Tgt:"); p = fmt_u32(p, tgt);
    APPEND(" Err:"); p = fmt_i32(p, err);
    APPEND(" Int:"); p = fmt_i32(p, integ);
    APPEND(" DAC:"); p = fmt_u32(p, dac);
    APPEND(" Kp:"); p = fmt_q16(p, kp);
    APPEND(" Ki:"); p = fmt_q16(p, ki);
    APPEND(" RV1:"); p = fmt_u32(p, rv1);
    APPEND(" RV2:"); p = fmt_u32(p, rv2);
    APPEND(" RV3:"); p = fmt_u32(p, rv3);
    *p++ = '\r'; *p++ = '\n';

    usb_cdc_tx(buf, (uint16_t)(p - buf));
}

/* =========================================================================
 * Command dispatcher
 *
 * Called from usb_cdc_poll() for each byte received from host.
 * Two-character commands (p+, p-, i+, i-, f+, f-) use a one-byte look-ahead.
 * ========================================================================= */

static uint8_t cmd_prefix = 0;  /* Holds 'p', 'i', or 'f' waiting for +/- */

static void dispatch_command(uint8_t c)
{
    static uint8_t response[32];
    uint8_t *rp = response;

    if (cmd_prefix) {
        /* Second byte of a two-character command */
        uint8_t prefix = cmd_prefix;
        cmd_prefix = 0;

        if (prefix == 'p') {
            if (c == '+') {
                int32_t kp = g_kp_q16 + KP_Q16_STEP;
                if (kp > KP_Q16_MAX) kp = KP_Q16_MAX;
                g_kp_q16 = kp;
                APPEND("Kp+\r\n");
            } else if (c == '-') {
                int32_t kp = g_kp_q16 - KP_Q16_STEP;
                if (kp < KP_Q16_MIN) kp = KP_Q16_MIN;
                g_kp_q16 = kp;
                APPEND("Kp-\r\n");
            }
        } else if (prefix == 'i') {
            if (c == '+') {
                int32_t ki = g_ki_q16 + KI_Q16_STEP;
                if (ki > KI_Q16_MAX) ki = KI_Q16_MAX;
                g_ki_q16 = ki;
                APPEND("Ki+\r\n");
            } else if (c == '-') {
                int32_t ki = g_ki_q16 - KI_Q16_STEP;
                if (ki < KI_Q16_MIN) ki = KI_Q16_MIN;
                g_ki_q16 = ki;
                APPEND("Ki-\r\n");
            }
        } else if (prefix == 'f') {
            if (c == '+') {
                uint32_t t = g_target_period;
                if (t < ADJUSTED_TARGET_MAX) g_target_period = t + 1;
                APPEND("f+\r\n");
            } else if (c == '-') {
                uint32_t t = g_target_period;
                if (t > ADJUSTED_TARGET_MIN) g_target_period = t - 1;
                APPEND("f-\r\n");
            }
        }

        if (rp > response) usb_cdc_tx(response, (uint16_t)(rp - response));
        return;
    }

    /* First byte */
    switch (c) {
    case 'p':
    case 'i':
    case 'f':
        cmd_prefix = c;   /* Wait for +/- */
        break;

    case 't':   /* Telemetry snapshot */
        send_telemetry();
        break;

    case 'T':   /* Toggle continuous telemetry */
        telem_continuous ^= 1;
        APPEND(telem_continuous ? "Telem ON\r\n" : "Telem OFF\r\n");
        usb_cdc_tx(response, (uint16_t)(rp - response));
        break;

    case 's':   /* Save to flash */
    {
        FlashResult res = flash_save();
        if (res == FLASH_OK) {
            APPEND("Saved\r\n");
        } else {
            APPEND("Save ERR\r\n");
        }
        usb_cdc_tx(response, (uint16_t)(rp - response));
        break;
    }

    case 'r':   /* Reset to defaults (SRAM only) */
        flash_reset_defaults();
        APPEND("Defaults\r\n");
        usb_cdc_tx(response, (uint16_t)(rp - response));
        break;

    case '?':   /* Help */
    {
        static const char help[] =
            "DSR-1 v0.1 commands:\r\n"
            " p+/p- Kp +/-0.01\r\n"
            " i+/i- Ki +/-0.001\r\n"
            " f+/f- target period +/-1 tick\r\n"
            " t     telemetry snapshot\r\n"
            " T     toggle continuous telemetry\r\n"
            " s     save to flash\r\n"
            " r     reset to defaults (no flash write)\r\n"
            " ?     this help\r\n";
        usb_cdc_tx((const uint8_t *)help, sizeof(help) - 1);
        break;
    }

    default:
        /* Unknown command — cancel any pending prefix */
        cmd_prefix = 0;
        telem_continuous = 0;   /* Any other key stops continuous telemetry */
        break;
    }
}

/* =========================================================================
 * send_banner()
 *
 * Sent once to the host immediately after enumeration completes.
 * Reports firmware version and the flash load result so the user knows
 * immediately whether their saved calibration was loaded or defaults are
 * running. A corrupt or missing settings block is not silent.
 * ========================================================================= */
static void send_banner(void)
{
    static const char ok_banner[] =
        "\r\nDSR-1 v0.1 — WM-D6C Servo Replacement\r\n"
        "Settings: loaded from flash\r\n"
        "Type ? for commands\r\n";

    static const char warn_banner[] =
        "\r\nDSR-1 v0.1 — WM-D6C Servo Replacement\r\n"
        "*** WARNING: Flash settings missing or corrupt — running defaults ***\r\n"
        "*** Calibrate and type 's' to save before use                     ***\r\n"
        "Type ? for commands\r\n";

    if (boot_flash_status == FLASH_OK) {
        usb_cdc_tx((const uint8_t *)ok_banner, sizeof(ok_banner) - 1);
    } else {
        usb_cdc_tx((const uint8_t *)warn_banner, sizeof(warn_banner) - 1);
    }
}

/* =========================================================================
 * usb_cdc_set_boot_status() — called from main() before __enable_irq().
 * Stores the flash load result for inclusion in the connection banner.
 * ========================================================================= */
void usb_cdc_set_boot_status(FlashResult status)
{
    boot_flash_status = status;
}

/* =========================================================================
 * usb_cdc_idle() — returns 1 if there is no pending work to do.
 *
 * Called from the main loop to decide whether __WFI() is safe. Returns 0
 * (not idle) if there are bytes in the receive buffer, a banner is pending,
 * or continuous telemetry is active and TX is free. The CPU should not
 * sleep in any of these cases — it would miss or delay processing.
 * ========================================================================= */
int usb_cdc_idle(void)
{
    if (!usb_configured)                    return 1;  /* No host — sleep is fine */
    if (rx_tail != rx_head)                 return 0;  /* Bytes waiting to drain   */
    if (banner_pending && !tx_busy)         return 0;  /* Banner queued            */
    if (telem_continuous && !tx_busy)       return 0;  /* Telemetry to send        */
    return 1;
}

/* =========================================================================
 * usb_cdc_poll() — main loop service function
 *
 * Call from the main loop as frequently as possible.
 * Drains the receive ring buffer and dispatches commands.
 * Sends the boot banner on first connection.
 * Sends one telemetry line if continuous mode is active and TX is free.
 * ========================================================================= */
void usb_cdc_poll(void)
{
    if (!usb_configured) return;

    /* Send boot status banner on first connection — once only */
    if (banner_pending && !tx_busy) {
        banner_pending = 0;
        send_banner();
        return;  /* Let the TX complete before draining commands */
    }

    /* Drain receive buffer */
    while (rx_tail != rx_head) {
        uint8_t c = rx_buf[rx_tail];
        rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
        dispatch_command(c);
    }

    /* Continuous telemetry — one line per poll if TX is free */
    if (telem_continuous && !tx_busy) {
        send_telemetry();
    }
}
