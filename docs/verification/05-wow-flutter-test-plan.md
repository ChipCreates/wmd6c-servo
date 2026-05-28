# Wow and Flutter Test Plan

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/05-wow-flutter-test-plan.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Procedure for measuring speed stability, wow/flutter behavior, drift, and USB/power-related disturbance after DSR-1 installation.

---

## 1. Purpose

This document defines the wow/flutter and speed-stability test plan for DSR-1.

DSR-1 must not be considered successful merely because the motor runs or the servo reports lock. The Sony WM-D6C is a precision cassette transport. DSR-1 must demonstrate that it can control the capstan without introducing unacceptable speed error, hunting, drift, oscillation, power-related instability, or USB-induced disturbance.

This plan answers:

- Does DSR-1 maintain correct tape speed?
- Does it introduce measurable wow or flutter?
- Does speed drift over time?
- Does USB telemetry affect speed?
- Does USB-C power or PD negotiation affect speed?
- Does power source affect servo behavior?
- Does Speed Tune behave safely and predictably?
- Is performance good enough to justify claims in the README?

---

## 2. Test Philosophy

The test must measure the actual transport, not just firmware telemetry.

DSR-1 telemetry is useful, but it is not the final proof. The final proof is measured playback behavior from a known tape reference.

A valid test should document:

1. the machine,
2. the DSR-1 hardware revision,
3. the firmware build,
4. the timebase configuration,
5. the power source,
6. the USB state,
7. the test tape or measurement source,
8. the measurement instrument,
9. raw readings,
10. and the interpretation.

Do not claim “better than original,” “quartz-lock equivalent,” or “WM-D6C compatible” without measured evidence.

---

## 3. Prerequisites

Before running this plan, complete or review:

```text
docs/verification/00-rev-a-bringup-checklist.md
docs/verification/01-wmd6c-preinstall-measurements.md
docs/verification/02-fg901-waveform-capture.md
docs/verification/03-motor-drive-characterization.md
docs/verification/04-speed-calibration-procedure.md
docs/hardware/timebase-decision.md
docs/hardware/power-usb-c-architecture.md
```

Required before wow/flutter testing:

| Requirement | Status |
|---|---|
| DSR-1 installed without unsafe behavior | Required |
| Servo locks during playback | Required |
| Speed calibration completed | Required |
| Firmware settings saved and restored | Required |
| USB telemetry functional | Required |
| Power rails stable | Required |
| Motor output not saturating during normal play | Required |
| No mechanical distress from transport | Required |

A mechanically unhealthy tape transport invalidates the test. DSR-1 cannot compensate for all mechanical defects.

---

## 4. Required Equipment

| Equipment | Purpose |
|---|---|
| Calibrated test tape | Tape-speed reference |
| Wow/flutter meter | Primary wow/flutter measurement |
| Audio analyzer | Alternate speed/wow/flutter measurement |
| Frequency counter | Playback tone frequency |
| Oscilloscope | FG and output correlation |
| USB host/logger | DSR-1 telemetry logging |
| Regulated DC supply | Controlled external power |
| Batteries or battery simulator | Battery-mode testing |
| USB-C power meter/analyzer | USB-C power testing |
| PD analyzer, if applicable | PD contract/renegotiation testing |
| Temperature probe | Drift and warmup testing |
| Measurement log | Raw data capture |

If a dedicated wow/flutter meter is unavailable, use the best available audio analyzer method and clearly mark the result as provisional.

---

## 5. Measurement Log

Recommended path:

```text
docs/measurements/wm-d6c-serial-72795-wow-flutter-YYYY-MM-DD.md
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
| Power mode | Yes |
| USB state | Yes |
| Test tape identity | Yes |
| Measurement instrument | Yes |
| Measurement standard / weighting | If available |
| Ambient temperature | Yes |
| Warmup time | Yes |
| Raw readings | Yes |
| Interpretation | Yes |

---

## 6. Terms

Use terms carefully.

| Term | Meaning |
|---|---|
| Speed error | Difference between measured playback frequency and expected frequency |
| Drift | Slow change in speed over time |
| Wow | Low-frequency speed variation |
| Flutter | Higher-frequency speed variation |
| WRMS | Weighted root-mean-square measurement, if instrument supports it |
| Peak | Peak deviation measurement, if instrument supports it |
| Servo lock | Firmware state indicating FG loop is stable |
| Output saturation | Servo output at clamp limit, meaning control authority is exhausted |

If the instrument reports multiple standards or weightings, record all relevant settings.

---

## 7. Test Conditions

### 7.1 Required Operating Conditions

| Condition | Required? | Notes |
|---|---:|---|
| Regulated external DC | Yes | Service reference condition |
| Battery operation | Yes, if supported |
| USB connected, data only | Yes |
| USB disconnected | Yes |
| USB telemetry active | Yes |
| USB telemetry idle | Yes |
| USB-C power / PD | Yes, if implemented |
| Speed Tune off | Yes |
| Speed Tune on / centered | Yes, if supported |
| Warm transport | Yes |
| Cold start | Yes |

### 7.2 Optional Conditions

| Condition | Purpose |
|---|---|
| Low battery simulation | Check dropout behavior |
| Multiple tapes | Verify test repeatability |
| Different USB hosts | Detect host/ground differences |
| Different USB-C chargers | Detect PD/power differences |
| Case open vs closed | Thermal/mechanical comparison |

---

## 8. Baseline Before DSR-1 Claims

If possible, record original-unit performance before modification.

| Measurement | Original unit value | Notes |
|---|---:|---|
| Playback frequency | Pending | |
| Speed error | Pending | |
| Wow/flutter WRMS | Pending | |
| Wow/flutter peak | Pending | |
| Power mode | Pending | |
| Speed Tune state | Pending | |
| Warmup time | Pending | |

If original performance cannot be measured because the unit is failed, mark this clearly.

Do not invent baseline performance.

---

## 9. DSR-1 Measurement Sequence

### 9.1 Cold Start Speed

Procedure:

1. Let unit rest unpowered long enough to be considered cold.
2. Insert test tape.
3. Apply selected power mode.
4. Start playback.
5. Begin audio frequency measurement immediately.
6. Begin DSR-1 telemetry logging.
7. Record readings at defined time intervals.

Table:

| Time | Audio frequency | Speed error | FG period | Servo output | Temperature | Notes |
|---:|---:|---:|---:|---:|---:|---|
| 0 min | Pending | Pending | Pending | Pending | Pending | |
| 1 min | Pending | Pending | Pending | Pending | Pending | |
| 5 min | Pending | Pending | Pending | Pending | Pending | |
| 10 min | Pending | Pending | Pending | Pending | Pending | |
| 20 min | Pending | Pending | Pending | Pending | Pending | |
| 30 min | Pending | Pending | Pending | Pending | Pending | |

### 9.2 Steady-State Wow/Flutter

After warmup:

| Measurement | Value |
|---|---:|
| Test tape | Pending |
| Measurement standard / weighting | Pending |
| Wow/flutter WRMS | Pending |
| Wow/flutter peak | Pending |
| Playback frequency | Pending |
| Speed error | Pending |
| FG period mean | Pending |
| FG period variation | Pending |
| Servo output mean | Pending |
| Servo output variation | Pending |
| Power mode | Pending |
| USB state | Pending |

Take multiple readings.

| Run | WRMS | Peak | Speed error | Notes |
|---:|---:|---:|---:|---|
| 1 | Pending | Pending | Pending | |
| 2 | Pending | Pending | Pending | |
| 3 | Pending | Pending | Pending | |

### 9.3 Long-Run Drift

Run at least one extended playback test.

| Time | Audio frequency | Speed error | FG period | Output | Rail voltage | USB state | Notes |
|---:|---:|---:|---:|---:|---:|---|---|
| Start | Pending | Pending | Pending | Pending | Pending | Pending | |
| 15 min | Pending | Pending | Pending | Pending | Pending | Pending | |
| 30 min | Pending | Pending | Pending | Pending | Pending | Pending | |
| 45 min | Pending | Pending | Pending | Pending | Pending | Pending | |
| 60 min | Pending | Pending | Pending | Pending | Pending | Pending | |

A full tape-side test is preferred.

---

## 10. USB Disturbance Test

USB-C service is part of DSR-1. It must not disturb playback.

With tape running and servo locked:

| Action | Required result | Actual |
|---|---|---|
| Plug USB cable in | No audible/measurable disturbance | Pending |
| Enumerate USB device | No speed disturbance | Pending |
| Open CDC terminal | No speed disturbance | Pending |
| Start telemetry stream | No speed disturbance | Pending |
| Increase telemetry rate | No unsafe timing impact | Pending |
| Send status command | No speed disturbance | Pending |
| Save settings | No unacceptable disturbance | Pending |
| Stop telemetry | No speed disturbance | Pending |
| Disconnect USB | No speed disturbance | Pending |
| Host suspend/resume | Safe behavior | Pending |

Record speed and telemetry before, during, and after each event.

If a disturbance occurs:

- capture the audio measurement,
- capture telemetry,
- capture rail voltage,
- capture USB state,
- and mark whether the disturbance is audible, measurable, or both.

---

## 11. USB-C Power / PD Disturbance Test

If USB-C powers the transport or participates in PD:

| Event | Required result | Actual |
|---|---|---|
| Attach USB-C source before playback | Safe startup | Pending |
| Start playback under USB-C power | Stable servo | Pending |
| Cable flip | No behavior change | Pending |
| PD negotiation | No motor glitch | Pending |
| PD renegotiation | Safe behavior | Pending |
| Low-current source | Safe refusal or limited operation | Pending |
| USB-C disconnect | Controlled shutdown or safe transition | Pending |
| External power + USB-C connected | No backfeed / safe priority | Pending |

Do not perform destructive power transition tests on the Sony unit until equivalent behavior has been validated on bench hardware.

---

## 12. Speed Tune Behavior

Test Speed Tune after base calibration.

| Condition | Audio frequency | FG period | Servo output | Notes |
|---|---:|---:|---:|---|
| Speed Tune off | Pending | Pending | Pending | |
| Speed Tune on center | Pending | Pending | Pending | |
| Speed Tune minimum | Pending | Pending | Pending | |
| Speed Tune maximum | Pending | Pending | Pending | |
| Speed Tune returned off | Pending | Pending | Pending | |

Acceptance:

- Speed Tune must remain bounded.
- Returning Speed Tune off should return to calibrated base speed.
- Speed Tune changes should not cause runaway, output saturation, or persistent integral windup.

---

## 13. Power Mode Comparison

| Power mode | Speed error | WRMS | Peak | Rail voltage | Notes |
|---|---:|---:|---:|---:|---|
| Regulated external DC | Pending | Pending | Pending | Pending | |
| Battery | Pending | Pending | Pending | Pending | |
| USB-C power | Pending | Pending | Pending | Pending | |
| External DC + USB data | Pending | Pending | Pending | Pending | |
| Battery + USB data | Pending | Pending | Pending | Pending | |

If speed changes significantly by power mode, DSR-1 may need rail compensation, clock correction, grounding changes, or revised output-stage design.

---

## 14. Pass / Fail Guidance

A pass/fail threshold must eventually be defined by project goals. Until then, classify results:

| Classification | Meaning |
|---|---|
| Pass | Meets target with margin |
| Provisional pass | Works but needs repeat data |
| Marginal | Usable but needs design review |
| Fail | Unacceptable speed error, instability, or disturbance |
| Invalid test | Measurement setup or transport condition not trustworthy |

Do not lower performance expectations to match a weak result. Mark weak results honestly.

---

## 15. Acceptance Criteria

Wow/flutter validation is accepted when:

| Requirement | Status |
|---|---|
| Test tape identified | Pending |
| Measurement instrument identified | Pending |
| Speed calibration completed first | Pending |
| Cold-start speed measured | Pending |
| Steady-state wow/flutter measured | Pending |
| Long-run drift measured | Pending |
| USB connected/disconnected disturbance tested | Pending |
| USB telemetry disturbance tested | Pending |
| USB-C power/PD disturbance tested, if implemented | Pending |
| Power mode comparison completed | Pending |
| Speed Tune behavior tested | Pending |
| Raw logs committed | Pending |
| Claims updated to match results | Pending |

---

## 16. Release Claim Gate

Do not claim:

- “better speed stability than original,”
- “quartz-lock equivalent,”
- “WM-D6C compatible,”
- “audiophile-grade,”
- “professional-grade,”
- or “drop-in replacement”

until measured results support the claim.

---

## 17. Final Rule

Firmware lock is not proof of speed quality.

The tape must play at the correct measured speed, with acceptable wow/flutter, across power modes and USB states.
