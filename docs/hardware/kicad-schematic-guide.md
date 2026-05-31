# DSR-1 KiCad 10 Schematic Implementation Guide

**Project:** WM-D6C Capstan Servo Replacement Module (DSR-1)
**Tool:** KiCad 10.x
**Scope:** Complete schematic entry instructions, sheet by sheet

> This document is the working guide for translating the DSR-1 design into a
> KiCad 10 schematic. Read each section before beginning that sheet. The signal
> chain analysis, interface contract, and power architecture documents are the
> authoritative sources for all component values — this document tells you *how*
> to enter them in KiCad, not *why* they are what they are.

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
2. `hardware/datasheets/` contains `stm32g0b1cc.pdf` (DS13560 Rev 6), `mt3608.pdf`,
   `fb4872.pdf`, and `sony_wm-d6c_tc-d6c_ver-1.1.pdf`
3. The `DSR-1.kicad_sym` project symbol library has been created and registered
   under Project scope in Preferences → Manage Symbol Libraries
4. Custom power symbols `B+1` and `B+3` have been created in `DSR-1.kicad_sym`
   with pin type **Power input**, pin number and name hidden, flagpole graphic body

---

## KiCad Shortcut Reference

| Action | Shortcut |
|---|---|
| Add symbol | `A` |
| Add wire | `W` |
| Add net label | `L` |
| Add power port | `P` |
| Add global label | `Ctrl+L` |
| Add hierarchical label | `Ctrl+L` (inside sub-sheet) |
| Mirror horizontal | `X` |
| Mirror vertical | `Y` |
| Edit properties | `E` |
| Run ERC | Inspect → Electrical Rules Checker |
| Add no-connect flag | `Q` |

---

## Sheet Architecture

Six sheets total. Sheet 1 is the root — it contains only sub-sheet symbols and
the wires connecting their hierarchical pins. Sheets 2–6 contain actual components.

```
Sheet 1 — DSR-1.kicad_sch (root)
├── Sheet 2 — Power input zone.kicad_sch
├── Sheet 3 — Power management.kicad_sch
├── Sheet 4 — STM32G0C1 and decoupling.kicad_sch
├── Sheet 5 — Signal conditioning.kicad_sch
└── Sheet 6 — Connectors.kicad_sch
```

Sheet numbering follows signal flow: power enters at Sheet 2, is regulated in
Sheet 3, drives the MCU on Sheet 4, conditions signals on Sheet 5, and exits
through connectors on Sheet 6.

> **Note:** `rev-a-schematic-plan.md` proposes a finer 10-sheet hierarchy. The
> 6-sheet structure above matches the actual KiCad project files and is the
> authoritative sheet layout for Rev A. The schematic plan's section headings are
> still useful as design checklists for each sheet's responsibility.

---

## Hierarchical Label Rules

**Hierarchical Labels** (inside sub-sheets) are the boundary ports of each sheet.
**Sheet Pins** (on the sub-sheet rectangles in Sheet 1) are the matching endpoints.
**Power Symbols** (`B+1`, `B+3`, `+3V3`, `GND`) connect globally — never use
hierarchical pins for supply rails.

| Label | Direction | Sheet where defined |
|---|---|---|
| `9V_PD` | Output | Sheet 2 (Variant A) |
| `B+1_RAW` | Output | Sheet 2 (Variant B) |
| `FG_IN` | Output | Sheet 5 → Sheet 4 |
| `MOTOR_PWM` | Input | Sheet 5 ← Sheet 4 |
| `RV601_WIPER` | Output | Sheet 5 → Sheet 4 |
| `RV602_WIPER` | Output | Sheet 5 → Sheet 4 |
| `RV603_WIPER` | Output | Sheet 5 → Sheet 4 |
| `MOTOR_EN_MON` | Output | Sheet 5 → Sheet 4 |
| `SPEED_TUNE_SW` | Output | Sheet 6 → Sheet 4 |
| `SWDIO` | Bidirectional | Sheet 6 ↔ Sheet 4 |
| `SWDCLK` | Bidirectional | Sheet 6 ↔ Sheet 4 |
| `USB_DP` | Output | Sheet 2 → Sheet 4 |
| `USB_DM` | Output | Sheet 2 → Sheet 4 |
| `CC1` | Output | Sheet 2 → Sheet 4 |
| `CC2` | Output | Sheet 2 → Sheet 4 |

