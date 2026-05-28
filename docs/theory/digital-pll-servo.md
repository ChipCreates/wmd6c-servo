# The DSR-1 Digital PLL Servo Loop

## Purpose of This Document

This document explains how the DSR-1 module's digital servo loop works — the period
measurement, the PI control algorithm, the fixed-point arithmetic, and the motor
drive output. It is written for someone who understands the original analog servo
(see [original-servo-circuit.md](original-servo-circuit.md)) and wants to understand
how the digital replacement achieves the same function, and why it does so more
accurately.

---

## 1. The Same Goal, a Different Approach

The original CX20084 analog PLL and the DSR-1 digital PI loop accomplish the same
task: measure actual tape speed, compare it to a target speed, generate a correction
signal, and apply it to the motor. The underlying physics is identical. What differs
is how the comparison and correction are implemented.

The analog PLL compares the phases of two continuous signals — the FG pulse train
and the crystal reference — using analog circuitry whose behaviour is governed by
the laws of physics acting on physical components. The digital loop measures the
*period* of FG pulses in hardware timer counts, compares that integer to a target
integer, and applies a mathematical correction calculated in the microcontroller's
integer arithmetic unit. One system lives in continuous time and analogue voltage.
The other lives in discrete samples and binary numbers.

Both work. The digital approach has measurable advantages for this application that
are detailed at the end of this document.

---

## 2. The FG Period Measurement

### 2.1 Why Period, Not Frequency

The natural temptation is to measure the FG signal as a frequency — count pulses per
unit time and compare to a target frequency. This works but introduces a fundamental
trade-off: the longer you count, the more accurate your frequency measurement, but
the slower your servo responds. At 2500 Hz FG rate, counting for 100ms gives you
250 pulses and excellent accuracy — but your servo is now 100ms behind reality.
Count for 1ms and you have only 2-3 pulses, giving poor accuracy.

The DSR-1 measures **period** instead — the time between successive FG rising edges.
This gives you one measurement per FG pulse (2500 measurements per second at
correct speed), with resolution limited only by the timer clock frequency rather
than the counting interval. At 64 MHz the timer ticks every 15.6 nanoseconds. A
single period measurement at 2500 Hz (target period ~25,600 ticks) has a
measurement resolution of 15.6 ns / (25,600 × 15.6 ns) = 1/25,600 = approximately
0.004%. That is more than an order of magnitude finer than the servo's target
accuracy of 0.05%.

### 2.2 TIM2 Input Capture

The STM32G0B1's TIM2 is configured as a free-running 32-bit counter clocked at
64 MHz. The input capture function on channel 1 (PA0) monitors the FG signal and,
on every rising edge, **automatically records the current counter value in the
CCR1 register** and sets an interrupt flag. This happens entirely in hardware with
no software latency.

The interrupt service routine (ISR) reads CCR1 immediately on entry:

```c
void TIM2_IRQHandler(void) {
    uint32_t now = TIM2->CCR1;          // hardware captured timestamp
    uint32_t period = now - last_cap;   // period in timer ticks
    last_cap = now;
    // ... control calculation ...
    TIM2->SR &= ~TIM_SR_CC1IF;          // clear flag
}
```

The subtraction `now - last_cap` gives the period in ticks. Because TIM2 is a
32-bit counter, the subtraction handles counter wraparound correctly through
unsigned arithmetic overflow — the result is always the correct period regardless
of whether the counter wrapped during the measurement interval.

### 2.3 The Target Period

The target period is the number of 64 MHz timer ticks that should elapse between
successive FG pulses at exactly the correct tape speed. It is calculated as:

```
TARGET_PERIOD = SYSCLK_HZ / FG_TARGET_HZ
             = 64,000,000 / FG_TARGET_HZ
```

where FG_TARGET_HZ is the FG pulse rate at correct tape speed, measured on the
specific unit using a calibrated test tape. This value is stored as a 32-bit integer
constant in `firmware/include/config.h`.

The critical property of TARGET_PERIOD is that it is a **fixed integer constant**
stored in non-volatile flash memory. It does not drift with temperature. It does not
age. It does not vary with supply voltage. Once calibrated, it is the servo's
permanent speed reference.

---

## 3. The PI Control Algorithm

### 3.1 What PI Means

PI stands for Proportional-Integral. These are the two terms in the control law.
Understanding what each term does is the key to understanding the servo's behaviour.

**Proportional term**: The correction is proportional to the current error. If the
motor is running 2% fast, the proportional term applies twice as much correction as
if it were running 1% fast. The proportional term responds immediately to the
current error but cannot by itself eliminate a sustained steady-state error —
because at zero error, the proportional term produces zero correction, which means
any systematic bias (bearing friction, tape tension, motor characteristic) will
produce a non-zero steady-state error that the proportional term stabilises around
rather than eliminating.

