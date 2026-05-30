/**
 * adc.c — DSR-1 Speed Trim Potentiometer ADC Scan
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 * Hardware: STM32G0B1KBU6, bare-metal, no HAL, no RTOS
 *
 * Peripherals owned by this file:
 *   ADC1   — 3-channel scan: PA1 (IN1/RV601), PA2 (IN2/RV602), PA3 (IN3/RV603)
 *   DMA    — channel 1, continuous circular transfer from ADC DR to g_adc_raw[]
 *   PA7    — GPIO input, S601 Speed Tune enable switch
 *
 * Operation:
 *   ADC runs in continuous + DMA circular mode. Hardware fills g_adc_raw[]
 *   autonomously. The servo ISR (servo.c) calls adc_get_adjusted_target()
 *   each execution to read the current pot values and compute the adjusted
 *   target period. No interrupt needed — the main loop and servo ISR simply
 *   read g_adc_raw[] whenever they need current values.
 *
 *   Three-channel scan completes in approximately 11µs at maximum sampling
 *   time (239.5 cycles per channel at 64MHz ADC clock).
 *   See signal-chain-analysis.md §4.3 for derivation.
 *
 * ADC channel mapping:
 *   ADC_IN1 (PA1) → g_adc_raw[ADC_IDX_RV601] — base speed trim
 *   ADC_IN2 (PA2) → g_adc_raw[ADC_IDX_RV602] — Speed Tune slider (user)
 *   ADC_IN3 (PA3) → g_adc_raw[ADC_IDX_RV603] — Speed Tune range scalar
 *
 * WARNING: Pot wiper voltages MUST be verified on the bench before connecting
 * to the STM32. If any wiper exceeds 3.3V, voltage dividers must be added
 * before the ADC inputs. Exceeding VDD+0.3V permanently damages the ADC.
 * See signal-chain-analysis.md §4.2.
 */

#include "stm32g0xx.h"
#include <stdint.h>
#include "config.h"
#include "adc.h"

/* -------------------------------------------------------------------------
 * DMA result buffer
 *
 * Declared volatile — DMA writes this without CPU involvement; the CPU
 * must not cache or reorder reads from this array.
 * Aligned to 4 bytes for efficient 16-bit DMA transfers.
 * ------------------------------------------------------------------------- */
volatile uint16_t g_adc_raw[ADC_CHANNEL_COUNT];

/* -------------------------------------------------------------------------
 * adc_init()
 *
 * Configures GPIO, DMA, and ADC1 for continuous circular scan.
 * Call once from main() after clock init, before servo_init().
 * The ADC starts converting immediately on return.
 * ------------------------------------------------------------------------- */
void adc_init(void)
{
    /* ---- GPIO: PA1, PA2, PA3 analog inputs; PA7 digital input (S601) ---- */

    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    /* PA1/PA2/PA3: analog mode (MODER = 11), no pull */
    GPIOA->MODER  |=  (3U << (1*2)) | (3U << (2*2)) | (3U << (3*2));
    GPIOA->PUPDR  &= ~((3U << (1*2)) | (3U << (2*2)) | (3U << (3*2)));

    /* PA7: input mode (MODER = 00), pull-down.
     * S601 connects PA7 to VDD (3.3V) when Speed Tune is enabled.
     * Pull-down ensures a defined LOW state when switch is open. */
    GPIOA->MODER  &= ~(3U << (7*2));   /* Input mode */
    GPIOA->PUPDR   = (GPIOA->PUPDR & ~(3U << (7*2))) | (2U << (7*2)); /* Pull-down */

    /* ---- DMA: channel 1, ADC1 → g_adc_raw[], circular ---- */

    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    /* DMAMUX channel 0: request source = ADC1 (request ID = 5 on STM32G0B1) */
    DMAMUX1_Channel0->CCR = 5U;   /* ADC1 DMA request */

    DMA1_Channel1->CCR = 0;       /* Disable channel before configuring */

    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;          /* Source: ADC data reg   */
    DMA1_Channel1->CMAR  = (uint32_t)g_adc_raw;           /* Dest: result buffer    */
    DMA1_Channel1->CNDTR = ADC_CHANNEL_COUNT;              /* 3 transfers per cycle  */

    DMA1_Channel1->CCR =
        DMA_CCR_CIRC    |   /* Circular mode — restarts automatically            */
        DMA_CCR_MINC    |   /* Memory increment — advance through g_adc_raw[]   */
        DMA_CCR_PSIZE_0 |   /* Peripheral size: 16-bit (ADC DR is 16-bit)        */
        DMA_CCR_MSIZE_0 |   /* Memory size: 16-bit (uint16_t array)              */
        DMA_CCR_EN;         /* Enable channel                                    */

    /* ---- ADC1: continuous scan, 3 channels, DMA circular ---- */

    RCC->APBENR2 |= RCC_APBENR2_ADCEN;

    /* ADC voltage regulator startup: must wait ≥20µs after enabling ADVREGEN.
     * At 64MHz a simple spin loop is sufficient. */
    ADC1->CR = ADC_CR_ADVREGEN;
    for (volatile uint32_t i = 0; i < 1280; i++) { __asm__("nop"); } /* ≥20µs */

    /* Self-calibration — must be done before enabling ADC.
     * Wait for ADCAL to clear (hardware clears when done). */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }

    /* Configure ADC clock: use PCLK/4 = 64MHz/4 = 16MHz (within spec) */
    ADC1->CFGR2 = (2U << ADC_CFGR2_CKMODE_Pos);   /* CKMODE = 10: PCLK/4 */

    /* Sampling time: 239.5 cycles for all channels.
     * Required for high-impedance pot sources (up to ~24kΩ source impedance).
     * See signal-chain-analysis.md §4.2 for derivation. */
    ADC1->SMPR = ADC_SMPR_SMP1_2 | ADC_SMPR_SMP1_1 | ADC_SMPR_SMP1_0; /* 111 = 239.5 cycles */

    /* Channel sequence: IN1, IN2, IN3 (scanned in this order into g_adc_raw) */
    ADC1->CHSELR = ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 | ADC_CHSELR_CHSEL3;

    /* Oversampling: 16× with right-shift 4 — improves effective resolution
     * from 12-bit to ~14-bit at the cost of reduced sample rate.
     * Effective rate per channel: (16MHz / (239.5+12.5) / 3) / 16 ≈ 1300 Hz.
     * More than adequate for pot wiper tracking. */
    ADC1->CFGR2 |= ADC_CFGR2_OVSE                   /* Oversampling enable    */
               |  (3U << ADC_CFGR2_OVSR_Pos)         /* OVSR = 011: 16× ratio  */
               |  (4U << ADC_CFGR2_OVSS_Pos);        /* OVSS = 0100: >>4 shift */

    /* Main configuration:
     *   CONT  — continuous conversion mode
     *   DMACFG — DMA circular mode (matches DMA_CCR_CIRC)
     *   DMAEN — enable DMA requests from ADC
     *   RES   — 12-bit resolution (default, field = 00) */
    ADC1->CFGR1 = ADC_CFGR1_CONT | ADC_CFGR1_DMACFG | ADC_CFGR1_DMAEN;

    /* Enable ADC */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }   /* Wait for ADC ready */

    /* Start continuous conversion — runs forever, DMA keeps g_adc_raw[] fresh */
    ADC1->CR |= ADC_CR_ADSTART;
}

