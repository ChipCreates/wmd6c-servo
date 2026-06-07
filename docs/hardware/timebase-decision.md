# Timebase Decision

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/hardware/timebase-decision.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Servo timebase, USB clocking, USB-C service implications, PCB/pinout consequences, and validation requirements.

---

## 1. Purpose

This document records the timebase decision process for DSR-1.

The timebase is not a minor firmware detail. It determines the accuracy of the digital servo loop and interacts with USB data, USB-C service behavior, firmware update mode, PCB layout, pin allocation, and performance claims.

DSR-1 measures capstan speed by measuring the period of the FG signal. That measurement is only as accurate as the clock driving the timer. A target period stored in flash is numerically stable, but the real-world speed accuracy of the machine depends on the physical clock used to measure that period.

This document exists to prevent a false assumption:

> A digital target value does not drift, but the clock that measures it can.

---

## 2. Decision Summary

**Current decision state: open.**

The internal STM32 oscillator may be used for early firmware development, simulation, and basic bring-up. It must not be treated as sufficient for final WM-D6C-grade speed claims until measured evidence supports that conclusion.

The conservative Rev A position is:

> Rev A must either use a precision external/reference-derived timebase or prove through measurement that the selected internal-clock strategy meets DSR-1 speed requirements across realistic operating conditions.

---

## 3. Why the Timebase Matters

DSR-1 does not count motor speed in abstract units. It measures time between FG pulses using a microcontroller timer.

The control loop compares:

```text
measured_fg_period_ticks - target_fg_period_ticks
```

If the timer clock is off by 1%, the measured period is off by 1%. The servo can still lock tightly to its internal reference, but the transport may be locked to the wrong physical speed.

The servo loop can be stable and still be inaccurate if the timebase is inaccurate.

Timebase quality affects:

- absolute tape speed,
- long-term speed stability,
- temperature behavior,
- speed calibration repeatability,
- wow/flutter interpretation,
- USB timing,
- telemetry trustworthiness,
- firmware update reliability,
- and whether the project can honestly claim original-equivalent or improved speed performance.

---

## 4. Servo Timing Requirements

### 4.1 Servo Measurement Method

The firmware design measures FG period using timer input capture.

Conceptually:

```text
FG rising edge N     -> capture timer count A
FG rising edge N + 1 -> capture timer count B
measured_period      -> B - A
```

The period is then compared with the calibrated target period.

### 4.2 Servo Accuracy Problem

The target period may be saved perfectly in flash. That does not guarantee correct tape speed.

The true speed reference is:

```text
timer_clock_frequency / target_period_ticks
```

Therefore the timer clock must be accurate enough to support the WM-D6C speed target.

### 4.3 Required Servo Validation

Before final performance claims:

| Test | Required result |
|---|---|
| Tape speed at room temperature | Meets project target after calibration |
| Tape speed after warmup | Drift remains within acceptable limits |
| Tape speed across battery/external/USB power modes | No unacceptable shift |
| Tape speed with USB connected and disconnected | No unacceptable shift |
| Tape speed across expected temperature range | No unacceptable shift |
| Wow/flutter with DSR-1 installed | No unacceptable degradation |
| Long-duration playback | No unacceptable slow drift |

---

## 5. USB Timing Requirements

DSR-1 includes USB-C data/service access. USB timing and servo timing must be considered together.

USB-related functions include:

- USB CDC telemetry,
- live tuning,
- diagnostic commands,
- firmware update / DFU path,
- possible native USB-C PD support through STM32 UCPD,
- and possible service-only mode with the transport unpowered.

### 5.1 USB Clocking

USB 2.0 Full Speed has its own timing requirements. The selected STM32G0C1KCU6 supports USB FS, and firmware may use internal clocking and clock recovery features where appropriate.

The USB clock strategy must answer:

- What clock source is used for USB FS?
- Is HSI48 used?
- Is CRS used?
- Does USB require host SOF synchronization?
- What happens when USB is disconnected?
- Does the servo timer depend on the same clock source?
- Does USB attach/detach affect servo timing?
- Does firmware update mode use the same clock configuration as the application?

