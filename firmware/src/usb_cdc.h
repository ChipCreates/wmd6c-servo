/**
 * usb_cdc.h — DSR-1 USB CDC Virtual COM Port — Public Interface
 *
 * Include in main.c only.
 * servo.c, adc.c, and flash.c are called directly from within usb_cdc.c
 * and do not need to know about the USB layer.
 */

#ifndef USB_CDC_H
#define USB_CDC_H

#include "flash.h"   /* for FlashResult */

/**
 * usb_cdc_init() — configure HSI48, CRS, GPIO, and USB peripheral.
 * Call once from main() after clock init, before enabling global interrupts.
 * The USB peripheral starts and will enumerate when a host is connected.
 */
void usb_cdc_init(void);

/**
 * usb_cdc_set_boot_status() — store the flash load result for the banner.
 * Call after usb_cdc_init() and before __enable_irq().
 * @status: result of flash_load() — FLASH_OK or FLASH_ERR_INVALID.
 */
void usb_cdc_set_boot_status(FlashResult status);

/**
 * usb_cdc_idle() — returns 1 if there is no pending USB work.
 * Call from main loop to determine whether __WFI() is safe.
 * Returns 0 if the receive buffer has unprocessed bytes, a banner is
 * pending, or continuous telemetry is active and TX is free.
 */
int usb_cdc_idle(void);

/**
 * usb_cdc_poll() — service the USB CDC interface from the main loop.
 * Sends the boot banner on first connection, drains the receive ring
 * buffer, dispatches commands, and sends queued telemetry.
 * Call as frequently as possible from main loop.
 * Not safe to call from ISR context.
 */
void usb_cdc_poll(void);

#endif /* USB_CDC_H */
