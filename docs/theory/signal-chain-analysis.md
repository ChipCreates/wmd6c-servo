# Signal Chain Analysis

## Purpose of This Document

This document traces every signal that passes between the WM-D6C machine and the
DSR-1 Servo Control Board — from its source in the original circuitry, through each conditioning
stage on the Servo Control Board, to the STM32 pin that receives or drives it, and back
out through the motor drive chain to the load. For each signal it explains the
voltage levels involved, the purpose of every component in the conditioning network,
the protection mechanisms, and the failure modes each mechanism guards against.

This is the companion document to the schematic. The schematic shows *what* is
connected. This document explains *why*.

---

## Interface Overview

All signals between the DSR-1 Servo Control Board and the WM-D6C main board pass through the
eight-position JST PH J1 connector. The signals are:

| J1 Pin | Net | Direction | Nature |
|---|---|---|---|
| 1 | FG_RAW | machine → Servo Control Board | Digital pulse train, machine voltage levels |
| 2 | Q601_BASE | Servo Control Board → machine | Analog motor drive, machine voltage levels |
| 3 | MOTOR_EN | machine → Servo Control Board | Digital logic level, machine voltage levels |
| 4 | RV601_WIPER | machine → Servo Control Board | Analog DC, machine voltage levels |
| 5 | RV602_WIPER | machine → Servo Control Board | Analog DC, machine voltage levels |
| 6 | RV603_WIPER | machine → Servo Control Board | Analog DC, machine voltage levels |
| 7 | B+1 / VBATT | machine ↔ Servo Control Board | DC power rail, 6V (Servo Control Board drives it in wall variants; receives it in the two-board battery build) |
| 8 | GND | Shared | Common ground reference |

The fundamental electrical challenge is that all signals on the machine side operate
at voltages referenced to the B+1 supply rail — nominally 6V from batteries. The
STM32G0C1KCU6 operates at 3.3V and its absolute maximum input voltage is VDD + 0.3V
= 3.6V. Every signal must be conditioned to bring it within safe operating range
before it reaches an STM32 pin, and every output from the STM32 must be level-
shifted or amplified to drive the machine's circuitry correctly.

---

## 1. FG Signal Path (J1 Pin 1: FG_RAW → PA0)

### 1.1 Signal Source

The FG901 GP2S22AB is an optical interrupter sensor — an infrared LED and
phototransistor in a slotted housing. A precision-slotted metal disc on the capstan
motor shaft passes through the slot. When a slot aligns with the beam, the
phototransistor conducts and pulls its collector toward ground. When the disc
material blocks the beam, the phototransistor is off and the collector is pulled
toward the supply rail by an external resistor on the WM-D6C main board.

The FG901 output is therefore **open-collector**: the collector (output pin) is
pulled high by an external pull-up resistor to the machine's B+1 rail, and pulled
low by the phototransistor when the beam is unobstructed. The signal swings between
approximately 0V (beam clear, phototransistor on, low saturation voltage ~0.1V) and
approximately the B+1 rail voltage minus the pull-up resistor voltage drop (at the
light FG sensor load current, this is approximately B+1 - 0.1V ≈ 5.9V at full
battery charge).

The resulting waveform is a pulse train whose frequency is directly proportional to
the motor's rotational speed. At correct tape speed (4.75 cm/s), this frequency is
a known value determined by the number of slots in the disc and the capstan's
mechanical gear ratio. This value — FG_TARGET_HZ — must be measured on the specific
unit with a calibrated test tape and entered into `config.h`.

**Nominal FG signal levels on the machine side:**
- Logic HIGH: approximately 5.5V to 6.0V (B+1 minus pull-up drop)
- Logic LOW: approximately 0.1V to 0.2V (FG901 phototransistor saturation)
- Swing amplitude: approximately 5.4V to 5.8V

**STM32 PA0 absolute maximum input voltage: 3.6V (VDD + 0.3V)**

The FG signal cannot be connected directly to PA0. Conditioning is mandatory.

### 1.2 Conditioning Network

The conditioning network between J1 pin 1 and PA0 consists of four components:

```
J1 Pin 1 (FG_RAW)
    │
   [R3: 10kΩ series]
    │
    ├──[R4: 22kΩ to GND]    ← Voltage divider lower leg
    │
    ├──[D2: BAT54 anode to node, cathode to VDD 3.3V]  ← High-side clamp
    ├──[D3: BAT54 anode to GND, cathode to node]       ← Low-side clamp
    │
    ├──[C7: 10pF C0G to GND]  ← HF noise filter
    │
   PA0 (TIM2_CH1 input)
```

**R3 and R4 — Resistive Voltage Divider**

R3 (10kΩ) and R4 (22kΩ) form a voltage divider. The FG signal passes through R3
and is divided by the R3/R4 ratio before reaching the conditioning node:

```
V_node = V_FG × R4 / (R3 + R4)
       = V_FG × 22 / (10 + 22)
       = V_FG × 0.6875
```

For a 5.5V FG high level: V_node = 5.5 × 0.6875 = **3.78V**
For a 6.0V FG high level: V_node = 6.0 × 0.6875 = **4.13V**

These values exceed the 3.6V STM32 absolute maximum. The divider alone is
insufficient — the clamp diodes provide the final protection.

**The R4 value must be recalculated after measuring the actual FG swing on the
specific unit. If the measured swing is lower than 5.5V, R4 can be reduced to
provide a wider noise margin below 3.3V. If it is higher, R4 must be increased to
ensure the divided voltage does not exceed 3.3V before the clamp operates.**

The general formula for choosing R4 given a target conditioned voltage V_target and
measured swing V_swing:

```
R4 = R3 × V_target / (V_swing - V_target)
```

For a target of 3.0V (comfortable margin below 3.3V) with a measured 5.5V swing:
```
R4 = 10kΩ × 3.0 / (5.5 - 3.0) = 10kΩ × 1.2 = 12kΩ → use 12kΩ standard value
```

**D2 and D3 — BAT54 Schottky Clamp Diodes**

D2 is connected with its anode at the conditioning node and its cathode to VDD
(3.3V). D3 is connected with its anode to GND and its cathode to the conditioning
node. These form a **voltage clamp** that prevents the node from rising above
VDD + Vf_D2 or falling below GND - Vf_D3, where Vf is the Schottky forward voltage
(approximately 0.3V for BAT54 at low currents).

Effective clamp range: approximately −0.3V to +3.6V at the conditioning node.

The clamp diodes serve two purposes:

First, they provide **hard protection** against any FG swing that exceeds the divider
calculation. If the battery is fresh and the B+1 rail is higher than expected, or
if the pull-up resistor value is different from the assumed value, the divided
voltage might exceed 3.3V. The clamp prevents it from exceeding 3.6V regardless.

Second, they provide **transient protection** against high-frequency spikes on the
FG line. The optical sensor and its pull-up wiring can act as an antenna for
electrical noise from the motor brushes and from the switching converter. Sharp
transients that pass through the divider are clamped immediately by the diodes,
before they can reach PA0.

The current through D2 when clamping is limited by R3 to a maximum of
(V_swing - 3.6V) / R3. For a 6V swing: (6.0 - 3.6) / 10,000 = 0.24mA. This is
well within the BAT54's 200mA continuous rating and the STM32's 5mA injection
current limit for clamped inputs.

**C7 — 10pF C0G Filter Capacitor**

C7 forms a low-pass RC filter with R4 (and with R3 in parallel when the source is
driving low). The corner frequency of the R4/C7 filter is:

```
f_corner = 1 / (2π × R4 × C7)
         = 1 / (2π × 22,000 × 10×10⁻¹²)
         = 724 kHz
```

At the nominal FG frequency of 2500 Hz, the filter has negligible effect — the
signal passes with less than 0.001dB of attenuation. At the motor brush noise
frequencies (typically 10kHz to 1MHz), the filter provides 20-40dB of attenuation.
This ensures the input to TIM2_CH1 is a clean, well-defined pulse rather than a
noisy signal that could cause spurious edge captures.

C0G (NP0) dielectric is specified because of its extremely low temperature
coefficient and absence of DC bias-dependent capacitance variation. In this position,
a standard X5R or X7R capacitor would also be acceptable — the 10pF value is small
enough that dielectric type has negligible practical effect.

