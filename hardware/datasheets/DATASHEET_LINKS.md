# Referenced Datasheets and Source Documents — WM-D6C DSR-1

**Project:** `ChipCreates/wmd6c-servo`  
**File:** `hardware/datasheets/DATASHEET_LINKS.md`  
**Status:** Aligned with the locked Power Board and Servo Board KiCad guides  
**Updated:** 2026-06-08

All linked documents remain the property of their respective manufacturers or
publishers. Links are provided for engineering reference only.

The DSR-1 design uses two physical boards:

- **Power Board:** USB-C 5 V/GND input, LiPo charging and protection, and regulated
  Sony `B+1` at 6.0 V.
- **Servo Board:** STM32, USB data/CC handling, local `+3V3`, `B+3`, servo control,
  Sony signal conditioning, SWD, and the mandatory LED battery monitor.

A link in this file does not by itself approve a part for the BOM. Parts marked
**baseline** are the current design basis. Parts marked **selection pending** require
an exact manufacturer, orderable part number, package, footprint, and datasheet review
before schematic freeze.

---

## 1. Document Authority

When sources disagree, use this order:

1. Bench measurements on the actual target WM-D6C.
2. Sony service manual matching the confirmed board family.
3. Exact manufacturer datasheet for the selected orderable component.
4. DSR-1 hardware guides and design notes.
5. Distributor-hosted datasheet mirrors.
6. Unverified third-party summaries.

The Rev A target is:

| Field | Value |
|---|---|
| Model | Sony WM-D6C |
| Serial number | 72795 |
| PCB marking | `C11-494-12` |
| Sony circuit family | Former type / Ver. 1.0 |
| Original servo IC | CX20084 |
| Primary Sony manual | Original/former-type service manual |

---

## 2. Primary MCU and STM32 Documentation

### Active baseline: STM32G0C1KCU6

| Document | Publisher | Document / status | Link |
|---|---|---|---|
| STM32G0C1xC/xE Datasheet | STMicroelectronics | **DS13564 Rev 5**; governs STM32G0C1KCU6 electrical limits, UFQFPN32 pinout, USB, UCPD, ADC, timers, and package data | https://www.st.com/resource/en/datasheet/stm32g0c1cc.pdf |
| STM32G0C1KC Product Page | STMicroelectronics | Exact device-family portal; includes STM32G0C1KCU6 ordering entry and current documentation | https://www.st.com/en/microcontrollers-microprocessors/stm32g0c1kc.html |
| STM32G0x1 Reference Manual | STMicroelectronics | RM0444; peripheral registers and operation | https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf |
| STM32 Cortex-M0+ Programming Manual | STMicroelectronics | PM0223; Cortex-M0+ programming model | https://www.st.com/resource/en/programming_manual/pm0223-stm32-cortexm0-mcus-programming-manual-stmicroelectronics.pdf |
| USB Hardware and PCB Guidelines for STM32 MCUs | STMicroelectronics | AN4879; USB FS routing, ESD, clocking, power, and PCB guidance | https://www.st.com/resource/en/application_note/an4879-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.pdf |

### MCU selection note

The selected Rev A part is **STM32G0C1KCU6**, not STM32G0B1KBU6 and not
STM32G0C1KCU6N.

The schematic symbol, pin mapping, package, and firmware configuration must all match
the exact non-`N` UFQFPN32 device. Recheck the exact package pin table before final
layout, especially:

- PA8 / CC1
- PA9 / CC2
- PA11 / USB_DM
- PA12 / USB_DP
- PA13 / SWDIO
- PA14-BOOT0 / SWDCLK
- exposed-pad VSS
- combined VDD/VDDA and VSS/VSSA pins

Use the ST product page to obtain the latest errata sheet before PCB release.

---

## 3. USB-C Data, CC, and ESD — Servo Board

The Servo Board owns USB D+/D−, CC1/CC2, USB protocol, firmware update, and SWD
recovery. The Power Board receives only USB-C 5 V and GND.

| Document | Manufacturer | Status | Link |
|---|---|---|---|
| USBLC6-2SC6Y ESD Protection | STMicroelectronics | **Baseline candidate** for USB D+/D− ESD protection; confirm package and placement | https://www.st.com/resource/en/datasheet/usblc6-2sc6y.pdf |
| STM32 USB Hardware and PCB Guidelines | STMicroelectronics | Required routing and ESD reference; also listed above | https://www.st.com/resource/en/application_note/an4879-usb-hardware-and-pcb-guidelines-using-stm32-mcus-stmicroelectronics.pdf |

