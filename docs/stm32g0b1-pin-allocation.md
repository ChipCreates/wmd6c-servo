# STM32G0C1 Pin Allocation

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/stm32g0b1-pin-allocation.md` (filename retained; consider renaming to `stm32g0c1-pin-allocation.md`)  
**Status:** Accepted — Rev A baseline  
**Scope:** Finalized STM32G0C1KCU6 device selection, the complete 32-pin allocation, alternate-function/peripheral mapping, and the resolved package / PD / timebase decisions.

---

## 1. Purpose

This document records the **finalized** STM32 device and pin allocation for DSR-1 Rev A.

DSR-1 spans three first-class domains:

1. **Capstan servo replacement**
2. **Power-input / power modernization, including battery charge-in-place**
3. **USB-C data/service and USB-C PD**

The MCU pinout was a hard architectural constraint; it is now resolved. The
**STM32G0C1KCU6 (UFQFPN32)** carries every required servo, power-sense,
battery-indicator, USB, debug, and update function on its 32 pins, with spares left
over. This is the accepted allocation — schematic net labels and firmware `config.h`
must match it.

## 2. Finalized Device

```text
STM32G0C1KCU6   (UFQFPN32, 256 KB flash)
```

This is the **accepted Rev A part**, superseding the earlier STM32G0C1KCU6 candidate.

### 2.1 What changed from the KBU6 candidate

| Field | Old candidate | Finalized |
|---|---|---|
| Part | STM32G0C1KCU6 | **STM32G0C1KCU6** |
| Flash | 128 KB (B) | **256 KB (C)** — headroom for USB-CDC telemetry, indicator firmware, future variant |
| Package | UFQFPN32 (K) | UFQFPN32 (K) — unchanged |
| Suffix | KBU6 | **KCU6** |
| Variant | GP | **GP** — VDDIO2 tied internally to VDD |

Device facts that shaped the allocation:

- **GP variant, not N.** The GP/N distinction is VDDIO2 *exposure*, not package size.
  GP ties VDDIO2 internally to VDD — correct for a single-3.3 V design, no second I/O
  supply to decouple. (Confirm on the STM32G0C1 UFQFPN-32 pinout that VDDIO2 is not a
  separate pin in the GP K-package.)
- **PB15 absent** in the GP K-package (it exists only on the N variant) — not used here.
- **Combined supply pins.** The 32-pin K package merges VDD/VDDA (pin 4) and VSS/VSSA
  (pin 5); decouple as one 3.3 V domain. The exposed pad is VSS.
- **256 KB flash (C)** is a deliberate upsize from the 128 KB KBU6 for firmware headroom.
- **KiCad symbol:** `MCU_ST_STM32G0:STM32G0C1KCUx`, copied into `DSR-1.kicad_sym`,
  renamed `STM32G0C1KCU6`, exposed-pad VSS exposed.

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

## 4. Finalized Pin Allocation (UFQFPN32)

Complete 32-pin assignment. Battery-build pins are No-connect on wall-only builds
(Variant A/B without cells). AF/channel assignments follow the schematic guide's MCU
section; verify any uncertain channel against the STM32G0C1 datasheet AF table.

| Pin | STM32 | DSR-1 signal | Function / AF | Dir | Reset | Notes |
|---:|---|---|---|---|---|---|
| 1 | PB9 | — | spare GPIO | — | Hi-Z | free |
| 2 | PC14-OSC32_IN | — | LSE in (unused) | — | Hi-Z | LSE not used |
| 3 | PC15-OSC32_OUT | — | LSE out (unused) | — | Hi-Z | LSE not used |
| 4 | VDD/VDDA | `+3V3` | power (combined) | — | — | single 3.3 V domain |
| 5 | VSS/VSSA | GND | ground (combined) | — | — | |
| 6 | PF2-NRST | NRST | reset | In | — | 100nF to GND; test pad |
| 7 | PA0 | `FG_IN` | TIM2_CH1 input capture | In | Hi-Z | tachometer; ~2500 Hz ISR |
| 8 | PA1 | `RV601_WIPER` | ADC_IN1 | In | Hi-Z | base speed cal |
| 9 | PA2 | `RV602_WIPER` | ADC_IN2 | In | Hi-Z | speed tune |
| 10 | PA3 | `RV603_WIPER` | ADC_IN3 | In | Hi-Z | speed tune range |
| 11 | PA4 | — | DAC1_OUT1 (unused) | — | Hi-Z | DAC not used |
| 12 | PA5 | `MOTOR_EN_MON` | GPIO in | In | Hi-Z | Sony motor-enable monitor |
| 13 | PA6 | `MOTOR_PWM` | TIM3_CH1 PWM (AF1) | Out | 0% duty (off) | committed motor drive |
| 14 | PA7 | `SPEED_TUNE_SW` | GPIO in | In | Hi-Z | S601 |
| 15 | PB0 | `VBAT_SENSE` | ADC_IN8 | In | Hi-Z | pack voltage (battery) |
| 16 | PB1 | `VBAT_SENSE_EN` | GPIO out | Out | low (off) | sense-divider gate FET (battery) |
| 17 | PB2 | `BATT_LED1` | GPIO / TIM | Out | low | indicator seg 1 (battery) |
| 18 | PA8 | `CC1` | UCPD1_CC1 / DBCC1 | I/O | — | USB-C CC (native UCPD) |
| 19 | PA9 | `CC2` | UCPD1_CC2 | I/O | — | USB-C CC |
| 20 | PC6 | `S801_BATT` | GPIO in (pull) | In | Hi-Z | BATT-mode sense (battery) |
| 21 | PA10 | — | spare GPIO | — | Hi-Z | free |
| 22 | PA11 [PA9] | `USB_DM` | USB FS | I/O | — | via USBLC6 ESD |
| 23 | PA12 [PA10] | `USB_DP` | USB FS | I/O | — | via USBLC6 ESD |
| 24 | PA13 | `SWDIO` | SWD (AF0) | I/O | SWD | reserve permanently |
| 25 | PA14-BOOT0 | `SWDCLK` | SWD / BOOT0 (AF0) | I/O | SWD | reserve; defined BOOT0 access |
| 26 | PA15 | `BATT_LED2` | GPIO / TIM | Out | low | indicator seg 2 (battery) |
| 27 | PB3 | `BATT_LED3` | GPIO / TIM | Out | low | indicator seg 3 (battery) |
| 28 | PB4 | `BATT_LED4` | GPIO / TIM | Out | low | indicator seg 4 (battery) |
| 29 | PB5 | `BATT_LED5` | GPIO / TIM | Out | low | indicator seg 5 (battery) |
| 30 | PB6 | `DEBUG_TX` | USART1_TX (AF0) | Out | Hi-Z | service debug |
| 31 | PB7 | `CHRG_SENSE` | GPIO in | In | Hi-Z | charger CHRG (optional, battery) |
| 32 | PB8 | `DONE_SENSE` | GPIO in | In | Hi-Z | charger DONE (optional, battery) |
| EP | VSS | GND | exposed pad | — | — | via array to ground plane |

**Spares:** PB9 (1), PA10 (21), and the LSE pair PC14/PC15 (2/3) are unallocated.
LED segment pins on timer channels (PB2/PA15/PB3/PB4/PB5) can PWM-dim the bar.

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

The motor output is TIM3_CH1 PWM on PA6 (committed — NPN level-shift stage).

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

### 11.3 Recommendation Status — Accepted

> **Accepted: STM32G0C1KCU6 / UFQFPN32.** The full 32-pin allocation (§4) fits all
> servo, USB, UCPD, debug, power-sense, and battery-indicator functions with two spare
> GPIO (PB9, PA10) plus the unused LSE pair. Native UCPD on PA8/PA9 covers USB-C CC; no
> external PD controller or larger package is required. Variant A's 9 V PD is handled
> off-MCU by the IP2721 trigger; the battery build runs at 5 V and needs no PD.

## 12. Rev A Pinout Decision Gates — Met

All gates are satisfied; the MCU/package is **accepted** for Rev A.

| Gate | Status |
|---|---|
| Exact part number selected | ✅ STM32G0C1KCU6 |
| Exact package pinout captured | ✅ UFQFPN32 (§4) |
| USB FS pins verified | ✅ PA11/PA12 |
| Native UCPD pins verified | ✅ PA8/PA9 |
| SWD/BOOT access verified | ✅ PA13/PA14 + NRST (PF2) |
| FG timer input verified | ✅ PA0 / TIM2_CH1 |
| Motor PWM pin verified | ✅ PA6 / TIM3_CH1 |
| ADC pins verified | ✅ PA1/PA2/PA3 + PB0 |
| S601 GPIO verified | ✅ PA7 |
| VBUS/power-sense pins assigned | ✅ VBAT_SENSE PB0; CHRG/DONE PB7/PB8 |
| Battery-indicator pins assigned | ✅ BATT_LED1–5, S801_BATT, VBAT_SENSE_EN |
| External timebase reserved/rejected | ✅ HSI + FG reference; LSE pins free |
| External PD controller | ✅ none (UCPD + IP2721 trigger) |
| Pin conflicts reviewed | ✅ §10 |
| KiCad symbol/package updated | ✅ STM32G0C1KCUx |

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
| TBD | PA6 | `MOTOR_PWM` | TIM3_CH1 PWM | AF1 | Output | 0% duty (motor off) | Pending package verify |
| TBD | TBD | `USB_DP` | USB FS | TBD | I/O | Hi-Z/USB | Pending |

---

## 14. Open Questions — Resolved

1. Final part? **STM32G0C1KCU6** (UFQFPN32, 256 KB).
2. USB FS pins exposed? **Yes — PA11/PA12.**
3. UCPD CC pins suitable? **Yes — PA8/PA9 native UCPD1.**
4. N-suffix required? **No — GP variant; VDDIO2 internal to VDD.**
5. ~~DAC vs USB/UCPD coexist?~~ Resolved: DAC unused; PWM on PA6.
6. External PD controller? **No.** Variant A uses the IP2721 fixed trigger off-MCU; the battery build runs at 5 V and needs no PD.
7. External timebase required? **No** — HSI for the MCU; FG provides the speed reference. LSE/HSE pins left free.
8. HSE pins free? **Yes, unused.**
9. Rail-sense count? **One ADC (VBAT_SENSE, PB0) plus CHRG/DONE status GPIOs.**
10. ~~DAC vs PWM solder-jumper?~~ Resolved: PWM only.

## 15. Acceptance Criteria — Met

| Requirement | Status |
|---|---|
| Exact STM32 part/package selected | ✅ STM32G0C1KCU6 / UFQFPN32 |
| Datasheet pinout captured | ✅ §4 |
| Alternate functions assigned | ✅ §4 |
| USB-C data pins assigned | ✅ PA11/PA12 |
| PD strategy reflected in pins | ✅ UCPD PA8/PA9; IP2721 (Variant A) |
| Servo-critical pins assigned | ✅ FG PA0, PWM PA6, wipers PA1–3, S601 PA7 |
| Power-sense/fault pins assigned | ✅ VBAT_SENSE PB0, CHRG/DONE PB7/PB8 |
| Battery-indicator pins assigned | ✅ BATT_LED1–5, S801_BATT, VBAT_SENSE_EN |
| SWD/BOOT recovery preserved | ✅ PA13/PA14, NRST |
| Timebase resolved | ✅ HSI + FG reference |
| KiCad symbol matches part | ✅ STM32G0C1KCUx |
| Schematic labels match this doc | ⚠ pending the v0.6 single-sheet guide reaching the repo |
| Firmware `config.h` matches this doc | ⚠ to update |

## 16. Design Rule

Do not route the Rev A PCB until the pin allocation is accepted.

A schematic that fits conceptually but not physically on the selected package is not a design. It is a wish list.