After placing each Hierarchical Label inside a sub-sheet, return to Sheet 1,
right-click the sub-sheet rectangle, and select **Import Sheet Pin** to place
the matching pin on the rectangle boundary.

---

## Sheet 1 — Top Level

**Purpose:** Architecture map only. No components. No values. One glance =
full system understanding.

### What to place

1. Five sub-sheet rectangles using Place → Add Hierarchical Sheet (`S`):

| Sheet name | Filename (must match exactly) |
|---|---|
| `Power_Input_Zone` | `Power input zone.kicad_sch` |
| `Power_Management` | `Power management.kicad_sch` |
| `Microcontroller` | `STM32G0C1 and decoupling.kicad_sch` |
| `Signal_Conditioning` | `Signal conditioning.kicad_sch` |
| `Connectors` | `Connectors.kicad_sch` |

2. After importing all sheet pins, wire the connections between rectangles:

```
Power_Input_Zone  ──9V_PD──────────────► Power_Management   (Variant A)
                  ──B+1_RAW────────────► Power_Management   (Variant B)
                  ──USB_DP, USB_DM─────► Microcontroller
                  ──CC1, CC2───────────► Microcontroller

Microcontroller   ──MOTOR_PWM──────────► Signal_Conditioning
                  ──FG_IN──────────────── Signal_Conditioning
                  ──RV601/2/3_WIPER────── Signal_Conditioning
                  ──MOTOR_EN_MON───────── Signal_Conditioning
                  ──SPEED_TUNE_SW─────── Connectors
                  ──SWDIO, SWDCLK──────── Connectors

Signal_Conditioning ──FG_RAW──────────── Connectors
                    ──MOTOR_DRIVE────────► Connectors
                    ──RV_WIPERS─────────── Connectors
```

3. Add a schematic text block noting:
   - KiCad 10 required
   - Rev A targets Ver. 1.0 CX20084 boards (1984–2001)
   - Variant A (USB-C PD) and Variant B (barrel) differentiated by DNP
   - Revision and date

**Annotation:** Sheet 1 has no components to annotate.

---

## Sheet 2 — Power Input Zone

**Purpose:** All components between the external power connector and the first
regulated rail output. Variant A (USB-C PD) and Variant B (barrel jack) components
both live here, differentiated by DNP.

### Variant A — USB-C PD (DNP for Variant B)

**J_USB — USB-C mid-mount receptacle**

- Symbol: search `USB_C` in KiCad Connector library or use custom symbol
- VBUS → `9V_PD` net label (becomes Hierarchical Label output)
- D+/D− → `USB_DP_RAW`, `USB_DM_RAW` net labels
- CC1, CC2 → `CC1`, `CC2` net labels
- GND, Shield → GND power symbol
- Mark: `DNP Variant B`

**U1 — IP2721 USB PD Trigger (TSSOP-16)**

- Custom symbol with pins: VIN, VBUSG, VBUS, CC1, CC2, SEL, GND
- VIN (P$2) and VBUSG (P$1) → both connect to VBUS from J1 (tie together)
- VBUS (P$16) → `9V_PD` net label after successful PD negotiation
- CC1 (P$13), CC2 (P$12) → `CC1`, `CC2` net labels
- SEL (P$11) → configure per IP2721 datasheet for 9V output request
- Decoupling: C1 4.7µF 16V X5R between VBUS and GND
- Mark: `DNP Variant B`

> **VBUSG note:** VBUSG is the gate drive for the internal pass element. It must
> be tied to VIN — leaving it floating prevents 9V_PD from becoming valid after
> PD negotiation.

**U3 — AP63203WU-7 Buck Converter (SOT-23-6), 9V → 6V — shared, both variants**

- Input: `9V_PD` net (Variant A via U1) or bridge rectifier output (Variant B via `B+1_RAW` net)
- Output: `B+1` power symbol (global), 6.0V regulated
- Vref = 0.8V; R1 = 649kΩ E96, R2 = 100kΩ → Vout = 6.19V ✓
- Add PWR_FLAG on `B+1` output net (use power symbol "Generate power flag" checkbox)
- **Not DNP — populated on both Variant A and Variant B boards**

