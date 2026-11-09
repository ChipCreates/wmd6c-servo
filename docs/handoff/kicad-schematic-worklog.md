# Rev A Schematic Capture — Work Log

**Date:** 2026-11-09
**Boards:** Servo Control Board (`hardware/kicad/DSR-1/DSR-1.kicad_sch`) and Power Board
(`hardware/kicad/Power-Board/wm-d6c-power-board/wm-d6c-power-board.kicad_sch`)
**Authority:** `docs/hardware/kicad-servo-board-schematic-guide.md`,
`docs/hardware/kicad-power-board-guide.md` (see the handoff, §2)

---

## 1. Result

| Board | Parts | Pin→net checks | ERC (errors + warnings) |
|---|---:|---:|---:|
| Servo Control Board | 88 | 209 / 209 correct | **0** |
| Power Board | 45 | 114 / 114 correct | **0** |

PDFs: `hardware/schematic-pdfs/DSR-1-servo-board.pdf`,
`hardware/schematic-pdfs/wm-d6c-power-board.pdf`.

Both sheets are one flat D-size page, split into labelled zones that match the guides.
Every connected pin has a short wire stub ending in a net label, power symbol (vertical
pins) or rail global label (sideways pins); unused pins carry no-connect flags.

## 2. How the schematics are produced

The schematics are **generated** by Python scripts, so every connection is computed and
checked rather than hand-placed:

```bash
python3 tools/test_kicad_sch.py /some/scratch/dir   # generator self-test (372 pin checks)
python3 tools/gen_servo_board.py                    # writes DSR-1.kicad_sch, verifies netlist
python3 tools/gen_power_board.py                    # writes power board .kicad_sch/.kicad_sym/sym-lib-table
```

- `tools/kicad_sch.py` — reads KiCad's stock libraries (flattening `extends`), places
  parts, draws stubs/labels, writes the file, then exports the netlist with `kicad-cli`
  and asserts every pin is on its intended net.
- Each generator run **overwrites** its schematic. Once you start editing in the KiCad
  GUI, treat the GUI file as the master and stop re-running the generator (or port the
  edits into the generator first).

## 3. What changed from the previous schematics

**Servo Board** — removed the old wall-power front end (USB-C power input, IP2721 PD
trigger, barrel jack, SS14 bridge, polyfuse, SMBJ12A, AP63203) and the `9V_PD`↔GND
short. Rewired the MT3608 boost to the guide's topology. Replaced the hierarchical
labels. Added: CP304 connector, Sony servo-lead connector, pots/S601 connector,
LED-board harness, Power Board telemetry connector, SWD header, NRST cap, BOOT0
pull-down, CC Rd resistors, USB ESD, LED/S801 interface, pot dividers, 25 test points.

**Power Board** — was 7 unwired symbols. Now complete: USB 5 V entry + TVS, BQ24074 with
every pin handled, three cell connectors, DW01A + FS8205A protection, TPS63070 6.0 V
regulator, machine-output connector, telemetry connector with battery-sense divider,
8 test points. New project library `wm-d6c-power-board.kicad_sym` (TPS63070RNM,
FS8205A, B+1) and `sym-lib-table`.

## 4. Computed values (with source)

| Part | Value | Basis |
|---|---|---|
| Servo R (MT3608 FB) | 169 k / 10 k | 0.6 V × (1 + 169/10) = 10.74 V (MT3608 datasheet, FB = 0.6 V) |
| Servo MCP1700 caps | 1 µF in / out | MCP1700 datasheet: stable with 1 µF ceramic |
| Servo CC1 Rd | none (STM32) | PA9/DBCC1 tied to CC1: dead-battery Rd unpowered, UCPD1 sink Rd running (RM0444 §7.3.16, §38.4.6) |
| Servo CC2 Rd | 5.1 k 1 % | No UCPD CC2 pin on UFQFPN32; external sink Rd, sensed on PA4 / ADC_IN4 |
| BQ24074 R_ISET | 1.13 k 1 % | I_CHG = 890 AΩ / 1.13 k = 0.79 A (K_ISET typ) |
| BQ24074 R_ILIM | 1.13 k 1 % | I_IN = 1610 AΩ / 1.13 k = 1.42 A; R_ILIM range 1.1–8 k |
| BQ24074 TS | 10 k fixed | Datasheet: fixed 10 k when no NTC is used |
| BQ24074 TMR, ITERM | open | Datasheet: default safety timers, 10 % termination |
| BQ24074 EN2/EN1/CE | OUT / GND / GND | ILIM-resistor mode, charging enabled |
| BQ24074 caps | 10 µF on IN, OUT, BAT | Datasheet ranges 1–10 µF (IN), 4.7–47 µF (OUT, BAT) |
| DW01A | 470 Ω + 100 nF VCC, 2 k CS | DW01x datasheet Rev 1.7, typical application table |
| TPS63070 divider | 649 k / 100 k | 0.8 V × (1 + 6.49) = 5.99 V |
| TPS63070 L, caps | 1.5 µH; CIN 2×10 µF + 10 µF; COUT 3×22 µF + 10 µF; VAUX 100 nF | TPS63070 datasheet Fig. 10 / Table 2 |
| TPS63070 pins | EN, PS/SYNC = V_SYS; VSEL, FB2 = GND; PG unused | Always on, power-save mode, single output voltage |
| Battery sense | 220 k / 470 k 1 % | 4.2 V × 470/690 = 2.86 V max; 6 µA always-on drain |

