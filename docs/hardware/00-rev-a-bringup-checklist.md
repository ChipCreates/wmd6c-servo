# Rev A Bring-Up Checklist

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/00-rev-a-bringup-checklist.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Safe bring-up sequence for DSR-1 Rev A hardware before, during, and after installation in a Sony WM-D6C / TC-D6C.

---

## 1. Purpose

This document defines the staged bring-up process for the first DSR-1 hardware revision.

The goal is to validate DSR-1 without damaging the Sony WM-D6C, the DSR-1 board, the USB host, the test equipment, or the motor/transport system.

DSR-1 combines three risk domains:

1. **Servo control** — can run the capstan motor incorrectly if unstable or miswired.
2. **Power conversion / protection** — can damage the Walkman or DSR-1 if polarity, rails, or fault paths are wrong.
3. **USB-C data / PD / service access** — can introduce VBUS, host ground, ESD, power-role, firmware-update, and backfeed risks.

Bring-up must therefore proceed in layers. No later stage may begin until the previous stage has passed or the failure has been documented and corrected.

---

## 2. Safety Position

DSR-1 Rev A must be assumed unsafe until proven otherwise.

Do not connect Rev A to a valuable WM-D6C until:

- bare board inspection passes,
- power rails are verified,
- USB behavior is verified,
- firmware boots and reports sane state,
- all outputs are confirmed safe at reset,
- ADC inputs are verified with simulated voltages,
- FG input is validated with a signal generator,
- motor-control output is validated into dummy loads,
- power-fault behavior is tested,
- and the target WM-D6C has been measured before installation.

The Sony machine should be the **last** thing connected, not the first.

---

## 3. Required Equipment

Minimum recommended equipment:

| Equipment | Purpose |
|---|---|
| Current-limited bench supply | Safe power-up, rail testing, fault testing |
| Digital multimeter | Rail voltage, continuity, resistance checks |
| Oscilloscope | FG waveform, motor-control output, power rail transients |
| Logic analyzer | FG capture, USB/service timing if needed |
| USB power meter / analyzer | USB-C VBUS/current monitoring |
| USB-C breakout / PD analyzer | CC/PD testing if PD is implemented |
| ST-Link or compatible SWD probe | Firmware bring-up and debug |
| USB host computer | CDC/DFU/service validation |
| Signal generator | Simulated FG input |
| Dummy load / resistor load | Motor-output and power-output validation |
| Test tape | Final speed calibration |
| Frequency counter / audio analyzer | Tape speed measurement |
| Wow/flutter meter or equivalent | Final performance validation |
| ESD-safe tools | Handling and installation |
| Magnification / microscope | Solder and PCB inspection |

Optional but useful:

- thermal camera,
- programmable electronic load,
- differential probe,
- bench USB isolator,
- audio analyzer,
- temperature probe,
- variable environmental test setup.

---

## 4. Documentation Required During Bring-Up

Every bring-up session should produce a log.

Recommended path:

```text
docs/measurements/rev-a-bringup-log-YYYY-MM-DD.md
```

Minimum required fields:

| Field | Required |
|---|---:|
| Date | Yes |
| Board revision | Yes |
| PCB serial / assembly ID | Yes |
| Firmware commit / build ID | Yes |
| Test operator | Yes |
| Test equipment | Yes |
| Power source | Yes |
| USB source / host | If used |
| PD controller / configuration | If used |
| Test step | Yes |
| Expected result | Yes |
| Actual result | Yes |
| Pass/fail | Yes |
| Raw measurements | Yes |
| Photos/screenshots | If relevant |
| Corrective action | If failed |

No “works fine” notes. Record numbers.

---

## 5. Stage Gate Summary

