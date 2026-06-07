# DSR-1 Servo Module — KiCad 10 Schematic Guide

**Project:** `hardware/servo-module/` — WM-D6C Capstan Servo Replacement Module (DSR-1)
**Tool:** KiCad 10.x
**Scope:** Servo module schematic only. Power daughter board has its own KiCad project.

> This document is the working guide for translating the DSR-1 **servo module**
> design into a KiCad 10 schematic. Read each zone before drawing it. The signal
> chain analysis, interface contract, and power architecture documents are the
> authoritative sources for all component values — this document tells you *how*
> to enter them in KiCad, not *why* they are what they are.
>
> **Structure:** the design is one flat **D-size** sheet — no hierarchy, no sub-sheets.
> The KiCad project's sheet is already a single D page; its component set is still the
> pre-battery design (AP63203, no charger), so the wall-variant power and servo entries
> here are the changes to apply.
>
> **Two KiCad projects.** The battery-integrated build uses a separate power daughter
> board. That board's schematic lives in `hardware/power-board/` and is covered by
> `kicad-power-board-guide.md`. This guide covers the servo module only — the board
> that fits in the CP304 cavity in all three builds.

---

## Compatibility Scope

DSR-1 targets the full WM-D6C / TC-D6C production run (1984–2002). Two distinct
servo circuit families exist across that run:

| Family | Servo IC | Motor drive | Years (approx.) | Governing manual |
|---|---|---|---|---|
| **Ver. 1.0** (primary) | CX20084 | Single Q601 (2SB1013 PNP) | 1984–mid 2001 | `fb4872.pdf` |
| **Ver. 1.1** (planned variant) | CX-069A | Q601–Q605 (five transistors) | mid 2001–2002 | `sony_wm-d6c_tc-d6c_ver-1.1.pdf` |

**Rev A targets Ver. 1.0 boards.** This covers the majority of surviving units.
CX-069A board support is planned as a separate variant and is out of scope for
Rev A schematic capture.

Within Ver. 1.0, there are two PCB construction generations:

| Sub-generation | Approx. years | Construction |
|---|---|---|
| Early through-hole | 1984–~1994 | Through-hole PCB |
| Later SMD | ~1994–2001 | SMD main board |

Both sub-generations have the same CX20084-based servo circuit and the same DSR-1
electrical interface. The difference is mechanical — harness routing and connector
placement. The DSR-1 board does not change; installation harness documentation
covers both sub-generations.

**To identify board family before installation:** open the machine and read the
IC at position IC601. CX20084 = Ver. 1.0 (proceed with this guide). CX-069A =
Ver. 1.1 (do not install Rev A; wait for Ver. 1.1 variant documentation).

---

## Prerequisites

Before opening KiCad:

1. `.gitignore` is committed (excludes `*-backups/`, `fp-info-cache`, `*.lock`,
   `*_autosave.*`)
2. `hardware/datasheets/` contains `stm32g0c1cc.pdf` (DS13560 Rev 6), `mt3608.pdf`,
   `fb4872.pdf`, and `sony_wm-d6c_tc-d6c_ver-1.1.pdf`
3. The `DSR-1.kicad_sym` project symbol library has been created and registered
   under Project scope in Preferences → Manage Symbol Libraries
4. Custom power symbols `B+1` and `B+3` have been created in `DSR-1.kicad_sym`
   with pin type **Power input**, pin number and name hidden, flagpole graphic body
5. Page size set to **D** (File → Page Settings → Page size **D**) — the whole design is one flat sheet

---

## KiCad Shortcut Reference

| Action | Shortcut |
|---|---|
| Add symbol | `A` |
| Add wire | `W` |
| Add net label | `L` |
| Add power port | `P` |
| Mirror horizontal | `X` |
| Mirror vertical | `Y` |
| Edit properties | `E` |
| Run ERC | Inspect → Electrical Rules Checker |
| Add no-connect flag | `Q` |

---

## Single-Sheet Layout (D-size)

The DSR-1 schematic is a **single flat sheet** on a **D-size** page (File → Page
Settings → Page size **D**). There is no hierarchy, no root sheet, and no sub-sheet
pins — the whole design (~90 components) fits on one D-sheet with headroom for the
future Ver. 1.1 / CX-069A variant.

Organize the canvas into four visual **zones**, left-to-right by signal flow. Zones are
just regions (optionally boxed with a graphic rectangle + text label); they carry no
electrical meaning:

```
┌─ Power ─────────────┐ ┌─ Microcontroller ─┐ ┌─ Signal Conditioning ─┐ ┌─ Connectors ─┐
│ USB-C / barrel in   │ │ STM32G0C1KCU6     │ │ FG divider + clamp    │ │ J1 harness   │
│ BQ24074 charge +    │ │ decoupling, boot, │ │ NPN motor drive       │ │ J2 SWD       │
│ LiPo power-path     │ │ pin assignments   │ │ ADC clamps            │ │ J3 LED board │
│ TPS63070 B+1        │ │                   │ │ VBAT sense divider    │ │ test pads    │
│ MT3608 B+3, MCP1700 │ │                   │ │                       │ │              │
└─────────────────────┘ └───────────────────┘ └───────────────────────┘ └──────────────┘
```

The sections below are written per zone. Component values come from the design docs
(signal-chain analysis, interface contract, power-supply design); this guide tells you
how to enter them.

---

## Net Labeling

On a single sheet, connectivity is trivial — **a net label connects to every identical
label anywhere on the sheet.** There are no hierarchical labels, sheet pins, or global
labels to manage (those exist only to cross sheet boundaries).

