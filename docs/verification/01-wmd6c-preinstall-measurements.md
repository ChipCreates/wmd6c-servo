# WM-D6C Preinstall Measurements

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/01-wmd6c-preinstall-measurements.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Required documentation and measurements before installing DSR-1 in a Sony WM-D6C / TC-D6C.

---

## 1. Purpose

This document defines the measurements required before DSR-1 is connected to a Sony WM-D6C / TC-D6C.

The goal is to characterize the target machine while it is still as close to original as possible. These measurements establish the safe electrical boundary for DSR-1 and prevent the project from relying on assumptions.

This file must be completed before:

- cutting traces,
- removing IC601,
- removing or bypassing CP304,
- modifying CN301,
- installing a USB-C connector,
- connecting DSR-1 to FG901,
- connecting DSR-1 to the motor-control node,
- or applying power to a DSR-1-connected Sony board.

---

## 2. Safety Position

The target WM-D6C is treated as a rare machine.

Before any modification:

1. Photograph it.
2. Identify it.
3. Inspect it.
4. Measure it.
5. Record raw values.
6. Compare it against the service manual.
7. Only then design or connect the DSR-1 interface.

Do not trust wire colors, prior repairs, internet diagrams, or assumptions from a different board revision.

---

## 3. Measurement Log Location

Create a measurement log for each unit.

Recommended path:

```text
docs/measurements/wm-d6c-serial-72795-preinstall-YYYY-MM-DD.md
```

For other units, replace the serial number.

The log should include:

| Field | Required |
|---|---:|
| Date | Yes |
| Operator | Yes |
| Machine model | Yes |
| Serial number | Yes |
| Board revision | If known |
| Service manual used | Yes |
| Photos taken | Yes |
| Test equipment | Yes |
| Power source | Yes |
| Measurement ground point | Yes |
| Measurement conditions | Yes |
| Raw readings | Yes |
| Interpretation | Yes |
| Open questions | Yes |

---

## 4. Test Unit Identification

### 4.1 Exterior Identification

Record:

| Field | Value |
|---|---|
| Model | Sony WM-D6C |
| Serial number | 72795 |
| Serial-number evidence | Battery-compartment label photo |
| Region/suffix markings | Pending |
| Cosmetic condition | Pending |
| DC jack condition | Pending |
| Battery compartment condition | Pending |
| Prior service evidence | Pending |

### 4.2 Required Exterior Photos

Take clear photos of:

- front face,
- rear face,
- side with controls,
- side with jacks,
- battery compartment,
- serial label,
- CN301 / external DC jack,
- all visible model/suffix markings.

---

## 5. Internal Inspection Photos

Before measuring or removing parts, photograph the internal board.

Required photo set:

| Photo | Purpose |
|---|---|
| Full board overview | Board revision and layout |
| IC601 / CX20084 area | Servo reference |
| IC701 / X701 area | Original reference/timebase |
| CP304 area | Power-support reference |
| CN301 area | External DC input path |
| M901 / motor wiring | Motor and FG reference |
| FG901 area, if visible | FG source |
| Q601 and motor-drive network | Motor-control reference |
| RV601 | Base speed adjustment |
| RV602 | User Speed Tune |
| RV603 | Speed Tune range |
| S601 | Speed Tune switch |
| Battery terminals and rails | Power reference |
| Ground/chassis points | Measurement reference |
| Any prior repair | Compatibility warning |

Photo rules:

- Use good lighting.
- Use macro mode or magnification.
- Take multiple angles if labels are hidden.
- Do not remove parts before photographing them.
- Keep original full-resolution files.

---

## 6. Board / Component Presence Check

Record whether each reference is present and appears original.

| Reference | Present? | Appears original? | Notes |
|---|---:|---:|---|
| IC601 / CX20084 | Pending | Pending | |
| IC701 | Pending | Pending | |
| X701 | Pending | Pending | |
| CP304 | Pending | Pending | |
| CN301 | Pending | Pending | |
| M901 | Pending | Pending | |
| FG901 | Pending | Pending | |
| Q601 | Pending | Pending | |
| Q603 | Pending | Pending | |
| Q604 | Pending | Pending | |
| Q605 | Pending | Pending | |
| RV601 | Pending | Pending | |
| RV602 | Pending | Pending | |
| RV603 | Pending | Pending | |
| S601 | Pending | Pending | |

