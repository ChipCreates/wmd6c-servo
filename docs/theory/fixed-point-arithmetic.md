# Fixed-Point Arithmetic in the DSR-1 Firmware

## Purpose of This Document

The DSR-1 servo firmware uses fixed-point integer arithmetic instead of floating-
point for all control calculations. This document explains what fixed-point arithmetic
is, why it is used here, how the Q16 format works, and how to convert between the
floating-point gain constants you might think about and the integer values stored in
`config.h`.

This is primarily useful if you want to tune the PI coefficients manually, port the
firmware to a different microcontroller, or understand why the code looks the way
it does.

---

## 1. The Problem With Floating Point on Cortex-M0+

The selected STM32G0C1KCU6's Cortex-M0+ core is an extremely capable 32-bit processor, but
it has no hardware floating-point unit (FPU). This is a deliberate design choice for
a low-power microcontroller — a hardware FPU adds significant silicon area and power
consumption that is not justified for most embedded applications.

Without a hardware FPU, floating-point arithmetic is performed in software by a
library of routines that emulate the floating-point operations using integer
instructions. This works correctly, but it is slow:

| Operation | Hardware FPU (cycles) | Software float (cycles) |
|---|---|---|
| Float add | 1 | ~30-50 |
| Float multiply | 1 | ~20-40 |
| Float divide | ~15 | ~50-100 |

For a single occasional calculation, software floating-point is perfectly acceptable.
For a control loop ISR that runs 2500 times per second and must complete in
microseconds, it is a problem.

The servo ISR contains two multiplications (proportional and integral terms), an
addition, a subtraction, and two comparisons. Using floating-point, these operations
would take approximately 200-400 clock cycles — at 64 MHz, that is 3-6 microseconds
per ISR execution. With 2500 executions per second, the ISR alone would consume
2500 × 5µs = 12.5ms out of every 1000ms — 1.25% of total CPU time. More critically,
the execution time would be **variable** depending on operand values, introducing
jitter into the timing of the control output.

Using Q16 fixed-point integer arithmetic, the same calculations take approximately
15-20 clock cycles, or about 250-300 nanoseconds. This is 10-20× faster and
completely deterministic — the execution time is identical on every call.

---

## 2. What Fixed-Point Arithmetic Is

Fixed-point arithmetic represents fractional numbers as integers, with an implicit
agreed-upon position for the decimal (or binary) point.

Consider ordinary decimal fixed-point: if we agree that a number always has two
decimal places, then the integer 1500 represents 15.00, the integer 746 represents
7.46, and the integer 3 represents 0.03. We can add and subtract these directly:
1500 + 746 = 2246, which represents 22.46. We can multiply with a correction:
1500 × 746 = 1,119,000 — but this has four implied decimal places, not two, so we
divide by 100 to get 11190, representing 111.90. This is exactly how Q16 works,
except the "decimal point" is in binary.

In binary fixed-point, Q16 format means: the integer value divided by 2^16 = 65536
gives the actual represented value. The "Q" stands for "quotient" and the number
after it indicates how many fractional bits there are.

Examples:
- The value 1.0 is represented as 65536 (1.0 × 65536 = 65536)
- The value 0.15 is represented as 9830 (0.15 × 65536 = 9830.4 ≈ 9830)
- The value 0.008 is represented as 524 (0.008 × 65536 = 524.3 ≈ 524)
- The value 0.5 is represented as 32768 (0.5 × 65536 = 32768)

---

## 3. Q16 Arithmetic Operations

### Addition and Subtraction

Two Q16 numbers can be added directly — no correction needed. If A and B are Q16:

```
A_Q16 + B_Q16 = (A × 65536) + (B × 65536) = (A + B) × 65536 = (A+B)_Q16
```

The result is also a Q16 number representing A+B. This works because both operands
have the same scaling.

### Multiplication

When two Q16 numbers are multiplied, the result has Q32 scaling (scaled by
65536 × 65536 = 2^32). To convert back to Q16, right-shift by 16:

```c
// Multiply Q16 gain by Q0 (plain integer) error:
int32_t proportional = (KP_Q16 * error) >> 16;
```

Here `KP_Q16` is a Q16 number (the gain constant scaled by 65536) and `error` is a
plain integer (Q0 — no fractional bits). The product has Q16 scaling, so right-
shifting by 16 gives a plain integer result. This is the pattern used throughout
the servo code:

```c
int32_t output = DAC_CENTER
               - ((KP_Q16 * error)    >> 16)   // Q16 × Q0 → Q16 >> 16 = Q0
               - ((KI_Q16 * integral) >> 16);  // Q16 × Q0 → Q16 >> 16 = Q0
```

### Why Right-Shift Instead of Divide?

Right-shifting a positive integer by N positions divides it by 2^N. `>> 16` divides
by 65536, which is 2^16. The key advantage of right-shift over division is that the
Cortex-M0+ executes a right-shift in a single clock cycle using its barrel shifter.
Integer division takes 2-12 cycles depending on the operands. For code that runs
thousands of times per second, this matters.