- **Net labels (`L`)** — name any net you want to join by name instead of a drawn wire
  (`FG_IN`, `MOTOR_PWM`, `VBAT_SENSE`, `BATT_LED1`, …). Same name = same net. Use them
  to avoid long cross-zone wires.
- **Power symbols (`P`)** — `B+1`, `B+3`, `+3V3`, `GND` connect globally by symbol.
  Place a PWR_FLAG at each rail's source so ERC sees it driven.
- **Local wiring** — short within-zone nets can just be wires; reserve labels for
  signals that cross zones.

Every net is traceable end-to-end with no import/export step — which removes the most
common ERC pitfall of the old hierarchical layout ("net has no driver / not imported").

---

## Power Zone

**Purpose:** The entire power chain on one region of the sheet, in signal-flow
sub-zones: **(A)** input — USB-C and/or barrel; **(B)** charge, battery, and power-path
(battery-integrated build); **(C)** the TPS63070 B+1 buck-boost; and **(D)** B+3
(MT3608) and 3.3V (MCP1700). Three builds share this zone, differentiated by DNP: the
**battery-integrated build (primary)**, **Variant A** (wall, USB-C 9V PD, no cells),
and **Variant B** (wall, barrel, no cells).

> **Authoritative values:** `power-supply-design.md` §5.2 (TPS63070) and §7.1–§7.8
> (cell, charger, power-path, protection, indicator). This guide tells you how to enter
> them in KiCad.
>
> **⚠ KiCad is behind the design.** The committed project still shows U3 = AP63203WU-7
> (a fixed buck) and no battery. The entries below are the intended design — the
> AP63203 is **replaced** by the TPS63070 (sub-zone C) and the battery front end
> (sub-zone B) is added. Treat sub-zones B and C as the changes to apply.

---

### A. Input

**J_USB — USB-C mid-mount receptacle** (battery build + Variant A; DNP for Variant B)

- Symbol: search `USB_C` in the KiCad Connector library or use a custom symbol.
- VBUS → `VBUS` net (feeds the BQ24074 charger IN pin at 5V in the battery build) and, in
  Variant A, the IP2721 input.
- D+/D− → `USB_DP_RAW`, `USB_DM_RAW`; CC1, CC2 → `CC1`, `CC2`; GND/Shield → GND.

**U1 — IP2721 USB PD Trigger (TSSOP-16)** — Variant A only

- Pins VIN, VBUSG, VBUS, CC1, CC2, SEL, GND. VIN (P$2) and VBUSG (P$1) → VBUS (tie
  together). VBUS (P$16) → `9V_PD` after successful negotiation. CC1/CC2 → `CC1`/`CC2`.
  SEL (P$11) → configure for a 9V request. C1 4.7µF 16V X5R VBUS→GND.
- **DNP on the battery build and Variant B.** In the battery build a 9V contract would
  over-volt the BQ24074 charger (VIN abs-max ~6.5V) — the battery build stays at 5V.

> **VBUSG note:** VBUSG is the internal pass-element gate drive and must be tied to VIN;
> floating it prevents `9V_PD` from becoming valid after negotiation.

**ESD — USBLC6-2SC6Y (SOT-23-6)**

- I/O1 → `USB_DM_RAW`; I/O2 → `USB_DP_RAW`; GND → GND; VCC → `+3V3`.
- Outputs `USB_DM`, `USB_DP` → the Microcontroller zone.

**Variant B — barrel jack, polarity-agnostic** (DNP for battery build and Variant A)

> Variant B accepts 9V DC via a 5.5mm/2.1mm barrel jack with **no assumed polarity** —
> a Schottky bridge guarantees correct output polarity regardless of adapter wiring,
> eliminating the original CN301 centre-negative failure mode.

- **J2** barrel jack: both pins feed the bridge (no polarity). 9V DC, ≥500mA adapter.
- **Bridge D2–D5 (SS14, SOD-123):**

```
         D2               D3
tip ───►|──┬─── DC+ ───────┬──|◄─── sleeve
           │               │
        (left AC)       (right AC)
           │               │
tip ───|◄──┴───────────────┴──►|─── sleeve
         D4               D5
                   │
                  GND
```

- DC+ (D2,D3 cathodes) → **F1** (polyfuse 500mA hold / 1.5A trip, 0805) → `B+1_RAW`;
  GND (D4,D5 anodes) → GND; DC+ ≈ 8.2V at load.
- **D6** SMBJ12A-13-F TVS (SMA): cathode → `B+1_RAW` (after F1), anode → GND. 12V
  standoff, 19.9V max clamp — under the regulator VIN max.

---

### B. Charge, Battery & Power-Path — Power Daughter Board

> **These components are not on the servo module.** The entire battery-integrated
> power subsystem — USB-C input, BQ24074 charger, cell connectors, DW01/FET protection,
> and the TPS63070 boost that generates B+1 — lives on the **power daughter board**,
> a separate KiCad project in `hardware/power-board/`.
>
> See **`kicad-power-board-guide.md`** for the full schematic entry of sub-zone B.
>
> What crosses from the daughter board to this servo module schematic as net labels
> on the board-to-board connector J_BBL (defined in the Connectors zone below):
> `VBAT_SENSE` → PB0 (ADC_IN8), `CHG_STAT` → PB7 (GPIO), `PGOOD` → PB8 (GPIO).
> `VBAT_SENSE_EN` (PB1) drives back to the daughter board to gate the sense divider.

---

### C. B+1 Regulator — TPS63070 Buck-Boost (wall variants; DNP for battery build)