### 5.2 USB Must Not Disturb Servo Timing

USB activity must not:

- delay the servo ISR,
- change the servo timer frequency unexpectedly,
- reconfigure clocks while the transport is running,
- cause motor-control glitches,
- or introduce speed drift during telemetry streaming.

### 5.3 USB Service While Playing

If DSR-1 supports USB telemetry during playback, the timebase plan must prove:

- USB enumeration does not disturb capstan speed,
- telemetry load does not affect servo timing,
- host suspend/resume does not affect servo timing,
- USB disconnect does not affect motor output,
- and firmware update mode cannot be entered accidentally during Play.

---

## 6. Existing Sony Reference

The original WM-D6C servo system includes a quartz-based reference path. DSR-1 must decide whether to preserve, replace, ignore, or derive from that reference.

### 6.1 Relevant Sony References

| Sony reference | Function |
|---|---|
| X701 | Original quartz reference source |
| IC701 | Original divider/reference logic |
| IC601 / CX20084 | Original servo IC |
| FG901 | Motor/capstan feedback source |

The original machine's identity is closely tied to its quartz-lock speed behavior. Removing the original reference path may be valid, but only if DSR-1 replaces it with an adequately stable timing reference.

### 6.2 Open Questions

- Is X701 still present and usable on the target unit?
- Is IC701 removed, retained, or bypassed in the proposed DSR-1 installation?
- Can a divided Sony reference be safely exposed to DSR-1?
- Does retaining the Sony reference simplify or complicate installation?
- Does use of the Sony reference preserve more original design character?
- Does it consume MCU pins needed for USB-C / UCPD / ADC / PWM?

---

## 7. STM32G0C1 Clock Options

### 7.1 HSI16 + PLL

The internal HSI16 oscillator with PLL is attractive because it requires no external timing parts.

| Advantage | Concern |
|---|---|
| No external crystal or resonator | Absolute accuracy may be insufficient |
| Simple PCB layout | Temperature drift must be proven acceptable |
| Fast firmware bring-up | Calibration may be unit-specific |
| Frees pins | Final speed claims may be hard to defend |

Recommended use:

- firmware development,
- early bench tests,
- signal-chain validation,
- simulated FG testing,
- non-final servo experiments.

Not recommended as final timebase unless validated.

### 7.2 HSI48 / CRS

HSI48 and clock recovery may be useful for USB operation.

| Advantage | Concern |
|---|---|
| Supports USB-related clocking | USB clocking is not automatically a servo-grade reference |
| May synchronize during USB connection | Servo must also work without USB connected |
| Reduces external parts | Behavior across attach/detach must be tested |

Open issue:

> Do not let USB-connected behavior become the only accurate servo mode unless the product is intentionally designed that way.

### 7.3 External HSE Crystal or Oscillator

A dedicated external reference can provide a more defensible servo timebase.

| Advantage | Concern |
|---|---|
| Better absolute frequency accuracy | Adds parts and layout constraints |
| Stable independent reference | Consumes pins / space |
| Easier performance claim | Must be selected and specified carefully |
| Works without USB | Requires BOM and sourcing decision |

Candidate types:

- crystal,
- ceramic resonator,
- MEMS oscillator,
- precision clock oscillator.

A crystal or oscillator must be selected based on ppm accuracy, temperature behavior, package size, current draw, availability, and layout feasibility.

### 7.4 External Clock Input

Instead of a crystal, the MCU may accept an external clock signal.

| Advantage | Concern |
|---|---|
| Can use a precision oscillator module | May require extra supply/filtering |
| Easier layout than crystal in some cases | EMI/noise coupling must be reviewed |
| Strong accuracy if selected well | Consumes clock input pin |

### 7.5 Retained Sony-Derived Reference

DSR-1 may derive its servo timebase from the original Sony reference path.

| Advantage | Concern |
|---|---|
| Preserves original quartz-lock character | Requires careful interface to old circuitry |
| Potentially uses known Sony reference | May depend on IC701/X701 health |
| Strong preservation argument | May complicate installation |
| Could reduce calibration burden | Board revision differences may matter |

