# WM-D6C Power Board — KiCad 10 Schematic Guide

**Project:** `hardware/kicad/Power-Board/wm-d6c-power-board/`  
**Tool:** KiCad 10.x  
**Scope:** Power Board schematic only; one flat schematic sheet  
**Rev A target:** Battery-integrated WM-D6C using three parallel LiPo cells

This guide defines the locked Power Board architecture for the DSR-1 project.

The Power Board replaces the original four-AA holder, the corroded battery contacts,
and the original barrel-jack power path. It receives only USB-C 5 V power and ground,
charges the internal 1S3P LiPo pack, and generates regulated Sony `B+1` at 6.0 V.

The Power Board does **not** own USB data, USB-C CC handling, the STM32, the LED battery
monitor, `B+3`, or `+3V3`.

---

## 1. Locked Board Boundary

### Power Board owns

| Function | Responsibility |
|---|---|
| USB-C power input | `VBUS_5V` and GND only |
| Charging | BQ24074-class power-path charger |
| Energy storage | Three 803040 LiPo cells in parallel, treated as one 1S3P pack |
| Pack protection | DW01A/FS8205-class single-cell protection topology or selected equivalent |
| Main rail generation | TPS63070 buck-boost, `V_SYS` to regulated `B+1` = 6.0 V |
| Machine power injection | `B+1` and GND to the original battery-terminal PCB pads |
| Telemetry export | ADC-safe battery sense and charger status to the Servo Board |

### Power Board does not own

| Excluded function | Owner |
|---|---|
| USB D+ / D− | Servo Board |
| USB-C CC1 / CC2 | Servo Board |
| USB CDC, DFU, tuning, firmware update, data debugging | Servo Board |
| STM32 | Servo Board |
| Five-segment LED battery monitor | Servo Board |
| `B+3` generation | Servo Board |
| `+3V3` generation | Servo Board |
| Barrel-jack input | Removed |
| USB-C PD / 9 V negotiation | Not used |

---

## 2. System Power Flow

```text
USB-C VBUS_5V + GND
    -> BQ24074 power-path charger
    -> V_SYS
    -> TPS63070 buck-boost
    -> B+1 = regulated 6.0 V
    -> original WM-D6C battery-positive and battery-negative PCB pads
    -> S901 / original WM-D6C B+1 distribution
    -> original CP304 connector
    -> Servo Board receives B+1 and GND
```

The original battery holder and corroded contacts are removed. The PCB pads formerly
fed by those contacts become the permanent B+1 and GND injection points.

`B+1` is Sony's 6 V rail. Do not create a second independent `+6V` rail.

---

## 3. Current KiCad Cleanup

The current Power Board project is a migration skeleton. Before adding the final
battery architecture:

1. Remove or DNP IP2721 and all `9V_PD` circuitry.
2. Remove or DNP the barrel-jack input and polarity-correction blocks.
3. Remove AP63203.
4. Remove MT3608 and all `B+3` generation from this project.
5. Remove MCP1700 and all `+3V3` generation from this project.
6. Remove USB D+/D− and CC1/CC2 circuitry from this project.
7. Retain or place the BQ24074, three cell connectors, protection circuit, TPS63070,
   machine-output pads, telemetry connector, and test points.
8. Run ERC after each major block is completed.

The Power Board shall remain one flat schematic sheet.

---

## 4. Suggested Sheet Layout

```text
+ USB-C Power +----+ Charger / Pack +----+ B+1 Converter +----+ Machine Output +----+ Telemetry +
| 5 V and GND |    | BQ24074        |    | TPS63070      |    | battery pads   |    | to Servo  |
| only         |    | 1S3P + protect |    | 6.0 V         |    | B+1 and GND    |    | status     |
+--------------+----+----------------+----+---------------+----+----------------+----+------------+
```

Visual zones have no electrical meaning. Use ordinary same-sheet net labels.

---

## 5. Zone A — USB-C Power Input

The system USB-C receptacle is shared between the boards, but the Power Board receives
only the power pair.

| USB-C item | Power Board connection |
|---|---|
| VBUS pins | `VBUS_5V` |
| GND pins | GND |
| D+ / D− | No Power Board connection |
| CC1 / CC2 | No Power Board connection |
| SBU1 / SBU2 | No connection |
| Shield | Explicit EMC/chassis strategy |

### Rules

