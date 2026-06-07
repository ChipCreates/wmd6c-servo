# WM-D6C Servo Replacement (DSR-1) — Project Context Summary
**Purpose:** A current-state snapshot of the DSR-1 project for resuming work in a new chat.
**Repository:** github.com/ChipCreates/wmd6c-servo

---

## The Problem

Sony Walkman WM-D6C (serial SN72795, US model, **main board 1-611-494-12, suffix -12**)
with a failed capstan servo IC. Symptom: playback runs fast — motor uncontrolled.
Root cause: the CX20084 servo IC is dead, most likely from a standard centre-positive
DC adapter plugged into the WM-D6C's non-standard negative-centre DC-IN jack (CN301),
applying reverse polarity directly to the unprotected B+1 rail that feeds the CX20084.

The suffix-12 board is the **CX20084-based "former type"** servo board per service
manual Supplement-4 — i.e. the DSR-1 Rev A primary target.

---

## The Solution

A single open-source module (the **DSR-1**) that replaces three failure-prone parts at
once — the CX20084 servo IC (IC601), the CP304 DC-DC boost converter, and the CN301
power input jack — with a documented, serviceable, component-level design built around
an STM32 running a bare-metal digital PLL servo loop. The reverse-polarity failure mode
is made physically impossible in every build.

The module ships in three configurations sharing one design:
- **Variant A (wall):** USB-C PD negotiated to 9V (IP2721 trigger), bucked to B+1.
- **Variant B (wall):** original barrel jack with polarity-agnostic LTC4359 bridge.
- **Battery-integrated (primary, portable):** two-board design — a power daughter board
  with a rechargeable LiPo pack charged in place over USB-C. See Power, below.

---

## Hardware Architecture

### MCU
**STM32G0C1KCU6** — UFQFPN32 (5×5mm, 0.5mm pitch), 64 MHz Cortex-M0+, **256 KB flash**,
144 KB RAM. GP variant: **VDDIO2 is tied to VDD internally** (no separate VDDIO2 pin),
correct for a single 3.3V system. (Supersedes the earlier KBU6/128 KB selection; the
256 KB C-variant was chosen for headroom. WLCSP52 was rejected vs UFQFPN32 — the 2 mm
size saving was not worth the manufacturability tradeoff.)
- Native USB 2.0 Full Speed device, crystal-less (HSI48 + SOF trimming) — no bridge, no
  crystal anywhere; HSI16 × PLL → 64 MHz.
- Built-in UCPD controller (CC handling) used for PD-contract monitoring in Variant A.
- 12-bit DAC (motor drive), 12-bit ADC (three pot channels), TIM2 input capture (FG).
- Boot to servo loop ~2 ms, well inside the ~300 ms mechanical spin-up.

### Pin Assignments
| Pin | Function | Signal |
|-----|----------|--------|
| PA0 / TIM2_CH1 | FG input capture | FG_IN |
| PA1 / ADC_IN1 | Base speed trim | ADC_RV601 |
| PA2 / ADC_IN2 | Speed tune slider | ADC_RV602 |
| PA3 / ADC_IN3 | Speed range trim | ADC_RV603 |
| PA4 / DAC1_OUT1 | Motor drive output | DAC_MOTOR |
| PA5 / GPIO | Motor enable input | MOTOR_EN |
| PA6 / TIM3_CH1 | PWM output (Option B) | PWM_OUT |
| PA7 / GPIO | S601 speed-tune switch | S601_GPIO |
| PA8 / UCPD1_CC1 | USB-C CC1 (Variant A) | CC1 |
| PB15 / UCPD1_CC2 | USB-C CC2 (Variant A) | CC2 |
| PA9 / USART1_TX | Debug UART (test pad) | UART_TX |
| PA11 / USB_DM | USB D− | USB_DM |
| PA12 / USB_DP | USB D+ | USB_DP |
| PA13 / SWDIO | SWD programming | SWDIO |
| PA14 / SWDCLK | SWD programming | SWDCLK |
| (battery build) ADC | VBAT_SENSE from daughter board | ADC_VBAT |
| (battery build) GPIO | BQ24074 CHG/PGOOD status | CHG_STAT |
| (battery build) GPIO | S801 BATT-position mode sense | S801_SENSE |
| (battery build) GPIO/PWM ×5 | LED bar fuel gauge drive | D801–D805 |

### Power Architecture
A **TPS63070 buck-boost generates the 6.0V B+1 rail in every build**; everything
downstream of B+1 is identical:
- **MT3608** synchronous boost: B+1 6V → **B+3 10.8V** motor rail (replaces CP304).
- **MCP1700** LDO: B+1 6V → **3.3V** for the STM32.

