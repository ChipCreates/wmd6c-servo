# FG901 Waveform Capture

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/02-fg901-waveform-capture.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Procedure for measuring and documenting the WM-D6C FG901 feedback waveform before designing or connecting the DSR-1 FG input.

---

## 1. Purpose

This document defines the required procedure for capturing and characterizing the FG901 waveform in the Sony WM-D6C / TC-D6C before connecting DSR-1 to the capstan feedback path.

The FG signal is the primary speed-feedback signal for the DSR-1 servo loop. If this signal is misunderstood, DSR-1 may measure the wrong speed, miss pulses, overcorrect, oscillate, or drive the capstan motor incorrectly.

This document answers:

- What does the FG901 signal actually look like on the target unit?
- What voltage range must the DSR-1 input survive?
- Is the waveform safe for direct STM32 input?
- Does it require protection, filtering, clamping, buffering, comparator conditioning, or Schmitt-trigger cleanup?
- What is the measured FG frequency at correct tape speed?
- How does FG behave during startup, stop, pause, Speed Tune use, and different power modes?

No Rev A FG input schematic should be finalized until this procedure has been completed on at least one physical WM-D6C.

---

## 2. Safety Position

Do not connect FG901 directly to the STM32.

Until measured, FG901 is an unknown signal from a decades-old electromechanical system. It may have:

- voltage outside the MCU supply range,
- slow or noisy edges,
- motor-induced ringing,
- DC offset,
- ground noise,
- startup transients,
- missing pulses,
- or board-revision-specific behavior.

Use an oscilloscope first. Use high-impedance probing. Use short ground leads. Do not load the circuit.

---

## 3. Prerequisites

Complete or review these documents first:

```text
docs/hardware/wmd6c-interface-contract.md
docs/hardware/wmd6c-revision-compatibility.md
docs/hardware/timebase-decision.md
docs/verification/00-rev-a-bringup-checklist.md
docs/verification/01-wmd6c-preinstall-measurements.md
```

Before beginning:

- the unit must be identified by serial number,
- board photos must be taken,
- the ground reference must be selected,
- the Sony service-manual schematic area must be reviewed,
- and the FG test point must be identified without modifying the board.

---

## 4. Required Equipment

| Equipment | Purpose |
|---|---|
| Oscilloscope | Primary waveform capture |
| 10x passive probe | Minimize circuit loading |
| Short ground spring / short ground lead | Reduce ringing and false readings |
| Frequency counter or scope frequency measurement | FG rate measurement |
| Regulated DC supply | Controlled power condition |
| Batteries or battery simulator | Battery-mode comparison |
| Calibrated test tape | Correct-speed reference |
| Audio frequency counter / analyzer | Compare tape speed with FG |
| Camera | Document probe point and setup |
| Notebook / measurement log | Record raw values |
| Optional logic analyzer | Check capture suitability after analog measurement |

Do not use a logic analyzer as the first measurement tool. A logic analyzer can hide voltage and waveform problems.

---

## 5. Measurement Log

Create a measurement log before starting.

Recommended path:

```text
docs/measurements/wm-d6c-serial-72795-fg901-waveform-YYYY-MM-DD.md
```

Minimum metadata:

| Field | Required |
|---|---:|
| Date | Yes |
| Operator | Yes |
| Unit serial number | Yes |
| Board revision | If known |
| Power source | Yes |
| Probe model / attenuation | Yes |
| Scope model | Yes |
| Ground point | Yes |
| FG probe point | Yes |
| Tape state | Yes |
| Speed Tune state | Yes |
| Raw screenshots | Yes |
| Interpretation | Yes |

---

## 6. Probe Point Identification

Before measuring, identify:

| Item | Description |
|---|---|
| FG source | `FG901` / motor feedback source |
| Probe point | Exact pad, pin, component lead, or trace point |
| Ground point | Exact board/chassis ground used |
| Nearby circuitry | Any series resistor, capacitor, transistor, or IC pin in the path |
| Board photo | Annotated photo required |

Record the probe point with a photograph before attaching the probe.

Suggested photo filenames:

```text
docs/measurements/photos/unit-72795-fg901-probe-point.jpg
docs/measurements/photos/unit-72795-fg901-ground-point.jpg
```

---

## 7. Test Conditions

Capture FG901 under multiple operating conditions.

### 7.1 Required Conditions

| Condition | Required? | Notes |
|---|---:|---|
| Stop / motor off | Yes | Establish idle voltage/noise |
| Play startup | Yes | Capture transient behavior |
| Steady Play | Yes | Primary waveform |
| Steady Play with test tape | Yes | Tie FG to speed reference |
| Speed Tune off | Yes | Normal reference |
| Speed Tune on | Yes, if safe | Determine frequency shift |
| Battery power | Yes, if safe | Compare portable operation |
| Regulated external DC | Yes | Service condition |
| USB connected | Later, after DSR-1 exists | Check disturbance only after safe install |

### 7.2 Optional Conditions

| Condition | Purpose |
|---|---|
| Low battery simulation | Check FG behavior near dropout |
| Warm unit after 30 minutes | Check drift |
| Different tape load | Check load sensitivity |
| Pause / record modes | If relevant to servo behavior |

---

## 8. Oscilloscope Setup

Initial suggested setup:

| Scope setting | Starting point |
|---|---|
| Probe | 10x |
| Coupling | DC |
| Vertical scale | Start high enough to avoid clipping |
| Timebase | Start around expected FG period, then adjust |
| Trigger | Rising edge, mid-level |
| Acquisition | Normal, then single-shot for startup |
| Bandwidth limit | Off initially, then compare with limit on |
| Measurements | Vmin, Vmax, Vpp, frequency, period, rise/fall time |

Record actual settings in the log.

---

## 9. Measurements to Capture

### 9.1 Idle / Stop

Capture the FG node while the motor is stopped.

Record:

| Measurement | Value |
|---|---:|
| DC voltage | Pending |
| Noise amplitude | Pending |
| Any periodic interference | Pending |
| Pull-up/pull-down behavior | Pending |
| Safe idle state? | Pending |

Questions:

- Is the line floating?
- Is it pulled high or low?
- Does it carry motor or digital noise even when stopped?
- Would the STM32 input see false edges?

### 9.2 Play Startup

Use single-shot capture.

Record:

| Measurement | Value |
|---|---:|
| First pulse time after motor start | Pending |
| Startup overshoot | Pending |
| Startup ringing | Pending |
| Missing/irregular pulses | Pending |
| Peak voltage during startup | Pending |
| Minimum voltage during startup | Pending |

Questions:

- Does startup produce voltage beyond steady-state range?
- Does frequency ramp smoothly?
- Does the servo need startup blanking or lock detection?
- Are false captures likely?

### 9.3 Steady Play

Record:

| Measurement | Value |
|---|---:|
| High voltage | Pending |
| Low voltage | Pending |
| Peak-to-peak voltage | Pending |
| DC offset | Pending |
| Frequency | Pending |
| Period | Pending |
| Rise time | Pending |
| Fall time | Pending |
| Duty cycle | Pending |
| Jitter / period variation | Pending |
| Noise/ringing | Pending |

Required screenshots:

- full waveform,
- zoomed edge,
- persistence or long capture showing jitter/noise,
- measurement overlay.

### 9.4 Correct-Speed Reference

With calibrated test tape:

| Measurement | Value |
|---|---:|
| Test tape frequency | Pending |
| Measured audio output frequency | Pending |
| FG frequency at same time | Pending |
| FG period at same time | Pending |
| Power source | Pending |
| Speed Tune state | Pending |
| RV601 setting | Pending |

This measurement becomes the basis for the DSR-1 target period.

### 9.5 Speed Tune Effect

If safe:

| Condition | FG frequency | Audio frequency | Notes |
|---|---:|---:|---|
| Speed Tune off | Pending | Pending | |
| Speed Tune on, center | Pending | Pending | |
| Speed Tune minimum | Pending | Pending | |
| Speed Tune maximum | Pending | Pending | |

This helps determine whether DSR-1 should replicate original Speed Tune behavior through target-period adjustment.

---

## 10. Interpreting the Waveform

### 10.1 Direct MCU Input Candidate

Direct or lightly protected STM32 input may be considered only if:

- waveform never exceeds MCU input limits,
- low level is reliably below logic-low threshold,
- high level is reliably above logic-high threshold,
- edges are fast enough,
- ringing does not cross thresholds repeatedly,
- startup transients remain safe,
- and the signal remains stable across power modes.

Even then, series resistance and clamp/protection should be considered.

### 10.2 Comparator / Schmitt Conditioning Candidate

Use comparator or Schmitt conditioning if:

- waveform amplitude is analog or marginal,
- edges are slow,
- ringing creates multiple threshold crossings,
- noise is high,
- DC offset varies,
- or logic thresholds are unreliable.

### 10.3 Protection Required

Protection is required if:

- voltage can exceed MCU supply,
- negative excursions occur,
- motor noise/ringing is significant,
- cable/harness length increases exposure,
- or the signal may be present while DSR-1 is unpowered.

---

## 11. Firmware Consequences

The measured FG behavior determines firmware requirements.

| Observed behavior | Firmware implication |
|---|---|
| Missing pulses during startup | Add startup/unlocked state |
| Noise or false edges | Add input qualification / reject impossible periods |
| Wide frequency ramp | Add acquisition mode |
| Signal absent in Stop | Define motor-off state |
| Jitter | Tune filtering and PI gains carefully |
| Very high pulse rate | Check ISR load |
| Very low pulse rate | Check timeout behavior |

Firmware telemetry should report:

- raw captured period,
- rejected captures,
- timeout state,
- lock state,
- current target,
- computed error,
- and output correction.

---

## 12. Acceptance Criteria

FG901 measurement is accepted when:

| Requirement | Status |
|---|---|
| Probe point photographed | Pending |
| Ground point photographed | Pending |
| Stop waveform captured | Pending |
| Startup waveform captured | Pending |
| Steady Play waveform captured | Pending |
| Correct-speed FG frequency measured | Pending |
| Voltage high/low measured | Pending |
| Rise/fall behavior measured | Pending |
| Noise/ringing assessed | Pending |
| Power-mode comparison completed | Pending |
| Speed Tune effect measured | Pending |
| Conditioning recommendation written | Pending |
| Firmware implications documented | Pending |

---

## 13. Output of This Procedure

This procedure should produce:

1. Measurement log.
2. Probe-point photos.
3. Oscilloscope screenshots.
4. FG voltage/frequency table.
5. Correct-speed target period candidate.
6. Recommended FG input circuit.
7. Firmware capture requirements.
8. Open questions for Rev A schematic.

---

## 14. Final Rule

Do not design the DSR-1 FG input around a guessed waveform.

Measure FG901 first. Let the waveform choose the circuit.