### 1.3 Signal at PA0 After Conditioning

After the divider and clamp network, the signal at PA0 is:
- Logic HIGH: approximately 2.5V to 3.0V (depending on FG swing and R4 choice)
- Logic LOW: approximately 0.1V to 0.2V (phototransistor still pulls close to GND)
- Rise time: slightly degraded by the R/C network, but still well within TIM2's
  input capture requirements
- Noise: motor brush interference attenuated by C7 and the clamping action of D2/D3

The STM32 PA0 input threshold (Schmitt trigger) VIH is typically 0.7 × VDD = 2.31V.
A conditioned high level of 2.5-3.0V is comfortably above this threshold.

### 1.4 TIM2 Input Capture

PA0 is configured as TIM2_CH1 in input capture mode. On every rising edge of the
conditioned FG signal, TIM2 hardware automatically:
1. Copies the current 32-bit counter value into CCR1
2. Sets the CC1IF flag in TIM2_SR
3. Triggers the TIM2 interrupt if CC1IE is set in TIM2_DIER

The period between edges is computed in the ISR as `CCR1_now - CCR1_previous`. The
subtraction is performed in unsigned 32-bit arithmetic, which handles counter
wraparound correctly — if the counter rolled from 0xFFFFFFFF back to 0 during the
measurement interval, the subtraction still gives the correct period in ticks due
to modular arithmetic.

---

## 2. Motor Drive Path (PA6 → J1 Pin 2: Q601_BASE)

This signal travels in the opposite direction — from the STM32 to the motor drive
transistor Q601 in the WM-D6C.

### 2.1 The Motor Drive Chain in the WM-D6C

The capstan motor M901 is driven through a two-stage transistor circuit:

```
B+3 rail (10.8V from MT3608 boost) ──→ Q704 collector/emitter ──→ M901 one terminal
                                                                         │
                                        Q601 collector/emitter ──→ M901 other terminal
                                        Q601 emitter connected to B+3 via Q703/Q704 chain
                                        Q601 base ←── DSR-1 output (J1 pin 2)
```

Q601 is the off-board WM-D6C motor-control device driven from the former CX20084
output node. The primary unit is the surface-mount C11-494-12 board, so the exact
Q601 package/marking and pinout must be physically confirmed before installation.
The working electrical model is a PNP-style motor-current control path whose base
drive is controlled by the former IC601 output node.

**PNP transistor convention**: Q601 conducts when its base is at a lower voltage
than its emitter. More precisely, it conducts when the base-emitter junction is
forward biased — when Vbase is approximately 0.6V lower than Vemitter. Since the
emitter is connected to the B+3 rail (approximately 10.8V via the mode switch
chain), the base must be held below approximately 10.2V to keep Q601 conducting.
Pulling the base toward the emitter voltage (10.8V) turns Q601 off. Pulling the
base further down (toward 9V or less) increases base current and drives Q601 harder,
increasing motor current and speed.

The critical bench question is: **what voltage range does the Q601 base actually
operate in during normal playback?** This confirms R9 sizing, safe-off margin, and
the PWM-duty-to-speed sign on the actual unit.

### 2.2 Direct DAC Drive — Rejected

PA4/DAC1_OUT1 is not used for motor drive. The Rev A design uses TIM3 PWM on PA6
through an RC filter and NPN level-shift stage. PA4 may remain no-connect or spare.

### 2.3 Committed PWM + NPN Level-Shift Drive

This configuration is the committed Rev A output topology. Bench measurement still
confirms R9 sizing and the duty-cycle-to-speed sign.

```
PA6 (TIM3_CH1 PWM)
    │
   [R7: 1kΩ]
    │
    ├──[C8: 100nF to GND]    ← PWM-to-analog RC filter
    │
    [RC node: analog control voltage]
    │
   [R8: 10kΩ]
    │
   Q_LS base (MMBT3904 NPN)
    │
   Q_LS emitter → GND
   Q_LS collector
    │
   [R9: 100kΩ pullup to B+1]
    │
   J1 Pin 2 → Q601 Base
```

**PWM to analog conversion (R7, C8)**: TIM3 channel 1 on PA6 generates a PWM signal
at approximately 25kHz with variable duty cycle. The RC low-pass filter (R7 1kΩ,
C8 100nF) converts this to a smooth DC analog voltage. The filter corner frequency
is:

```
f_corner = 1 / (2π × R7 × C8)
         = 1 / (2π × 1000 × 100×10⁻⁹)
         = 1.59 kHz
```

At 25kHz PWM frequency, the filter provides:
```
Attenuation = 1 / √(1 + (f/f_corner)²)
            = 1 / √(1 + (25000/1590)²)
            = 1/15.7 = 0.064 (−24dB)
```

The residual ripple on the analog voltage is approximately 6.4% of the PWM swing
— for a 3.3V swing, about 210mV peak-to-peak ripple. This is acceptable because
the RC filter at Q601's base-emitter junction provides additional filtering, and
the motor's mechanical time constant (tens of milliseconds) further averages out
any remaining ripple.

**Q_LS (MMBT3904 NPN level-shift transistor)**: The analog voltage at the RC node
drives the base of Q_LS through R8 (10kΩ). When the control voltage is above
approximately 0.7V, Q_LS conducts and pulls its collector toward GND. The collector
is connected to R9 and from there to the Q601 base.

**R9 (100kΩ pullup to B+1)**: When Q_LS is off (RC node voltage low, motor slow),
R9 pulls the Q601 base toward B+1 (6V). Since Q601's emitter is at B+3 (10.8V),
a 6V base voltage provides 6V - 10.8V = -4.8V base-emitter bias — Q601 is off,
motor stops. This is the correct fail-safe behaviour.

When Q_LS is on (RC node voltage high, motor fast), Q_LS pulls the Q601 base toward
GND through its collector saturation voltage (~0.1V). The base-emitter voltage
becomes 0.1V - 10.8V = -10.7V — Q601 conducts heavily.

The control range is therefore determined by how much Q_LS conducts. At partial
conduction, the Q601 base sits at an intermediate voltage determined by the current
division between R9 (pulling toward B+1) and Q_LS (pulling toward GND). The servo
loop adjusts the PWM duty cycle to set this intermediate voltage to whatever value
produces the correct motor speed.

**Boot safety**: The TIM3 PWM output is in its reset state (output
low, 0% duty cycle) until the firmware initialises it. With 0% duty cycle, the RC
node is at 0V, Q_LS is off, and R9 pulls Q601's base toward B+1 — motor is off.
Safe.

### 2.4 The Forward Motor Drive Path Summary

```
STM32 servo output (PA6 PWM)
    │
    [R7/C8/Q_LS/R8/R9]
    │
J1 Pin 2 → Q601 base (on WM-D6C main board)
    │
    [Q601 motor-control device — exact package/marking pending physical confirmation]
    │
Q601 collector → M901 capstan motor (one terminal)
    │
M901 other terminal ← Q703/Q704 switching → B+3 (10.8V from DSR-1 MT3608 boost)
```

The complete forward signal path from the servo algorithm's output value to actual
motor current spans the Servo Control Board, WM-D6C main board, and the motor
itself, with two signal conversion stages: PWM-to-filtered-control voltage on the
Servo Control Board, and voltage-to-current at the Q601 base node. The servo loop's
job is to adjust the first stage — the PWM duty cycle — to achieve
the motor speed that produces the target FG period.

---

## 3. Motor Enable Signal (J1 Pin 3: MOTOR_EN → PA5)

### 3.1 Signal Source

IC601 pin 7 on the WM-D6C main board is connected to the motor enable network.
During playback mode, this pin is held HIGH by resistor R605 connected to the B+1
rail. The voltage on this pin during playback is approximately 4.4V — close to B+1
but reduced by the divider formed by R605 and the impedance of the downstream
circuitry.

Q702 on the Auto-Off board pulls this pin LOW when end-of-tape is detected, or when
the Auto-Off timer expires. This signals the servo to stop driving the motor.

**Signal levels on the machine side:**
- Playback active: approximately 4.4V
- Auto-off triggered / tape ended: approximately 0.1V to 0.5V

**STM32 PA5 absolute maximum input voltage: 3.6V (VDD + 0.3V)**

Direct connection would apply 4.4V to a 3.6V-maximum pin. Conditioning required.