What differs is the front end:
- **Variant A:** USB-C 9V PD (IP2721) → TPS63070 *bucks* to 6V → harness drives B+1.
- **Variant B:** barrel jack ±polarity → LTC4359 ideal-diode bridge + polyfuse +
  SMBJ7.0A TVS + 220µF bulk → TPS63070 → harness drives B+1.
- **Battery-integrated (two-board):**
  - **Power daughter board (behind the battery bay):** USB-C 5V (no PD) →
    **BQ24074 power-path charger** (DPPM, supplement mode; R_ISET≈1.1k ≈0.8A,
    R_ILIM≈800Ω ≈2A) → **3× 803040 LiPo in 3P** (3.7V, 3000mAh / 11.1Wh) behind
    **DW01 + FS8205 dual-FET** protection → **TPS63070 boost** to 6.0V.
  - The daughter board **injects 6V at the original battery-terminal node**, ahead of
    the **S901 power switch** (so S901 still commands the machine). B+1 propagates
    through the machine's B+1 net and reaches the module via the CP304 harness.
  - In this build the **module's power-input zone is unpopulated** — the module
    *receives* B+1 (J1 pin 7 is input), it does not generate it.
  - **Board-to-board link:** B+1, GND, VBAT_SENSE (→ STM32 ADC), CHG/PGOOD (→ STM32
    GPIO), optional USB D+/D−.
  - Cells plug into the daughter board (service disconnect); **balance-match within
    ~30–50 mV before first parallel join.**
  - Runtime ≈ **3–4 h** per charge at realistic ~2.5–3W draw; charge-in-place over USB-C.
  - **No USB PD** in this build — BQ24074 charges from plain 5V VBUS; 9V would over-volt it.

Chemistry/charger/protection note: this supersedes the earlier 1S4P IFR14500 **LFP**
pack with CN3058E + HY2112-CB. LiPo was chosen because the cells exist off-the-shelf
with JST pigtails and protection in the required form factor; with LiPo, the DW01 (a
Li-ion-threshold part) is now the *correct* protection IC.

### Battery cavity (measured)
~57 × 32 × 29 mm (≈52 cm³); lid-to-holder-bottom depth 1 1/8" ≈ 28.6 mm. Three 803040
cells stack flat at 40 × 30 × 24 mm, leaving ~4.6 mm swell clearance on the stacking
axis (must be left empty).

### FG Level Shifter (FG901 → PA0)
- FG901 is a GP2S22AB optical interrupter, open-collector output; pull-up on the main
  board. Divider R3/R4 scales the swing to ≤3.3V — **values pending bench measurement**
  of the actual swing. BAT54 Schottky clamps to +3V3/GND; small C0G HF filter.

### Motor Output Stage — **pending bench measurement** (populate one option)
- **Option A — DAC direct** (if Q601 base ≤ 3.3V): PA4 DAC → series R → Q601 base;
  pull-up to +3V3 (PNP base high = motor OFF at boot).
- **Option B — PWM + RC + NPN level shift** (if Q601 base > 3.3V): PA6 PWM → RC →
  NPN (MMBT3904) → Q601 base; pull-up to B+1-referenced node.

### USB-C
- USB-C receptacle; CC1/CC2 → PA8/PB15 (UCPD1) in Variant A; D+/D− → PA12/PA11 through
  a USBLC6-2SC6Y ESD array; VBUS bypass.

### WM-D6C Interface Connector J1 (8-position 1.25 mm JST)
| Pin | Net | Direction | WM-D6C Connection |
|-----|-----|-----------|-------------------|
| 1 | FG_RAW | Machine → Module | FG901 optical sensor output |
| 2 | Q601_BASE | Module → Machine | CX20084 pin 15 solder pad (motor drive) |
| 3 | MOTOR_EN | Machine → Module | IC601 pin 7 net (Auto-Off board) |
| 4 | RV601_WIPER | Machine → Module | Base speed trim wiper |
| 5 | RV602_WIPER | Machine → Module | Speed-tune slider wiper |
| 6 | RV603_WIPER | Machine → Module | Speed range trim wiper |
| 7 | B+1 / VBATT | Machine ↔ Module | 6V rail — module drives it (wall variants) / receives it (battery build) |
| 8 | GND | Shared | Chassis ground |

### Board
- KiCad schematic capture in progress (power input and power management sheets done;
  MCU sheet in progress). Two-layer, ENIG, 1oz, 1.6mm FR4.

---

## WM-D6C Original Circuit — Critical Facts

