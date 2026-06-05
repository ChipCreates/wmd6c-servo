# Power Supply Design

## Purpose of This Document

This document explains every aspect of the DSR-1 module's power supply architecture
— why each design decision was made, how each component was chosen, how the circuits
work quantitatively, and how they relate specifically to the requirements of the
Sony WM-D6C. It is written for the engineer who wants to understand, verify, or
modify the power supply, and for the contributor who wants to port the module to a
different machine with different power requirements.

The power supply is not an afterthought in this project. The original WM-D6C power
architecture had exactly one protection mechanism — none — and the consequence of
that omission is the single most common cause of failure in every surviving unit.
The DSR-1 power supply is designed with the explicit goal of making the same failure
mode physically impossible, while simultaneously replacing a 35-year-old potted
module of unknown internal condition with a documented, serviceable, component-level
design.

---

## 1. The WM-D6C's Original Power Architecture and Its Problems

### 1.1 What the Machine Needs

The WM-D6C's circuits require three distinct supply voltages derived from the
primary power source:

**B+1 (nominally 6V)**: The main supply rail. Powers the audio signal chain
(IC101, IC201 preamplifiers; IC301, IC302 op-amps; IC102 headphone amplifier),
the servo logic, the LED board, the Auto-Off board, and virtually everything on the
main PCB that is not the motor itself. When Sony designed this machine to run on
four AA alkaline cells in series, they chose a nominal operating voltage of 6V
because this is the fresh-battery voltage of four alkaline cells in series
(4 × 1.5V = 6V). As the batteries discharge toward 4V (four cells at 1V each), the
audio circuits operate at reduced supply voltage — the audio quality degrades
gracefully, but the servo system becomes increasingly difficult to control because
the FG sensor pull-up voltage also drops, changing the signal levels the servo IC
expects.

**B+3 (nominally 10.8V)**: The capstan motor supply. M901 is a small DC motor that
runs most efficiently at approximately 10.8V. This is higher than the battery
supply, so CP304 boost-converts B+1 to produce B+3. The 10.8V level was chosen by
Sony's engineers based on the specific motor's torque-speed curve — at this voltage
the motor produces the correct rotational speed for 4.75 cm/s tape transport through
the capstan mechanism's gear ratio, while the servo system has adequate headroom
above and below the operating point to apply corrective torque in both directions.

**Implicitly: approximately 3-5V for logic**: The CX20084 servo IC and the
MSM58141RS divider IC operate from B+1 directly. At 6V battery supply these ICs
have comfortable operating margin — their specified supply range is typically 4-7V.
The DSR-1's STM32G0B1KCU6 requires a separate regulated 3.3V supply because modern
CMOS logic at 64 MHz demands tighter voltage regulation than a battery rail can
provide.

### 1.2 The Original CP304 Module

CP304 (Sony part number 1-464-183-21) is a completely potted DC-DC boost converter
module. "Potted" means the internals are encapsulated in epoxy resin — the
components are invisible, the design is proprietary, and repair is impossible. Sony
designed it as a black-box module that is replaced as a unit, not serviced.

CP304 has served the WM-D6C for over 35 years in surviving units. In failing units,
it has failed in several characteristic ways. The most common is electrolytic
capacitor degradation — the filter capacitors inside the potted module dry out over
decades, reducing their capacitance and increasing their ESR. This causes the output
voltage to drop (reduced capacitance means less hold-up between switching cycles)
and the output ripple to increase (higher ESR means the capacitors can no longer
filter the switching transients effectively). The motor receives an unsteady supply,
runs irregularly, and the servo loop cannot compensate for what appears to be
mechanical speed variation but is actually electrical noise on the motor supply.

The DSR-1's MT3608-based replacement uses documented, available, component-level
parts. When it eventually needs service — decades from now — every component can be
identified, replaced, or verified with standard test equipment. The design is not
mysterious.

### 1.3 The Original CN301 Jack and Its Fatal Flaw

CN301 is a 5.5mm outer diameter / 2.1mm centre pin barrel connector that connects
the external DC power source directly to the B+1 rail. There is no protection
between the jack and the B+1 copper. No series diode. No polarity detection. No
fuse. No crowbar. The B+1 rail and the jack centre pin are electrically the same
node.

Sony wired CN301 with negative polarity on the centre pin — the opposite of
virtually every other manufacturer's convention. The Sony AC-D4M adapter matches
this polarity. Every generic replacement adapter has the opposite polarity.

When a reversed adapter is connected, the B+1 rail is driven negative relative to
ground. The CX20084 is destroyed in milliseconds. The DSR-1 module makes this
failure mode impossible in both variants — and replaces CP304 in the same
installation — addressing all three of the original power system's weaknesses
simultaneously.

---

## 2. The DSR-1 Power Architecture Overview

The DSR-1 produces all three supply rails (B+1, B+3, and 3.3V) from a single B+1
node. A TPS63070 buck-boost generates B+1 (6.0V) in every build; what differs is
*where that regulator physically lives* and which input front end feeds it:

- **Battery-integrated (primary)** — a **two-board** design. A dedicated **power
  daughter board**, mounted behind the battery bay, carries the entire power
  subsystem: USB-C 5V input, the BQ24074 power-path charger, the 3-cell LiPo pack,
  DW01 + dual-FET pack protection, and the TPS63070 boost that produces B+1. The
  daughter board injects its regulated 6.0V **at the original battery-terminal node**
  on the main board (the contacts the AA cartridge used to feed), so the stock S901
  power switch remains the machine's real on/off control. The servo module in the
  CP304 cavity *receives* B+1 through its harness and no longer generates it — its
  power-input zone is unpopulated in this build (Section 7).
- **Variant A (wall-only)** — single module. A 9V USB-C PD contract, which the
  module's on-board TPS63070 *bucks* down to B+1, with no cells fitted (Section 5).
- **Variant B (wall-only)** — single module. The original barrel jack with polarity
  correction, presenting 5–9V that the module's on-board TPS63070 bucks or boosts to
  B+1, with no cells fitted (Section 6).

The crucial architectural point is that the B+1 regulator is a **buck-boost that
crosses through unity automatically** — it bucks the 9V case and boosts the
battery/5V cases without any reconfiguration — so the same converter, divider, and
layout serve every front end. Everything *downstream* of B+1 is likewise identical in
every build; the wall variants drive B+1 out through the harness, while the battery
build drives B+1 in at the machine's battery terminals and the module consumes it.

```
WALL VARIANTS (single module, B+1 generated on-module, driven out via harness):
  Variant A: USB-C 9V PD ──────────────────────────────────────────┐
  Variant B: Barrel 5–9V either polarity ─► [LTC4359 bridge] ───────┤
                                                  V_RAIL (≈5–9V) ────┘
                                                        │
                                                [TPS63070 buck-boost]  (on module)
                                                        │
                                                    B+1 (6.0V) ──► harness J1.7 ──► WM-D6C main board
                                                        ├──[MT3608]──► B+3 (10.8V) ──► Motor
                                                        └──[MCP1700]─► 3.3V ──► STM32

BATTERY BUILD (two boards; B+1 generated on the daughter board, injected at battery terminals):
  ── Power daughter board (behind battery bay) ──────────────────────────────────┐
  USB-C 5V ─► BQ24074 power-path ─┬─► SYS ─► [TPS63070 boost] ─► B+1 (6.0V) ──────┼──► main-board
                                  └─► BAT ─► 3P LiPo pack ─► [DW01 + dual-FET]      │   battery terminals
                                            (3.7V, 3000mAh) ◄── charged in place    │   (─► S901 ─► B+1 net)
  ───────────────────────────────────────────────────────────────────────────────┘
        B+1 propagates through the machine's B+1 net to the CP304 harness ─► servo module
        Servo module: receives B+1 (power-input zone unpopulated)
                        ├──[MT3608]──► B+3 (10.8V) ──► Motor
                        └──[MCP1700]─► 3.3V ──► STM32
```