### 3.2 Voltage Divider

```
J1 Pin 3 (MOTOR_EN, ~4.4V during playback)
    │
   [R10: 10kΩ series]
    │
    ├──[R11: 22kΩ to GND]
    │
   PA5 (GPIO input)
```

The R10/R11 divider attenuates the 4.4V playback signal:

```
V_PA5 = 4.4V × R11 / (R10 + R11)
       = 4.4V × 22 / (10 + 22)
       = 4.4V × 0.6875
       = 3.025V
```

This is safely below the 3.6V absolute maximum and above the PA5 VIH threshold
of 0.7 × VDD = 2.31V.

In the off state:
```
V_PA5 = 0.3V × 0.6875 = 0.21V
```

This is safely below the VIL threshold of approximately 0.3 × VDD = 0.99V.

The divider values are confirmed correct for a 4.4V playback voltage. If the
measured playback voltage differs significantly from 4.4V, R10 and R11 should be
recalculated using:

```
R11 = R10 × V_target / (V_measured - V_target)
```

For V_target = 2.8V (comfortable margin, 0.5V below 3.3V) and various V_measured:

| Measured V | R10 | R11 (calculated) | R11 (standard) | PA5 voltage |
|---|---|---|---|---|
| 4.0V | 10kΩ | 23.3kΩ | 22kΩ | 2.75V ✓ |
| 4.4V | 10kΩ | 21.9kΩ | 22kΩ | 3.03V ✓ |
| 5.0V | 10kΩ | 12.7kΩ | 12kΩ | 2.73V ✓ |
| 5.5V | 10kΩ | 10.4kΩ | 10kΩ | 2.75V ✓ |

No clamp diodes are used on MOTOR_EN because the divider provides sufficient
attenuation at all realistic voltages. The signal transitions slowly (100ms
timescale for auto-off detection) and requires no fast transient protection.

### 3.3 Firmware Behaviour

PA5 is configured as a GPIO input with the internal pull-down disabled (the divider
provides a defined low state). The firmware checks PA5 on each servo loop cycle. A
LOW state (auto-off triggered) causes the servo output to ramp the motor drive
toward the off condition over approximately 50ms — a gradual stop rather than an
abrupt cutoff, preventing tape spill and mechanism stress.

---

## 4. Speed Reference ADC Inputs (J1 Pins 4, 5, 6 → PA1, PA2, PA3)

### 4.1 Signal Sources

Three potentiometers on the WM-D6C main board provide analog control voltages:

**RV601** (47kΩ cermet trimmer, rear panel): Connected in a voltage divider network
between B+1 and GND. The wiper produces a DC voltage somewhere in the range 0V to
approximately B+1 (6V), depending on the wiper position. This pot is factory-set
for the correct base speed calibration and is covered by a small rubber plug — it
is not intended for user adjustment.

**RV602** (20kΩ carbon slide potentiometer, front panel Speed Tune slider): Also
connected in a voltage divider between reference points in the servo circuit. The
user adjusts this to shift playback speed over a small range (typically ±2-3%) to
match tapes recorded on other machines.

**RV603** (47kΩ cermet trimmer): Sets the sensitivity of RV602. Its wiper controls
how much speed change is produced by full travel of the Speed Tune slider.

**Critical measurement requirement**: The exact voltage ranges at all three wipers
must be measured on the specific unit before finalising the ADC input connections.
If any wiper voltage exceeds 3.3V under any operating condition, voltage dividers
must be added before the ADC pins. The STM32 ADC input absolute maximum is VDD +
0.3V = 3.6V, and exceeding this damages the ADC input permanently.

### 4.2 Input Conditioning

For all three ADC inputs the conditioning approach is identical:

```
J1 Pin 4/5/6 (pot wiper)
    │
   [R_series: 100Ω]    ← Source impedance limit for ADC accuracy
    │
    ├──[D_hi: BAT54 anode to node, cathode to VDD 3.3V]  ← Overvoltage clamp
    ├──[D_lo: BAT54 anode to GND, cathode to node]       ← Undervoltage clamp
    │
   PA1/PA2/PA3 (ADC_IN1/IN2/IN3)
```

**R_series (100Ω series resistor)**: The STM32 ADC datasheet specifies a maximum
source impedance of approximately 10kΩ for full 12-bit accuracy at the 1 Msps ADC
clock. The pot wipers have source impedances of up to half the pot resistance
(23.5kΩ for RV601 at midpoint, 10kΩ for RV602 at midpoint) — well above the ADC
requirement.

However, the R_series 100Ω combined with the ADC's internal 5pF sample capacitor
creates an additional filter time constant of 100Ω × 5pF = 0.5ns — negligible.
The primary purpose of R_series is protection: it limits the current into the clamp
diodes if the pot wiper swings above 3.3V, preventing damage to both the diodes and
the ADC input.

The high pot source impedance means the ADC must use a sufficiently long sampling
time to allow the ADC's internal sample-and-hold capacitor to fully charge through
the pot's source impedance plus R_series. At the 64 MHz ADC clock with the maximum
239.5 cycle sampling time, this is approximately 3.7µs per conversion. This is more
than sufficient for the pot impedances involved, and the three-channel DMA scan
completes in approximately 11µs — fast enough for real-time servo loop updates.

**BAT54 clamp diodes**: As with the FG input, the clamp diodes provide hard voltage
limits on the ADC inputs. If a pot wiper rises above 3.3V + 0.3V = 3.6V, D_hi
conducts and clamps it. If it goes below −0.3V (impossible under normal conditions
but possible from static discharge), D_lo conducts.

The BAT54's low forward voltage (~0.3V) means the clamping threshold is 3.6V for
the high side — very close to the absolute maximum, but the current through R_series
limits the diode current to a safe level.

**If pot wipers exceed 3.3V**: Add a voltage divider (R_upper in series before
R_series, R_lower from node to GND) to reduce the maximum wiper voltage below 3.0V
at the ADC input. Recalculate the ADC reading-to-speed-offset mapping in the
firmware to account for the divider ratio.

### 4.3 ADC DMA Scan and Firmware Usage

The three ADC channels (PA1, PA2, PA3 as ADC_IN1, IN2, IN3) are configured for
continuous DMA conversion in a circular buffer. The ADC runs autonomously, keeping
the conversion results up to date in SRAM. The servo ISR reads the current values
from the DMA buffer on each execution without waiting for a conversion.

The conversion results are 12-bit values (0 to 4095) representing the input voltage
from 0V to VDD (3.3V). The firmware maps these to speed corrections:

```c
// RV601: base speed trim — shifts target period by a fixed calibration offset
int32_t rv601_trim = ((int32_t)adc_rv601 - 2048) * BASE_TRIM_SCALE / 2048;

// RV603: sets the speed range scalar for RV602
int32_t rv603_range = (int32_t)adc_rv603 * SPEED_RANGE_MAX / 4095;

// RV602: user speed tune — applies a signed offset scaled by RV603
int32_t rv602_offset = ((int32_t)adc_rv602 - 2048) * rv603_range / 2048;

// Final adjusted target period
uint32_t adjusted_target = TARGET_PERIOD + rv601_trim + rv602_offset;
```

The centering at 2048 (half-scale) assumes the pot wipers are biased to sit near
the midpoint of their range. If the measured wiper voltages show a different
operating center, the centering constant in the firmware should be adjusted to match.

---

## 5. Speed Tune Switch (S601 → PA7)

### 5.1 Signal Source

S601 is a slide switch on the front panel that enables or disables the Speed Tune
(RV602) function. In its original circuit, it switches RV602 in or out of the servo
reference network.

In the DSR-1 implementation, the firmware reads S601's state on PA7 and includes
or excludes the RV602 offset from the target period calculation based on this reading.

S601 is rewired to connect directly between VDD (3.3V) and PA7 during installation.
No level shifting or voltage divider is needed because the switch operates at 3.3V.

### 5.2 Conditioning

```
VDD (3.3V) ──[S601]── PA7
                       │
                      [Internal pull-down enabled in STM32]
```

PA7 is configured with its internal pull-down resistor enabled (approximately 40kΩ
to GND). When S601 is open (Speed Tune disabled), the pull-down holds PA7 at 0V —
logic LOW. When S601 is closed (Speed Tune enabled), VDD drives PA7 to 3.3V through
S601 — logic HIGH.