### Components Removed
| Component | What it is | Why removed |
|-----------|-----------|-------------|
| CX20084 (IC601) | Capstan servo IC | Dead — replaced by STM32 digital servo |
| X701 34.7 kHz crystal | Servo reference oscillator | Auto-Off board — no longer needed |
| IC701 MSM58141RS | Crystal divider/phase comparator | Auto-Off board — no longer needed |
| CP304 | DC-DC boost (6V→10.8V) | **Definitively replaced by MT3608** on the module |
| CN301 | DC-IN barrel jack (negative centre!) | Opening repurposed for USB-C / Variant B input |
| Battery spring terminals | Stock AA contacts | Removed in battery build (≥1 corroded from past leakage); daughter board feeds 6V here |

### Components Kept
- **Q601 2SB733** — PNP motor-current driver — **TEST in diode mode before proceeding**
  (B-E and B-C ~0.6–0.7V; if failed, replace 2SA1015 / BC327-40 / MMBT3906). PNP
  convention: base LOW = motor ON harder. PI sign: positive error (too slow) → lower
  base voltage / more drive.
- **Q703 / Q704** — motor switching transistors — untouched.
- **FG901 GP2S22AB** — optical speed sensor — stays.
- **RV601 / RV602 / RV603** — base-speed trim / Speed-Tune slider / range trim — stay
  wired to J1 pins 4/5/6.
- **S601** — Speed-Tune switch → PA7.
- **D801–D805 / IC801 (CX10043) / Q801 / S801** — front-panel LED bar; re-referenced by
  the STM32 as a 1–5 segment **LiPo fuel gauge** in BATT mode (battery build).

### Motor Enable
- IC601 pin 7 net held ~4.4V during play by R605; Q702 (Auto-Off) pulls LOW at
  end-of-tape. **Requires a divider before PA5** (~10k/22k → ~3.0V).

---

## Servo Algorithm

- **FG target:** test tape (3 kHz at LINE OUT = correct speed); FG901 pulse rate at
  correct speed ≈ 2000–3000 Hz — **measure on bench** (FG_TARGET_HZ pending).
  TARGET_PERIOD = 64,000,000 / FG_TARGET_HZ ticks at 64 MHz.
- **FG measurement:** TIM2 input capture on PA0, ISR computes period in ticks (zero CPU
  overhead). Loop runs ~2500 Hz.
- **Control:** **Q16 fixed-point PI** (no float, no HAL, no RTOS, no malloc). PNP
  convention — longer period (too slow) decreases output. Integral clamped (anti-windup);
  output clamped to drive limits. Starting constants tuned live on the bench.
- **Speed tune:** three ADC channels — RV601 base trim, RV602 user slider (±freq
  offset), RV603 scales RV602's range.
- **Live tuning:** USB CDC virtual COM — adjust KP/KI, print telemetry, save to flash,
  reset. Telemetry includes FG period, target, error, integral, DAC value, pot ADCs.
- **Digital measurement floor:** ~0.0039% WRMS per TIM2 tick (15.6 ns) — not the W&F
  bottleneck; the mechanical transport is.

---

## Bench Measurements Still Pending (on the physical unit)

1. Q601 base voltage range → Option A vs B output stage
2. FG901 signal swing → R3/R4 divider values (or omit)
3. Pot wiper voltage ranges (confirm ≤3.3V)
4. FG_TARGET_HZ at correct speed
5. CP304 cavity dimensions (module fit)
6. Q601 continuity (diode mode)
7. Battery-build: confirm S801 BATT reroute / CX10043 high-Z behaviour, and LED forward
   characteristics through the 180Ω limiters

---

## Physical Restoration (sequenced before electrical diagnostics)

Mechanical → electrical → firmware tuning. Liberty watch oil + rubber restoration kit
(flywheel tire, pinch roller) on order. Capstan bearing lubrication, flywheel tire,
pinch roller replacement, then servo diagnostics. Battery-terminal corrosion cleaning
(white vinegar → IPA → fiberglass scratch pen / brass brush) during the initial pass.

**W&F targets:** ≤0.05% WRMS on a mechanically sound machine; ceiling ~0.03% WRMS after
full mechanical + firmware optimization.

---

## Tooling & Licensing

- KiCad (schematic), OWON HDS272S (70 MHz handheld scope), ImageMagick (large-format
  service-manual printing: `-adaptive-sharpen 0x1.5` after resize).
- Sony WM-D6C / TC-D6C service manual ver 1.1 — primary reference.
- **Licenses (maximally permissive):** CERN OHL-P v2 (hardware), MIT (firmware),
  CC BY 4.0 (documentation).