| AP63203WU-7 pin | Connection |
|---|---|
| VIN (pin 5) | 9V_PD / bridge output net + C2 (10µF 16V X5R) to GND |
| GND (pin 2) | GND |
| EN (pin 3) | Tie to VIN — always-on, internal clamp handles 9V safely |
| SW (pin 1) | L1 (4.7µH 1.5A) → B+1 output node |
| FB (pin 4) | R1 (649kΩ) from B+1 output node; R2 (100kΩ) to GND |
| BST (pin 6) | C3 (100nF 16V X5R) to SW (pin 1) |

Output node connects to: B+1 power symbol, B+1_RAW hierarchical label, R1 top, C4 (22µF 10V X5R) to GND.

**ESD Protection — USBLC6-2SC6Y (SOT-23-6)**

- I/O1 → `USB_DM_RAW`; I/O2 → `USB_DP_RAW`; GND → GND; VCC → `+3V3`
- Outputs: `USB_DM`, `USB_DP` → Hierarchical Label outputs to Sheet 4

---

### Variant B — Barrel Jack (DNP for Variant A)

**Design principle: polarity agnostic**

> Variant B accepts 9V DC via a 5.5mm/2.1mm barrel jack with **no assumed
> polarity**. The original WM-D6C CN301 used non-standard centre-negative
> polarity, which was the primary external-power failure mode. DSR-1 Variant B
> eliminates this failure mode entirely: a Schottky bridge rectifier ensures
> correct polarity at the output regardless of which way the adapter is wired.
> No polarity marking on the connector is required or expected.

**J2 — 5.5mm/2.1mm barrel jack**

- Symbol: `Connector:Barrel_Jack` or equivalent
- Pin 1 (tip) and Pin 2 (sleeve) both feed into the bridge — no polarity assignment
- Mark: `DNP Variant A`
- Input spec: **9V DC nominal, 500mA minimum adapter rating**

**Bridge rectifier — D2, D3, D4, D5 (SS14, SOD-123)**

Four Schottky diodes in a full-bridge configuration:

```
         D2               D3
tip ───►|──┬─── DC+ ───────┬──|◄─── sleeve
           │               │
J2      (left AC)       (right AC)
           │               │
tip ───|◄──┴───────────────┴──►|─── sleeve
         D4               D5
                   │
                  GND
```

- DC+ (cathodes of D2 and D3) → F1 → B+1_RAW
- GND (anodes of D4 and D5) → GND
- Voltage at DC+ after bridge: ~8.2V (9V − 0.8V Schottky drop at load)
- Mark all: `DNP Variant A`

**F1 — Polyfuse, 500mA hold / 1.5A trip (0805)**

- In series between bridge DC+ and the B+1_RAW / TVS node
- Mark: `DNP Variant A`

**D6 — SMBJ12A-13-F TVS (SMA), unidirectional**

- Cathode (pin 2) → B+1_RAW node (after F1)
- Anode (pin 1) → GND
- Standoff voltage 12V — safely above 9V nominal; clamp voltage 19.9V max —
  well below AP63203WU-7 VIN absolute maximum of 32V
- Mark: `DNP Variant A`

**Protection stack — correct node order:**

```
Bridge DC+
      │
     [F1] Polyfuse 500mA / 1.5A trip (0805)
      │
      ├──────────────── B+1_RAW ──► AP63203WU-7 VIN (shared U3)
      │
     [D6] SMBJ12A cathode here, anode → GND
```

Mark all: `DNP Variant A`

---

### Hierarchical Labels for Sheet 2

- `9V_PD` — Output (Variant A only) — 9V PD-negotiated rail from IP2721 to AP63203WU-7 VIN
- `B+1_RAW` — Output (both variants) — connects to AP63203WU-7 VIN and B+1 output node; Variant B bridge feeds this net; Variant A IP2721 output feeds this net via 9V_PD
- `USB_DP`, `USB_DM` — Output → Sheet 4 (Variant A only; DNP Variant B)
- `CC1`, `CC2` — Output → Sheet 4

### DNP summary for Sheet 2

| Component | Variant A | Variant B |
|---|---|---|
| J1 USB_C_Receptacle | Populate | DNP |
| U1 IP2721 | Populate | DNP |
| D1 USBLC6-2SC6Y | Populate | DNP |
| C1 4.7µF (VBUS decoupling) | Populate | DNP |
| J2 Barrel_Jack | DNP | Populate |
| D2–D5 SS14 (bridge) | DNP | Populate |
| F1 Polyfuse | DNP | Populate |
| D6 SMBJ12A TVS | DNP | Populate |
| U3 AP63203WU-7 | Populate | Populate |
| L1, C2, C3, C4, R1, R2 | Populate | Populate |