No external components are needed for this signal. The internal pull-down eliminates
the need for an external pull-down resistor, saving board space.

---

## 6. USB Data Path (PA11/PA12 and PA8/PA9)

### 6.1 USB D+ and D− (PA12 and PA11)

The USB 2.0 Full Speed differential data lines connect from the USB-C connector
through the USBLC6-2SC6Y ESD protection array to PA12 (USB_DP) and PA11 (USB_DM).

**USBLC6-2SC6Y ESD protection**: This device provides rail-to-rail ESD protection
for both data lines simultaneously. It clamps any ESD event on D+ or D− to within
the supply rails, protecting the STM32's USB PHY from the 15kV IEC 61000-4-2
contact discharge specification. The device's very low capacitance (2.5pF per line)
means it has negligible effect on the USB 2.0 signal integrity at 12 Mbit/s.

The USB signals are driven directly by the STM32's internal USB Full Speed
transceiver — no external resistors or components are needed. The STM32G0C1
datasheet (DS13560 Rev 6, Table 80) confirms the internal matching impedance of
36Ω on the output driver, satisfying the USB 2.0 specification's 45Ω line impedance
requirement.

**Routing requirement**: D+ and D− must be routed as a matched-length differential
pair on the PCB, with 90Ω differential impedance. Traces must be kept short (under
25mm) and away from the MT3608 switching node. Any imbalance between D+ and D−
trace lengths or impedances degrades the USB eye diagram and can cause enumeration
failures.

### 6.2 USB-C CC Lines (PA8 and PA9 — Variant A only)

The CC1 and CC2 lines of the USB-C connector connect to PA8 (UCPD1_CC1) and PA9
(UCPD1_CC2) of the STM32. These lines carry the USB Power Delivery signalling that
negotiates the 9V supply contract with the USB charger.

In Variant A, the IP2721 hardware PD trigger handles the actual power negotiation.
The STM32 UCPD peripheral monitors the CC lines to confirm that a valid PD contract
is in place before enabling the converter outputs. This provides a software-
controlled enable for the power supply — if no PD contract is detected within a
timeout period, the boost converter and motor supply are held off, preventing the
machine from starting with an insufficient supply voltage.

The CC lines require no level shifting — they operate at USB PD signal levels
(0V to 1.1V for BMC signalling) which are well within the STM32's 3.3V input range.

---

## 7. Power Rails (J1 Pins 7 and 8)

### 7.1 B+1 Rail (J1 Pin 7)

J1 pin 7 connects the Servo Control Board to the WM-D6C's main 6V B+1 supply rail. Its
direction depends on the build:

- **Wall variants (A and B):** the rail is *driven by the Servo Control Board* — the board's
  regulated TPS63070 output is fed into the machine through this pin, powering all the
  machine's circuits. J1 pin 7 is an **output**.
- **Battery build (two-board):** B+1 is generated on the **Power Board**
  (behind the battery bay) and injected at the machine's original battery-terminal
  node, ahead of the S901 power switch. It propagates through the machine's B+1 net
  and reaches the Servo Control Board through this same pin. J1 pin 7 is therefore an
  **input** in this build — the board receives B+1 and its own power-input zone is
  unpopulated. Because injection is on the battery side of S901, the front-panel power
  switch still commands the whole machine.

The batteries are never wired to J1 pin 7 directly: their raw 3.7V would be far below
the 6V the rail expects. The Power Board's charger, protection, and boost convert
the pack to a regulated 6.0V first (see Power Supply Design §7).

**Protection**: A 1N5819 Schottky diode in series at J1 pin 7 (on the Servo Control Board)
prevents reverse current flow in the event of a power-sequencing issue. The 0.3V
forward drop at operating current means the Servo Control Board sees approximately 5.7V on B+1
instead of 6.0V — within operating specification. In the battery build, where J1 pin 7
is input-only, this Schottky may be retained as input reverse-protection or omitted,
since its original anti-back-feed purpose (Servo Control Board driving the rail out) no longer
applies.

**Battery-build board-to-board link.** Moving the pack and B+1 generation onto the
Power Board adds a small interface between the Power Board and the Servo Control Board,
separate from the J1 machine harness: B+1 and GND (the power path, via the battery
terminals), **VBAT_SENSE** (pack voltage to an STM32 ADC, for the fuel gauge — see
§14.4), the BQ24074 **CHG_STAT/PGOOD** status lines (to an STM32 GPIO, for the charge
animation), and optionally the **USB D+/D−** pair if USB-CDC tuning is routed from the
Power Board's USB-C rather than a separate bench header on the Servo Control Board.

### 7.2 GND (J1 Pin 8)

J1 pin 8 connects the Servo Control Board GND plane to the WM-D6C chassis ground. All voltage
measurements and signal levels described in this document are referenced to this
common ground.

**Ground routing on the PCB**: The GND connection at J1 pin 8 must have a low-
impedance return path to the exposed VSS pad of the STM32, to the negative terminal
of all bypass capacitors, and to the return path of the MT3608 boost converter.
A dedicated GND polygon pour on the board bottom layer provides this low-impedance
connection. High-current paths (motor supply return, boost converter switching
return) should not share traces with the analog signal return paths (ADC inputs,
filtered PWM/control-output reference).

---

## 8. Signal Chain Summary Table

| Signal | Source | Source Level | Conditioning | STM32 Pin | Safe Range |
|---|---|---|---|---|---|
| FG_RAW | FG901 open-collector + pull-up | 0 to ~6V swing | R3/R4 divider, BAT54 clamp, C7 filter | PA0 / TIM2_CH1 | 0 to 3.0V after conditioning |
| Q601_BASE | STM32 PWM+RC+NPN | 0 to 3.3V PWM → level shifted | R7, C8 RC, Q_LS NPN, R8, R9 | PA6 / TIM3_CH1 | N/A (output) |
| MOTOR_EN | IC601 pin 7 / R605 | 0 to ~4.4V | R10/R11 divider | PA5 / GPIO | 0 to 3.03V after divider |
| RV601_WIPER | 47kΩ pot wiper | 0 to ~6V (measure) | 100Ω + BAT54 clamp | PA1 / ADC_IN1 | Must be ≤3.3V — verify |
| RV602_WIPER | 20kΩ pot wiper | 0 to ~6V (measure) | 100Ω + BAT54 clamp | PA2 / ADC_IN2 | Must be ≤3.3V — verify |
| RV603_WIPER | 47kΩ pot wiper | 0 to ~6V (measure) | 100Ω + BAT54 clamp | PA3 / ADC_IN3 | Must be ≤3.3V — verify |
| S601 | Slide switch | 0 to 3.3V (rewired) | None — direct | PA7 / GPIO | 0 to 3.3V |
| USB D+/D− | USB-C connector | USB 2.0 FS levels | USBLC6-2SC6Y ESD | PA12/PA11 | Handled by USB PHY |
| CC1/CC2 | USB-C connector | USB PD BMC levels | Direct | PA8/PA9 / UCPD | 0 to 1.1V |
| B+1 | Battery / converter | 6V DC | Schottky diode | J1 pin 7 → VIN | 5.5V to 6.5V |

---

## 9. Using This Document with the Schematic

When reviewing the KiCad schematic, use this document to verify:

1. **Every net connected to a machine-voltage signal** has appropriate conditioning
   before reaching an STM32 pin — check the divider ratio calculation is correct
   for the measured swing voltage.

2. **Every STM32 output net** has a safe path to the machine load — verify R5 or
   R7 are present, verify the boot-safe pullup is in place.

3. **Clamp diodes** are present on FG_RAW and all three ADC inputs — verify they
   are correctly oriented (D_hi cathode to VDD, D_lo anode to GND).

4. **The boot-safe condition** holds for every output: motor off, no uncontrolled
   drive before firmware initialisation.

5. **USB differential pair** is matched-length, correctly terminated, and routed
   away from the MT3608 switching node.

---

## See Also

- [Original Servo Circuit](original-servo-circuit.md) — signal sources in the WM-D6C
- [Digital PLL Servo](digital-pll-servo.md) — how the STM32 processes these signals
- [Bench Measurements](../bench-measurements/) — measured values for SN72795
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — complete electrical specification

---

