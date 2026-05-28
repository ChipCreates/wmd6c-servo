# Power Fault Test Plan

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/06-power-fault-test-plan.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Procedure for validating DSR-1 power-path protection, reverse-polarity behavior, overvoltage behavior, overcurrent behavior, backfeed prevention, USB-C VBUS safety, and motor-safe power failure behavior.

---

## 1. Purpose

This document defines the power-fault test plan for DSR-1.

Power protection is not a secondary feature. One of the reasons DSR-1 exists is to prevent or reduce the power-input failure modes that threaten surviving WM-D6C units. The new design must not introduce a different failure path through USB-C, VBUS, PD negotiation, SWD, battery rails, or motor-control outputs.

This plan answers:

- Does DSR-1 survive wrong-polarity input?
- Does it protect the Sony machine from wrong-polarity input?
- Does it survive overvoltage?
- Does current limiting behave correctly?
- Are USB, battery, external DC, SWD, and Sony rails isolated correctly?
- Does a power fault leave the motor-control output safe?
- Does USB-C PD failure leave the machine safe?
- Does DSR-1 fail safely under brownout, reset, and cable events?

---

## 2. Safety Position

Power-fault tests can destroy hardware.

Perform destructive-risk tests on:

1. unconnected DSR-1 boards,
2. bench fixtures,
3. dummy loads,
4. sacrificial assemblies,
5. and only then protected Sony-connected configurations.

Do not perform reverse-polarity or overvoltage tests on a valuable WM-D6C until the DSR-1 protection hardware has already passed bench testing.

Use current-limited supplies. Increase fault severity gradually. Stop immediately if temperature, smell, smoke, current, or voltage behavior is unexpected.

---

## 3. Prerequisites

Complete or review:

```text
docs/hardware/power-usb-c-architecture.md
docs/hardware/wmd6c-interface-contract.md
docs/verification/00-rev-a-bringup-checklist.md
docs/verification/01-wmd6c-preinstall-measurements.md
docs/verification/07-usb-c-data-pd-test-plan.md
```

Required before fault testing:

| Requirement | Status |
|---|---|
| Power architecture selected | Required |
| Protection components populated | Required |
| Rail names and test points documented | Required |
| USB-C role documented | Required |
| PD architecture documented, if used | Required |
| Current-limited bench supply available | Required |
| Dummy loads available | Required |
| Thermal monitoring available | Recommended |
| Sony machine disconnected for early tests | Required |

---

## 4. Required Equipment

| Equipment | Purpose |
|---|---|
| Current-limited bench supply | Controlled fault injection |
| Multimeter | Voltage/resistance verification |
| Oscilloscope | Transient and rail behavior |
| USB-C power meter | VBUS/current observation |
| USB-C PD analyzer | PD negotiation/fault testing |
| Electronic load | Controlled load and overload tests |
| Dummy motor load | Motor rail/output validation |
| Thermal camera or probe | Heating/fault detection |
| Resettable fuse / sacrificial fuse spares | Protection validation |
| ESD-safe tools | Handling |
| Safety glasses | Fault testing |
| Fire-safe bench area | High-energy fault safety |

---

## 5. Measurement Log

Recommended path:

```text
docs/measurements/dsr-1-rev-a-power-fault-YYYY-MM-DD.md
```

Minimum metadata:

| Field | Required |
|---|---:|
| Date | Yes |
| Operator | Yes |
| DSR-1 hardware revision | Yes |
| Firmware build | If MCU powered |
| Power architecture | Yes |
| PD controller / config | If applicable |
| Protection components | Yes |
| Test equipment | Yes |
| Current limit | Yes |
| Fault voltage/current | Yes |
| Duration | Yes |
| Rail readings | Yes |
| Temperature notes | Yes |
| Pass/fail | Yes |
| Damage/rework notes | If any |

---

## 6. Test Levels

Use staged severity.

| Level | Meaning |
|---|---|
| Level 0 | Unpowered resistance/continuity checks |
| Level 1 | Low-current functional test |
| Level 2 | Nominal operating test |
| Level 3 | Mild fault with strict current limit |
| Level 4 | Full expected user fault |
| Level 5 | Destructive-margin test, optional/sacrificial only |

Do not skip directly to high-energy faults.

---

## 7. Rails and Nodes Under Test

Define all relevant rails before testing.

