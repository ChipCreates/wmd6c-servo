# WM-D6C Power Board - KiCad 10 Schematic Guide

**Project:** `hardware/kicad/Power-Board/wm-d6c-power-board/`  
**Tool:** KiCad 10.x  
**Scope:** Power Board schematic only. One flat schematic sheet.

This guide is for the board you are working on now: the separate WM-D6C Power Board.
It owns the USB-C input, charging/power-path circuitry, cell protection, the B+1
generation path, and any downstream power rails that have been moved out of the
`DSR-1` Servo Control Board project.

The Servo Control Board guide is now only the MCU, signal-conditioning, and connector
guide. Use this file when editing `wm-d6c-power-board.kicad_sch`.

---

## Schematic Status

The current Power Board KiCad file appears to contain the moved legacy power sheets:

- USB-C PD / IP2721 front end,
- barrel-jack variant,
- AP63203 buck regulator,
- MT3608 B+3 boost,
- MCP1700 3.3V LDO.

That is useful as a migration starting point, but it is not the target primary
battery Power Board. For the battery-integrated build, the Power Board target is:

```text
USB-C 5V VBUS
  -> BQ24074 power-path charger
  -> V_SYS
  -> TPS63070 buck-boost
  -> B+1 = 6.0V
  -> WM-D6C battery-terminal injection point

3x LiPo cells in parallel
  -> DW01 + dual-FET pack protection
  -> BQ24074 BAT / system handoff path
```

If B+3 and +3.3V are now owned by this Power Board project, keep those blocks here
and remove them from the Servo Control Board project. If they are not physically on
this board, mark them DNP or remove them from this schematic.

---

## Sheet Layout

Use one flat schematic sheet with visual regions only:

```text
+ USB-C Input + Protection +----+ Charger / Pack +-------+ B+1 Converter +----+ Output / Service +
| J_USB, VBUS, CC Rd, ESD   |    | BQ24074, cells, DW01   | | TPS63070 6V  |    | battery tabs,    |
| optional USB D+/D-        |    | FS8205, status lines   | | test points  |    | J_BBL, test pads |
+---------------------------+----+------------------------+--------------------+------------------+
```

Do not create KiCad hierarchy for this project. Same-name net labels on this sheet
are the connection mechanism.

---

## Critical USB-C Rule

`VBUS` from the USB-C connector is **not USB-C PD** by itself.

For the primary battery Power Board:

- `VBUS` is default USB-C 5V sink power.
- The board should present a plain Type-C sink on CC1/CC2.
- The BQ24074 charger input is a 5V input. Do not feed it from a 9V PD contract.
- `9V_PD` is only valid in an optional wall/PD variant with an active PD controller
  or UCPD stack negotiating on CC1/CC2.

Practical schematic consequence:

- DNP/remove IP2721 and `9V_PD` for the battery build.
- Add CC1 and CC2 Rd pull-downs for a plain 5V sink, unless a Type-C controller is
  intentionally used.
- Use `VBUS` or `VBUS_5V` for the connector bus. Do not name the 5V net `9V_PD`.

---

## Zone A - USB-C Input

**J_USB - USB-C receptacle**

- VBUS pins -> `VBUS` / `VBUS_5V`.
- GND pins -> GND.
- CC1, CC2 -> sink advertisement strategy:
  - primary battery build: Rd pull-down to GND on each CC pin,
  - PD variant only: route to PD controller/UCPD instead.
- D+ / D-:
  - route through ESD if USB CDC/service comes through the Power Board USB-C port,
  - otherwise leave the service interface to a Servo Board bench header.
- Shield -> chosen chassis/system-ground strategy; do not leave it accidental.

**Protection and filtering**

- Put input ESD/TVS protection at the connector.
- Add VBUS bulk capacitance appropriate for the charger input.
- Add a test point on `VBUS`.
- Confirm no path can backfeed voltage onto the USB-C connector when USB is absent.

Schematic note:

```text
USB-C VBUS is 5V default Type-C sink power in the battery build.
No USB PD negotiation is used. Do not connect 9V_PD to the BQ24074 input.
```

---

## Zone B - Charger, Power Path, and Pack

**U_CHG - BQ24074 power-path charger**

- IN -> `VBUS` / `VBUS_5V`.
- SYS -> `V_SYS`.
- BAT -> protected pack node per the chosen DW01/dual-FET reference design.
- CHG_STAT -> board-to-board connector for Servo MCU status.
- PGOOD -> board-to-board connector for Servo MCU status.
- ISET resistor: start with about 1.1k for roughly 0.8A charge current.
- ILIM resistor: start with about 800 ohm for roughly 2A input current limit.
- TS pin: use a valid fixed divider or route to a real pack NTC. Do not leave it
  outside the allowed charger window.

Give the BQ24074 enough copper for heat. At 5V in and a partly discharged pack, the
linear charger can dissipate meaningful power.

**Cell connectors**

- Three 803040 LiPo cells in parallel are the target pack.
- Bring each cell into a serviceable connector or clearly documented pad pair.
- The cells must be voltage-matched before joining in parallel.
- Label the raw/protected pack nets so assembly cannot confuse pack-positive,
  protected-positive, pack-negative, and system ground.

**Pack protection**

- Use DW01 + FS8205-class dual FET protection for the 3P logical cell.
- Follow the selected reference topology exactly; FET orientation matters.
- The pack protection is the board-level backstop in addition to any on-cell PCMs.
- Add test points for `VBAT`, protected pack output, and GND.