| Stage | Name | Sony WM-D6C connected? | Pass required before |
|---:|---|---:|---|
| 0 | Design-file review | No | PCB order / assembly |
| 1 | Bare board inspection | No | Power application |
| 2 | Unpowered electrical checks | No | Rail bring-up |
| 3 | Current-limited first power | No | MCU power-up |
| 4 | 3.3 V / local rail validation | No | SWD / firmware |
| 5 | SWD and firmware flash | No | USB/service |
| 6 | USB-C data validation | No | PD / VBUS testing |
| 7 | USB-C PD / VBUS validation | No | Output enable testing |
| 8 | Safe-state validation | No | ADC/DAC/FG tests |
| 9 | ADC input validation | No | Speed-control connection |
| 10 | DAC/PWM output validation | No | Motor-control connection |
| 11 | FG input validation | No | Servo simulation |
| 12 | Servo loop simulation | No | Sony preinstall work |
| 13 | Power-fault testing | No | Sony connection |
| 14 | WM-D6C preinstall measurements | Yes, unmodified | DSR-1 install |
| 15 | Passive Sony connection checks | Yes | Powered Sony tests |
| 16 | Powered Sony no-motor tests | Yes | Motor-control enable |
| 17 | Motor staged enable | Yes | Transport test |
| 18 | Real transport servo test | Yes | Speed calibration |
| 19 | Speed calibration | Yes | Wow/flutter testing |
| 20 | USB service during playback | Yes | Soak testing |
| 21 | Long-run soak test | Yes | Release claims |

---

## 6. Stage 0 — Design-File Review

Before ordering or assembling Rev A hardware:

### 6.1 Schematic Review

Confirm:

- every Sony-facing signal is listed in `wmd6c-interface-contract.md`,
- every power/USB signal is listed in `power-usb-c-architecture.md`,
- every clock assumption is listed in `timebase-decision.md`,
- every target board assumption is listed in `wmd6c-revision-compatibility.md`.

Checklist:

| Item | Status |
|---|---|
| FG input protection present | Pending |
| Motor-control safe-state present | Pending |
| ADC input protection/scaling present | Pending |
| USB D+/D− ESD protection present | Pending |
| USB-C CC strategy present | Pending |
| VBUS protection present | Pending |
| Reverse-polarity protection present if external DC retained | Pending |
| Overcurrent protection present | Pending |
| Backfeed paths reviewed | Pending |
| SWD access present | Pending |
| BOOT0/update access present | Pending |
| Test points present | Pending |
| External/reference timebase decision reflected | Pending |

### 6.2 PCB Review

Confirm:

- USB-C connector has mechanical support,
- USB D+/D− routing is short and intentional,
- shield/ground strategy is implemented deliberately,
- high-current motor/power paths are separated from FG/ADC references,
- analog inputs are not routed beside switching nodes,
- test pads are accessible after assembly,
- no pads require impossible soldering during installation,
- board outline fits intended cavity,
- connector/cable routing is realistic.

### 6.3 BOM Review

Confirm:

- all active parts are available,
- substitutes are documented,
- voltage/current ratings have margin,
- protection devices are correctly rated,
- regulator thermal behavior is plausible,
- USB-C connector is mechanically suitable,
- PD controller selection is documented if external PD is used.

---

## 7. Stage 1 — Bare Board / Assembly Inspection

Before power is applied:

### 7.1 Visual Inspection

Inspect under magnification:

| Area | Check |
|---|---|
| MCU pins | Bridges, opens, orientation |
| USB-C connector | Solder joints, shell tabs, pin bridges |
| PD controller, if present | Orientation and bridges |
| Regulators | Orientation and thermal pad |
| Protection devices | Orientation, correct placement |
| DAC/PWM output path | Incorrect values, shorts |
| FG input path | Incorrect values, shorts |
| ADC input network | Incorrect values, shorts |
| SWD/BOOT pads | Accessibility |
| Test pads | Accessibility |

### 7.2 Assembly Record

Record:

- PCB revision,
- assembly date,
- parts substitutions,
- missing/DNP parts,
- known rework,
- photos of top and bottom.

---

## 8. Stage 2 — Unpowered Electrical Checks

Before applying power:

### 8.1 Continuity / Resistance Checks

Measure resistance:

| Measurement | Expected |
|---|---|
| 3.3 V to GND | No short |
| VBUS to GND | No short |
| RAW_POWER to GND | No short |
| MOTOR_CTRL to GND | No short unless designed |
| FG_IN to GND | No unexpected short |
| USB shield to GND | Matches intended strategy |
| SWDIO/SWDCLK to GND | No short |
| BOOT0 to expected pull | Matches design |

### 8.2 Diode-Mode / Protection Checks

Check:

- ESD devices not shorted,
- reverse-polarity path not shorted,
- ideal diode / MOSFET orientation,
- regulator input/output behavior,
- USB VBUS path not shorted to system rails.

