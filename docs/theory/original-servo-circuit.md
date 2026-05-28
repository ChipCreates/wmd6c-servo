# The Original WM-D6C Capstan Servo Circuit

## Purpose of This Document

This document explains how the original Sony WM-D6C capstan servo system worked —
the CX20084 IC, the 34.7 kHz crystal reference, the MSM58141RS divider, and the
motor drive chain. Understanding the original circuit is the foundation for
understanding why the DSR-1 module replaces it, and why the digital implementation
is more robust than the analog circuit it replaces.

This is written for a technically curious person who has serviced cassette decks
before but is not necessarily a professional electronics engineer. The goal is
genuine understanding, not simplified hand-waving.

---

## 1. What the Servo System Does

A cassette tape must travel past the playback head at precisely 4.75 cm/s. This is
the international standard for compact cassette, defined by IEC 60094. Deviations
from this speed produce audible pitch shift — a 1% error at 440 Hz produces a tone
at 444 Hz or 436 Hz, which a trained ear hears immediately. A 2% error is obvious
to almost anyone. The WM-D6C is a professional field recorder; its specification of
0.05% wow and flutter (WRMS) means the speed variation is so small and so fast that
it is essentially inaudible under any normal conditions.

Achieving this requires more than simply applying a fixed voltage to a motor. Motors
vary in speed with load, temperature, battery voltage, bearing friction, and dozens
of other factors. A simple open-loop motor driver — voltage in, speed out — cannot
maintain 4.75 cm/s reliably. What is needed is a **closed-loop control system**: one
that continuously measures the actual tape speed, compares it to the desired speed,
and adjusts the motor drive voltage to correct any error. This is the capstan servo
system.

---

## 2. The Mechanical Chain

Before examining the electronics, it helps to understand the mechanical path from
motor to tape.

The capstan motor M901 drives a **flywheel** via a rubber belt. The flywheel is a
precision-machined metal disc with significant rotational inertia — its mass smooths
out any small speed variations in the motor itself, acting as a mechanical low-pass
filter. The **capstan shaft** is a hardened steel pin mounted concentrically on the
flywheel. When a cassette is in play mode, the **pinch roller** presses the tape
against the capstan shaft. The tape is driven forward by the friction between the
capstan shaft surface and the pinch roller rubber, at a speed determined entirely by
the capstan shaft's surface speed — which is, in turn, determined by the flywheel's
rotational speed.

The take-up reel motor provides gentle tension to keep the tape taut, but it does
not determine transport speed. The capstan and pinch roller are the sole speed
determinants.

---

## 3. The FG901 Optical Sensor

Mounted on the capstan motor shaft — not the flywheel, the motor shaft directly —
is a **slotted disc**. This is a thin metal disc with a series of precision-cut
slots around its circumference. The **FG901 GP2S22AB** optical sensor straddles
this disc: an infrared LED on one side shines through the slots, and a phototransistor
on the other side detects the light pulses.

As the motor rotates, the slots pass through the optical sensor beam, producing a
pulse train. Each pulse represents one slot passing. The **frequency** of this pulse
train is directly proportional to the motor's rotational speed. If the motor is
spinning at the correct speed for 4.75 cm/s tape transport, the pulse frequency is a
known value — let us call it F_FG. If the motor runs faster than correct, the
frequency rises above F_FG. If it runs slower, the frequency drops below F_FG.

This signal is called the **FG signal** — Frequency Generator. It is the servo
system's measurement of actual motor speed.

The FG901 is an open-collector device. Its output is pulled up to the B+1 supply
rail (nominally 6V from batteries) through a resistor on the main board. The output
swings between approximately 0V (phototransistor conducting, slot aligned with beam)
and B+1 (phototransistor off, disc material blocking beam). This signal is fed to
the CX20084 on pin 13.

---

## 4. The Crystal Reference

The CX20084 needs to know what speed is *correct* in order to detect and correct
deviations from it. It obtains this reference from the **X701 crystal oscillator**.

X701 is a 34.7 kHz quartz crystal mounted on the Auto-Off board. Quartz crystals
oscillate at a frequency determined by their physical dimensions with extraordinary
stability — typically better than ±50 parts per million over a wide temperature
range. At 34.7 kHz, this means the oscillation frequency stays within ±1.7 Hz of
nominal across normal operating temperatures. This stability is the foundation of
the servo system's accuracy.

