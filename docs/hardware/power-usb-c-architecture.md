# Power and USB-C Architecture

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/hardware/power-usb-c-architecture.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Power input, power-path protection, USB-C data/service, USB-C PD strategy, firmware responsibilities, and Rev A acceptance criteria.

---

## 1. Purpose

This document defines the power and USB-C architecture for DSR-1.

DSR-1 is not a servo-only daughterboard. Rev A must treat the following as first-class design domains:

1. **Capstan servo replacement**
2. **Power-input and power-support modernization**
3. **USB-C data/service access**
4. **USB-C PD or USB-C power-role strategy**

The power and USB-C design must be developed with the same discipline as the servo design. It must be grounded in the Sony WM-D6C/TC-D6C service manual, the STM32G0B1 datasheet, and bench measurements from real hardware.

This file answers:

- How does DSR-1 receive power?
- How does DSR-1 protect the WM-D6C from wrong-adapter and power-fault conditions?
- What role does USB-C play: data only, power, PD negotiation, or all of the above?
- How does USB service access coexist with real-time servo control?
- What must be true before Rev A hardware can be considered electrically safe?

---

## 2. Design Position

Power and USB-C are now part of the core project scope.

The servo loop remains the performance-critical function, but the power system and USB-C service interface are not optional later add-ons. They affect:

- mechanical layout,
- connector placement,
- board size,
- power sequencing,
- grounding,
- firmware boot behavior,
- USB telemetry,
- field update procedure,
- motor safety,
- and failure behavior.

Therefore, Rev A must include an explicit power and USB-C architecture, even if some implementation details remain selectable by variant.

---

## 3. Governing References

The power and USB-C design is governed by:

1. **Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1, 2001.06**
   - Used for CN301, CP304, battery/external power behavior, service test conditions, board references, and the original system context.

2. **STMicroelectronics STM32G0B1xB/xC/xE Datasheet, DS13560 Rev 6, February 2026**
   - Used for USB FS, UCPD, GPIO, ADC, DAC, clock, flash, power, reset, operating-voltage, and package/pinout constraints.

3. **Bench measurements**
   - Required for actual rail voltages, current draw, startup surges, motor load behavior, CN301 behavior, CP304 behavior, USB-connected ground effects, and power-fault validation.

Design assumptions may be used during planning, but no assumption may become a Rev A hardware fact without measurement or datasheet support.

---

## 4. Required Functions

The DSR-1 power and USB-C architecture must support these functions.

| Function | Required for Rev A? | Notes |
|---|---:|---|
| Safe power input | Yes | Must not recreate the original wrong-adapter failure mode |
| Reverse-polarity protection | Yes | Especially if any barrel/external DC path remains |
| Overcurrent protection | Yes | Protect DSR-1, Sony rails, and motor path |
| Overvoltage protection | Yes | Required for external power fault handling |
| Backfeed prevention | Yes | USB, battery, external DC, and SWD paths must not unintentionally power each other |
| USB-C connector | Yes | Required for service/data; power behavior to be explicitly designed |
| USB 2.0 FS data | Yes | Telemetry, tuning, diagnostics, firmware update |
| USB CDC service mode | Yes | Live visibility into servo and power state |
| Firmware update path | Yes | DFU or equivalent service workflow |
| USB-C PD strategy | Yes | Native STM32 UCPD or external PD controller decision required |
| Battery coexistence | Yes | Battery operation must be understood and not broken accidentally |
| Motor-safe boot state | Yes | No uncontrolled motor drive during reset, boot, USB attach, or PD negotiation |

---

## 5. Power Sources

DSR-1 must explicitly define how each possible power source behaves.

### 5.1 Battery Power

The WM-D6C is a portable machine. Battery operation should be preserved unless a future design variant deliberately chooses otherwise.

Required questions:

- Does DSR-1 operate from the original battery rail?
- Does DSR-1 generate its own 3.3 V rail from the Sony rail?
- Is the motor rail supplied, sensed, or replaced by DSR-1?
- Does USB service work while the machine is battery-powered?
- Can USB data be connected without powering the transport?
- Does battery operation require lower current draw modes?

