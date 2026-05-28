/**
 * main.c — DSR-1 Startup, Clock Initialisation, and Main Loop
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 * Hardware: STM32G0B1KBU6, bare-metal, no HAL, no RTOS
 *
 * Boot sequence (total time to servo loop: ~2ms):
 *
 *   t = 0        Power-on reset released, execution begins at reset vector
 *   t = ~0.3ms   clock_init(): HSI16 → PLL → 64 MHz system clock
 *   t = ~0.3ms   flash_load(): load saved settings from flash (or use defaults)
 *   t = ~0.5ms   adc_init(): GPIO + DMA + ADC1 continuous scan starts
 *   t = ~0.5ms   servo_init(): TIM2 input capture + DAC + NVIC configured
 *   t = ~0.5ms   usb_cdc_init(): HSI48 + CRS + USB peripheral, DP pull-up asserted
 *   t = ~0.5ms   __enable_irq(): all interrupts unmasked
 *   t = ~2ms     First FG edge arrives, TIM2_IRQHandler fires, servo loop running
 *   t = ~300ms   Motor reaches operating speed, servo locked
 *
 * Main loop:
 *   Runs at maximum speed after interrupt overhead (~99.9% idle).
 *   Only responsibilities: service USB CDC (usb_cdc_poll) and execute WFI
 *   to yield CPU while waiting for the next interrupt.
 *   The servo loop runs entirely in TIM2_IRQHandler — the main loop never
 *   touches servo state directly.
 *
 * Interrupt priority hierarchy (see why-bare-metal.md §5.1):
 *   Priority 0 (highest): TIM2 capture — servo ISR, non-negotiable
 *   Priority 1:           USB — CDC data, enumeration
 *   Priority 2 (lowest):  SysTick (if used — currently unused)
 *
 * Motor safety at all times:
 *   PA4 DAC is pre-loaded to DAC_CENTER before any interrupt fires.
 *   R6 (100kΩ pullup to VDD) holds Q601 base at 3.3V during reset/boot,
 *   which reverse-biases Q601 (emitter at 10.8V), keeping motor off.
 *   The motor cannot run uncontrolled at any point in the boot sequence.
 */

#include <stdint.h>
#include "config.h"
#include "servo.h"
#include "adc.h"
#include "flash.h"
#include "usb_cdc.h"

/* Boot status flags — set during init, reported via USB CDC on first connection.
 * Passed explicitly to usb_cdc_set_boot_status() after USB init so the CDC
 * layer can report it as a banner when the host first connects. */

/* =========================================================================
 * clock_init()
 *
 * Brings the system clock from the power-on default (HSI16, 16 MHz) to
 * the target (HSI16 × PLL, 64 MHz).
 *
 * PLL configuration: HSI16 → PLLM=/1 → 16 MHz → PLLN=×8 → 128 MHz VCO
 *                    → PLLR=/2 → 64 MHz SYSCLK
 *
 * Flash latency must be set to 2 wait states BEFORE switching to 64 MHz.
 * Switching first then setting latency risks a read error from the flash
 * controller at the higher clock frequency.
 *
 * See usb-crystalless-operation.md §3.2 for full derivation.
 * ========================================================================= */
static void clock_init(void)
{
    /* Step 1: Ensure HSI16 is on and stable (it is the default on reset,
     * but be explicit to handle any edge case where it was disabled) */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    /* Step 2: Configure PLL — must be done while PLL is off */
    RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI          /* Source: HSI16       */
                 | (0U << RCC_PLLCFGR_PLLM_Pos)     /* PLLM = /1           */
                 | (8U << RCC_PLLCFGR_PLLN_Pos)     /* PLLN = ×8 (VCO=128MHz) */
                 | (1U << RCC_PLLCFGR_PLLR_Pos)     /* PLLR = /2 → 64 MHz */
                 | RCC_PLLCFGR_PLLREN;              /* Enable PLLR output  */

    /* Step 3: Enable PLL and wait for lock */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    /* Step 4: Set flash latency for 64 MHz BEFORE switching clock source.
     * At 64 MHz (VDD 3.3V, Range 1): 2 wait states required (RM0444 §3.3.4).
     * Enable prefetch and instruction cache for best performance. */
    FLASH->ACR = (2U << FLASH_ACR_LATENCY_Pos)
               | FLASH_ACR_PRFTEN
               | FLASH_ACR_ICEN;
    /* Verify latency was accepted before proceeding */
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != (2U << FLASH_ACR_LATENCY_Pos)) { }

    /* Step 5: Switch system clock to PLLR output */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_1;  /* SW = 10: PLLRCLK */
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_1) { } /* Wait for switch */

    /* AHB prescaler = /1 (HCLK = SYSCLK = 64 MHz) — default, explicit for clarity */
    RCC->CFGR &= ~RCC_CFGR_HPRE;

    /* APB prescaler = /1 (PCLK = HCLK = 64 MHz) — default */
    RCC->CFGR &= ~RCC_CFGR_PPRE;
}