| Node | Description | Status |
|---|---|---|
| `VBUS` | USB-C bus voltage | Pending |
| `RAW_POWER` | Original/external input or battery path | Pending |
| `+3V3` | MCU and logic rail | Pending |
| `B_PLUS` | Sony servo/motor rail | Pending |
| `B_PLUS_3` | Sony support rail | Pending |
| `MOTOR_SUPPLY` | Motor energy path, if separate | Pending |
| `MOTOR_CTRL` | Motor-control output node | Pending |
| `FG_IN` | FG input node | Pending |
| `USB_SHIELD` | Connector shell/chassis path | Pending |
| `SWD_VREF` | Debug connection power reference | Pending |

---

## 8. Pre-Fault Checks

Before applying fault conditions:

| Check | Pass condition |
|---|---|
| 3.3 V to GND resistance | No short |
| VBUS to GND resistance | No short |
| RAW_POWER to GND resistance | No short |
| VBUS to RAW_POWER | No unintended short |
| USB shield to GND | Matches design |
| SWD VREF to rails | Matches design |
| Motor output at reset | Safe |
| Protection devices inspected | Correct orientation |
| Fuse/polyfuse present | Correct value |
| TVS/clamp present | Correct orientation |
| Ideal diode/MOSFET path | Correct orientation |

Do not proceed if unpowered checks fail.

---

## 9. Nominal Power Tests

Before fault tests, prove nominal behavior.

### 9.1 External / RAW Power

| Test | Expected |
|---|---|
| Apply minimum valid input | No overcurrent; rails valid if designed |
| Apply nominal input | Rails valid |
| Apply maximum valid input | Rails valid; no overheating |
| Load step | No unsafe drop/glitch |
| Power removal | Safe shutdown |

### 9.2 USB-C VBUS

| Test | Expected |
|---|---|
| Attach USB-C source | Safe attach |
| 5 V VBUS present | Correct detection/protection |
| Data-only USB use | No unintended motor rail power unless designed |
| USB disconnect | Safe state |
| Cable flip | Same behavior |

### 9.3 Battery Simulation

If battery operation is supported:

| Test | Expected |
|---|---|
| Minimum battery voltage | Safe behavior |
| Nominal battery voltage | Normal behavior |
| Fresh battery/high voltage | Safe behavior |
| Brownout ramp down | Safe shutdown |
| Brownout ramp up | Safe startup |

---

## 10. Reverse-Polarity Tests

### 10.1 External DC Reverse Polarity

Only if an external/barrel path is supported.

Procedure:

1. Disconnect Sony machine.
2. Connect DSR-1 to current-limited supply.
3. Set voltage low.
4. Apply reverse polarity.
5. Observe input current.
6. Increase toward expected user-fault voltage if safe.
7. Monitor rails and temperature.
8. Remove power.
9. Re-test nominal operation.

Record:

| Test | Voltage | Current limit | Actual current | Result |
|---|---:|---:|---:|---|
| Low reverse | Pending | Pending | Pending | Pending |
| Nominal reverse | Pending | Pending | Pending | Pending |
| Max expected reverse | Pending | Pending | Pending | Pending |

Pass condition:

- no downstream rail is driven incorrectly,
- no MCU damage,
- no Sony-facing rail hazard,
- protection does not overheat under expected duration,
- board returns to normal operation.

### 10.2 USB-C Reverse / Invalid Source

USB-C should not expose user-reversible polarity in the same way, but invalid sources and cable faults must be considered.

Test with known safe USB-C test equipment only.

---

## 11. Overvoltage Tests

### 11.1 External Input Overvoltage

Procedure:

1. Use current-limited supply.
2. Start at nominal voltage.
3. Increase in small steps.
4. Observe clamp/fuse/regulator behavior.
5. Stop at predefined safety limit.

Record:

| Input voltage | Current | 3.3 V | Protected rail | Temperature | Result |
|---:|---:|---:|---:|---:|---|
| Nominal | Pending | Pending | Pending | Pending | |
| Mild OV | Pending | Pending | Pending | Pending | |
| Max expected fault | Pending | Pending | Pending | Pending | |

Pass condition:

- protected rails remain safe,
- protection devices behave as expected,
- no uncontrolled motor output,
- no backfeed to USB,
- no damage at defined fault levels.

### 11.2 USB-C Overvoltage / PD Wrong Voltage

If PD is implemented:

| Test | Expected |
|---|---|
| Unexpected 5 V only | Safe behavior |
| Expected negotiated voltage | Normal behavior |
| Wrong negotiated voltage | Refuse or fault safe |
| VBUS surge | Protected behavior |
| PD contract loss | Safe behavior |