The elegance of this architecture is that the entire machine — audio circuits,
servo logic, motor, and the DSR-1 module itself — runs from a single protected,
regulated B+1 rail no matter which input path produced it. In the battery-integrated
configuration, the pack's internal-resistance variation (the effect that made the
original servo work harder as the original AA cells discharged) is hidden behind the
TPS63070's feedback loop: B+1 is held constant at 6.0V until the pack is nearly
empty, exactly as the MT3608 holds B+3 constant against B+1 variation.

---

## 3. The MT3608 Boost Converter — CP304 Replacement

### 3.1 Why a Boost Converter Is Still Needed

When the module is powered from a USB-C PD source at 9V (Variant A), one might
wonder why we don't simply use 9V directly for the motor. The answer is that the
motor circuit in the WM-D6C — Q601, Q703, Q704, and M901 — was designed for a
10.8V supply. The Q601 PNP transistor's emitter is connected to the B+3 rail. The
servo loop's DAC output controls Q601's base to modulate motor current. The servo
algorithm's DAC operating range (DAC_MIN to DAC_MAX centered on DAC_CENTER) was
calibrated around the assumption that B+3 is 10.8V. At 9V, the motor would run
slower for the same base drive, changing the loop gain and requiring different PI
coefficients. Rather than complicate the firmware and the calibration, it is simpler
and more correct to maintain the original 10.8V motor supply.

Additionally, operating M901 at its design voltage ensures the motor's thermal
operating point, brush current, and bearing lubrication viscosity all remain within
the regime Sony's engineers specified. Running a DC motor at reduced voltage
increases the current required to produce the same torque, increasing heating.

### 3.2 MT3608 Operating Principle

The MT3608 is a current-mode PWM boost converter. Its operation in one switching
cycle:

**Phase 1 — Switch ON** (duration determined by control loop):
The internal N-channel MOSFET switch connects the SW pin to GND. Current flows from
VIN through inductor L1 and the SW pin to GND. The inductor current ramps up
linearly at a rate of:

```
dI/dt = (VIN - V_SW_sat) / L = (6.0V - 0.1V) / 4.7µH = 1.26 A/µs
```

During this phase, the output capacitor supplies the load current.

**Phase 2 — Switch OFF** (remainder of switching cycle):
The SW pin is disconnected from GND. The inductor resists the sudden change in
current and its voltage reverses polarity, now adding to VIN. The voltage at the
SW pin rises until the SS14 Schottky diode conducts, connecting the inductor to
VOUT. Current flows from the inductor through the SS14 diode into the output
capacitor and the load. The inductor current ramps down at a rate of:

```
dI/dt = -(VOUT - VIN - V_diode) / L = -(10.8 - 6.0 - 0.4) / 4.7µH = -0.94 A/µs
```

The duty cycle D required to produce VOUT from VIN (in continuous conduction mode):

```
D = 1 - (VIN / VOUT)    (ideal, ignoring losses)
  = 1 - (6.0 / 10.8)
  = 1 - 0.556
  = 0.444  →  44.4% duty cycle
```

The MT3608 internal control loop measures the output voltage through the feedback
resistor divider and adjusts the duty cycle to maintain VOUT = 10.8V regardless of
load changes. This feedback loop has a bandwidth of approximately 100kHz — it
corrects output voltage deviations 100,000 times per second.

### 3.3 Output Voltage Setting

The MT3608 regulates its output to maintain 0.6V on its feedback (FB) pin. Two
external resistors (R_upper from VOUT to FB, R_lower from FB to GND) set the
output voltage:

```
VOUT = 0.6V × (1 + R_upper / R_lower)
```

For VOUT = 10.8V:
```
10.8 = 0.6 × (1 + R_upper / R_lower)
18 = 1 + R_upper / R_lower
R_upper / R_lower = 17
```

Choosing R_lower = 10kΩ (a convenient standard value):
```
R_upper = 17 × 10kΩ = 170kΩ
```

Standard resistor value: 169kΩ (E96 series) gives VOUT = 0.6 × (1 + 169/10) =
0.6 × 17.9 = 10.74V — within 0.5% of target. Alternatively, 180kΩ + 10kΩ in
series gives 180kΩ for R_upper: VOUT = 0.6 × 19 = 11.4V — too high. Use 169kΩ
(E96) or 162kΩ + 8.2kΩ (two standard values) for R_upper.

**Important**: The feedback resistors form a voltage divider connected between
VOUT (10.8V) and GND. The current through this divider is 10.8V / (169kΩ + 10kΩ)
= 60µA — negligible relative to the output current but representing a small
constant quiescent load. This is intentional — a minimum load improves regulation
at light loads.

### 3.4 Inductor Selection

The boost inductor L1 is the most physically significant component in the MT3608
circuit. Its value determines the switching current ripple, which affects efficiency,
output ripple voltage, and the required input and output capacitor sizes.

**Inductor current ripple at nominal conditions** (VIN = 6V, VOUT = 10.8V,
IOUT = 300mA, fsw = 1.2MHz):

First, calculate input current from output power (assuming 85% efficiency):
```
IIN = POUT / (VIN × η) = (10.8V × 0.3A) / (6.0V × 0.85) = 636mA
```

Current ripple for a 4.7µH inductor at 44.4% duty cycle:
```
ΔIL = VIN × D / (L × fsw)
    = 6.0 × 0.444 / (4.7×10⁻⁶ × 1.2×10⁶)
    = 2.664 / 5.64
    = 0.472A peak-to-peak
```

Peak inductor current:
```
IL_peak = IIN + ΔIL/2 = 0.636 + 0.236 = 0.872A
```

The MT3608 datasheet specifies a current limit of approximately 4A — the peak
current of 0.872A is well within this limit. However, the inductor's saturation
current rating must also exceed the peak current. A CDRH4D28 4.7µH inductor has
a saturation current of approximately 1.5A — sufficient with margin.

**Why 4.7µH specifically?** The inductor value is chosen to keep the converter in
continuous conduction mode (CCM) at the minimum expected load current. In
discontinuous conduction mode (DCM), which occurs at very light loads, the output
voltage regulation changes character and the feedback loop requires different
compensation. The transition from CCM to DCM occurs when the ripple current exceeds
twice the input current:

```
IIN_critical = ΔIL / 2 = 0.236A   →   POUT_critical = 6.0 × 0.236 × 0.85 = 1.2W
```

This corresponds to a motor current of approximately 1.2W / 10.8V = 111mA. Below
this load, the converter enters DCM. For the WM-D6C motor in normal playback, the
motor current is well above this threshold. Even in standby with the motor stopped,
the feedback resistors provide the minimum load that keeps the output regulated.
4.7µH keeps the converter in CCM across the expected operating range.

**Height constraint**: The MT3608 boost inductor height must not exceed 3mm for the
DSR-1 board to fit within the CP304 cavity clearance. The CDRH4D28 series from
Sumida (4×4mm footprint, 2.8mm height) satisfies this requirement.

### 3.5 Output Capacitor Selection

The output capacitor (C_BOOST_OUT, 22µF 16V) filters the switching ripple on the
B+3 rail. The output voltage ripple is:

```
ΔV_out = IOUT × D / (C × fsw)
       = 0.3 × 0.444 / (22×10⁻⁶ × 1.2×10⁶)
       = 0.133 / 26.4
       = 5.0mV peak-to-peak (ignoring ESR)
```