The single B+1 regulator for wall variants. In the battery build this sub-zone is
**DNP** — B+1 is generated by the TPS63070 on the power daughter board and arrives
via the battery-terminal harness path. In wall builds this TPS63070 **replaces the
AP63203 buck** so one part covers both: it bucks the 9V Variant A case and boosts a
5V source if Variant B input sags below 6V.

| TPS63070 pin | Connection (wall builds only) |
|---|---|
| VINA / VINB | `9V_PD` (Variant A) or `B+1_RAW` bridge output (Variant B) + 10µF input cap to GND |
| VOUT | `B+1` node, 6.0V + bulk output cap to GND |
| FB | R1 (649kΩ) from `B+1` / R2 (100kΩ) to GND |
| L1, L2 | inductor **between the two switch pins** (not SW→VOUT), 1.5µH class, ≥3.6A sat |
| EN | RC soft-start to input rail; **do not float** (0.8V threshold) |
| PS/SYNC, PG | per datasheet |
| GND / PGND / thermal pad | GND + via array |

- Vref 0.8V; 0.8 × (1 + 649/100) = 5.99V ✓ (PWR_FLAG on `B+1`).
- Schematic note: `Buck-boost inductor sits between L1 and L2 — different routing from
  the AP63203 buck. DNP for battery-integrated build (power board provides B+1).
  See power-supply-design.md §5.2.`

---

### D. B+3 (10.8V) and 3.3V Generation

Downstream of the `B+1` node; present in every build.

**U_BOOST — MT3608 Boost (SOT-23-6), 6V → 10.8V**

| MT3608 pin | Connection |
|---|---|
| VIN | `B+1` + 10µF input cap to GND |
| GND | GND |
| SW | L_BOOST (4.7µH, CDRH4D28) → D_BOOST anode |
| EN | `B+1` (always-on) |
| FB | R_UPPER (169kΩ) / R_LOWER (10kΩ) junction |
| (implicit) | D_BOOST cathode = VOUT = `B+3` |

- VOUT = 0.6 × (1 + 169/10) = 10.74V ✓. D_BOOST = SS14 (SMA).
- C_BOOST_IN 10µF 10V X5R; **C_BOOST_OUT 47µF 16V X5R** (not 22µF — DC-bias derating).
- PWR_FLAG on `B+3`.
- Schematic note: `SW node high dV/dt at 1.2 MHz — trace <3mm; ≥2mm clearance to all
  FG/ADC/DAC traces.`

**U_LDO — MCP1700T-3302E/TT LDO (SOT-23-3), 6V → 3.3V**

- VIN → `B+1`; VOUT → `+3V3`; GND → GND.
- C_LDO_IN / C_LDO_OUT: 1µF 10V X5R (0402) + 10µF 10V X5R (0805) each. PWR_FLAG on `+3V3`.

---

### Build matrix & DNP

| Component | Battery-integrated | Variant A (wall) | Variant B (wall) |
|---|---|---|---|
| U3 TPS63070 buck-boost (B+1) | Populate | Populate | Populate |
| AP63203WU-7 | — superseded | — superseded | — superseded |
| J_USB USB-C receptacle | Populate (5V) | Populate (9V PD) | DNP |
| U1 IP2721 PD trigger | **DNP** (9V over-volts charger) | Populate | DNP |
| D1 USBLC6 ESD | Populate | Populate | DNP |
| J2 barrel + D2–D5 + F1 + D6 TVS | DNP | DNP | Populate |
| BT1 3P LiPo + U_PROT DW01/FET + U_CHG BQ24074 (daughter board) | Populate | DNP | DNP |
| U_BOOST MT3608, U_LDO MCP1700 | Populate | Populate | Populate |

---

### Net labels used in this zone

- `VBUS` — USB 5V in (BQ24074 charger IN pin; battery build on daughter board)
- `9V_PD` — Variant A only (IP2721 → TPS63070 VIN)
- `B+1_RAW` — Variant B bridge output → TPS63070 VIN
- `V_SYS` — BQ24074 SYS pin output → TPS63070 VINx (battery build; daughter board); local
- `VBAT` — pack / protection node (BQ24074 BAT, sense-divider tap); local on daughter board
- `VBAT_SENSE` → the Microcontroller zone (PB0/ADC_IN8) — arrives via board-to-board link from daughter board
- `CHG_STAT` → the Microcontroller zone (PB7) — BQ24074 CHG status; board-to-board link
- `PGOOD` → the Microcontroller zone (PB8) — BQ24074 power-good; board-to-board link
- `USB_DP`, `USB_DM`, `CC1`, `CC2` → the Microcontroller zone

---

## Microcontroller Zone (STM32G0C1)

**Purpose:** STM32G0C1KCU6 and all support components. Place the STM32 in the
centre and work outward.

### U1 — STM32G0C1KCU6 (UFQFPN32)

Use the KiCad `MCU_ST_STM32G0` library symbol `STM32G0C1KCUx`, copied to
`DSR-1.kicad_sym` and renamed `STM32G0C1KCU6`. The UFQFPN-32 package has
**1 VDD and 1 VSS** in the 32-pin K package. VDD and VDDA share one pin;
VSS and VSSA share one pin. Expose the thermal pad (EP) as described below.

> **VDDIO2 confirmed absent in the GP variant.** STM32G0C1KCU6 (_KxU) has VDDIO2
> tied to VDD internally — no separate VDDIO2 pin, no additional decoupling needed.
> (stm32g0c1cc.pdf Figure 4, GP version confirmed.)

