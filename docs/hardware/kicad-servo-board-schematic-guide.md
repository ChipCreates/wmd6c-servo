# DSR-1 Servo Control Board — KiCad 10 Schematic Guide

**Project:** `hardware/kicad/DSR-1/`  
**Tool:** KiCad 10.x  
**Scope:** Servo Control Board schematic only; one flat D-size sheet  
**Rev A target:** Sony WM-D6C serial 72795, PCB `C11-494-12`, former-type CX20084 circuit

This guide reflects the locked Rev A architecture.

The Servo Board replaces the original CP304 DC-DC converter and the failed CX20084
servo function. It receives Sony `B+1` at 6.0 V and GND from the original CP304
connector, generates local `+3V3` and `B+3`, handles all USB signals except VBUS power
and ground, runs the digital servo, and provides the mandatory five-segment battery
monitor.

The separate Power Board receives USB-C 5 V and ground, charges the 1S3P LiPo pack,
and injects regulated `B+1` into the former battery-terminal PCB pads.

---

## 1. Confirmed Compatibility

The target unit is:

| Item | Confirmed value |
|---|---|
| Model | Sony WM-D6C |
| Serial | 72795 |
| PCB marking | `C11-494-12` |
| Manual family | Former type / Ver. 1.0 |
| Servo IC | CX20084 |
| Motor-control family | Single-Q601 former-type architecture |
| Rev A applicability | Yes |

The v1.1 manual identifies the corresponding `1-611-494-12` board family as former
type. The later CX-069A/Q601–Q605 circuit is out of scope for Rev A.

Use the original service manual `fb4872.pdf` as the governing Sony electrical
reference. Use the v1.1 manual to identify the incompatible later circuit family.

The service manual establishes Q601 as a PNP control path. The exact SMD package,
marking, base voltage, and current on this physical unit still require bench
confirmation before final resistor values are frozen.

---

## 2. Locked Board Boundary

### Servo Board owns

| Function | Responsibility |
|---|---|
| Sony power input | Receive `B+1` 6 V and GND at the former CP304 connector |
| Motor rail | Generate `B+3` at approximately 10.8 V |
| Logic rail | Generate local `+3V3` |
| Servo controller | STM32G0C1KCU6 and firmware |
| FG input | Conditioning and TIM2 input capture |
| Motor drive | PWM, filter, level shift, Q601-base interface |
| Speed controls | RV601, RV602, RV603, S601 |
| Motor enable | Monitor former IC601 pin-7 network |
| USB signaling | D+, D−, CC1, CC2, ESD, sink termination, USB firmware |
| Programming/recovery | USB DFU/application update plus mandatory SWD |
| Battery monitor | Five LED outputs, S801 BATT-mode input, battery/status telemetry |

### Servo Board does not own

| Excluded function | Owner |
|---|---|
| USB-C 5 V power processing | Power Board |
| LiPo charging | Power Board |
| LiPo protection | Power Board |
| B+1 generation | Power Board |
| Original barrel jack | Removed |
| USB-C PD / 9 V negotiation | Not used |

---

## 3. Current KiCad Cleanup

The committed `DSR-1.kicad_sch` root is already a flat D-size sheet. Do not attempt
to remove hierarchical sheet rectangles that are not present.

Use this migration sequence:

1. Make a branch or backup.
2. Open `DSR-1.kicad_sch` as the working flat schematic.
3. Remove IP2721, barrel-jack, AP63203, charger, battery-protection, and B+1-generation
   blocks from the root.
4. Retain or rebuild the MT3608 B+3 converter.
5. Retain or rebuild the local 3.3 V regulator.
6. Consolidate the STM32, signal conditioning, USB signaling, CP304 interface,
   Sony servo harness, LED interface, telemetry link, and debug header on the root.
7. Use orphaned legacy child schematics only as visual references.
8. Archive or delete legacy child sheets only after confirming no unique circuitry
   remains.
9. Replace hierarchical labels with ordinary same-sheet net labels.
10. Run ERC after each completed zone.

---

## 4. Suggested Flat-Sheet Layout

```text
+ Power Rails +----+ MCU / USB +----+ Signal Conditioning +----+ Sony Interfaces +
| B+1 input   |    | STM32      |    | FG, ADC, motor      |    | CP304, IC601    |
| 3.3 V       |    | USB/CC     |    | enable, clamps      |    | pots, S601      |
| B+3 boost   |    | SWD/reset  |    | battery sense       |    | LED/status      |
+-------------+----+------------+----+---------------------+----+-----------------+
```