Use proper USB-C/PD test equipment. Do not improvise high-voltage faults into a normal USB host.

---

## 12. Overcurrent and Short-Circuit Tests

Test outputs and rails with dummy loads.

| Node | Test | Expected |
|---|---|---|
| 3.3 V | Load step | Regulator stable or current limits |
| Motor rail | Load step | No unsafe collapse |
| Motor rail | Short through current limit | Protection responds |
| VBUS downstream | Overload | Protection responds |
| External input | Overload | Fuse/current limit responds |

Record:

| Node | Load | Current | Voltage | Duration | Result |
|---|---:|---:|---:|---:|---|
| Pending | Pending | Pending | Pending | Pending | Pending |

Pass condition:

- no fire/smoke,
- no unsafe heating,
- no MCU latch-up,
- no motor-control glitch,
- no permanent damage under defined limits.

---

## 13. Backfeed Prevention Tests

Backfeed is a major DSR-1 risk.

Test each source with all others disconnected/unpowered.

### 13.1 USB to Sony Rails

| Condition | Expected |
|---|---|
| USB connected, Sony unpowered | No unintended motor/servo rail power unless explicitly designed |
| USB connected, DSR-1 service mode | MCU may power if designed; motor rail remains safe |
| USB connected, external DC absent | No backfeed into CN301 path |

### 13.2 Battery / External DC to USB VBUS

| Condition | Expected |
|---|---|
| Battery/external power present, USB disconnected | No voltage on exposed VBUS |
| Battery/external power present, USB host connected | No source conflict |
| External DC present, USB-C PD source present | Defined priority / no contention |

### 13.3 SWD Backfeed

| Condition | Expected |
|---|---|
| SWD connected, board unpowered | No unintended full board power unless accepted |
| SWD VREF present | Correct behavior |
| SWD halt/reset | Motor output safe |

Record all measured rail voltages.

---

## 14. Brownout and Power Cycling

Test power transitions.

| Test | Expected |
|---|---|
| Slow ramp up | Safe boot |
| Slow ramp down | Safe shutdown |
| Rapid power cycle | Safe recovery |
| Brownout during playback | Motor output safe |
| Brownout during flash write | Settings recover or fail safe |
| USB attach during brownout | Safe behavior |
| PD renegotiation during brownout | Safe behavior |

Firmware must report reset reason if available.

---

## 15. Motor-Safe Fault Behavior

During all power-fault tests, monitor `MOTOR_CTRL`.

Required behavior:

| Fault | Required motor-control behavior |
|---|---|
| MCU reset | Safe |
| 3.3 V loss | Safe |
| VBUS attach | Safe |
| PD negotiation | Safe |
| External input fault | Safe |
| Brownout | Safe |
| Watchdog reset | Safe |
| Bootloader mode | Safe |
| Flash write failure | Safe |

No power fault may command uncontrolled motor drive.

---

## 16. Tests With Sony Connected

Only after bench tests pass.

Start with Sony connected but motor disabled.

| Test | Expected |
|---|---|
| DSR-1 powered, Sony off | No unsafe rail injection |
| Sony powered, DSR-1 observing | Rails match expectations |
| USB connected, Sony off | No unintended motor rail power |
| USB connected, Sony on | No disturbance |
| External power connected | Protected behavior |
| Power removed | Safe shutdown |

Do not perform full reverse-polarity or destructive overvoltage tests on a valuable Sony unit unless the protection strategy explicitly permits and the risk is accepted.

---

## 17. Acceptance Criteria

Power-fault validation is accepted when:

| Requirement | Status |
|---|---|
| Unpowered checks passed | Pending |
| Nominal power tests passed | Pending |
| Reverse-polarity test passed | Pending |
| Overvoltage test passed | Pending |
| Overcurrent test passed | Pending |
| Short-circuit test passed | Pending |
| Backfeed tests passed | Pending |
| USB VBUS behavior verified | Pending |
| PD fault behavior verified, if implemented | Pending |
| Brownout behavior verified | Pending |
| Motor-safe fault behavior verified | Pending |
| Post-fault normal operation verified | Pending |
| Results logged | Pending |

---

## 18. Release Claim Gate

Do not claim:

- reverse-polarity protection,
- overvoltage protection,
- USB-C safe power,
- PD safe operation,
- backfeed prevention,
- or protected barrel-input compatibility

until this test plan has passing logs.

---

## 19. Final Rule

A protection circuit is not proven because it is drawn on the schematic.

It is proven when it survives the fault, protects the Sony machine, recovers safely, and leaves the motor path controlled.