**UFQFPN-32 GP (_KxU) confirmed pin map — DS13560 (stm32g0c1cc.pdf):**

| Pin | Name | DSR-1 assignment |
|-----|------|-----------------|
| 1 | PB9 | No-connect |
| 2 | PC14-OSC32_IN | No-connect |
| 3 | PC15-OSC32_OUT | No-connect |
| 4 | VDD/VDDA | `+3V3` (combined) |
| 5 | VSS/VSSA | GND (combined) |
| 6 | PF2-NRST | Reset — 100nF to GND |
| 7 | PA0 | `FG_IN` / TIM2_CH1 |
| 8 | PA1 | `RV601_WIPER` / ADC_IN1 |
| 9 | PA2 | `RV602_WIPER` / ADC_IN2 |
| 10 | PA3 | `RV603_WIPER` / ADC_IN3 |
| 11 | PA4 | No-connect (DAC1_OUT1 — not used, see note) |
| 12 | PA5 | `MOTOR_EN_MON` / GPIO |
| 13 | PA6 | `MOTOR_PWM` / TIM3_CH1 |
| 14 | PA7 | `SPEED_TUNE_SW` / GPIO |
| 15 | PB0 | `VBAT_SENSE` / ADC_IN8 (battery) |
| 16 | PB1 | `VBAT_SENSE_EN` / GPIO sense-gate (battery) |
| 17 | PB2 | `BATT_LED1` / GPIO·TIM (battery) |
| 18 | PA8 | `CC1` / UCPD1_CC1, UCPD1_DBCC1 |
| 19 | PA9 | `CC2` / UCPD1_CC2 |
| 20 | PC6 | `S801_BATT` / GPIO in (battery) |
| 21 | PA10 | No-connect / spare |
| 22 | PA11 [PA9] | `USB_DM` |
| 23 | PA12 [PA10] | `USB_DP` |
| 24 | PA13 | `SWDIO` |
| 25 | PA14-BOOT0 | `SWDCLK` |
| 26 | PA15 | `BATT_LED2` / GPIO·TIM (battery) |
| 27 | PB3 | `BATT_LED3` / GPIO·TIM (battery) |
| 28 | PB4 | `BATT_LED4` / GPIO·TIM (battery) |
| 29 | PB5 | `BATT_LED5` / GPIO·TIM (battery) |
| 30 | PB6 | `DEBUG_TX` / USART1_TX |
| 31 | PB7 | `CHG_STAT` / GPIO in — BQ24074 CHG status (low while charging; battery build) |
| 32 | PB8 | `PGOOD` / GPIO in — BQ24074 power-good status (battery build) |
| EP | VSS | GND — expose this pin in Symbol Editor; via array in layout |

> **PB15 does not exist in the GP variant** (_KxU). PB15 is only present in
> the N variant (_KxUxN). The previous guide entry "PB15 / UCPD1_CC2" was
> wrong. CC2 → PA9 (pin 19) for UCPD1_CC2.
>
> **DEBUG_TX moved from PA9 to PB6.** PA9 is required for UCPD1_CC2 and cannot
> simultaneously serve USART1_TX. PB6 (pin 30) carries USART1_TX and is
> available.
>
> **VDDIO2 confirmed absent** in GP variant. Pin 20 = PC6. No VDDIO2 decoupling
> needed. (stm32g0c1cc.pdf Figure 4, GP version _KxU confirmed.)

> **PA4 (DAC1_OUT1) is not used for motor drive.** Q601 (2SB1013 PNP) on
> Ver. 1.0 boards has its emitter at B+3 (10.8V); the base operating range
> exceeds the 3.3V DAC output range. Motor drive uses TIM3 PWM on PA6 through
> the NPN level-shift stage on the Signal Conditioning zone. PA4 may be left unconnected with a
> no-connect flag or reserved as a spare ADC/DAC pin.

**Unused pins:** Place a No-Connect flag (`Q`) on every unused GPIO pin.

---

> **Battery-build pins:** pins 15–17, 20, 26–29, and 31–32 carry the battery level
> indicator and charge-status signals (No-connect on wall-only builds). LED segment
> pins on timer channels can PWM-dim the bar (night dimming, charge animation); plain
> GPIO gives on/off segments. Reconcile with `stm32g0c1-pin-allocation.md`.

### Decoupling Network

**VDD/VDDA decoupling (pin 4 — combined supply in 32-pin package):**
- 100nF 10V X5R 0402 — power supply decoupling
- 100nF 10V C0G 0402 — analog noise filtering (C0G/NP0, lower noise than X5R)
- 1µF 10V X5R 0402 — bulk bypass

All three caps connect between pin 4 and GND. The C0G cap should be placed
closest to the pin on the PCB as it serves the ADC inputs.

> **VDDIO2:** Absent in the STM32G0C1KCU6 GP variant (_KxU). No additional
> decoupling needed — VDDIO2 is tied to VDD internally.

All capacitor GND terminals connect directly to the nearest GND power symbol.

---

### Boot and Reset

**BOOT0 (PA14-BOOT0 — shared with SWDCLK):**

```
+3V3 ──[10kΩ]────┬── BOOT0
                  │
                 GND (holds BOOT0 low = normal boot from flash)
```

**NRST:**
- 100nF to GND; no external reset button on Rev A unless space allows
- Add no-connect flag if not bringing out to a test point

---

### Net labels used in this zone

