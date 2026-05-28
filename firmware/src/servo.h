/**
 * servo.h — DSR-1 Capstan Servo — Public Interface
 *
 * Include this in main.c and usb_cdc.c.
 * Do not include stm32 register headers here — servo.h is interface-only.
 */

#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Telemetry snapshot — written by TIM2 ISR, read by usb_cdc.c
 * All fields are 32-bit aligned; reads are atomic on Cortex-M0+.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t fg_period_ticks;   /* Measured FG period in 64 MHz ticks       */
    uint32_t target_period;     /* Current target period in ticks            */
    int32_t  error;             /* measured - target (positive = too slow)   */
    int32_t  integral;          /* Integral accumulator current value        */
    uint16_t dac_value;         /* Last DAC/PWM output value (0–4095)        */
    uint16_t _pad;              /* Alignment padding                         */
} ServoTelemetry;

/* -------------------------------------------------------------------------
 * Globals — defined in servo.c
 * Gain constants are volatile so the compiler does not cache them across
 * the ISR boundary.  Writes from main loop are atomic (32-bit aligned).
 * ------------------------------------------------------------------------- */
extern volatile ServoTelemetry g_telemetry;
extern volatile int32_t        g_kp_q16;
extern volatile int32_t        g_ki_q16;
extern volatile uint32_t       g_target_period;

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/**
 * servo_init() — configure TIM2, DAC1/TIM3, and NVIC.
 * Call once from main() after clock init, before enabling global interrupts.
 */
void servo_init(void);

/**
 * servo_set_target_period() — update the master speed reference.
 * @ticks: desired FG period in 64 MHz clock ticks.
 *         ticks = 64,000,000 / FG_target_Hz
 * Thread-safe: 32-bit aligned write is atomic on Cortex-M0+.
 */
void servo_set_target_period(uint32_t ticks);

/**
 * servo_reset_integral() — zero the integral accumulator.
 * Call when transitioning from stopped to play to prevent initial overshoot.
 * Briefly disables TIM2 interrupt — safe to call from main loop only.
 */
void servo_reset_integral(void);

#endif /* SERVO_H */