This option should be investigated before dismissing it.

---

## 8. Candidate Architectures

### Option A — Internal HSI16 + PLL Only

| Field | Value |
|---|---|
| Parts count | Lowest |
| PCB complexity | Lowest |
| Firmware complexity | Low |
| Servo accuracy risk | High until proven |
| USB interaction | Separate USB clock plan still required |
| Recommended status | Development only unless validated |

### Option B — Internal HSI16 Calibrated Per Unit

| Field | Value |
|---|---|
| Parts count | Low |
| PCB complexity | Low |
| Firmware complexity | Medium |
| Servo accuracy risk | Medium |
| Calibration burden | High |
| Recommended status | Possible, but must be measured across temperature and power modes |

This option requires a robust calibration workflow, probably tied to the Sony speed adjustment procedure and a test tape/frequency counter.

### Option C — External Crystal / Oscillator as Servo Timebase

| Field | Value |
|---|---|
| Parts count | Medium |
| PCB complexity | Medium |
| Firmware complexity | Low to medium |
| Servo accuracy risk | Low if part is well-selected |
| Calibration burden | Lower |
| Recommended status | Strong candidate for Rev A/B |

### Option D — Retain / Derive from Sony Quartz Reference

| Field | Value |
|---|---|
| Parts count | Potentially low |
| PCB complexity | Medium |
| Firmware complexity | Medium |
| Servo accuracy risk | Potentially low |
| Preservation value | High |
| Board-revision risk | Medium to high |
| Recommended status | Investigate before final schematic |

### Option E — USB-Synchronized Timebase

| Field | Value |
|---|---|
| Parts count | Low |
| PCB complexity | Low |
| Firmware complexity | Medium |
| Servo accuracy risk | Depends on USB connection |
| Standalone behavior | Problematic unless fallback exists |
| Recommended status | Not acceptable as sole servo reference unless USB is required for operation |

---

## 9. Pinout and PCB Consequences

The timebase decision affects hardware.

Potential pin consumers:

| Function | Pin / resource pressure |
|---|---|
| HSE crystal | Oscillator pins |
| External clock input | Clock input pin |
| Sony-derived reference input | Timer/GPIO/EXTI pin |
| USB FS | D+ / D- pins |
| UCPD | CC1 / CC2 pins |
| SWD | SWDIO / SWDCLK |
| FG input | Timer input-capture pin |
| Motor PWM | PWM output pin |
| Optional PWM | Timer output pin |
| RV601/RV602/RV603 | ADC inputs |
| S601 | GPIO input |

The selected STM32G0C1KCU6 package must be checked before the schematic is considered stable.

Rules:

- Do not select a timebase architecture without checking exact package pinout.
- Do not assume family-level features are available on the chosen package pins.
- Do not consume clock pins needed for USB/UCPD without an explicit tradeoff.
- Provide test points for the selected clock/reference where possible.

---

## 10. Firmware Consequences

The timebase decision affects firmware architecture.

### 10.1 Configuration

Firmware must know:

- active clock source,
- timer frequency,
- USB clock source,
- whether calibration is applied,
- stored target period,
- and whether target period is tied to a specific clock mode.

### 10.2 Telemetry

Telemetry should report:

- firmware version,
- clock source,
- nominal timer frequency,
- measured or calibrated clock trim if applicable,
- target period,
- measured FG period,
- computed speed error,
- USB connection state,
- power state,
- and servo lock state.

### 10.3 Calibration

If calibration is supported, firmware must define:

- what value is calibrated,
- where calibration is stored,
- how it is validated,
- how it is restored after firmware update,
- how corrupt calibration is detected,
- and how to return to a safe default.

### 10.4 Failure Behavior

Firmware must fail safely if:

- clock source fails,
- oscillator does not start,
- USB clock recovery is unavailable,
- calibration data is missing/corrupt,
- power mode changes,
- USB is connected/disconnected,
- or firmware update mode is entered.

---

## 11. Measurement Plan

### 11.1 Bench Equipment

Recommended equipment:

- calibrated test tape,
- frequency counter or audio analyzer,
- oscilloscope,
- logic analyzer,
- temperature probe,
- regulated power supply,
- USB host/logger,
- wow/flutter meter or equivalent measurement setup.

### 11.2 Required Measurements

| Measurement | Purpose |
|---|---|
| FG frequency at correct tape speed | Establish target period |
| Tape speed after cold start | Evaluate initial accuracy |
| Tape speed after warmup | Evaluate drift |
| Tape speed on battery | Evaluate power-mode effect |
| Tape speed on external DC | Evaluate power-mode effect |
| Tape speed on USB-C power, if supported | Evaluate USB power effect |
| Tape speed with USB data connected | Evaluate USB ground/clock/service effect |
| Temperature drift | Evaluate oscillator suitability |
| Long-run drift | Evaluate stability |
| Wow/flutter | Evaluate transport impact |

### 11.3 Acceptance Method

A candidate timebase is not accepted until it is tested on real hardware.

The minimum acceptable evidence:

1. Measurement setup documented.
2. Test unit identified by serial number.
3. Power mode documented.
4. Clock source documented.
5. Calibration state documented.
6. Tape speed result recorded.
7. Drift over time recorded.
8. USB state recorded.
9. Raw readings preserved.

---

## 12. Decision Matrix

| Option | Accuracy confidence | Complexity | Preservation value | USB independence | Rev A suitability |
|---|---:|---:|---:|---:|---:|
| HSI16 + PLL only | Low until proven | Low | Low | High | Development only |
| Calibrated HSI16 | Medium | Medium | Low | High | Possible with testing |
| External crystal/oscillator | High | Medium | Medium | High | Strong candidate |
| Sony-derived reference | Potentially high | Medium/High | High | High | Investigate |
| USB-synchronized only | Medium when connected | Medium | Low | Low | Not preferred |

---

## 13. Rev A Recommendation

The Rev A design should not assume the internal oscillator is final.

Recommended Rev A path:

1. Use internal HSI16/PLL for early firmware development and board bring-up.
2. Add schematic and PCB provision for a precision external timebase or reference input.
3. Investigate whether the original Sony quartz reference can be safely retained or sampled.
4. Do not remove the external/reference-derived option until measurements justify doing so.
5. Require measured speed and drift data before making performance claims.

Preferred wording for documentation:

> The internal oscillator is acceptable for development and early validation. Final speed-performance claims require either measured proof of sufficient stability or use of a precision/reference-derived timebase.

---

## 14. Open Questions

1. What absolute speed accuracy target should DSR-1 claim?
2. What wow/flutter target should DSR-1 claim?
3. Is the original Sony X701 reference available and healthy in the target unit?
4. Can a Sony-derived reference be routed to the STM32 safely?
5. Does the selected STM32G0C1KCU6 package have enough pins for external clock plus USB/UCPD plus servo I/O?
6. Is an external oscillator mechanically and electrically feasible in the available board space?
7. Does USB connection improve, worsen, or change clock behavior?
8. Should USB service mode be allowed during playback?
9. How should clock calibration be stored and reported?
10. What happens if calibration data is corrupt?

---

## 15. Acceptance Criteria

The timebase decision is not accepted until:

| Requirement | Status |
|---|---|
| Exact STM32G0C1KCU6 package pinout reviewed | Complete; see `docs/stm32g0c1-pin-allocation.md` |
| USB FS clocking plan selected | Pending |
| UCPD/PD clocking implications reviewed | Pending |
| Servo timer clock source selected | Pending |
| External/reference-derived option evaluated | Pending |
| Sony X701/IC701 reference possibility evaluated | Pending |
| FG target frequency measured on real unit | Pending |
| Speed accuracy tested after calibration | Pending |
| Drift tested over warmup period | Pending |
| USB connected/disconnected behavior tested | Pending |
| Power-mode behavior tested | Pending |
| Wow/flutter measured | Pending |
| Documentation updated with final decision | Pending |

---

## 16. Design Rule

No final DSR-1 speed-performance claim may be made until the timebase is proven by measurement.

A stable digital loop locked to an unstable clock is still unstable in the real world.