### USB-C items still requiring exact part selection

No generic datasheet can replace the exact component datasheet for these items:

| Item | Requirement before schematic/BOM freeze |
|---|---|
| USB-C receptacle or panel connector | Select exact part; verify pin numbering, shield tabs, footprint, current rating, mating cycles, and mechanical fit |
| USB data inter-board flex/connector | Select exact part; verify USB 2.0 Full Speed suitability and ground-reference arrangement |
| CC1/CC2 protection | Select only after final CC/UCPD circuit is frozen |
| CC Rd resistors | Confirm resistance/tolerance from the final USB-C sink implementation |
| VBUS transient suppressor | Select exact 5 V-compatible device from surge/ESD requirements |
| Shield termination components | Freeze after chassis/EMC strategy is documented |

Rev A is a **5 V-only sink**. IP2721 and 9 V PD negotiation are not part of the
battery-integrated architecture.

---

## 4. Power Board — Charger and Power Path

### Active baseline: BQ24074

| Document | Manufacturer | Document / status | Link |
|---|---|---|---|
| BQ2407x Standalone 1-Cell 1.5-A Linear Battery Charger with PowerPath | Texas Instruments | **BQ24074 baseline**; Rev. N at time of this update | https://www.ti.com/lit/ds/symlink/bq24074.pdf |
| BQ24074 Product Page | Texas Instruments | Current product, package, tools, and documentation portal | https://www.ti.com/product/BQ24074 |

The exact BQ24074 orderable suffix and package must match the final KiCad symbol and
footprint.

The schematic review must cover all applicable pins and requirements, including:

- IN
- OUT
- BAT
- VSS and exposed thermal pad
- CE
- EN1 / EN2
- ISET
- ILIM
- TS
- TMR
- ITERM
- CHG
- PGOOD
- required IN, OUT, and BAT capacitors

Do not restore the superseded `800 ohm = 2 A` ILIM assumption. Program input and charge
current from the exact datasheet equations and the documented USB-C source capability.

---

## 5. Power Board — 1S3P Pack Protection

The design uses three 803040 LiPo cells in parallel as one logical 1S3P pack.

### Baseline protection topology; exact parts require BOM confirmation

| Document | Manufacturer | Status | Link |
|---|---|---|---|
| DW01A Single-Cell Li-Ion Protection IC | Fortune Semiconductor | **Baseline topology reference only**; confirm exact manufacturer and orderable part before use | https://www.ic-fortune.com/upload/Download/DW01A-DS-17_EN.pdf |
| FS8205A Dual N-Channel MOSFET | Fortune Semiconductor | **Baseline topology reference only**; Rev. 1.7 manufacturer datasheet | https://www.ic-fortune.com/upload/Download/FS8205A-DS-17_EN.pdf |

### Protection selection rules

- The selected protection IC and dual FET must be reviewed as a matched circuit.
- Copy the selected manufacturer's reference topology exactly.
- Verify overcharge, overdischarge, overcurrent, short-circuit, delay, and release
  thresholds against the selected cells.
- Confirm FET pinout and orientation against the exact package.
- Raw cell negative must not connect directly to system GND.
- If each purchased cell has an internal PCM, document how those PCMs interact with
  the board-level protection before deciding whether both remain populated.

### LiPo cell documentation required

The term `803040` describes dimensions, not a complete electrical specification.

Before ordering or PCB release, add the exact supplier datasheet for the selected cell,
including:

- manufacturer and orderable part number
- nominal and minimum capacity
- maximum continuous discharge current
- maximum charge current
- charge cutoff voltage
- discharge cutoff voltage
- internal protection, if any
- NTC, if any
- connector and wire gauge
- cell thickness tolerance
- swelling allowance
- cycle-life and temperature limits

---

## 6. Power Board — B+1 Buck-Boost

### Active baseline: TPS63070

| Document | Manufacturer | Document / status | Link |
|---|---|---|---|
| TPS63070 2-V to 16-V Buck-Boost Converter with 3.6-A Switch Current | Texas Instruments | **Baseline B+1 regulator**; Rev. B at time of this update | https://www.ti.com/lit/ds/symlink/tps63070.pdf |
| TPS63070 Product Page | Texas Instruments | Current product, package, design tools, and documentation portal | https://www.ti.com/product/TPS63070 |

The final schematic must explicitly handle:

- VIN pins
- VOUT pins
- L1 / L2
- FB and the 6.0 V divider
- FB2 / VSEL
- EN
- PS/SYNC
- VAUX
- PG
- GND and exposed pad
- input/output capacitance
- inductor saturation and RMS current
- startup and transient loading

Current target divider:

```text
R_TOP = 649 kΩ
R_BOTTOM = 100 kΩ

B+1 = 0.8 V × (1 + 649 kΩ / 100 kΩ)
    ≈ 6.0 V
```

The exact inductor, ceramic capacitors, and any bulk capacitor require their own
manufacturer datasheets before BOM freeze.

---

## 7. Servo Board — Local 3.3 V Regulator

### Baseline candidate: MCP1700-3302

| Document | Manufacturer | Document / status | Link |
|---|---|---|---|
| MCP1700 Low-Quiescent-Current LDO | Microchip Technology | **Baseline 3.3 V candidate**; DS20001826F | https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP1700-Data-Sheet-20001826F.pdf |
| MCP1700 Product Page | Microchip Technology | Current ordering and documentation portal | https://www.microchip.com/en-us/product/mcp1700 |

Before locking MCP1700, calculate the complete 3.3 V current budget for:

- STM32 at maximum intended clock and peripheral activity
- USB operation
- status pull-ups
- signal-conditioning loads
- mandatory LED-interface circuitry
- SWD/debug activity
- margin for startup and future firmware behavior

Confirm that 6.0 V operation is acceptable at the selected load, including tolerance,
transients, and dissipation. The MCP1700 datasheet specifies 6.0 V as the upper end of
its normal input operating range, so B+1 tolerance and startup overshoot must be
included in this review.

---

## 8. Servo Board — B+3 Boost Converter

### Baseline candidate: MT3608

| Document | Manufacturer / source | Status | Link |
|---|---|---|---|
| MT3608 Boost Converter | XI'AN Aerosemi Tech; LCSC-hosted manufacturer datasheet mirror | **Baseline candidate**; verify exact supply source and marking | https://www.lcsc.com/datasheet/lcsc_datasheet_1811151539_XI-AN-Aerosemi-Tech-MT3608_C84817.pdf |

The final design must select exact orderable parts and datasheets for:

- MT3608 or replacement boost IC
- boost inductor
- boost rectifier
- input capacitors
- output capacitors
- feedback resistors

Current target output:

```text
B+1 ≈ 6.0 V
B+3 ≈ 10.8 V
R_TOP ≈ 169 kΩ
R_BOTTOM = 10 kΩ
```

The boost rectifier is **selection pending**. Do not treat `SS14` as approved merely
because it appeared in an earlier draft.

Because low-cost MT3608 supply chains can contain substitutions or questionable
markings, record the actual manufacturer/source used for prototypes.

---

## 9. Servo Board — Signal Conditioning and Motor Interface

### Baseline candidate devices

| Document | Manufacturer | Status | Link |
|---|---|---|---|
| MMBT3904 NPN Transistor | onsemi | **Baseline candidate** for the PWM level-shift stage | https://www.onsemi.com/pdf/datasheet/mmbt3904-d.pdf |
| BAT54S Dual Series Schottky Diode | Diodes Incorporated | **Baseline candidate** where a dual rail-clamp package is appropriate | https://www.diodes.com/assets/Datasheets/ds11005.pdf |

These links do not freeze the circuit values.

The following remain measurement-dependent:

- FG divider and clamp values
- motor-enable divider
- RV601/RV602/RV603 dividers and clamps
- Q601 base-interface resistors
- PWM filter values
- safe-off pull-up arrangement
- LED drive/buffer requirements

Do not select or size these components solely from generic example circuits.

---

## 10. Sony WM-D6C Source Documents

These are project source documents rather than manufacturer-hosted semiconductor
datasheets.

| Document | Role in Rev A |
|---|---|
| `fb4872.pdf` | **Governing Sony manual** for the former-type CX20084 circuit used by the target unit |
| `sony_wm-d6c_tc-d6c_ver-1.1.pdf` | Revision supplement used to distinguish former-type boards from the later CX-069A/Q601–Q605 circuit |
| `sony_wm-d6c_tc-d6c_ver-1.1_sm.pdf` | Alternate local filename for the same correct v1.1 PDF, if retained by the project source system |

### Confirmed target-board interpretation

The v1.1 manual's new/former-type discrimination identifies the
`1-611-494-12` family corresponding to PCB marking `C11-494-12` as former type.

Therefore Rev A uses:

- CX20084 former-type circuit
- single-Q601 control architecture
- original/former-type service manual as the primary Sony schematic

The later CX-069A/Q601–Q605 circuit is not a source for Rev A interface values.

### Sony components without reliable public datasheets

| Component | Treatment |
|---|---|
| CX20084 | Proprietary Sony servo ASIC; use service manual and bench measurements |
| CP304 | Proprietary/potted Sony DC-DC converter; use service manual interface data |
| CX10043 | Use Sony LED/VU schematic and bench measurements |
| FG901 / GP2S22AB | Use Sony schematic plus measured waveform and source impedance |
| Q601 | Service manual establishes a PNP control path, but exact SMD marking/package on serial 72795 must be physically confirmed before linking an external transistor datasheet |

Do not retain the previous unverified `2SB1013` Q601 entry as an approved Rev A part.

---

## 11. Mandatory LED Battery Monitor References

The LED battery monitor is part of the Rev A design, not an option.

Its electrical design depends primarily on:

- Sony service-manual schematic for D801–D805, R814–R818, IC801/CX10043, Q801, and S801
- STM32G0C1 GPIO electrical limits
- measured LED polarity and current
- measured CX10043 behavior in BATT mode
- the exact buffer transistor or MOSFET datasheet, if direct STM32 drive is not safe

No LED buffer part is approved yet. Add the exact buffer-device datasheet after bench
characterization determines whether the STM32 can safely drive or release the five
lines without contention.

---

## 12. Connectors and Mechanical Parts Requiring Exact Datasheets

Add exact manufacturer datasheets before PCB layout for:

| Item | Required review |
|---|---|
| USB-C receptacle | Pin numbering, shell, footprint, current, mechanical life |
| USB power harness to Power Board | Current rating, polarization, wire gauge |
| USB data/CC harness or flex to Servo Board | USB FS suitability, ground reference, impedance, length |
| Power Board telemetry connector | Pin current, pitch, latching, polarization |
| CP304 replacement connector/harness | Physical pinout, current rating, fit |
| Sony servo-interface harness | Pinout, pitch, retention, installation clearance |
| LED-board harness | Pinout, current, retention |
| SWD/Tag-Connect footprint | Exact cable and footprint drawing |
| Cell connectors | Current rating, polarization, mating cycles, wire size |

Generic connector-family names are not enough to approve footprints.

---

## 13. Removed or Superseded Rev A Components

The following parts belonged to earlier wall-power or all-in-one architectures and
are not active Rev A battery-build components:

| Component / block | Rev A status |
|---|---|
| STM32G0B1KBU6 | Superseded by STM32G0C1KCU6 |
| IP2721 USB PD trigger | Removed; Rev A is 5 V only |
| LTC4359 barrel-jack polarity protection | Removed with barrel-jack architecture |
| AP63203 buck regulator | Removed |
| Servo-board B+1 generator | Removed; Power Board generates B+1 |
| Power-board MT3608 | Removed; B+3 generation belongs on Servo Board |
| Power-board MCP1700 | Removed; +3V3 generation belongs on Servo Board |
| 9 V PD rail | Removed |
| SMBJ7.0A from the old barrel-jack path | Not part of current Rev A baseline |

Links for these parts are intentionally omitted from the active sections to avoid
accidental reuse.

---

## 14. Datasheet Capture Checklist

Before a component becomes BOM-approved:

- [ ] Exact manufacturer is recorded.
- [ ] Exact orderable part number is recorded.
- [ ] Package suffix matches the schematic symbol and footprint.
- [ ] Manufacturer datasheet is linked.
- [ ] Absolute maximum ratings are checked.
- [ ] Normal operating range is checked.
- [ ] Pinout is checked against the KiCad symbol.
- [ ] Recommended external components are checked.
- [ ] Thermal requirements are checked.
- [ ] Startup, shutdown, and unpowered behavior are checked.
- [ ] Reverse-current and back-power paths are checked.
- [ ] Component tolerances are included in calculations.
- [ ] Bench-test requirements are recorded.
- [ ] Distributor-only mirrors are identified as mirrors.
- [ ] Selection-pending parts are not presented as final.

---

## 15. License and Attribution

All linked documents are subject to their respective owners' copyright and terms of
use. Links are provided solely for engineering reference. No endorsement by Sony,
STMicroelectronics, Texas Instruments, Microchip Technology, Fortune Semiconductor,
onsemi, Diodes Incorporated, XI'AN Aerosemi Tech, or any distributor is implied.
