# STM32G0B1 Pin Allocation

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/hardware/stm32g0b1-pin-allocation.md`  
**Status:** Draft / pre-Rev A  
**Scope:** STM32G0B1 package choice, pin budget, alternate-function conflicts, USB-C/UCPD feasibility, servo I/O allocation, and Rev A package-risk assessment.

---

## 1. Purpose

This document defines the STM32G0B1 pin-allocation process for DSR-1.

DSR-1 now includes three first-class domains:

1. **Capstan servo replacement**
2. **Power-input / power-support modernization**
3. **USB-C data/service and USB-C PD strategy**

That means the MCU pinout is a hard architectural constraint. The project must prove that the selected STM32G0B1 package can support all required servo, power, USB, debug, update, and optional timing functions before Rev A schematic capture is considered stable.

This document answers:

- Which exact STM32G0B1 package is being used?
- Which pins are required by servo control?
- Which pins are required by USB-C data and PD?
- Which pins are required by SWD, BOOT0, clocks, ADC, DAC, PWM, and rail sensing?
- Which functions conflict?
- Is the originally selected package still viable?
- Should Rev A move to a larger package or external PD controller?

---

## 2. Current Package Assumption

The existing project materials refer to:

```text
STM32G0B1KBU6
```

This is treated as the **current candidate**, not an accepted final decision.

The STM32G0B1 family includes multiple packages and variants. The datasheet indicates that some low-pin-count packages have alternate pinouts and that N-suffix variants can expose VDDIO2 and an additional UCPD port. Because DSR-1 now includes USB-C data and PD scope, the exact package and suffix matter.

### 2.1 Current Status

| Field | Value |
|---|---|
| Current candidate MCU | STM32G0B1KBU6 |
| Package class | UFQFPN32 candidate |
| Status | Not yet accepted for Rev A |
| Main risk | Pin pressure from servo + ADC + DAC/PWM + USB + UCPD + SWD + BOOT0 + timebase |
| Rev A rule | Package is accepted only after exact pin and alternate-function audit |

---

## 3. Required DSR-1 MCU Functions

### 3.1 Servo-Critical Functions

| Function | Required? | Notes |
|---|---:|---|
| FG input capture | Yes | Timer input-capture capable pin required |
| Motor-control PWM output | Required | TIM3_CH1 on PA6, NPN level-shift stage |
| RV601 ADC input | Yes | Base speed calibration |
| RV602 ADC input | Yes | User Speed Tune |
| RV603 ADC input | Yes | Speed Tune range |
| S601 GPIO input | Yes | Speed Tune enable/disable |
| Optional motor-enable/state input | Possible | If Sony state detection is needed |
| Optional rail-sense ADC | Recommended | Power diagnostics and compensation |

### 3.2 USB-C / Service Functions

| Function | Required? | Notes |
|---|---:|---|
| USB D+ | Yes | USB 2.0 FS service/data |
| USB D− | Yes | USB 2.0 FS service/data |
| USB VBUS sense | Yes | Attach/power-state awareness |
| USB CC1 | Required if native UCPD | May be external PD-controller input instead |
| USB CC2 | Required if native UCPD | May be external PD-controller input instead |
| USB shield strategy | Hardware | Not MCU pin unless shield sense is added |
| USB CDC service | Firmware | Requires reliable USB pins/clock |
| DFU/update mode | Yes | BOOT0 or application-controlled update path |

### 3.3 Debug / Boot / Programming Functions

| Function | Required? | Notes |
|---|---:|---|
| SWDIO | Yes | Development and recovery |
| SWDCLK | Yes | Development and recovery |
| NRST | Strongly recommended | Recovery and reliable debug |
| BOOT0 | Required access | Test pad, jumper, button, or alternate update mechanism |
| UART debug | Optional | Only if pin budget allows |

### 3.4 Timebase Functions

| Function | Required? | Notes |
|---|---:|---|
| Internal oscillator | Development OK | Not accepted for final speed claims without proof |
| External HSE crystal/oscillator | Strong candidate | Consumes oscillator pins |
| External clock input | Candidate | Consumes clock-capable pin |
| Retained Sony-derived reference input | Candidate | Consumes timer/GPIO input |
| USB clock recovery / HSI48 | USB-related | Must not destabilize servo timebase |

---

## 4. Candidate Pin Allocation Table

This table records candidate assignments. It does **not** prove alternate-function validity. Each entry must be verified against the exact STM32G0B1 package, datasheet pin table, and alternate-function table.

| DSR-1 function | Candidate signal | Candidate STM32 pin | Peripheral need | Status |
|---|---|---|---|---|
| FG input capture | `FG_IN` | `PA0` | TIM2_CH1 or equivalent input capture | Candidate / verify AF |
| RV601 ADC | `RV601_WIPER` | `PA1` | ADC input | Candidate / verify ADC channel |
| RV602 ADC | `RV602_WIPER` | `PA2` | ADC input | Candidate / verify ADC channel |
| RV603 ADC | `RV603_WIPER` | `PA3` | ADC input | Candidate / verify ADC channel |
| Motor PWM | `MOTOR_PWM` | `PA6` | TIM3_CH1 PWM output | Candidate / verify AF |
| Speed Tune switch | `S601_SPEED_TUNE` | `PA7` | GPIO / EXTI optional | Candidate |
| USB D− | `USB_DM` | `PA11` or package-defined USB DM | USB FS | Candidate / verify |
| USB D+ | `USB_DP` | `PA12` or package-defined USB DP | USB FS | Candidate / verify |
| SWDIO | `SWDIO` | `PA13` | SWD | Required / verify package |
| SWDCLK / BOOT0 | `SWDCLK_BOOT0` | `PA14-BOOT0` | SWD / BOOT0 | Required / verify access strategy |
| USB-C CC1 | `USB_CC1` | TBD | UCPD or external PD status | Open |
| USB-C CC2 | `USB_CC2` | TBD | UCPD or external PD status | Open |
| VBUS sense | `VBUS_SENSE` | TBD | GPIO/ADC with divider/protection | Open |
| RAW power sense | `RAW_POWER_SENSE` | TBD | ADC with divider/protection | Optional / open |
| Motor rail sense | `MOTOR_RAIL_SENSE` | TBD | ADC with divider/protection | Recommended / open |
| External timebase | `EXT_CLK` / `HSE` | TBD | HSE/external clock input | Open |
| PD controller status | `PD_STATUS` | TBD | GPIO/I2C/SPI/UART depending controller | Open |
| PD controller I2C | `PD_SCL` / `PD_SDA` | TBD | I2C | Open if external PD |
| Fault input | `POWER_FAULT` | TBD | GPIO/EXTI | Recommended |
| Motor enable gate | `MOTOR_EN` | TBD | GPIO output | Recommended |

---

## 5. Servo-Critical Pins

Servo-critical functions have priority over convenience features.

### 5.1 FG Input Capture

Requirements:

- timer input-capture capable,
- low interrupt latency,
- protected against overvoltage and negative transients,
- available while USB and PD are active,
- not shared with clock or debug pins,
- routed away from switching noise.

Questions:

- Is `PA0` input capture available in the selected package?
- Does the chosen timer have sufficient resolution?
- Does FG conditioning need comparator output instead of direct GPIO/timer input?
- Is a comparator-to-timer path preferable?

### 5.2 Motor Output

The motor output may be DAC or PWM.

#### PWM (committed output topology)

Requirements:

- timer PWM output,
- adequate PWM frequency,
- output filter possible,
- level-shift/buffer possible,
- no audible or servo noise coupling,
- safe reset state.

PA6 (TIM3_CH1) is the committed motor PWM output pin.

### 5.3 Speed-Control ADC Inputs

Required ADC channels:

- RV601,
- RV602,
- RV603.

Recommended additional ADC channels:

- VBUS sense,
- RAW_POWER sense,
- motor rail sense,
- internal reference/temperature if useful.

Risk:

> Three speed-control inputs plus rail sensing may exceed comfortable ADC pin budget on a 32-pin package once USB/UCPD/debug/timebase pins are included.

---

## 6. USB-C / UCPD Pins

USB-C scope creates the largest pin-budget risk.

### 6.1 USB FS

Required:

- USB D+,
- USB D−,
- VBUS awareness,
- ESD protection,
- controlled routing,
- stable USB clock strategy.

Questions:

- Are USB D+/D− present on the exact selected package?
- Are they shared with pins needed by other functions?
- Does USB require VDDIO2 on the selected package variant?
- Is the package suffix appropriate for USB applications?

### 6.2 Native UCPD

If native STM32 UCPD is used, required:

- CC1,
- CC2,
- VBUS sense,
- USB/PD firmware support,
- power-role safe-state logic,
- PD interrupt isolation from servo timing.

Critical question:

> Does the candidate package expose the required UCPD pins without sacrificing servo-critical pins?

If the answer is unclear or unfavorable, use an external PD controller.

### 6.3 External PD Controller

An external PD controller may reduce MCU pin pressure.

Possible MCU connections:

| Signal | Requirement |
|---|---|
| `PD_STATUS` | GPIO input, indicates valid contract |
| `PD_FAULT` | GPIO input, optional |
| `PD_ENABLE` | GPIO output, optional |
| `I2C_SCL/SDA` | Optional if controller has configuration/status bus |
| `VBUS_SENSE` | ADC/GPIO sense still recommended |

Advantages:

- fewer UCPD pin constraints,
- simpler servo firmware timing,
- safer default PD behavior possible.

Disadvantages:

- more BOM,
- more board area,
- another part to source and validate.

---

## 7. SWD / BOOT / Recovery Pins

Recovery pins are not optional during Rev A.

Required:

| Function | Requirement |
|---|---|
| SWDIO | Accessible test pad/header |
| SWDCLK | Accessible test pad/header |
| NRST | Strongly recommended test pad |
| BOOT0 | Defined access method |
| GND | Debug header/test pad |
| 3.3 V / VREF | Debug reference |

Rules:

- Do not bury SWD pads under the board after installation.
- Do not let USB-C be the only recovery path until DFU/update has been proven.
- BOOT0 access must not require destructive disassembly unless the final install guide accepts that tradeoff.
- SWD connection must not backfeed the Sony machine or motor rail.

---

## 8. Optional External Timebase Pins

The timebase decision may require pins.

Options:

| Timebase option | Pin consequence |
|---|---|
| Internal HSI only | No external pins |
| External HSE crystal | Uses oscillator pins |
| External oscillator input | Uses clock input pin |
| Sony-derived reference | Uses timer/GPIO input |
| USB-synchronized only | No separate pin, but not preferred as sole servo reference |

Rules:

- Do not consume oscillator pins with other functions until timebase decision is closed.
- If using a 32-pin package, verify whether oscillator pins are present and usable.
- Provide test pad access to the selected reference if practical.
- Do not make final speed claims without a proven timebase.

---

## 9. Power and Rail-Sense Pins

Rail sensing is not mandatory for first motor motion, but it is valuable for safety and diagnostics.

Candidate sense inputs:

| Signal | Purpose | Priority |
|---|---|---|
| `VBUS_SENSE` | USB attach/power state | High |
| `RAW_POWER_SENSE` | Sony/external input state | Medium |
| `B_PLUS_SENSE` | Motor/servo rail state | Medium |
| `B_PLUS_3_SENSE` | Support rail state | Medium |
| `MOTOR_CURRENT_SENSE` | Fault/diagnostics | Optional |
| `POWER_FAULT` | Fast shutdown/fault indication | High if protection IC supports it |

If pin budget is tight, prefer fault-status GPIOs and one or two critical ADC sense inputs over excessive analog monitoring.

---

## 10. Conflict Matrix

| Conflict | Risk | Mitigation |
|---|---|---|
| USB D+/D− vs alternate package pins | USB unavailable or awkward routing | Verify exact package early |
| UCPD CC pins vs servo pins | Native PD may be impossible on 32-pin package | External PD controller or larger package |
| PA6 TIM3_CH1 availability | Verify AF1 on exact package | Required for motor PWM |
| ADC count vs rail sensing | Not enough analog inputs | Reduce rail sense or use external mux/ADC |
| SWD/BOOT vs user functions | Loss of debug/recovery | Reserve SWD/BOOT permanently |
| HSE pins vs other I/O | External timebase may be blocked | Reserve clock option until timebase decision closes |
| USB service vs servo timing | Firmware timing disturbance | Interrupt priorities and main-loop isolation |
| External PD I2C vs ADC/GPIO pins | More pin pressure | Select fixed-output PD trigger or larger package |

---

## 11. Package Risk Assessment

### 11.1 UFQFPN32 Candidate

| Area | Risk |
|---|---|
| Servo I/O | Likely possible, pending verification |
| USB FS | Possible only if package exposes required pins |
| Native UCPD | High risk / must verify exact suffix and pinout |
| External timebase | Pin pressure risk |
| Rail sensing | Limited headroom |
| External PD controller | May still require status/I2C pins |
| Debug access | Must be preserved |
| Overall | Risky but not rejected until audited |

### 11.2 UFQFPN48 / LQFP48 Candidate

| Area | Risk |
|---|---|
| Servo I/O | More comfortable |
| USB FS | More comfortable |
| Native UCPD | More likely feasible depending exact variant |
| External timebase | More practical |
| Rail sensing | More headroom |
| Debug access | Easier |
| Board size | Larger |
| Overall | Strong candidate if Rev A pin budget exceeds UFQFPN32 |

### 11.3 Recommendation Status

Current recommendation:

> Do not commit to STM32G0B1KBU6 / UFQFPN32 until the pin-allocation table is verified against the exact datasheet package and alternate-function tables. Because USB-C data and PD are in scope, a 48-pin package or external PD controller may be the more robust Rev A path.

---

## 12. Rev A Pinout Decision Gates

The MCU/package is accepted only when these are complete.

| Gate | Status |
|---|---|
| Exact part number selected | Pending |
| Exact package pinout captured | Pending |
| USB FS pins verified | Pending |
| Native UCPD pins verified, if used | Pending |
| SWD/BOOT access verified | Pending |
| FG timer input verified | Pending |
| PA6 TIM3_CH1 PWM pin verified | Pending |
| Three ADC pins verified | Pending |
| S601 GPIO verified | Pending |
| VBUS/power sense pins assigned | Pending |
| External timebase pins reserved or rejected | Pending |
| External PD controller pins assigned, if used | Pending |
| Pin conflicts reviewed | Pending |
| KiCad symbol/package updated | Pending |

---

## 13. Documentation Requirements

When the pinout is accepted, this file must be updated with:

1. Exact MCU part number.
2. Exact package.
3. Pin number.
4. Port/pin name.
5. Alternate function.
6. DSR-1 signal name.
7. Direction.
8. Voltage domain.
9. Protection/scaling requirement.
10. Reset-state behavior.
11. Firmware owner.
12. Test point requirement.

Final table format:

| Pin # | STM32 pin | DSR-1 signal | Function | AF/channel | Direction | Reset state | Notes |
|---:|---|---|---|---|---|---|---|
| TBD | TBD | `FG_IN` | Timer capture | TBD | Input | Safe | Pending |
| TBD | TBD | `MOTOR_CTRL` | DAC/PWM | TBD | Output | Safe inactive | Pending |
| TBD | TBD | `USB_DP` | USB FS | TBD | I/O | Hi-Z/USB | Pending |

---

## 14. Open Questions

1. Is STM32G0B1KBU6 the correct final part, or should Rev A move to a larger package?
2. Does the exact KBU6 package expose all USB FS pins required for USB-C data?
3. Does it expose UCPD CC pins suitable for native USB-C PD?
4. Is the N-suffix alternate pinout required?
5. Can DAC and USB/UCPD coexist with the required servo pins?
6. Is an external PD controller the safer way to preserve pin budget?
7. Is an external timebase required?
8. If yes, are HSE pins available and not conflicting?
9. How many rail-sense inputs are worth the pin cost?
10. Should Rev A include solder-jumper options to choose DAC vs PWM output?

---

## 15. Acceptance Criteria

The pin allocation is accepted when:

| Requirement | Status |
|---|---|
| Exact STM32G0B1 part/package selected | Pending |
| Datasheet pinout verified | Pending |
| Alternate functions verified | Pending |
| USB-C data pins assigned | Pending |
| PD strategy reflected in pins | Pending |
| Servo-critical pins assigned | Pending |
| Power sense/fault pins assigned | Pending |
| SWD/BOOT recovery preserved | Pending |
| Timebase option resolved or reserved | Pending |
| KiCad symbol matches part | Pending |
| Schematic labels match this document | Pending |
| Firmware `config.h` matches this document | Pending |

---

## 16. Design Rule

Do not route the Rev A PCB until the pin allocation is accepted.

A schematic that fits conceptually but not physically on the selected package is not a design. It is a wish list.
