/**
 * adc.h — DSR-1 Speed Trim ADC — Public Interface
 *
 * Include in servo.c (calls adc_get_adjusted_target from ISR),
 * main.c (calls adc_init), and usb_cdc.c (reads g_adc_raw for telemetry).
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "config.h"   /* for g_target_period, BASE_TRIM_SCALE, etc. */
#include "servo.h"    /* for g_target_period */

/* -------------------------------------------------------------------------
 * DMA result buffer indices
 * Order matches the ADC channel scan sequence configured in adc_init().
 * ------------------------------------------------------------------------- */
#define ADC_CHANNEL_COUNT   3
#define ADC_IDX_RV601       0   /* PA1 / ADC_IN1 — base speed trim    */
#define ADC_IDX_RV602       1   /* PA2 / ADC_IN2 — Speed Tune slider  */
#define ADC_IDX_RV603       2   /* PA3 / ADC_IN3 — Speed Tune range   */

/* -------------------------------------------------------------------------
 * DMA result buffer — written by DMA, read by servo ISR and main loop.
 * Values are 12-bit ADC results (0–4095) after 16× oversampling + >>4.
 * Declared extern here; defined in adc.c.
 * ------------------------------------------------------------------------- */
extern volatile uint16_t g_adc_raw[ADC_CHANNEL_COUNT];

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/**
 * adc_init() — configure GPIO, DMA, and ADC1 for continuous circular scan.
 * Call once from main() before servo_init().
 * ADC starts converting immediately on return; g_adc_raw[] is valid after
 * the first scan completes (~11µs).
 */
void adc_init(void);

/**
 * adc_get_adjusted_target() — compute servo target period from pot readings.
 *
 * Reads g_adc_raw[], applies RV601 base trim, RV603-scaled RV602 offset,
 * and S601 Speed Tune enable, then clamps to [ADJUSTED_TARGET_MIN,
 * ADJUSTED_TARGET_MAX].
 *
 * Called from TIM2_IRQHandler (servo ISR) on each FG edge.
 * Integer arithmetic only — safe to call from ISR context.
 *
 * @returns adjusted target period in 64 MHz clock ticks.
 */
uint32_t adc_get_adjusted_target(void);

#endif /* ADC_H */
