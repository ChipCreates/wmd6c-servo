# WM-D6C Interface Contract

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/hardware/wmd6c-interface-contract.md`  
**Status:** Draft / pre-Rev A  
**Purpose:** Define the electrical and mechanical interface between the Sony WM-D6C/TC-D6C and the DSR-1 replacement module.

---

## 1. Purpose

This document is the hardware interface contract between the Sony WM-D6C/TC-D6C and the DSR-1 digital capstan-servo replacement module.

The DSR-1 project must not proceed from concept to PCB layout by assumption. Every signal that crosses the boundary between the Sony machine and the DSR-1 module must be identified, measured, documented, and justified against the source material and bench data.

This file answers four questions for every interface signal:

1. **What Sony circuit node does DSR-1 connect to?**
2. **What does that node do in the original WM-D6C?**
3. **What electrical limits must DSR-1 obey?**
4. **What bench measurements are required before Rev A hardware can be finalized?**

This document is not a tutorial and not a theory document. It is a design-control document.

---

## 2. Governing References

The Rev A interface is governed by:

1. **Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1, 2001.06**
   - Used for schematic references, board/component references, adjustment procedures, service cautions, and the documented servo-circuit revision.

2. **STMicroelectronics STM32G0B1xB/xC/xE Datasheet, DS13560 Rev 6, February 2026**
   - Used for MCU electrical limits, ADC/DAC constraints, timer capability, package limitations, oscillator limitations, flash behavior, USB capability, and operating conditions.

3. **Bench measurements from physical WM-D6C units**
   - Required for final signal voltage ranges, waveform shape, noise behavior, source impedance, motor-control behavior, and safe interface design.

If these sources conflict, the order of authority is:

1. Bench measurement on the actual target unit.
2. Sony service manual.
3. STM32G0B1 datasheet.
4. DSR-1 design assumptions.

Assumptions must never be treated as measured facts.

---

## 3. Compatibility Scope

### 3.1 Primary Target

The first DSR-1 hardware revision targets:

| Target | Status |
|---|---|
| Sony WM-D6C / TC-D6C Ver. 1.1 servo circuit | Primary reference target |

The Sony service manual identifies a servo-circuit change in the Ver. 1.1 documentation set. Because of that, older or alternate WM-D6C board revisions must not be assumed compatible until mapped.

### 3.2 Out of Scope Until Separately Mapped

| Machine / revision | Status |
|---|---|
| Earlier WM-D6C servo revisions | Not assumed compatible |
| WM-D6 | Future variant candidate |
| WM-D3 / WM-D3C | Future variant candidate |
| TC-D5M and related Sony professional machines | Future variant candidate |

A future variant requires its own schematic comparison, harness map, measured FG target, motor-drive characterization, and physical-unit validation.

---

## 4. Test Unit Identification

Each physical machine used for measurements must be logged.

### Current Known Unit

| Field | Value |
|---|---|
| Model | Sony WM-D6C |
| Serial number | 72795 |
| Serial-number source | Battery-compartment label photo |
| Board revision | Pending internal inspection |
| Servo circuit revision | Pending comparison against service-manual schematic |
| Measurement status | Not yet measured |

The serial number identifies the unit, but it does **not** prove the board revision or servo-circuit revision. Internal board photographs and schematic comparison are still required.

---

## 5. Boundary Definition

DSR-1 is not merely a replacement for the CX20084 IC. It replaces or interfaces with the capstan-servo subsystem surrounding IC601 while preserving as much of the original machine as practical.

### 5.1 DSR-1 May Interface With

| Sony subsystem | Function |
|---|---|
| IC601 / CX20084 area | Original capstan-servo IC location / servo node access |
| M901 / FG901 | Motor and frequency-generator feedback |
| Q601 and surrounding motor-drive network | Capstan motor control path |
| RV601 | Base tape-speed adjustment |
| RV602 | User Speed Tune control |
| RV603 | Speed Tune range adjustment |
| S601 | Speed Tune on/off switch |
| CN301 | External DC input path |
| CP304 | DC-DC / power-support board |
| Ground / chassis reference | Electrical reference and shielding behavior |

### 5.2 DSR-1 Should Preserve

| Original system | Preservation goal |
|---|---|
| Tape transport mechanics | No unnecessary mechanical modification |
| Audio signal path | No intentional modification |
| User controls | Preserve normal user-facing behavior |
| Sony adjustment procedure | Remain compatible where practical |
| Serviceability | Make the machine easier to diagnose, not harder |

---

## 6. Signal Summary

| DSR-1 signal | Sony reference area | Direction | Required before Rev A |
|---|---|---:|---|
| `FG_IN` | M901 / FG901 | Sony → DSR-1 | Waveform capture and voltage-range proof |
| `MOTOR_CTRL` | Q601 / motor-drive network | DSR-1 → Sony | Control-voltage and current characterization |
| `RV601_WIPER` | RV601 speed adjustment | Sony → DSR-1 | Wiper voltage range and source impedance |
| `RV602_WIPER` | RV602 Speed Tune | Sony → DSR-1 | Wiper voltage range and switch behavior |
| `RV603_WIPER` | RV603 Speed Tune range | Sony → DSR-1 | Wiper voltage range and scaling behavior |
| `S601_SPEED_TUNE` | S601 Speed Tune switch | Sony → DSR-1 | Logic polarity and voltage |
| `RAW_POWER` | CN301 / battery / CP304 region | Power | Polarity, voltage, surge, and load behavior |
| `B_PLUS` | Sony servo/motor rail | Power / sense | Voltage behavior during stop/start/play |
| `B_PLUS_3` | Sony regulated/support rail | Power / sense | Voltage and loading behavior |
| `GND` | Main board ground / chassis | Reference | Grounding and noise strategy |
| `USB_DP` / `USB_DM` | DSR-1 service interface | DSR-1 ↔ host | Firmware/debug use only |
| `SWDIO` / `SWDCLK` | DSR-1 programming interface | DSR-1 ↔ programmer | Development access |
| `BOOT0` | DSR-1 boot control | Input | DFU/programming access |

Signal names may change as the schematic is finalized, but the contract must remain clear.

---

## 7. FG Feedback Interface

### 7.1 Signal Name

`FG_IN`

### 7.2 Sony Reference Area

| Sony reference | Meaning |
|---|---|
| M901 | Capstan motor assembly |
| FG901 | Frequency-generator feedback source |

### 7.3 Function

The original servo uses FG feedback from the motor/capstan system to determine actual tape speed. DSR-1 measures the FG period using an STM32 timer input-capture peripheral.

### 7.4 DSR-1 Design Intent

DSR-1 should condition the FG signal into a clean, safe logic-level input suitable for STM32 timer capture.

The firmware design assumes:

| Firmware assumption | Current status |
|---|---|
| FG pulse train is periodic at correct speed | Must be measured |
| FG period can be measured with TIM2 input capture | Implemented in firmware prototype |
| Approximate nominal FG rate is around 2500 Hz | Placeholder estimate only |
| Signal is safe for direct MCU input | Not assumed; must be proven |

### 7.5 Required Bench Measurements

Before Rev A schematic finalization:

| Measurement | Required result |
|---|---|
| FG waveform amplitude | Determine if direct input, divider, clamp, comparator, or Schmitt stage is required |
| FG DC offset | Ensure STM32 input limits are not violated |
| FG edge shape | Determine need for hysteresis / filtering |
| Noise / ringing | Determine conditioning and grounding strategy |
| FG frequency at correct tape speed | Establish target period |
| FG behavior during startup | Prevent false capture / unstable lock |
| FG behavior during stop / pause | Define firmware state handling |

### 7.6 Electrical Safety Rules

- Do not connect FG901 directly to an STM32 pin until voltage range is measured.
- If FG exceeds STM32 input limits, add conditioning.
- If FG is slow-edged or noisy, add hysteresis or comparator conditioning.
- If FG is referenced to a noisy motor ground, ground routing must be reviewed before PCB layout.

### 7.7 Open Questions

- What is the actual FG waveform amplitude on the target unit?
- Is the FG waveform centered, pulled up, open collector, or otherwise conditioned by the Sony circuit?
- Does the Ver. 1.1 servo circuit change alter FG behavior compared with earlier revisions?
- Should DSR-1 use a digital input with protection, a comparator, or an analog conditioning stage?

---

## 8. Motor-Control Interface

### 8.1 Signal Name

`MOTOR_CTRL`

### 8.2 Sony Reference Area

| Sony reference | Meaning |
|---|---|
| Q601 | Motor-control transistor / servo drive area |
| Q603–Q605 | Supporting motor-control network, pending schematic confirmation |
| M901 | Capstan motor |

### 8.3 Function

The original servo controls motor speed by applying a correction signal into the motor-drive network. DSR-1 must reproduce the control behavior without overdriving, loading, or destabilizing the Sony motor circuit.

### 8.4 DSR-1 Design Intent

Two possible output strategies are under consideration:

| Option | Description | Use only if |
|---|---|---|
| Direct DAC | STM32 DAC drives the motor-control node through protection / isolation | Required voltage range is safely within DAC capability |
| PWM + filter + level shift | STM32 PWM is filtered and level-shifted into the Sony control range | Motor-control node exceeds DAC range or needs isolation |

The output strategy must be chosen from bench measurements, not assumptions.

### 8.5 Required Bench Measurements

| Measurement | Required result |
|---|---|
| Q601 / motor-control voltage during stop | Determine safe inactive state |
| Motor-control voltage during startup | Determine transient behavior |
| Motor-control voltage during steady play | Determine operating point |
| Motor-control voltage during speed correction | Determine output range |
| Motor current at startup | Determine power-path and protection requirements |
| Motor current during steady play | Determine normal loading |
| Motor response to small control perturbations | Tune PI loop and output scaling |

### 8.6 Electrical Safety Rules

- The DSR-1 output must fail safe at reset.
- MCU reset, bootloader entry, firmware crash, or USB connection must not drive the motor uncontrolled.
- A passive default state should hold the motor-control interface in a safe condition until firmware is active.
- The output stage must not backfeed Sony rails when DSR-1 is unpowered.
- Direct DAC drive is prohibited until the control-node voltage range is proven safe.

### 8.7 Open Questions

- What voltage range does the Sony motor-control node actually use?
- Does the motor-control node require current drive, voltage drive, or high-impedance control?
- What is the safe inactive state?
- Does the Sony motor circuit need to remain partially intact for stable operation?
- Which original components should be removed, bypassed, or retained?

---

## 9. Speed-Control Inputs

The WM-D6C speed-control network must be preserved where practical. DSR-1 should retain the original adjustment behavior instead of replacing it with firmware-only constants.

### 9.1 RV601 — Base Speed Adjustment

| Field | Value |
|---|---|
| DSR-1 signal | `RV601_WIPER` |
| Sony reference | RV601 |
| Direction | Sony → DSR-1 |
| Function | Base tape-speed calibration |

#### Required Measurements

- Minimum wiper voltage.
- Maximum wiper voltage.
- Center / nominal setting voltage.
- Source impedance.
- Noise while motor is running.
- Interaction with the rest of the speed-control network.

#### Safety Rules

- The wiper must not exceed STM32 ADC input limits.
- If voltage can exceed safe range, add scaling and protection.
- Firmware must not assume midscale until measured.

### 9.2 RV602 — User Speed Tune

| Field | Value |
|---|---|
| DSR-1 signal | `RV602_WIPER` |
| Sony reference | RV602 |
| Direction | Sony → DSR-1 |
| Function | User-accessible Speed Tune control |

#### Required Measurements

- Minimum wiper voltage.
- Maximum wiper voltage.
- Center detent or nominal user setting, if present.
- Behavior with S601 enabled.
- Behavior with S601 disabled.
- Effective speed range in the original circuit.

#### Safety Rules

- Preserve the user-facing Speed Tune function where practical.
- Do not allow disconnected or dirty potentiometer readings to command unsafe speed.
- Firmware should clamp adjusted target period to safe limits.

### 9.3 RV603 — Speed Tune Range

| Field | Value |
|---|---|
| DSR-1 signal | `RV603_WIPER` |
| Sony reference | RV603 |
| Direction | Sony → DSR-1 |
| Function | Sets Speed Tune range / sensitivity |

#### Required Measurements

- Minimum wiper voltage.
- Maximum wiper voltage.
- Relationship to RV602 effect.
- Whether the adjustment is factory-only or user-accessible.
- Whether the original range corresponds to the firmware scaling assumption.

#### Safety Rules

- Treat RV603 as calibration data, not a free-running user control, until confirmed.
- Firmware should bound the range contribution even if the pot is open or noisy.

---

## 10. Speed Tune Switch

### 10.1 Signal Name

`S601_SPEED_TUNE`

### 10.2 Sony Reference

`S601`

### 10.3 Function

S601 determines whether the Speed Tune function is active in the original circuit.

### 10.4 Required Bench Measurements

| Measurement | Required result |
|---|---|
| Switch logic when OFF | Determine pull direction |
| Switch logic when ON | Determine active polarity |
| Voltage level | Determine STM32 compatibility |
| Contact bounce | Determine firmware debounce need |
| Interaction with RV602/RV603 | Determine adjusted-target logic |

### 10.5 Safety Rules

- Do not assume active-high or active-low until measured.
- Use a defined pull-up or pull-down so the MCU input never floats.
- Firmware should default to normal calibrated speed if switch state is invalid.

---

## 11. Power Interface

### 11.1 Signals

| DSR-1 signal | Sony reference area | Function |
|---|---|---|
| `RAW_POWER` | CN301 / battery path | Incoming supply |
| `B_PLUS` | Sony power rail | Servo/motor rail, exact role pending measurement |
| `B_PLUS_3` | Sony support rail | Regulated/support rail, exact role pending measurement |
| `GND` | Main board ground | Reference |

### 11.2 Design Intent

The power interface must support DSR-1, preserve the WM-D6C transport behavior, and eliminate or reduce the wrong-adapter failure mode associated with the original external DC input.

### 11.3 Required Bench Measurements

| Measurement | Required result |
|---|---|
| CN301 polarity and voltage | Confirm original external power behavior |
| Battery rail voltage | Determine operating range |
| CP304 output behavior | Determine replacement/support requirement |
| Startup current | Size protection and conversion components |
| Steady-play current | Size normal operation |
| Motor transient current | Confirm surge margin |
| Reverse-polarity event behavior | Define protection test |
| Overvoltage event behavior | Define protection test |

### 11.4 Safety Rules

- DSR-1 must not make the external power failure mode worse.
- Reverse polarity protection must be verified, not merely assumed.
- Overvoltage behavior must be tested with current-limited supplies.
- DSR-1 must not backfeed rails when unpowered.
- USB, SWD, and external power paths must not create unintended power injection.

### 11.5 Rev A Recommendation

For Rev A, a protected barrel/lab-input power strategy is preferred over making USB-C PD part of the critical path. Prove the servo first. Add USB-C PD after motor control, timing, and interface behavior are validated.

---

## 12. Grounding and Noise

### 12.1 Signal Name

`GND`

### 12.2 Function

Ground is not just a return conductor. It is the reference for FG measurement, ADC readings, DAC/PWM output, USB signaling, and motor noise behavior.

### 12.3 Required Bench Checks

- Identify Sony main-board ground points near IC601.
- Identify motor-current return paths.
- Identify audio-path ground sensitivity.
- Measure ground noise during motor startup and steady play.
- Determine whether DSR-1 should use separate analog/digital/motor return routing joined at one point.

### 12.4 Safety Rules

- Do not route motor current through MCU analog reference paths.
- Do not allow USB ground to inject noise into sensitive audio or servo references without review.
- Avoid long high-impedance ADC wiring.
- Place FG and motor-control returns intentionally.

---

## 13. MCU-Side Electrical Constraints

The STM32G0B1 interface must remain within datasheet limits.

### 13.1 ADC Inputs

Affected signals:

- `RV601_WIPER`
- `RV602_WIPER`
- `RV603_WIPER`
- Optional rail-sense inputs, if added.

Rules:

- ADC input voltage must remain within safe limits for the selected MCU supply.
- Add resistor dividers, series resistors, clamps, or buffers if Sony wipers exceed safe MCU range.
- High-impedance sources require appropriate sampling-time configuration or buffering.
- Analog inputs should include test pads for validation.

### 13.2 Digital Inputs

Affected signals:

- `FG_IN`
- `S601_SPEED_TUNE`
- Any motor-enable or state-detect signal added later.

Rules:

- Confirm whether selected STM32 pins are 5 V tolerant before using them with any non-3.3 V signal.
- Add protection if voltage can exceed MCU supply.
- Use defined pull states.
- Use hysteresis or comparator conditioning if edges are slow or noisy.

### 13.3 DAC / PWM Output

Affected signals:

- `MOTOR_CTRL`
- Optional filtered PWM output.

Rules:

- DAC output may only be used directly if the Sony control node is within safe voltage/current range.
- PWM output must be filtered and shielded from audio-sensitive paths if used.
- Output stage must have a known safe state during reset and boot.

### 13.4 Clock / Timebase

The firmware can store a target period in flash, but the physical accuracy of that period depends on the timer clock.

Rules:

- Do not claim final speed accuracy until the timebase strategy is decided and verified.
- Internal oscillator operation is acceptable for firmware development and simulation.
- Final hardware must either use a proven timing reference, calibration strategy, or measured evidence that the chosen clock meets project requirements.
- Any external oscillator or retained Sony reference must be reflected in the interface and schematic.

---

## 14. Required Measurement Log Format

Each measurement session should produce a log file under:

```text
docs/measurements/
```

Suggested filename format:

```text
wm-d6c-serial-72795-measurement-log-YYYY-MM-DD.md
```

Each log should include:

| Field | Required |
|---|---|
| Machine model | Yes |
| Serial number | Yes |
| Board photos | Yes |
| Board revision, if known | Yes |
| Service-manual reference used | Yes |
| Test equipment | Yes |
| Power source | Yes |
| Tape/test fixture used | If applicable |
| Probe grounding method | Yes |
| Measurement location | Yes |
| Raw readings | Yes |
| Interpretation | Yes |
| Open questions | Yes |

Measurements should include raw numbers, not only conclusions.

---

## 15. Rev A Interface Acceptance Criteria

The DSR-1 Rev A interface is not accepted until all items below are complete.

| Requirement | Status |
|---|---|
| Sony-side connection points identified | Pending |
| Board revision documented | Pending |
| FG waveform measured | Pending |
| FG conditioning selected | Pending |
| Motor-control voltage range measured | Pending |
| Motor output stage selected | Pending |
| RV601/RV602/RV603 voltage ranges measured | Pending |
| S601 logic measured | Pending |
| Power rails measured | Pending |
| Grounding strategy selected | Pending |
| MCU pinout checked against exact package | Pending |
| Timebase strategy selected | Pending |
| Schematic updated from measurements | Pending |
| ERC passed | Pending |
| Bench simulation passed | Pending |
| Physical Rev A board tested | Pending |
| Real transport validation passed | Pending |

---

## 16. Open Engineering Questions

1. Which exact servo-circuit revision is present in serial-numbered unit 72795?
2. What is the actual FG901 waveform at correct tape speed?
3. What is the actual Q601 / motor-control operating voltage?
4. Can the STM32 DAC safely drive the motor-control point, or is a level-shift stage required?
5. Are RV601/RV602/RV603 wipers safe for direct ADC input?
6. What is the actual logic behavior of S601?
7. What power rails must DSR-1 generate versus merely sense?
8. Should Rev A omit USB-C PD to reduce bring-up complexity?
9. Is the STM32 internal oscillator acceptable after calibration, or is an external timebase required?
10. What original Sony components must be removed, retained, or isolated?

---

## 17. Design Rule

No DSR-1 schematic node that connects to the Sony WM-D6C may be finalized without one of the following:

1. A service-manual citation and matching schematic location.
2. A bench measurement from a physical unit.
3. A clearly marked assumption with a required validation step.

Assumptions are allowed during design. They are not allowed to become Rev A hardware facts without measurement.