Use ordinary net labels for same-sheet connections. Use power symbols for `B+1`,
`B+3`, `+3V3`, and GND.

---

## 5. Zone A — CP304 Replacement Power Interface

The Servo Board occupies the original CP304 function and receives power through the
original CP304 connector or its documented replacement harness.

Required electrical functions:

| Net | Direction | Function |
|---|---|---|
| `B+1` | WM-D6C to Servo Board | Regulated 6.0 V input |
| GND | Shared | Power return |
| `B+3` | Servo Board to WM-D6C | Approximately 10.8 V motor rail |

The exact connector pin numbering and mechanical footprint must be confirmed against
the physical machine and service manual before layout.

### Rules

- `B+1` is already the Sony 6 V rail; do not regenerate it locally.
- Do not place a series diode that blocks B+1 from entering the Servo Board.
- If reverse-current protection is required, use a topology compatible with the
  actual power direction.
- Add test points for B+1, B+3, +3V3, and GND.
- Place a PWR_FLAG at the incoming B+1 connector so ERC recognizes the external source.

---

## 6. Zone B — Local 3.3 V Rail

Generate `+3V3` locally from B+1.

The existing architecture uses an MCP1700-class 3.3 V regulator. Confirm the final
part's current capability against:

- STM32 maximum current
- USB activity
- five LED-control interfaces
- status pull-ups
- signal-conditioning loads
- debug and test loads

Required connections:

```text
B+1 -> regulator VIN
regulator GND -> GND
regulator VOUT -> +3V3
```

Use the final regulator datasheet's required input and output capacitors close to the
pins.

Add:

- `TP_3V3`
- input bulk capacitance as justified
- local decoupling at each load
- a PWR_FLAG at the real +3V3 source

Do not power the Servo Board from the SWD header or USB VBUS.

---

## 7. Zone C — B+3 Motor-Rail Converter

The Servo Board replaces CP304 and generates approximately 10.8 V B+3 from 6.0 V B+1.

### MT3608-class boost topology

```text
B+1 -> inductor -> SW node
                     +-> MT3608 SW
                     +-> Schottky diode anode
Schottky cathode -> B+3
B+3 -> output capacitors -> GND
B+3 -> upper feedback resistor -> FB
FB -> lower feedback resistor -> GND
```

Existing target divider:

```text
R_upper = approximately 169 k
R_lower = 10 k
VOUT = approximately 10.8 V
```

### Rules

- Verify the final converter datasheet and exact part.
- Keep the SW node short.
- Keep switching current away from FG, ADC, USB, and Q601-control traces.
- Select the inductor and diode for startup and motor-current transients.
- Use output capacitance appropriate for the motor supply.
- Add B+3 overvoltage and loaded-startup testing to bring-up.
- Add `TP_B3`.

---

## 8. Zone D — STM32G0C1KCU6

Use `STM32G0C1KCU6` in UFQFPN32.

### Confirmed Rev A pin allocation

| Pin | MCU signal | DSR-1 function |
|---:|---|---|
| 1 | PB9 | Spare / NC |
| 2 | PC14 | NC |
| 3 | PC15 | NC |
| 4 | VDD/VDDA | `+3V3` |
| 5 | VSS/VSSA | GND |
| 6 | PF2-NRST | NRST |
| 7 | PA0 | `FG_IN` / TIM2_CH1 |
| 8 | PA1 | `RV601_WIPER` / ADC_IN1 |
| 9 | PA2 | `RV602_WIPER` / ADC_IN2 |
| 10 | PA3 | `RV603_WIPER` / ADC_IN3 |
| 11 | PA4 | Spare / NC |
| 12 | PA5 | `MOTOR_EN_MON` |
| 13 | PA6 | `MOTOR_PWM` / TIM3_CH1 |
| 14 | PA7 | `SPEED_TUNE_SW` |
| 15 | PB0 | `VBAT_SENSE` / ADC_IN8 |
| 16 | PB1 | `VBAT_SENSE_EN` |
| 17 | PB2 | `BATT_LED1` |
| 18 | PA8 | `CC1` |
| 19 | PA9 | `CC2` |
| 20 | PC6 | `S801_BATT` |
| 21 | PA10 | Spare / NC |
| 22 | PA11 | `USB_DM` |
| 23 | PA12 | `USB_DP` |
| 24 | PA13 | `SWDIO` |
| 25 | PA14-BOOT0 | `SWDCLK` / BOOT0 |
| 26 | PA15 | `BATT_LED2` |
| 27 | PB3 | `BATT_LED3` |
| 28 | PB4 | `BATT_LED4` |
| 29 | PB5 | `BATT_LED5` |
| 30 | PB6 | `DEBUG_TX` |
| 31 | PB7 | `CHG_STAT_N` |
| 32 | PB8 | `PGOOD_N` |
| EP | VSS | GND |