## 5. PROVISIONAL (set from bench measurements)

Marked on the sheet notes and with a hidden `Status` field on each part:

- FG divider 10 k / 10 k, 10 pF filter. **Changed from the old 10 k / 22 k**, which puts a
  6 V FG high at 4.1 V and relies on continuous clamp conduction (forbidden by guide §10).
- Motor drive: 1 k / 100 nF filter, 100 k pull-up to B+1, 10 k to Q601 base (old topology kept).
- MOTOR_EN divider 10 k / 22 k (4.4 V → 3.03 V).
- Pot dividers 100 k / 120 k + 100 nF + BAT54S clamp. **Changed from 100 Ω + clamp**, which
  guide §12 calls unacceptable for a sustained over-voltage. If the wipers measure ≤ 3.3 V,
  the 120 k parts can be DNP and the 100 k reduced.
- S601 and S801 inputs: 10 k / 12 k dividers (input level unknown).
- LED outputs: 1 k series resistors; buffers may be needed after the LED/CX10043 checks.

## 6. Open issues — need your decision

1. **USB-C CC wiring — RESOLVED 2026-11-09 (pin table revised).** On the UFQFPN32
   STM32G0C1, PA9 is UCPD1_DBCC1, not UCPD1_CC2 (that is PB15, not bonded out). Chosen
   fix: PA9 tied to CC1 as RM0444 §7.3.16 prescribes, the external CC1 Rd removed (it
   would have paralleled the internal dead-battery Rd, ≈2.55 kΩ), CC2 keeps an external
   5.1 k Rd and moves to the former spare PA4 (ADC_IN4). Updated: servo guide §8–9,
   pin allocation, PROJECT_STATUS, README, signal-chain and USB-PD theory notes.
   Firmware must: configure UCPD1 sink on CC1 before writing `SYSCFG_CFGR1.UCPD1_STROBE`;
   never drive PA9 (no USART1_TX on PA9); read CC2 Rp level on PA4. Bench-check CC1
   termination with the Servo Board unpowered (deck off, charging).
2. **USB-C receptacle location** (handoff D1) — Servo Board uses a 5-pin "USB-C signal
   entry" placeholder; Power Board a 2-pin "USB-C power entry".
3. **USB source requirement** — the 1.42 A input limit assumes a USB-C supply rated
   ≥ 1.5 A. A 500 mA / 900 mA source would be overloaded. Alternatives: a larger R_ILIM
   (lower limit, slower charging), or switching EN1/EN2 from firmware based on the
   CC-advertised current (needs MCU control of those pins and CC monitoring).
4. **VBAT_SENSE_EN is unused** on the Power Board (always-on 6 µA divider instead of a
   switched one). The pin is kept on both connectors. With the Servo Board off (S901 off)
   the divider can feed ≈17 µA max into the unpowered PB0 pin; acceptable but noted.
5. **Connector families/footprints** (handoff D2) — all connectors are generic with empty
   footprints. Stock ICs keep KiCad's default footprints; TPS63070 (VQFN-15 RNM) has none
   yet. The cell connectors were previously `B2B-PH-SM4-TB` (JST PH SMD); that footprint
   library is not in the repo.
6. **TP_VBAT** is not separate from TP_CELL_POS: with low-side protection the protected
   and raw positive are the same conductor.

## 7. Housekeeping left alone

- The orphan child sheets in `hardware/kicad/DSR-1/` (`Power input zone`,
  `Power management`, `Signal conditioning`, `STM32G0C1 and decoupling`, `Connectors`,
  `mcu`) are no longer referenced. Nothing unique remains in them; delete when ready.
- `DSR-1.kicad_sym` still holds unused symbols (AP63203, IP2721, MT3608, etc.). The new
  schematic uses stock symbols plus `DSR-1:B+1` / `DSR-1:B+3`.
- Footprint assignment and PCB layout not started.