**Inputs:**
- `USB_DP`, `USB_DM`, `CC1`, `CC2` ← the Power zone
- `FG_IN`, `RV601_WIPER`, `RV602_WIPER`, `RV603_WIPER`, `MOTOR_EN_MON` ← the Signal Conditioning zone
- `SPEED_TUNE_SW`, `SWDIO`, `SWDCLK` ← the Connectors zone

**Outputs:**
- `MOTOR_PWM` → the Signal Conditioning zone

**Battery build:**
- Inputs: `VBAT_SENSE`, `S801_BATT`, `CHG_STAT`, `PGOOD`
- Outputs: `VBAT_SENSE_EN`, `BATT_LED1`…`BATT_LED5`

---

## Signal Conditioning Zone

**Purpose:** Every analog interface between the STM32 and the machine. Conditions
incoming machine-voltage signals down to 3.3V; level-shifts STM32 PWM output up
to drive the machine's motor transistor.

### FG Signal Path (J1 Pin 1 → PA0)

FG901 GP2S22AB is an open-collector optical interrupter. Its output is pulled to
B+1 (~6V) through a resistor on the WM-D6C main board. The swing is approximately
0 to 5.9V — exceeding the STM32's 3.6V absolute maximum input.

```
FG_RAW (from the Connectors zone, J1 pin 1)
    │
   [R3: 10kΩ 1% 0402]
    │
   [R4: 22kΩ 1% 0402] ──── GND        ← voltage divider lower leg
    │
   ├──[D_FG1: BAT54S, anode→node, cathode→+3V3]   ← high-side clamp (BAT54S = dual SOT-23; one package serves both clamps)
   ├──[D_FG2: BAT54S, anode→GND, cathode→node]    ← low-side clamp
   │
   [C7: 10pF C0G 0402] ──── GND        ← motor-brush noise filter
    │
   FG_IN (net label → the Microcontroller zone, PA0)
```

Divider math: V_node = V_FG × 22/(10+22) = V_FG × 0.6875
At 5.9V swing: 5.9 × 0.6875 = 4.06V → BAT54S clamps to ≤3.6V ✓

Schematic note: `⚠ VERIFY ACTUAL FG SWING ON BENCH BEFORE POWER-ON.
R4 value may need adjustment based on measured B+1 and pull-up resistor.
See signal-chain-analysis.md §1 for formula. Do not connect FG directly
to PA0 without confirming the conditioned voltage is below 3.3V.`

---

### Motor Drive Output (PA6 → J1 Pin 2 → Q601 base)

Q601 on WM-D6C Ver. 1.0 boards is a **2SB1013 PNP transistor** with emitter at
B+3 (10.8V). The base must be pulled well below the emitter to turn it on. The
STM32 3.3V PWM output cannot drive this base directly — a level-shift stage
using a small-signal NPN (Q_LS) is required.

**Boot-safe behavior:** At reset, TIM3 PWM defaults to 0% duty cycle. Q_LS is
off. R9 pulls Q601's base toward B+1 (6V). Vbe = 6V − 10.8V = −4.8V — Q601
is firmly off. Motor cannot run uncontrolled during boot or firmware update.

```
MOTOR_PWM (from the Microcontroller zone, PA6 TIM3_CH1, 15.6 kHz)
    │
   [R7: 1kΩ 5% 0402]
    │
    ├──[C8: 100nF 10V X5R 0402] ──── GND    ← PWM-to-analog RC filter
    │                                          fc = 1/(2π×1kΩ×100nF) = 1.59 kHz
    │
   Q_LS base (MMBT3904 NPN, SOT-23)
   Q_LS emitter → GND
   Q_LS collector
    │
   [R8: 10kΩ 5% 0402] ──── [Q601_BASE net / J1 pin 2]
    │
   [R9: 100kΩ 5% 0402] ──── B+1 power symbol
    │
   Q601_BASE (net label → the Connectors zone, J1 pin 2)
```

Q_LS is the MMBT3904 NPN on the DSR-1 board. Q601 (2SB1013) is on the WM-D6C
main board — use a generic PNP symbol with value `2SB1013` and note `Off-board`.

Schematic note: `Q601 (2SB1013 PNP) is on WM-D6C main board — NOT on DSR-1 PCB.
Q601 emitter → B+3 via Q703/Q704 mode-switch chain. Q601 base operating range
must be measured on bench to confirm R9 sizing. See signal-chain-analysis.md §2.`

---

### Motor Enable Monitor (J1 Pin 3 → PA5)

The MOTOR_EN signal at the IC601 footprint pin 7 pad is held HIGH (~4.4V) during
playback via R605 (pullup to B+1). Q702 on the auto-off board pulls it LOW at
end-of-tape or auto-off. After IC601 is removed, this signal is accessible at
the IC601 footprint pin 7 pad or the R605 junction.

```
MOTOR_EN (from the Connectors zone, J1 pin 3)
    │
   [R_EN1: 10kΩ 1% 0402]
    │
   [R_EN2: 22kΩ 1% 0402] ──── GND
    │
   MOTOR_EN_MON (net label → the Microcontroller zone, PA5)
```

Divider: 22/(10+22) = 0.6875. At 4.4V playback: 3.025V ≤ 3.3V ✓
At auto-off (low): ≈0.2V, below VIL ✓

Schematic note: `MOTOR_EN tap point: IC601 footprint pin 7 pad on WM-D6C main
board. Verify voltage on bench before connecting. See signal-chain-analysis.md §3.`

---

### ADC Inputs — RV601, RV602, RV603 (J1 Pins 4, 5, 6 → PA1, PA2, PA3)