With ESR contribution (approximately 5mΩ for a good X5R ceramic):
```
ΔV_ESR = ΔIL × ESR = 0.472 × 0.005 = 2.4mV
```

Total ripple ≈ 7.4mV peak-to-peak at 1.2MHz. This is negligible relative to the
10.8V output — 0.069% ripple. The motor's inductance provides additional filtering,
so the actual ripple at the motor terminals is even lower.

**Why 16V rating?** The output voltage is 10.8V. Capacitors should be derated to
approximately 50-80% of their rated voltage for long-term reliability. At 10.8V
on a 16V-rated capacitor, the derating is 67.5% — within the recommended range.
The next standard voltage is 10V, which would be operating at 108% of rating —
unacceptable. 16V is the correct choice.

**X5R vs X7R dielectric**: An X5R ceramic at 16V, 22µF loses approximately 60% of
its capacitance at 10.8V DC bias due to the piezoelectric effect inherent in Class
II ceramics. Effective capacitance at operating voltage is approximately 8.8µF —
the ripple calculation above should be redone with 8.8µF:

```
ΔV_out = 0.3 × 0.444 / (8.8×10⁻⁶ × 1.2×10⁶) = 12.5mV
```

Still acceptable (0.12% of 10.8V), but the design should use a 47µF 16V X5R
capacitor rather than 22µF to account for DC bias derating and achieve the
originally intended effective capacitance. **This is an important correction from
the initial BOM — the output capacitor should be 47µF 16V X5R, not 22µF.**

### 3.6 Input Capacitor Selection

The input capacitor (10µF, 10V) supplies the MT3608's peak switching current demand
that its input source — the TPS63070 B+1 regulator, common to every build — cannot
instantaneously provide due to that regulator's own output impedance. Without
adequate input capacitance, the input voltage sags
during the switch-on phase, reducing efficiency and potentially destabilising the
input supply.

The input capacitor current during the switch-on phase:
```
I_cap_peak = IL_peak - IIN_average = 0.872 - 0.636 = 0.236A
```

The charge drawn from the input capacitor in one half-cycle (switch-on time):
```
Q = I_cap_peak × D / fsw = 0.236 × 0.444 / 1.2×10⁶ = 87nC
```

The resulting input voltage sag for 10µF:
```
ΔV_in = Q / C = 87×10⁻⁹ / 10×10⁻⁶ = 8.7mV
```

This is negligible. 10µF is more than adequate. The capacitor should be rated at
16V (the B+3 output voltage is the maximum that could appear at the input in a fault
condition, though normally it will never exceed B+1 = 6V).

### 3.7 Efficiency and Thermal Analysis

The MT3608 efficiency at the nominal operating point (VIN = 6V, VOUT = 10.8V,
IOUT = 300mA) is approximately 85% from the datasheet efficiency curves.

Power dissipated in the MT3608:
```
P_loss = POUT × (1 - η) / η = (10.8 × 0.3) × (1 - 0.85) / 0.85
       = 3.24 × 0.176 = 0.57W
```

The MT3608's thermal resistance junction-to-ambient in SOT-23 is approximately
250°C/W. At 0.57W dissipation:
```
T_junction = T_ambient + P × θJA = 25°C + 0.57 × 250 = 168°C
```

This exceeds the MT3608's maximum junction temperature of 125°C. At 300mA output
current this would be a problem. However, the WM-D6C motor current in normal
playback is much lower than 300mA — closer to 100-150mA. At 150mA:

```
P_loss = (10.8 × 0.15) × (1 - 0.85) / 0.85 = 1.62 × 0.176 = 0.285W
T_junction = 25 + 0.285 × 250 = 96°C
```

At 96°C junction temperature, operation is within the MT3608's specification with
margin. The 300mA case represents a stress condition (motor stall or large
acceleration) that occurs only briefly. Thermal mass limits the junction temperature
rise during brief overloads.

**Thermal management note**: On the DSR-1 PCB, the MT3608 SOT-23 package has no
exposed pad. Heat dissipates through the package leads and PCB copper. The GND and
VIN pads of the MT3608 should have copper pours connected to them to improve
thermal dissipation. A 5×5mm copper pour under and around the MT3608, connected to
the GND plane through multiple vias, reduces the effective thermal resistance by
approximately 20-30%.

### 3.8 The SS14 Schottky Rectifier Diode

The SS14 is a 1A, 40V Schottky rectifier in SMA package. It conducts during the
switch-off phase, transferring stored inductor energy to the output capacitor and
load. Key parameters:

**Forward voltage**: Approximately 0.4V at 300mA, 0.35V at 150mA (from Vishay
SS14 datasheet). The forward voltage appears directly as a power loss:
```
P_diode = I_out × Vf × (1 - D) = 0.15 × 0.35 × 0.556 = 29mW at 150mA load
```

**Reverse voltage**: The SS14 must withstand the VOUT voltage (10.8V) when the
switch is on and the diode is reverse biased. At 40V rating, the 10.8V reverse
voltage represents 27% of rating — well within specification.

**Why Schottky and not standard silicon rectifier?** A standard silicon rectifier
(1N4001-type) has a forward voltage of approximately 0.7-1.0V — nearly three times
the SS14's 0.35V. This would increase the diode loss from 29mW to approximately
83mW at the same current, reducing converter efficiency by approximately 2.5
percentage points and adding unnecessary heat. In a boost converter the rectifier
diode conducts for the majority of the switching cycle; its forward voltage is
directly subtracted from the conversion efficiency. Schottky rectifiers are
essentially mandatory in modern boost converters.

---

## 4. The MCP1700 LDO — STM32 Supply

### 4.1 Why a Linear Regulator for the STM32

The STM32G0B1KCU6 requires a stable, low-noise 3.3V supply. Two approaches are
possible: a switching step-down (buck) converter from B+1, or a linear LDO
regulator.

A buck converter would be more efficient — at 9mA load current, an LDO dissipates
(6.0 - 3.3) × 0.009 = 24mW, while a buck would dissipate approximately 5-7mW.
The difference is 17-19mW — negligible in the context of the WM-D6C's total power
consumption of approximately 3W. The efficiency advantage of a buck for a 9mA load
is not worth the additional switching noise, components, and cost.

An LDO produces an inherently low-noise output because it regulates by dissipating
excess voltage as heat in the pass transistor — a purely resistive process with no
switching transitions. The output noise is dominated by the LDO's internal bandgap
reference noise, which is typically 10-100µV RMS. A switching converter's output
noise is dominated by switching ripple and is typically 5-50mV peak-to-peak —
100-500× higher. For a supply feeding an ADC and a servo control loop sensitive to
millivolt-level signals, the LDO's noise advantage is significant.

### 4.2 MCP1700 Characteristics Relevant to WM-D6C Operation

The MCP1700T-3302E/TT has several characteristics that make it the correct choice
for this application:

**Input voltage range: 2.3V to 6.0V**. The WM-D6C B+1 rail operates between
approximately 3.5V (nearly discharged batteries) and 6.5V (fresh alkaline cells
or adapter high-end). With Variant A or B regulated supply, B+1 is a constant 6.0V
— perfectly within range. With battery-only operation (if someone retains battery
power alongside the module), the LDO continues operating down to 3.5V input,
which occurs when each of the four AA cells has discharged to 0.875V — well past
the point where the machine's audio performance has already degraded unacceptably.

**Dropout voltage: 178mV at 100mA**. The dropout voltage is the minimum difference
between input and output required to maintain regulation. At 178mV dropout, the
LDO maintains 3.3V output down to 3.478V input. This is lower than the battery
discharge point discussed above. Regulation never fails before the audio circuits
become unusable — the LDO is never the limiting factor in battery runtime.