The GP K package has combined VDD/VDDA and VSS/VSSA pins and no separate VDDIO2 pin.

### Decoupling

Place local decoupling from pin 4 to GND:

- 100 nF X5R
- 1 uF X5R
- optional additional analog-noise capacitor only if justified by the final ADC design

Keep the primary 100 nF capacitor closest to the supply pin.

Ground the exposed pad with a via array.

### Reset and boot

NRST:

```text
NRST -> 100 nF -> GND
NRST -> SWD/recovery header
```

BOOT0 / PA14:

```text
PA14-BOOT0 -> 10 k pull-down -> GND
PA14-BOOT0 -> SWDCLK
```

Do not hard-wire PA14 to both 3.3 V and ground.

Provide documented test access if forced bootloader entry is required.

---

## 9. Zone E — USB-C Signaling

The Power Board receives only USB-C 5 V and GND. The Servo Board receives and owns
all remaining USB-C signals:

| Signal | Servo Board function |
|---|---|
| `USB_DP` | STM32 USB D+ |
| `USB_DM` | STM32 USB D− |
| `CC1` | USB-C sink configuration / monitoring |
| `CC2` | USB-C sink configuration / monitoring |
| GND | USB signal reference |

### USB data path

```text
USB-C D+ / D−
    -> ESD protection near connector or Servo Board entry
    -> optional required series components from STM32 reference design
    -> PA12 / PA11
```

Route D+ and D− as a short, tightly coupled differential pair with a continuous
ground reference.

### CC handling

Rev A is a 5 V-only USB-C sink.

The hardware must present valid sink termination before firmware starts. Use one
datasheet-compliant Rd termination on each CC pin unless the final STM32 UCPD
implementation provides an explicitly verified equivalent.

```text
CC1 -> valid Rd / protection -> PA8 as designed
CC2 -> valid Rd / protection -> PA9 as designed
```

No 9 V PD negotiation is used.

### USB functions

USB is mandatory for:

- USB CDC telemetry
- servo tuning
- diagnostic logging
- firmware update
- application communication

USB does not replace SWD for blank-device programming, breakpoint debugging, or
recovery from corrupted firmware.

### Physical interconnect

If the USB-C receptacle is not physically mounted on the Servo Board:

- use a short connector or flex designed for USB 2.0 Full Speed
- route D+ and D− together
- provide adjacent ground reference
- do not use a long unordered wire bundle
- place ESD protection at the connector-side entry whenever practical

The Servo Board is not powered from USB VBUS. Its operating power remains B+1 through
the CP304 connection.

---

## 10. Zone F — FG Input

FG901 is an open-collector optical feedback source pulled up on the Sony circuitry.

The actual target waveform must be measured before final values are frozen.

Recommended placeholder topology:

```text
FG_RAW
    -> upper divider resistor
    -> FG_COND
FG_COND
    -> lower divider resistor -> GND
    -> rail clamp network
    -> small C0G filter if measurement supports it
    -> PA0 / FG_IN
```

Rules:

- Do not connect FG_RAW directly to the STM32.
- Size the divider so normal high level remains below the STM32 input limit without
  relying on continuous clamp conduction.
- Ensure the clamp cannot back-power an unpowered +3V3 rail.
- Select filtering from measured ringing and edge rate.
- Add `TP_FG_RAW` and `TP_FG_IN`.

The existing 10 k / 22 k values remain provisional until measured.

---

## 11. Zone G — Motor-Control Output

The original manual establishes a former-type PNP Q601 control path. Lower base
voltage relative to the B+3-referenced emitter increases motor drive; raising the
base toward the emitter turns Q601 off.