Each potentiometer wiper may be referenced to B+1 and could exceed 3.3V.
The 100Ω series resistor plus BAT54S clamp is the minimum safe configuration.

**⚠ Measure wiper voltage on bench before first power-on.** If any wiper exceeds
3.3V at B+1 = 6V, add a resistor divider before the clamp.

```
RV601 (from the Connectors zone, J1 pin 4)
    │
   [R_ADC1: 100Ω 5% 0402]              ← source impedance limit + protection
    │
   ├──[BAT54S, anode→node, cathode→+3V3]  ← overvoltage clamp
   ├──[BAT54S, anode→GND, cathode→node]   ← undervoltage clamp
    │
   RV601_WIPER (net label → the Microcontroller zone, PA1/ADC_IN1)
```

Repeat identically for RV602 → PA2 and RV603 → PA3.

Schematic note: `⚠ VERIFY WIPER VOLTAGE BEFORE POWER-ON. See signal-chain-
analysis.md §4 for divider formula. If wiper exceeds 3.3V, add R/2R divider
before the 100Ω series resistor. Adjust ADC scaling in firmware to match.`

---

### Battery Sense (VBAT → ADC) — battery build

> **VBAT is on the daughter board.** In the battery-integrated build, the raw pack
> voltage (VBAT) lives on the daughter board's VBAT node. It arrives at the servo
> module as `VBAT_SENSE` via the board-to-board link — a wire from the daughter
> board's sense tap to PB0 on the servo module. The divider and sense-gate below
> sit on the **servo module** (receiving end).

```
VBAT_SENSE (from daughter board board-to-board link)
    │
   [C_VS: 10nF C0G 0402] ──── GND     (at the VBAT_SENSE node)
    │
   ┌─ ADC_IN8 / PB0 (Microcontroller zone)
   │
[R_VS1: 33kΩ 1% 0402] ──┬── above node
                         │
                     [R_VS2: 100kΩ 1% 0402]
                         │
                     Q_VS drain  (2N7002 N-FET; gate ← VBAT_SENSE_EN / PB1; source → GND)
```

- **33kΩ / 100kΩ divider** (updated from 22k for the LiPo 4.2V peak): ratio
  100/133 = 0.75 → 4.2V float presents ~3.16V to ADC (under 3.3V VDDA with margin);
  3.0V floor → ~2.26V. LiPo discharge curve (4.2V → 3.0V) is usefully sloped, so
  voltage-based SoC is more reliable here than on the flat LFP curve.
- Q_VS gates the divider's ground leg so it draws ~0 except during a measurement
  (firmware raises `VBAT_SENSE_EN`, settles, samples, lowers) — no standby drain on
  the pack. Q_VS source sits on the servant board module schematic; it drains down
  through R_VS2.
- Schematic note: `Source impedance ~25kΩ — allow adequate ADC sample time (≥10µs
  sample time at 64MHz). LiPo OCV table in firmware; sloped 4.2V→3.0V curve gives
  reliable SoC. VBAT arrives via board-to-board link from daughter board. See §7.8.`

**Battery level indicator drive:** the five `BATT_LED1…5` lines route from the
Microcontroller zone to J3 (Connectors zone) and on to the WM-D6C LED board. Optional
small series resistors per line may go here; the existing 180Ω limiters (R814–818) live
on the LED board. CHG_STAT and PGOOD (PB7/PB8) drive the charge animation — segments
march upward while PGOOD is high and CHG_STAT is low (charging); hold full when
CHG_STAT goes high (done).

### Net labels used in this zone

**Inputs (from other sheets):**
- `MOTOR_PWM` ← the Microcontroller zone (PA6)
- `FG_RAW`, `MOTOR_EN`, `RV601`, `RV602`, `RV603` ← the Connectors zone

**Outputs (to other sheets):**
- `FG_IN` → the Microcontroller zone (PA0)
- `RV601_WIPER`, `RV602_WIPER`, `RV603_WIPER` → the Microcontroller zone
- `MOTOR_EN_MON` → the Microcontroller zone
- `Q601_BASE` → the Connectors zone (J1 pin 2)

**Battery build:**
- `VBAT_SENSE_EN` ← the Microcontroller zone; `VBAT_SENSE` → the Microcontroller zone

---

## Connectors Zone

**Purpose:** Physical boundaries of the board. All connectors and headers.
No signal conditioning — just mechanical connections and net labels.

### J1 — 8-pin JST SH 1.25mm (WM-D6C Interface Harness)

Symbol: `Connector_JST:JST_SH_SM08B-SRSS-TB_1x08-1MP_P1.00mm_Horizontal`
or equivalent 8-pin 1.25mm JST SH footprint.

| J1 Pin | Net Label | Signal | Direction |
|---|---|---|---|
| 1 | `FG_RAW` | FG901 optical sensor output | Machine → Module |
| 2 | `Q601_BASE` | Motor drive to Q601 base | Module → Machine |
| 3 | `MOTOR_EN` | MOTOR_EN net (IC601 pin 7 pad) | Machine → Module |
| 4 | `RV601` | RV601 wiper (base speed trim) | Machine → Module |
| 5 | `RV602` | RV602 wiper (Speed Tune slider) | Machine → Module |
| 6 | `RV603` | RV603 wiper (Speed Tune range) | Machine → Module |
| 7 | `B+1` power symbol | 6V supply rail | Shared |
| 8 | GND power symbol | Chassis ground | Shared |

Pin 7 and Pin 8 use power symbols, not net labels.

**Series diode on J1 pin 7 (B+1 supply line):**

