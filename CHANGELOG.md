# Changelog

All notable changes to the wmd6c-servo project are documented here.

This file is written in plain English, not git log summaries. Each entry describes
what changed, why it changed, and whether existing boards or installations are
affected. The format is designed to be readable by someone who owns an installed
module and wants to know whether they need to update.

---

## [Unreleased] — Design Phase

### Hardware
- Board outline pending: CP304 cavity dimensions to be measured on SN72795 unit
  arriving 2 June 2025
- Motor output stage committed to PWM + RC filter + NPN level-shift (TIM3_CH1 PA6):
  Q601 (2SB1013 PNP) emitter at B+3 (10.8V) rules out direct DAC drive on all
  Ver. 1.0 CX20084 boards. Q601 base voltage measurement still required to confirm
  R9 sizing.
- R3/R4 FG divider values pending: FG901 swing voltage measurement required on
  target unit

### Firmware
- All peripheral initialisation code pending bench verification of hardware
- FG_TARGET_HZ pending test tape measurement on target unit
- Initial PI constants (Kp = 0.15, Ki = 0.008) are simulation-derived starting
  points — will be updated with bench-tuned values after first hardware build

### Documentation
- Installation guide pending: requires photographs of actual installation on SN72795
- Bench measurement log for SN72795 pending machine arrival
- Theory documents complete: original servo circuit, digital PLL servo, fixed-point
  arithmetic, why this failed

---

## How to Read This File

Each entry is tagged with one of:

**Hardware** — changes to the KiCad schematic, PCB layout, gerbers, or BOM. If your
board is already manufactured, check whether the change affects your revision.

**Firmware** — changes to the C source code or config constants. Pre-built binaries
for each release are in `firmware/releases/`. Update via USB DFU.

**Documentation** — changes to guides, theory documents, or installation
instructions. No hardware or firmware action required.

**[BREAKING]** — a change that requires action from anyone with an installed module.
Breaking changes are rare and are clearly marked.

---

*This project follows [Semantic Versioning](https://semver.org/). v1.0.0 will be
tagged when the hardware has been verified on a physical unit and the installation
guide is complete with photographs.*