**Quiescent current: 1.6µA typical**. The MCP1700 draws only 1.6µA from the input
when unloaded. In the WM-D6C context this matters because the module is always
powered when the machine's power switch S901 is on — even in pause mode with the
motor stopped, the LDO remains active to keep the STM32 running. At 1.6µA +
9mA (STM32 at idle) = 10.6mA total LDO load in pause mode, the power consumption
is minimal.

**Output noise: 40µV RMS (DS20001826F, Section 1.0)**. This is the noise on the
3.3V rail that becomes the ADC reference. At 40µV RMS with a 3.3V reference and
12-bit ADC, the noise contributes approximately 40µV / (3.3V / 4096) = 0.05 LSB
of ADC noise floor. This is below the fundamental quantisation noise of ±0.5 LSB
and does not affect measurement accuracy.

**Power Supply Ripple Rejection: 44dB at 100Hz (DS20001826F)**. This specifies how
much of the input supply noise appears on the output. At 44dB rejection, input
ripple is attenuated by a factor of 158. The MT3608 switching ripple at 1.2 MHz
will experience much less PSRR (closer to 10dB at 1MHz), but this is partly why
the MT3608 and MCP1700 are physically separated on the board with dedicated
decoupling. See Section 3 of the signal chain analysis for the noise coupling
analysis.

### 4.3 Decoupling Network

The MCP1700 application circuit from the datasheet specifies 1µF ceramic on both
input and output. The DSR-1 adds 10µF electrolytic on both sides for additional
bulk storage. The purpose of each:

**1µF ceramic (input)**: Provides high-frequency decoupling of the input supply
against the LDO's switching transient demands. When the STM32 transitions from
light to heavy load (e.g., USB start-of-frame processing), the LDO's input draws a
brief current pulse. The 1µF ceramic supplies this current instantaneously while
the larger input source catches up.

**10µF electrolytic (input)**: Provides lower-frequency bulk reservoir. During the
MT3608's switching cycle, the B+1 rail has small voltage ripple (see Section 3.5,
calculated at 5-12mV). The 10µF provides additional damping of this ripple.

**1µF ceramic (output)**: Required by the MCP1700 for loop stability. The LDO's
internal compensation requires a minimum output capacitance to maintain stability
at all load conditions. 1µF is specified as the minimum; higher values are
acceptable and improve transient response.

**10µF electrolytic (output)**: Provides bulk energy for STM32 load transients.
When the STM32's internal activity increases suddenly (e.g., USB interrupt handler
executing), the supply current demand increases by several milliamps in nanoseconds.
The 10µF holds the 3.3V rail stable during the nanoseconds before the LDO output
stage responds.

**All capacitors must be placed within 2mm of the MCP1700 pins** to minimise the
trace inductance between the capacitor and the IC. A 10mm trace has approximately
8nH of inductance; at the LDO's transient frequencies (>1MHz), this inductance
dominates over the capacitor impedance, negating its bypass effect.

### 4.4 Thermal Analysis

The MCP1700 power dissipation at normal STM32 load:
```
P_LDO = (VIN - VOUT) × IOUT = (6.0 - 3.3) × 0.009 = 24mW
```

In SOT-23 package, thermal resistance junction-to-ambient is approximately 256°C/W:
```
T_junction = 25°C + 0.024 × 256 = 31°C
```

The MCP1700 runs essentially at ambient temperature. No thermal management is
required. Peak STM32 consumption (USB active, ADC scanning, servo loop running)
is approximately 15mA:
```
P_LDO_max = 2.7 × 0.015 = 40mW
T_junction_max = 25 + 0.040 × 256 = 35°C
```

Well within the MCP1700's 125°C junction temperature limit.

---

## 5. Variant A Power Input — USB-C Power Delivery

### 5.1 The Input Voltage Decision

Variant A uses USB-C Power Delivery to negotiate a supply voltage. With the TPS63070
buck-boost now serving as the B+1 regulator (Section 5.2), the input voltage is no
longer *dictated* by the converter topology — the part holds 6.0V from anything
between 2V and 16V. This is the central change from the original design, which used
a fixed step-down regulator that could only buck and therefore *required* an input
above 6V.

9V remains the negotiated target for Variant A, but now for headroom and efficiency
rather than necessity:

**Buck mode is the favourable region**: at 9V → 6V the TPS63070 operates as a pure
synchronous buck, its most efficient mode, with full 2A output headroom. The 9V PDO
is offered by essentially every USB-C PD charger ever made.

**Lower input current**: 9V at a given output power draws roughly half the input
current of 5V, reducing cable drop and connector heating.

The regulator *itself* would also produce 6.0V from a 5V input — by boosting, exactly
as it does in the battery build (Section 7.4) — so the strict "5V is insufficient,
two cascaded boosts, unworkable" problem of the old fixed-buck design no longer
exists. Variant A still gates its output on a successful 9V contract through the
IP2721 (see the USB PD document), so in this build 9V is the operative target; the
buck-boost simply removes the topological reason it *had* to be.

### 5.2 The TPS63070 Buck-Boost Converter

The B+1 regulator is a **TPS63070** (Texas Instruments) — a synchronous buck-boost
rated 2–16V in, 2.5–9V out, 2A output, with a 3.6A switch current limit and a 0.8V
feedback reference, in a 15-pin VQFN. It replaces the fixed step-down regulator of
the original schematic (a part that could neither reach 6V from a 5–6V input nor be
adjusted) and is the **single B+1 regulator shared by all three front ends**
(Section 2).

**Why buck-boost and not a plain buck.** A buck can only step down — it requires
VIN > VOUT at all times. That is acceptable for a guaranteed 9V PD source, but it
fails the instant the input can reach or fall below 6V, which it does in two of the
three front ends (the 5V and battery cases). The TPS63070 crosses through unity
automatically: it bucks when VIN > 6V, boosts when VIN < 6V, and transitions
seamlessly through the region where the two are close. One part therefore covers the
entire 2.8V (battery floor) to 9V (PD) input span without reconfiguration.

**Topology note.** Unlike a buck (SW → inductor → VOUT) or a boost (VIN → inductor →
SW), the buck-boost's single inductor connects *between the device's two switch pins*
(L1 and L2) — the input stage switches one end, the output stage the other. This
changes the inductor's placement and routing relative to the original buck layout.
The EN pin compares against a precise 0.8V rising threshold and must not float; an RC
network on EN sets a defined soft-start.

**Duty cycle** depends on which side of unity the input sits:
```
Buck  (VIN = 9.0V):   D ≈ VOUT / VIN     = 6.0 / 9.0      = 0.667
Boost (VIN = 3.2V):   D ≈ 1 − VIN / VOUT = 1 − 3.2 / 6.0  = 0.467
```
The device manages the internal buck/boost duty itself; these are the equivalent
single-stage figures at the two extremes.

**Output voltage setting** uses the 0.8V reference:
```
VOUT = 0.8V × (1 + R_upper / R_lower)
6.0V = 0.8 × (1 + 649k / 100k) = 0.8 × 7.49 = 5.99V
```
R_upper = 649kΩ, R_lower = 100kΩ (both E96). The FB divider current is
6.0V / 749kΩ ≈ 8µA — well under the datasheet's 1mA guidance for the feedback node.
These are the same divider values used in every build.