## 10. Noise and Interference Analysis

### 10.1 The Interference Problem on a Mixed-Signal Board

In wall-input variants, the Servo Control Board places two fundamentally different electrical environments on
the same small PCB: a switching power converter (MT3608, switching at 1.2 MHz,
handling currents up to 500mA) and a precision measurement/control system (12-bit
ADC reading pot wipers, PWM motor control, FG pulse timing sensitive to
nanosecond-level jitter). On a larger board these would be separated by distance
and ground plane partitioning. On a 30×22mm board they are millimetres apart and
share a common power supply and ground structure.

Understanding how noise couples between these domains — and how to prevent it from
doing so — is essential for achieving the ADC accuracy and servo timing stability
the design requires.

### 10.2 MT3608 Switching Noise Sources

The MT3608 boost converter produces noise through three mechanisms:

**Switch node voltage transitions**: The SW pin of the MT3608 switches between
approximately 0V (switch on, inductor charging) and approximately 10.8V + diode
forward voltage (switch off, inductor discharging into output). This transition
occurs at 1.2 MHz with a rise and fall time of approximately 5-10ns. The dV/dt at
this node is on the order of (10.8V / 7ns) = 1.5 V/ns. Any capacitive coupling
between the SW node trace and nearby signal traces will inject a proportional noise
current into those traces.

The coupling capacitance between two parallel PCB traces of width W, length L,
separated by distance D on a 1.6mm FR-4 board is approximately:

```
C_coupling ≈ ε₀ × εᵣ × W × L / D
           ≈ 8.85×10⁻¹² × 4.4 × W × L / D   (F, dimensions in metres)
```

For a 0.3mm wide trace, 5mm long, 0.5mm away from the SW node:
```
C_coupling ≈ 8.85e-12 × 4.4 × 0.3e-3 × 5e-3 / 0.5e-3
           ≈ 0.12 pF
```

The noise voltage induced on the victim trace from a dV/dt of 1.5 V/ns through
0.12pF into a 10kΩ source impedance (R3 in the FG divider):

```
V_noise = C_coupling × dV/dt × Z_source
        = 0.12e-12 × 1.5e9 × 10,000
        = 1.8V peak
```

This is catastrophic — a 1.8V noise spike on the FG input would trigger spurious
edge captures in TIM2 and completely destroy servo accuracy. This calculation
illustrates why the SW node trace must be treated as the most critical routing
constraint on the board.

**Inductor magnetic field**: The boost inductor L1 carries a triangular current
waveform with a peak-to-peak ripple of approximately 100-200mA at 1.2 MHz. This
changing current creates a magnetic field that couples into nearby loops. The
induced voltage in a nearby loop of area A at distance r from the inductor is:

```
V_induced = μ₀/(2π) × (dI/dt) × A / r
```

For dI/dt = 200mA / (0.4µs) = 500 A/s, a 1cm² loop area, at 5mm from the inductor:
```
V_induced ≈ 2×10⁻⁷ × 500 × 1×10⁻⁴ / 5×10⁻³
           ≈ 2µV
```

Magnetic coupling from the inductor is therefore negligible at typical trace loop
sizes and separation distances. Capacitive coupling from the SW node trace is the
dominant interference mechanism and must be controlled by layout.

**Input and output capacitor ESR noise**: The ceramic capacitors on the MT3608 input
and output have very low ESR (< 5mΩ for X5R ceramic), which means switching
transients are absorbed quickly. This is a benefit — ceramic capacitors at both
the input and output of the boost converter minimise the voltage spikes on the B+1
and B+3 supply rails that would otherwise couple into analog circuitry.

### 10.3 Coupling Paths and Mitigation

**SW node to FG signal path**: The FG conditioning network (R3, R4, D2, D3, C7)
connects from J1 pin 1 to PA0. The traces forming this network must be routed as
far as physically possible from the MT3608 SW pin and its connecting trace to L1.
On a 30×22mm board, "as far as possible" may be only 5-8mm. The minimum acceptable
separation between the SW node trace and any FG signal trace is 2mm. The SW node
trace should be as short as possible (SW pin → L1 anode, under 3mm), as wide as
practical (0.5mm minimum for current capacity), and surrounded by ground plane
stitching to confine its fringing field.

**SW node to ADC inputs**: The pot wiper traces from J1 pins 4/5/6 to PA1/PA2/PA3
carry slowly-varying DC voltages and are read by the ADC. Any capacitively coupled
SW noise appears as a high-frequency voltage on these lines that the ADC sampling
process aliases into the DC measurement. With a 1.2 MHz switching frequency and
an ADC sample rate of approximately 90kSps (three channels in continuous mode), the
Nyquist criterion means any noise above 45kHz is aliased. At 1.2 MHz the aliased
noise appears as a DC offset of approximately:

```
V_alias ≈ V_noise_at_ADC × (sampling_time / switching_period)
        ≈ V_noise × (11µs / 0.83µs)  — approximate
```

The mitigation is to keep ADC input traces away from the SW node (same 2mm
minimum), and to rely on the 100Ω series resistors plus the ADC's internal sample-
and-hold capacitor (approximately 5pF) for additional filtering. The RC corner
frequency of 100Ω + 5pF is 318 MHz — this provides no filtering at 1.2 MHz.
If noise coupling into the ADC inputs is found to be problematic during bench
verification, adding a 4.7nF X5R capacitor from each ADC input to GND creates an
RC filter with a 337kHz corner frequency that attenuates 1.2 MHz noise by 11dB.

**Supply rail noise — B+1 to VDDA**: The STM32's analog supply VDDA is connected
to the same 3.3V rail as VDD. The MCP1700 LDO does an excellent job of rejecting
supply rail noise (44dB PSRR at 100Hz, decreasing at higher frequencies). However,
at 1.2 MHz the PSRR is much lower — approximately 10-15dB. This means switching
noise that reaches the 3.3V rail appears on VDDA attenuated by only a factor of 5.

The solution is the VDDA decoupling network: 1µF and 100nF ceramics placed directly
at VDDA pin 5, with the 100nF placed within 0.5mm of the pin. These capacitors
present a low impedance path to GND for high-frequency noise on VDDA, bypassing it
before it reaches the ADC reference. The ADC reference in the 32-pin package is
internally connected to VDDA — this decoupling is therefore the primary determinant
of ADC noise floor.

**PWM/control-node noise**: The motor output is generated by PA6 PWM through R7/C8
and the Q_LS level-shift stage. Keep the RC node and Q601_BASE trace away from the
MT3608 switching loop and USB differential pair. If bench measurements show ripple
or motor-speed jitter, adjust the RC filter and layout before changing firmware gains.

### 10.4 Layout Rules Derived From Noise Analysis

These rules are derived directly from the noise analysis above and must be observed
in the KiCad layout:

**Rule 1 — SW Node Isolation**: The MT3608 SW pin trace must be as short as
possible, routed away from all signal traces, and surrounded by ground stitching
vias on both sides. Maximum length 3mm. Minimum clearance to any signal trace 2mm.
Do not route signal traces under the inductor L1.

**Rule 2 — Analog Signal Zone**: All analog signal traces (FG conditioning, ADC
inputs, filtered PWM/control output) must be routed in a dedicated zone on the PCB that does not
overlap with the switching converter circuitry. Place the MT3608 and its associated
components in one corner of the board, and the signal conditioning network and STM32
analog pins in the opposite region.

**Rule 3 — Decoupling Capacitor Placement**: The STM32G0C1KCU6 UFQFPN32 GP package
has combined VDD/VDDA and VSS/VSSA pins and no separate VDDIO2 pin. Place the
combined VDD/VDDA decoupling network within 0.5mm of pin 4. Capacitors placed further
away have their bypass effectiveness reduced by the inductance of the connecting trace.

**Rule 4 — Ground Pour Connectivity**: All decoupling capacitor GND connections
must return directly to the exposed VSS pad of the STM32 via the bottom copper
GND pour, not through a shared trace that also carries switching converter return
currents. See Section 13 for the complete ground topology.

**Rule 5 — J1 Connector Placement**: J1 should be placed so that the signal traces
from its pins to the STM32 are as short as possible. Longer traces have more
opportunity to pick up noise from the switching converter. Placing J1 near the
STM32, on the opposite side of the board from the MT3608, satisfies this
requirement.

