# wmd6c-servo — DSR-1

**Open-source / open-hardware digital capstan-servo replacement project for the Sony Walkman WM-D6C / TC-D6C**

DSR-1 is an in-development replacement module for the WM-D6C capstan-servo subsystem. Its goal is to replace the ageing CX20084-based servo path with a source-grounded, observable, tunable STM32G0B1-based digital servo while preserving the character and serviceability of the original machine.

This project is not yet a finished drop-in repair module. It is currently in **source-grounded design and firmware-prototype development**. The firmware architecture exists, the KiCad project structure exists, and the design is being revised against the Sony WM-D6C/TC-D6C service manual and the STM32G0B1 datasheet. PCB layout, hardware validation, installation documentation, and performance testing remain pending.

---

## Project Intent

The Sony WM-D6C is a professional-grade portable cassette recorder with a quartz-lock capstan servo system, excellent speed stability, and a transport worth preserving. Many surviving units are threatened by failures in and around the servo and power-input circuitry:

- **CX20084 / IC601 servo failure** — the original capstan-servo IC is obsolete and difficult to source reliably.
- **CN301 external power risk** — the original DC input arrangement is vulnerable to wrong-adapter failures.
- **CP304 / power-support aging** — the surrounding DC-DC and power-support circuitry is now decades old.
- **Serviceability limits** — the original analog servo has limited internal observability and cannot easily expose measured FG period, loop error, correction output, or tuning state.

DSR-1 aims to make the WM-D6C repairable without reducing it to a generic motor controller. The goal is not to modernize for its own sake. The goal is to preserve the machine by replacing an increasingly fragile control subsystem with one that can be measured, tuned, documented, and reproduced.

---

## Current Status

**Status: active design / firmware prototype. Not bench-validated. Not ready for installation in a valuable machine.**

| Area | Status |
|---|---|
| Architecture | Defined at a high level |
| Sony service-manual review | In progress |
| STM32G0B1 datasheet review | In progress |
| Firmware prototype | Register-level C source exists; bench validation pending |
| KiCad schematic | Project hierarchy exists; functional sheets still being populated |
| PCB layout | Not complete |
| Bench measurements | Required before Rev A finalization |
| Installation guide | Planned; not yet valid for users |
| Pre-built module | Not available |
| Performance claims | Pending measured speed and wow/flutter data |

Until the electrical interface is measured on a real WM-D6C and a Rev A board is validated, DSR-1 should be treated as an engineering project, not a repair product.

---

## Governing References

Rev A is being grounded against these primary references:

1. **Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1, 2001.06**
   - Primary reference for the WM-D6C/TC-D6C service procedures, schematic, board layout, adjustments, parts identification, and the documented new-servo-circuit revision.

2. **STMicroelectronics STM32G0B1xB/xC/xE Datasheet, DS13560 Rev 6, February 2026**
   - Primary reference for MCU capabilities, electrical limits, package constraints, ADC/DAC/timer/USB/UCPD features, oscillator characteristics, flash, and operating conditions.

The repository documentation should distinguish clearly between:

- facts supported by the Sony service manual,
- facts supported by the STM32G0B1 datasheet,
- behavior implemented in source code,
- behavior measured on the bench,
- and design goals not yet proven.

---

## Compatibility Scope

The first target is the **Sony WM-D6C / TC-D6C Ver. 1.1 servo circuit** documented in the 2001 service-manual supplement.

Earlier WM-D6C servo revisions and related machines must not be assumed compatible until their schematics, harness points, FG behavior, speed-control network, and motor-drive interface are separately mapped and measured.

| Machine / board | Current status |
|---|---|
| WM-D6C / TC-D6C Ver. 1.1 new servo circuit | Primary reference target |
| Older WM-D6C servo-circuit revisions | Not yet mapped |
| WM-D6 | Future variant candidate only |
| WM-D3 / WM-D3C | Future variant candidate only |
| TC-D5M and related Sony professional machines | Future variant candidates only |

A variant port will require a schematic comparison, a harness/interface map, measured FG target data, and proof of operation on physical hardware.

---

## What DSR-1 Is Intended to Replace

The project boundary is larger than the CX20084 alone. The intended Rev A design work concerns the WM-D6C capstan-servo subsystem and the power path required to support it.

| Original function | DSR-1 design intent |
|---|---|
| CX20084 / IC601 capstan-servo function | STM32G0B1 digital PI servo loop using timer input capture |
| FG feedback path from motor / FG901 | Protected, conditioned MCU timer-capture input |
| Motor correction output to Sony drive network | DAC or PWM/level-shift output, selected after bench measurement |
| RV601 base speed adjustment | Retained as measured analog input |
| RV602 Speed Tune control | Retained as measured analog input |
| RV603 Speed Tune range control | Retained as measured analog input |
| S601 Speed Tune switch | Retained as digital input, polarity to be verified |
| CP304 / supporting power conversion | Replacement or support circuit pending Rev A power design |
| CN301 external power vulnerability | Protected barrel input or USB-C power path, revision-dependent |