**Current capability and the battery-floor case.** The TPS63070 delivers up to 2A
output in both buck and boost mode, bounded by its 3.6A switch current limit. The
WM-D6C's total B+1 load is ~400–650mA. In buck mode (Variant A, 9V in) this is
trivial. Boost mode is the demanding case: at the LiPo floor of ~3.0V producing 6.0V
at ~0.65A, the input current is ~1.5A and the peak inductor current approaches ~2A —
comfortably under the 3.6A limit, with a motor-surge transient briefly higher but
bounded by the B+1 bulk capacitor. The LiPo discharge curve sags more gradually than
LFP — the pack holds above ~3.5V for much of its capacity and reaches ~3.0V only near
the end — so the worst-case boost current is a small sliver of total runtime (see
Section 7.4 for the battery-build budget).

**Efficiency and thermal.** Peak efficiency is approximately 93–95%. At ~3W
throughput and 90% efficiency the device dissipates roughly 300mW; in the VQFN with
its thermal pad and a copper pour vias to the ground plane, junction temperature
stays well within the 125°C limit. As with the MT3608, the thermal pad copper is the
primary heat path.

### 5.3 VBUS Bulk Decoupling

A 4.7µF, 16V X5R capacitor on VBUS provides bulk energy storage for the converter
input. The IP2721 PD trigger (Section 4 of the USB PD document) has a finite output
impedance and response time. During large load steps (motor starting), the 4.7µF
capacitor provides the initial charge burst while the USB charger's output responds.

---

## 6. Variant B Power Input — Polarity-Agnostic Barrel Jack

### 6.1 The Core Requirement

Variant B must accept any 6V barrel jack adapter — the original Sony AC-D4M
(negative centre), any generic replacement (positive centre, the more common type),
or any third-party adapter — and deliver correct-polarity 6V to the rest of the
circuit. It must also survive the accidental connection of higher-voltage adapters
and the back-EMF transients from the motor switching circuit.

The failure history of the WM-D6C makes these requirements non-negotiable. Every
protection device in the Variant B input stage exists because of a documented,
real-world failure mode that has destroyed machines.

### 6.2 The LTC4359 Ideal Diode Bridge

A conventional diode bridge rectifier (four diodes) would achieve polarity
correction but with a forward voltage drop of approximately 1.2V (two diodes in
series in the forward path). A 6.0V input minus 1.2V drop gives 4.8V out —
insufficient for the WM-D6C's 5.5V minimum operating voltage.

The LTC4359 ideal diode bridge controller drives four external P-channel MOSFETs
configured as a full-wave bridge. Unlike a passive diode, a MOSFET's on-resistance
can be made very low — the Si2333DS P-channel MOSFET has an on-resistance of
approximately 0.18Ω at 4V gate-source drive. At 400mA load current:

```
V_drop = 2 × I × Rds_on = 2 × 0.4 × 0.18 = 144mV
```

The factor of 2 accounts for the two MOSFETs in the current path (positive rail
MOSFET and negative rail MOSFET). The 144mV drop means 5.856V reaches the output
from a 6.0V input — close enough to 6.0V that the downstream circuitry operates
at its full specification.

**How the LTC4359 works**: The LTC4359 monitors the polarity of the input and drives
the gate of each P-channel MOSFET to put it in its correct state for the detected
polarity. When the centre pin is negative (correct Sony polarity), two MOSFETs in
one diagonal of the bridge conduct; when the centre pin is positive (incorrect
generic adapter polarity), the other diagonal conducts. The output is always positive
regardless of input polarity. The transition between configurations takes
approximately 1ms as the LTC4359's control circuit detects the polarity and
activates the correct gates — during this 1ms the output is held near 0V by the
input bulk capacitor.

**Why four P-channel rather than two N-channel and two diodes?** Several
configurations are possible for a polarity-correcting bridge. Four P-channel MOSFETs
driven by the LTC4359 is the best choice for this application because:

- P-channel MOSFETs require no charge pump for gate drive at the operating voltage
  range (the gate can be driven lower than source directly from the LTC4359 output)
- The configuration handles the specific input voltage range (4.5V to 9V) correctly
  without special biasing
- Component count is minimised — the LTC4359 handles all four gates

### 6.3 Polyfuse — Overcurrent Protection

The polyfuse (MF-R050, 500mA hold / 1.5A trip) is placed in series with the input,
before the ideal diode bridge. Its purpose is to protect against two fault
conditions:

**Wiring fault**: If the harness from J1 to the main board is incorrectly assembled
or develops a short, the current demand can exceed what any barrel jack adapter can
safely supply. The polyfuse trips within seconds (at 1.5A it trips in approximately
2-5 seconds from room temperature) and holds the fault current to a safe level.
When the fault is cleared, the polyfuse resets automatically and full operation
resumes.

**Adapter overcurrent**: Some aftermarket adapters are rated at very low currents
(200-300mA) but have no internal current limiting. The WM-D6C at full load draws
approximately 400mA on B+1. An underrated adapter driving 400mA may overheat. The
polyfuse at 500mA hold provides no protection against this — it is intentionally
sized to allow normal operation (400mA continuous) while tripping on fault currents
(>1.5A). Protecting underrated adapters is the user's responsibility; the polyfuse
protects the machine and the module, not the adapter.

**Polyfuse characteristics that matter**:
- Hold current: 500mA — must be above maximum normal operating current (~400mA)
- Trip current: 1.5A — must be below the short-circuit current the machine circuitry
  can experience without damage
- Hold current derating with temperature: at 60°C ambient, the polyfuse holds
  approximately 400mA — still adequate but with reduced margin. If the module is
  installed in a warm enclosure, a 750mA polyfuse provides better margin.
- Reset time: 30-60 seconds after the fault is cleared and polyfuse cools

### 6.4 SMBJ7.0A TVS Diode — Transient Protection

The SMBJ7.0A is a bidirectional transient voltage suppressor (TVS) diode with a
7.0V standoff voltage. It is placed across the output of the polarity bridge (after
the polyfuse, before the downstream circuitry). Its function is to clamp voltage
transients that exceed 7V to ground, limiting the peak voltage seen by the
downstream circuits.

**Why 7.0V standoff?** The B+1 operating voltage is 6.0V. The TVS must have a
standoff voltage above 6.0V to avoid conducting during normal operation (which would
clamp the supply and waste current). The SMBJ7.0A has a standoff voltage of 7.0V
and a breakdown voltage of 7.78-8.60V — it does not conduct at 6.0V but clamps any
voltage spike that reaches approximately 8V. The next standard value down (SMBJ6.5A,
6.5V standoff) would conduct partially at 6.0V, creating unnecessary quiescent
current.

**What transients does it protect against?**

*Motor back-EMF*: When Q703/Q704 switches the motor off (mode change, stop command),
the motor's inductance resists the sudden current interruption and generates a
back-EMF spike. This spike propagates backwards through the motor drive circuit
toward the B+3 rail and, to a lesser extent, toward B+1. The TVS clamps these
spikes before they can reach the polarity bridge or the input adapter.

*Adapter hot-plug*: When a barrel jack adapter is connected to a live machine (or
when the module first powers on from an adapter that has been sitting in an outlet),
the inrush current into the module's bulk capacitors creates a voltage spike on the
adapter output. If the adapter has a high source impedance (common in cheap
unregulated adapters), this spike can momentarily exceed the normal output voltage.
The TVS clamps it.

*Electrostatic discharge*: The barrel jack CN301 is on the machine's panel — a
user-accessible connector. ESD events from a statically charged user are possible.
The SMBJ7.0A provides a low-impedance clamp path for ESD energy, protecting the
module's electronics.

**TVS clamping voltage**: The SMBJ7.0A clamping voltage (maximum voltage during
a 1A pulse) is 12V. This means during a high-energy transient, the voltage at the
clamped node can momentarily reach 12V before falling to 7-8V. The LTC4359's
input voltage range is typically up to 20V, and the downstream MCP1700 input rating
is up to 6.5V (it sees 6.0V nominal). The brief excursion to 12V at the TVS node
is harmless because it appears across the polyfuse resistance plus the TVS, not
directly across the LTC4359 output. The output of the bridge sees only the TVS
clamped voltage.