---

## 11. Timing and Speed Range Analysis

### 11.1 Normal Operating Range

At correct tape speed (4.75 cm/s), the FG pulse rate is FG_TARGET_HZ — a value
between approximately 2000 Hz and 3000 Hz depending on the number of slots in the
disc and the motor's mechanical arrangement. This must be measured on the specific
unit. For this analysis we use 2500 Hz as the nominal value.

At 2500 Hz, one FG period is:
```
T_FG = 1 / 2500 = 400µs
TARGET_PERIOD = 64,000,000 × 400µs = 25,600 ticks
```

The ISR executes every 400µs and must complete well within this period to avoid
missing the next FG edge. The ISR execution time is approximately 20-30 clock
cycles at 64 MHz = 300-470ns. This leaves 399,530ns = 99.9% of the available time
as margin. The servo timing budget is extremely comfortable at normal operating
speed.

### 11.2 Speed Tune Range

The Speed Tune slider RV602 modifies the adjusted target period. If the speed range
is ±3% (typical for WM-D6C Speed Tune):

```
Adjusted target range:
  Maximum speed (+3%): TARGET_PERIOD × 0.97 = 24,832 ticks  (2577 Hz)
  Minimum speed (−3%): TARGET_PERIOD × 1.03 = 26,368 ticks  (2427 Hz)
```

At the maximum speed setting, FG pulses arrive every 388µs. ISR timing budget
reduces to 387,530ns — still 99.9% margin.

The firmware must ensure that the rv601_trim and rv602_offset terms in the adjusted
target calculation cannot drive the adjusted target below a minimum safe value. A
reasonable minimum is half the nominal TARGET_PERIOD (motor running at 2× speed):
```
ADJUSTED_TARGET_MIN = TARGET_PERIOD / 2 = 12,800 ticks
```

Below this value the servo is chasing an unreachable target (the motor cannot
physically run that fast) and the integral accumulator will wind up without bound.
The anti-windup clamp on the integral prevents runaway, but an explicit minimum
on adjusted_target provides belt-and-suspenders protection.

### 11.3 Motor Spin-Up Phase

When the play button is pressed, the motor starts from rest. During the spin-up
phase (approximately 200-400ms for the WM-D6C flywheel to reach operating speed),
the FG pulse period starts very long and decreases toward TARGET_PERIOD.

At the very start of spin-up, the motor may be running at only 10% of correct
speed. The FG period is then approximately 256,000 ticks (25× TARGET_PERIOD). This
is a very large positive error. The proportional and integral terms both drive the
PWM/output value toward the maximum-drive clamp (`DAC_MIN` in current firmware
naming), which is the intended response under the current sign convention.

**Integral accumulator during spin-up**: The integral term accumulates the sum of
all errors. During a 300ms spin-up with an average error of +200,000 ticks running
at 2500 Hz update rate, the integral accumulates:

```
Integral = 200,000 ticks × 2500 Hz × 0.3s = 150,000,000 tick-periods
```

With KI_Q16 = 524, the integral term contributes:
```
KI contribution = (524 × 150,000,000) >> 16 = 1,196,289 output counts
```

This far exceeds the 12-bit output range of 0-4095. Without the anti-windup clamp, the
integral would take a very long time to unwind after the motor reaches speed, causing
severe overshoot and oscillation. The INTEGRAL_LIMIT clamp prevents this —
recommended value is TARGET_PERIOD × 50 = 1,280,000, which limits the maximum
integral contribution to approximately 10,000 output counts — still larger than the
PWM/output range, but the output clamp handles this. The integral will unwind within
approximately 2-3 FG periods (1-2ms) after the motor reaches speed, giving a
clean, non-oscillating lock.

**A note on spin-up accuracy**: During spin-up, the servo is running open-loop at
maximum motor drive — it cannot control the speed, only observe it. The transition
from open-loop spin-up to closed-loop speed regulation happens naturally as the
error decreases to a range where the proportional term's output falls within the
PWM/output range. There is no explicit mode switch — the PI algorithm handles it
continuously. This is one of the advantages of a PI servo over the original PLL
implementation, which had a distinct capture/lock sequence.

### 11.4 Measurement System Speed Limits

**Maximum measurable FG frequency**: The TIM2 input capture ISR must complete within
one FG period to avoid missing edges. The ISR takes approximately 30 cycles = 470ns
at 64 MHz. The maximum FG frequency at which the ISR can keep up is therefore:

```
F_max = 1 / (ISR_time + interrupt_latency)
      = 1 / (470ns + 235ns)  — Cortex-M0+ interrupt entry/exit = 15 cycles each
      = 1 / 705ns
      = 1.42 MHz
```

The maximum practical motor speed is limited by the mechanics and the motor's
maximum RPM. For the WM-D6C capstan motor, the maximum speed in fast-forward mode
is approximately 10-20× playback speed, giving a maximum FG frequency of
approximately 25,000-50,000 Hz. This is well within the ISR's capability.

However, the servo loop does not run during fast-forward — the Q703/Q704 mode
switching circuit disconnects the servo from the motor during FF/REW. The TIM2
input capture should be disabled or masked during these modes to prevent the ISR
from running at high rates unnecessarily. The MOTOR_EN signal (PA5) provides the
flag to suppress servo operation during non-playback modes.

**Minimum measurable FG frequency**: The 32-bit TIM2 counter at 64 MHz rolls over
every 2^32 / 64,000,000 = 67 seconds. At motor speeds below 1/67 of the normal
operating frequency (approximately 0.015 Hz), the period counter would overflow
between edges. In practice the minimum meaningful speed is when the motor first
starts producing regular pulses — approximately 10-50% of operating speed — and the
period is 10-50× TARGET_PERIOD. The 32-bit counter handles this comfortably.

**Period measurement quantisation**: Each timer tick represents 1/64,000,000 seconds
= 15.625ns. At TARGET_PERIOD = 25,600 ticks, one tick represents a fractional
speed error of 1/25,600 = 0.0039%. This is approximately 12.6× better than the
target specification of 0.05% wow and flutter — the digital measurement system is
not the limiting factor in servo accuracy.

### 11.5 The Speed Tune Pot Update Rate

The ADC scans three channels in continuous DMA mode. At the ADC clock frequency
of 64 MHz / 16 = 4 MHz, with 239.5 sampling cycles plus 12.5 conversion cycles
= 252 cycles per conversion, and three channels, the total scan cycle time is:

```
T_scan = 3 × 252 / 4,000,000 = 189µs
```

The pot wiper voltages are updated approximately 5,300 times per second — more than
twice the servo ISR rate. The servo loop always sees fresh pot readings. The speed
tune response is essentially instantaneous from the user's perspective.

---

## 12. ADC Accuracy Analysis

### 12.1 The Source Impedance Problem

The STM32G0C1 ADC has an internal sample-and-hold capacitor CADC of approximately
5pF (specified in DS13560 Rev 6, Figure 29 and surrounding text). To achieve full
12-bit accuracy, the source must be able to charge this capacitor to within 0.5 LSB
during the sampling time.

The charging time constant is τ = (R_source + R_ADC) × C_ADC, where R_ADC is the
ADC's internal switch resistance (approximately 6kΩ from the datasheet). The time
required to charge to 12-bit accuracy (within 1/2^13 = 0.012% of final value) is:

```
t_charge = ln(2^13) × τ = 9 × (R_source + R_ADC) × C_ADC
```

For the worst-case source impedance of RV601 at midpoint (47kΩ/2 = 23.5kΩ) plus
R_series (100Ω):

```
t_charge = 9 × (23,500 + 100 + 6,000) × 5×10⁻¹²
         = 9 × 29,600 × 5×10⁻¹²
         = 1,332ns = 1.33µs
```

The maximum sampling time in the STM32G0C1 ADC is 239.5 cycles at the ADC clock
of 4 MHz (16 MHz / 4 prescaler) = 59.9µs. This is more than 44× longer than the
required charging time. Full 12-bit accuracy is achievable even with the highest
pot source impedances.

**However**, the STM32G0C1 datasheet specifies a maximum external source impedance
of 10kΩ for full ADC accuracy (DS13560 Rev 6, Section 5.3.18). The pot wipers
can present source impedances up to 23.5kΩ — more than twice this limit. At high
source impedances, parasitic effects (leakage currents, input capacitance variation)
degrade accuracy beyond what the sampling time alone determines.