The 34.7 kHz signal from X701 is fed to the **IC701 MSM58141RS**, a frequency
divider IC. IC701 divides the crystal frequency down by a fixed ratio, producing a
lower-frequency reference signal whose period represents "correct speed." This
divided signal becomes the servo system's reference clock — the target against which
the FG pulse train is compared.

The specific division ratio is chosen so that at exactly 4.75 cm/s tape speed, the
FG pulse rate and the divided crystal reference are **identical in frequency and in
phase**. When they match, the servo is locked and no correction is needed.

---

## 5. The CX20084 Phase-Locked Loop

The CX20084 is a proprietary Sony ASIC (Application-Specific Integrated Circuit)
designed specifically for cassette servo applications. It implements an **analog
phase-locked loop (PLL)**.

A PLL has three fundamental components: a phase detector, a loop filter, and a
voltage-controlled output. In the CX20084:

### 5.1 Phase Detector

The phase detector receives two signals: the FG pulse train from FG901 (via pin 13)
and the divided crystal reference from IC701 (via pin 14). It continuously computes
the **phase difference** between these two signals.

Phase difference is more than just frequency difference. Two signals can have the
same average frequency but still be offset in time — one slightly ahead of the
other. The phase detector measures this time offset and produces an output voltage
proportional to it. Zero phase difference produces a reference voltage (typically
mid-supply). A positive phase error (FG pulses arriving early — motor running fast)
produces a voltage above reference. A negative phase error (FG pulses arriving late
— motor running slow) produces a voltage below reference.

This error voltage is a continuous analog signal. It contains information about both
frequency errors (sustained speed deviations) and phase errors (instantaneous
timing deviations). The combination makes the PLL sensitive to both slow drift and
fast fluctuations.

### 5.2 Loop Filter

The error voltage from the phase detector is passed through a **loop filter** — a
network of resistors and capacitors (components C311, C607, and associated resistors
around IC601 on the main board) that determines the servo system's dynamic
behaviour.

The loop filter has two critical parameters:

**Bandwidth** — how fast the servo responds to speed errors. A wide bandwidth means
fast response to disturbances but can cause the servo to overcorrect and oscillate.
A narrow bandwidth means slow response but very smooth, stable speed. The WM-D6C's
loop filter is tuned for a bandwidth appropriate to cassette tape mechanics —
responding fast enough to correct for capstan bearing irregularities and pinch
roller variations, but slowly enough to ignore the mechanical vibration of the motor
brushes.

**Damping** — how the servo behaves when correcting an error. An underdamped system
oscillates around the correct speed before settling. An overdamped system corrects
slowly without oscillation. The WM-D6C filter achieves critical or slightly
overdamped response — the fastest settling time without overshoot.

The values of the loop filter components were determined empirically by Sony's
engineers during development and are fixed in hardware. They cannot be adjusted
without modifying the PCB.

### 5.3 Voltage-Controlled Motor Drive

The filtered error voltage drives pin 15 of the CX20084, which is the motor drive
output. This pin controls the base of Q601, a 2SB1013 PNP transistor whose collector
current drives the capstan motor M901.

Q601 is configured as a common-emitter amplifier. Its emitter is connected to the
B+3 motor supply rail (approximately 10.8V, produced by the CP304 boost converter).
Its collector drives one terminal of M901 through the Q703/Q704 mode-switching
circuit. The motor current — and therefore the motor speed — is determined by the
base current, which is determined by the voltage on pin 15 of the CX20084.

Because Q601 is a PNP transistor, the relationship between base voltage and
conduction is **inverted**: a lower base voltage causes more base current and
therefore more collector current and more motor drive. A higher base voltage
reduces motor drive. This means:

- Motor running **too fast** → FG frequency too high → phase error indicates fast
  → CX20084 increases pin 15 voltage → Q601 conducts less → motor slows down

- Motor running **too slow** → FG frequency too low → phase error indicates slow
  → CX20084 decreases pin 15 voltage → Q601 conducts more → motor speeds up

This feedback loop runs continuously, correcting deviations in real time.

---

## 6. Speed Trim Potentiometers

Three potentiometers allow the servo speed reference to be adjusted:

**RV601** (47kΩ cermet trimmer, rear panel, covered by a small plug): Sets the base
calibration of the servo reference. This is adjusted once during factory calibration
or after service to set the exact correct tape speed. Under normal circumstances it
should not require readjustment.

