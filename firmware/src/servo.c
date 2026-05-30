/**
 * servo.c — DSR-1 Capstan Servo
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 * Hardware: STM32G0B1KBU6, bare-metal, no HAL, no RTOS
 *
 * Peripherals owned by this file:
 *   TIM2   — free-running 32-bit counter at 64 MHz, CH1 input capture on PA0
 *             captures rising edges of the FG901 signal
 *   TIM3   — channel 1 on PA6, PWM output to NPN level-shift stage (Q_LS)
 *
 * Motor output stage:
 *   TIM3 CH1 PWM → R7/C8 RC filter → Q_LS (MMBT3904 NPN) base
 *   Q_LS collector pulls Q601 (2SB1013 PNP) base toward GND via R9.
 *   R9 (100kΩ to B+1) holds Q601 base high (motor off) when Q_LS is off.
 *   At reset/boot, TIM3 PWM defaults to 0% duty — Q_LS off — motor off.
 *
 * PI control convention:
 *   Error = measured_period - target_period
 *   Positive error  → motor running too slow (period longer than target)
 *   Q601 is PNP: lower base voltage = more collector current = faster motor
 *   Higher PWM duty → Q_LS conducts more → Q601 base pulled lower → faster
 *   Therefore: positive error → subtract from DAC_CENTER → lower PWM output
 *
 * Fixed-point arithmetic:
 *   All gain constants stored in Q16 format (value × 65536).
 *   Multiplication result right-shifted 16 to recover integer units.
 *   No software floating-point in ISR — single-cycle barrel shift only.
 *
 * See fixed-point-arithmetic.md and digital-pll-servo.md for full derivation.
 */

#include "stm32g0xx.h"
#include <stdint.h>
#include "config.h"
#include "servo.h"

/* -------------------------------------------------------------------------
 * Module-private state
 * All variables shared between ISR and main loop are volatile.
 * The ISR is the only writer; main loop reads for telemetry.
 * ------------------------------------------------------------------------- */

static volatile uint32_t last_capture   = 0;
static volatile int32_t  integral       = 0;

/* Public telemetry — written by ISR, read by usb_cdc.c */
volatile ServoTelemetry g_telemetry;

/* Gain constants — written by usb_cdc.c command handler (main loop only),
 * read by ISR. Write must be atomic on Cortex-M0+: 32-bit aligned store is
 * single-instruction and therefore inherently atomic. Safe without a mutex. */
volatile int32_t g_kp_q16        = KP_Q16_DEFAULT;
volatile int32_t g_ki_q16        = KI_Q16_DEFAULT;
volatile uint32_t g_target_period = TARGET_PERIOD_DEFAULT;

/* -------------------------------------------------------------------------
 * servo_init()
 *
 * Called once from main() after clock init.
 * Configures TIM2 input capture, TIM3 PWM output, and NVIC priority.
 * Does NOT start the servo loop — that begins automatically on the first
 * FG edge after this function returns.
 * ------------------------------------------------------------------------- */
