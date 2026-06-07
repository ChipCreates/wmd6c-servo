# WM-D6C Servo Replacement (DSR-1) — Project Context Summary
**Purpose:** A current-state snapshot of the DSR-1 project for resuming work in a new chat.
**Repository:** github.com/ChipCreates/wmd6c-servo

---

## The Problem

Sony Walkman WM-D6C (serial SN72795, US model, **observed PCB marking C11-494-12**)
with a failed surface-mount CX20084 capstan servo IC at IC601. Symptom: playback runs
fast — motor uncontrolled.
Root cause: the CX20084 servo IC is dead, most likely from a standard centre-positive
DC adapter plugged into the WM-D6C's non-standard negative-centre DC-IN jack (CN301),
applying reverse polarity directly to the unprotected B+1 rail that feeds the CX20084.

The C11-494-12 board is a **surface-mount CX20084-based "former type"** servo board
and the DSR-1 Rev A primary target. Earlier through-hole-board assumptions are
superseded by the actual unit inspection.

---

## The Solution

An open-source two-board subsystem (the **DSR-1**) that replaces three failure-prone
areas at once — the CX20084 servo IC (IC601), the CP304 DC-DC boost converter, and
the CN301/power-input risk — with a documented, serviceable, component-level design
built around an STM32 running a bare-metal digital PLL servo loop. The reverse-polarity
failure mode is made physically impossible in every build.

The design is split conceptually for manufactureability:
- **Servo Control Board:** STM32, CX20084 replacement logic, motor/servo control,
  audio-related integration, USB data/service, debug, and the machine harness.
- **Power Board:** USB-C input, charger, LiPo/protection, power regulation, and
  battery telemetry.

The project supports three power configurations:
- **Variant A (wall):** USB-C PD negotiated to 9V (IP2721 trigger), bucked to B+1.
- **Variant B (wall):** original barrel jack with polarity-agnostic LTC4359 bridge.
- **Battery-integrated (primary, portable):** two-board design — a Power Board
  with a rechargeable LiPo pack charged in place over USB-C. See Power, below.

---

## Hardware Architecture

### MCU
**STM32G0C1KCU6** — UFQFPN32 (5×5mm, 0.5mm pitch), 64 MHz Cortex-M0+, **256 KB flash**,
144 KB RAM. GP variant has no separate VDDIO2 pin, correct for a single 3.3V system.
(Supersedes the earlier STM32G0B1KBU6/128 KB selection; the 256 KB C-variant was
chosen for headroom. WLCSP52 was rejected vs UFQFPN32 — the 2 mm size saving was not
worth the manufacturability tradeoff.)
- Native USB 2.0 Full Speed device, crystal-less (HSI48 + SOF trimming) — no bridge, no
  crystal anywhere; HSI16 × PLL → 64 MHz.
- Built-in UCPD controller (CC handling) used for PD-contract monitoring in Variant A.
- TIM3 PWM motor drive, 12-bit ADC inputs, TIM2 input capture (FG), USB FS, and UCPD.
- Boot to servo loop ~2 ms, well inside the ~300 ms mechanical spin-up.

### Pin Assignments
| Pin | Function | Signal |
|-----|----------|--------|
| PA0 / TIM2_CH1 | FG input capture | FG_IN |
| PA1 / ADC_IN1 | Base speed trim | ADC_RV601 |
| PA2 / ADC_IN2 | Speed tune slider | ADC_RV602 |
| PA3 / ADC_IN3 | Speed range trim | ADC_RV603 |
| PA4 / DAC1_OUT1 | No-connect / spare; DAC not used for motor drive | NC |
| PA5 / GPIO | Motor enable input | MOTOR_EN |
| PA6 / TIM3_CH1 | PWM motor output | MOTOR_PWM |
| PA7 / GPIO | S601 speed-tune switch | S601_GPIO |
| PA8 / UCPD1_CC1 | USB-C CC1 (Variant A) | CC1 |
| PA9 / UCPD1_CC2 | USB-C CC2 (Variant A) | CC2 |
| PA11 / USB_DM | USB D− | USB_DM |
| PA12 / USB_DP | USB D+ | USB_DP |
| PA13 / SWDIO | SWD programming | SWDIO |
| PA14 / SWDCLK | SWD programming | SWDCLK |
| PB6 / USART1_TX | Debug UART (test pad) | DEBUG_TX |
| PB0 / ADC_IN8 | VBAT_SENSE from Power Board | VBAT_SENSE |
| PB1 / GPIO | Battery sense-divider enable | VBAT_SENSE_EN |
| PC6 / GPIO | S801 BATT-position enable sense | S801_BATT |
| PB7 / GPIO | BQ24074 charge status | CHG_STAT |
| PB8 / GPIO | BQ24074 power-good status | PGOOD |
| PB2, PA15, PB3, PB4, PB5 | LED bar fuel-gauge drive | BATT_LED1–5 / D801–D805 |

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
  - **Power Board (behind the battery bay):** USB-C 5V (no PD) →
    **BQ24074 power-path charger** (DPPM, supplement mode; R_ISET≈1.1k ≈0.8A,
    R_ILIM≈800Ω ≈2A) → **3× 803040 LiPo in 3P** (3.7V, 3000mAh / 11.1Wh) behind
    **DW01 + FS8205 dual-FET** protection → **TPS63070 boost** to 6.0V.
  - The Power Board **injects 6V at the original battery-terminal node**, ahead of
    the **S901 power switch** (so S901 still commands the machine). B+1 propagates
    through the machine's B+1 net and reaches the Servo Control Board via the CP304 harness.
  - In this build the Servo Control Board's local wall-input power zone is unpopulated — it
    *receives* B+1 (J1 pin 7 is input), it does not generate it.
  - **Board-to-board link:** B+1, GND, VBAT_SENSE (→ STM32 ADC), CHG_STAT and PGOOD
    (→ STM32 GPIO), optional USB D+/D−.
  - Cells plug into the Power Board (service disconnect); **balance-match within
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

