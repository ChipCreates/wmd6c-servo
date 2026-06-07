# wmd6c-servo — DSR-1

**Open-source / open-hardware servo, power, and USB-C service subsystem for the Sony Walkman WM-D6C / TC-D6C**

DSR-1 is an in-development replacement and modernization subsystem for the Sony WM-D6C / TC-D6C. Rev A targets the **CX20084 former-type servo family**, with the first physical unit identified as WM-D6C serial `72795`, PCB marking `C11-494-12`, and a surface-mount CX20084 at IC601. Its purpose is to preserve machines by replacing the fragile capstan-servo IC path, modernizing the external power interface, and adding USB-C data/service access for tuning, telemetry, firmware update, and diagnostics.

This project is not yet a finished installation-ready repair. It is currently in **source-grounded design and firmware-prototype development**. The firmware architecture exists, the KiCad files are in-flight and not yet synchronized with the latest documentation, and the design is being revised against the Sony WM-D6C/TC-D6C service manual and the selected STM32G0C1KCU6 device. PCB layout, hardware validation, installation documentation, and performance testing remain pending.

DSR-1 should be understood as an integrated two-board subsystem project, not a servo-only daughterboard: a **Power Board** and a **Servo Control Board**.

---

## Project Intent

The Sony WM-D6C is a professional-grade portable cassette recorder with a quartz-lock capstan servo system, excellent speed stability, and a transport worth preserving. Many surviving units are threatened by failures in and around the servo and power-input circuitry:

- **CX20084 / IC601 servo failure** — the original capstan-servo IC is obsolete and difficult to source reliably.
- **CN301 external power risk** — the original DC input arrangement is vulnerable to wrong-adapter failures.
- **CP304 / power-support aging** — the surrounding DC-DC and power-support circuitry is now decades old.
- **Serviceability limits** — the original analog servo has limited internal observability and cannot easily expose measured FG period, loop error, correction output, or tuning state.
- **Modern service access gap** — the original machine has no digital tuning, telemetry, firmware update path, or structured diagnostic interface.

DSR-1 aims to make the WM-D6C repairable without reducing it to a generic motor controller. The goal is not to modernize for its own sake. The goal is to preserve the machine by replacing an increasingly fragile control and power-support subsystem with one that can be measured, tuned, documented, powered safely, and reproduced.

---

## Current Status

**Status: active design / firmware prototype. Not bench-validated. Not ready for installation in a valuable machine.**

| Area | Status |
|---|---|
| Architecture | Defined at a high level |
| Sony service-manual review | In progress |
| STM32G0C1KCU6 package/pinout review | Accepted baseline; firmware migration pending |
| Firmware prototype | Register-level C source exists; bench validation pending |
| USB-C data/service path | Design requirement; validation pending |
| USB-C PD / power path | First-class design requirement; final implementation pending |
| KiCad schematic | In-flight legacy project exists; manual split to Power Board and Servo Control Board pending |
| PCB layout | Not complete |
| Bench measurements | Required before Rev A finalization |
| Installation guide | Planned; not yet valid for users |
| Pre-built module | Not available |
| Performance claims | Pending measured speed, power, USB, and wow/flutter data |

Until the electrical interface is measured on a real WM-D6C and a Rev A board is validated, DSR-1 should be treated as an engineering project, not a repair product.

---

## Compatibility Scope

DSR-1 targets the **full WM-D6C / TC-D6C production run (1984–2002)**. Two servo circuit families exist:

| Circuit family | Years | Servo IC | Rev A status |
|---|---|---|---|
| Ver. 1.0 | 1984 – mid 2001 | CX20084 | **Primary Rev A target** |
| Ver. 1.1 | mid 2001 – 2002 | CX-069A | Planned post-Rev A variant |

Rev A targets CX20084 boards — the large majority of surviving units. CX-069A board support is planned as a separate variant once Rev A is validated. See `docs/hardware/wmd6c-revision-compatibility.md` for detailed production timeline and identification guidance.

The primary unit currently being mapped is serial `72795`, observed PCB marking
`C11-494-12`, with a surface-mount CX20084 at IC601. Earlier assumptions that this
unit was a through-hole board are superseded.

