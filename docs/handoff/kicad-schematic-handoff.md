# DSR-1 KiCad Schematic Capture — Handoff

**For:** the agent continuing Rev A schematic capture (DeepSeek)
**Written:** 2026-11-09
**Repository:** `/mnt/Deep_Forge/Projects/wmd6c-servo` (GitHub `ChipCreates/wmd6c-servo`)
**Goal:** bring both KiCad 10 schematics — the **Servo Control Board** and the
**Power Board** — to an ERC-clean, fully connected Rev A state that matches the locked
architecture, so PCB layout can start.

> **Update 2026-11-09:** the initial capture of both boards has been done (ERC clean,
> netlist-verified) by generator scripts in `tools/`. See
> `docs/handoff/kicad-schematic-worklog.md` for the current state and open issues before
> continuing; §3 below describes the files as they were *before* that work.

Read this whole file before editing anything. Section 6 (how to edit KiCad files as
text) contains rules that were verified against `kicad-cli` on this machine; follow
them exactly.

---

## 1. What the project is

DSR-1 replaces the dead CX20084 capstan-servo IC (IC601), the CP304 DC-DC converter,
and the original battery/barrel-jack power path in a Sony WM-D6C cassette recorder
(serial 72795, PCB `C11-494-12`, "former type" board). It is two boards:

- **Servo Control Board** — sits in the CP304 cavity. Receives Sony `B+1` (6.0 V),
  makes `+3V3` (MCP1700) and `B+3` ≈ 10.8 V (MT3608 boost), runs an STM32G0C1KCU6
  digital PI servo, owns USB D+/D−/CC1/CC2, SWD, and the five-LED battery gauge.
- **Power Board** — sits behind the battery bay. USB-C 5 V only → BQ24074 charger →
  three 803040 LiPo cells in parallel (1S3P) behind DW01A + FS8205A protection →
  TPS63070 buck-boost → `B+1` 6.0 V injected at the old battery-terminal pads.

Firmware (`firmware/`) already builds for the STM32G0C1KCU6. The MCU pin allocation is
locked; firmware depends on it. **Do not change pin assignments.**

---

## 2. Document authority (read in this order; earlier wins on conflict)

| # | Document | Use it for |
|---|---|---|
| 1 | `docs/hardware/kicad-servo-board-schematic-guide.md` (2026-06-08) | **Servo Board: authoritative.** Board boundary, zones A–J, pin table (§8), test points, ERC targets. |
| 2 | `docs/hardware/kicad-power-board-guide.md` (2026-06-08) | **Power Board: authoritative.** Zones A–E, net names, protection rules, ERC targets. |
| 3 | `docs/hardware/connectors-sheet.md` | Machine-side **pin numbers** (CP304 J1 pins 1/2/3, CX20084 pads 7/13/14/15/16, LED harness). Its signal *directions* are superseded where they disagree with #1. |
| 4 | `docs/stm32g0c1-pin-allocation.md` | Pin-function background. Must agree with guide #1 §8. |
| 5 | `docs/theory/signal-chain-analysis.md`, `docs/hardware/wmd6c-interface-contract.md` | Rationale for component values and Sony-side electrical limits. |
| 6 | Manufacturer datasheets in `hardware/datasheets/` | Reference circuits, pin functions, value formulas. |

**Superseded — do not follow where they conflict with #1/#2:**

- `docs/rev-a-schematic-plan.md` — proposes 10 hierarchical sheets. The locked guides
  require **one flat D-size sheet per board.**
- `docs/hardware/kicad-schematic-guide.md` — older single-guide version; still useful
  for the Annotation Scheme and Symbol Properties Checklist sections only.
- `PROJECT_STATUS.md` "Variant A / Variant B" (USB-C PD 9 V, barrel jack, LTC4359) —
  removed from Rev A. No PD, no barrel jack, no 9 V anywhere.
- `docs/review/power-rev-a-findings.md` — written against the old wall-power design.
  Its MT3608 boost-topology correction (§3.1) is still valid; everything about the
  barrel bridge, polyfuse, TVS, and AP63203 is moot because those blocks are removed.