- Rev A uses default USB-C 5 V only.
- Do not negotiate or accept a 9 V PD contract.
- Do not place CC pull-downs on the Power Board.
- Do not place USB data ESD devices on the Power Board.
- Add VBUS transient protection and charger-input capacitance near the physical
  VBUS entry point.
- Add `TP_VBUS`.
- Verify that no battery or system rail can backfeed `VBUS_5V`.

### Source-current policy

CC current advertisement is handled on the Servo Board. The Power Board therefore
must be designed around a documented external USB-C supply requirement.

The programmed charger input limit must not exceed the current available from that
documented source. Deck operation takes priority over charging.

---

## 6. Zone B — Charger and Power Path

### U_CHG — BQ24074-class charger

Follow the selected device datasheet reference circuit. Do not add an external
VBUS-to-system Schottky load-sharing diode or a separate battery load-sharing P-FET
that bypasses the charger's internal power path.

| Function | Connection |
|---|---|
| IN | `VBUS_5V` |
| OUT | `V_SYS` |
| BAT | Protected pack positive domain |
| VSS / exposed pad | GND and thermal copper |
| CHG | `CHG_STAT_N` to Servo Board |
| PGOOD | `PGOOD_N` to Servo Board |
| ISET | Select using datasheet formula |
| ILIM | Select using datasheet formula and allowed range |
| TS | Real NTC or datasheet-valid fixed configuration |
| CE / EN pins | Defined state; never floating |
| TMR / ITERM | Deliberately configured |
| IN / OUT / BAT capacitors | Values and placement per datasheet |

### Mandatory corrections

- Do not use the earlier `800 ohm = 2 A` ILIM claim.
- Do not assume the charger can accept a 2 A programmed input limit.
- Pull `CHG_STAT_N` and `PGOOD_N` up to Servo Board `+3V3`, not to VBUS.
- Use enough copper for charger thermal dissipation.
- Select charge current to support simultaneous deck operation and charging without
  collapsing the USB source or overheating the charger.

---

## 7. Zone C — Three-Cell 1S3P Pack

Three 803040 LiPo cells are connected in parallel.

```text
Cell 1 + ----+
Cell 2 + ----+---- CELL_POS_RAW
Cell 3 + ----+

Cell 1 - ----+
Cell 2 - ----+---- CELL_NEG_RAW
Cell 3 - ----+
```

This is one logical 1S3P pack:

- nominal voltage: approximately 3.7 V
- full-charge voltage: 4.2 V
- three times the capacity of one cell, subject to actual cell rating

### Assembly rules

- Voltage-match the cells before first parallel connection.
- Use serviceable, polarized connectors or clearly documented solder pads.
- Do not mix cells of different chemistry, age, capacity, or condition.
- Leave mechanical allowance for cell swelling.
- Document cell orientation and polarity on the silkscreen.

---

## 8. Zone D — Pack Protection

Use the exact reference circuit for the final selected DW01A/FS8205-class parts.

Recommended net naming:

| Net | Meaning |
|---|---|
| `CELL_POS_RAW` | Raw parallel-cell positive |
| `CELL_NEG_RAW` | Raw parallel-cell negative before protection FETs |
| `VBAT_PROT` | Protected battery positive / charger BAT domain |
| `PACK_NEG_PROT` | Protected negative after the FETs |
| GND | System ground on the protected side |

### Critical rules

- Do not connect `CELL_NEG_RAW` directly to system GND.
- The protection FETs must remain in the only current path between raw cell negative
  and protected system ground.
- Follow the final protection IC's recommended FET orientation exactly.
- Add test points on raw cell voltage and protected pack output.
- If the selected cells include individual protection boards, document the interaction
  with the Power Board protection before deciding whether both are populated.

---

## 9. Zone E — B+1 Regulator

### U_B1 — TPS63070 buck-boost

The TPS63070 generates regulated Sony `B+1` from `V_SYS`.

| Function | Connection |
|---|---|
| VIN pins | `V_SYS` |
| VOUT pins | `B+1` |
| FB | Feedback-divider midpoint |
| L1 / L2 | Inductor between the two switch pins |
| EN | Defined enable state |
| PS/SYNC | Defined operating mode |
| VSEL / FB2 | Deliberately configured for the chosen single-output mode |
| VAUX | Local bypass capacitor; not a general-purpose supply |
| PG | Use or mark correctly; pull up if used |
| GND / exposed pad | Low-impedance ground and thermal copper |

Target feedback divider:

```text
B+1 = 0.8 V * (1 + 649 k / 100 k)
     = approximately 6.0 V
```

