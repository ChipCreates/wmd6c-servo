/**
 * config.h — DSR-1 Tunable Constants
 *
 * WM-D6C Servo Replacement Module (DSR-1)
 *
 * THIS IS THE ONLY FILE YOU SHOULD NEED TO EDIT FOR NORMAL CONFIGURATION.
 *
 * All machine-specific values, gain constants, hardware limits, and feature
 * selection flags live here. Edit this file and rebuild — do not scatter
 * magic numbers through the source files.
 *
 * To convert between floating-point and Q16:
 *   Q16 = (int32_t)(float_value * 65536)
 *   float = Q16_value / 65536.0
 *
 * Motor output stage: PWM + RC filter + NPN level-shift via PA6 (TIM3_CH1).
 * Q601 on WM-D6C serial 72795 is a 2SB1013 PNP transistor with emitter at
 * B+3 (10.8V). The base operating range is well above 3.3V, ruling out direct
 * DAC drive. The NPN level-shift stage (Q_LS MMBT3904, R7-R9) is the sole
 * motor output topology for this build.
 *
 * See fixed-point-arithmetic.md for full derivation and overflow analysis.
 * See digital-pll-servo.md for servo algorithm context.
 * See tools/calibration/fg-period-calculator.py for conversion helpers.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* =========================================================================
 * SECTION 1 — SYSTEM CLOCK
 *
 * Do not change unless you have modified clock_init() in main.c.
 * All timing constants below are derived from this value.
 * ========================================================================= */

#define SYSCLK_HZ           64000000UL   /* HSI16 × PLL → 64 MHz            */

/* =========================================================================
 * SECTION 2 — FG TARGET SPEED
 *
 * TARGET_PERIOD_DEFAULT is the FG pulse period in 64 MHz clock ticks that
 * corresponds to correct tape transport speed (4.75 cm/s).
 *
 * *** THIS VALUE MUST BE MEASURED ON THE BENCH ***
 * Use test tape WS-48B: correct speed = 3 kHz on LINE OUT.
 * Measure FG901 pulse frequency with an oscilloscope or logic analyser at
 * that speed. Then:
 *   TARGET_PERIOD_DEFAULT = SYSCLK_HZ / FG_target_Hz
 *
 * The placeholder value below assumes 2500 Hz FG rate (25,600 ticks).
 * This is a reasonable starting estimate but will NOT be correct for your
 * specific unit. Do not use this value for final calibration.
 *
 * Example: if bench measurement gives FG = 2487 Hz:
 *   TARGET_PERIOD_DEFAULT = 64000000 / 2487 = 25,729
 * ========================================================================= */

#define FG_TARGET_HZ_NOMINAL    2500UL
#define TARGET_PERIOD_DEFAULT   (SYSCLK_HZ / FG_TARGET_HZ_NOMINAL)  /* 25,600 ticks */

/* Safety bounds on adjusted target (after pot trim is applied).
 * ADJUSTED_TARGET_MIN = half nominal (motor cannot run at 2× speed physically)
 * ADJUSTED_TARGET_MAX = double nominal (motor cannot run at 0.5× speed usefully) */
#define ADJUSTED_TARGET_MIN     (TARGET_PERIOD_DEFAULT / 2)   /* 12,800 ticks */
#define ADJUSTED_TARGET_MAX     (TARGET_PERIOD_DEFAULT * 2)   /* 51,200 ticks */

/* =========================================================================
 * SECTION 3 — PI GAIN CONSTANTS
 *
 * Stored in Q16 fixed-point format (value × 65536).
 * Starting values are engineering estimates — tune on bench with test tape.
 * Use USB CDC commands p+/p-/i+/i- for live adjustment; save with 's'.
 *
 * Kp = 0.15  →  KP_Q16 = (int32_t)(0.15 × 65536) = 9830
 * Ki = 0.008 →  KI_Q16 = (int32_t)(0.008 × 65536) = 524
 *
 * Tuning guidance (see digital-pll-servo.md §4.3):
 *   Increase Kp → faster response, risk of audible speed oscillation
 *   Decrease Kp → sluggish response, slow recovery from tape tension changes
 *   Increase Ki → faster correction of steady-state drift, risk of low-freq hunt
 *   Decrease Ki → slower drift correction, longer time to lock after start
 * ========================================================================= */

#define KP_Q16_DEFAULT          9830    /* Kp = 0.1500 */
#define KI_Q16_DEFAULT          524     /* Ki = 0.0080 */

/* USB CDC step sizes for p+/p-/i+/i- commands */
#define KP_Q16_STEP             655     /* ≈ +0.01 in float  (655 / 65536 = 0.00999) */
#define KI_Q16_STEP             66      /* ≈ +0.001 in float (66  / 65536 = 0.00101) */

/* Safety limits — gain constants are clamped to these ranges.
 * Prevents runaway if USB CDC receives corrupted input. */
#define KP_Q16_MIN              0
#define KP_Q16_MAX              65536   /* Kp = 1.0 — well above any practical value */
#define KI_Q16_MIN              0
#define KI_Q16_MAX              6554    /* Ki = 0.1 — well above any practical value  */

/* =========================================================================
 * SECTION 4 — INTEGRAL ANTI-WINDUP
 *
 * INTEGRAL_LIMIT bounds the integral accumulator to prevent windup during
 * motor spin-up and large speed transients.
 *
 * Derivation (see signal-chain-analysis.md §11.3 and fixed-point-arithmetic.md §4):
 *   Must satisfy: KI_Q16 × INTEGRAL_LIMIT < 2^31 (no 32-bit overflow)
 *   KI_Q16_DEFAULT × INTEGRAL_LIMIT < 2,147,483,647
 *   524 × INTEGRAL_LIMIT < 2,147,483,647
 *   INTEGRAL_LIMIT < 4,098,250
 *
 *   Recommended value = TARGET_PERIOD × 50 = 1,280,000
 *   This limits maximum integral PWM contribution to:
 *     (524 × 1,280,000) >> 16 ≈ 10,224 counts
 *   Which saturates the output (0–4095) but unwinds within ~2–3 FG periods
 *   after lock — clean, non-oscillating capture.
 * ========================================================================= */