```
B+1 power symbol ──[D_B1: 1N5819, anode]──[cathode]── J1 pin 7
```

Schematic note: `D_B1 prevents reverse current from Sony B+1 rail into DSR-1
converter when Sony is powered and DSR-1 is not. Forward drop ~0.3V at load
current. Machine sees ~5.7V on B+1 — within specification.`

**S601 Speed Tune switch — direct connection, no level shift:**

S601 is rewired to operate at 3.3V logic level. Connect one terminal to `+3V3`
and the other to `SPEED_TUNE_SW`. STM32 PA7 is configured with internal pull-down.
No external components needed.

```
+3V3 ──[S601 switch]── SPEED_TUNE_SW (→ the Microcontroller zone, PA7)
                        │
                       [PA7 internal pull-down ~40kΩ to GND]
```

This pin does not route through J1 — S601 is rewired during installation to
connect directly to the DSR-1 board.

---

### J2 — SWD Debug Header (4-pin 2.54mm, 1×4)

Symbol: `Connector:Conn_01x04_Pin`

| J2 Pin | Net Label | Signal |
|---|---|---|
| 1 | GND | Ground |
| 2 | `SWDCLK` | SWD clock |
| 3 | `SWDIO` | SWD data |
| 4 | `+3V3` | 3.3V reference only |

Schematic note: `J2: SWD programming header. Use STM32CubeProgrammer or OpenOCD.
3.3V pin is reference only — do not use to power the board. Do not bury this
header under the installed board; keep accessible during Rev A development.`

---

### J3 — LED-Board Harness (battery indicator), 7-pin

| J3 Pin | Net | Signal | Direction |
|---|---|---|---|
| 1 | `BATT_LED1` | LED D801 drive | Module → Machine |
| 2 | `BATT_LED2` | LED D802 drive | Module → Machine |
| 3 | `BATT_LED3` | LED D803 drive | Module → Machine |
| 4 | `BATT_LED4` | LED D804 drive | Module → Machine |
| 5 | `BATT_LED5` | LED D805 drive | Module → Machine |
| 6 | `S801_BATT` | S801 BATT-position sense | Machine → Module |
| 7 | GND | Common / return | Shared |

> Schematic note: `⚠ Bench-confirm before wiring J3 (power-supply-design.md §7.7):
> (1) how S801's BATT position reroutes D801–D805 and whether the CX10043 outputs go
> high-impedance there (so the STM32 drives them without contention); (2) LED common
> polarity through the on-board 180Ω limiters, to set drive direction. The stock
> single-LED brightness path (Q801) is left disconnected once the STM32 owns the bar.`

### J_BBL — Board-to-Board Link (battery build only)

A small polarised connector carrying the signals between the power daughter board and
this servo module. **DNP on wall-only builds.** Use a latching, foolproof connector
rated for ≥2A continuous on the power conductors — JST-PH or Molex PicoBlade are
suitable.

| J_BBL Pin | Net | Dir | Description |
|---|---|---|---|
| 1 | GND | Shared | Common ground return |
| 2 | `VBAT_SENSE` | DB → Module | LiPo pack voltage sense (33kΩ/100kΩ divider; see Signal zone) |
| 3 | `VBAT_SENSE_EN` | Module → DB | Gate control for daughter board sense divider (PB1 GPIO) |
| 4 | `CHG_STAT` | DB → Module | BQ24074 CHG status — low while charging (PB7 GPIO) |
| 5 | `PGOOD` | DB → Module | BQ24074 power-good status (PB8 GPIO) |
| 6 | USB_D− *(optional)* | Bidirectional | CDC tuning, if USB-C is on daughter board |
| 7 | USB_D+ *(optional)* | Bidirectional | CDC tuning, if USB-C is on daughter board |

Schematic note: `J_BBL: board-to-board link to power daughter board (battery build).
DNP for Variant A and Variant B wall builds. B+1 and GND are NOT on this connector —
they travel via the battery-terminal injection point on the WM-D6C main board and the
CP304 harness. USB D+/D- pins populate only if CDC tuning routes through the daughter
board USB-C rather than a bench header.`

---

### Test Pads

Add `Device:TestPoint` for each of:

| Reference | Net | Purpose |
|---|---|---|
| TP1 | `+3V3` | MCU supply check |
| TP2 | `B+3` | Motor supply check |
| TP3 | `FG_RAW` | FG oscilloscope probe point |
| TP4 | `Q601_BASE` | Motor drive output check |
| TP5 | `DEBUG_TX` | UART debug / PA9 |
| TP6 | GND | Scope ground reference |

---

## Annotation Scheme

The design is one flat sheet, so annotate in a single pass: Tools → Annotate Schematic
→ Annotate. A single sequential pass is fine; for readability you can group designators
by zone by assigning ranges as you place parts. After merging or adding the battery
block, re-run Annotate to resolve any duplicate designators (e.g. the MCP1700 LDO
should be a single Ux, and the flatten/merge can leave gaps — that is harmless).

## Symbol Properties Checklist

Before running ERC, verify every symbol has:

| Field | Example |
|---|---|
| **Value** | `169kΩ`, `STM32G0C1KCU6`, `MMBT3904` |
| **Footprint** | `DSR-1:STM32G0C1KCU6_UFQFPN32` |
| **Datasheet** | relative path in `hardware/datasheets/` or URL |
| **LCSC** | JLCPCB part number (custom field) |
| **DNP** | `Variant A only`, `Variant B only`, or blank for shared |

---

## ERC — Expected Errors and Resolutions

Run Inspect → Electrical Rules Checker after completing all sheets.

