/**
 * servo.c — DSR-1 Capstan Servo
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 * Hardware: STM32G0B1KBU6, bare-metal, no HAL, no RTOS
 *
 * Peripherals owned by this file:
 *   TIM2   — free-running 32-bit counter at 64 MHz, CH1 input capture on PA0
 *             captures rising edges of the FG901 signal
 *   DAC1   — channel 1 on PA4, 12-bit output to Q601 base (Option A)
 *   TIM3   — channel 1 on PA6, PWM output for Option B NPN level-shift stage
 *
 * PI control convention:
 *   Error = measured_period - target_period
 *   Positive error  → motor running too slow (period too long)
 *   Q601 is PNP: lower base voltage = more collector current = faster motor
 *   Therefore: positive error → subtract from DAC_CENTER → lower output voltage
 *
 * Fixed-point arithmetic:
 *   All gain constants stored in Q16 format (value × 65536).
 *   Multiplication result right-shifted 16 to recover integer units.
 *   No software floating-point in ISR — single-cycle barrel shift only.
 *
 * See fixed-point-arithmetic.md and digital-pll-servo.md for full derivation.
 */

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
 * Configures TIM2 input capture, DAC output, and NVIC priority.
 * Does NOT start the servo loop — that begins automatically on the first
 * FG edge after this function returns.
 * ------------------------------------------------------------------------- */