**Integral term**: The correction is proportional to the accumulated sum of all
past errors. If the motor runs consistently 0.1% slow for 10 seconds, the integral
accumulator builds up a value that produces a sustained upward correction. The
integral term eliminates steady-state error — it keeps winding up until the error
goes to zero, then holds the correction constant to keep it there. The downside is
that it can overshoot and oscillate if its gain is too high.

Together, the PI controller combines the fast response of the proportional term with
the steady-state accuracy of the integral term. For a capstan servo application —
where the primary disturbances are slow thermal drift, tape tension variation, and
bearing load changes — PI control is exactly the right choice. A derivative term
(making it a PID controller) is not needed and would add noise sensitivity without
benefit.

### 3.2 The Error Signal

The error is calculated in the period domain:

```c
int32_t error = (int32_t)measured_period - (int32_t)target_period;
```

A **positive error** means the measured period is longer than the target — the motor
is running **too slow** (fewer FG pulses per second, longer time between them).

A **negative error** means the measured period is shorter than the target — the
motor is running **too fast**.

This sign convention is important for understanding the control law below.

### 3.3 The Control Law

```c
// Accumulate integral with anti-windup clamp
integral += error;
if (integral >  INTEGRAL_LIMIT) integral =  INTEGRAL_LIMIT;
if (integral < -INTEGRAL_LIMIT) integral = -INTEGRAL_LIMIT;

// Calculate output (Q16 fixed-point arithmetic)
int32_t output = DAC_CENTER
               - ((KP_Q16 * error)    >> 16)
               - ((KI_Q16 * integral) >> 16);

// Clamp to DAC range
if (output < DAC_MIN) output = DAC_MIN;
if (output > DAC_MAX) output = DAC_MAX;

DAC1->DHR12R1 = (uint32_t)output;
```

The subtraction in the output calculation reflects the **PNP motor drive
convention**: Q601 is a PNP transistor whose base requires a **lower voltage** to
conduct **more** (faster motor). Therefore:

- Positive error (too slow) → subtract positive proportional and integral terms
  → output decreases → lower DAC voltage → Q601 conducts more → motor speeds up ✓

- Negative error (too fast) → subtract negative proportional and integral terms
  → output increases → higher DAC voltage → Q601 conducts less → motor slows down ✓

DAC_CENTER (2048, mid-scale of the 12-bit DAC) is the operating point around which
corrections are applied. The DAC_MIN and DAC_MAX clamps prevent the output from
driving the motor to a full stop or full speed during large transients.

### 3.4 Anti-Windup

The integral accumulator is clamped to ±INTEGRAL_LIMIT. Without this clamp, a
large sustained error — such as the motor spinning up from rest — would cause the
integral to accumulate an enormous value. When the error eventually reduces, the
integral would take a long time to unwind, causing significant overshoot past the
target speed.

The clamp limits the maximum sustained correction the integral term can apply,
bounding the overshoot while still allowing the integral to perform its steady-state
error elimination function in normal operation.

---

## 4. Fixed-Point Arithmetic

### 4.1 Why No Floating Point

The STM32G0B1's Cortex-M0+ core has no hardware floating-point unit. A floating-
point multiplication in software takes approximately 20-40 clock cycles, depending
on operand values and the compiler's soft-float library implementation. At 64 MHz
this is 300-600 nanoseconds — acceptable for a single operation, but the ISR
contains two multiplications (proportional and integral terms) plus additional
arithmetic, and must complete fast enough to not interfere with the next FG pulse.

At 2500 Hz FG rate, pulses arrive every 400 microseconds. The entire ISR must
complete well within this budget. Using fixed-point integer arithmetic, each
multiplication takes a single clock cycle (the Cortex-M0+ has a hardware 32×32
integer multiplier). The complete ISR executes in under 30 clock cycles — less than
500 nanoseconds. This leaves 399.5 microseconds of spare time between executions,
which the main loop uses for USB CDC processing, ADC scanning, and system monitoring.

### 4.2 Q16 Format

Q16 fixed-point represents numbers as integers where the value is the integer
divided by 2^16 = 65536. The gain constant Kp = 0.15 in floating point becomes:

```
KP_Q16 = (int32_t)(0.15 * 65536) = 9830
```

In the control calculation, the multiplication `KP_Q16 * error` produces a Q16
result (the "unit" of the product is the original units divided by 65536). The right
shift by 16 positions (`>> 16`) removes the Q16 scaling, recovering the result in
the original units. This is equivalent to dividing by 65536 but executes as a
single-cycle barrel shift rather than a multi-cycle division.

The full range of representable Q16 values with a 32-bit integer is ±32767.9999...
This is more than sufficient for all practical gain constants in this application.

### 4.3 Choosing Kp and Ki