Required measurements:

| Measurement | Purpose |
|---|---|
| Battery rail voltage range | Determine regulator input limits |
| Battery rail under motor startup | Determine surge and dropout margin |
| Battery rail during steady play | Determine normal operating margin |
| Battery rail with USB connected | Detect unintended ground/power interactions |

### 5.2 Original External DC Path / CN301

CN301 is part of the failure story. If retained or replaced, its behavior must be explicitly engineered.

Possible design choices:

| Choice | Description |
|---|---|
| Retain CN301 with protection | Keep original external power look/behavior, but add reverse/overvoltage protection |
| Remove or bypass CN301 | Use USB-C or another protected input instead |
| Hybrid | Retain CN301 for compatibility while adding USB-C service/power |

Rules:

- Any retained barrel input must be protected.
- Any replacement path must not make the machine easier to damage.
- Wrong-polarity behavior must be tested with current-limited supplies.
- The external DC input must not backfeed USB or SWD.

Required measurements:

| Measurement | Purpose |
|---|---|
| CN301 polarity | Confirm actual unit behavior |
| CN301 voltage under service conditions | Confirm rail relationship |
| Current draw through CN301 | Determine protection sizing |
| Wrong-polarity test behavior | Validate protection |
| Overvoltage clamp behavior | Validate protection |

### 5.3 USB-C VBUS

USB-C introduces a new external power source and a new external ground reference.

Possible VBUS roles:

| Role | Description |
|---|---|
| Data/service only | USB powers only interface detection or MCU service mode, not the transport |
| MCU/service power | USB can power DSR-1 logic for diagnostics |
| Full operating power | USB-C/PD can power DSR-1 and the WM-D6C transport |
| Hybrid | USB service always available; transport power depends on selected mode/rail state |

Required decisions:

- Is USB-C required for normal operation?
- Does USB-C power the motor path?
- Is USB-C data available during battery operation?
- Does plugging in USB wake the MCU?
- Can USB power the MCU while the Walkman power switch is off?
- What happens if USB is connected during Play?
- What happens if USB is removed during Play?

Safety rule:

> USB attach, detach, enumeration, suspend, reset, or PD negotiation must not produce uncontrolled motor drive.

---

## 6. USB-C Functional Roles

### 6.1 USB Data / Service

USB data is required as a DSR-1 service function.

Required service functions:

| Function | Description |
|---|---|
| Telemetry | FG period, target, error, integral, output value, ADC readings, state |
| Live tuning | Adjust Kp, Ki, target, speed scaling, and calibration values |
| Configuration save/load | Flash-backed settings with checksum/validation |
| Boot banner | Report firmware version, configuration, flash status, and power status |
| Diagnostics | Report faults, rail state, USB state, servo lock state |
| Firmware update | DFU or equivalent workflow |
| Manufacturing test | Commands for Rev A bring-up and validation |

USB service must not compromise servo timing. The servo ISR has priority over all USB work.

### 6.2 USB Firmware Update

The firmware update path should avoid requiring permanent SWD access after the module is installed.

Possible update paths:

| Update path | Notes |
|---|---|
| STM32 factory USB DFU | Attractive if BOOT0 access and USB routing are compatible |
| Application-level bootloader | More flexible but adds firmware complexity |
| SWD only | Acceptable for development, not ideal for installed modules |

Required decisions:

- How does the user enter update mode?
- Is BOOT0 exposed as a test pad, button, solder bridge, or command?
- Can update mode be entered without opening the WM-D6C?
- What prevents accidental update-mode entry during normal use?
- Are saved calibration settings preserved across updates?

### 6.3 USB-C PD / Power Negotiation

USB-C PD is in scope. The implementation remains open.

Possible PD architectures:

| Architecture | Description |
|---|---|
| Native STM32 UCPD | STM32G0B1 handles CC/PD behavior directly |
| External PD controller | Dedicated controller negotiates power and provides a ready rail/status |
| Staged hybrid | Rev A includes USB-C data and pads/options for PD architecture validation |
| Dual-path | USB-C service plus separate protected external power path |