Document corrosion, lifted pads, replaced parts, missing shields, jumper wires, bodge wires, and cracked solder joints.

---

## 7. Power Source Conditions

Measurements should be repeated under defined power conditions where practical.

### 7.1 Required Power Conditions

| Condition | Required? | Notes |
|---|---:|---|
| Batteries installed | Yes, if safe |
| Regulated external DC supply | Yes |
| Original adapter, if available | Optional |
| USB connected | Only after DSR-1 exists; not for original unit unless applicable |
| Motor running / Play | Yes, for relevant rail tests |
| Stop / idle | Yes |

### 7.2 Regulated Supply Setup

When using external DC:

- use current limiting,
- start at conservative current limit,
- observe polarity,
- document voltage,
- document current limit,
- document actual current draw.

Do not perform wrong-polarity testing on the original unmodified Sony machine unless specifically prepared to risk damage. Fault testing belongs on DSR-1/protection hardware first.

---

## 8. Ground Reference

Before measuring signals, choose and document the ground point.

Record:

| Field | Value |
|---|---|
| Ground point used | Pending |
| Physical location | Pending |
| Reason selected | Pending |
| Probe ground length | Pending |
| Alternate ground points checked | Pending |

Required checks:

- continuity between selected ground and battery negative,
- continuity to chassis where applicable,
- ground noise during Play if possible,
- difference between USB host ground and Sony ground once DSR-1 USB is involved.

Ground choice matters for FG, ADC, USB, and motor measurements.

---

## 9. Power-Path Measurements

### 9.1 CN301 / External DC Input

Measure and record:

| Measurement | Stop | Play | Notes |
|---|---:|---:|---|
| CN301 center contact voltage | Pending | Pending | |
| CN301 sleeve/contact voltage | Pending | Pending | |
| Polarity relative to ground | Pending | Pending | |
| External supply current | Pending | Pending | |
| Voltage sag at motor startup | Pending | Pending | |
| Noise/ripple at input | Pending | Pending | |

Questions to answer:

- Is CN301 original?
- Is polarity as expected?
- Is the jack mechanically reliable?
- Has it been rewired?
- Does external power bypass or interact with battery terminals?
- Does DSR-1 need to retain, replace, or isolate this path?

### 9.2 Battery Rail

Measure:

| Measurement | Stop | Startup | Play | Notes |
|---|---:|---:|---:|---|
| Battery terminal voltage | Pending | Pending | Pending | |
| Rail voltage at board | Pending | Pending | Pending | |
| Current draw | Pending | Pending | Pending | |
| Ripple/noise | Pending | Pending | Pending | |

Questions:

- Does DSR-1 need to support battery operation?
- What is the lowest expected rail voltage?
- Is there a surge during motor startup?
- Does the rail remain stable during Play?

### 9.3 CP304 / Power-Support Circuit

Measure if safely accessible:

| Measurement | Stop | Startup | Play | Notes |
|---|---:|---:|---:|---|
| CP304 input voltage | Pending | Pending | Pending | |
| CP304 output voltage | Pending | Pending | Pending | |
| Ripple/noise | Pending | Pending | Pending | |
| Current estimate | Pending | Pending | Pending | |
| Temperature after operation | Pending | Pending | Pending | |

Questions:

- Is CP304 functional?
- Does DSR-1 replace CP304 or only replace part of its role?
- What rail does CP304 provide?
- What load does that rail support?
- Does CP304 behavior differ between battery and external power?

---

## 10. Servo / FG Measurements

### 10.1 FG901 Waveform

Measure at the FG signal point before connecting DSR-1.

Record:

| Measurement | Value |
|---|---:|
| Probe point | Pending |
| Ground point | Pending |
| Tape state | Pending |
| Power source | Pending |
| FG waveform high voltage | Pending |
| FG waveform low voltage | Pending |
| DC offset | Pending |
| Peak-to-peak voltage | Pending |
| Frequency at nominal speed | Pending |
| Period at nominal speed | Pending |
| Edge rise/fall behavior | Pending |
| Noise/ringing | Pending |
| Behavior during startup | Pending |
| Behavior during stop | Pending |