---

## Governing References

Rev A is being grounded against these primary references:

1. **Sony WM-D6C / TC-D6C Service Manual, original edition (`fb4872.pdf`)**
   - Governing reference for Ver. 1.0 CX20084 servo circuit, component values, board layout, and tape speed calibration procedure.

2. **Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1, 2001.06 (`sony_wm-d6c_tc-d6c_ver-1.1.pdf`)**
   - Documents the 2001 servo circuit change to CX-069A. Reference for Ver. 1.1 variant design (post Rev A).

3. **STMicroelectronics STM32G0C1KCU6 / STM32G0 family documentation**
   - Primary reference for MCU capabilities, electrical limits, package constraints, ADC/timer/USB/UCPD features, and operating conditions.

The repository documentation should distinguish clearly between:

- facts supported by the Sony service manual,
- facts supported by the selected STM32G0C1KCU6 package/device documentation,
- behavior implemented in source code,
- behavior measured on the bench,
- and design goals not yet proven.

---

## Design Position

DSR-1 is now scoped as an integrated two-board subsystem with three first-class domains:

| Domain | Purpose |
|---|---|
| Capstan servo | Replace the obsolete CX20084-based servo path with a measurable digital control loop |
| Power | Power Board for USB-C input, charger, LiPo/protection, regulation, and battery telemetry |
| USB-C data/service | Provide telemetry, live tuning, firmware update, diagnostics, and optional PD negotiation |

The servo loop remains the hardest performance-critical problem, but power and USB-C are not deferred side features. They are part of the Rev A architecture and must be represented in schematics, firmware, verification plans, and documentation.

---

## Known Target Unit Facts

The first physical target is a **former-type CX20084 WM-D6C**, not the 2001 CX-069A
new-type circuit.

| Machine / board | Current status |
|---|---|
| WM-D6C serial `72795` | Primary physical unit |
| PCB marking `C11-494-12` | Observed on the actual unit |
| IC601 | Surface-mount CX20084 |
| CX-069A boards | Planned post-Rev A variant only |
| WM-D6 | Future variant candidate only |
| WM-D3 / WM-D3C | Future variant candidate only |
| TC-D5M and related Sony professional machines | Future variant candidates only |

A variant port will require a schematic comparison, a harness/interface map, measured FG target data, power-interface notes, USB/service constraints, and proof of operation on physical hardware.

---

## What DSR-1 Is Intended to Replace or Add

The project boundary is larger than the CX20084 alone. The intended Rev A design work concerns the WM-D6C capstan-servo subsystem, the power path required to support it, and a USB-C service/data interface.

| Original function / missing function | DSR-1 design intent |
|---|---|
| CX20084 / IC601 capstan-servo function | STM32G0C1KCU6 digital PI servo loop using timer input capture |
| FG feedback path from motor / FG901 | Protected, conditioned MCU timer-capture input |
| Motor correction output to Sony drive network | PWM + RC filter + NPN level-shift to Q601 base; exact Q601 part/package pending physical confirmation |
| RV601 base speed adjustment | Retained as measured analog input |
| RV602 Speed Tune control | Retained as measured analog input |
| RV603 Speed Tune range control | Retained as measured analog input |
| S601 Speed Tune switch | Retained as digital input, polarity to be verified |
| S801 VU/BATT switch and D801-D805 LEDs | `S801_BATT` enables MCU battery-gauge ownership; MCU drives LEDs only in BATT mode and releases them in VU/non-BATT mode |
| CP304 / supporting power conversion | Replacement or support circuit pending Rev A power design |
| CN301 external power vulnerability | Protected input and/or USB-C power path |
| USB service interface absent in original | USB CDC / DFU / diagnostics / telemetry |
| USB-C PD absent in original | Power negotiation strategy, implementation pending |

The original transport, heads, audio path, user controls, and mechanical adjustment procedures should remain intact wherever possible.

---

## Architecture Overview

DSR-1 is designed around an STM32G0C1KCU6 microcontroller running register-level bare-metal C.

Planned/implemented firmware architecture:

- **FG measurement:** TIM2 input capture measures the period between FG pulses.
- **Servo loop:** PI control calculates motor correction from measured period error.
- **Output stage:** TIM3 PWM filtered through an RC network and NPN level-shift transistor (Q_LS MMBT3904) drives Q601's base on the WM-D6C main board.
- **Speed controls:** RV601/RV602/RV603 and S601 are retained and read by the MCU after voltage-safety verification.
- **USB data/service:** USB CDC is intended to expose live tuning, telemetry, command/status reporting, and diagnostics.
- **Firmware update:** USB DFU or other update path is intended to avoid requiring permanent SWD access after installation.
- **USB-C / power:** CC1/CC2 map to PA8/PA9. Variant A may use PD support; the battery build uses plain 5V USB-C into the Power Board charger.
- **Battery indicator:** In the battery build, the MCU reads `VBAT_SENSE`, `CHG_STAT`, `PGOOD`, and `S801_BATT`; it drives `BATT_LED1-5` only while S801 is in BATT.
- **Persistence:** Tuned constants are intended to be saved to flash with validation.
- **Debugging:** SWD remains available during development.

Important design caveat: the target period stored in flash is numerically stable, but final speed accuracy depends on the MCU timebase. The STM32 internal oscillator is suitable for firmware development, but final speed-performance claims require either a proven timing strategy, an external reference, calibration, or measured evidence that the selected clocking approach meets the WM-D6C speed requirements across realistic conditions.

---

## Rev A Engineering Priorities

The next project milestone is a source-grounded electrical interface and a bench-measured Rev A design covering servo, power, and USB-C.

Required before Rev A PCB finalization:

1. Map every DSR-1 signal to a Sony service-manual reference or a DSR-1-only service function.
2. Measure FG901 waveform voltage, offset, edge shape, noise, and frequency at correct speed.
3. Measure Q601 / motor-drive control voltage range during stop, startup, and play.
4. Measure RV601, RV602, and RV603 wiper voltage ranges and source behavior.
5. Verify S601 Speed Tune switch logic polarity and voltage.
6. Verify CN301 / battery / CP304 rail behavior under startup and play load.
7. Define the USB-C data/service connector role, ESD protection, shield/ground strategy, and firmware mode behavior.
8. Verify the S801 BATT-position sense, LED polarity/current limits, and CX10043 high-Z/release behavior before MCU LED ownership is finalized.
9. Decide the final servo timebase strategy.
10. Split and repopulate the KiCad schematic into the Power Board and Servo Control Board projects from measured interface requirements.
11. Validate power rails, reverse-polarity protection, USB enumeration, DFU/update behavior, and servo loop behavior before real-machine installation.
12. Publish measured speed, wow/flutter, power, and USB validation results before claiming original-equivalent or improved performance.

---

## Power and USB-C Strategy

Power and USB-C are part of the project scope.

| Function | Rev A intent |
|---|---|
| Protected external power | Required |
| Reverse-polarity protection | Required |
| Overcurrent / fault protection | Required |
| USB-C connector | Required for service/data; power role to be finalized |
| USB CDC telemetry | Required design goal |
| Firmware update path | Required design goal |
| USB-C PD | First-class design requirement; implementation architecture pending |
| Original barrel jack retention | Optional variant decision |
| CN301 replacement/removal | Optional variant decision |

Possible power approaches:

| | USB-C-centered variant | Protected barrel / hybrid variant |
|---|---|---|
| Power input | USB-C power path with PD strategy | Original-style or lab/protected input retained |
| User-facing behavior | Modern external power and data through USB-C | More conservative power bring-up |
| Complexity | Higher | Lower |
| Project status | In scope | In scope |

The preferred architecture should be selected by schematic feasibility, pin availability, installation constraints, safety behavior, and bench validation—not by minimizing scope.

---

## Repository Structure

```text
wmd6c-servo/
├── hardware/        KiCad project, schematic work, future PCB/BOM/gerbers
├── firmware/        Bare-metal STM32G0 family firmware prototype
├── docs/            Theory, datasheet references, installation and validation docs
└── tools/           Calibration and development utilities
```

Some paths and build commands are still being normalized as the project moves from design notes into a reproducible hardware/firmware package.

