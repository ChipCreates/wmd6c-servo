# USB-C Data and PD Test Plan

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/verification/07-usb-c-data-pd-test-plan.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Procedure for validating DSR-1 USB-C data/service behavior, USB CDC telemetry, firmware update path, USB-C attach/detach behavior, USB-C PD behavior, VBUS handling, and servo safety during USB activity.

---

## 1. Purpose

This document defines the USB-C data and PD test plan for DSR-1.

USB-C is a first-class DSR-1 feature. It is not only a connector. It may provide:

- service access,
- telemetry,
- live tuning,
- diagnostics,
- firmware update,
- VBUS detection,
- operating power,
- USB-C attach behavior,
- and possibly USB-C PD negotiation.

USB-C also introduces risks:

- host ground injection,
- VBUS backfeed,
- ESD exposure,
- cable attach/detach transients,
- firmware timing load,
- PD negotiation complexity,
- and possible servo disturbance.

This plan verifies that USB-C improves serviceability without compromising the WM-D6C transport.

---

## 2. Test Scope

This plan covers:

| Area | Included |
|---|---|
| USB physical connection | Connector, cable flip, attach/detach |
| USB electrical behavior | VBUS, D+/D−, ESD strategy, shield/ground |
| USB enumeration | Device recognition by host |
| USB CDC | Telemetry and command interface |
| USB service commands | Tuning, status, save, diagnostics |
| Firmware update | DFU or equivalent |
| USB fault behavior | Disconnect, suspend, invalid command, spam |
| PD behavior | If implemented |
| USB power behavior | If USB-C powers any part of DSR-1 |
| Servo interaction | No timing disturbance or unsafe motor output |

---

## 3. Prerequisites

Complete or review:

```text
docs/hardware/power-usb-c-architecture.md
docs/hardware/wmd6c-interface-contract.md
docs/hardware/timebase-decision.md
docs/verification/00-rev-a-bringup-checklist.md
docs/verification/06-power-fault-test-plan.md
```

Required before USB-C testing:

| Requirement | Status |
|---|---|
| USB-C connector inspected | Required |
| USB ESD protection populated | Required |
| USB D+/D− continuity checked | Required |
| VBUS path checked | Required |
| Shield/ground strategy documented | Required |
| 3.3 V rail verified | Required |
| Firmware boots safely | Required |
| Motor output safe at reset | Required |
| PD architecture documented, if used | Required |

---

## 4. Required Equipment

| Equipment | Purpose |
|---|---|
| USB host computer | Enumeration and CDC testing |
| USB-C cable set | Cable variation testing |
| USB-C power meter | VBUS/current measurement |
| USB protocol tools, if available | Enumeration/debug |
| PD analyzer | PD contract testing |
| Oscilloscope | VBUS/rail transient and noise |
| Logic analyzer | Optional D+/D− or debug timing |
| Current-limited supply | Safe power mode testing |
| SWD probe | Recovery/debug |
| Terminal software | CDC interaction |
| Test scripts | Command spam and telemetry logging |
| Audio analyzer / frequency counter | Servo disturbance during USB use |

---

## 5. Measurement Log

Recommended path:

```text
docs/measurements/dsr-1-rev-a-usb-c-data-pd-YYYY-MM-DD.md
```

Minimum metadata:

| Field | Required |
|---|---:|
| Date | Yes |
| Operator | Yes |
| DSR-1 hardware revision | Yes |
| Firmware build | Yes |
| USB host OS | Yes |
| USB cable used | Yes |
| USB power source | Yes |
| PD source / charger | If applicable |
| PD analyzer | If applicable |
| Power mode | Yes |
| Servo state | Yes |
| Raw observations | Yes |
| Pass/fail | Yes |

---

## 6. USB-C Physical Inspection

Before plugging in:

| Check | Pass condition |
|---|---|
| Connector solder joints | No bridges or weak joints |
| Shell tabs | Mechanically secure |
| D+/D− continuity | Correct |
| CC1/CC2 continuity | Correct |
| VBUS to protection path | Correct |
| Shield to intended node | Matches design |
| No VBUS-to-GND short | Pass |
| No D+/D− short | Pass |
| Cable insertion clearance | Pass |
| Mechanical strain | Acceptable |

Do not connect to a host if VBUS or D+/D− are shorted.

---

## 7. USB Attach / Detach Electrical Tests

No Sony machine connected for initial tests.

| Test | Expected |
|---|---|
| Plug cable into unpowered DSR-1 | Defined behavior |
| Plug cable into powered DSR-1 | No reset unless expected |
| Unplug cable | Safe state |
| Flip cable orientation | Same behavior |
| Use short cable | Works |
| Use longer cable | Works or limitation documented |
| Attach through hub | Works or limitation documented |
| Attach to charger-only source | Safe behavior |