Required captures:

- Stop / no FG.
- Motor startup.
- Steady Play.
- Speed Tune off.
- Speed Tune on, if safe.
- Different power modes, if practical.

Questions:

- Is FG safe for direct STM32 input?
- Does it require division, clamping, buffering, comparator, or Schmitt conditioning?
- Is the waveform clean enough for timer capture?
- What is the true target period at correct speed?

### 10.2 Audio Speed Reference

If a calibrated test tape is available:

| Measurement | Value |
|---|---:|
| Test tape type | Pending |
| Expected tone frequency | Pending |
| Measured output frequency | Pending |
| Power source | Pending |
| Speed Tune state | Pending |
| RV601 position | Pending |
| FG frequency at same time | Pending |

This ties FG target to actual tape speed.

---

## 11. Motor-Control Measurements

### 11.1 Q601 / Motor-Control Node

Before connecting DSR-1 output, measure the original control behavior.

Record:

| Measurement | Stop | Startup | Play | Notes |
|---|---:|---:|---:|---|
| Control node voltage | Pending | Pending | Pending | |
| Motor terminal voltage | Pending | Pending | Pending | |
| Motor current estimate | Pending | Pending | Pending | |
| Response to load/tape | Pending | Pending | Pending | |
| Output ripple/noise | Pending | Pending | Pending | |

Questions:

- What voltage range must DSR-1 reproduce?
- Is the control node above STM32 DAC range?
- Does the node expect voltage drive, current drive, or high impedance?
- What is the safe inactive state?
- What happens during Stop?
- What happens during motor startup?

### 11.2 Motor Startup Current

If safely measurable:

| Measurement | Value |
|---|---:|
| Peak startup current | Pending |
| Steady play current | Pending |
| Power source | Pending |
| Tape loaded? | Pending |
| Duration of surge | Pending |

This determines protection, regulators, PD current requirements, and safe output staging.

---

## 12. Speed-Control Network Measurements

### 12.1 RV601 — Base Speed Adjustment

Measure:

| Measurement | Value |
|---|---:|
| Wiper min voltage | Pending |
| Wiper max voltage | Pending |
| Wiper nominal voltage | Pending |
| Supply/reference side voltage | Pending |
| Ground/reference side voltage | Pending |
| Source impedance estimate | Pending |
| Noise while motor running | Pending |

Questions:

- Is RV601 safe for STM32 ADC?
- Does it need scaling?
- What firmware midpoint should be used?
- Is RV601 factory-set or accessible?

### 12.2 RV602 — User Speed Tune

Measure:

| Measurement | S601 off | S601 on | Notes |
|---|---:|---:|---|
| Wiper min voltage | Pending | Pending | |
| Wiper max voltage | Pending | Pending | |
| Center/nominal voltage | Pending | Pending | |
| Effective behavior | Pending | Pending | |

Questions:

- Does RV602 electrically disconnect when S601 is off?
- Is its range ADC-safe?
- Does it have a center detent?
- How much speed change does it command in the original system?

### 12.3 RV603 — Speed Tune Range

Measure:

| Measurement | Value |
|---|---:|
| Wiper min voltage | Pending |
| Wiper max voltage | Pending |
| Nominal position voltage | Pending |
| Interaction with RV602 | Pending |
| ADC safety | Pending |

Questions:

- Is RV603 a factory range trim?
- Does it scale RV602 directly?
- What firmware scaling range should correspond to the original behavior?

### 12.4 S601 — Speed Tune Switch

Measure:

| Measurement | Value |
|---|---:|
| Voltage when off | Pending |
| Voltage when on | Pending |
| Active polarity | Pending |
| Pull-up/pull-down behavior | Pending |
| Contact bounce observed? | Pending |
| Interaction with RV602 | Pending |

Questions:

- Should firmware read S601 directly?
- Is external debounce/filtering needed?
- Is the signal safe for GPIO input?

---