---

## Sheet 3 — Power Management

**Purpose:** Generate B+3 (10.8V motor rail) from B+1 using MT3608 boost
converter. Generate 3.3V for the STM32 using MCP1700 LDO. Both converters
present on both Variants A and B.

### U2 — MT3608 Boost Converter (SOT-23-6), 6V → 10.8V

| MT3608 Pin | Connection |
|---|---|
| VIN | `B+1` power symbol + 10µF input cap to GND |
| GND | GND |
| SW | L1 (4.7µH, CDRH4D28) → D_BOOST anode |
| EN | `B+1` (tie high for always-on) |
| FB | Junction of R_UPPER / R_LOWER feedback divider |
| (implicit) | D_BOOST cathode = VOUT = `B+3` power symbol |

**Feedback resistors for 10.8V:**

```
B+3 ──[R_UPPER: 169kΩ E96]────┬──[R_LOWER: 10kΩ]── GND
                               │
                              FB (pin 5)
```

VOUT = 0.6 × (1 + 169/10) = 10.74V ✓

**Remaining components:**

- L1: 4.7µH ±20%, 1.5A saturation, CDRH4D28 (4×4mm, 2.8mm max height)
- D_BOOST: SS14, SMA package — anode to SW/L1, cathode to B+3
- C_BOOST_IN: 10µF 10V X5R (0805) — VIN to GND
- C_BOOST_OUT: **47µF 16V X5R** (0805) — VOUT to GND
  - Use 47µF not 22µF — accounts for X5R DC bias derating at 10.8V
- PWR_FLAG on `B+3`
- Schematic note: `SW node: high dV/dt at 1.2 MHz. Keep trace short (<3mm).
  Minimum 2mm clearance to all FG, ADC, and DAC signal traces.`

---

### U3 — MCP1700T-3302E/TT LDO (SOT-23-3), 6V → 3.3V

| MCP1700 Pin | Connection |
|---|---|
| VIN | `B+1` power symbol |
| GND | GND |
| VOUT | `+3V3` power symbol |

**Decoupling:**

- C_LDO_IN: 1µF 10V X5R (0402) + 10µF 10V X5R (0805) — VIN to GND
- C_LDO_OUT: 1µF 10V X5R (0402) + 10µF 10V X5R (0805) — VOUT to GND
- PWR_FLAG on `+3V3`

---

## Sheet 4 — Microcontroller (STM32G0C1)

**Purpose:** STM32G0C1KCU6 and all support components. Place the STM32 in the
centre and work outward.

### U1 — STM32G0C1KCU6 (UFQFPN32)

Use the KiCad `MCU_ST_STM32G0` library symbol `STM32G0C1KCUx`, copied to
`DSR-1.kicad_sym` and renamed `STM32G0C1KCU6`. The UFQFPN-32 package has
**1 VDD and 1 VSS** in the 32-pin K package. VDD and VDDA share one pin;
VSS and VSSA share one pin. Expose the thermal pad (EP) as described below.

> **⚠ Verify VDDIO2 against STM32G0C1 datasheet.** The G0B1 GP variant had no
> separate VDDIO2 pin. The G0C1 may differ — confirm in the STM32G0C1 UFQFPN-32
> pinout table. If a separate VDDIO2 pin is present, add 100nF X5R 0402 +
> 4.7µF X5R 0402 decoupling to `+3V3` and update this guide.

**UFQFPN-32 GP (_KxU) confirmed pin map — DS13564 Rev 5:**

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
| 15 | PB0 | No-connect |
| 16 | PB1 | No-connect |
| 17 | PB2 | No-connect |
| 18 | PA8 | `CC1` / UCPD1_CC1, UCPD1_DBCC1 |
| 19 | PA9 | `CC2` / UCPD1_CC2 |
| 20 | PC6 | No-connect |
| 21 | PA10 | No-connect / spare |
| 22 | PA11 [PA9] | `USB_DM` |
| 23 | PA12 [PA10] | `USB_DP` |
| 24 | PA13 | `SWDIO` |
| 25 | PA14-BOOT0 | `SWDCLK` |
| 26 | PA15 | No-connect |
| 27 | PB3 | No-connect |
| 28 | PB4 | No-connect |
| 29 | PB5 | No-connect |
| 30 | PB6 | `DEBUG_TX` / USART1_TX |
| 31 | PB7 | No-connect |
| 32 | PB8 | No-connect |
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
> needed. (DS13564 Figure 4, GP version _KxU confirmed.)