### Design rules

- Use datasheet-recommended input and output capacitance.
- Do not treat one 10 uF output capacitor as a sufficient default.
- Account for ceramic-capacitor DC-bias derating.
- Select the inductor from calculated peak current at minimum pack voltage, maximum
  deck load, startup surge, and expected efficiency.
- Keep the high-current switching loop compact.
- Keep the switch node away from battery sense and telemetry traces.
- Add `TP_SYS`, `TP_B1`, and GND test points.
- Place a PWR_FLAG only at the real B+1 source.

---

## 10. Machine Power Output

The Power Board connects permanently to the main-board pads formerly connected to
the battery holder.

| Net | Direction | Destination |
|---|---|---|
| `B+1` | Power Board to WM-D6C | Former battery-positive PCB pad |
| GND | Shared | Former battery-negative PCB pad |

Use mechanically robust wire attachment, strain relief, and adequate conductor gauge.

The original battery holder, spring contacts, and corroded terminal hardware are not
retained.

S901 remains the WM-D6C master power switch.

---

## 11. Servo Board Telemetry Connector

This connector carries only low-current telemetry and control signals. It does not
carry USB data, CC signals, B+1, B+3, or +3V3.

| Pin/net | Direction | Purpose |
|---|---|---|
| GND | Shared | Telemetry reference |
| `VBAT_SENSE` | Power Board to Servo Board | ADC-safe scaled battery voltage |
| `VBAT_SENSE_EN` | Servo Board to Power Board | Optional divider enable |
| `CHG_STAT_N` | Power Board to Servo Board | Active-low charger status |
| `PGOOD_N` | Power Board to Servo Board | Active-low valid-input status |

### Battery-sense implementation

Preferred arrangement:

```text
raw pack voltage
    -> high-value divider on Power Board
    -> optional high-side or properly isolated enable switch
    -> VBAT_SENSE, always ADC-safe
    -> Servo Board PB0 / ADC_IN8
```

Rules:

- `VBAT_SENSE` must never mean raw 4.2 V pack voltage.
- Do not use the same net name on both sides of the upper divider resistor.
- Do not use a low-side switching arrangement that lets the disabled ADC node float
  toward raw battery voltage.
- Place the ADC filter capacitor on the divided `VBAT_SENSE` node.

---

## 12. Required Test Points

| Test point | Net |
|---|---|
| TP_VBUS | `VBUS_5V` |
| TP_SYS | `V_SYS` |
| TP_CELL_POS | `CELL_POS_RAW` |
| TP_CELL_NEG | `CELL_NEG_RAW` |
| TP_VBAT | Protected battery domain |
| TP_B1 | `B+1` |
| TP_CHG | `CHG_STAT_N` |
| TP_PGOOD | `PGOOD_N` |
| TP_GND | GND |

---

## 13. ERC and Review Targets

Before layout:

- No `9V_PD`, IP2721, barrel-jack, B+3, +3V3, USB data, or CC circuitry remains.
- `VBUS_5V` cannot be backfed.
- All charger pins and required capacitors are handled intentionally.
- Raw cell negative cannot bypass the protection FETs.
- Battery-sense output is always STM32-safe.
- TPS63070 L1/L2, VAUX, PS/SYNC, VSEL, FB2, PG, EN, FB, grounds, and exposed pad are
  all handled intentionally.
- TPS63070 output capacitance and inductor current rating are justified.
- `B+1` and GND are the only machine power-injection nets.
- ERC has no unexplained errors.

---

## 14. Standalone Bring-Up Order

1. Continuity and polarity checks with no cells and no USB.
2. Verify no raw-negative-to-GND protection bypass.
3. Apply current-limited USB 5 V with no cells.
4. Verify charger input behavior and absence of VBUS backfeed.
5. Connect a protected test cell or current-limited battery simulator.
6. Verify charger state transitions and thermal behavior.
7. Verify `V_SYS`.
8. Bring up TPS63070 into a dummy load.
9. Verify 6.0 V B+1 across expected load and startup transients.
10. Verify USB removal hands the load to the pack.
11. Verify telemetry outputs and battery-sense scaling.
12. Only then connect the Power Board to the WM-D6C.

---

## Commit Template

```text
git add hardware/kicad/Power-Board/wm-d6c-power-board \
        docs/hardware/kicad-power-board-guide.md
git commit -m "Revise Power Board guide for locked Rev A architecture"
```