Schematic note:

```text
BQ24074 provides DPPM power-path behavior: SYS powers the deck first, and charging
uses remaining USB input budget. USB removal hands SYS to the pack.
```

---

## Zone C - B+1 Regulator

**U_B1 - TPS63070 buck-boost**

The Power Board generates regulated B+1 for the machine. This replaces the legacy
AP63203 block.

- VIN / VINA / VINB -> `V_SYS`.
- VOUT -> `B+1`.
- FB divider -> 649k from `B+1` to FB, 100k from FB to GND.
- Inductor -> between the TPS63070 switch pins, per datasheet. This is not the same
  topology as a simple buck.
- EN -> defined soft-start or valid enable logic. Do not float EN.
- Add input/output capacitors per datasheet and keep the high-current loop compact.
- Add a `B+1` test point and PWR_FLAG at the regulator output.

Expected output:

```text
B+1 = 0.8V * (1 + 649k / 100k) = about 6.0V
```

Route `B+1` to the WM-D6C battery-terminal injection output. Size this path for the
full machine current plus motor surge. Keep it short and mechanically robust.

---

## Zone D - Downstream Rails, If Owned Here

If the Power Board project now owns the moved power-management blocks, keep them in
this schematic. Otherwise DNP/remove them and leave those rails on the Servo Control
Board.

**B+3 boost - MT3608**

- VIN -> `B+1`.
- Standard boost topology:
  - `B+1` -> inductor -> SW node.
  - SW node -> MT3608 SW pin and diode anode.
  - diode cathode -> `B+3`.
  - divider top -> `B+3`; divider midpoint -> FB; divider bottom -> GND.
- Existing target divider: 169k / 10k for about 10.8V.
- Existing target output cap: 47uF / 16V class.
- Keep SW node short and away from USB, ADC, FG, and sense traces.

**3.3V LDO - MCP1700**

- VIN -> `B+1`.
- VOUT -> `+3V3`.
- Use the required input/output capacitors close to the pins.
- Add `+3V3` and GND test points.

---

## Zone E - Interfaces and Test Points

**Machine power output**

| Net | Direction | Destination | Notes |
|---|---|---|---|
| `B+1` | Power Board -> WM-D6C | battery-positive injection point | Regulated 6.0V machine supply |
| GND | shared | battery-negative / chassis reference | Common return |

**Servo Control Board link**

| Net | Direction | Notes |
|---|---|---|
| `VBAT_SENSE` | Power Board -> Servo Board | Scaled pack voltage for ADC |
| `VBAT_SENSE_EN` | Servo Board -> Power Board | Enables/gates sense divider if implemented on Power Board |
| `CHG_STAT` | Power Board -> Servo Board | Charger status, low while charging |
| `PGOOD` | Power Board -> Servo Board | USB/input power-good status |
| `USB_DM` / `USB_DP` | bidirectional, optional | CDC/service path if USB-C is on Power Board |
| `CC1` / `CC2` | optional monitor/control | Only if Servo MCU participates in Type-C/UCPD |

**Required test points**

| Test point | Net | Purpose |
|---|---|---|
| TP_VBUS | `VBUS` | USB-C input voltage |
| TP_SYS | `V_SYS` | BQ24074 system output / TPS63070 input |
| TP_VBAT | `VBAT` | Pack voltage |
| TP_B1 | `B+1` | 6V machine output |
| TP_3V3 | `+3V3`, if present | Logic rail |
| TP_B3 | `B+3`, if present | Motor/support rail |
| TP_GND | GND | Scope/meter return |

---

## Current Schematic Cleanup Checklist

Use this as the first pass through `wm-d6c-power-board.kicad_sch`:

- Remove or DNP IP2721 and the `9V_PD` output for the battery build.
- Remove or DNP the barrel-jack variant unless this Power Board intentionally keeps a
  wall-input option.
- Replace AP63203 with TPS63070 for B+1 generation.
- Add BQ24074, cell connectors, DW01/FS8205 protection, and charge/status outputs.
- Decide whether MT3608 and MCP1700 are physically on this Power Board. If yes, keep
  and verify them here. If no, remove/DNP them and keep those rails on the Servo Board.
- Add board-to-board and machine-injection connectors before running ERC.
- Add PWR_FLAGs at real driven rail sources only.

---

## ERC / Review Targets

Before layout, the schematic should make these claims true:

- `VBUS` is not shorted to `B+1`, `V_SYS`, `VBAT`, or GND.
- `VBUS` cannot be backfed when USB is disconnected.
- `CHG_STAT` and `PGOOD` have valid pull-up strategy for the receiving MCU logic.
- The BQ24074 TS pin is in a valid operating window.
- TPS63070 FB divider midpoint actually connects to FB.
- TPS63070 inductor is between the switch pins, not wired as a buck inductor.
- Pack protection FET orientation matches the selected reference design.
- `B+1` output is the only machine power-injection output.
- Optional USB D+/D- and CC routing is deliberate, not accidental.

Do not connect a valuable WM-D6C until the Power Board passes standalone continuity,
USB attach, charger, pack handoff, B+1 dummy-load, and fault/backfeed tests.

---

## Commit Template

```text
git add hardware/kicad/Power-Board/wm-d6c-power-board docs/hardware/kicad-power-board-guide.md
git commit -m "Add WM-D6C Power Board schematic guide"
```

