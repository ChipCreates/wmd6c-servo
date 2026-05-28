# Why Your WM-D6C Failed — and How to Make Sure It Never Happens Again

## You Are Probably Reading This Because

Your Sony WM-D6C plays tapes at the wrong speed, or doesn't control speed at all —
it runs fast, runs uncontrolled, or the speed is erratic. You may have also just
plugged in a DC power adapter and heard nothing, or the machine worked briefly then
stopped. You searched for the problem and found this repository.

This document explains exactly what happened, why it happened, and what the DSR-1
module does about it.

---

## The Short Version

Someone connected a standard DC power adapter to your WM-D6C. The adapter had the
wrong polarity for this machine. The polarity reversal instantly and permanently
destroyed the capstan servo IC — a Sony CX20084 chip that has been out of production
for decades. There is no repair for the destroyed IC using original parts. The
DSR-1 module replaces it entirely with a modern microcontroller, and eliminates the
failure mode so it can never happen again.

---

## 1. The Unusual Power Jack

The WM-D6C has a DC power input jack (CN301) on the lower panel. It is a standard-
looking 5.5mm outer diameter / 2.1mm centre pin barrel connector — the same
physical format used by hundreds of audio devices, guitar pedals, and consumer
electronics products.

But it is wired **backwards** compared to almost everything else made since the
1980s.

On virtually every device that uses this barrel jack format, the convention is:
**centre pin = positive (+), outer sleeve = negative (−)**. This is so universal
that it is effectively an industry standard, and most generic DC adapters are wired
this way by default.

On the WM-D6C, it is the opposite: **centre pin = negative (−), outer sleeve =
positive (+)**.

Sony used this polarity on certain professional equipment in the 1980s, and supplied
the machine with a matching Sony AC-D4M adapter that uses the same non-standard
wiring. The adapter's plug is physically identical to a standard adapter, so there
is no visual indication that the polarity is reversed.

Over 35 years, the original Sony adapters have been lost, discarded, borrowed and
never returned, or simply failed. The machine has passed through multiple owners.
At some point, someone looked at the barrel jack, picked up what appeared to be a
compatible generic adapter, and plugged it in.

---

## 2. What Happened Electrically

The WM-D6C's B+1 power rail — the main 6V supply that powers the audio circuitry,
the servo system, and the logic boards — connects directly to CN301 through a simple
wiring harness. There is no protection diode, no polarity detection circuit, no
fuse, and no reverse-voltage crowbar between the jack and the main board.

When a standard centre-positive adapter is connected:

The B+1 rail, instead of sitting at +6V relative to chassis ground, is driven to
−6V (or whatever the adapter's output voltage is) relative to ground. Every
component connected to B+1 now has its supply rail at a large negative voltage.

Most of the components on the main board survive this. The audio ICs (IC101, IC201,
IC301, IC302) have substrate protection diodes that clamp the supply to approximately
−0.6V, limiting the current and preventing catastrophic failure. The transistors in
the motor drive circuit similarly survive through substrate conduction.

The CX20084 servo IC (IC601) does not survive.

The CX20084 is a CMOS ASIC designed to operate from a 6V supply with its internal
reference set to that supply voltage. When the supply pin is driven to a large
negative voltage, the internal transistors are biased into reverse avalanche
breakdown. The junctions conduct in an uncontrolled manner, drawing large currents
through paths that were not designed to carry them. The silicon junction temperatures
spike to destructive levels within milliseconds. The result is permanent physical
damage to the silicon — metal interconnects vaporise, junctions short, or oxide
layers rupture.

The machine's capacitors and the adapter's output capacitance extend this destructive
event over several milliseconds rather than nanoseconds — long enough to ensure
thorough destruction of the IC's internal structures.

When power is removed and reconnected with the correct polarity, the audio circuits
often work because they survived. The servo does not work because the CX20084 is
physically destroyed.

**The symptom is almost always: audio works, but tape speed is wrong.** The machine
plays. You can hear the recording clearly. But it plays at the wrong speed —
typically faster than correct, because without the servo actively controlling the
motor, the motor runs at an uncontrolled speed determined by the open-loop drive
transistor biasing rather than feedback.

---

## 3. Why the CX20084 Cannot Be Replaced With Original Parts

The CX20084 is a Sony proprietary ASIC — Application-Specific Integrated Circuit.
It was designed by Sony specifically for their cassette servo products, manufactured
in volume during the 1980s, and has been out of production for well over 30 years.

There is no second-source manufacturer. There is no pin-compatible replacement from
any other manufacturer. There is no modern equivalent. The part number does not
appear in any current distributor catalogue.

Occasional new-old-stock (NOS) units appear on eBay, typically pulled from donor
machines or old service stock. These command prices of $50-150 or more per IC, their
provenance is uncertain, and there is no guarantee they have not themselves been
damaged by incorrect storage or previous handling. Installing an unknown-provenance
CX20084 into a repaired machine and then connecting the wrong adapter again would
simply destroy it a second time.

The DSR-1 module sidesteps this problem entirely. Instead of replacing the
CX20084 with another CX20084, it replaces the *function* of the CX20084 with a
modern, currently-manufactured, fully-documented microcontroller that performs the
same servo control task with better accuracy and permanent protection against the
failure mode.

---

## 4. How Common Is This Failure?

Extremely common. In the vintage audio restoration community, a failed CX20084 from
reverse polarity is considered the default assumption for any WM-D6C with a speed
problem — it is the *first* thing experienced restorers check, not the last.

Contributing factors that make this failure uniquely prevalent:

**Age**: These machines are 35-40 years old. Original accessories have been
separated from the units across decades of ownership changes. The original Sony
AC-D4M adapter is genuinely difficult to find.

**The jack looks standard**: Someone who knows about barrel jack conventions will
plug in their generic adapter with complete confidence that they are doing the right
thing. The WM-D6C gives no indication — no label, no diagram, no colour coding —
that its polarity is non-standard.

**Estate sales and bulk lots**: Many WM-D6C units in circulation today came from
estate sales, storage unit auctions, and broadcast station equipment disposals where
the original documentation and accessories were not included. The new owner has no
way to know about the polarity requirement.

**The damage is silent**: There is no spark, no smoke, no blown fuse, and often no
immediate obvious symptom. The machine may appear to power on normally. The audio
may work. The failure only becomes apparent when the tape speed is checked, which
may be some time after the damaging event.

---

## 5. What the DSR-1 Module Does About It

The DSR-1 module addresses this failure mode at three levels:

### Level 1: Replace the destroyed component

The CX20084 is replaced by the STM32G0B1KBU6 microcontroller running a digital PI
servo loop that performs the same speed control function — in fact, more accurately
than the original. This restores the machine to full operational specification.

### Level 2: Eliminate the failure mode

**Variant A (USB-C)**: CN301 is removed. The panel opening is modified to accept a
USB-C connector. USB-C is a physically symmetrical connector — it cannot be inserted
upside down, and the USB Power Delivery protocol negotiates the supply voltage
electronically before any power is delivered. It is physically impossible to apply
the wrong polarity through a USB-C connector.

**Variant B (protected barrel jack)**: CN301 is retained in the panel. However, the
module's input stage includes an ideal diode bridge — four MOSFETs controlled by an
LTC4359 — that detects the polarity of whatever is connected and corrects it before
the voltage reaches any sensitive circuitry. A standard centre-positive adapter,
the original Sony AC-D4M, or any other adapter of either polarity produces the same
correct-polarity output. It is impossible to apply damaging reverse polarity to the
machine through Variant B's input stage.

In addition, Variant B includes a resettable polyfuse that limits input current
during fault conditions, and a TVS transient suppressor that clamps any voltage
spike above 7V to ground, protecting against motor back-EMF transients and adapter
overvoltage.

### Level 3: Prevent future damage through education

This document exists because the best protection is knowing about the failure mode
before it happens. If you have a WM-D6C that is currently working, and you are
reading this before connecting any power source to it: **verify the polarity of any
adapter before connecting it.** Use a multimeter on DC voltage, touch the red probe
to the centre pin and the black probe to the outer sleeve, and confirm the reading
is negative (indicating negative centre, correct for WM-D6C).

Better yet, install the DSR-1 Variant B module as a preventive measure, even on a
healthy machine. The protection it provides costs approximately the same as a single
replacement CX20084 of uncertain provenance, and it makes the machine immune to this
failure permanently.

---

## 6. Other Causes of Incorrect Speed

The reverse polarity failure is the most common cause of a WM-D6C with uncontrolled
or incorrect tape speed, but it is not the only one. Before assuming the CX20084 is
the problem, rule out mechanical causes — they are cheaper and easier to fix:

**Worn or degraded capstan belt**: The rubber belt that drives the flywheel from the
motor stretches and hardens with age. A stretched belt slips under load and changes
the effective drive ratio. A glazed belt produces erratic speed. Belt replacement is
inexpensive and straightforward.

**Hardened pinch roller**: The pinch roller rubber hardens over decades, losing its
grip on the tape. This causes the tape to slip rather than being driven at capstan
speed, producing apparent fast or erratic playback speed.

**Contaminated FG slotted disc**: A film of lubricant, dust, or belt rubber residue
on the slotted disc can partially block the optical sensor beam, reducing the FG
pulse amplitude to the point where the servo loses lock. Cleaning the disc with
isopropyl alcohol often resolves this.

**Failed X701 crystal**: The 34.7 kHz reference crystal can crack, drift, or stop
oscillating entirely. If the crystal is dead, the servo has no reference and runs
open-loop at an uncontrolled speed — the same symptom as a failed CX20084.

**The speed tune switch S601**: If this switch is faulty or in an unexpected
position, it can modify the servo reference and cause incorrect speed.

A calibrated test tape and a frequency counter or phone spectrum analyser app can
distinguish between these causes: if the speed is consistently fast or slow by a
fixed amount, the servo reference is wrong (crystal, pot calibration). If the speed
wanders, the problem is mechanical.

---

## 7. Checklist Before Installing the DSR-1 Module

Complete these checks before installing the module:

1. **Clean the battery compartment**: If battery leakage is present, neutralise
   with white vinegar on a cotton swab, then clean with isopropyl alcohol. Check
   that leakage has not tracked onto the main board.

2. **Test Q601**: The 2SB1013 PNP motor drive transistor (IC601 pin 15 driver) may
   have been damaged in the same reverse polarity event. Test in diode mode:
   base-to-emitter and base-to-collector should both read 0.6-0.7V. A zero reading
   or short indicates failure — replace with a PNP transistor of equivalent rating.

3. **Replace the capstan belt**: At this age the belt needs replacement regardless
   of the electronic fault. Do it now while the machine is open.

4. **Replace the pinch roller**: Same rationale. Rubber this old is hardened.

5. **Clean the FG slotted disc**: With IPA on a cotton swab, while the mechanism
   is accessible.

6. **Measure B+3 rail**: With batteries installed, measure the CP304 output. It
   should be approximately 10.8V. If zero or very low, CP304 has also failed — the
   DSR-1 module's MT3608 boost converter replaces this function.

The DSR-1 module addresses the electronic failure. Mechanical service addresses the
wear. A properly serviced machine with the DSR-1 module installed will perform at or
beyond its original specification, and will be immune to the failure mode that caused
the problem in the first place.

---

## See Also

- [Original Servo Circuit](original-servo-circuit.md) — what the CX20084 did
- [Digital PLL Servo](digital-pll-servo.md) — what the DSR-1 does instead
- [Installation Guide](../installation/) — step-by-step installation procedure
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — complete specification