**RV602** (20kΩ carbon slider, front panel Speed Tune control): Allows the user to
shift the playback speed over a small range, typically ±2-3%. This is used when
playing back tapes recorded on a machine with a slightly different speed calibration,
or when the listening environment causes pitch perception shifts.

**RV603** (47kΩ cermet trimmer): Sets the sensitivity of RV602 — how much speed
change is produced by full travel of the Speed Tune slider.

These potentiometers feed voltage divider networks that offset the crystal reference
seen by the CX20084, effectively shifting the target speed up or down. The servo
loop then locks to this shifted reference rather than the raw crystal frequency.

---

## 7. The CP304 Boost Converter

The capstan motor M901 operates most efficiently at a supply voltage of approximately
10.8V, which is higher than the 6V battery supply. The **CP304 module** (Sony part
1-464-183-21) is a potted DC-DC boost converter that steps the 6V B+1 rail up to
approximately 10.8V for the motor supply (B+3 rail).

CP304 is a complete, self-contained switching converter module — its internal
topology and component values are not documented in the service manual. It is a
proprietary Sony assembly. When CP304 fails — which it eventually does in all
surviving units due to electrolytic capacitor degradation in the potted assembly —
the 10.8V motor rail collapses, the motor receives insufficient voltage, and the
machine either runs slowly or stops entirely.

The DSR-1 module replaces CP304 with an MT3608-based boost converter designed with
known, documented, replaceable components.

---

## 8. The Failure Mode

The CX20084 receives its supply voltage directly from the B+1 rail through a simple
RC filter. There is no reverse voltage protection, no crowbar circuit, and no
transient suppression between CN301 and the IC's supply pin.

CN301 is a 5.5mm outer diameter barrel jack with the **centre pin as negative and
the sleeve as positive**. This is the opposite of the convention adopted by virtually
every other manufacturer of consumer and professional electronics. The Sony AC-D4M
adapter supplied with the machine uses this non-standard polarity, as does the Sony
BP-23 rechargeable battery pack.

When a standard centre-positive adapter is connected to CN301, the B+1 rail is
driven to the adapter's output voltage with reversed polarity. The CX20084's supply
pin is driven to a large negative voltage relative to its ground reference. The IC
contains no protection against this condition. Its internal transistors operate in
avalanche breakdown. The damage is instantaneous and irreversible — the IC fails
within milliseconds of connection.

The CX20084 is a Sony ASIC from the 1980s. It has been out of production for decades.
There is no modern equivalent, no pin-compatible replacement, and no repair path for
the damaged IC itself. A WM-D6C with a dead CX20084 cannot be restored to original
specification using original parts.

This is the failure mode that the DSR-1 module was designed to address — not by
repairing the original circuit, but by replacing it entirely with a more capable and
permanently protected alternative.

---

## 9. Limitations of the Original Design

With the benefit of hindsight, the original analog servo has several inherent
limitations that the digital replacement does not share:

**Temperature drift**: The crystal frequency and the loop filter time constants both
vary with temperature. In a device carried in a shirt pocket in a warm studio versus
used outdoors in cold weather, the servo reference can shift by tens of parts per
million, producing a small but measurable pitch shift.

**Component aging**: The electrolytic capacitors in the loop filter age over decades,
changing their capacitance and ESR values. This shifts the loop bandwidth and
damping, eventually causing the servo to behave differently than it did when new.
In some cases the loop becomes underdamped and audible speed oscillation results.

**Crystal aging**: Quartz crystals age, shifting their frequency over decades. A 30-
year-old X701 crystal may have drifted enough to require RV601 readjustment that
was not needed when the machine was new.

**Fixed loop parameters**: The resistor and capacitor values around CX20084 are fixed
in hardware. If the motor's dynamic characteristics change — due to bearing wear,
brush wear, or lubricant changes — the servo's response cannot be adjusted to
compensate.

**No telemetry**: There is no way to observe the servo's internal state without
specialised test equipment. Diagnosing a poorly-performing servo requires an
oscilloscope, a test tape, and considerable experience.

The DSR-1 module addresses all of these limitations. Its reference is a digital
integer stored in flash — it does not drift with temperature or age. Its loop
parameters are constants in a header file, adjustable in real time via USB. Its
internal state is observable via USB CDC telemetry at any time. And it is immune to
the polarity reversal that destroyed the original.

---

## See Also

- [Digital PLL Servo](digital-pll-servo.md) — how the DSR-1 replacement works
- [Why This Failed](why-this-failed.md) — the reverse polarity failure explained
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — complete DSR-1 specification
