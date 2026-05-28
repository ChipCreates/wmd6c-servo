# Speed Calibration Procedure

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/04-speed-calibration-procedure.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Procedure for calibrating DSR-1 capstan speed against a known tape-speed reference and saving verified settings.

---

## 1. Purpose

This document defines the speed calibration procedure for DSR-1.

DSR-1 controls capstan speed by measuring FG period and adjusting motor drive. Calibration establishes the target period and related trim settings that produce correct tape speed on a real WM-D6C / TC-D6C.

The procedure must tie DSR-1's digital target to a physical tape-speed reference. A stored integer target is not enough. The calibrated target must be proven against the actual audio output of a known test tape or equivalent measurement setup.

This document answers:

- What equipment is required to calibrate speed?
- How is the correct FG target determined?
- How do RV601/RV602/RV603/S601 interact with firmware calibration?
- How are settings saved?
- How is calibration verified after power cycle?
- How is calibration checked across power modes and USB connection states?

---

## 2. Calibration Philosophy

DSR-1 must preserve the WM-D6C as a precision transport.

Calibration should follow the spirit of the Sony service procedure:

- use controlled power,
- use known measurement conditions,
- use a calibrated tape-speed reference,
- adjust carefully,
- record raw values,
- verify after adjustment,
- and do not rely on subjective listening.

A calibration is valid only when the measurement setup is documented.

---

## 3. Prerequisites

Before speed calibration:

```text
docs/verification/00-rev-a-bringup-checklist.md
docs/verification/01-wmd6c-preinstall-measurements.md
docs/verification/02-fg901-waveform-capture.md
docs/verification/03-motor-drive-characterization.md
docs/hardware/timebase-decision.md
```

Must be complete or sufficiently resolved:

| Requirement | Status |
|---|---|
| FG input validated | Required |
| Motor-control output validated | Required |
| Servo loop stable on real transport | Required |
| Timebase strategy known | Required |
| USB service/telemetry working | Required |
| Power rails stable | Required |
| Speed-control inputs characterized | Required |
| Safe output clamps configured | Required |

Do not perform calibration on an unstable or runaway-prone servo.

---

## 4. Required Equipment

| Equipment | Purpose |
|---|---|
| Calibrated test tape | Physical tape-speed reference |
| Audio frequency counter / analyzer | Measure playback tone |
| Oscilloscope or logic capture | Observe FG if needed |
| USB host / terminal | DSR-1 telemetry and tuning |
| Regulated power supply | Controlled service power |
| Batteries or battery simulator | Battery-mode verification |
| Frequency counter | Optional FG verification |
| Wow/flutter meter or equivalent | Later performance validation |
| Temperature probe | Warmup/drift documentation |
| Measurement log | Raw values and settings |

Recommended test tape:

- known frequency test tape,
- documented calibration status,
- known speed/tone accuracy.

If using an unverified tape, mark calibration as provisional.

---

## 5. Measurement Log

Recommended path:

```text
docs/measurements/wm-d6c-serial-72795-speed-calibration-YYYY-MM-DD.md
```

Minimum metadata:

| Field | Required |
|---|---:|
| Date | Yes |
| Operator | Yes |
| Unit serial number | Yes |
| DSR-1 hardware revision | Yes |
| Firmware commit/build | Yes |
| Timebase configuration | Yes |
| Power source | Yes |
| Test tape identity | Yes |
| Measurement instrument | Yes |
| USB connection state | Yes |
| Temperature / warmup time | Yes |
| Raw frequency readings | Yes |
| FG telemetry | Yes |
| Final saved settings | Yes |

---

## 6. Calibration States

Firmware should expose or conceptually support these states:

| State | Meaning |
|---|---|
| `UNCALIBRATED` | No valid stored target |
| `CALIBRATION_MODE` | Servo running with live tuning allowed |
| `TARGET_ADJUSTING` | Target period being changed |
| `LOCKED` | Servo stable near target |
| `SAVE_PENDING` | Settings changed but not saved |
| `SAVED` | Settings written and validated |
| `FAULT` | Calibration invalid or unsafe |