The practical consequence: at 23.5kΩ source impedance, the effective ADC resolution
is approximately 10-11 bits rather than 12. The voltage measurement error at 12-bit
scale is approximately 2-4 LSB. For the servo loop's use of these measurements as
speed trim offsets, this translates to a speed accuracy limitation of:

```
Trim resolution = 4 LSB × (trim_range / 4096)
```

For a trim range of ±200 ticks on TARGET_PERIOD, 4 LSB corresponds to:
```
Trim resolution = 4 × (400 / 4096) = 0.39 ticks = 0.0015% speed accuracy
```

This is negligible compared to the 0.05% specification. The source impedance
accuracy limitation does not affect the system's speed accuracy.

### 12.2 Oversampling for Improved Accuracy

If higher effective ADC resolution is desired (for example, to provide finer Speed
Tune granularity), oversampling can be used. The STM32G0C1 ADC has hardware
oversampling support that averages multiple conversions and provides up to 16-bit
effective resolution.

For 4× oversampling (averaging 4 conversions per result), the effective resolution
improves by 1 bit (to approximately 11-12 bits effective at high source impedances).
For 16× oversampling, effective resolution improves by 2 bits. The tradeoff is
reduced sample rate:

```
Effective sample rate with N-times oversampling = 5,300 Hz / N
```

For 16× oversampling: 5,300 / 16 = 331 Hz. The pot wiper voltage is updated 331
times per second — still far faster than any human Speed Tune adjustment. Oversampling
is recommended for the pot readings and can be enabled with a single register write
in the ADC configuration.

### 12.3 ADC Reference Accuracy

The STM32G0C1 in the 32-pin UFQFPN32 package has no VREF+ pin. The ADC reference
is internally connected to VDDA. The absolute accuracy of the ADC voltage measurement
therefore depends on the stability and accuracy of the 3.3V MCP1700 output.

The MCP1700 output voltage accuracy is ±2% over temperature (DS20001826F). This
means the full-scale ADC reference (VDDA = 3.3V) has an absolute accuracy of
approximately ±66mV. This affects the absolute voltage measurement but not the
relative accuracy (linearity) of measurements within the ADC range.

For the servo loop's use of ADC readings as speed corrections, absolute accuracy
is irrelevant — only relative changes matter. Moving RV602 from one position to
another produces a proportional change in ADC reading, which produces a proportional
change in speed offset. The VDDA variation affects the proportionality constant
slightly, but this is accounted for in the bench calibration of the speed range.

**The critical implication**: Any variation in the MCP1700 output voltage (due to
temperature, load changes, or LDO noise) appears as a variation in the ADC reference
and causes all three pot readings to shift proportionally. This would produce a
small, correlated speed error — all three pots apparently moved slightly. At 2%
VDDA accuracy and a 200-tick trim range, the worst-case correlated error is
±4 ticks = ±0.016% speed shift. This is within specification.

### 12.4 PWM Output Resolution

Motor drive is TIM3 PWM on PA6, filtered by R7/C8 and level-shifted by Q_LS. The
firmware currently keeps the older `DAC_*` names for its 12-bit output constants,
but the hardware output is PWM duty cycle. A 12-bit duty value gives 4096 command
steps before the analog RC filter and Q601 transfer curve. Final usable resolution,
clamps, and sign must be confirmed by the motor-drive characterization procedure.

---

## 13. Grounding and Return Current Architecture

### 13.1 Why Ground Architecture Matters

In a mixed-signal system, the ground plane is not a uniform zero-volt reference.
Every current that flows through the board must return to its source through the
ground plane, and the finite resistance and inductance of the ground plane means
that return currents create small voltage differences between different points on
the ground plane. If a high-current switching return path shares ground with a
sensitive analog measurement reference, the voltage created by the switching current
appears as noise in the analog measurement.

For the Servo Control Board, the problematic currents are:

**MT3608 switching current**: The inductor charges and discharges with a triangular
current waveform of approximately 200mA peak-to-peak at 1.2 MHz. The return current
flows from the MT3608 GND pin back to the input capacitor negative terminal. This
path must be kept separate from the analog ground paths.

**Motor rail return current**: The motor draws 100-200mA from the B+3 rail. This
current returns through the Servo Control Board GND plane to the J1 pin 8 harness connection.
This is a relatively steady DC current with switching transients when the motor
mode changes.