> **PA4 (DAC1_OUT1) is not used for motor drive.** Q601 (2SB1013 PNP) on
> Ver. 1.0 boards has its emitter at B+3 (10.8V); the base operating range
> exceeds the 3.3V DAC output range. Motor drive uses TIM3 PWM on PA6 through
> the NPN level-shift stage on Sheet 5. PA4 may be left unconnected with a
> no-connect flag or reserved as a spare ADC/DAC pin.

**Unused pins:** Place a No-Connect flag (`Q`) on every unused GPIO pin.

---

### Decoupling Network

**VDD/VDDA decoupling (pin 4 — combined supply in 32-pin package):**
- 100nF 10V X5R 0402 — power supply decoupling
- 100nF 10V C0G 0402 — analog noise filtering (C0G/NP0, lower noise than X5R)
- 1µF 10V X5R 0402 — bulk bypass

All three caps connect between pin 4 and GND. The C0G cap should be placed
closest to the pin on the PCB as it serves the ADC inputs.

> **VDDIO2:** Verify whether the STM32G0C1KCU6 UFQFPN-32 exposes VDDIO2 as a
> separate pin. If present, add 100nF X5R 0402 + 4.7µF X5R 0402 between
> VDDIO2 and GND. If absent (combined with VDD), no additional decoupling needed.

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

### Hierarchical Labels for Sheet 4

**Inputs:**
- `USB_DP`, `USB_DM`, `CC1`, `CC2` ← Sheet 2
- `FG_IN`, `RV601_WIPER`, `RV602_WIPER`, `RV603_WIPER`, `MOTOR_EN_MON` ← Sheet 5
- `SPEED_TUNE_SW`, `SWDIO`, `SWDCLK` ← Sheet 6

**Outputs:**
- `MOTOR_PWM` → Sheet 5

---

## Sheet 5 — Signal Conditioning

**Purpose:** Every analog interface between the STM32 and the machine. Conditions
incoming machine-voltage signals down to 3.3V; level-shifts STM32 PWM output up
to drive the machine's motor transistor.

### FG Signal Path (J1 Pin 1 → PA0)

FG901 GP2S22AB is an open-collector optical interrupter. Its output is pulled to
B+1 (~6V) through a resistor on the WM-D6C main board. The swing is approximately
0 to 5.9V — exceeding the STM32's 3.6V absolute maximum input.

```
FG_RAW (from Sheet 6, J1 pin 1)
    │
   [R3: 10kΩ 1% 0402]
    │
   [R4: 22kΩ 1% 0402] ──── GND        ← voltage divider lower leg
    │
   ├──[D_FG1: BAT54, anode→node, cathode→+3V3]   ← high-side clamp
   ├──[D_FG2: BAT54, anode→GND, cathode→node]    ← low-side clamp
   │
   [C7: 10pF C0G 0402] ──── GND        ← motor-brush noise filter
    │
   FG_IN (Hierarchical Label → Sheet 4, PA0)
```

Divider math: V_node = V_FG × 22/(10+22) = V_FG × 0.6875
At 5.9V swing: 5.9 × 0.6875 = 4.06V → BAT54 clamps to ≤3.6V ✓

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
MOTOR_PWM (from Sheet 4, PA6 TIM3_CH1, 15.6 kHz)
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
   Q601_BASE (Hierarchical Label → Sheet 6, J1 pin 2)
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
MOTOR_EN (from Sheet 6, J1 pin 3)
    │
   [R_EN1: 10kΩ 1% 0402]
    │
   [R_EN2: 22kΩ 1% 0402] ──── GND
    │
   MOTOR_EN_MON (Hierarchical Label → Sheet 4, PA5)