Exact names may vary, but the behavior should exist.

---

## 7. Initial Setup

### 7.1 Mechanical / Transport Setup

Before electronic calibration:

- confirm belts and mechanical service are acceptable,
- confirm tape path is clean,
- confirm pinch roller/capstan condition,
- confirm the unit can play without mechanical binding,
- confirm batteries or regulated power are stable,
- allow brief warmup if appropriate.

DSR-1 cannot calibrate around a mechanically unhealthy transport indefinitely.

### 7.2 Electrical Setup

- Connect regulated power or documented battery source.
- Connect audio output to frequency counter/analyzer.
- Connect USB service terminal.
- Start DSR-1 telemetry.
- Confirm firmware reports correct board/config/timebase.
- Confirm no fault state is active.
- Confirm Speed Tune switch state.
- Confirm RV readings are sane.
- Confirm servo locks without output saturation.

---

## 8. Baseline Readings

Before adjustment, record:

| Reading | Value |
|---|---:|
| Audio output frequency | Pending |
| Expected test tape frequency | Pending |
| FG period | Pending |
| FG frequency | Pending |
| Target period | Pending |
| Servo error | Pending |
| Output command | Pending |
| Integral value | Pending |
| RV601 ADC | Pending |
| RV602 ADC | Pending |
| RV603 ADC | Pending |
| S601 state | Pending |
| Power rail voltage | Pending |
| USB state | Pending |

Do not adjust until baseline is logged.

---

## 9. Target Period Adjustment

### 9.1 Direction Check

Before large changes:

- adjust target period by a small amount,
- observe audio output frequency,
- confirm whether frequency increases or decreases,
- confirm firmware sign convention.

Record:

| Adjustment | Audio frequency changed how? | Correct sign? |
|---|---|---|
| Target + small step | Pending | Pending |
| Target - small step | Pending | Pending |

Abort if correction sign is wrong.

### 9.2 Coarse Adjustment

Adjust target until playback frequency is near expected value.

Use small enough steps to avoid overshoot.

Record several points:

| Target period | FG frequency | Audio frequency | Error from expected | Notes |
|---:|---:|---:|---:|---|
| Pending | Pending | Pending | Pending | |
| Pending | Pending | Pending | Pending | |
| Pending | Pending | Pending | Pending | |

### 9.3 Fine Adjustment

Fine adjust until measured audio output is within target tolerance.

Final values:

| Field | Value |
|---|---:|
| Final target period | Pending |
| Final FG frequency | Pending |
| Final audio frequency | Pending |
| Error from expected | Pending |
| Output command at lock | Pending |
| Integral at lock | Pending |
| Power source | Pending |
| USB state | Pending |
| Temperature | Pending |

---

## 10. RV601 / RV602 / RV603 Handling

### 10.1 RV601 Base Speed

RV601 should preserve factory-style base speed adjustment where practical.

Calibration options:

| Option | Description |
|---|---|
| Mechanical/analog centered | Physically set RV601 to service nominal, firmware target calibrated around it |
| Firmware base target | Use firmware target as primary, RV601 as trim input |
| Hybrid | Use firmware target plus measured RV601 offset |

Record selected strategy.

### 10.2 RV602 Speed Tune

If S601 enables Speed Tune:

- center RV602 or set to documented neutral before base calibration,
- measure min/max effect after base calibration,
- ensure Speed Tune cannot drive target outside safe range,
- verify S601 off returns to calibrated base speed.

### 10.3 RV603 Range

RV603 should be measured and bounded.

Record:

| RV603 state | RV602 min effect | RV602 max effect | Notes |
|---|---:|---:|---|
| Minimum | Pending | Pending | |
| Nominal | Pending | Pending | |
| Maximum | Pending | Pending | |

Do not allow firmware scaling to exceed safe motor or speed limits.

---