**ADC reference return current**: The ADC draws tiny currents (nanoamps to
microamps) from VDDA through the internal reference. These return through VSSA
(the STM32's analog ground, internally connected to VSS in the 32-pin package).
Any voltage drop on the ground plane between the decoupling capacitor returns
and the ADC measurement reference point appears directly as measurement error.

### 13.2 The Correct Ground Topology

The recommended ground topology for the DSR-1 PCB is a **split return path** on a
single shared ground plane. This is distinct from a split ground plane (which would
require careful bridge connections and is generally problematic on small boards).
Instead, the topology is achieved by routing return currents such that high-current
paths and sensitive analog paths use different routes through the same GND copper
pour.

```
Conceptual current flow topology (not actual traces — all share GND pour):

[MT3608 GND] ──────────────────────────────────────────► [J1 Pin 8 GND]
                                                           (chassis GND)
[B+3 return] ─────────────────────────────────────────►

[MCP1700 GND] ──┐
[STM32 VSS pads]├──► [STM32 exposed VSS pad] ──────────►
[Decoupling caps]┘         (star point)
```

The key principle is that the **STM32 exposed VSS pad acts as the star point** for
all analog and digital supply returns. All decoupling capacitor negative terminals,
the MCP1700 GND pin, and the STM32 VSS pins connect to the exposed pad via short,
direct GND copper. The high-current paths (MT3608 return, motor rail return) connect
to the GND pour at a different physical location — ideally at the J1 connector end
of the board, as far as possible from the STM32.

### 13.3 Via Array Under STM32

The exposed VSS pad (pin 33 of the UFQFPN32) is the thermal and electrical ground
connection for the STM32 die. It must be connected to the bottom GND copper through
a via array. The recommended array is a 2×2 or 3×3 grid of 0.3mm drill / 0.6mm
annular ring vias, distributed evenly under the 3.5×3.5mm exposed pad.

These vias serve two purposes:
1. Low-impedance GND connection from the chip die to the bottom GND plane
2. Thermal path for any heat generated by the STM32 (minimal at these power levels)

The vias must be tented (covered) or plugged on the top side to prevent solder
wicking through the via holes during reflow, which would starve the exposed pad
solder joint.

### 13.4 Decoupling Capacitor Placement and Return Routing

Each decoupling capacitor has a power terminal connected to the combined VDD/VDDA
domain and a GND terminal. The GND terminal connection to the GND pour must be as short
as possible, and the GND pour return path from that terminal to the STM32 exposed
pad must not cross the MT3608 switching return path.

**Critical placement rules:**
- 100nF VDD decoupling caps: placed within 0.5mm of VDD pins, GND terminal via
  directly into GND pour
- 100nF VDDA cap: placed within 0.5mm of VDDA pin 5, GND terminal via into GND
  pour between the cap and the STM32 body, not on the far side
- no VDDIO2 capacitor: the selected GP package ties that domain internally and does
  not expose a separate VDDIO2 pin

### 13.5 MT3608 Ground Isolation

The MT3608's GND pin should connect to the GND pour through a short trace and via
that returns to the J1 connector end of the board. The input capacitor (10µF on
the MT3608 VIN pin) and output capacitor (22µF on VOUT) both have GND terminals
that must return through this same isolated path.

The MT3608 switching return loop — the physical loop formed by the switch node,
inductor, output capacitor, and GND pin — should be as small as possible in area.
The area of this loop determines the strength of the radiated magnetic field at
1.2 MHz. Minimising loop area means:

- Place the input capacitor physically adjacent to the MT3608 VIN and GND pins
- Place the output capacitor adjacent to the SS14 diode cathode (VOUT) and the
  MT3608 GND pin
- Keep the inductor between the SW pin and the diode anode as close as possible
  to both components

The recommended component placement order (reading the MT3608 switching loop from
VIN to VOUT) is: Input cap → MT3608 → Inductor → SS14 Diode → Output cap.

### 13.6 USB Ground and Shield

The USB-C connector body (shield/shell) connects to the board GND plane through
the connector's mounting tabs. These tabs should be connected directly to the GND
pour with short, low-impedance connections. Do not use ferrite beads between the
USB connector shell and GND — this is a common but incorrect practice that creates
ground resonances at USB signalling frequencies.

The USBLC6-2SC6Y GND pin must connect to the GND pour at the same physical location
as the USB connector GND, not through a long trace to a distant GND via. The ESD
clamp's effectiveness depends on a low-impedance ground return — a 10mm trace to
the nearest GND via adds approximately 8nH of inductance, which limits the clamp
speed significantly during a fast ESD transient.

### 13.7 Ground Pour Design Rules Summary

| Rule | Requirement | Reason |
|---|---|---|
| STM32 exposed VSS pad | Via array 2×2 or 3×3, 0.3mm drill, tented top | Low impedance die ground, thermal path |
| Decoupling cap GND returns | Via within 0.5mm of capacitor GND terminal | Minimise high-frequency bypass inductance |
| MT3608 switching loop | Component placement adjacent, loop area minimised | Minimise 1.2MHz radiated field |
| MT3608 GND return | Route away from STM32 analog supply area | Prevent switching return current through analog reference |
| USB connector shell | Direct to GND pour, no ferrite | ESD transient return path, no resonance |
| USBLC6 GND pin | Same physical location as USB connector GND | Fast ESD clamping |
| J1 connector GND (pin 8) | Direct to GND pour at connector | Motor return current enters at connector, not through signal area |
| Analog signal traces | Route in opposite board region from MT3608 | Minimise capacitive coupling from SW node |

---

## 14. Startup and Shutdown Sequences

### 14.1 Power-On Sequence

Understanding the sequence of events from the moment power is applied to the
moment the servo loop is running is important both for firmware design and for
verifying that no unsafe condition (motor running uncontrolled, signals at incorrect
levels) exists at any intermediate point.

**t = 0ms**: Power applied (USB-C connected or power switch closed).

**t = 0 to ~1ms**: Supply rails ramp up. The MCP1700 LDO output rises from 0V to
3.3V with a slew rate limited by the output capacitor (10µF). During this ramp:
- All STM32 pins are in reset state (outputs floating, inputs high-impedance)
- PA6 (PWM output) is high-impedance until configured
- The motor remains off because R9 pulls Q601_BASE toward the planned safe-off node

**t = ~1ms**: STM32 VDD reaches the POR (Power-On Reset) threshold (~1.6V). The
internal reset circuit releases. The STM32 begins executing from the reset vector
in flash.

**t = ~1ms to ~1.5ms**: Clock initialisation. HSI16 starts (~1µs), PLL locks
(~200µs). System clock transitions from HSI16 to 64 MHz PLL output.

**t = ~1.5ms**: Peripheral initialisation begins. GPIO, TIM2, TIM3 PWM, ADC, USB CDC
are configured in sequence. During GPIO initialisation:
- PA4 remains unused/no-connect for motor drive.
- PA5 is configured as GPIO input — motor enable monitoring begins immediately.
- PA6 is configured as TIM3 PWM with 0% duty cycle — Q_LS base
  is held low, motor remains off.

**t = ~2ms**: All peripherals initialised. Servo loop enters its first execution.
The first FG period measurement is taken. The PWM/output value is set based on the first
control calculation.

**t = ~2ms to ~300ms**: Motor spin-up phase. The servo drives the motor at maximum
output while monitoring the FG period. The period decreases from a large initial
value toward TARGET_PERIOD.

**t = ~300ms**: Motor reaches operating speed. FG period settles to TARGET_PERIOD ±
servo error. Speed lock is achieved.

The motor is never uncontrolled at any point in this sequence. R9 holds Q601_BASE in
the safe-off state until the firmware explicitly enables motor drive through PWM.

### 14.2 Power-Off Sequence

When power is removed (USB-C disconnected, power switch opened):

**t = 0**: VIN to MCP1700 collapses. The LDO output begins discharging through the
10µF output capacitor and the STM32's supply current (~9mA). The discharge time
constant is approximately 10µF × (3.3V / 9mA) = 3.7ms.

**t = 0 to ~3ms**: The STM32 continues executing with supply voltage decreasing from
3.3V. The Brown-Out Reset (BOR) threshold (programmable, recommend 2.8V) is not
reached for approximately 2ms. During this window, the servo loop continues running
and the motor is still being driven.

**t = ~2ms**: BOR threshold reached. STM32 enters reset. All GPIO outputs go to
high-impedance. R9 pulls Q601 base toward its safe state — motor drive is removed.

**Motor coasting**: M901 continues to rotate due to flywheel inertia for
approximately 1-2 seconds after motor drive is removed. This is normal and harmless.

**Tape protection**: The pinch roller releases from the capstan when the transport
mode switches — this is controlled by the mechanical transport linkage, not the
servo electronics. The tape is not in contact with the moving capstan during the
coast-down phase in pause or stop mode.

### 14.3 USB Hot-Plug Events (Variant A)

When the USB-C cable is connected or disconnected while the machine is in a running
state, the UCPD peripheral generates an interrupt. The firmware should handle the
disconnect interrupt by:

1. Setting the PWM/output value to the measured safe-off clamp immediately
2. Disabling the MT3608 boost enable pin (if implemented — this prevents the motor
   rail from being driven from a supply that may become unstable)
3. Setting a flag that holds the servo disabled until a new PD contract is confirmed

The reconnect/re-negotiation sequence takes approximately 100ms from cable insertion
to confirmed 9V PD contract. The machine should not resume transport during this
window.

### 14.4 Battery Insertion (Variant B, Battery-Powered Operation)

When the machine is operated from batteries with Variant B installed, the power-on
sequence differs in one important way: the input voltage ramps up more slowly as the
battery internal resistance charges the input capacitor network. The B+1 rail may
take 10-50ms to stabilise, depending on battery type and internal resistance.

The MCP1700 output follows the input with a delay determined by its transient
response. The STM32 will not begin executing until VDD reaches the POR threshold,
which occurs after B+1 has largely stabilised. No special handling is required.

**Low battery detection**: In the two-board battery build, B+1 is held at a regulated
6.0V by the Power Board's TPS63070 until the pack is nearly empty, so B+1 does *not*
sag with the cells the way the original raw-AA rail did. State of charge is instead
sensed on the Power Board's VBAT node and reported to the STM32 over the VBAT_SENSE
line (see the indicator note below). When the pack reaches its floor, the DW01 + FET
pack protection (and the cells' own PCMs) disconnect it; B+1 then collapses, the
MCP1700 drops out below ~3.5V input, and at approximately 2.8V supply the STM32 BOR
fires and the machine stops cleanly.

**Battery level indicator** (correction): the front-panel five-LED bar (D801–D805,
driven by IC801/CX10043, BATT selected by S801) originally indicated from the machine's
main rail. In the two-board battery design that rail is a regulated, constant 6.0V B+1,
so the stock battery indication (a single-LED brightness drive via Q801) is pinned at
full and no longer reflects the cells. The true state of charge lives on the VBAT node
*on the Power Board*, which the stock indicator circuit cannot see. The DSR-1
therefore re-references it: the STM32 reads VBAT through the **VBAT_SENSE** board-to-
board conductor, estimates state of charge from a LiPo open-circuit-voltage table, and
drives the LED bar directly as a true 1–5 segment fuel gauge only while `S801_BATT`
is asserted. In VU/non-BATT modes the MCU LED outputs must be released/high-Z so they
do not contend with the CX10043 circuitry. Charge animation comes from the BQ24074
CHG_STAT/PGOOD status. See Power Supply Design §7.8 for the divider, LED interface,
and firmware mapping.

---

## See Also

- [Original Servo Circuit](original-servo-circuit.md) — signal sources in the WM-D6C
- [Digital PLL Servo](digital-pll-servo.md) — how the STM32 processes these signals
- [Power Supply Design](power-supply-design.md) — MT3608 and MCP1700 design detail
- [Fixed-Point Arithmetic](fixed-point-arithmetic.md) — control algorithm mathematics
- [Bench Measurements](../bench-measurements/) — measured values for SN72795
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — complete specification