The initial values in config.h are starting points established by simulation and
engineering judgement, not empirically tuned values. Final tuning is performed on
the bench with a specific motor and mechanical assembly:

**Kp = 0.15 (KP_Q16 = 9830)**: Provides moderate proportional response. Increasing
Kp makes the servo respond faster to speed changes but risks instability and audible
speed oscillation at very high values. Decreasing Kp makes response sluggish.

**Ki = 0.008 (KI_Q16 = 524)**: Provides slow but effective integral action. The
integral term's time constant is approximately 1/Ki FG periods = 125 FG periods =
50ms. This means sustained speed errors are corrected over a timescale of tens of
milliseconds — fast enough to counteract thermal drift and tape tension changes, slow
enough not to interfere with the proportional term's fast response.

The USB CDC command interface (`p+`, `p-`, `i+`, `i-`) allows these values to be
adjusted in real time while the servo is running and the telemetry output (`t`, `T`)
shows the effect immediately. The optimised values are then saved to flash with `s`.

---

## 5. The Speed Reference Potentiometers

The three speed trim potentiometers from the original circuit are retained and read
by the STM32 ADC in a continuous DMA scan. Their values modify the target period
used by the servo loop:

```c
// RV601: base speed trim (fine calibration offset)
int32_t rv601_offset = ((int32_t)adc_rv601 - 2048) * BASE_TRIM_SCALE / 2048;

// RV602: user Speed Tune slider
int32_t rv602_offset = ((int32_t)adc_rv602 - 2048) * rv603_scale / 2048;

// RV603: Speed Tune range scalar
int32_t rv603_scale  = ((int32_t)adc_rv603) * SPEED_RANGE_MAX / 4095;

// Combined adjusted target
uint32_t adjusted_target = TARGET_PERIOD + rv601_offset + rv602_offset;
```

This faithfully reproduces the original Speed Tune functionality: RV601 provides a
fine calibration trim, RV602 is the user-accessible speed control, and RV603 sets
the sensitivity of RV602. The user experience of operating these controls is
identical to the original machine.

---

## 6. The Motor Drive Output

Q601 (2SB1013 PNP) on WM-D6C serial 72795 has its emitter at B+3 (10.8V). The
base operating range is well above the STM32's 3.3V output capability. DSR-1 uses
a PWM + RC filter + NPN level-shift stage exclusively.

### 6.1 PWM with RC Filter and NPN Level Shift

TIM3 channel 1 on PA6 produces a PWM signal at approximately 15.6 kHz. An RC
low-pass filter (R7 1kΩ, C8 100nF, corner frequency ~1.6 kHz) converts this to a
smooth analog voltage. This voltage drives the base of Q_LS (MMBT3904 NPN), whose
collector drives Q601's base through R8 10kΩ. The NPN level shift allows a 3.3V
control signal to drive a base that operates well above the STM32's output range.

---

## 7. Why Digital Is Better for This Application

Having explained how the digital loop works, it is worth being explicit about why it
is superior to the original analog circuit for this specific application.

**The reference does not drift.** TARGET_PERIOD is a 32-bit integer in flash. It
does not change with temperature, supply voltage, or time. The original crystal
reference drifts with temperature and ages over decades. The digital reference is
perfect for the life of the device.

**The loop parameters are adjustable.** Kp and Ki are constants in a header file,
changeable via USB CDC and saveable to flash. If a motor's characteristics change —
due to bearing wear, brush wear, or lubrication changes — the servo can be retuned
in minutes without any hardware modification. The original loop parameters are fixed
in hardware and require PCB modification to change.

**The update rate is higher.** The digital loop executes on every FG pulse — 2500
times per second at correct speed. The original analog PLL's effective update rate
is limited by the crystal divider chain to approximately the divided reference
frequency, which is much lower. Higher update rate means faster correction of
transient disturbances.

**The resolution is higher.** The period measurement resolution is one timer clock
period — 15.6 ns at 64 MHz. This is a fractional speed error of 0.004% per count.
The original analog phase detector's resolution is limited by the noise floor of its
analog circuitry.

**The internal state is observable.** The USB CDC telemetry output provides real-
time visibility into the measured period, error, integral accumulator, DAC value,
and all pot readings. Diagnosing a poorly-performing servo requires only a USB cable
and a terminal application. The original circuit requires an oscilloscope and
specialised knowledge to observe its internal state.

**It cannot be destroyed by a wrong power adapter.** The original CX20084 had no
protection against reverse polarity. The DSR-1 module eliminates this failure mode
at the hardware level in both variants.

---

## See Also

- [Original Servo Circuit](original-servo-circuit.md) — how the CX20084 worked
- [Fixed-Point Arithmetic](fixed-point-arithmetic.md) — Q16 format explained in detail
- [Why This Failed](why-this-failed.md) — the reverse polarity failure
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — complete specification
