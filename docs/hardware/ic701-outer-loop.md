# IC701 Outer Loop: Schematic Audit and DSR-1 Installation Analysis

## Purpose of This Document

When the DSR-1 Servo Control Board replaces IC601 (CX20084), it eliminates the original inner
servo loop entirely. However, the WM-D6C's original servo architecture was a
**two-loop system**, not one. The second loop — the crystal phase comparator outer
loop — involves IC701 (MSM58141RS) on the Auto-Off board. This document answers the
question: what happens to that outer loop when IC601 is absent, does it interfere
with the DSR-1, and does anything need to be done about it?

The answer, established by direct schematic tracing, is that the outer loop is
open-circuited at both ends by the removal of IC601, that it has no electrical path
to interfere with DSR-1 operation, and that no hardware modification is required.

---

## 1. The Two-Loop Architecture of the Original Servo

The original servo system was not a single loop. It comprised two nested control
loops operating on different timescales:

### 1.1 Inner Loop — FG Speed Control (IC601)

The CX20084 (IC601) measured the FG pulse period at pin 13 and compared it to a
reference derived from IC701's divided crystal output at pin 14. The resulting error
voltage passed through the internal sample/hold, double integrator, ramp frequency
amplifier, sawtooth wave generator, and comparator chain, and exited at pin 15 to
control Q601's base current. This inner loop corrected speed errors on a timescale
of individual FG pulse periods — at 2500 Hz, approximately every 400 µs.

### 1.2 Outer Loop — Crystal Phase Correction (IC701)

The CX20084 also produced a divided version of the FG pulse stream at **pin 9**,
which was routed via wire harness to the Auto-Off board and into IC701. IC701's
phase comparator section compared this divided FG stream against the divided 34.7 kHz
crystal reference. The resulting phase error was output from **IC701 pin 5**, passed
through **R715** (a resistor on the Auto-Off board that sets the drive strength of
the correction signal), and the resulting voltage was integrated by **C605** (also on
the Auto-Off board, forming a first-order low-pass integrator with R715) before
returning via wire harness to **IC601 pin 4** — the phase correction summing node.

This outer loop operated on the slowly varying average phase relationship between
the FG stream and the crystal, correcting for any residual DC speed error that the
inner loop's proportional response could not fully eliminate. Its time constant —
set by R715 × C605 — was much longer than the inner loop's bandwidth, so the two
loops did not compete.

### 1.3 The Complete Signal Path

```
FG901 optical sensor
    │
    ▼
IC601 pin 13 ──► [inner FG speed loop inside CX20084]
                                    │
IC601 pin 15 ◄──────────────────────┘
    │
    ▼
Q601 base → motor current → motor speed

IC601 pin 9 ──► [wire to Auto-Off board] ──► IC701 FG input
                                                    │
                                          [crystal phase compare]
                                                    │
                                          IC701 pin 5
                                                    │
                                          R715 (Auto-Off board)
                                                    │
                                          C605 to GND (integrator)
                                                    │
                                    [wire back to main board]
                                                    │
                                                    ▼
                                          IC601 pin 4 (phase correction input)
```

The IC701 outer loop is therefore a slow-acting phase trim that feeds a correction
voltage into IC601's summing node, nudging the inner loop's operating point to
maintain long-term phase lock against the crystal reference.

---

## 2. Schematic Verification

The above topology was confirmed by direct tracing of the WM-D6C/TC-D6C service
manual schematic (Section 4.2, pages 23–25 of the service manual, spanning the Main
Board and Auto-Off Board layouts).

Key findings from the schematic trace:

**IC601 pin 9** exits the CX20084 block, routes downward in the schematic, crosses
the dashed board boundary line, and enters the IC701 block (labelled "CRYSTAL OSC /
DIVIDER / PHASE COMPARATOR", part MSM58141RS) on the Auto-Off board. This is an
input to the PHASE COMPARATOR section of IC701.

**IC701 phase comparator output** exits the IC701 block rightward. The node is
labelled with the bias supply annotation ("B+VS 3.3V/5V") and connects to R715.
R715 is a discrete resistor on the Auto-Off board between IC701 pin 5 and the
correction voltage node. C605 connects from this node to ground, forming the RC
integrator. The node then connects to a wire that routes upward, crosses the board
boundary, and enters the Main Board where it connects to **IC601 pin 4**.

**The PLL bypass solder bridge** is also visible on the schematic. This is a PCB
pad on the Main Board that, when shorted, disconnects the IC601 pin 9 output from
the IC701 input path. The service manual speed calibration procedure requires this
bridge to be opened before setting RV601 (base speed trim) and closed afterward to
re-engage crystal phase lock. Its presence confirms that Sony's engineers knew the
outer loop had to be independently disengageable for calibration.

**X701 crystal and the oscillator/divider** section of IC701 are entirely separate
from the phase comparator section. The crystal oscillator drives the divider chain
(the row of flip-flops visible in the schematic as a chain of D-type cells), which
produces the divided reference for both the PHASE COMPARATOR within IC701 and the
divided reference output to IC601 pin 14. The auto-off and tape counter functions
use other outputs of the divider chain and are independent of the phase comparator.

---

## 3. What Happens When IC601 Is Removed

When the CX20084 is removed from its footprint on the Main Board and the DSR-1
Servo Control Board is installed to drive Q601 directly:

### 3.1 IC601 Pin 9 Net — FG Pulses to IC701

The wire that previously carried divided FG pulses from IC601 pin 9 to IC701's FG
input on the Auto-Off board now connects to the **empty IC601 pad**. No signal
drives it. IC701's phase comparator FG input is floating — it receives neither a
valid FG pulse stream nor a defined logic level.

**Effect on IC701**: The phase comparator operates with a floating input on one
side. Its output at pin 5 will be indeterminate — likely drifting to a rail or
oscillating at low frequency depending on the IC's internal input configuration.
The MSM58141RS input structures are CMOS — a floating input is high-impedance and
will pick up stray coupling. In practice the comparator output will be rail-to-rail
noise or a slow oscillation.

**Effect on DSR-1**: None. The output of IC701 pin 5 feeds through R715 and C605
and terminates at the empty IC601 pin 4 pad. There is no connection from this net
to J1 or to any DSR-1 signal. The noise on IC701 pin 5 is isolated from the DSR-1
module.

### 3.2 IC601 Pin 4 Net — Phase Correction Voltage from IC701

The wire that previously carried the R715/C605 integrator output to IC601 pin 4
now terminates at the **empty IC601 pad**. IC701 pin 5 → R715 → C605 → open circuit.

C605 will charge or discharge to whatever DC level IC701 pin 5 settles at, through
R715. The resulting voltage sits on the Auto-Off board at the C605/R715 junction,
floating to some indeterminate level referenced to the Auto-Off board's GND.

**Effect on DSR-1**: None. This net has no connection to J1. The Servo Control Board's
Q601_BASE output (J1 pin 2) drives Q601's base directly and does not share any
node with the R715/C605 net.

**Effect on Auto-Off board**: None. The indeterminate voltage on the C605/R715 node
does not affect the auto-off timer, the tape counter, the crystal oscillator, or
the divider chain. These circuits use separate supply and signal nets within IC701.

### 3.3 Summary: The Outer Loop Is Electrically Isolated

```
IC601 pin 9 pad (empty) ←── dead end
    (previously received: divided FG pulses from CX20084 inner loop)

IC701 phase comparator:
    Input:  floating (was: IC601 pin 9 net)
    Output: → R715 → C605 → IC601 pin 4 pad (empty) ← dead end

No path exists from this network to:
    - J1 (DSR-1 interface connector)
    - Q601 base
    - FG901 signal
    - Any STM32 pin
    - B+1, B+3, or GND of the Servo Control Board
```

The outer loop is open at both ends. It cannot inject signals into the DSR-1
control path. No interference occurs.

---

## 4. Why the DSR-1 Does Not Need the Outer Loop

The original outer loop served one function: eliminating the residual DC speed error
that the inner FG frequency loop could not fully correct on its own. The inner loop
was proportional in nature — at zero error its correction term was zero, meaning any
systematic bias (bearing friction, motor characteristic variation, tape tension) would
produce a small but nonzero steady-state speed offset.

The DSR-1's digital PI control loop includes an **integral term** that explicitly
accumulates all past speed errors and applies a correction proportional to the
accumulated sum. This integral action is mathematically equivalent to what the outer
crystal phase loop provided, but it is:

- **More precise**: the integrator accumulates in 32-bit integer arithmetic with
  no thermal drift, component aging, or supply voltage sensitivity
- **Faster acting**: the integral accumulates on every FG edge (2500 times per
  second at nominal speed) rather than on the slow RC time constant of R715 × C605
- **Adjustable**: Ki can be changed via USB CDC without hardware modification
- **Observable**: the integral accumulator value is visible in the USB telemetry
  output at any time

The outer loop's function is therefore already superseded by the integral term in the
DSR-1 firmware. Reintroducing it — for example, by connecting IC701 pin 5 to an
STM32 ADC input — would add a competing correction signal that would interfere with
the integral term rather than complement it. It would also reintroduce all the
thermal drift and component aging characteristics that the DSR-1 design deliberately
avoids.

---

## 5. IC701 Functions That Remain Active

IC701 is not purely a servo component. The MSM58141RS also provides:

**Tape counter clock**: The divider chain output drives the tape counter display
logic. This function uses divider outputs that are independent of the phase
comparator section and continues to operate normally.

**Auto-off timing**: The auto-off circuit on the same board uses IC701 divider
outputs for its timing reference. Q701 and Q702 (the auto-off transistors) and
their associated logic remain functional. The auto-off signal that drives the
MOTOR_EN net (J1 pin 3 on the Servo Control Board) is produced by this circuit and
continues to work correctly.

**Crystal oscillator**: X701 and its associated inverter buffer chain within IC701
continue to oscillate at 34.7 kHz. This signal is not used by the DSR-1 but its
continued operation has no adverse effect.

The phase comparator section of IC701 is the only part affected by the removal of
IC601, and as established above, it has no electrical path to interfere with
DSR-1 operation.

---

## 6. Optional: Stabilising IC701's Floating Input

While the floating IC701 FG input has no electrical effect on the DSR-1, a
floating CMOS input is marginally undesirable from a board-health standpoint — it
draws slightly more supply current than a defined logic level (due to the CMOS
input transistors both partially conducting). The additional current is in the
microampere range and is not practically significant.

If absolute cleanliness is desired, the existing **PLL bypass solder bridge** on the
Main Board can be bridged permanently. This connects the IC601 pin 9 net to the
Speed Tune switch network at a defined point in the circuit, giving IC701's FG input
a defined logic level rather than a floating state. The effect on the DSR-1 is zero
either way, but it is the tidiest solution and requires no modification beyond
adding a small solder blob to an existing PCB pad that was already designed for
exactly this purpose.

Bridging the PLL solder pad is **optional, not required**.

---

## 7. Required Actions

| Item | Action | Priority |
|---|---|---|
| IC701 outer loop interference | None required — open-circuit at both ends | — |
| IC701 floating FG input | Optional: bridge PLL solder pad for tidiness | Low |
| Documentation | This document covers the open question | Done |
| Firmware | No change needed — PI integral supersedes outer loop | — |
| Calibration reference | Use 1084.75 Hz at pin 3 of IC601 footprint (or FG_RAW) as no-tape speed cross-check | See below |

### 7.1 No-Tape Speed Calibration Cross-Check

The outer loop's absence does not affect speed calibration, but it does remove the
benefit of IC701's crystal as an independent frequency reference check. In its
place, the following no-tape cross-check is available and should be added to the
calibration procedure:

The 34.7 kHz crystal divides by 32 to produce the nominal FG target frequency.
At exactly correct tape speed (4.75 cm/s), the FG signal measured at J1 pin 1
(or equivalently at the oscilloscope probe on FG901's output before the DSR-1
conditioning network) should be:

```
F_FG_nominal = 34,700 Hz ÷ 32 = 1,084.75 Hz
```

After initial calibration using a test tape and the USB CDC `r` telemetry command,
verify the measured FG frequency against this nominal value. A reading within ±5 Hz
of 1,084.75 Hz confirms the calibration is consistent with the crystal reference.
Larger deviations indicate either a test tape calibration error or that the unit's
specific slot count differs from the nominal 32-slot assumption — in the latter
case, the test tape measurement takes precedence and the nominal value is for
cross-check only.

This cross-check does not replace test tape calibration. It provides a secondary
sanity check using physics rather than a reference recording.

---

## 8. Relationship to Other Documents

- [Original Servo Circuit](original-servo-circuit.md) — describes IC601 and IC701
  in the context of the original two-loop design; Section 4 ("The Crystal Reference")
  describes IC701's divider function but does not cover the outer loop return path
  via R715/C605 to IC601 pin 4, which is fully documented here
- [Digital PLL Servo](digital-pll-servo.md) — describes the DSR-1 PI control loop
  and its integral term, which functionally supersedes the outer crystal phase loop
- [Signal Chain Analysis](signal-chain-analysis.md) — documents all signals on
  J1; the IC701 phase comparator output net is not on J1 and is therefore not covered
  there; this document covers it
- [Why This Failed](why-this-failed.md) — covers the CX20084 failure mode; the
  absence of IC601 is the precondition for this document's analysis