## 11. Save and Restore

Once calibrated:

1. Save settings using the firmware command.
2. Confirm firmware reports successful save.
3. Stop playback.
4. Power down.
5. Wait at least 10 seconds.
6. Power up.
7. Confirm settings load correctly.
8. Replay test tape.
9. Confirm audio frequency remains correct.

Record:

| Step | Result |
|---|---|
| Save command accepted | Pending |
| CRC/checksum valid | Pending |
| Power-cycle load valid | Pending |
| Target period restored | Pending |
| Audio frequency after restore | Pending |
| Settings corruption warning absent | Pending |

If settings fail to restore, calibration is not complete.

---

## 12. Power Mode Verification

Verify speed under supported power modes.

| Power mode | Audio frequency | FG period | Notes |
|---|---:|---:|---|
| Regulated external DC | Pending | Pending | |
| Battery | Pending | Pending | |
| USB-C power, if supported | Pending | Pending | |
| USB connected, not powering transport | Pending | Pending | |

Questions:

- Does speed shift between power modes?
- Does USB connection affect speed?
- Does PD negotiation affect speed?
- Does low battery affect speed?
- Does rail compensation need firmware support?

---

## 13. USB Service During Calibration

USB telemetry and tuning are part of DSR-1, but they must not disturb speed.

Test during calibrated playback:

| Action | Required result |
|---|---|
| Connect USB | No measurable speed jump |
| Start telemetry | No measurable speed jump |
| Stop telemetry | No measurable speed jump |
| Send status command | No measurable speed jump |
| Save settings | No unacceptable speed disturbance |
| Disconnect USB | No measurable speed jump |
| Host suspend/resume | Safe behavior |

Record any measurable disturbance.

---

## 14. Warmup / Drift Check

After final calibration:

| Time | Audio frequency | FG period | Output command | Temperature | Notes |
|---:|---:|---:|---:|---:|---|
| 0 min | Pending | Pending | Pending | Pending | |
| 5 min | Pending | Pending | Pending | Pending | |
| 10 min | Pending | Pending | Pending | Pending | |
| 20 min | Pending | Pending | Pending | Pending | |
| 30 min | Pending | Pending | Pending | Pending | |

This is especially important if using an internal oscillator or calibration-based timebase.

---

## 15. Fault and Bounds Checks

After calibration, verify firmware refuses unsafe values.

Tests:

| Test | Required result |
|---|---|
| Target too high | Rejected or clamped |
| Target too low | Rejected or clamped |
| Kp too high | Rejected or clamped |
| Ki too high | Rejected or clamped |
| Invalid save | Rejected |
| Corrupt settings simulation | Defaults/fault safe |
| RV open/invalid | Safe target clamp |
| FG missing | Output disabled or safe |

Do not allow calibration features to become a motor runaway path.

---

## 16. Calibration Acceptance Criteria

Calibration is accepted when:

| Requirement | Status |
|---|---|
| Test tape identified | Pending |
| Measurement instrument identified | Pending |
| Power source documented | Pending |
| Timebase configuration documented | Pending |
| Baseline values recorded | Pending |
| Target period adjusted correctly | Pending |
| Final audio frequency within target | Pending |
| Settings saved successfully | Pending |
| Settings restored after power cycle | Pending |
| Speed verified after restore | Pending |
| Power mode comparison completed | Pending |
| USB connection disturbance checked | Pending |
| Warmup/drift check completed | Pending |
| Calibration log committed | Pending |

---

## 17. Output of This Procedure

This procedure should produce:

1. Calibration log.
2. Final target period.
3. Final gain starting values.
4. Saved settings record.
5. Power-mode comparison.
6. USB disturbance check.
7. Warmup/drift data.
8. Open questions for timebase/gain refinement.

---

## 18. Final Rule

A DSR-1 calibration is not valid because the servo reports lock.

It is valid only when the tape plays at the correct measured speed and the result survives save, power cycle, USB connection, and supported power modes.