/* =========================================================================
 * main()
 * ========================================================================= */
int main(void)
{
    /* ---- 1. Clock init — must be first ---- */
    clock_init();

    /* ---- 2. Load settings from flash ----
     * Done before any peripheral init so that g_kp_q16, g_ki_q16, and
     * g_target_period hold the correct values when servo_init() runs.
     * If the flash block is absent or corrupt, compiled-in defaults from
     * config.h are used — but we capture the result so the USB CDC layer
     * can warn the user via the connection banner.
     * flash_load() is a read-only operation — no erase/write at this point.
     * We intentionally do NOT auto-save defaults on a corrupt block — the
     * user should verify settings via USB CDC before committing to flash. */
    FlashResult flash_status = flash_load();

    /* ---- 3. ADC init — before servo_init() ----
     * The ADC DMA scan starts immediately. By the time the first FG edge
     * arrives (~2ms), at least one complete 3-channel conversion has
     * completed and g_adc_raw[] holds valid pot readings. */
    adc_init();

    /* ---- 4. Servo init ----
     * Configures TIM2 input capture, DAC output (pre-loaded to DAC_CENTER),
     * and registers TIM2 in NVIC at priority 0.
     * The TIM2 ISR will not fire until __enable_irq() below. */
    servo_init();

    /* ---- 5. USB CDC init ----
     * Configures HSI48, CRS, USB peripheral, and GPIO for PA11/PA12.
     * Asserts DP pull-up — the host will begin enumeration as soon as
     * __enable_irq() unmasks the USB interrupt.
     * USB interrupt registered in NVIC at priority 1. */
    usb_cdc_init();

    /* Pass flash load result to USB CDC so it can report it in the
     * connection banner. Done after usb_cdc_init() but before
     * __enable_irq() — no race possible yet. */
    usb_cdc_set_boot_status(flash_status);

    /* ---- 6. Enable global interrupts ----
     * From this point:
     *   - TIM2 ISR fires on every FG rising edge (servo loop running)
     *   - USB ISR fires on enumeration and CDC data transfers
     * Order of steps 3–5 above ensures all peripherals are fully configured
     * before any interrupt can fire. */
    __enable_irq();

    /* ---- 7. Main loop ----
     * The servo loop runs entirely in TIM2_IRQHandler. The main loop has
     * one job: service USB CDC. Everything else is interrupt-driven.
     *
     * WFI behaviour:
     *   We only call __WFI() when there is genuinely nothing pending —
     *   no bytes in the receive buffer and continuous telemetry is off.
     *   This prevents the pathological case where an interrupt wakes the
     *   CPU, its handler queues data into rx_buf[], the handler returns,
     *   and the CPU immediately sleeps again before usb_cdc_poll() has
     *   had a chance to drain the buffer.
     *
     *   If there is pending work (bytes to drain or telemetry to send),
     *   the loop spins at full speed. When truly idle, WFI reduces current
     *   from ~9mA to ~2mA. TIM2 input capture is hardware-triggered and
     *   wakes the CPU within one clock cycle regardless of sleep state. */
    for (;;) {
        usb_cdc_poll();

        /* Only sleep if there is genuinely nothing pending */
        if (usb_cdc_idle()) {
            __WFI();
        }
    }

    /* Unreachable — suppress compiler warning */
    return 0;
}