/* -------------------------------------------------------------------------
 * adc_get_adjusted_target()
 *
 * Reads current ADC values from g_adc_raw[] and computes the adjusted
 * target period for the servo loop, incorporating all three pot corrections.
 *
 * Called from servo ISR (TIM2_IRQHandler) on each FG edge.
 * Must be fast — reads volatile array, integer arithmetic only, no division
 * in the hot path (divisions are by constants the compiler can optimise).
 *
 * Returns: adjusted target period in 64MHz ticks, clamped to safe range.
 * ------------------------------------------------------------------------- */
uint32_t adc_get_adjusted_target(void)
{
    /* Snapshot current ADC values — volatile reads, but no sync needed.
     * Worst case: we read a value mid-update, off by one conversion cycle.
     * At ~1300 Hz update rate this is an error of ~0.8ms — completely
     * negligible for pot-based speed trim. */
    int32_t rv601 = (int32_t)g_adc_raw[ADC_IDX_RV601];
    int32_t rv602 = (int32_t)g_adc_raw[ADC_IDX_RV602];
    int32_t rv603 = (int32_t)g_adc_raw[ADC_IDX_RV603];

    /* RV601: base speed trim — signed offset centred at ADC mid-scale (2048).
     * Range: ±BASE_TRIM_SCALE ticks at full pot deflection. */
    int32_t rv601_trim = ((rv601 - 2048) * BASE_TRIM_SCALE) / 2048;

    /* RV603: speed range scalar — unsigned, 0 to SPEED_RANGE_MAX ticks.
     * Sets the sensitivity of the RV602 Speed Tune slider. */
    int32_t rv603_range = (rv603 * SPEED_RANGE_MAX) / 4095;

    /* RV602: Speed Tune slider — signed offset centred at mid-scale,
     * scaled by RV603. Only applied if S601 switch is HIGH (PA7 = 1). */
    int32_t rv602_offset = 0;
    if (GPIOA->IDR & (1U << 7)) {
        rv602_offset = ((rv602 - 2048) * rv603_range) / 2048;
    }

    /* Combine base target with all corrections */
    int32_t adjusted = (int32_t)g_target_period + rv601_trim + rv602_offset;

    /* Clamp to safe range — prevents integral windup if a pot is disconnected
     * or drives the target to a physically unreachable value. */
    if (adjusted < (int32_t)ADJUSTED_TARGET_MIN) adjusted = (int32_t)ADJUSTED_TARGET_MIN;
    if (adjusted > (int32_t)ADJUSTED_TARGET_MAX) adjusted = (int32_t)ADJUSTED_TARGET_MAX;

    return (uint32_t)adjusted;
}