```

Divider: 22/(10+22) = 0.6875. At 4.4V playback: 3.025V ≤ 3.3V ✓
At auto-off (low): ≈0.2V, below VIL ✓

Schematic note: `MOTOR_EN tap point: IC601 footprint pin 7 pad on WM-D6C main
board. Verify voltage on bench before connecting. See signal-chain-analysis.md §3.`

---

### ADC Inputs — RV601, RV602, RV603 (J1 Pins 4, 5, 6 → PA1, PA2, PA3)

Each potentiometer wiper may be referenced to B+1 and could exceed 3.3V.
The 100Ω series resistor plus BAT54 clamp is the minimum safe configuration.

**⚠ Measure wiper voltage on bench before first power-on.** If any wiper exceeds
3.3V at B+1 = 6V, add a resistor divider before the clamp.

```
RV601 (from Sheet 6, J1 pin 4)
    │
   [R_ADC1: 100Ω 5% 0402]              ← source impedance limit + protection
    │
   ├──[BAT54, anode→node, cathode→+3V3]  ← overvoltage clamp
   ├──[BAT54, anode→GND, cathode→node]   ← undervoltage clamp
    │
   RV601_WIPER (Hierarchical Label → Sheet 4, PA1/ADC_IN1)
```

Repeat identically for RV602 → PA2 and RV603 → PA3.

Schematic note: `⚠ VERIFY WIPER VOLTAGE BEFORE POWER-ON. See signal-chain-
analysis.md §4 for divider formula. If wiper exceeds 3.3V, add R/2R divider
before the 100Ω series resistor. Adjust ADC scaling in firmware to match.`

---

### Hierarchical Labels for Sheet 5

**Inputs (from other sheets):**
- `MOTOR_PWM` ← Sheet 4 (PA6)
- `FG_RAW`, `MOTOR_EN`, `RV601`, `RV602`, `RV603` ← Sheet 6

**Outputs (to other sheets):**
- `FG_IN` → Sheet 4 (PA0)
- `RV601_WIPER`, `RV602_WIPER`, `RV603_WIPER` → Sheet 4
- `MOTOR_EN_MON` → Sheet 4
- `Q601_BASE` → Sheet 6 (J1 pin 2)

---

## Sheet 6 — Connectors

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
+3V3 ──[S601 switch]── SPEED_TUNE_SW (→ Sheet 4, PA7)
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

Run Tools → Annotate Schematic with hierarchical annotation:

| Sheet | Reference designator start |
|---|---|
| Sheet 2 (Power Input) | R1, C1, U1, D1 |
| Sheet 3 (Power Mgmt) | R101, C101, U101, L101, D101 |
| Sheet 4 (MCU) | R201, C201, U201 |
| Sheet 5 (Signal) | R301, C301, Q301, D301 |
| Sheet 6 (Connectors) | J1, J2, TP1 |

---

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
| Power pin not driven | Missing PWR_FLAG | Add PWR_FLAG on B+1, B+3, +3V3 at source sheet |
| Pin unconnected | Unused STM32 GPIO | Add No-Connect flag (`Q`) |
| Net has no driver | Hierarchical label not imported | Right-click sub-sheet → Import Sheet Pin |
| Conflicting net | Duplicate label names | Ensure all labels are unique or intentionally shared |

Target: **zero errors** before moving to PCB layout. Warnings on properly-flagged
DNP open pins are acceptable.

---

## First Commit After Schematic Completion

```
git add hardware/kicad/
git commit -m "Complete DSR-1 Rev A schematic — all 6 sheets, ERC clean, pre-layout

- Sheet 1: top-level hierarchy
- Sheet 2: power input — Variant A (USB-C/IP2721) and Variant B (barrel/LTC4359)
- Sheet 3: power management — MT3608 boost (10.8V) and MCP1700 LDO (3.3V)
- Sheet 4: STM32G0C1KCU6 decoupling, pin assignments; motor PWM on PA6 (TIM3_CH1)
- Sheet 5: signal conditioning — FG divider/clamp, NPN motor drive, ADC inputs
- Sheet 6: J1 harness, J2 SWD, S601 rewire, test pads
- Rev A targets Ver. 1.0 CX20084 boards (1984–2001)
- ERC: 0 errors"

git tag -a hw-v0.1-schematic-complete -m "Schematic complete, ERC clean, ready for layout review"
git push origin main --tags
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

*Document version: 0.2 — Matches DSR-1 hardware Rev A*
*Replaces: initial draft (v0.1) — corrected motor drive topology, full production run scope added, sheet filenames aligned with actual KiCad project*
