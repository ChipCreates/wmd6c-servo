# WM-D6C Interface Contract

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/hardware/wmd6c-interface-contract.md`  
**Status:** Draft / pre-Rev A  
**Purpose:** Define the electrical, power, USB-C, mechanical, and service interface between the Sony WM-D6C/TC-D6C and the DSR-1 Power Board / Servo Control Board subsystem.

---

## 1. Purpose

This document is the hardware interface contract between the Sony WM-D6C/TC-D6C and the DSR-1 digital servo, power, and USB-C service module.

DSR-1 is not being scoped as a servo-only daughterboard. The Rev A design must treat the following as first-class project domains:

1. **Capstan servo replacement**
2. **Power-input and power-support modernization**
3. **USB-C data/service interface**
4. **USB-C PD or USB-C power-role strategy**

The project must not proceed from concept to PCB layout by assumption. Every signal that crosses the boundary between the Sony machine and the DSR-1 boards must be identified, measured, documented, and justified against the source material and bench data.

This file answers five questions for every interface signal:

1. **What Sony circuit node or DSR-1 service node does this connect to?**
2. **What does that node do in the original or new system?**
3. **What electrical limits must DSR-1 obey?**
4. **What firmware behavior is required?**
5. **What bench measurements are required before Rev A hardware can be finalized?**

This document is not a tutorial and not a theory document. It is a design-control document.

---

## 2. Governing References

### 2.1 For Ver. 1.0 Boards (CX20084, 1984 – mid 2001) — Rev A Primary Target

1. **Sony WM-D6C / TC-D6C Service Manual, original edition (`fb4872.pdf`)**
   - Used for the CX20084 servo circuit schematic, component values, board layout, adjustment procedures, and tape speed calibration procedure.
   - This is the governing servo-circuit reference for all CX20084 boards, including the primary test unit (serial 72795, ~1987).

2. **STMicroelectronics STM32G0C1KCU6 / STM32G0 family documentation**
   - Used for MCU electrical limits, ADC constraints, timer capability, PWM output, USB, UCPD, package limitations, and operating conditions.

3. **Bench measurements from physical WM-D6C units**
   - Required for final signal voltage ranges, waveform shape, noise behavior, source impedance, motor-control behavior, power-rail behavior, and safe interface design.

### 2.2 For Ver. 1.1 Boards (CX-069A, mid 2001 – 2002) — Planned Variant

4. **Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1, 2001.06 (`sony_wm-d6c_tc-d6c_ver-1.1.pdf`)**
   - Documents the servo circuit change (ECN-WMA00831) to CX-069A + five-transistor motor drive.
   - Governing reference for Ver. 1.1 board variant design (post Rev A).

### 2.3 Authority Order

If sources conflict:

1. Bench measurement on the actual target unit.
2. Service manual appropriate to the confirmed board revision.
3. STM32G0C1KCU6 package/device documentation.
4. DSR-1 design assumptions.

Assumptions must never be treated as measured facts.

---

## 3. Design Scope

DSR-1 has three first-class interface domains.

| Domain | Scope |
|---|---|
| Servo | FG input, motor-control output, speed-control inputs, servo timing, telemetry |
| Power | CN301/battery/CP304 behavior, protected input, power rails, fault protection |
| USB-C data / PD | USB data, telemetry, firmware update, diagnostics, optional/native PD, connector behavior |

The servo remains performance-critical, but power and USB-C are not optional documentation afterthoughts. They must appear in the schematic, pinout, verification plan, and firmware architecture.

---

## 4. Compatibility Scope

DSR-1 targets the **full WM-D6C / TC-D6C production run (1984–2002)**.

### 4.1 Rev A Primary Target

| Target | Status |
|---|---|
| WM-D6C / TC-D6C Ver. 1.0 CX20084 boards (1984 – mid 2001) | **Rev A primary target** |

Rev A targets CX20084 boards. This covers the majority of surviving and
collector-held units across the full production run prior to the 2001 servo
circuit change.

Primary test unit: serial 72795, observed PCB marking C11-494-12, surface-mount
CX20084 confirmed at IC601, rounded amorphous 35711 head confirmed.

### 4.2 Planned Variants

| Machine / revision | Status |
|---|---|
| WM-D6C / TC-D6C Ver. 1.1 CX-069A boards (mid 2001 – 2002) | **In scope — planned post-Rev A variant** |
| WM-D6 | Future variant candidate |
| WM-D3 / WM-D3C | Future variant candidate |
| TC-D5M and related Sony professional machines | Future variant candidate |

A planned variant requires its own schematic comparison, harness map, motor-drive
characterization, power-interface characterization, USB/service constraints, and
physical-unit validation before it may be called supported.

---

## 5. Test Unit Identification

Each physical machine used for measurements must be logged.

### Current Known Unit

| Field | Value |
|---|---|
| Model | Sony WM-D6C |
| Serial number | 72795 |
| Serial-number source | Battery-compartment label photo |
| Board revision | Pending internal inspection |
| Servo circuit revision | Pending comparison against service-manual schematic |
| Power-path condition | Pending inspection |
| CN301 condition | Pending inspection |
| CP304 condition | Pending inspection |
| Measurement status | Not yet measured |

The serial number identifies the unit, but it does **not** prove the board revision, servo-circuit revision, or power-path condition. Internal board photographs and schematic comparison are still required.

---

## 6. Boundary Definition

DSR-1 replaces or interfaces with the capstan-servo subsystem, the power-support path, and the new USB-C service/power interface.

### 6.1 DSR-1 May Interface With

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
| Battery rail | Portable power behavior |
| Ground / chassis reference | Electrical reference and shielding behavior |

### 6.2 DSR-1 Adds

| DSR-1-added subsystem | Function |
|---|---|
| USB-C connector | Service/data and possible power input |
| USB CDC interface | Telemetry, tuning, diagnostics |
| Firmware update path | DFU or equivalent update mechanism |
| USB-C PD strategy | Power negotiation, either native UCPD or external controller |
| Protection network | ESD, reverse-polarity, overcurrent, and overvoltage protections as required |

### 6.3 DSR-1 Should Preserve

| Original system | Preservation goal |
|---|---|
| Tape transport mechanics | No unnecessary mechanical modification |
| Audio signal path | No intentional modification |
| User controls | Preserve normal user-facing behavior |
| Sony adjustment procedure | Remain compatible where practical |
| Serviceability | Make the machine easier to diagnose, not harder |
| Portable behavior | Preserve safe battery and external-power operation where practical |

---

## 7. Signal Summary

| DSR-1 signal | Sony / DSR-1 reference area | Direction | Required before Rev A |
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
| `USB_DP` / `USB_DM` | DSR-1 USB-C service interface | DSR-1 ↔ host | USB FS routing, ESD, firmware behavior |
| `USB_CC1` / `USB_CC2` | USB-C connector / UCPD or PD controller | DSR-1 ↔ source | Attach detection / PD strategy |
| `VBUS` | USB-C connector | Host/source → DSR-1 | Voltage, protection, power mux behavior |
| `USB_SHIELD` | USB-C shell / chassis strategy | Mechanical/electrical | Shield termination decision |
| `SWDIO` / `SWDCLK` | DSR-1 programming interface | DSR-1 ↔ programmer | Development access |
| `BOOT0` | DSR-1 boot control | Input | DFU/programming access |

Signal names may change as the schematic is finalized, but the contract must remain clear.

---

## 8. FG Feedback Interface

### 8.1 Signal Name

`FG_IN`

### 8.2 Sony Reference Area

| Sony reference | Meaning |
|---|---|
| M901 | Capstan motor assembly |
| FG901 | Frequency-generator feedback source |

### 8.3 Function

The original servo uses FG feedback from the motor/capstan system to determine actual tape speed. DSR-1 measures the FG period using an STM32 timer input-capture peripheral.

### 8.4 DSR-1 Design Intent

DSR-1 should condition the FG signal into a clean, safe logic-level input suitable for STM32 timer capture.

The firmware design assumes:

| Firmware assumption | Current status |
|---|---|
| FG pulse train is periodic at correct speed | Must be measured |
| FG period can be measured with TIM2 input capture | Implemented in firmware prototype |
| Approximate nominal FG rate is around 2500 Hz | Placeholder estimate only |
| Signal is safe for direct MCU input | Not assumed; must be proven |

### 8.5 Required Bench Measurements

| Measurement | Required result |
|---|---|
| FG waveform amplitude | Determine if direct input, divider, clamp, comparator, or Schmitt stage is required |
| FG DC offset | Ensure STM32 input limits are not violated |
| FG edge shape | Determine need for hysteresis / filtering |
| Noise / ringing | Determine conditioning and grounding strategy |
| FG frequency at correct tape speed | Establish target period |
| FG behavior during startup | Prevent false capture / unstable lock |
| FG behavior during stop / pause | Define firmware state handling |

### 8.6 Electrical Safety Rules

- Do not connect FG901 directly to an STM32 pin until voltage range is measured.
- If FG exceeds STM32 input limits, add conditioning.
- If FG is slow-edged or noisy, add hysteresis or comparator conditioning.
- If FG is referenced to a noisy motor ground, ground routing must be reviewed before PCB layout.

---

## 9. Motor-Control Interface

### 9.1 Signal Name

`MOTOR_CTRL`

### 9.2 Sony Reference Area

| Sony reference | Meaning |
|---|---|
| Q601 | Motor-control transistor / servo drive area |
| Q603–Q605 | Supporting motor-control network, pending schematic confirmation |
| M901 | Capstan motor |

### 9.3 Function

The original servo controls motor speed by applying a correction signal into the motor-drive network. DSR-1 must reproduce the control behavior without overdriving, loading, or destabilizing the Sony motor circuit.

### 9.4 DSR-1 Design Intent

DSR-1 uses a PWM + RC filter + NPN level-shift output stage (TIM3 CH1 on PA6).

Q601 on WM-D6C serial 72795 is on the surface-mount C11-494-12 board. Its exact
package/marking and base operating range are pending physical confirmation. Direct
DAC drive is not used; the NPN level-shift topology (Q_LS MMBT3904, R7–R9) is the
committed output design.

Bench measurement of the Q601 base voltage during playback is still required to
confirm R9 sizing and the PWM duty-cycle-to-speed mapping.

### 9.5 Required Bench Measurements

| Measurement | Required result |
|---|---|
| Q601 / motor-control voltage during stop | Determine safe inactive state |
| Motor-control voltage during startup | Determine transient behavior |
| Motor-control voltage during steady play | Determine operating point |
| Motor-control voltage during speed correction | Determine output range |
| Motor current at startup | Determine power-path and protection requirements |
| Motor current during steady play | Determine normal loading |
| Motor response to small control perturbations | Tune PI loop and output scaling |

### 9.6 Electrical Safety Rules

- The DSR-1 output must fail safe at reset.
- MCU reset, bootloader entry, firmware crash, USB connection, or PD negotiation must not drive the motor uncontrolled.
- A passive default state should hold the motor-control interface in a safe condition until firmware is active.
- The output stage must not backfeed Sony rails when DSR-1 is unpowered.
- Direct DAC drive is not used; control-node voltage measurements are for validating
  the committed PWM + NPN level-shift output stage.

---

## 10. Speed-Control Inputs

The WM-D6C speed-control network must be preserved where practical. DSR-1 should retain the original adjustment behavior instead of replacing it with firmware-only constants.

### 10.1 RV601 — Base Speed Adjustment

| Field | Value |
|---|---|
| DSR-1 signal | `RV601_WIPER` |
| Sony reference | RV601 |
| Direction | Sony → DSR-1 |
| Function | Base tape-speed calibration |

Required measurements:

- Minimum wiper voltage.
- Maximum wiper voltage.
- Center / nominal setting voltage.
- Source impedance.
- Noise while motor is running.
- Interaction with the rest of the speed-control network.

Safety rules:

- The wiper must not exceed STM32 ADC input limits.
- If voltage can exceed safe range, add scaling and protection.
- Firmware must not assume midscale until measured.

### 10.2 RV602 — User Speed Tune

| Field | Value |
|---|---|
| DSR-1 signal | `RV602_WIPER` |
| Sony reference | RV602 |
| Direction | Sony → DSR-1 |
| Function | User-accessible Speed Tune control |

Required measurements:

- Minimum wiper voltage.
- Maximum wiper voltage.
- Center detent or nominal user setting, if present.
- Behavior with S601 enabled.
- Behavior with S601 disabled.
- Effective speed range in the original circuit.

Safety rules:

- Preserve the user-facing Speed Tune function where practical.
- Do not allow disconnected or dirty potentiometer readings to command unsafe speed.
- Firmware should clamp adjusted target period to safe limits.

### 10.3 RV603 — Speed Tune Range

| Field | Value |
|---|---|
| DSR-1 signal | `RV603_WIPER` |
| Sony reference | RV603 |
| Direction | Sony → DSR-1 |
| Function | Sets Speed Tune range / sensitivity |

Required measurements:

- Minimum wiper voltage.
- Maximum wiper voltage.
- Relationship to RV602 effect.
- Whether the adjustment is factory-only or user-accessible.
- Whether the original range corresponds to the firmware scaling assumption.

Safety rules:

- Treat RV603 as calibration data, not a free-running user control, until confirmed.
- Firmware should bound the range contribution even if the pot is open or noisy.

---

## 11. Speed Tune Switch

### 11.1 Signal Name

`S601_SPEED_TUNE`

### 11.2 Sony Reference

`S601`

### 11.3 Function

S601 determines whether the Speed Tune function is active in the original circuit.

### 11.4 Required Bench Measurements

| Measurement | Required result |
|---|---|
| Switch logic when OFF | Determine pull direction |
| Switch logic when ON | Determine active polarity |
| Voltage level | Determine STM32 compatibility |
| Contact bounce | Determine firmware debounce need |
| Interaction with RV602/RV603 | Determine adjusted-target logic |

### 11.5 Safety Rules

- Do not assume active-high or active-low until measured.
- Use a defined pull-up or pull-down so the MCU input never floats.
- Firmware should default to normal calibrated speed if switch state is invalid.

---

## 12. Power Interface

### 12.1 Signals

| DSR-1 signal | Sony / connector reference area | Function |
|---|---|---|
| `RAW_POWER` | CN301 / battery path | Incoming supply |
| `B_PLUS` | Sony power rail | Servo/motor rail, exact role pending measurement |
| `B_PLUS_3` | Sony support rail | Regulated/support rail, exact role pending measurement |
| `VBUS` | USB-C connector | USB-C power input / attach detection |
| `GND` | Main board ground | Reference |

### 12.2 Design Intent

The power interface must support DSR-1, preserve the WM-D6C transport behavior, and eliminate or reduce the wrong-adapter failure mode associated with the original external DC input.

USB-C power and PD behavior are in scope. DSR-1 may ultimately use one of these architectures:

| Architecture | Description |
|---|---|
| Native STM32 UCPD | STM32G0C1 handles USB-C attach/PD functions directly |
| External PD controller | Dedicated PD trigger/controller negotiates power and presents a simpler rail to DSR-1 |
| Hybrid protected input | USB-C data/service is required while external power strategy remains protected barrel/lab input for early Rev A |
| Dual-path | USB-C and protected barrel input coexist with a defined mux/protection strategy |

The architecture must be selected by pin availability, firmware complexity, safety behavior, layout feasibility, and bench validation.

### 12.3 Required Bench Measurements

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
| USB VBUS behavior | Define allowed power states |
| USB attach behavior | Define firmware/power sequencing |

### 12.4 Safety Rules

- DSR-1 must not make the external power failure mode worse.
- Reverse polarity protection must be verified, not merely assumed.
- Overvoltage behavior must be tested with current-limited supplies.
- DSR-1 must not backfeed Sony rails when unpowered.
- USB, SWD, battery, and external power paths must not create unintended power injection.
- PD negotiation failure must leave the machine in a safe state.
- USB connection for data alone must not unexpectedly power the motor path unless explicitly designed.

---

## 13. USB-C Data / Service Interface

### 13.1 Signals

| DSR-1 signal | Function |
|---|---|
| `USB_DP` | USB 2.0 FS D+ |
| `USB_DM` | USB 2.0 FS D- |
| `USB_CC1` | USB-C configuration / PD channel |
| `USB_CC2` | USB-C configuration / PD channel |
| `VBUS` | USB-C bus voltage |
| `USB_SHIELD` | Connector shell / shield strategy |
| `GND` | USB reference and system ground relationship |

### 13.2 Required Functions

The USB-C service interface should support:

| Function | Requirement |
|---|---|
| Telemetry | Live FG period, target, error, integral, output, ADC values, state |
| Live tuning | Adjust Kp, Ki, target, and calibration values |
| Configuration save/load | Flash-backed settings with validation |
| Firmware update | DFU or equivalent service process |
| Diagnostics | Boot status, fault reporting, power state, USB state |
| Manufacturing / bring-up | Board identification and test commands |

### 13.3 USB-C PD / Power Role

USB-C PD is now part of project scope. The exact implementation remains open.

Required decisions:

| Decision | Options |
|---|---|
| PD implementation | Native STM32 UCPD / external PD controller / staged hybrid |
| Power role | Sink only / service-only USB plus separate power / dual-path |
| Negotiated voltage | Pending power architecture |
| VBUS use | MCU only / full module / motor rail source |
| Failure behavior | Safe no-run / degraded service mode / battery-only |
| Firmware responsibility | Full PD stack / external-controller monitor / none |

### 13.4 Electrical Requirements

- USB D+/D− must be routed as a controlled, short, protected pair appropriate for full-speed USB.
- USB ESD protection is required.
- USB shield termination must be deliberate, not accidental.
- USB ground must not inject objectionable noise into audio, FG, or ADC references.
- USB attach/detach must not disturb capstan servo operation.
- USB telemetry and command handling must not interfere with the servo ISR.
- If native UCPD is used, PD interrupts and firmware must be isolated from real-time servo timing requirements.
- If an external PD controller is used, its default power behavior must be safe without MCU intervention.

### 13.5 Open Questions

- Is the selected STM32G0C1KCU6 package pinout sufficient for servo, ADC, PWM, USB FS, UCPD, BOOT0, and SWD simultaneously? **Resolved: yes; see `docs/stm32g0c1-pin-allocation.md`.**
- Should Rev A use native UCPD or an external PD controller?
- Should USB-C power be required for normal operation, or should battery/barrel operation remain independent?
- Should connecting USB for diagnostics while the machine is otherwise powered be supported?
- How should USB shield connect to WM-D6C chassis/ground?
- Can USB service be exposed without modifying the case beyond acceptable limits?

---

## 14. Grounding and Noise

### 14.1 Signal Name

`GND`

### 14.2 Function

Ground is not just a return conductor. It is the reference for FG measurement, ADC readings, PWM output, USB signaling, PD behavior, power conversion, and motor noise behavior.

### 14.3 Required Bench Checks

- Identify Sony main-board ground points near IC601.
- Identify motor-current return paths.
- Identify audio-path ground sensitivity.
- Measure ground noise during motor startup and steady play.
- Measure USB-connected ground behavior.
- Determine whether DSR-1 should use separate analog/digital/motor/USB return routing joined at one point.

### 14.4 Safety Rules

- Do not route motor current through MCU analog reference paths.
- Do not allow USB ground to inject noise into sensitive audio or servo references without review.
- Avoid long high-impedance ADC wiring.
- Place FG and motor-control returns intentionally.
- Treat USB shield and signal ground separately until the chassis/EMI strategy is selected.

---

## 15. MCU-Side Electrical Constraints

The STM32G0C1KCU6 interface must remain within datasheet limits.

### 15.1 ADC Inputs

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

### 15.2 Digital Inputs

Affected signals:

- `FG_IN`
- `S601_SPEED_TUNE`
- Any motor-enable or state-detect signal added later.
- USB-C / PD status signals if an external controller is used.

Rules:

- Confirm whether selected STM32 pins are 5 V tolerant before using them with any non-3.3 V signal.
- Add protection if voltage can exceed MCU supply.
- Use defined pull states.
- Use hysteresis or comparator conditioning if edges are slow or noisy.

### 15.3 PWM Output

Affected signals:

- `MOTOR_CTRL`
- Optional filtered PWM output.

Rules:

- PA4/DAC1 direct drive is not used for motor drive.
- PWM output must be filtered and shielded from audio-sensitive paths.
- Output stage must have a known safe state during reset and boot.

### 15.4 USB / UCPD Pins

Affected signals:

- `USB_DP`
- `USB_DM`
- `USB_CC1`
- `USB_CC2`
- `VBUS`

Rules:

- Verify all USB and UCPD pins against the exact selected STM32G0C1KCU6 package.
- Do not assume all family features are available on the chosen package pins.
- USB and UCPD routing must be reflected in the PCB constraints.
- USB service must not consume pins required for safe servo operation unless the pinout decision is explicit.

### 15.5 Clock / Timebase

The firmware can store a target period in flash, but the physical accuracy of that period depends on the timer clock.

Rules:

- Do not claim final speed accuracy until the timebase strategy is decided and verified.
- Internal oscillator operation is acceptable for firmware development and simulation.
- Final hardware must either use a proven timing reference, calibration strategy, or measured evidence that the chosen clock meets project requirements.
- Any external oscillator or retained Sony reference must be reflected in the interface and schematic.
- USB clocking requirements and servo timebase requirements must be reviewed together.

---

## 16. Required Measurement Log Format

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
| USB connection state | If applicable |
| Tape/test fixture used | If applicable |
| Probe grounding method | Yes |
| Measurement location | Yes |
| Raw readings | Yes |
| Interpretation | Yes |
| Open questions | Yes |

Measurements should include raw numbers, not only conclusions.

---

## 17. Rev A Interface Acceptance Criteria

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
| Power architecture selected | Pending |
| USB-C connector role selected | Pending |
| Native UCPD vs external PD controller decided | Pending |
| USB data/service behavior specified | Pending |
| USB ESD/shield strategy selected | Pending |
| Grounding strategy selected | Pending |
| MCU pinout checked against exact package | Pending |
| Timebase strategy selected | Pending |
| Schematic updated from measurements | Pending |
| ERC passed | Pending |
| Bench simulation passed | Pending |
| Physical Rev A board tested | Pending |
| USB enumeration tested | Pending |
| USB telemetry tested | Pending |
| Firmware update path tested | Pending |
| PD / power negotiation tested, if implemented | Pending |
| Real transport validation passed | Pending |

---

## 18. Open Engineering Questions

1. ~~Which exact servo-circuit revision is present in serial-numbered unit 72795?~~ **Resolved: CX20084 former-type board, observed PCB marking C11-494-12.**
2. What is the actual FG901 waveform at correct tape speed?
3. What is the actual Q601 / motor-control operating voltage?
4. ~~Can the STM32 DAC safely drive the motor-control point?~~ **Resolved: No. Direct DAC drive is not used; PWM + NPN level-shift is the committed topology.**
5. Are RV601/RV602/RV603 wipers safe for direct ADC input?
6. What is the actual logic behavior of S601?
7. What power rails must DSR-1 generate versus merely sense?
8. Should USB-C provide both service data and operating power?
9. Should PD be implemented with STM32 native UCPD or an external controller?
10. What is the safe behavior when USB is connected but the Walkman is off?
11. What is the safe behavior when external power is present but USB data is disconnected?
12. Is the STM32 internal oscillator acceptable after calibration, or is an external timebase required?
13. What original Sony components must be removed, retained, or isolated?
14. How should USB shield/chassis/ground be handled?
15. What case modification, if any, is acceptable for USB-C access?

---

## 19. Design Rule

No DSR-1 schematic node that connects to the Sony WM-D6C or to the USB-C external world may be finalized without one of the following:

1. A service-manual citation and matching schematic location.
2. A datasheet-backed electrical requirement.
3. A bench measurement from a physical unit.
4. A clearly marked assumption with a required validation step.

Assumptions are allowed during design. They are not allowed to become Rev A hardware facts without measurement.