### Motor Output Stage — **committed topology; values pending bench measurement**
PA6 PWM → RC filter → NPN level shift (MMBT3904) → Q601 base; pull-up to a
B+1-referenced safe-off node. Direct DAC drive is not used. Q601 exact part/package,
base-voltage range, R9 sizing, and PWM-to-speed sign remain pending bench verification
on the actual C11-494-12 board.

### USB-C
- USB-C receptacle; CC1/CC2 → PA8/PA9 (UCPD1) in Variant A; D+/D− → PA12/PA11 through
  a USBLC6-2SC6Y ESD array; VBUS bypass.

### WM-D6C Interface Connector J1 (8-position 1.25 mm JST)
| Pin | Net | Direction | WM-D6C Connection |
|-----|-----|-----------|-------------------|
| 1 | FG_RAW | machine → Servo Control Board | FG901 optical sensor output |
| 2 | Q601_BASE | Servo Control Board → machine | CX20084 pin 15 solder pad (motor drive) |
| 3 | MOTOR_EN | machine → Servo Control Board | IC601 pin 7 net (Auto-Off board) |
| 4 | RV601_WIPER | machine → Servo Control Board | Base speed trim wiper |
| 5 | RV602_WIPER | machine → Servo Control Board | Speed-tune slider wiper |
| 6 | RV603_WIPER | machine → Servo Control Board | Speed range trim wiper |
| 7 | B+1 / VBATT | Machine ↔ Module | 6V rail — module drives it (wall variants) / receives it (battery build) |
| 8 | GND | Shared | Chassis ground |

### Board
- KiCad schematic capture is in-flight and still behind this status document. The
  committed `hardware/kicad/DSR-1` project should be treated as a legacy working file
  until it is manually split into the Power Board and Servo Control Board projects.
  Two-layer, ENIG, 1oz, 1.6mm FR4 remains the assumed board stack unless revised.

---

## WM-D6C Original Circuit — Critical Facts

### Components Removed
| Component | What it is | Why removed |
|-----------|-----------|-------------|
| CX20084 (IC601) | Capstan servo IC | Dead — replaced by STM32 digital servo |
| X701 34.7 kHz crystal | Servo reference oscillator | Auto-Off board — no longer needed |
| IC701 MSM58141RS | Crystal divider/phase comparator | Auto-Off board — no longer needed |
| CP304 | DC-DC boost (6V→10.8V) | **Definitively replaced by MT3608** on the Servo Control Board |
| CN301 | DC-IN barrel jack (negative centre!) | Opening repurposed for USB-C / Variant B input |
| Battery spring terminals | Stock AA contacts | Removed in battery build (≥1 corroded from past leakage); Power Board feeds 6V here |

### Components Kept
- **Q601** — motor-current driver on the WM-D6C main board — **TEST in diode mode and
  identify package/marking before proceeding**. The exact part/package on this
  surface-mount board is pending physical confirmation. PNP convention remains the
  working model until bench measurement supersedes it: base LOW = motor ON harder.
  PI sign: positive error (too slow) → lower base voltage / more drive.
- **Q703 / Q704** — motor switching transistors — untouched.
- **FG901 GP2S22AB** — optical speed sensor — stays.
- **RV601 / RV602 / RV603** — base-speed trim / Speed-Tune slider / range trim — stay
  wired to J1 pins 4/5/6.
- **S601** — Speed-Tune switch → PA7.
- **D801–D805 / IC801 (CX10043) / Q801 / S801** — front-panel LED bar and VU/battery
  switch. `S801_BATT` is routed to the STM32 as the BATT-position enable flag. The
  STM32 may drive `BATT_LED1–5` as a 1–5 segment LiPo fuel gauge only while
  `S801_BATT` is asserted; in VU/non-BATT operation the LED outputs must be released
  or high-Z so they do not contend with the original CX10043/VU circuitry.

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
  reset. Telemetry includes FG period, target, error, integral, PWM/output value,
  battery status where populated, and pot ADCs.
- **Digital measurement floor:** ~0.0039% WRMS per TIM2 tick (15.6 ns) — not the W&F
  bottleneck; the mechanical transport is.

---

## Bench Measurements Still Pending (on the physical unit)

1. Q601 exact package/marking and base-voltage range → confirm R9 sizing and PWM sign
2. FG901 signal swing → R3/R4 divider values (or omit)
3. Pot wiper voltage ranges (confirm ≤3.3V)
4. FG_TARGET_HZ at correct speed
5. CP304 cavity dimensions (module fit)
6. Q601 continuity (diode mode)
7. Battery-build: confirm S801 BATT reroute, `S801_BATT` polarity, CX10043 high-Z
   behaviour in BATT mode, and LED forward characteristics through the 180Ω limiters

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