One important caveat: right-shifting a **negative** integer in C is implementation-
defined behaviour — the C standard does not specify whether the shift fills with
zeros (logical shift) or with the sign bit (arithmetic shift). In practice, every
ARM compiler for Cortex-M0+ implements right-shift of signed integers as an
arithmetic right-shift (fills with sign bit), which is the mathematically correct
behaviour for signed fixed-point. The firmware relies on this behaviour, which is
correct for `arm-none-eabi-gcc` and all compatible compilers.

---

## 4. Overflow Considerations

With 32-bit integers and Q16 scaling, the range of representable values is:

```
Maximum Q16 value: 2^31 / 65536 = 32767.9999...
Minimum Q16 value: -2^31 / 65536 = -32768
```

For the gain constants, this is more than sufficient — Kp and Ki values for a
cassette servo are always well below 32768.

For intermediate multiplication results, there is a risk. `KP_Q16 * error` involves
multiplying a Q16 gain (up to ~10000 for Kp = 0.15) by an error value. What is the
maximum possible error?

The error is `measured_period - target_period`. The target period at 2500 Hz is
25,600 ticks. If the motor is completely stopped, the measured period would be
enormous (or zero if no pulses arrive). In practice the maximum realistic period
deviation is bounded by the anti-windup clamp on the integral and by the motor's
physical acceleration limits.

To be safe, the multiplication `KP_Q16 * error` is performed in 32-bit arithmetic.
With KP_Q16 = 9830 and a maximum realistic error of ±10000 ticks:

```
9830 × 10000 = 98,300,000
```

This is well within the signed 32-bit range of ±2,147,483,647. No overflow occurs
under normal operating conditions.

The integral accumulator is bounded by INTEGRAL_LIMIT to prevent overflow in the
`KI_Q16 * integral` product. The default INTEGRAL_LIMIT is set so that
`KI_Q16 × INTEGRAL_LIMIT` does not exceed 2^31.

---

## 5. Converting Between Float and Q16

When tuning PI constants, it is natural to think in floating-point terms ("I want
Kp to be 0.12") and then need to express this in the Q16 format used by `config.h`.

**Float to Q16:**
```python
kp_float = 0.12
kp_q16 = int(kp_float * 65536)  # = 7864
```

**Q16 to float:**
```python
kp_q16 = 7864
kp_float = kp_q16 / 65536  # = 0.11999...
```

The rounding introduces a small error — Q16 represents 0.12 as 7864/65536 =
0.119995... rather than exactly 0.12. This error is negligible for servo control
purposes (0.004% error on the gain constant).

A helper script is provided in `tools/calibration/fg-period-calculator.py` that
performs these conversions and also calculates TARGET_PERIOD from a measured FG
frequency.

---

## 6. USB CDC Live Tuning

The USB CDC command interface provides a convenient way to adjust gain constants
without recompiling firmware. The gain constants used at runtime are stored in SRAM
and can be modified by the `p+`, `p-`, `i+`, `i-` commands:

```
p+   Increase KP_Q16 by 655  (approximately +0.01 in float)
p-   Decrease KP_Q16 by 655
i+   Increase KI_Q16 by 66   (approximately +0.001 in float)
i-   Decrease KI_Q16 by 66
```

The `t` command prints the current values alongside their float equivalents:

```
FG period:  25612 ticks (2499.1 Hz, error: +12)
Integral:   -847
PWM/output: 2034 / 4095
Kp: 9830 (0.1500)   Ki: 524 (0.0080)
RV601: 2048  RV602: 1923  RV603: 2100
```

Once satisfactory constants are found, `s` saves them to the last 4KB flash sector.
They persist across power cycles and survive firmware updates (the save sector is
preserved by the update process unless explicitly erased).

---

## 7. Porting to a Different Microcontroller

If porting the firmware to a microcontroller with a hardware FPU (such as Cortex-M4F
or Cortex-M7), you can replace the Q16 arithmetic with standard float:

```c
// Original Q16 version
int32_t output = DAC_CENTER
               - ((KP_Q16 * error)    >> 16)
               - ((KI_Q16 * integral) >> 16);

// Float version for FPU-equipped MCU
float output = DAC_CENTER
             - (KP_FLOAT * error)
             - (KI_FLOAT * integral);
```

The servo behaviour will be identical (subject to floating-point rounding, which is
negligible at this precision level). The Q16 version is retained in the DSR-1
firmware because the Cortex-M0+ makes it the right choice for this hardware, and
because the explicit fixed-point arithmetic makes the scaling and precision explicit
in a way that floating-point can obscure.

---

## See Also

- [Digital PLL Servo](digital-pll-servo.md) — the complete servo algorithm context
- `firmware/include/config.h` — all constants including KP_Q16, KI_Q16, INTEGRAL_LIMIT
- `tools/calibration/fg-period-calculator.py` — conversion utility