The architecture must be selected by pinout feasibility, firmware complexity, real-time isolation, safety behavior, and layout constraints.

---

## 7. Native STM32 UCPD Option

The STM32G0B1 family includes USB Type-C / PD capability, but family-level capability is not the same as a completed design.

### 7.1 Advantages

| Advantage | Why it matters |
|---|---|
| Fewer external ICs | Smaller BOM and potentially smaller PCB |
| Integrated status/control | Firmware can observe and control power-role behavior |
| Cleaner service story | One MCU owns servo, USB data, and USB-C state |

### 7.2 Risks

| Risk | Why it matters |
|---|---|
| Pin pressure | Servo, ADC, DAC/PWM, USB, UCPD, SWD, BOOT0, and optional clock pins all compete |
| Firmware complexity | PD handling must not interfere with real-time servo timing |
| Validation burden | PD fault states must be tested thoroughly |
| Boot-state behavior | PD negotiation during reset must leave motor path safe |
| Documentation burden | Contributors must understand both servo and PD firmware responsibilities |

### 7.3 Native UCPD Requirements

If native STM32 UCPD is selected:

- Verify exact package pinout supports required USB/UCPD pins.
- Define PD interrupt priority below servo-critical timing.
- Confirm PD firmware cannot starve or delay servo work.
- Define behavior before firmware is fully initialized.
- Define default sink behavior and safe no-contract behavior.
- Test attach, detach, brownout, cable flip, renegotiation, suspend, and reset.
- Ensure no PD state can command motor output unexpectedly.

---

## 8. External PD Controller Option

An external PD controller may reduce firmware risk and isolate power negotiation from servo timing.

### 8.1 Advantages

| Advantage | Why it matters |
|---|---|
| Lower firmware complexity | Servo firmware remains simpler and more deterministic |
| Safer default behavior possible | Some controllers can negotiate fixed voltage without MCU involvement |
| Less UCPD pin pressure | Frees MCU pins for servo, ADC, timing, and diagnostics |
| Easier power bring-up | Power rail can be validated separately from servo firmware |

### 8.2 Risks

| Risk | Why it matters |
|---|---|
| More BOM | More parts, more sourcing, more layout |
| Controller selection burden | Must choose a reliable and available part |
| Less flexibility | Fixed trigger/controller behavior may limit diagnostics |
| Need status monitoring | MCU may still need to know power state |
| Failure-state analysis required | Controller default/fault behavior must be understood |

### 8.3 External Controller Requirements

If an external PD controller is selected:

- Document exact controller behavior with no MCU firmware running.
- Define negotiated voltage and current.
- Define how DSR-1 knows whether a valid PD contract exists.
- Prevent motor power until rails are valid.
- Prevent backfeed into USB VBUS.
- Define fault behavior if controller fails, cable is unplugged, or voltage drops.
- Provide test points for VBUS, negotiated rail, controller status, and downstream rails.

---

## 9. Power-Path Safety Rules

The DSR-1 power path must obey these rules.

### 9.1 Reverse Polarity

- Any retained barrel or external DC input must survive wrong polarity.
- Reverse-polarity testing must be performed with a current-limited supply.
- The test must verify both DSR-1 survival and Sony machine protection.

### 9.2 Overvoltage

- External input overvoltage behavior must be defined and tested.
- Protection devices must be sized for realistic adapter faults.
- Overvoltage protection must not dump unsafe energy through delicate Sony paths.

### 9.3 Overcurrent

- Motor startup current must be measured before protection values are finalized.
- Fuses, current limits, regulators, and boost converters must tolerate normal transients.
- Fault current paths must be intentional.

### 9.4 Backfeed Prevention

Backfeed must be prevented between:

- USB VBUS and Sony rails,
- battery rail and USB VBUS,
- external DC input and USB VBUS,
- SWD programmer and DSR-1 rails,
- DSR-1 rails and Sony rails when either side is unpowered.

