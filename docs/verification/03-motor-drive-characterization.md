# Motor-Drive Characterization

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/03-motor-drive-characterization.md`  
**Status:** Draft / pre-Rev A
**Scope:** Procedure for measuring the Sony WM-D6C motor-control network to confirm DSR-1 output stage sizing and characterize servo tuning parameters.

---

## 1. Purpose

This document defines the required procedure for characterizing the WM-D6C capstan motor-drive interface before connecting DSR-1 to the Sony motor-control network.

The DSR-1 servo loop can only control tape speed safely if its output stage matches the electrical behavior expected by the original Sony circuit.

For **Ver. 1.0 CX20084 boards**, the output stage is already committed:
PWM + RC filter + NPN level-shift (TIM3_CH1 PA6 → Q_LS MMBT3904 → Q601 base).
Q601 exact part/package and base operating range on the C11-494-12 SMD board remain
pending physical confirmation. Direct DAC drive is not used.

This procedure now answers:

- What is Q601's actual base voltage range during playback? (Needed for R9 sizing)
- What is the motor-control node impedance?
- What happens during Stop, Play startup, steady Play, and Speed Tune movement?
- How much current does the motor path require? (Needed for power protection sizing)
- What is the servo loop sign? (Confirm firmware PI convention)
- What clamp values are appropriate for the firmware output limits (`DAC_MIN` and
  `DAC_MAX`, retained names for PWM compatibility)?

---

## 2. Safety Position

The motor-control output is one of the highest-risk DSR-1 interfaces.

An incorrect output stage can:

- run the capstan motor at full speed,
- stall the motor,
- overheat the motor drive path,
- damage Q601 or surrounding circuitry,
- damage the STM32,
- corrupt speed calibration,
- or mechanically stress the tape transport.

Do not connect DSR-1 output to the Sony motor-control node until the original node has been measured and a dummy-load output test has passed.

---

## 3. Prerequisites

Complete or review these first:

```text
docs/hardware/wmd6c-interface-contract.md
docs/hardware/power-usb-c-architecture.md
docs/verification/00-rev-a-bringup-checklist.md
docs/verification/01-wmd6c-preinstall-measurements.md
docs/verification/02-fg901-waveform-capture.md
```

Before beginning:

- identify unit serial number,
- photograph the motor-drive area,
- identify Q601 and associated drive components,
- select a ground point,
- define safe power source,
- and confirm the machine can be powered safely.

---

## 4. Required Equipment

| Equipment | Purpose |
|---|---|
| Oscilloscope | Motor-control node and rail waveform |
| DMM | DC voltage/current checks |
| Current-limited bench supply | Safe external power |
| Current probe or series measurement setup | Motor current estimate |
| Test tape | Real transport load |
| Audio frequency counter / analyzer | Speed reference |
| Thermal camera or temperature probe | Detect overheating |
| Camera | Probe-point documentation |
| Optional differential probe | Safer motor/drive measurements |
| Optional electronic load | DSR-1 output-stage bench simulation |

---

## 5. Measurement Log

Recommended path:

```text
docs/measurements/wm-d6c-serial-72795-motor-drive-YYYY-MM-DD.md
```

Minimum metadata:

| Field | Required |
|---|---:|
| Date | Yes |
| Operator | Yes |
| Unit serial number | Yes |
| Board revision | If known |
| Probe point | Yes |
| Ground point | Yes |
| Power source | Yes |
| Tape state | Yes |
| Speed Tune state | If relevant |
| Instrument settings | Yes |
| Raw values | Yes |
| Screenshots | Yes |
| Interpretation | Yes |

---

## 6. Node Identification

Identify the motor-control node before measuring.

Potential reference area:

| Sony reference | Purpose |
|---|---|
| Q601 | Primary motor-control transistor area |
| Q603–Q605 | Supporting motor-control path, pending schematic confirmation |
| M901 | Capstan motor |
| IC601 / CX20084 area | Original servo output context |

Document:

- exact probe point,
- exact ground point,
- surrounding components,
- whether node is before/after a resistor/transistor,
- whether node connects directly to a transistor base/gate,
- whether node remains connected to IC601 during measurement,
- and whether any prior repair is present.

Take annotated photos.

---

## 7. Required Measurements

### 7.1 Stop / Inactive State

Measure the motor-control node while the transport is stopped.

| Measurement | Value |
|---|---:|
| Control node DC voltage | Pending |
| Motor terminal voltage | Pending |
| Motor current | Pending |
| Rail voltage | Pending |
| Noise/ripple | Pending |
| Node impedance estimate | Pending |

Questions:

- What is the safe inactive state?
- Does the node float?
- Is it pulled up or down?
- Does the original circuit actively brake or simply remove drive?
- What state should DSR-1 present during reset?

### 7.2 Play Startup

Capture startup transient.

| Measurement | Value |
|---|---:|
| Control node initial voltage | Pending |
| Control node peak voltage | Pending |
| Control node settling voltage | Pending |
| Startup duration | Pending |
| Motor current peak | Pending |
| Rail sag | Pending |
| FG begins after | Pending |

Questions:

- Does startup demand a large correction?
- Does the node exceed the STM32's safe direct-drive range?
- Is a soft-start needed?
- Does the motor current surge affect DSR-1 power design?
- Does DSR-1 need to delay servo lock until FG is stable?

### 7.3 Steady Play

Measure steady-state behavior.

| Measurement | Value |
|---|---:|
| Control node DC voltage | Pending |
| Control node ripple/noise | Pending |
| Motor terminal voltage | Pending |
| Motor current | Pending |
| Rail voltage | Pending |
| FG frequency | Pending |
| Audio test tone frequency | If available |

Questions:

- What is the normal operating voltage?
- How much headroom exists above and below normal?
- Is the control signal quiet or noisy?
- Is the loop likely voltage-driven or current-driven?
- Is output filtering needed?

### 7.4 Speed Correction Behavior

If safe, observe motor-control node while speed is adjusted.

Conditions:

| Condition | Control voltage | FG frequency | Audio frequency | Notes |
|---|---:|---:|---:|---|
| Normal speed | Pending | Pending | Pending | |
| Speed Tune min | Pending | Pending | Pending | |
| Speed Tune max | Pending | Pending | Pending | |
| Slight mechanical load, if safe | Pending | Pending | Pending | |

Questions:

- Which direction increases motor speed?
- What voltage change corresponds to speed change?
- How sensitive is the control node?
- Does the original loop saturate?

### 7.5 Power Mode Comparison

Measure under:

| Power mode | Required? | Notes |
|---|---:|---|
| Battery | Yes, if safe |
| Regulated external DC | Yes |
| USB-C power | Later, after DSR-1 exists |
| Low-voltage condition | Optional / caution |

Questions:

- Does motor-control voltage depend strongly on supply mode?
- Does rail sag change control behavior?
- Does DSR-1 need rail compensation?

---

## 8. Current Measurement

Motor current matters for power protection and PD sizing.

Recommended measurements:

| Measurement | Value |
|---|---:|
| Stop current | Pending |
| Startup peak current | Pending |
| Steady Play current | Pending |
| Current during load disturbance | Pending |
| Current during Speed Tune change | Pending |
| Current during low-voltage operation | Pending |

Notes:

- Use current-limited supply.
- Avoid inserting too much series resistance.
- Document measurement method.
- Capture startup with scope if possible.

---

## 9. Output Stage — Confirmed Design

The motor output stage for Ver. 1.0 CX20084 boards is committed:

**TIM3_CH1 PWM (PA6) → R7/C8 RC filter → Q_LS (MMBT3904 NPN) → R9 pullup to B+1 → Q601 base**

This measurement procedure does not choose the output stage — that decision is made.
The measurements in this procedure are used to:

1. **Confirm R9 sizing.** R9 (100kΩ pullup to B+1, currently planned) sets the
   Q601 base voltage when Q_LS is off. Verify the measured inactive-state base
   voltage matches the expected safe-off condition (base at or near B+1, Q601
   firmly off). Adjust R9 if needed.

2. **Confirm output clamps.** The `DAC_MIN` and `DAC_MAX` names in `config.h` are
   retained for firmware compatibility, but they limit PWM duty. The clamp values must keep
   the motor within safe operating range. Measure the base voltage corresponding
   to maximum safe drive and minimum safe drive, then back-calculate the required
   PWM clamp values.

3. **Confirm PI sign convention.** Verify that increasing PWM duty cycle increases
   motor speed (expected: higher duty → Q_LS conducts more → Q601 base pulled
   lower → more collector current → faster motor).

---

## 10. Safe-State Requirements

The selected output stage must be safe during:

| Condition | Required behavior |
|---|---|
| DSR-1 unpowered | No backfeed / no motor drive |
| MCU reset | Motor-control node safe |
| BOOT0 / DFU | Motor-control node safe |
| Firmware crash | Motor-control node safe |
| SWD halt | Motor-control node safe |
| USB attach | No output glitch |
| PD negotiation | No output glitch |
| Brownout | Motor-control node safe |
| Flash write | Output held safe or controlled |

A passive default state is preferred over firmware-only safety.

---

## 11. Dummy-Load Validation Before Sony Connection

Before connecting to Sony:

1. Build or simulate equivalent load based on measured motor-control node.
2. Drive output through full expected range.
3. Confirm voltage/current limits.
4. Confirm reset state.
5. Confirm output with USB connected/disconnected.
6. Confirm output during firmware update mode.
7. Confirm output during power faults.

Do not connect to the Sony motor-control node until dummy-load behavior is documented.

---

## 12. Firmware Consequences

The measurements determine firmware requirements.

| Measured behavior | Firmware implication |
|---|---|
| Output sign positive | Confirm PI sign |
| Output sign negative | Invert correction direction |
| Narrow control range | Tight output clamps |
| Sensitive node | Lower gains / filtering |
| Startup transient large | Add acquisition/soft-start |
| Rail-dependent behavior | Add rail sensing or compensation |
| No FG at startup | Hold output until valid FG |
| Motor runaway risk | Add watchdog/fault state |

Firmware telemetry should report:

- output command,
- output clamp state,
- measured FG period,
- target period,
- error,
- integral,
- lock state,
- rail status,
- USB state,
- and fault state.

---

## 13. Acceptance Criteria

Motor-drive characterization is accepted when:

| Requirement | Status |
|---|---|
| Probe point photographed | Pending |
| Ground point photographed | Pending |
| Stop/inactive voltage measured | Pending |
| Startup transient captured | Pending |
| Steady Play voltage measured | Pending |
| Motor current estimated/measured | Pending |
| Rail sag measured | Pending |
| Speed correction direction identified | Pending |
| Output voltage range defined | Pending |
| Safe inactive state defined | Pending |
| R9 sizing confirmed from measured base voltage | Pending |
| PWM output clamp values (`DAC_MIN` / `DAC_MAX` names) derived | Pending |
| PI sign convention confirmed | Pending |
| Dummy-load output test plan written | Pending |
| Firmware implications documented | Pending |

---

## 14. Output of This Procedure

This procedure should produce:

1. Measurement log.
2. Probe-point photos.
3. Oscilloscope captures.
4. Motor-control voltage table.
5. Motor-current estimates.
6. Output-stage recommendation.
7. Safe-state requirements.
8. Firmware control-sign and clamp requirements.
9. Open questions for Rev A schematic.

---

## 15. Final Rule

Do not let DSR-1 drive the WM-D6C motor-control node until the original node has been measured and the DSR-1 output stage has passed dummy-load testing.

The motor should be the last thing trusted, not the first.