The exact target-unit Q601 package, base voltage, base current, startup behavior, and
safe-off voltage must be measured.

Committed architecture:

```text
PA6 / MOTOR_PWM
    -> PWM filter / base limiting
    -> MMBT3904-class NPN level shifter
    -> Q601_BASE interface
    -> off-board Sony Q601
```

Rules:

- Default reset state must leave Q601 safely off.
- BOOT0, USB attach, firmware update, and MCU reset must not command uncontrolled motor
  drive.
- Do not freeze R7, R8, R9, or filter values before Q601 bench characterization.
- Provide `TP_MOTOR_PWM`, `TP_LEVEL_SHIFT`, and `TP_Q601_BASE`.
- Test the output stage into a dummy load before connecting it to Q601.

---

## 12. Zone H — Motor Enable and Speed Controls

### Motor-enable monitor

The former IC601 pin-7 network is approximately 4.4 V during play and is pulled low
by the Auto-Off circuitry.

Use a divider sized from bench measurements before PA5.

```text
MOTOR_EN_RAW -> divider -> MOTOR_EN_MON -> PA5
```

Add a test point on both sides of the divider.

### RV601, RV602, RV603

Each Sony wiper must be measured for:

- full voltage range
- source impedance
- polarity
- interaction with Speed Tune
- behavior when the Servo Board is unpowered

Use a divider where required. A small series resistor and clamp alone is not an
acceptable final design for a sustained overvoltage.

Map:

| Sony control | STM32 |
|---|---|
| RV601 | PA1 / ADC_IN1 |
| RV602 | PA2 / ADC_IN2 |
| RV603 | PA3 / ADC_IN3 |
| S601 | PA7 / `SPEED_TUNE_SW` |

Add ADC filter capacitors only after determining acceptable source impedance and
sample time.

---

## 13. Zone I — Power Board Telemetry

The Power Board sends only telemetry/control signals to the Servo Board.

| Net | Direction | STM32 |
|---|---|---|
| GND | Shared | Ground reference |
| `VBAT_SENSE` | Power Board to Servo Board | PB0 / ADC_IN8 |
| `VBAT_SENSE_EN` | Servo Board to Power Board | PB1 |
| `CHG_STAT_N` | Power Board to Servo Board | PB7 |
| `PGOOD_N` | Power Board to Servo Board | PB8 |

Rules:

- `VBAT_SENSE` must already be ADC-safe when it reaches the Servo Board.
- Place the final ADC filter capacitor on the divided sense node.
- Pull `CHG_STAT_N` and `PGOOD_N` up to local +3V3.
- Verify whether the selected charger outputs are open-drain and active-low.
- Add test points for all telemetry nets.

USB D+/D− and CC1/CC2 do not pass through this telemetry connector.

---

## 14. Zone J — Mandatory LED Battery Monitor

The five-segment front-panel battery display is a required Rev A feature.

Required STM32 signals:

| Net | MCU pin | Function |
|---|---|---|
| `BATT_LED1` | PB2 | Segment 1 |
| `BATT_LED2` | PA15 | Segment 2 |
| `BATT_LED3` | PB3 | Segment 3 |
| `BATT_LED4` | PB4 | Segment 4 |
| `BATT_LED5` | PB5 | Segment 5 |
| `S801_BATT` | PC6 | Battery-display mode detection |

The LED interface must also use:

- `VBAT_SENSE`
- `CHG_STAT_N`
- `PGOOD_N`

### Required behavior

- In BATT mode, display pack state of charge.
- While external USB power is valid and charging is active, provide the defined
  charging animation.
- When charging is complete, indicate full state according to firmware design.
- Outside BATT mode, release all five LED-drive pins to high impedance.
- Never contend with the original CX10043/VU circuitry.

### Required bench confirmation

Before final wiring:

1. Determine LED common polarity.
2. Confirm current direction through the existing 180 ohm resistors.
3. Confirm what the CX10043 outputs do in BATT mode.
4. Confirm whether direct STM32 drive is safe.
5. Add transistor or open-drain buffers if direct drive would exceed STM32 limits or
   contend with the original circuitry.
6. Confirm the electrical state of `S801_BATT`.

The LED monitor is not DNP and is not a future option.

---

## 15. Sony Servo Interface Connector

Use a dedicated harness for the signals that replace CX20084 functions.

Minimum signal set:

| Net | Direction |
|---|---|
| `FG_RAW` | Sony to Servo Board |
| `Q601_BASE` | Servo Board to Sony |
| `MOTOR_EN_RAW` | Sony to Servo Board |
| `RV601_RAW` | Sony to Servo Board |
| `RV602_RAW` | Sony to Servo Board |
| `RV603_RAW` | Sony to Servo Board |
| GND | Shared |

S601 and the LED-board interface may use separate connectors if that better matches
the physical installation.

Do not claim a connector family or pitch until the actual harness, available space,
and installation method are fixed.

---

## 16. SWD and Recovery Header

SWD is mandatory even though USB firmware update is also mandatory.

Minimum connections:

| Pin | Signal |
|---|---|
| 1 | GND |
| 2 | SWDCLK |
| 3 | SWDIO |
| 4 | NRST |
| 5 | +3V3 reference |

A compact Tag-Connect or equivalent footprint is acceptable.

The +3V3 pin is a reference unless the programming procedure explicitly allows
external target power.

Keep the recovery connector accessible during Rev A development.

---

## 17. Required Test Points

| Test point | Net |
|---|---|
| TP_B1 | `B+1` |
| TP_B3 | `B+3` |
| TP_3V3 | `+3V3` |
| TP_GND | GND |
| TP_FG_RAW | `FG_RAW` |
| TP_FG_IN | `FG_IN` |
| TP_MOTOR_PWM | `MOTOR_PWM` |
| TP_Q601_BASE | `Q601_BASE` |
| TP_MOTOR_EN_RAW | `MOTOR_EN_RAW` |
| TP_MOTOR_EN_MON | `MOTOR_EN_MON` |
| TP_RV601 | conditioned RV601 |
| TP_RV602 | conditioned RV602 |
| TP_RV603 | conditioned RV603 |
| TP_USB_DP | `USB_DP` |
| TP_USB_DM | `USB_DM` |
| TP_CC1 | `CC1` |
| TP_CC2 | `CC2` |
| TP_VBAT_SENSE | `VBAT_SENSE` |
| TP_CHG | `CHG_STAT_N` |
| TP_PGOOD | `PGOOD_N` |
| TP_NRST | NRST |
| TP_SWDIO | SWDIO |
| TP_SWDCLK | SWDCLK |

---

## 18. ERC and Review Targets

Before layout:

- The root schematic is one flat D-size sheet.
- No charger, cells, pack protection, TPS63070, B+1 generator, barrel jack, IP2721,
  or 9 V PD path remains.
- B+1 enters through the CP304 interface.
- B+3 and +3V3 are generated locally.
- USB D+/D− and CC1/CC2 are present and owned by the Servo Board.
- No USB VBUS power rail powers the Servo Board.
- Both CC pins have a valid 5 V sink strategy.
- USB ESD and physical routing are defined.
- NRST, SWDIO, and SWDCLK are accessible.
- BOOT0 is not incorrectly tied to both rails.
- LED battery monitor circuitry is present and mandatory.
- All unmeasured Sony-facing values are clearly marked provisional.
- Externally driven rails have appropriate PWR_FLAGs or justified ERC treatment.
- ERC has no unexplained errors.

---

## 19. Standalone Bring-Up Order

1. Continuity checks with no power.
2. Verify no B+1-to-GND, B+3-to-GND, or +3V3-to-GND short.
3. Apply current-limited 6 V B+1 with no Sony signal harness connected.
4. Verify +3V3.
5. Verify B+3 into a dummy load.
6. Program the STM32 over SWD.
7. Verify USB enumeration and firmware update without motor connection.
8. Verify CC orientation handling with both USB-C plug orientations.
9. Inject known FG test signals and verify timer capture.
10. Inject known ADC voltages into each conditioned speed input.
11. Verify motor-enable monitor thresholds.
12. Verify motor-output safe state during reset, bootloader, USB attach, and firmware
    update.
13. Exercise Q601 output into a dummy load.
14. Verify Power Board telemetry inputs.
15. Verify LED monitor behavior with a bench LED/test load.
16. Bench-characterize the actual Sony Q601 and control nodes.
17. Connect to the WM-D6C only after all standalone tests pass.

---

## Commit Template

```text
git add hardware/kicad/DSR-1 docs/hardware/kicad-schematic-guide.md
git commit -m "Revise Servo Board guide for locked Rev A architecture"
```