| Error | Cause | Resolution |
|---|---|---|
| Power pin not driven | Missing PWR_FLAG | Add PWR_FLAG on B+1, B+3, +3V3 at the rail source |
| Pin unconnected | Unused STM32 GPIO | Add No-Connect flag (`Q`) |
| Net has no driver | Net-label typo or missing PWR_FLAG | Check label spelling; add PWR_FLAG at the rail source |
| Conflicting net | Duplicate label names | Ensure all labels are unique or intentionally shared |

Target: **zero errors** before moving to PCB layout. Warnings on properly-flagged
DNP open pins are acceptable.

---

## First Commit After Schematic Completion

```
git add hardware/kicad/
git commit -m "DSR-1 Rev A schematic — single D-size sheet, ERC clean, pre-layout

- Single flat D-size sheet (no hierarchy)
- Power zone: USB-C / barrel input, BQ24074 charge + LiPo power-path (battery
  build), TPS63070 B+1 buck-boost, MT3608 B+3, MCP1700 3.3V
- MCU zone: STM32G0C1KCU6 decoupling, pin assignments; motor PWM on PA6 (TIM3_CH1)
- Signal zone: FG divider/clamp, NPN motor drive, ADC clamps, VBAT sense
- Connectors zone: J1 harness, J2 SWD, J3 LED board, S601 rewire, test pads
- Rev A targets Ver. 1.0 CX20084 boards (1984-2001)
- ERC: 0 errors"

git tag -a hw-v0.1-schematic-complete -m "Schematic complete, ERC clean, ready for layout review"
git push origin main --tags
```

---

## Distribution & Printing

A single D-size sheet does **not** require downstream users to own a large-format
plotter. For public release (CERN-OHL-P), ship readable artifacts alongside the KiCad
source:

- **Full-size schematic PDF** — `kicad-cli sch export pdf` renders the D-sheet to a PDF
  anyone can open and zoom/pan on screen, or tile-print across letter/A4 in their PDF
  reader's poster/tile mode. Screen review needs no paper at all.
- **(Optional) tiled letter/A4 PDF** — for bench/service use, provide a paginated PDF so
  a single page can sit next to the machine. Generate it in a release script rather than
  maintaining a second schematic by hand.
- **Release file set** — schematic PDF (readable by everyone) + `.kicad_sch`/project (for
  editors) + BOM + gerbers. The source's sheet size never reaches a plotter unless someone
  chooses to send it there.

Example export (from the project directory):

```
kicad-cli sch export pdf hardware/kicad/DSR-1/DSR-1.kicad_sch -o dist/DSR-1-schematic.pdf
```

---

## PCB Layout Priority Constraints

After ERC passes, update the PCB (Tools → Update PCB from Schematic) and begin
placement in this priority order:

1. STM32 decoupling caps within 0.5mm of VDD/VDDA/VDDIO2 pins
2. MT3608 switching loop (VIN cap → L1 → SW → D_BOOST → VOUT cap) as compact as possible; SW node trace ≤3mm
3. USB differential pair: matched length, 90Ω differential impedance, ≤25mm, routed away from MT3608 SW node
4. B+3 motor drive traces: minimum 0.5mm width for 500mA
5. GND pour on both layers; via array under STM32 exposed VSS pad (2×2 or 3×3 grid, 0.3mm drill)
6. Analog signal zone (FG, ADC, motor drive) on opposite side of board from MT3608

See `signal-chain-analysis.md §10` for full layout rule derivations.

---

*Document version: 0.8 — Matches DSR-1 hardware Rev A*
*Replaces: initial draft (v0.1) — corrected motor drive topology, full production run scope added, sheet filenames aligned with actual KiCad project*
*v0.3 adds: battery-integrated charge-in-place (CN3058E + power-path), TPS63070 buck-boost B+1 regulator, and STM32-driven battery level indicator (CX10043 re-reference). Per power-supply-design.md §5.2, §7.*
*v0.4: consolidated power onto a single Power sheet (was Power Input Zone + Power Management); sheets renumbered to 5 total (MCU→3, Signal→4, Connectors→5).*
*v0.6: merged the battery appendix inline — battery/charge/power-path/TPS63070 now live in the Power zone, and the indicator pins/sense/J3 in the MCU, Signal, and Connectors zones; single cohesive guide, no addendum.*
*v0.5: flattened to a single D-size sheet — hierarchy, root sheet, and sheet pins removed; zones replace sheets; added Distribution & Printing.*
*v0.7: chemistry swap LFP → LiPo; charger CN3058E → BQ24074 (DPPM power-path, R_ISET=1.1kΩ/0.8A, R_ILIM=800Ω/2A); protection HY2112-CB → DW01+FS8205; two-board architecture noted; **NOTE: v0.7 incorrectly renamed G0C1 → G0B1 throughout — reverted in v0.8.** CHG_STAT/PGOOD nets; VBAT_SENSE divider 22k→33k; LiPo OCV note; §7.8 cross-reference.*
*v0.8: two-project split — servo module guide only; Section B replaced with pointer to kicad-power-board-guide.md; J_BBL board-to-board connector added to Connectors zone; TPS63070 sub-zone C marked DNP for battery build; **MCU corrected back to STM32G0C1KCU6** (verified against repo schematic STM32G0C1 and decoupling.kicad_sch, symbol MCU_ST_STM32G0:STM32G0C1KCUx, U5); DS reference updated; BAT54 → BAT54S (dual SOT-23, as placed in Signal conditioning.kicad_sch); intro updated for two-project structure.*