#define INTEGRAL_LIMIT          1280000L   /* TARGET_PERIOD_DEFAULT × 50 */

/* =========================================================================
 * SECTION 5 — PWM OUTPUT RANGE
 *
 * TIM3 PWM uses a 12-bit range (0–4095) on PA6 (TIM3_CH1).
 * DAC_CENTER is the quiescent operating point (mid-scale duty cycle).
 * DAC_MIN/MAX are the servo output clamps — not hard rail limits.
 * Leaving headroom above MIN and below MAX avoids driving Q601 fully off
 * or fully on during large transients, which would cause audible clicks.
 *
 * Naming retained as DAC_* for compatibility with servo.c arithmetic.
 * ========================================================================= */

#define DAC_CENTER              2048    /* Mid-scale PWM duty cycle           */
#define DAC_MIN                 100     /* Near-full motor drive              */
#define DAC_MAX                 3995    /* Motor nearly off                   */

/* =========================================================================
 * SECTION 6 — FG INPUT NOISE REJECTION
 *
 * MIN_PERIOD_TICKS rejects implausibly short FG periods at start-up and
 * on power-on before the capstan reaches speed. Any captured period shorter
 * than this is discarded without updating the servo state.
 *
 * Set to SYSCLK_HZ / max_plausible_FG_Hz.
 * Max plausible FG during normal play ≈ 2× nominal = 5000 Hz → 12,800 ticks.
 * A value of 6400 (10,000 Hz equivalent) provides comfortable margin.
 * ========================================================================= */

#define MIN_PERIOD_TICKS        6400UL  /* Reject periods shorter than 6400 ticks */

/* =========================================================================
 * SECTION 7 — SPEED TUNE POTENTIOMETER SCALING
 *
 * RV601: base speed trim — fine calibration offset applied to TARGET_PERIOD.
 * RV602: user Speed Tune slider — user-accessible speed control.
 * RV603: Speed Tune range scalar — sets sensitivity of RV602.
 *
 * BASE_TRIM_SCALE: maximum period offset RV601 can apply (ticks).
 *   ±BASE_TRIM_SCALE at ADC full deflection from mid-scale.
 *   Default ±256 ticks ≈ ±1% speed adjustment at 25,600 tick target.
 *
 * SPEED_RANGE_MAX: maximum period offset RV602 can apply when RV603 is
 *   at maximum. Default ±768 ticks ≈ ±3% — matches original WM-D6C spec.
 * ========================================================================= */

#define BASE_TRIM_SCALE         256     /* RV601 max trim: ±256 ticks ≈ ±1%  */
#define SPEED_RANGE_MAX         768     /* RV603 max range: ±768 ticks ≈ ±3% */

/* =========================================================================
 * SECTION 8 — FLASH SETTINGS STORAGE
 *
 * Settings are stored in the last 4 KB sector of the 128 KB flash.
 * This sector is preserved across firmware updates (DFU does not erase it).
 *
 * STM32G0B1KBU6 flash: 128 KB = 0x08000000–0x0801FFFF
 * Last 4 KB sector:    0x0801F000–0x0801FFFF
 *
 * SETTINGS_MAGIC: sentinel value written at start of settings block.
 * On power-on, if the magic value is present and the checksum passes,
 * saved constants are loaded. Otherwise defaults from this file are used.
 * ========================================================================= */

#define FLASH_SETTINGS_ADDR     0x0801F000UL
#define SETTINGS_MAGIC          0x44535231UL   /* ASCII "DSR1" */

/* =========================================================================
 * SECTION 9 — USB CDC TELEMETRY
 *
 * TELEMETRY_INTERVAL_MS: minimum interval between telemetry lines in
 * continuous mode ('T' command). Prevents USB from being overwhelmed at
 * high FG rates. 20ms = 50 lines/second — comfortable for terminal display.
 * ========================================================================= */

#define TELEMETRY_INTERVAL_MS   20

/* =========================================================================
 * SECTION 10 — BUILD ASSERTIONS
 *
 * Compile-time checks that catch configuration errors before they become
 * runtime bugs. The compiler emits an error if any condition is violated.
 * ========================================================================= */

/* Integral limit must not cause overflow: KI_Q16_DEFAULT × INTEGRAL_LIMIT < 2^31 */
_Static_assert(
    (int64_t)KI_Q16_DEFAULT * INTEGRAL_LIMIT < 2147483647LL,
    "INTEGRAL_LIMIT too large: KI_Q16 * INTEGRAL_LIMIT overflows int32_t"
);

/* PWM output range sanity */
_Static_assert(DAC_MIN >= 0,    "DAC_MIN must be >= 0");
_Static_assert(DAC_MAX <= 4095, "DAC_MAX must be <= 4095");
_Static_assert(DAC_MIN < DAC_CENTER && DAC_CENTER < DAC_MAX,
    "DAC_CENTER must be between DAC_MIN and DAC_MAX");

/* Target period sanity */
_Static_assert(TARGET_PERIOD_DEFAULT > MIN_PERIOD_TICKS,
    "TARGET_PERIOD_DEFAULT must be greater than MIN_PERIOD_TICKS");

#endif /* CONFIG_H */