void servo_init(void)
{
    /* ---- GPIO ---- */

    /* PA0: TIM2_CH1 input capture (AF1), FG901 signal
     * PA4: DAC1_OUT1 analog output, Q601 base drive (Option A)
     * PA6: TIM3_CH1 PWM output (AF1), Option B only — populate if needed */

    /* Enable GPIOA clock */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    /* PA0 — alternate function AF1 (TIM2_CH1) */
    GPIOA->MODER   = (GPIOA->MODER  & ~(3U << (0*2))) | (2U << (0*2)); /* AF mode */
    GPIOA->AFR[0]  = (GPIOA->AFR[0] & ~(0xFU << (0*4))) | (1U << (0*4)); /* AF1 */

    /* PA4 — analog mode for DAC output (MODER = 11) */
    GPIOA->MODER  |= (3U << (4*2));

#ifdef SERVO_OPTION_B
    /* PA6 — alternate function AF1 (TIM3_CH1) for Option B PWM */
    GPIOA->MODER   = (GPIOA->MODER  & ~(3U << (6*2))) | (2U << (6*2));
    GPIOA->AFR[0]  = (GPIOA->AFR[0] & ~(0xFU << (6*4))) | (1U << (6*4));
#endif

    /* ---- TIM2: free-running 32-bit counter, input capture on CH1 ---- */

    RCC->APBENR1 |= RCC_APBENR1_TIM2EN;

    TIM2->PSC   = 0;           /* No prescaler: 1 tick = 15.625 ns at 64 MHz */
    TIM2->ARR   = 0xFFFFFFFF;  /* Full 32-bit range, free-running            */

    /* CC1 configured as input, mapped to TI1 (PA0), no input filter */
    TIM2->CCMR1 = (1U << TIM_CCMR1_CC1S_Pos); /* CC1S = 01: CH1 input on TI1 */
    TIM2->CCER  = TIM_CCER_CC1E;              /* Enable capture, rising edge  */

    /* Enable CC1 capture interrupt */
    TIM2->DIER = TIM_DIER_CC1IE;

    /* Clear any pending flag before enabling in NVIC */
    TIM2->SR = 0;

    /* Start counter */
    TIM2->CR1 = TIM_CR1_CEN;

    /* ---- DAC1 channel 1 (Option A) ---- */

    RCC->APBENR1 |= RCC_APBENR1_DAC1EN;

    /* Enable DAC channel 1, output buffer on */
    DAC1->CR = DAC_CR_EN1;

    /* Pre-load DAC to mid-scale so motor starts at a sensible operating point.
     * At power-on the tape is not moving, so this is safe — the servo will
     * pull the output to the correct value within the first few FG pulses. */
    DAC1->DHR12R1 = DAC_CENTER;

#ifdef SERVO_OPTION_B
    /* ---- TIM3 CH1: PWM for Option B output stage ---- */

    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;

    /* 64 MHz / (PSC+1) / ARR = PWM frequency.
     * PSC = 0, ARR = 4095: PWM frequency = 64 MHz / 4096 ≈ 15.6 kHz
     * Well above audio band, well below FG capture interference threshold. */
    TIM3->PSC   = 0;
    TIM3->ARR   = 4095;

    /* CH1 in PWM mode 1: output high while CNT < CCR1 */
    TIM3->CCMR1 = (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM3->CCER  = TIM_CCER_CC1E;
    TIM3->CCR1  = DAC_CENTER;   /* Start at mid-scale */
    TIM3->CR1   = TIM_CR1_ARPE | TIM_CR1_CEN;
#endif

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
    /* Read capture register — hardware stores the counter value at the
     * moment the rising edge arrived.  Reading CCR1 also clears CC1IF on
     * some devices; clearing explicitly below is belt-and-suspenders. */
    uint32_t now    = TIM2->CCR1;
    uint32_t period = now - last_capture;
    last_capture    = now;

    /* Ignore implausibly short periods — these are noise or switch bounce
     * on start-up before the capstan is at speed.
     * MIN_PERIOD_TICKS = 64 MHz / MAX_PLAUSIBLE_FG_HZ */
    if (period < MIN_PERIOD_TICKS) {
        TIM2->SR &= ~TIM_SR_CC1IF;
        return;
    }

    /* Load gain constants and target once per ISR execution.
     * Volatile reads are single 32-bit loads — atomic on Cortex-M0+. */
    int32_t  kp     = g_kp_q16;
    int32_t  ki     = g_ki_q16;
    uint32_t target = g_target_period;

    /* --- PI control law ---
     *
     * Error: positive = motor running too slow (period longer than target)
     * PNP convention: to speed up motor, decrease DAC output voltage.
     * Therefore both P and I terms are SUBTRACTED from DAC_CENTER.
     */
    int32_t error = (int32_t)period - (int32_t)target;

    /* Integral accumulator with anti-windup clamp */
    integral += error;
    if (integral >  INTEGRAL_LIMIT) integral =  INTEGRAL_LIMIT;
    if (integral < -INTEGRAL_LIMIT) integral = -INTEGRAL_LIMIT;

    /* Proportional + integral output in Q16 fixed-point.
     * (kp * error) >> 16 : Q16 × Q0 → Q16 >> 16 = Q0 (integer ticks)
     * Relies on arithmetic right-shift of signed integers, which is
     * guaranteed by arm-none-eabi-gcc — see fixed-point-arithmetic.md §3 */
    int32_t output = DAC_CENTER
                   - ((kp * error)    >> 16)
                   - ((ki * integral) >> 16);

    /* Clamp to 12-bit DAC range */
    if (output < DAC_MIN) output = DAC_MIN;
    if (output > DAC_MAX) output = DAC_MAX;

    /* Write to output peripheral */
#ifndef SERVO_OPTION_B
    DAC1->DHR12R1 = (uint32_t)output;   /* Option A: direct DAC */
#else
    TIM3->CCR1    = (uint32_t)output;   /* Option B: PWM duty cycle */
#endif

    /* Update telemetry struct — values are read by main loop for USB CDC.
     * No synchronisation needed: main loop only reads, ISR only writes,
     * and 32-bit stores are atomic on Cortex-M0+. */
    g_telemetry.fg_period_ticks = period;
    g_telemetry.target_period   = target;
    g_telemetry.error           = error;
    g_telemetry.integral        = integral;
    g_telemetry.dac_value       = (uint16_t)output;

    /* Clear interrupt flag */
    TIM2->SR &= ~TIM_SR_CC1IF;
}

/* -------------------------------------------------------------------------
 * servo_set_target_period()
 *
 * Called from usb_cdc.c command handler (f+ / f- commands).
 * Write is atomic — 32-bit aligned store on Cortex-M0+.
 * ------------------------------------------------------------------------- */
void servo_set_target_period(uint32_t ticks)
{
    g_target_period = ticks;
}

/* -------------------------------------------------------------------------
 * servo_reset_integral()
 *
 * Clears the integral accumulator.  Called when the machine transitions
 * from stopped to play to prevent wind-up from the stopped state driving
 * an aggressive initial correction.
 *
 * Not called from ISR — disable interrupt briefly for atomic clear.
 * ------------------------------------------------------------------------- */
void servo_reset_integral(void)
{
    NVIC_DisableIRQ(TIM2_IRQn);
    integral = 0;
    NVIC_EnableIRQ(TIM2_IRQn);
}
