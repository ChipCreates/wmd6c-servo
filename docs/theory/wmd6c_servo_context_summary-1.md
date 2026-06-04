# WM-D6C Servo Replacement (DSR-1) — Project Context Summary

**Purpose:** Single-file project state for continuing work in a new chat.
**Last refreshed:** reflects current project files (KiCad schematic in progress,
firmware prototype, Rev A review findings) plus bench/restoration status.

> ⚠ **Source-conflict notes** are flagged inline with `⚠`. The project files were
> written at different times and disagree on a few points (power regulator topology,
> barrel-jack protection, MCU package, licenses). Where flagged, confirm which is
> canonical and this doc will be finalized.

---

## 1. Project Identity & Scope

**DSR-1** — an open-source replacement module for the Sony WM-D6C capstan drive.
A single compact PCB that replaces **three** original components at once:

1. **CX20084** (IC601) — the dead capstan servo IC
2. **CP304** — the potted 6V→10.8V DC-DC boost converter
3. **CN301** — the non-standard negative-centre DC input jack

Root-cause failure: a standard centre-positive adapter plugged into CN301 applied
reverse polarity to the B+1 rail and destroyed the unprotected CX20084.

- **Repo:** github.com/ChipCreates/wmd6c-servo
- **Licenses:** ⚠ *conflict.* Memory/intent = **maximally permissive**: CERN
  OHL-**P** v2 (hardware), MIT (firmware), CC BY 4.0 (docs). Repo files/datasheet
  currently say CERN OHL-**S** v2 (hardware), MIT, CC BY-**SA** 4.0. Decide and make
  consistent (LICENSE_HW/FW/DOCS files + both READMEs — see firmware finding #9).

---

## 2. Target Machine

- **Unit:** WM-D6C, serial 72795, US model.
- **Main board: 1-611-494-12 (suffix -12)** — confirmed from board silkscreen.
- This is a **"former type" CX20084 servo board** per service manual Supplement-4
  (ECN-WMA00831). Former types: 1-611-494-11, -12 and 1-651-545-12. New type
  (1-651-545-14) uses a CX-069A servo IC + five-transistor drive (Q601–Q605) — a
  different circuit, **out of scope for Rev A**.
- Symptom: playback runs fast (motor uncontrolled) → CX20084 dead.

**Compatibility scope (DSR-1):** full WM-D6C/TC-D6C run, two servo families —
**Ver. 1.0 (CX20084, single PNP Q601)** is the Rev A target; Ver. 1.1 (CX-069A,
Q601–Q605) is a planned future variant. Identify by reading IC601: CX20084 → proceed.

---

## 3. Status at a Glance

**Done / settled**
- MCU finalized: STM32G0B1KCU6.
- Board identity confirmed (suffix -12, former-type CX20084).
- Power input + power management schematic sheets instantiated in KiCad.
- Firmware prototype written (servo loop, ADC, USB CDC, flash skeleton).
- Block diagram + module datasheet drafted.

**In progress**
- MCU schematic sheet (STM32G0C1 + decoupling).
- Power regulator topology change (buck → buck-boost) — see §5.2.

**Pending**
- Signal-conditioning schematic sheet (currently an empty stub — FG conditioning +
  motor output the firmware already depends on).
- Connectors sheet (J1 harness, J2 SWD, S601 rewire, test pads).
- Mechanical restoration (parts on order) → then electrical bench bring-up.
- Firmware Rev A remediation (see §7.4) and bench tuning.

---

## 4. Hardware — MCU

**STM32G0B1KCU6** — UFQFPN-32, 64MHz Cortex-M0+, 256kB flash, 144kB RAM.
- Native USB 2.0 FS (no bridge chip); crystal-less via internal 48MHz RC + SOF trim.
- HSI16 × PLL → 64MHz, no external crystal. (Timebase is HSI16-only and acknowledged
  as **development-grade** — see docs/hardware/timebase-decision.md.)
- Boot ~2ms to servo running (well inside ~300ms mechanical spin-up).

**Selection rationale**
- **256kB flash (C variant)** — deliberate headroom upsize from the original 128kB.
  Lineage: KBT6 (LQFP/128k) → KBU6 (UFQFPN/128k) → **KCU6** (UFQFPN/256k, final).
  ⚠ Datasheet/block-diagram files still say KBU6 — stale, update to KCU6.
- **GP variant** (not N): GP/N is about VDDIO2 *exposure*, not package. GP ties
  VDDIO2 to VDD internally — correct for a single 3.3V system.
- **UFQFPN-32 over WLCSP52** — ~2mm saving not worth the manufacturability tradeoff.
- **KiCad symbol:** `STM32G0C1KCUx` confirmed correct/pin-compatible.

---

## 5. Hardware — Power

All builds produce the same three rails to everything downstream:
**B+1 = 6.0V** (machine), **B+3 = 10.8V** (motor, via MT3608), **3.3V** (MCU, via MCP1700).
Two input front ends share one PCB gerber (variant-specific parts are DNP).

### 5.1 Current schematic state (per power-rev-a-findings "what exists today")
- **Variant A — USB-C PD:** J_USB (USB-C), **IP2721** PD sink trigger (9V PDO, no
  firmware needed; falls back to 5V = won't run = under-voltage protection),
  **USBLC6-2SC6Y** ESD on D+/D−. Output net 9V_PD.
- **Variant B — barrel jack:** retains CN301-style jack; **4-diode Schottky full
  bridge** for polarity-agnostic input; polyfuse F1 (500mA hold); **SMBJ-series TVS**
  clamp. (Schottky bridge ~0.7V drop is harmless once the buck-boost is in — see 5.2.)
- **Shared B+1 regulator:** currently **AP63203WU-7** — ⚠ a *fixed buck* (wrong;
  can't reach 6V from 5/6V in). **Required change → TPS63070 buck-boost** (2–16V in,
  0.8V FB, 2A; FB divider 649k/100k carries over for 6.0V). Single inductor sits
  between the TPS63070 L1/L2 switch pins.
- **B+3:** **MT3608** boost → 10.8V (replaces CP304). SS14 catch diode; FB 169k/10k;
  47µF/16V output cap.
- **3.3V:** **MCP1700-3302** LDO; 1µF ceramic + 10µF bulk each side; 4× 100nF at MCU.

### 5.2 Pending core change
Replace the buck with the **TPS63070 buck-boost** so B+1 holds 6.0V across 5/6/9V and
either polarity; keep the Schottky bridge. Open decision: add an OVP load-switch/eFuse
if a wrong high-voltage adapter (e.g. 19V laptop brick) is considered plausible.

### 5.3 ⚠ Superseded / uncertain power notes
- Earlier docs describe an **LTC4359 ideal-diode MOSFET bridge** (power-supply-design.md;
  kicad-guide commit text) and a **TPS62xx fixed buck** (memory/datasheet). Current
  direction is **Schottky bridge + TPS63070** — confirm the LTC4359/TPS62xx text is dead.
- power-supply-design.md also explores an **optional battery-integrated front end**
  (USB-C 5V → CN3058E charger → 1S4P LiFePO4 pack → load-share). Status unclear —
  confirm whether this is in scope for Rev A or future.

---

## 6. Hardware — Signal Chain, Pins, Connector, Board

### 6.1 FG conditioning (FG901 → PA0)
- FG901 = GP2S22AB optical sensor, open-collector; pull-up on machine board.
- Divider R3 10kΩ series / R4 22kΩ to GND → ~3.44V from 5V swing.
  **⚠ Measure actual FG swing on bench before connecting; recompute R4 if needed.**
- BAT54 Schottky clamps to +3V3 and GND; 10pF C0G HF filter.

### 6.2 Motor output stage — Option A vs B (JP1 solder-bridge selects)
- **Option A — DAC direct** (if Q601 base ≤3.3V): PA4 DAC → R5 10kΩ → Q601 base;
  R6 100kΩ pull-up to +3V3 (PNP off at boot).
- **Option B — PWM + RC + NPN level shift** (if Q601 base >3.3V): PA6 PWM (TIM3_CH1)
  → R7 1kΩ → RC node (C8 100nF) → R8 10kΩ → Q_LS (MMBT3904) → Q601 base;
  R9 100kΩ pull-up to B+1.
- **Firmware currently commits to Option B nodes** (PA6 PWM, Q_LS, R7/C8/R9).
- ⚠ Motor-drive **sign convention** is flagged as internally contradictory and
  unverified (firmware finding #5) — resolve before driving a real motor.

### 6.3 Pin assignments (STM32G0B1K, UFQFPN-32)
| Pin | Function | Net |
|-----|----------|-----|
| PA0 / TIM2_CH1 | FG input capture | FG_IN (⚠ AF: PA0 TIM2_CH1 is **AF2**, fix in code) |
| PA1/2/3 / ADC | Speed pots | RV601 / RV602 / RV603 |
| PA4 / DAC1_OUT1 | Motor drive (Option A) | DAC_MOTOR |
| PA5 / GPIO | Motor enable monitor | MOTOR_EN |
| PA6 / TIM3_CH1 | PWM (Option B) | MOTOR_PWM |
| PA7 / GPIO | S601 speed-tune switch | SPEED_TUNE_SW |
| PA8 / PB15 (UCPD1) | CC1 / CC2 monitor | CC1 / CC2 |
| PA9 / USART1_TX | Debug UART (test pad) | UART_TX |
| PA11 / PA12 | USB D− / D+ | USB_DM / USB_DP |
| PA13 / PA14 | SWD | SWDIO / SWDCLK |

### 6.4 Machine interface connector
**J1 — 8-pin 1.25mm JST PH** (was a 2.54mm header in older notes; now JST PH).
Carries: FG_RAW, motor drive, RV601/2/3 wipers, motor-enable monitor, S601, B+1 (6V), GND.
Plus **J_SWD** 1×4 2.54mm header and optional UART test pad.

### 6.5 KiCad schematic (KiCad 10, 6-sheet hierarchy)
1. Top level (architecture map)
2. Power input zone — Variant A (USB-C/IP2721) + Variant B (barrel) ✅ instantiated
3. Power management — MT3608 boost + MCP1700 LDO ✅ instantiated
4. STM32G0C1 + decoupling — in progress
5. Signal conditioning — FG divider/clamp, NPN motor drive, ADC — **empty stub**
6. Connectors — J1 harness, J_SWD, S601 rewire, test pads — pending

Layout priorities (post-ERC): STM32 decoupling <0.5mm; MT3608 SW loop tight (SW ≤3mm);
USB diff pair 90Ω ≤25mm away from MT3608 SW; B+3 traces ≥0.5mm for 500mA; GND pour both
layers + via array under STM32 VSS pad; analog zone opposite side from MT3608.

---

## 7. Firmware

### 7.1 Architecture
Bare-metal C, register-level (no HAL/LL/RTOS/malloc). Q16 fixed-point throughout
(Cortex-M0+ has no FPU). Servo runs in the **TIM2 input-capture ISR**, once per FG
edge — **~2500 Hz** at correct speed (vs ~34 Hz effective for the original crystal
divider). TIM2 free-running 32-bit @64MHz → 15.6ns tick. Target period ≈ 25,600 ticks
(64e6 / 2500). Resolution ≈ 0.004%.

### 7.2 PI control law (period domain, Q16)
```c
void TIM2_IRQHandler(void) {
    uint32_t now    = TIM2->CCR1;
    uint32_t period = now - last_capture;
    last_capture    = now;

    // +error = running slow (period too long). PNP Q601: more drive = lower output.
    int32_t error = (int32_t)period - (int32_t)target_period;
    integral += error;
    if (integral >  INTEGRAL_LIMIT) integral =  INTEGRAL_LIMIT;
    if (integral < -INTEGRAL_LIMIT) integral = -INTEGRAL_LIMIT;

    int32_t output = DAC_CENTER
                   - ((KP_Q16 * error)    >> 16)
                   - ((KI_Q16 * integral) >> 16);
    if (output < 0)    output = 0;
    if (output > 4095) output = 4095;
    DAC1->DHR12R1 = (uint32_t)output;
    TIM2->SR &= ~TIM_SR_CC1IF;
}
```
Starting gains: Kp 0.15, Ki 0.008 → **KP_Q16 = 9830, KI_Q16 = 524** (simulation
placeholders; bench-tune via USB CDC, then save to last 4KB flash sector).
ADC scan (DMA) reads RV601/2/3; RV602 = user speed offset, RV603 = its range.
USB CDC live tuning: `p+/p-`, `i+/i-`, `t` (telemetry), `s` (save), `r` (reset).

### 7.3 Repository layout
`hardware/{kicad,gerbers,bom,datasheets}` · `firmware/{src,include,build,releases}`
(main.c, servo.c, adc.c, usb_cdc.c, ucpd.c, flash.c; config.h holds all tunables;
Makefile uses arm-none-eabi-gcc, no IDE) · `docs/{installation,theory,variants,
troubleshooting,review,hardware}` · CHANGELOG + LICENSE files.

### 7.4 Rev A review findings (status: prototype, not bench-validated)
High: (1) **does not build** — missing startup file + unvendored CMSIS;
(2) USB CDC **PMA access uses wrong USB-IP generation** for STM32G0 (companion:
usb-pma-stm32g0-port.md); (3) **pot/trim path is dead code** (adjusted-target never called).
Medium: (4) settings flash address vs linker reservation off by one page;
(5) **motor-drive sign convention contradictory + unverified** (companion doc).
Low/cleanup: (6) unused servo APIs; (7) PA0/PA6 AF numbers (PA0 = AF2); (8) ADC
calibrates before ADC clock selected; (9) Makefile `-DUSE_FULL_LL_DRIVER` + license
filename inconsistency; (10) telemetry-struct atomicity overstated.
Remediation order: build → USB PMA + trim path → motor sign → settings page/PA0 AF → cleanup.

---

## 8. WM-D6C Original Circuit — Facts

### Removed (replaced by module)
CX20084 (IC601) servo · CP304 DC-DC boost (→ MT3608) · CN301 jack (→ Variant A USB-C,
or retained+protected in Variant B) · X701 34.7kHz crystal + IC701 MSM58141RS
(reference/divider on Auto-Off board — no longer needed).

### Retained & driven/read by module
- **Q601 — PNP motor current transistor.** ⚠ Part conflict: notes/datasheet say
  **2SB733**, KiCad guide says **2SB1013** — reconcile. **Diode-test before bring-up**
  (may have been damaged in the same reverse-polarity event); sub: 2SA1015/BC327/MMBT3906.
- Q703/Q704 motor switching (untouched) · FG901 GP2S22AB optical sensor ·
  RV601 47k base-speed trim · RV602 20k speed-tune slider · RV603 47k range trim · S601.

### Motor drive chain (original, 1-611-494-12)
CX20084 pin 15 → resistor net → Q601 base (PNP) → Q601 collector → M901;
CP304 10.8V → Q704 switching → M901 other terminal. Module replaces the pin-15 drive;
Q704/mode switching untouched.

### Motor-enable signal
IC601 pin 7 net held ~4.4V in playback (R605), pulled low at end-of-tape by Q702 on the
Auto-Off board. At battery voltage → needs 10k/22k divider to PA5 (4.4×22/32 ≈ 3.03V).

---

## 9. Physical Restoration (sequence: mechanical → electrical → firmware)

On order: **Liberty watch oil**; **rubber kit** (new flywheel tire + pinch roller).
Mechanical pass first: capstan bearing lube, flywheel tire, pinch roller replacement,
clean FG901 slotted disc with IPA. Battery-terminal corrosion: white vinegar → IPA →
mechanical abrasion (fiberglass scratch pen / brass brush). Condition assessment before
any servo measurements.

---

## 10. Performance Targets

- Realistic W&F: **≤0.05% WRMS** on a mechanically sound machine; **ceiling ~0.03% WRMS**
  after full mechanical + firmware optimization.
- Digital measurement floor ≈ **0.0039% WRMS** per TIM2 tick (15.6ns) — *not* the
  bottleneck; the **mechanical transport is**.

---

## 11. Bench Measurements Pending (on the physical unit)

1. Q601 base voltage range → Option A vs B
2. FG901 signal swing → R3/R4 (or omit divider)
3. CP304/B+3 health (~10.8V on battery) — though CP304 is being replaced regardless
4. Q601 continuity (diode mode)
5. IC601 pin-7 net voltage in playback (~4.4V) → divider sizing
6. Pot wiper voltage ranges (stay <3.3V)
7. FG_TARGET_HZ (→ TARGET_PERIOD); CP304 cavity dimensions for the PCB

---

## 12. Open Questions to Confirm (consolidated)
1. **Licenses:** OHL-P + CC BY (permissive, per intent) vs OHL-S + CC BY-SA (repo files)?
2. **Power:** confirm Schottky bridge + TPS63070 buck-boost is canonical (kill LTC4359 /
   TPS62xx / AP63203 references). Is the battery-integrated front end in Rev A scope?
3. **MCU:** propagate KCU6 (256k) into datasheet + block diagram (still say KBU6).
4. **Q601 part:** 2SB733 vs 2SB1013.
5. **Connector:** confirm J1 8-pin 1.25mm JST PH (older notes: J4 2.54mm header).

---

## 13. Tools
KiCad 10 (schematic) · STM32G0B1KCU6 · OWON HDS272S (70MHz 2-ch handheld scope) ·
ImageMagick (schematic PDF prep; `-adaptive-sharpen 0x1.5` after resize) · GitHub
(ChipCreates) · Sony WM-D6C/TC-D6C service manual ver 1.1 + fb4872.pdf (Ver 1.0 ref).