### 6.5 220µF Bulk Input Capacitor

The 220µF 10V electrolytic capacitor at the output of the polarity bridge provides
energy storage for three purposes:

**Motor startup surge**: When the play button is pressed, M901 accelerates from rest
to operating speed. The inrush current during motor startup can momentarily reach
several times the steady-state current as the motor's back-EMF is zero and the
winding resistance is the only impedance limiting current. The 220µF capacitor
provides this energy without the input voltage collapsing.

**Holdup during LTC4359 switching**: The 1ms transition time of the LTC4359 during
polarity detection (see Section 6.2) leaves the output momentarily undriven. The
220µF holds the B+1 rail at 6.0V during this window.

**Input filter reservoir**: The capacitor acts as the first stage of input filtering
for all the downstream switch-mode converters (MT3608). Together with the series
resistance of the input path (wiring, polyfuse, MOSFET on-resistance), it forms an
RC low-pass filter for input-side noise.

**Voltage rating**: The operating voltage across this capacitor is 6.0V. A 10V
rating provides 67% derating. Electrolytic capacitors should be derated more
aggressively than ceramics for long-term reliability (electrolytic aging is
accelerated by operating close to rated voltage). 10V is the minimum recommended
rating; 16V would be preferable if the capacitor physically fits.

---

## 7. Battery-Integrated Input — Charge-in-Place over USB-C (Two-Board Design)

This is the primary DSR-1 configuration: a LiPo pack is the runtime source, and USB-C
both charges it in place and runs the machine at the same time. Unlike the wall
variants, the battery build is split across **two boards**:

- A **power daughter board**, mounted *behind the battery bay* (not inside it), which
  carries the whole power subsystem — USB-C 5V input, the BQ24074 power-path charger,
  the LiPo pack connections, DW01 + dual-FET pack protection, and the TPS63070 boost
  that produces B+1.
- The **servo module** in the CP304 cavity, which in this build *receives* B+1 and
  does not generate it. Its power-input zone (the on-module TPS63070 and front-end
  parts used by Variants A/B) is left unpopulated.

The daughter board injects its regulated 6.0V at the **original battery-terminal node**
on the main board — the contacts the four-AA cartridge used to feed. That 6.0V then
propagates through the machine's own B+1 net (through S901, the power switch) and
reaches the servo module via the existing CP304 harness, exactly as battery power
always did. Two consequences follow directly: the stock **S901 power switch keeps
working** as the machine's real on/off control, and the servo module needs no power
front end of its own.

One point worth stating plainly: in this configuration there is **no USB PD**. The
BQ24074 charges from default 5V VBUS, and a 1S LiPo charger wants ~5V in — a 9V PD
contract would be both unnecessary and harmful. The battery build presents a plain
5V-sink on the CC lines and the TPS63070 boosts to B+1 from the 5V/pack SYS rail.

### 7.1 The Pack — 3× LiPo in 3P

The runtime source is **three LiPo cells wired in parallel (3P)**. The reference cell
is the 803040 form factor (8 × 30 × 40 mm, 3.7V, 1000mAh, with on-cell PCM and a
PH2.0 pigtail), giving a pack that is nominally 3.7V, ~4.2V fully charged, ~3.0V
usable floor, **3000mAh / ~11.1Wh** total. This supersedes the earlier 1S4P IFR14500
LiFePO4 selection; the move to LiPo was made because LiPo pouches in the required
form factor are available off-the-shelf with JST pigtails and protection, whereas a
fitting LFP pouch is a custom order.

**Why three, and why they fit.** The real battery cavity measures approximately
57 × 32 × 29 mm (≈52 cm³) — confirmed by the depth from the battery-cover lid to the
bottom of the original Sony holder (1 1/8" ≈ 28.6 mm), not the shallower figure an
incomplete tape-measure reading first suggested. Three 803040 cells stacked flat
occupy 40 × 30 × 24 mm, leaving ~4.6 mm of clearance on the stacking (depth) axis.
That axis is also the direction LiPo cells swell, so the clearance is swell
allowance: three cells bulging ~0.8 mm each at end of life (~2.4 mm) sit comfortably
under the 4.6 mm gap. The gap must be left empty — it is the swell budget, not space
to pad out.

**3P relaxes the rate problem.** Each 803040 is a low-rate "non-power" cell (~1–2A).
In 3P the worst-case ~1.5A machine draw is ~0.5A per cell and a ~3A motor-startup
surge is ~1A per cell — well within each cell's comfort, with the B+1 bulk capacitor
absorbing sub-millisecond transients regardless.

**Balance before joining.** The three cells must be matched to within ~30–50 mV of
resting voltage before they are first paralleled, to avoid cell-to-cell inrush at the
moment of connection. After the first equalisation they track. The on-cell PCMs do
*not* prevent this inrush — they protect the external load/charge path, not two cells
meeting each other — so balance-first is a mandatory assembly step. The 3P junction
(three positives bussed, three negatives bussed) is formed on the daughter board,
where the cells plug in.

**Service disconnect.** The cells plug into the daughter board; pulling the three
cell connectors removes the pack for service. This restores the convenient
"removable pack" behaviour of the original cartridge without the AA holder (which
does not fit the cavity once cells of this length are stacked).

### 7.2 The BQ24074 Power-Path Charger

The charger is a TI **BQ24074** — a single-cell Li-ion/LiPo linear charger with
**integrated dynamic power-path management (DPPM)**. It is chosen specifically
because the design requires the machine to run from USB *while* the battery charges,
with the load taking priority. The BQ24074 has separate IN, SYS, and BAT terminals
and does this natively:

- **DPPM** powers the SYS load from USB first and routes only surplus input current
  to charging — i.e. "the deck runs from USB, the battery charges from what's left."
- **Supplement mode** lets the battery assist the SYS load when a transient (motor
  startup surge) briefly exceeds the input current limit, so the machine cannot brown
  out on inrush while plugged in.
- A **programmable input current limit** (ILIM) protects a weak USB source, and
  **thermal regulation** folds back charge current if the IC heats up — useful on the
  small daughter board.
- When USB is removed, SYS hands off to the battery seamlessly (the B+1 bulk
  capacitor covers the microsecond transition).

This replaces the earlier CN3058E-plus-discrete-load-share approach: a bare charger
(CN3058E or TP4056) has only a BAT terminal and cannot cleanly power a load while
charging. The BQ24074 folds the charger and the load-share into one correct part.

**Charge current** is set by R_ISET on the ISET pin (I_CHG = K_ISET / R_ISET,
K_ISET ≈ 890 AΩ). A starting value of **R_ISET ≈ 1.1 kΩ → ~0.8A** charge is a gentle
~0.27C into 3000mAh — kind to the cells and low-dissipation. At 5V in and ~4.0V on
the pack the pass element dissipates ≈ (5 − 4.0) × 0.8 ≈ 0.8W, so give the IC a
copper pour.

**Input current limit** is set by R_ILIM (I_INLIM = K_ILIM / R_ILIM, K_ILIM ≈
1600 AΩ). **R_ILIM ≈ 800 Ω → ~2A** matches a 5V/2.4A-or-better USB-C source. With DPPM
the deck always gets priority and charge throttles automatically when the deck draws
more, so the input budget does not have to be hand-partitioned. Total demand is
deck (~3–4W) plus charge (~0.8A × 4.2V ≈ 3.4W) ≈ 7–8W, which a 5V/3A (15W) source
covers with margin.

**Supporting connections.** The TS pin must be set into its valid window with a fixed
resistor pair (the 803040 cells expose no thermistor) or wired to a 10kΩ NTC against
the pack if temperature qualification is wanted. The CHG and PGOOD status outputs are
read by the STM32 (routed across the board-to-board link, Section 7.6) to drive the
charging animation on the LED fuel gauge.

### 7.3 Pack Protection — DW01 + Dual FET

The 3P pack needs over-charge, over-discharge, and over-current protection on the
logical cell. Use a **DW01** protection IC driving a dual N-channel FET (FS8205-class)
in series between the pack and the rest of the daughter board.

For LiPo this is the correct part: the DW01's thresholds (over-charge 4.25V,
over-discharge ~2.4V) are the Li-ion/LiPo values that match a 4.2V-float cell.
(An earlier revision of this project used an LFP pack and an HY2112-CB, and explicitly
warned *against* the DW01 — that warning applied only to the LFP chemistry, whose 3.6V
float made the DW01 thresholds wrong. With the change to LiPo, the DW01 is now exactly
right.) The three cells' on-cell PCMs provide a first layer; the daughter-board DW01 +
FET pack is the pack-level backstop. Because per-cell current stays well under each
PCM's trip point in normal operation, the layers do not nuisance-trip against each
other.