Record:

| Event | VBUS | 3.3 V | Current | Firmware state | Result |
|---|---:|---:|---:|---|---|
| Attach | Pending | Pending | Pending | Pending | Pending |
| Detach | Pending | Pending | Pending | Pending | Pending |
| Cable flip | Pending | Pending | Pending | Pending | Pending |

---

## 8. USB Enumeration

### 8.1 Expected Behavior

DSR-1 should enumerate consistently as the intended USB device class.

Expected possibilities:

| Mode | Description |
|---|---|
| CDC service mode | Normal telemetry/tuning interface |
| DFU mode | Firmware update mode |
| Bootloader mode | Factory or application update mode |
| Fault/service-only mode | Optional diagnostic fallback |

### 8.2 Tests

| Test | Pass condition |
|---|---|
| First plug-in enumeration | Device appears |
| Repeated plug/unplug | Device recovers |
| Cable flip | Device appears |
| Host reboot with device connected | Device recovers |
| Device reset with host connected | Device re-enumerates |
| Brownout recovery | Safe behavior |
| Fault state enumeration | If implemented, reports fault |

Record device names, VID/PID if applicable, OS behavior, and any driver requirements.

---

## 9. USB CDC Service Tests

### 9.1 Basic Command Tests

Verify commands for:

| Command category | Required behavior |
|---|---|
| Help / command list | Returns valid response |
| Version | Reports firmware and board info |
| Status | Reports servo, power, USB state |
| Telemetry snapshot | Returns one coherent record |
| Continuous telemetry | Starts/stops cleanly |
| Tuning command | Validates and clamps input |
| Save settings | Writes safely |
| Restore/defaults | Safe behavior |
| Invalid command | Rejected safely |

### 9.2 Command Robustness

Test:

| Test | Required result |
|---|---|
| Empty command | Safe response |
| Unknown command | Rejected |
| Too-long command | Rejected/truncated safely |
| Rapid command spam | No crash |
| Continuous telemetry + commands | No crash |
| Disconnect mid-command | Safe recovery |
| Host terminal closes | Safe recovery |
| Non-ASCII input | Rejected safely |

USB command parsing must not become a firmware stability risk.

---

## 10. Telemetry Validation

Telemetry should include enough information to diagnose servo and power state.

Recommended fields:

| Field | Required? |
|---|---:|
| Firmware version | Yes |
| Board/config ID | Yes |
| Clock source | Yes |
| Power state | Yes |
| USB state | Yes |
| PD state, if implemented | Yes |
| FG period | Yes |
| Target period | Yes |
| Error | Yes |
| Integral | Yes |
| Output command | Yes |
| Output clamp state | Yes |
| RV601 ADC | Yes |
| RV602 ADC | Yes |
| RV603 ADC | Yes |
| S601 state | Yes |
| Rail voltages, if sensed | Recommended |
| Fault flags | Yes |

Telemetry test:

| Test | Pass condition |
|---|---|
| Snapshot while idle | Coherent values |
| Snapshot while simulated FG active | Correct values |
| Continuous telemetry 1 minute | No crash |
| Continuous telemetry 30 minutes | No leak/lockup |
| Telemetry during playback | No speed disturbance |
| Telemetry during save | Safe behavior |

---

## 11. Firmware Update / DFU Test

### 11.1 Entry Method

Document how update mode is entered.

Possible methods:

| Method | Notes |
|---|---|
| BOOT0 pad/button | Hardware-controlled |
| USB command reboot-to-bootloader | Convenient but must be protected |
| Reset sequence | Possible but must be documented |
| SWD only | Development fallback |

### 11.2 Update Tests

| Test | Required result |
|---|---|
| Enter update mode intentionally | Works |
| Accidental entry during playback | Prevented |
| Host sees DFU/update device | Works |
| Flash firmware | Works |
| Preserve calibration/settings | If designed |
| Recover from failed update | Defined |
| Return to application | Works |
| Motor output during update | Safe |
| USB disconnect during update | Safe/recoverable |

Do not rely on firmware update mode until recovery is proven.

---

## 12. USB-C PD Tests

Only applicable if Rev A implements PD, either native STM32 UCPD or external PD controller.

### 12.1 PD Contract Tests

| Test | Required result |
|---|---|
| Attach to basic 5 V USB source | Safe behavior |
| Attach to PD charger | Expected contract |
| Cable flip | Same behavior |
| Non-PD charger | Safe fallback |
| Low-current source | Safe refusal or limited mode |
| High-current PD source | Correct contract only |
| Wrong/unsupported voltage | Rejected or protected |
| Contract renegotiation | Safe behavior |
| Source removal | Safe shutdown |

