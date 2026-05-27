# wmd6c-servo — DSR-1

**Open source digital servo replacement module for the Sony Walkman WM-D6C**

Replaces three ageing and increasingly unavailable components — the CX20084 capstan
servo IC, the CP304 DC-DC boost converter, and the CN301 power input jack — with a
single compact PCB based on the STM32G0B1KBU6 microcontroller. The result is a
WM-D6C with better speed stability than the original, modern power input, and full
protection against the reverse-polarity failure that kills most surviving units.

---

## What This Fixes

The WM-D6C is a professional-grade portable cassette recorder with a 0.05% wow and
flutter specification that still impresses today. Two problems prevent most surviving
units from being used:

**The CX20084 is dead.** The servo IC fails instantly when a standard centre-positive
DC adapter is plugged into CN301 — Sony's non-standard negative-centre power jack.
The IC has been out of production for decades. Replacements are scarce and unreliable.

**CN301 is a trap.** The original barrel jack has the opposite polarity to every
generic adapter made in the last 40 years. It has no protection. One wrong adapter
destroys the servo IC immediately and silently.

This module eliminates both problems permanently.

---

## Two Variants — One PCB

| | Variant A | Variant B |
|---|---|---|
| **Power input** | USB-C Power Delivery | Barrel jack (CN301 retained) |
| **Input voltage** | 9V negotiated via PD | 4.5V – 9V, either polarity |
| **Polarity protection** | Inherent in USB-C | LTC4359 ideal diode bridge |
| **Overcurrent protection** | PD source limiting | Resettable polyfuse |
| **Overvoltage protection** | PD contract capped at 9V | SMBJ7.0A TVS clamp |
| **CN301 disposition** | Removed, opening modified | Retained, now fully protected |

Both variants share identical firmware and the same PCB gerber. Only the populated
components differ.

---

## What Gets Replaced

| Original Component | Replaced By |
|---|---|
| CX20084 (IC601) — capstan servo IC | STM32G0B1KBU6 digital PI servo loop |
| CP304 — DC-DC boost converter | MT3608 synchronous boost (6V → 10.8V) |
| CN301 — negative-centre barrel jack | USB-C PD (Variant A) or protected barrel jack (Variant B) |
| X701 34.7kHz crystal | Not needed — STM32 uses internal oscillators |
| IC701 MSM58141RS crystal divider | Not needed — replaced by firmware |

Everything else in the machine is retained and reused unchanged.

---

## Key Technical Details

- **MCU:** STM32G0B1KBU6 — UFQFPN32, 64 MHz Cortex-M0+, 128KB flash, 144KB RAM
- **Servo loop:** Digital PI controller, TIM2 input capture, 12-bit DAC output
- **Control rate:** Runs on every FG pulse — approximately 2500 Hz at correct speed
- **Boot time:** ~2ms from power-on to active servo loop
- **USB:** Crystal-less USB 2.0 Full Speed — CDC virtual COM for live tuning and DFU firmware updates
- **Firmware:** Bare-metal C, no HAL, no RTOS, arm-none-eabi-gcc
- **Board:** 2-layer SMD, target ~30 × 22mm, fits CP304 board cavity

---

## Repository Structure

```

wmd6c-servo/
├── hardware/        KiCad schematic, PCB, gerbers, BOM, datasheet links
├── firmware/        Bare-metal C source, Makefile, pre-built releases
├── docs/            Module datasheet, installation guide, theory, bench logs
└── tools/           Python CDC monitor, FG period calculator
```

---

## Getting Started

### Installing a Pre-Built Module

See [docs/installation](docs/installation/) for the complete step-by-step guide
with photographs. The process takes approximately 45 minutes and requires only
basic soldering skills.

### Building From Source

```bash
git clone https://github.com/ChipCreates/wmd6c-servo.git
cd wmd6c-servo/firmware/build
make VARIANT=wmd6c
```

Requires `arm-none-eabi-gcc`. No IDE needed.

### Updating Firmware via USB DFU

Hold the BOOT0 test point while connecting USB-C. The module appears as a DFU
device. Flash `firmware/releases/wmd6c-servo-vX.X.bin` using STM32CubeProgrammer
(Windows) or `dfu-util` (macOS/Linux). No ST-Link required.

---

## Documentation

- [Module Datasheet (PDF)](docs/datasheet/WMD6C_Module_Datasheet.pdf) — complete hardware and firmware reference
- [System Block Diagram (PDF)](docs/datasheet/WMD6C_Block_Diagram.pdf) — dual variant architecture overview
- [Datasheet Links](hardware/datasheets/DATASHEET_LINKS.md) — all referenced component datasheets
- [Installation Guide](docs/installation/) — step-by-step with photographs
- [Theory Documents](docs/theory/) — servo loop, signal chain, power supply design

---

## Project Status

**Design phase — schematics in progress. Pending bench verification on target unit.**

- [x] Architecture defined
- [x] Component selection complete
- [x] Module datasheet written
- [x] Repository structure established
- [ ] KiCad schematics
- [ ] PCB layout
- [ ] Bench verification (unit arrives Monday 2 June 2025)
- [ ] Firmware
- [ ] Installation guide with photographs
- [ ] First hardware revision

---

## Contributing

Contributions are welcomed — bug reports, variant ports, firmware improvements,
and documentation translations. See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

**Porting to another machine?** A variant port requires three files: a schematic
diff from the reference design, a harness pinout diagram, and the calibrated FG
target period for that machine's capstan speed. See
[docs/variants/template](docs/variants/template/) to get started.

---

## License

| Artifact | License |
|---|---|
| Hardware (KiCad files, gerbers, BOM) | [CERN OHL-P v2](LICENSE_HARDWARE.txt) |
| Firmware (C source, Makefile, linker script) | [MIT](LICENSE) |
| Documentation | [CC BY 4.0](LICENSE_DOCS.txt) |

All three licenses require attribution only. No copyleft, no share-alike, no
restrictions on commercial use or closed derivative works.

---

*DSR-1 — Digital Servo Replacement, Revision 1*
*Designed for the Sony Walkman WM-D6C — Compatible with all suffix revisions*