Do not continue if any rail is shorted.

---

## 9. Stage 3 — First Power Application

Use current-limited bench supply.

### 9.1 Initial Conditions

- No Sony WM-D6C connected.
- No motor connected.
- USB disconnected unless specifically testing VBUS.
- SWD disconnected unless needed.
- Current limit set low.
- Scope or meter on 3.3 V rail.

### 9.2 Power-Up Checklist

| Step | Action | Pass condition |
|---:|---|---|
| 1 | Apply low input voltage if regulator allows | No overcurrent |
| 2 | Increase to nominal input | No overcurrent |
| 3 | Measure 3.3 V | Within expected tolerance |
| 4 | Measure MCU reset state | Valid |
| 5 | Check regulator temperature | No rapid heating |
| 6 | Check VBUS/backfeed | No unintended voltage |
| 7 | Check motor output node | Safe inactive state |

Record input voltage, current, 3.3 V, temperature, and output states.

---

## 10. Stage 4 — Local Rail Validation

Validate all DSR-1-generated rails.

| Rail | Test | Pass condition |
|---|---|---|
| `+3V3` | No-load and firmware-load voltage | Stable |
| `VBUS_SENSE`, if present | Correct scaling | Safe ADC/GPIO level |
| `RAW_POWER_SENSE`, if present | Correct scaling | Safe ADC/GPIO level |
| Motor/support rail, if generated | Dummy load test | Stable and current-limited |
| PD-negotiated rail, if present | Contract voltage test | Correct and protected |

Add dummy loads before connecting Sony rails.

---

## 11. Stage 5 — MCU Boot, SWD, and Firmware Flash

### 11.1 SWD Validation

Confirm:

- SWD connects reliably,
- chip ID is read correctly,
- erase/program succeeds,
- reset works,
- firmware starts,
- debug halt/resume does not drive motor output unsafe.

### 11.2 Firmware Boot Validation

Firmware should report:

- firmware version,
- board/config ID,
- clock source,
- reset reason,
- rail status,
- USB status,
- servo disabled state,
- calibration/settings status.

Pass condition:

> Firmware boots with servo output disabled until explicitly armed by valid state conditions.

---

## 12. Stage 6 — USB-C Data Validation

No Sony machine connected.

### 12.1 USB Electrical

Check:

| Test | Pass condition |
|---|---|
| USB attach | No overcurrent |
| VBUS | Correct voltage and no unintended backfeed |
| USB enumeration | Device appears consistently |
| USB disconnect | No reset unless expected |
| USB reconnect | Recovers consistently |
| USB shield behavior | Matches intended strategy |

### 12.2 USB CDC / Service

Verify:

- terminal connects,
- banner prints,
- telemetry command works,
- invalid commands are rejected,
- command spam does not lock firmware,
- telemetry rate limiting works,
- USB disconnect during telemetry is safe.

### 12.3 USB Noise Baseline

With oscilloscope:

- observe 3.3 V during USB attach,
- observe motor-output safe-state node,
- observe FG input idle state,
- confirm USB does not create large rail disturbances.

---

## 13. Stage 7 — USB-C PD / VBUS Validation

If PD is implemented in Rev A:

### 13.1 PD Contract Test

Check:

| Test | Pass condition |
|---|---|
| Cable attach | Safe default |
| PD negotiation | Expected voltage/current |
| Cable flip | Same behavior |
| Non-PD charger | Safe behavior |
| Low-current source | Safe behavior |
| Disconnect | No unsafe output |
| Renegotiation | No unsafe output |
| Brownout | Safe reset/fault state |

### 13.2 PD Failure Behavior

Test:

- no contract,
- wrong voltage,
- undervoltage,
- overvoltage,
- rapid detach/attach,
- host/source reset,
- controller fault if external PD controller is used.

Pass condition:

> PD fault state never enables motor output or backfeeds Sony rails.

---

## 14. Stage 8 — Reset and Safe-State Validation

Before testing active inputs/outputs, prove every unsafe output defaults safe.

Test safe state during:

| Condition | Required behavior |
|---|---|
| No firmware | Motor output safe |
| Reset held | Motor output safe |
| Bootloader / DFU | Motor output safe |
| Firmware crash / watchdog | Motor output safe |
| USB attach | Motor output safe |
| PD negotiation | Motor output safe |
| Flash write | Motor output held safe or controlled |
| Brownout | Motor output safe |
| SWD halt | Motor output safe |

Pass condition:

> The motor-control path is not dependent solely on firmware behaving correctly.

---

## 15. Stage 9 — ADC Input Validation

No Sony machine connected.

Use known voltages or a test jig to validate each ADC path.

Signals:

- `RV601_WIPER`
- `RV602_WIPER`
- `RV603_WIPER`
- rail sense inputs, if present
- temperature/reference inputs, if present

Checklist:

| Test | Pass condition |
|---|---|
| 0 V input | ADC near expected low |
| Midscale input | ADC near expected mid |
| Full safe input | ADC near expected high |
| Slight overrange through protection | No damage; clamp behavior as designed |
| Open input | Firmware handles safely |
| Noisy input | Filtering acceptable |

Firmware should report raw ADC values over USB.

---

## 16. Stage 10 — DAC / PWM Output Validation

No Sony machine connected.

Use dummy load and oscilloscope.

### 16.1 DAC Output

If DAC is used:

| Test | Pass condition |
|---|---|
| Reset state | Safe |
| Minimum command | Expected voltage |
| Center command | Expected voltage |
| Maximum command | Expected voltage |
| Clamp behavior | Works |
| USB command update | No glitch |
| Flash write | Holds safe behavior |

### 16.2 PWM Output

If PWM is used:

| Test | Pass condition |
|---|---|
| PWM frequency | Expected |
| Duty range | Expected |
| RC-filter output | Acceptable ripple |
| Level shift | Correct output range |
| Reset state | Safe |
| EMI/noise baseline | Acceptable before Sony install |

Pass condition:

> Motor-control output is predictable, bounded, and safe before connecting to the Sony circuit.

---

## 17. Stage 11 — FG Input Validation With Signal Generator

No Sony machine connected.

Use signal generator or logic source.

Test:

| Input | Expected result |
|---|---|
| Nominal FG frequency | Correct measured period |
| Half-speed frequency | Correct measured period |
| Double-speed frequency | Correct measured period |
| Too-fast pulses | Rejected or handled |
| Missing pulses | Servo enters safe/unlocked state |
| Noisy signal | Conditioning/firmware handles or failure documented |
| Startup pulse train | Lock behavior acceptable |

Telemetry should report:

- measured period,
- computed frequency,
- target period,
- error,
- servo state,
- rejected pulses if counted.

---

## 18. Stage 12 — Servo Loop Simulation

No Sony machine connected.

Use simulated FG and dummy output.

Test:

- proportional response sign,
- integral behavior,
- anti-windup,
- output clamps,
- lock/unlock state,
- speed target changes,
- RV input influence,
- USB live tuning,
- flash save/restore,
- reset recovery.

Pass condition:

> The servo loop behaves correctly against simulated signals before real motor hardware is connected.

---

## 19. Stage 13 — Power-Fault Tests

No Sony machine connected.

Perform with current-limited supplies.

Required tests:

| Test | Pass condition |
|---|---|
| Reverse polarity | No damage; protected behavior |
| Overvoltage | Protection engages safely |
| Overcurrent | Limit/fuse behaves as designed |
| Shorted load | No unsafe heating or rail damage |
| USB plus external power | No backfeed |
| Battery rail simulation plus USB | No backfeed |
| SWD plus unpowered board | No unintended powering |
| PD failure | Safe state |

Document exact voltages, currents, durations, and protection behavior.

---

## 20. Stage 14 — WM-D6C Preinstall Measurements

Before modifying the Sony unit, perform `01-wmd6c-preinstall-measurements.md`.

Minimum required before connection:

- board photos,
- board revision notes,
- serial number,
- CN301 behavior,
- CP304 behavior,
- battery rail behavior,
- FG waveform,
- motor-control node voltage,
- RV601/RV602/RV603 voltages,
- S601 logic,
- ground reference notes.

No DSR-1 connection should be made until the target unit is documented.

---

## 21. Stage 15 — Passive Sony Connection Checks

With DSR-1 unpowered and Sony unpowered:

- verify harness continuity,
- verify no shorts to ground,
- verify no unexpected loading of Sony nodes,
- verify no backfeed from DSR-1 to Sony rails,
- verify all connection points against photos and notes.

With DSR-1 powered but Sony motor disabled:

- verify Sony nodes are not pulled to unsafe voltages,
- verify USB connection does not energize Sony motor rail unless designed,
- verify outputs remain safe.

---

## 22. Stage 16 — Powered Sony, Motor Disabled

Connect DSR-1 to Sony measurement points, but keep motor-control output disabled.

Check:

| Test | Pass condition |
|---|---|
| Sony power on | DSR-1 remains stable |
| USB service connected | No rail disturbance |
| ADC readings | Match measured Sony wiper voltages |
| FG input idle/play manually observed | Safe reading |
| Motor output disabled | No unintended motor drive |
| Power rails | Within expected range |

Do not enable motor output until all readings match preinstall measurements.

---

## 23. Stage 17 — Motor-Control Staged Enable

Enable motor-control output in stages.

Suggested progression:

1. Output connected to Sony node but held at safe inactive value.
2. Output moved slightly within measured safe range.
3. Confirm motor response.
4. Confirm direction/sign of correction.
5. Enable limited-range servo.
6. Enable full-range servo only after stable behavior.

Abort immediately if:

- motor runs away,
- output saturates unexpectedly,
- current rises abnormally,
- FG disappears,
- USB/telemetry stalls,
- rail collapses,
- audible mechanical distress occurs.

---

## 24. Stage 18 — Real Transport Servo Test

With tape transport operating:

Record:

- FG period,
- target period,
- output value,
- error,
- integral,
- ADC values,
- rail voltages,
- USB state,
- motor current if measurable,
- audio output frequency if using test tape.

Test:

- Play start,
- Stop,
- Pause if applicable,
- USB connect/disconnect,
- Speed Tune enable/disable,
- RV602 sweep within safe limits,
- warmup behavior.

Pass condition:

> Servo controls capstan speed without runaway, oscillation, audible hunting, rail collapse, or USB-induced disturbance.

---

## 25. Stage 19 — Speed Calibration

Using the Sony service adjustment mindset:

- use regulated power or documented power source,
- use calibrated test tape,
- measure output frequency,
- adjust target/calibration,
- save configuration,
- power cycle,
- verify calibration persists.

Record:

- test tape identity,
- measurement instrument,
- power mode,
- target period,
- FG frequency,
- audio output frequency,
- final saved settings.

---

## 26. Stage 20 — USB Service During Playback

With the machine running stably:

Test:

| Action | Required result |
|---|---|
| Connect USB | No speed disturbance |
| Start telemetry | No speed disturbance |
| Stop telemetry | No speed disturbance |
| Send invalid command | Rejected safely |
| Adjust tuning slightly | Expected controlled response |
| Save settings | No audible/servo instability |
| Disconnect USB | No speed disturbance |
| Host suspend/resume | Safe behavior |

If USB-C PD powers the transport, also test PD-specific behavior during playback.

---

## 27. Stage 21 — Long-Run Soak Test

Run at least one extended test after initial validation.

Record:

- duration,
- power mode,
- USB state,
- ambient temperature,
- rail voltage at start/end,
- FG period drift,
- output drift,
- speed drift,
- temperature rise,
- motor behavior,
- any faults.

Suggested minimum:

| Test | Minimum duration |
|---|---:|
| Servo only bench simulation | 30 minutes |
| Transport play test | One full tape side |
| USB telemetry during play | 30 minutes |
| Power/thermal soak | 1 hour minimum |

---

## 28. Release Claim Gate

Do not claim any of the following until supported by logs:

| Claim | Required evidence |
|---|---|
| Drop-in replacement | Successful install documentation |
| Safe power input | Fault testing |
| USB-C service ready | Enumeration, telemetry, update tests |
| USB-C PD ready | PD tests across chargers/cables |
| Better than original speed | Speed and wow/flutter measurements |
| Compatible with WM-D6C | Board revision and validation evidence |
| Compatible with all WM-D6C revisions | Multiple revision validations |

---

## 29. Final Rule

The Rev A board is not proven when it powers up.

It is proven only when it can be measured, faulted, reset, connected, disconnected, tuned, updated, and run in a real WM-D6C without creating a new failure mode.