The original transport, heads, audio path, user controls, and mechanical adjustment procedures should remain intact wherever possible.

---

## Architecture Overview

DSR-1 is designed around an STM32G0B1 microcontroller running register-level bare-metal C.

Planned/implemented firmware architecture:

- **FG measurement:** TIM2 input capture measures the period between FG pulses.
- **Servo loop:** PI control calculates motor correction from measured period error.
- **Output stage:** 12-bit DAC or PWM/filtered level-shift output drives the motor-control point, depending on measured Sony circuit requirements.
- **Speed controls:** RV601/RV602/RV603 and S601 are retained and read by the MCU after voltage-safety verification.
- **Telemetry:** USB CDC is intended to expose live tuning and servo state.
- **Persistence:** Tuned constants are intended to be saved to flash with validation.
- **Debugging:** SWD remains available during development.

Important design caveat: the target period stored in flash is numerically stable, but final speed accuracy depends on the MCU timebase. The STM32 internal oscillator is suitable for firmware development, but final speed-performance claims require either a proven timing strategy, an external reference, calibration, or measured evidence that the selected clocking approach meets the WM-D6C speed requirements across realistic conditions.

---

## Rev A Engineering Priorities

The next project milestone is not cosmetic fit or installation packaging. The next milestone is a source-grounded electrical interface and a bench-measured Rev A design.

Required before Rev A PCB finalization:

1. Map every DSR-1 signal to a Sony service-manual reference.
2. Measure FG901 waveform voltage, offset, edge shape, noise, and frequency at correct speed.
3. Measure Q601 / motor-drive control voltage range during stop, startup, and play.
4. Measure RV601, RV602, and RV603 wiper voltage ranges and source behavior.
5. Verify S601 Speed Tune switch logic polarity and voltage.
6. Verify CN301 / battery / CP304 rail behavior under startup and play load.
7. Decide the final servo timebase strategy.
8. Populate the KiCad schematic from measured interface requirements.
9. Validate the servo loop first with simulated FG input, then on a real transport.
10. Publish measured speed and wow/flutter results before claiming original-equivalent or improved performance.

---

## Power Variants

The original concept supports two possible power approaches. These are design targets, not validated release options.

| | Variant A | Variant B |
|---|---|---|
| Power input | USB-C power path | Protected barrel input |
| User-facing behavior | Modern external power | Original-style external power retained |
| Risk profile | More firmware/hardware complexity if using native USB-C PD | Simpler Rev A validation path |
| Recommended role | Later revision unless needed immediately | Preferred first bench-validation path |

For Rev A, the safer engineering path is to prove the servo replacement first using a protected, well-characterized power input. USB-C PD can be added once the servo interface, clocking strategy, and motor-control behavior are proven.

---

## Repository Structure

```text
wmd6c-servo/
├── hardware/        KiCad project, schematic work, future PCB/BOM/gerbers
├── firmware/        Bare-metal STM32G0B1 firmware prototype
├── docs/            Theory, datasheet references, installation and validation docs
└── tools/           Calibration and development utilities
```

Some paths and build commands are still being normalized as the project moves from design notes into a reproducible hardware/firmware package.

---

## Firmware Development

The firmware is written in C for the STM32G0B1 family, using CMSIS device headers and direct register access. No STM32 HAL, no RTOS, and no dynamic memory allocation are intended.

Current firmware areas:

- `main.c` — clock setup, peripheral initialization order, main loop
- `servo.c` / `servo.h` — TIM2 capture ISR, PI loop, DAC/PWM output path
- `adc.c` / `adc.h` — speed-control potentiometer scan and target adjustment support
- `flash.c` / `flash.h` — persistent settings storage
- `usb_cdc.c` / `usb_cdc.h` — USB CDC command and telemetry path
- `config.h` — tunable constants and hardware-selection flags

Current firmware caveats:

- Bench validation has not yet been completed.
- The adjusted-target path from the analog speed controls must be verified end-to-end.
- The timebase decision remains open.
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
│   └── 06-power-fault-test-plan.md
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
- Bench measurements of FG901, Q601 motor-control behavior, RV601/RV602/RV603, S601, CP304, and CN301.
- STM32G0B1 pinout and package review for the exact selected part.
- Firmware review focused on interrupt timing, timer capture, ADC safety, flash behavior, and USB CDC isolation.
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

DSR-1 is being developed as a preservation-oriented engineering project. Its purpose is to make the WM-D6C repairable, measurable, and maintainable without erasing what made the original machine worth saving.

Performance claims will be earned by measurement, not assumed from architecture.