void servo_init(void)
{
    /* ---- GPIO ---- */

    /* PA0: TIM2_CH1 input capture (AF1), FG901 signal
     * PA6: TIM3_CH1 PWM output (AF1), NPN level-shift stage drive */

    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    /* PA0 — alternate function AF1 (TIM2_CH1) */
    GPIOA->MODER   = (GPIOA->MODER  & ~(3U << (0*2))) | (2U << (0*2));
    GPIOA->AFR[0]  = (GPIOA->AFR[0] & ~(0xFU << (0*4))) | (1U << (0*4));

    /* PA6 — alternate function AF1 (TIM3_CH1) */
    GPIOA->MODER   = (GPIOA->MODER  & ~(3U << (6*2))) | (2U << (6*2));
    GPIOA->AFR[0]  = (GPIOA->AFR[0] & ~(0xFU << (6*4))) | (1U << (6*4));

    /* ---- TIM2: free-running 32-bit counter, input capture on CH1 ---- */

    RCC->APBENR1 |= RCC_APBENR1_TIM2EN;

    TIM2->PSC   = 0;           /* No prescaler: 1 tick = 15.625 ns at 64 MHz */
    TIM2->ARR   = 0xFFFFFFFF;  /* Full 32-bit range, free-running            */

    /* CC1 configured as input, mapped to TI1 (PA0), no input filter */
    TIM2->CCMR1 = (1U << TIM_CCMR1_CC1S_Pos); /* CC1S = 01: CH1 input on TI1 */
    TIM2->CCER  = TIM_CCER_CC1E;              /* Enable capture, rising edge  */

    TIM2->DIER = TIM_DIER_CC1IE;
    TIM2->SR   = 0;
    TIM2->CR1  = TIM_CR1_CEN;

    /* ---- TIM3 CH1: PWM for NPN level-shift motor output ---- */

    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    /* 64 MHz / (PSC+1) / ARR = PWM frequency.
     * PSC = 0, ARR = 4095: PWM frequency = 64 MHz / 4096 ≈ 15.6 kHz
     * Well above audio band, well below FG capture interference threshold. */
    TIM3->PSC   = 0;
    TIM3->ARR   = 4095;

    /* CH1 in PWM mode 1: output high while CNT < CCR1 */
    TIM3->CCMR1 = (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM3->CCER  = TIM_CCER_CC1E;

    /* Pre-load at mid-scale. At 0% duty Q_LS is off and R9 holds Q601 base
     * toward B+1 — motor off. Mid-scale is a safe starting drive point once
     * the servo loop takes over. */
    TIM3->CCR1  = DAC_CENTER;
    TIM3->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;

    /* ---- NVIC ---- */

    /* TIM2 interrupt at highest priority (0).
     * This is non-negotiable — see why-bare-metal.md §5.1.
     * The servo ISR must preempt all other interrupts including USB. */
    NVIC_SetPriority(TIM2_IRQn, 0);
    NVIC_EnableIRQ(TIM2_IRQn);
}

/* -------------------------------------------------------------------------
 * TIM2_IRQHandler() — Servo ISR
 *
 * Called on every rising edge of the FG901 signal.
 * At correct tape speed (~2500 Hz FG rate) this runs every ~400 µs.
 * Execution time: <30 clock cycles (~470 ns at 64 MHz).
 *
 * Must not be modified to use floating-point, call library functions,
 * or do anything that cannot complete in well under 400 µs.
 * ------------------------------------------------------------------------- */
void TIM2_IRQHandler(void)
{
    uint32_t now    = TIM2->CCR1;
    uint32_t period = now - last_capture;
    last_capture    = now;

    /* Ignore implausibly short periods — noise or switch bounce on start-up
     * before the capstan is at speed. */
    if (period < MIN_PERIOD_TICKS) {
        TIM2->SR &= ~TIM_SR_CC1IF;
        return;
    }

    int32_t  kp     = g_kp_q16;
    int32_t  ki     = g_ki_q16;
    uint32_t target = g_target_period;

    /* --- PI control law ---
     *
     * Error: positive = motor running too slow (period longer than target)
     * PNP convention: to speed up motor, decrease PWM duty (less Q_LS drive
     * → Q601 base pulled less toward GND → Q601 emitter-base voltage increases
     * → more collector current → faster motor).
     * Therefore both P and I terms are SUBTRACTED from DAC_CENTER.
     */
    int32_t error = (int32_t)period - (int32_t)target;

    integral += error;
    if (integral >  INTEGRAL_LIMIT) integral =  INTEGRAL_LIMIT;
    if (integral < -INTEGRAL_LIMIT) integral = -INTEGRAL_LIMIT;

    int32_t output = DAC_CENTER
                   - ((kp * error)    >> 16)
                   - ((ki * integral) >> 16);

    if (output < DAC_MIN) output = DAC_MIN;
    if (output > DAC_MAX) output = DAC_MAX;

    TIM3->CCR1 = (uint32_t)output;

    g_telemetry.fg_period_ticks = period;
    g_telemetry.target_period   = target;
    g_telemetry.error           = error;
    g_telemetry.integral        = integral;
    g_telemetry.dac_value       = (uint16_t)output;

    TIM2->SR &= ~TIM_SR_CC1IF;
}

/* -------------------------------------------------------------------------
 * servo_set_target_period()
 * ------------------------------------------------------------------------- */
void servo_set_target_period(uint32_t ticks)
{
    g_target_period = ticks;
}

/* -------------------------------------------------------------------------
 * servo_reset_integral()
 * ------------------------------------------------------------------------- */
void servo_reset_integral(void)
{
    NVIC_DisableIRQ(TIM2_IRQn);
    integral = 0;
    NVIC_EnableIRQ(TIM2_IRQn);
}

/* -------------------------------------------------------------------------
 * servo_freeze()
 *
 * Prepares the servo for a flash write blackout window:
 *   1. Disables TIM2 capture interrupt — no more ISR executions.
 *   2. TIM3 PWM output retains its current value — motor holds drive level.
 *   3. Resets the integral so servo_unfreeze() restarts from a clean state.
 * ------------------------------------------------------------------------- */
void servo_freeze(void)
{
    NVIC_DisableIRQ(TIM2_IRQn);
    integral = 0;
}

/* -------------------------------------------------------------------------
 * servo_unfreeze()
 *
 * Restores servo operation after a flash write blackout.
 * Seeds last_capture to prevent a spurious huge-period measurement on the
 * first post-freeze FG edge, then re-enables TIM2.
 * ------------------------------------------------------------------------- */
void servo_unfreeze(void)
{
    last_capture = TIM2->CNT;
    NVIC_EnableIRQ(TIM2_IRQn);
}