### Known conflicts, already resolved

| Topic | Resolution |
|---|---|
| B+1 direction at CP304 J1 pin 1 | **Into** the Servo Board (Power Board makes B+1). No series diode blocking B+1 entry (guide #1 §5). Ignore the connectors-sheet "1N5819 series Schottky" note. |
| Net names | Use guide #1/#2 names: `MOTOR_EN_RAW`/`MOTOR_EN_MON`, `RV601_RAW`→conditioned→`RV601_WIPER` (to PA1; same pattern for 602/603), `CHG_STAT_N`, `PGOOD_N`, `SPEED_TUNE_SW`, `+3V3`. Older docs say `MOTOR_EN`, `CHG_STAT`, `PGOOD`, `+3.3V`. |
| STM32 datasheet number | DS13564 Rev 5 (`hardware/datasheets/DS_stm32g0c1cc.pdf`). Any "DS13560" you see elsewhere is wrong. |

---

## 3. Current state of the files (verified 2026-11-09)

### 3.1 Servo Board — `hardware/kicad/DSR-1/`

- `DSR-1.kicad_sch` — the working **flat root** (D-size, 87 symbols, 225 wires). It is a
  flattened copy of the old child sheets with references renumbered. It still contains
  the **old wall-power front end that must be removed**: J3 USB-C receptacle wired as a
  power input, U7 IP2721 (PD trigger), J4 barrel jack, D13–D16 SS14 bridge, F2 polyfuse,
  D17 SMBJ12A, U6 AP63203 + L3, and the `9V_PD` net — which is **shorted to GND**.
- Keep/rebuild: U10 STM32G0C1KCUx, U8 MT3608 (+L4, D18, R18 169 k + one of the 10 k R19–R21 as divider, C22),
  U9 MCP1700, D12 USBLC6-2SC6Y, Q2 MMBT3904 output stage, D19–D22 BAT54S clamps,
  signal-conditioning resistors/caps.
- The MT3608 block is **miswired** (FB on the inductor, SW on the divider, D18 reversed,
  inductor not fed from B+1). Rewire it to guide #1 §7.
- It uses **hierarchical labels on a root sheet**, which connect to nothing (9
  `isolated_pin_label` ERC hits). Replace them with ordinary `label`s.
- **Missing entirely:** CP304 power connector, Sony servo harness connector, pots/S601
  connector, LED-board harness, Power Board telemetry connector, SWD header, NRST cap,
  BOOT0 pull-down, CC Rd resistors, all test points, LED-drive circuitry.
- Orphan files `Power input zone`, `Power management`, `Signal conditioning`,
  `STM32G0C1 and decoupling`, `Connectors`, `mcu` (empty) `.kicad_sch` are **not
  referenced by the root**. Use them only as visual reference; delete them at the end
  once nothing unique remains (guide #1 §3 step 8).
- `DSR-1.kicad_sym` — project symbol library (AP63203WU-7, B+1, B+3, BAT54S, IP2721,
  MCP1700x-330xxTT, MMBT3904, MT3608, SS14, USBLC6-2SC6Y, USB_C_Receptacle).
  `sym-lib-table` registers it as `DSR-1`.
- `DSR-1.kicad_pcb` — empty. Out of scope.
- Baseline ERC (before libraries installed): **142 violations** — 69 lib_symbol_issues,
  42 pin_not_connected, 17 footprint_link_issues, 9 isolated_pin_label,
  3 power_pin_not_driven, 1 pin_to_pin, 1 multiple_net_names (the 9V_PD/GND short).

### 3.2 Power Board — `hardware/kicad/Power-Board/wm-d6c-power-board/`

- `wm-d6c-power-board.kicad_sch` — D-size skeleton: **7 symbols, zero wires.**
  U1 `Battery_Management:BQ24074RGT`, U2 `TPS63070RNMR:TPS63070RNMR`,
  J1/J2/J5 `B2B-PH-SM4-TB` (cell connectors), J3/J4 `Conn_01x01` (USB VBUS/GND pads).
- **No `sym-lib-table`.** `TPS63070RNMR` and `B2B-PH-SM4-TB` exist only in the
  schematic's embedded cache. Create a project library (see §6.6) or use stock symbols.
- Missing: DW01A, FS8205A, all passives, telemetry connector, battery-sense divider,
  B+1 output pads, test points, PWR_FLAGs.
- Baseline ERC: **63 violations.**

### 3.3 Libraries — IMPORTANT

The stock KiCad symbol/footprint libraries are **not installed** on this machine
(only the `kicad` package is). That is why ERC shows lib_symbol_issues and why
`power:`, `Device:`, `Connector_Generic:` etc. cannot be resolved. The user must run:

```bash
sudo pacman -S kicad-library
```

After that, `/usr/share/kicad/symbols/*.kicad_sym` and
`/usr/share/kicad/template/sym-lib-table` exist. Verify before Phase 1. Do not try to
install it yourself.

Custom footprint libraries referenced by symbols (`AP63203WU-7`, `IP2721`,
`SMBJ12A-13-F`, `USBLC6-2SC6Y`, `TPS63070RNMR`, `B2B-PH-SM4-TB`) are **not in the
repo**. Footprints are not required for this phase (see §5), so leave those fields as
they are or empty; just do not invent footprint library names.

### 3.4 Datasheets available locally (`hardware/datasheets/`, git-ignored)

STM32G0C1 (DS13564), RM0444, PM0223, AN4879, USBLC6-2SC6Y, BQ24074, TPS63070,
MCP1700, MT3608 (file `87b648f6991783104e708a12c210cc63.pdf`), MMBT3904L, BAT54S
(`ds11005.pdf`), DW01x family (covers DW01A-G), FS8205A, and the Sony WM-D6C/TC-D6C
service manuals (`sony_wm-d6c_tc-d6c_*.pdf`). Read them with `pdftotext -layout`.

---

## 4. Decisions the user still owns — do not resolve these yourself

| ID | Open question | What to do meanwhile |
|---|---|---|
| D1 | **Where is the physical USB-C receptacle**, and how do D+/D−/CC1/CC2 reach the Servo Board while VBUS/GND go to the Power Board? | Servo Board: a generic `Conn_01x05` "USB signal entry" (`USB_DP`, `USB_DM`, `CC1`, `CC2`, GND) with ESD at that entry. Power Board: `Conn_01x02` "USB power entry" (`VBUS_5V`, GND). Flag it in the log. |
| D2 | Connector families and pitch for every harness | Use `Connector_Generic:Conn_01xNN` with an **empty Footprint** field. Guide #1 §15 forbids claiming a family yet. |
| D3 | Bench-dependent values: FG divider, Q601 base network (R7/R8/R9), MOTOR_EN divider, pot dividers, LED-drive buffers vs direct drive | Draw the topology from the guides with the existing provisional values; put `PROVISIONAL – bench` in each part's Value or a `Notes` property. |
| D4 | Final DW01A/FS8205A sourcing and whether cells carry their own protection PCBs | Draw the datasheet reference circuit; note it. |

If you hit any other architecture-level choice not covered by the guides, stop and ask
the user rather than picking one.

Values you **may** compute (show the formula and datasheet section in the log):
TPS63070 input/output caps, inductor, VAUX cap, mode pins; BQ24074 ISET/ILIM/ITERM/TMR
and TS handling (guide #2 §6 says the old "800 Ω = 2 A" ILIM claim is wrong); CC Rd
= 5.1 kΩ; battery-sense divider ratio; decoupling.

---

## 5. Scope

**In scope:** schematic symbols, connectivity, values, reference designators, power
flags, no-connect flags, test points, title blocks, ERC cleanup, both boards.

**Out of scope:** PCB layout, footprint assignment/creation, 3D models, BOM ordering,
firmware changes, changing the MCU pin allocation, editing the theory/architecture
docs (except the work log you create — §8).

---

## 6. How to edit KiCad 10 schematics as text (verified)

There is no GUI available to you and no reliable Python API for eeschema. Edit
`.kicad_sch` S-expressions directly and verify every change with `kicad-cli`.

**Build a small helper tool first** (suggested: `tools/kicad_sch.py`) that can:
parse S-expressions, copy a library symbol into `lib_symbols`, place a symbol
instance, compute its pin endpoints, add wires/labels/junctions/no-connects/power
symbols with fresh UUIDs, and export + parse the netlist. Write it once, test it
(§6.4), then use it for all edits. Hand-editing hundreds of coordinates will fail.

### 6.1 File facts

- Schematic format token: `(version 20260306)`, `(generator_version "10.0")`.
  Symbol libraries: `(version 20251024)`. Do not change these.
- Root sheet UUID of the Servo Board: `c0f3f4ab-99e0-4b30-98cc-a10c83648838`. Every
  symbol's `(instances (project "DSR-1" (path "/<root uuid>" (reference "R25") (unit 1))))`
  must use that path and a reference identical to its `Reference` property. Read the
  Power Board's own root UUID from its file header; its project name is
  `wm-d6c-power-board`.
- Units are millimetres. Y grows **downward** on the sheet. Keep every coordinate on
  the 1.27 mm grid.
- All UUIDs must be unique (`uuid.uuid4()`).
- Files in the working tree currently have CRLF line endings. Preserve whatever a file
  already uses when you write it back.
- Copy the exact shape of an existing instance (e.g. R25 in `DSR-1.kicad_sch`)
  including `body_style`, `exclude_from_sim`, `in_bom`, `on_board`, `in_pos_files`,
  `dnp`, the five standard properties, one `(pin "N" (uuid ...))` per pin, and
  `(instances ...)`.

### 6.2 Embedding library symbols

Each `lib_id` used must have a definition inside the schematic's `(lib_symbols ...)`:

- Top-level name is `"Library:Name"` (e.g. `"DSR-1:MT3608"`, `"Device:R"`), but its
  sub-units keep the **bare** name: `(symbol "MT3608_0_1" ...)`, `(symbol "MT3608_1_1" ...)`.
  Suffix `_U_B` = unit U (0 = shared by all units), body style B.
- Source: `/usr/share/kicad/symbols/<Library>.kicad_sym` (after `kicad-library` is
  installed) or the project's `.kicad_sym`.
- Stock libraries often define a part as `(extends "Parent")`. The embedded copy must
  be **flattened**: take the parent's graphics and pins, rename its sub-units to the
  child name, and use the child's properties. KiCad will not resolve `extends` inside
  a schematic cache.

### 6.3 Pin endpoint math — verified rule

A pin's connection point is its `(at x y angle)` in the library (the outer end; the
`length` runs inward). To find where it lands on the sheet for an instance
`(at SX SY ROT)` with optional `(mirror x|y)`:

```python
from math import radians, cos, sin

def pin_xy(SX, SY, rot_deg, mirror, px, py):
    x, y = px, -py                               # library is Y-up, sheet is Y-down
    r = radians(rot_deg)
    x, y = x*cos(r) + y*sin(r), -x*sin(r) + y*cos(r)   # rotate counter-clockwise on screen
    if mirror == 'x':   y = -y                   # (mirror x) flips top-to-bottom
    elif mirror == 'y': x = -x                   # (mirror y) flips left-to-right
    return round(SX + x, 2), round(SY + y, 2)    # rotation first, THEN mirror
```

This was proven by placing MMBT3904, USBLC6-2SC6Y, MT3608 and STM32G0C1KCUx at all
4 rotations × {none, mirror x, mirror y}, putting a label at each predicted point, and
checking `kicad-cli`'s netlist: every pin joined the predicted net (the only
exceptions were a `no_connect`-type pin, which never joins nets, and STM32 pin 5/EP
pin 33, which share one location by design). Other orderings fail.

### 6.4 Self-test your helper before touching the project

Reproduce that proof in a scratch directory: generate a schematic with one asymmetric
symbol at all 12 orientations, a uniquely named label on every computed pin endpoint,
export the netlist, and assert each label net contains exactly the intended
`(ref, pin)`. Only proceed when it passes 100%.

### 6.5 Connectivity rules

- A connection exists only where connection points **coincide exactly**: pin end ↔
  wire end, wire end ↔ wire end, label anchor ↔ wire end or pin end, power-symbol pin
  ↔ wire end or pin end.
- Never rely on a wire passing over a pin or another wire's middle. **Split wires** so
  every join is endpoint-to-endpoint, and add a `(junction (at x y) ...)` wherever
  three or more endpoints meet.
- Preferred style ("routing"): a short orthogonal wire stub (2.54–5.08 mm) off each pin
  plus a local `label` at the stub end for any net leaving its zone; real wires within
  a zone for tightly coupled parts (regulator + caps + divider, divider + clamp + MCU
  pin). Wires horizontal/vertical only.
- Rails use power symbols: `power:GND`, `power:+3V3`, and the project's `DSR-1:B+1`,
  `DSR-1:B+3`. A power symbol's single pin sits at its `(at)` point.
- Every intentionally unused pin gets `(no_connect (at x y) (uuid ...))` at its end.
- Each externally sourced rail gets one `power:PWR_FLAG` at its entry
  (Servo: B+1 at the CP304 connector, GND; Power Board: VBUS_5V, GND).

### 6.6 Library tables

- Servo Board: add any new custom symbols to `DSR-1.kicad_sym` (same format as the
  existing entries). Prefer stock symbols when they exist.
- Power Board: create `wm-d6c-power-board.kicad_sym` + `sym-lib-table` in its project
  folder (copy the Servo Board's table format) for TPS63070, B2B-PH-SM4-TB, DW01A,
  FS8205A if no stock symbol fits. Check the stock `Battery_Management`,
  `Regulator_Switching` and `Transistor_FET` libraries first.
- Validate libraries with `kicad-cli sym upgrade -o /tmp/x.kicad_sym <lib>` (it parses
  the file; "was not updated" means it is already current).

### 6.7 Verification commands (run after every zone)

```bash
cd hardware/kicad/DSR-1
kicad-cli sch erc --format json --severity-all -o /tmp/dsr1-erc.json DSR-1.kicad_sch
kicad-cli sch export netlist --format kicadxml -o /tmp/dsr1.xml DSR-1.kicad_sch
kicad-cli sch export pdf -o ../../schematic-pdfs/DSR-1.pdf DSR-1.kicad_sch
```

(Power Board: same commands on `wm-d6c-power-board.kicad_sch`.)

- If `kicad-cli` fails to load the file, your S-expression is malformed — fix before
  anything else.
- For each zone, write the **intended** connectivity (net → list of `ref.pin`) in the
  work log, then assert it against the exported XML netlist with your helper. ERC alone
  will not catch a pin on the wrong net.
- KiCad has no CLI annotator: assign references yourself and keep them unique per board.

---

## 7. Work plan

**Phase 0 — Environment.** Confirm `kicad-cli version` = 10.0.8 and that
`/usr/share/kicad/symbols/power.kicad_sym` exists (if not, stop and ask the user to
install `kicad-library`). Create branch `schematic-rev-a` from the current `main`.
Save baseline ERC JSON and netlist for both boards to `hardware/kicad/baseline/`.

**Phase 1 — Tooling.** Write `tools/kicad_sch.py` and pass the §6.4 self-test.

**Phase 2 — Servo Board** (`DSR-1.kicad_sch`), guide #1, one zone per commit:

1. Cleanup (guide #1 §3): delete J3/U7/J4/D13–D16/F2/D17/U6/L3, the `9V_PD` net and all
   their wires/labels; convert hierarchical labels to local labels.
2. Zone A — CP304 power interface: 3-pin connector (pin 1 `B+1` in, 2 GND, 3 `B+3` out),
   PWR_FLAG, TP_B1/TP_B3/TP_GND.
3. Zone B — MCP1700 `+3V3` with datasheet caps, TP_3V3.
4. Zone C — rewire MT3608 boost to guide #1 §7 (169 k / 10 k divider).
5. Zone D — STM32: pin table guide #1 §8 exactly; 100 nF + 1 µF on pin 4; EP to GND;
   NRST 100 nF; PA14-BOOT0 10 k pull-down; no-connects on PB9, PC14, PC15, PA4, PA10.
6. Zone E — USB signal entry (D1 placeholder), USBLC6 ESD, 5.1 k Rd on CC1/CC2, TPs.
7. Zones F, G, H — FG conditioning, MMBT3904 motor-drive stage, MOTOR_EN divider,
   RV601/602/603 conditioning, S601 input; Sony servo harness connector (CX20084 pads
   13/15/7/14 per connectors sheet) and pots/S601 connector.
8. Zone I — Power Board telemetry connector (`VBAT_SENSE`, `VBAT_SENSE_EN`,
   `CHG_STAT_N`, `PGOOD_N`, GND) with pull-ups to `+3V3` and ADC filter cap.
9. Zone J — LED battery monitor: `BATT_LED1–5`, `S801_BATT`, LED harness (7 pins per
   connectors sheet J4). Mark drive circuitry PROVISIONAL (D3).
10. SWD header (GND, SWDCLK, SWDIO, NRST, +3V3 ref) and all guide #1 §17 test points.
11. Delete the orphan child `.kicad_sch` files once nothing unique remains.

**Phase 3 — Power Board** (`wm-d6c-power-board.kicad_sch`), guide #2, one zone per
commit: USB power entry, BQ24074 with all pins handled, 1S3P cell connectors, DW01A +
FS8205A protection (raw cell negative never reaches GND except through the FETs),
TPS63070 with every pin handled, B+1/GND machine-output pads, telemetry connector with
an always-ADC-safe `VBAT_SENSE`, test points (guide #2 §12).

**Phase 4 — Cross-board check.** Telemetry connector pin order and net names identical
on both boards; `B+1` produced only on the Power Board; no `9V_PD`, IP2721, barrel
jack, AP63203 left anywhere.

**Phase 5 — Finish.** ERC: zero errors; every remaining warning explained in the log.
Export PDFs of both schematics to `hardware/schematic-pdfs/`. Final report.

---

## 8. Deliverables

1. Edited `DSR-1.kicad_sch` and `wm-d6c-power-board.kicad_sch` (plus any library and
   `sym-lib-table` files).
2. `tools/kicad_sch.py` (and its self-test).
3. `docs/handoff/kicad-schematic-worklog.md` — per zone: what was drawn, intended
   netlist, verification result, computed values with formula + datasheet section,
   PROVISIONAL items, ERC warnings you accepted and why, open questions for the user.
4. Final ERC JSON for both boards and schematic PDFs in `hardware/schematic-pdfs/`.

## 9. Git rules

- Work on branch `schematic-rev-a`. One commit per zone, message
  `Servo Board: <zone>` / `Power Board: <zone>`, ending with your own attribution line.
- Never commit anything under `hardware/datasheets/` except `DATASHEET_LINKS.md`
  (it is git-ignored — keep it that way).
- Do not push, open PRs, rewrite history, or modify `main`.
- Do not commit `/tmp` outputs, KiCad backups, or `*-backups/` folders.

## 10. Housekeeping notes (low priority, don't block on them)

- `hardware/kicad/DSR-1/DSR-1.bak` is tracked even though `.gitignore` intends to ignore
  `*.bak` (the pattern only matches one directory level). Leave it unless asked.
- The USBLC6 "VBUS" pin tied to `+3V3` raises a `pin_to_pin` ERC warning; that tie is
  normal practice for this ESD part — justify it in the log rather than changing it.
- `firmware/src/usb_cdc.c` sets AF10 on PA11/PA12; that is a firmware item and out of
  scope here.