---

## Firmware Development

The firmware is written in C for the STM32G0 family, using CMSIS device headers and direct register access. The hardware target is STM32G0C1KCU6; some firmware filenames and comments may still reflect the earlier STM32G0B1KBU6 prototype until the firmware migration is completed. No STM32 HAL, no RTOS, and no dynamic memory allocation are intended.

Current firmware areas:

- `main.c` — clock setup, peripheral initialization order, main loop
- `servo.c` / `servo.h` — TIM2 capture ISR, PI loop, TIM3 PWM output path
- `adc.c` / `adc.h` — speed-control potentiometer scan and target adjustment support
- `flash.c` / `flash.h` — persistent settings storage
- `usb_cdc.c` / `usb_cdc.h` — USB CDC command and telemetry path
- `config.h` — tunable constants and hardware-selection flags

Firmware areas now required by project scope:

- USB CDC service and telemetry must remain isolated from the real-time servo loop.
- USB DFU or equivalent firmware update path must be documented and tested.
- USB-C attach/power-role behavior must be specified.
- If native STM32 UCPD is used, PD firmware must not compromise servo timing.
- If an external PD controller is used, firmware must define any required monitoring/control interface.
- Battery-gauge firmware must treat `S801_BATT` as the LED ownership enable and keep `BATT_LED1-5` released in VU/non-BATT modes.

Current firmware caveats:

- Bench validation has not yet been completed.
- The adjusted-target path from the analog speed controls must be verified end-to-end.
- The timebase decision remains open.
- USB-C PD implementation architecture remains open.
- Build layout and CI should be normalized before outside contributors rely on the firmware package.

See the firmware README for current build notes.

---

## Documentation Plan

The repository should grow toward the following documentation set:

```text
docs/
├── hardware/
│   ├── wmd6c-interface-contract.md
│   ├── wmd6c-revision-compatibility.md
│   ├── power-usb-c-architecture.md
│   └── timebase-decision.md
├── theory/
│   ├── original-servo-circuit.md
│   ├── digital-pll-servo.md
│   ├── signal-chain-analysis.md
│   └── power-supply-design.md
├── verification/
│   ├── 00-rev-a-bringup-checklist.md
│   ├── 01-wmd6c-preinstall-measurements.md
│   ├── 02-fg901-waveform-capture.md
│   ├── 03-motor-drive-characterization.md
│   ├── 04-speed-calibration-procedure.md
│   ├── 05-wow-flutter-test-plan.md
│   ├── 06-power-fault-test-plan.md
│   └── 07-usb-c-data-pd-test-plan.md
└── installation/
    └── planned after Rev A validation
```

Installation documentation should not be treated as valid until the hardware has been built, tested, photographed, and verified against the Sony adjustment procedure.

---

## Contributing

Contributions are welcome, but the project is still early enough that measurements and source-grounded review are more valuable than feature additions.

Most useful contributions right now:

- WM-D6C board-revision identification and photographs.
- Clear scans or notes that distinguish former/new servo circuit revisions.
- Bench measurements of FG901, Q601 motor-control behavior, RV601/RV602/RV603, S601, CP304, CN301, and original power rails.
- STM32G0C1KCU6 pinout and package review for servo, USB, UCPD, SWD, ADC, PWM, and timer conflicts.
- USB-C PD architecture review, including native UCPD versus external controller tradeoffs.
- Firmware review focused on interrupt timing, timer capture, ADC safety, flash behavior, USB CDC isolation, and PD interaction.
- KiCad schematic review once the Rev A sheets are populated.

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for contribution expectations.

---

## License

| Artifact | License |
|---|---|
| Hardware design files | [CERN OHL-P v2](LICENSE_HARDWARE.txt) |
| Firmware source | [MIT](LICENSE) |
| Documentation | [CC BY 4.0](LICENSE_DOCS.txt) |

---

## Project Position

DSR-1 is being developed as a preservation-oriented engineering project. Its purpose is to make the WM-D6C repairable, measurable, maintainable, safely powered, and digitally serviceable without erasing what made the original machine worth saving.

Performance and safety claims will be earned by measurement, not assumed from architecture.