Record:

| Source | Cable | Contract voltage | Current limit | DSR-1 state | Result |
|---|---|---:|---:|---|---|
| Pending | Pending | Pending | Pending | Pending | Pending |

### 12.2 Native UCPD-Specific Tests

If STM32 native UCPD is used:

| Test | Required result |
|---|---|
| PD interrupts under telemetry | No servo disturbance |
| PD attach during playback | Safe behavior |
| PD detach during playback | Safe behavior |
| PD error state | Fault safe |
| Firmware reset during contract | Safe behavior |
| Bootloader mode with USB-C attached | Safe behavior |

### 12.3 External PD Controller-Specific Tests

If external controller is used:

| Test | Required result |
|---|---|
| Controller negotiates without MCU | Safe expected behavior |
| MCU reads controller status | Correct |
| Controller fault | Safe |
| Controller absent/unpowered | Safe |
| Controller output undervoltage | Safe |
| Controller output overvoltage | Protected |

---

## 13. USB and Servo Interaction

USB must not disturb the servo.

Test while servo is running first on simulated FG, later on real transport.

| USB action | Required result |
|---|---|
| Attach USB | No motor output glitch |
| Enumerate | No motor output glitch |
| Open CDC | No speed disturbance |
| Start telemetry | No speed disturbance |
| Command burst | No speed disturbance |
| Save settings | Controlled output behavior |
| Enter update mode | Blocked during playback or motor safe |
| Disconnect USB | No speed disturbance |
| Host suspend | Safe behavior |
| Host resume | Safe behavior |

Measure:

- FG period,
- motor output,
- audio frequency if in real transport,
- rail voltage,
- USB current,
- firmware state.

---

## 14. USB Ground / Noise Test

USB host ground may affect the cassette recorder.

With real transport installed and stable:

| Test | Measurement |
|---|---|
| USB disconnected | Baseline audio/FG/noise |
| USB connected, idle | Compare |
| USB telemetry active | Compare |
| USB cable moved | Compare |
| Different USB host | Compare |
| Different charger/source | Compare |
| USB shield strategy variant, if testable | Compare |

Look for:

- audio noise,
- FG jitter,
- ADC reading shift,
- motor-control output shift,
- rail ripple,
- ground offset,
- hum or buzz.

---

## 15. Backfeed Tests

USB-specific backfeed tests overlap with the power-fault plan but must be verified here too.

| Condition | Required result |
|---|---|
| USB connected, Sony unpowered | No unintended motor/rail power unless designed |
| USB connected, DSR-1 service only | Motor output safe |
| Sony powered, USB host disconnected | No VBUS on connector |
| External DC + USB host | No source conflict |
| Battery + USB host | No source conflict |
| SWD + USB + external power | Safe defined behavior |

Record all rail voltages.

---

## 16. Mechanical USB Tests

If USB-C is exposed through the case:

| Test | Required result |
|---|---|
| Cable insertion | No PCB flex damage |
| Cable removal | No connector movement |
| Cable side load | Acceptable strain |
| Case closed | Cable fits |
| Battery door access | Not blocked |
| Controls accessible | Not blocked |
| Transport operation | Not obstructed |
| Plug clearance | Common cables fit |

Connector mechanical failure can destroy the board or case. Treat it as a validation item.

---

## 17. Acceptance Criteria

USB-C data/PD validation is accepted when:

| Requirement | Status |
|---|---|
| USB-C physical inspection passed | Pending |
| Attach/detach electrical tests passed | Pending |
| Cable flip test passed | Pending |
| USB enumeration reliable | Pending |
| CDC command interface stable | Pending |
| Telemetry validated | Pending |
| Invalid command handling validated | Pending |
| Firmware update path tested | Pending |
| Motor output safe during update mode | Pending |
| PD contract tested, if implemented | Pending |
| PD failure behavior tested, if implemented | Pending |
| USB during simulated servo tested | Pending |
| USB during real playback tested | Pending |
| USB ground/noise tested | Pending |
| Backfeed tests passed | Pending |
| Mechanical connector tests passed | Pending |
| Logs committed | Pending |

---

## 18. Release Claim Gate

Do not claim:

- USB-C service ready,
- USB CDC telemetry ready,
- USB firmware update ready,
- USB-C PD ready,
- USB-C powered operation ready,
- or safe USB connection during playback

until this plan has passing logs.

---

## 19. Final Rule

USB-C is not validated when the device enumerates.

It is validated when attach, detach, telemetry, tuning, update, power, ground, and PD behavior all remain safe while the servo continues to protect the transport.