### 7.4 The B+1 Regulator in Boost Mode — SYS → 6.0V

On the daughter board the B+1 regulator is the **same TPS63070 buck-boost** used by
the wall variants (Section 5.2), here running in boost mode because the BQ24074 SYS
rail sits below 6.0V (≈4.6–4.9V on USB, tracking the pack at 3.0–4.2V on battery).
The divider (R_upper = 649 kΩ, R_lower = 100 kΩ, 0.8V reference) and layout are
identical to the wall builds — the only difference is that in the battery build this
regulator lives on the daughter board rather than the servo module.

**Worst-case input current**, whole machine ~4W on B+1, 90% efficiency, pack near its
3.0V floor:

```
I_IN = P_OUT / (V_SYS × η) = 4 / (3.0 × 0.9) ≈ 1.5 A    (motor surge briefly higher)
```

Peak inductor current approaches ~2A at this corner — under the TPS63070's 3.6A
switch limit, with surge transients bounded by the B+1 bulk capacitor.

**Duty cycle** at the nominal 3.7V pack voltage:

```
D ≈ 1 − V_SYS / V_OUT = 1 − 3.7 / 6.0 = 0.383
```

### 7.5 6V Injection at the Battery Terminals, and S901

The daughter board's 6.0V output is wired to the main board at the **original
battery-positive terminal location** (the corroded/leaked stock spring contacts are
removed — see the installation notes — and the daughter-board feed replaces them).
Because the stock wiring runs battery → S901 → B+1 net, injecting on the battery side
of S901 keeps the front-panel power switch fully functional: it still switches the
whole machine on and off, including the servo module fed downstream through the CP304
harness.

A series Schottky (1N5819-class) at the injection point provides reverse protection
against a power-sequencing fault; its ~0.3V drop puts ~5.7V on the machine's B+1,
within the machine's operating specification.

The 6V output run now carries the full machine current — including the motor draw
reflected back through the MT3608, which peaks above 1.5A. This path is no longer the
millimetres of the original battery-spring-to-board connection; it runs from the
daughter board behind the bay to the injection point. Size the conductor for the
current and keep it short.

### 7.6 The Board-to-Board Interface

Because the pack and B+1 generation moved off the servo module, a small set of
conductors links the daughter board to the rest of the machine:

| Conductor | Direction | Purpose |
|---|---|---|
| B+1 (6.0V) | Daughter → main-board battery terminal | Machine + module supply, through S901 |
| GND | Shared | Common return |
| VBAT_SENSE | Daughter → servo module ADC | Pack voltage for the fuel gauge (Section 7.8) |
| CHG / PGOOD | Daughter → servo module GPIO | Charger status for the LED animation |
| USB D+/D− (optional) | Daughter ↔ servo module | If USB CDC tuning is routed from the daughter board's USB-C; otherwise a separate bench header on the module |

The servo module's J1 pin 7 (B+1) consequently flips from an **output** (it drove B+1
in the wall variants) to an **input** in the battery build — it now only receives B+1.
A Schottky there can be retained as input reverse-protection or omitted, since its
original anti-back-feed purpose no longer applies.

### 7.7 The Complete Battery Path

```
USB-C (5V VBUS)  ── on power daughter board ──────────────────────────────────────┐
   │                                                                                │
   └─► BQ24074  IN ──► SYS ──────────────────────────────► [TPS63070 boost] ─► B+1 │
        (power-path)    │  (deck priority via DPPM)              (D≈0.38)    (6.0V) │
        R_ISET≈1.1k     │                                                       │   │
        R_ILIM≈800Ω     └─► BAT ─► 3× 803040 LiPo (3P, 3.7V, 3000mAh)           │   │
        CHG/PGOOD ───────────────► [DW01 + FS8205 dual-FET protection]          │   │
                                    (cells balance-matched before join)         │   │
   ─────────────────────────────────────────────────────────────────────────────┘ │
                                                                                    │
   B+1 ─► main-board battery terminal ─► S901 ─► machine B+1 net ─► CP304 harness ──┘
                                                                          │
                                                          ── servo module (B+1 input) ──
                                                          ├──[MT3608]──► B+3 (10.8V) ──► Motor M901
                                                          └──[MCP1700]─► 3.3V ──► STM32
```

From B+1 rightward this is the existing, proven design. What the battery
configuration changes is only how B+1 comes to exist: from a charged 3P LiPo pack
through a power-path charger and a daughter-board boost, with USB-C running the
machine and topping the pack at the same time, injected at the battery terminals so
S901 still commands the machine.

**Runtime.** 3000mAh × 3.7V ≈ 11.1Wh; at a realistic ~2.5–3W average draw this is
roughly **3–4 hours** per charge — comparable to the original four-AA alkaline
runtime, and rechargeable in place. The cavity (~52 cm³) is near the physical ceiling
for rechargeable lithium energy in this space; the charge-in-place USB-C path is what
turns that ceiling from a limitation into "plug in and keep going."

### 7.8 Battery Level Indicator — Re-Referencing the CX10043

The machine's front-panel level meter is the five-LED bar D801–D805 (GL-9PR10),
driven by IC801 — a Sony custom **CX10043** — with Q801 (2SC1623) as a voltage-control
stage and the S801 switch selecting the meter's function (BATT / OFF / PARK). In
normal operation the five LEDs form the audio peak meter. In the BATT position the
stock circuit does *not* show a bar: it drives only the bottom LED (D801) and
indicates battery state by that LED's **brightness**, dimming as the cells deplete.
Q801 performs that brightness drive off the R808/R809/R810 (12k/56k/68k) divider.
Owners have long considered this scheme close to useless. The redesign below both
fixes it and upgrades it to the proper bar the hardware was always capable of.

**Why the battery indicator stops working in this design.** In the stock machine the
BATT brightness tracked the machine's main rail, which *was* the raw 4×AA pack — 6V
fresh, sagging as the cells depleted. The DSR-1 turns that rail into a regulated,
constant 6.0V B+1. With S801 on BATT, Q801 now sees a constant 6.0V and holds D801 at
full brightness regardless of the pack's real charge. The true state of charge has
moved to the VBAT node on the daughter board, which the indicator circuit no longer
sees.

**Architecture.** The STM32 (on the servo module) reads VBAT through the VBAT_SENSE
conductor (Section 7.6), estimates state of charge, and drives the five LED nodes as a
bar in BATT mode. It must not contend with the CX10043's peak-meter drive, so the
takeover is gated by sensing the S801 BATT position on a GPIO:

```
VBAT (daughter board) ──► VBAT_SENSE ──[22k]──┬── ADC_VBAT (STM32)
                                            [100k]
                                              │
                                             GND

S801 BATT pole ──► GPIO (mode sense)

STM32 GPIO/PWM ──[Rs]──►│├─ D801 ┐
                ─[Rs]──►│├─ D802 │   (driven only when S801 = BATT;
                ─[Rs]──►│├─ D803 │    released to CX10043 otherwise)
                ─[Rs]──►│├─ D804 │
                ─[Rs]──►│├─ D805 ┘
```

**VBAT sense divider.** R_top = 22 kΩ, R_bot = 100 kΩ, ratio 0.82. At the 4.2V LiPo
peak this presents ~3.45V to the ADC — above the 3.3V VDDA ceiling, so for LiPo either
raise R_top (e.g. 33 kΩ → ~3.05V at 4.2V) or accept clamping above full; at the 3.0V
floor a 33k/100k divider presents ~2.26V. A 10nF cap at the ADC node sets the sampling
time constant. Gate the divider's ground leg with a GPIO-controlled small-signal N-FET
so it draws nothing except during a measurement.

**LED interface.** Drive each LED node from a GPIO through a series resistor, or — to
preserve the original 180Ω limiters and avoid five extra pins — through small Schottky
diodes into the existing D801–D805 anodes so the STM32 can source them in BATT mode
while CX10043 is idle. Brightness is set by GPIO PWM, so the bar can also be dimmed for
night use.

**Firmware.** State of charge comes from a **LiPo open-circuit-voltage lookup table**
with light filtering and a fixed IR-drop offset for the known load. The LiPo discharge
curve (4.2V → 3.0V) is usefully sloped, so voltage-to-SoC mapping is reliable — easier
than the flat LFP curve the earlier design had to work around. SoC% → number of lit
segments (1–5). Reading the BQ24074's CHG/PGOOD status lets the firmware animate
segments upward while charging, hold steady-full when charge terminates, and flash the
bottom segment below ~15%.

**Bench-confirm items.** Two things to characterise before finalising the LED
interface: (1) how S801's BATT position reroutes the LED nodes, and whether the
CX10043 LED outputs go high-impedance in that position (so the STM32 can drive them
without contention); and (2) the LED forward characteristics through the existing
180Ω limiters, to size the GPIO drive or Schottky-injection resistors. The Q801
brightness path can simply be left disconnected once the STM32 owns the bar.

## 8. Power Rail Interdependencies and Sequencing

### 8.1 Rail Startup Order

When power is first applied (regardless of variant), the rails power up in a
specific order determined by the time constants of each converter:

1. **B+1 appears first** (approximately 1-5ms after input voltage application)
   — produced by the TPS63070 B+1 regulator, fed through the fitted front end (the
   diode bridge and polyfuse in Variant B, the PD path in Variant A, or the BQ24074
   power-path on the daughter board in the battery build). The TPS63070's own
   soft-start sets the B+1 rise time. In the battery build the BQ24074's power-path
   keeps SYS supplied from the pack the instant USB is absent, so there is no
   source-handoff gap at power-on.

2. **B+3 appears approximately 2-3ms after B+1** — the MT3608 must first build up
   inductor current and bring the feedback loop into regulation.

3. **3.3V appears approximately 1ms after B+1** — the MCP1700 LDO responds quickly
   with only its output capacitor charging time as delay.

4. **STM32 releases from reset approximately 1ms after 3.3V** reaches the POR
   threshold — the MCU begins executing.

5. **Servo loop begins approximately 2ms after power-on** — by this time B+3 is
   fully regulated and the motor can be driven.

The motor (M901) is never driven before the servo loop is running because the Q601
base pullup resistors hold Q601 off until the DAC or PWM output is explicitly
configured by firmware. This means B+3 can be present before the servo loop runs
without causing uncontrolled motor operation.

### 8.2 What Happens When B+1 Varies

The MT3608's feedback loop maintains B+3 constant regardless of B+1 variations.
If B+1 drops from 6.0V to 5.5V (e.g., a wall adapter sagging under load, or the
TPS63070 reaching its boost-mode input limit as the pack nears empty), the MT3608
increases its duty cycle to compensate:

```
D_new = 1 - (5.5 / 10.8) = 0.491  (was 0.444)
```

The motor continues to receive 10.8V. The servo loop continues to operate at its
calibrated setpoint. Tape speed is unaffected until B+1 drops below the MT3608's
minimum input voltage (2V from the datasheet).

The MCP1700 continues to regulate 3.3V as long as B+1 stays above 3.478V (3.3V +
178mV dropout). Below that, the STM32 supply drops, the STM32 behaviour becomes
unpredictable, and the BOR resets the MCU at approximately 2.8V supply. The machine
stops cleanly.

### 8.3 B+3 Load Step Response

The WM-D6C motor draws varying current depending on tape tension, reel fullness,
and transport mode. A transition from no-load (pause) to full-load (play) represents
a step change in B+3 current demand. The MT3608's control loop response time:

**Bandwidth of MT3608 control loop**: approximately 100kHz (typical for this class
of converter at 1.2 MHz switching frequency).

**Response time to a load step**: approximately 1/100kHz = 10µs to begin correcting.

**Output voltage droop during the response time**: The output capacitor (effective
22µF after DC bias derating — see Section 3.5) supplies the additional current
during the 10µs response:

```
ΔV = I_step × t / C = 0.15 × 10×10⁻⁶ / 22×10⁻⁶ = 68mV
```

This 68mV droop on the B+3 rail during a load step is well within acceptable limits
for the motor drive circuit. The motor's own inductance (several millihenries)
further filters this transient, and the servo loop compensates for any resulting
speed perturbation.

---

## 9. Component Selection Tradeoffs and Alternatives

### 9.1 MT3608 vs Other Boost Converters

The MT3608 was chosen over alternatives (LT1613, TPS61023, MAX1760) primarily for
cost and availability on LCSC. The MT3608 is one of the most widely stocked and
inexpensive boost converters available — under $0.10 in single quantities on LCSC.
Its performance at the WM-D6C operating point is adequate, its package (SOT-23-6)
fits the height constraint, and it is familiar to the community from its widespread
use in DIY electronics.

**Weakness**: The MT3608 is a simple, non-synchronous (diode rectifier) boost
converter. A synchronous boost (replacing the SS14 with an active MOSFET) would
improve efficiency by approximately 3-5 percentage points. For the DSR-1's power
levels, this represents 50-80mW of recovered power — not significant. The SS14
diode rectifier is the correct tradeoff for this application.

### 9.2 MCP1700 vs Other LDOs

The MCP1700 was chosen for its ultralow quiescent current (1.6µA), its input
voltage range matching B+1 exactly, its small package, and its widespread
availability. The LDO is a commodity component; any SOT-23-3 LDO with:
- Output: 3.3V fixed or adjustable
- Input range: 2.3V to 6.5V minimum
- Dropout: < 300mV at 50mA
- Quiescent current: < 50µA
- Package: SOT-23-3

...can substitute for the MCP1700. The RT9013, XC6206, and AP2112K are all
pin-compatible alternatives available at similar cost.

---

## See Also

- [USB PD Without Firmware](usb-pd-without-firmware.md) — how the IP2721 negotiates
  power delivery without MCU involvement
- [Signal Chain Analysis](signal-chain-analysis.md) — noise coupling from the MT3608
  into the analog signal paths
- [Why This Failed](why-this-failed.md) — the original power system failure modes
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — complete electrical
  specification including absolute maximum ratings and operating characteristics