## 13. Original Timebase / Reference Measurements

Document the original quartz/reference area before removing or bypassing anything.

| Reference | Measurement / observation | Status |
|---|---|---|
| X701 | Presence and condition | Pending |
| IC701 | Presence and condition | Pending |
| Reference waveform | Frequency / amplitude if safely measurable | Pending |
| Relation to servo circuit | Schematic comparison | Pending |

Questions:

- Can DSR-1 retain or sample the original reference?
- Does the original reference still operate?
- Is it safer/easier to add an independent external reference?
- Does retaining original reference improve preservation value?

---

## 14. USB-C Mechanical Preinstall Survey

Before deciding USB-C placement, document the case and internal clearance.

Required photos/notes:

| Area | Purpose |
|---|---|
| Existing CN301 opening | Possible replacement/alternate location |
| Side panel clearance | USB-C connector/cable clearance |
| Internal cavity near CP304 | Board placement |
| Battery compartment clearance | Avoid interference |
| Shell thickness | Connector mounting |
| Nearby controls/switches | Avoid obstruction |
| Cable insertion angle | User access |

Questions:

- Does USB-C replace CN301 or coexist with it?
- Is case modification acceptable?
- Can a USB-C cable be inserted without stressing the PCB?
- Does connector placement block batteries or controls?
- Does USB-C placement expose the board to mechanical damage?
- Is a panel-mount connector or short internal harness better than PCB-mounted USB-C?

---

## 15. USB Ground / Noise Preinstall Considerations

USB cannot be fully tested until DSR-1 exists, but original ground behavior can be measured.

Before installation:

- identify chassis ground,
- identify main board signal ground,
- identify motor return path,
- identify battery negative path,
- measure continuity between them,
- observe ground noise during Play if possible.

Record:

| Measurement | Value |
|---|---:|
| Battery negative to selected ground | Pending |
| Selected ground to chassis | Pending |
| Motor return to selected ground | Pending |
| Ground noise during Play | Pending |
| Ground noise during motor startup | Pending |

These values inform USB shield and ground strategy.

---

## 16. Preinstall Acceptance Checklist

DSR-1 may not be connected until the following are complete.

| Requirement | Status |
|---|---|
| Serial number recorded | Done: 72795 |
| Exterior photos taken | Pending |
| Internal board photos taken | Pending |
| Board revision identified or marked unknown | Pending |
| IC601 area documented | Pending |
| CP304 area documented | Pending |
| CN301 area documented | Pending |
| FG901/M901 area documented | Pending |
| Q601/motor-control area documented | Pending |
| RV601/RV602/RV603 documented | Pending |
| S601 documented | Pending |
| Power rails measured | Pending |
| FG waveform measured | Pending |
| Motor-control node measured | Pending |
| Speed-control voltages measured | Pending |
| S601 logic measured | Pending |
| Original timebase/reference documented | Pending |
| USB-C mechanical survey complete | Pending |
| Ground reference selected | Pending |
| Measurement log committed | Pending |

---

## 17. Abort Conditions

Stop measurement or installation immediately if:

- the board revision does not match the assumed reference,
- prior repairs make the circuit ambiguous,
- CN301 has been rewired,
- CP304 is missing or modified,
- FG signal exceeds safe probe/interface expectations,
- motor-control voltage is outside expected range,
- rail current is abnormally high,
- power supply current limit is hit unexpectedly,
- smoke, odor, heat, or audible distress occurs,
- mechanical disassembly risks damage.

Document the condition before proceeding.

---

## 18. Output of This Procedure

Completing this procedure should produce:

1. A unit measurement log.
2. Full exterior and interior photos.
3. Confirmed board/revision notes.
4. FG waveform captures.
5. Motor-control measurements.
6. Speed-control measurements.
7. Power-path measurements.
8. USB-C mechanical feasibility notes.
9. Grounding notes.
10. A list of safe DSR-1 connection points.
11. A list of open questions for schematic revision.

Only after those outputs exist should DSR-1 installation planning continue.

---

## 19. Final Rule

The Sony unit is the reference artifact. Do not modify it to fit the theory.

Modify the DSR-1 design to fit the measured machine.