### 9.5 Motor-Safe Boot

The motor path must remain safe during:

- MCU reset,
- bootloader mode,
- USB attach,
- USB enumeration,
- PD negotiation,
- firmware crash,
- flash write,
- brownout,
- cable unplug,
- host suspend,
- and SWD connection.

A passive safe state must exist independent of firmware wherever possible.

---

## 10. Power Sequencing

Power sequencing must be explicit.

### 10.1 Required Rails

Rev A must identify every rail and whether DSR-1 creates, consumes, or senses it.

| Rail | Source | Role | Status |
|---|---|---|---|
| `VBUS` | USB-C | USB power / attach detection | Pending design |
| `RAW_POWER` | CN301 / battery / external path | Machine power input | Pending measurement |
| `+3V3` | DSR-1 regulator | MCU, ADC, USB logic | Required |
| `B_PLUS` | Sony or DSR-1 | Motor/servo rail | Pending measurement |
| `B_PLUS_3` | Sony or DSR-1 | Support rail | Pending measurement |
| `MOTOR_SUPPLY` | Sony or DSR-1 | Capstan motor energy | Pending measurement |

### 10.2 Sequencing Questions

- Which rail powers the MCU first?
- Which rail enables the motor path?
- Can the MCU boot from USB while the Sony transport is off?
- Does the MCU need to sense Sony power-switch state?
- Should telemetry be available before motor power is enabled?
- Can the servo output ever become active before FG input is valid?
- Does PD negotiation occur before or after motor-rail enable?

### 10.3 Required Firmware States

At minimum, firmware should distinguish:

| State | Meaning |
|---|---|
| `POWER_UNKNOWN` | Rails not yet characterized |
| `USB_SERVICE_ONLY` | USB connected, transport not powered |
| `SONY_POWER_PRESENT` | Machine rail present |
| `PD_NEGOTIATING` | USB-C power state not yet valid |
| `RAILS_VALID` | Required rails within expected range |
| `SERVO_ARMED` | Servo may drive motor output |
| `SERVO_LOCKED` | FG loop is stable |
| `FAULT` | Output disabled or safe-latched |

The exact names may change, but the states must exist conceptually.

---

## 11. USB Grounding and Shielding

USB introduces a host-connected ground and shield into a compact analog cassette recorder. This must be deliberate.

### 11.1 Required Decisions

| Decision | Options |
|---|---|
| USB shield termination | Chassis, digital ground, RC/ESD network, floating shell |
| USB ground relation | Direct system ground, filtered, star-point reference |
| ESD path | Dedicated protection to ground/chassis |
| Cable noise handling | Layout, filtering, grounding strategy |
| Service mode with audio monitoring | Must not inject audible noise |

### 11.2 Rules

- USB shield must not be connected accidentally by layout convenience.
- USB ESD return path must be short and intentional.
- Motor current must not share sensitive USB/ADC/FG return paths.
- USB connection must not measurably disturb FG capture or ADC speed-control readings.
- USB connection should be tested while the machine is in Stop, Play, and powered-off states.

---

## 12. Firmware Responsibilities

The firmware must treat USB and power as safety-sensitive systems, not just convenience features.

### 12.1 USB CDC

USB CDC command handling must:

- never run inside the servo ISR,
- never block servo timing,
- validate all tuning commands,
- clamp all parameters to safe ranges,
- expose boot and fault status,
- report power and USB state,
- support telemetry without flooding the host,
- remain safe if the host disconnects mid-command.

### 12.2 Flash Writes

Flash writes must not destabilize motor control.

Required behavior:

- freeze or safely hold servo output before flash erase/write,
- prevent repeated writes from command spam,
- validate settings with checksum or CRC,
- preserve calibration across firmware updates where practical,
- recover safely from interrupted writes.

### 12.3 PD / Power Firmware

If firmware participates in PD or power sequencing, it must:

- keep PD work lower priority than servo-critical timing,
- treat invalid power state as motor-disable,
- report PD contract state over USB,
- avoid enabling motor output until rails are valid,
- fail safe on brownout or renegotiation,
- distinguish service-only USB from operating-power USB.

---

## 13. Mechanical and Connector Considerations

USB-C is both electrical and mechanical.

Required questions:

- Where does the USB-C connector physically exit the WM-D6C?
- Is case modification acceptable?
- Does USB-C replace CN301 or coexist with it?
- Can a cable be connected while the machine is assembled?
- Does cable insertion stress the PCB?
- Is the connector mounted to the PCB, chassis, or flex/harness?
- Is there clearance for common USB-C plugs?
- Does connector placement interfere with battery operation, service access, or transport mechanics?

Mechanical decisions must not be left until after schematic completion.

---

## 14. Rev A Recommended Direction

Because power and USB-C are now first-class scope, Rev A should include:

1. A USB-C connector footprint.
2. USB D+/D− routed for service/data.
3. ESD protection for USB lines and VBUS.
4. A defined USB shield/ground strategy.
5. A selected PD architecture or populated/unpopulated option strategy.
6. Test points for VBUS, CC1, CC2, 3.3 V, motor rail, Sony raw rail, and ground.
7. A safe default motor-disable state during USB attach and PD negotiation.
8. Firmware states for USB service, rail validity, and servo enable.
9. A validation plan for both data and power behavior.

The remaining open architectural decision is not whether USB-C belongs in the project. It does. The open decision is **how** it should be implemented safely.

---

## 15. Rev A Acceptance Criteria

The power and USB-C architecture is not accepted until all items below are complete.

| Requirement | Status |
|---|---|
| CN301 / original power path measured | Pending |
| Battery rail behavior measured | Pending |
| CP304 behavior measured | Pending |
| Motor startup current measured | Pending |
| Normal play current measured | Pending |
| Power architecture selected | Pending |
| USB-C connector role selected | Pending |
| USB D+/D− routing planned | Pending |
| USB ESD strategy selected | Pending |
| USB shield/ground strategy selected | Pending |
| Native UCPD vs external PD controller decided | Pending |
| VBUS protection selected | Pending |
| Backfeed paths reviewed | Pending |
| Safe boot state defined | Pending |
| Power-sequencing firmware states defined | Pending |
| USB service mode defined | Pending |
| Firmware update path selected | Pending |
| Rev A schematic updated | Pending |
| Bench power fault tests passed | Pending |
| USB enumeration tested | Pending |
| USB telemetry tested | Pending |
| Firmware update tested | Pending |
| PD negotiation tested, if implemented | Pending |
| Servo operation tested with USB connected | Pending |
| No audible/servo disturbance from USB connection | Pending |

---

## 16. Open Questions

1. Should USB-C be the primary operating power input, service-only input, or both?
2. Should CN301 be retained, protected, removed, or bypassed?
3. Should DSR-1 support battery-only operation?
4. Can USB service be active while the Walkman power switch is off?
5. Can USB service be active while playing a tape?
6. Should Rev A use native STM32 UCPD or an external PD controller?
7. What negotiated USB-C voltage is required if USB-C powers the transport?
8. What is the maximum safe current draw from USB-C?
9. How should USB shield connect to chassis/system ground?
10. Does USB connection inject measurable noise into audio, FG, or ADC readings?
11. What happens if USB is unplugged during playback?
12. What happens if PD renegotiates or collapses during playback?
13. Does the MCU need to sense Sony power-switch state?
14. Should USB power ever enable motor drive without Sony-side power confirmation?
15. How should firmware distinguish service mode from operating mode?

---

## 17. Design Rule

No DSR-1 power, USB, PD, or service-interface node may be finalized without one of the following:

1. A Sony service-manual reference and matching schematic location.
2. A datasheet-backed electrical requirement.
3. A bench measurement from a physical unit.
4. A clearly marked assumption with a required validation step.

USB-C and power faults can damage the Sony machine as surely as a bad servo connection. Treat them as preservation-critical.
