# Contributing to wmd6c-servo

Thank you for your interest in contributing. This project exists to restore and
preserve professional Sony Walkman machines for the communities that use and love
them. Every contribution — bug reports, variant ports, firmware improvements,
documentation translations, and installation photographs — makes the project more
useful to more people.

---

## Table of Contents

1. [Code of Conduct](#1-code-of-conduct)
2. [Ways to Contribute](#2-ways-to-contribute)
3. [Reporting a Bug](#3-reporting-a-bug)
4. [Submitting a Variant Port](#4-submitting-a-variant-port)
5. [Firmware Contributions](#5-firmware-contributions)
6. [Hardware Contributions](#6-hardware-contributions)
7. [Documentation Contributions](#7-documentation-contributions)
8. [How to Open a Pull Request](#8-how-to-open-a-pull-request)
9. [Release Process](#9-release-process)

---

## 1. Code of Conduct

This project is a technical endeavour in a community that values precision,
craftsmanship, and preserving instruments worth preserving. Discussions should be
technical and respectful. Disagreements about design choices should cite technical
reasoning. Personal attacks, gatekeeping, and condescension have no place here.

---

## 2. Ways to Contribute

### You Have a WM-D6C and the Module Is Installed

The most valuable contribution you can make right now is to document your
installation:

- Photograph each step of your installation in good lighting
- Record the bench measurements from your specific unit (FG swing voltage, Q601
  base voltage, pot wiper ranges, B+3 rail health) in
  `docs/bench-measurements/measurement-log-SNXXXXX.md` using the template
- Note anything that differed from the installation guide
- Open a pull request or just create an issue with the photos and measurements

Real-world installation data from multiple units is more valuable than any amount
of theoretical documentation.

### You Own a Different Sony Professional Walkman

The DSR-1 module's variant system is designed specifically for you. A variant port
requires exactly three files and is described in detail in
[Section 4](#4-submitting-a-variant-port) below.

Compatible machines in principle include:
- Sony WM-D6 (predecessor to WM-D6C, similar servo architecture)
- Sony WM-D3 / WM-D3C
- Sony TC-D5M
- Sony TCD-D3 derivatives with similar capstan servo topology
- Other Sony professional Walkmans from the same era using the CX20084

### You Found a Bug

See [Section 3](#3-reporting-a-bug).

### You Want to Improve the Firmware

See [Section 5](#5-firmware-contributions).

### You Speak Another Language

The installation guide and theory documents translated into any language are
immediately useful. See [Section 7](#7-documentation-contributions).

---

## 3. Reporting a Bug

Use the GitHub issue tracker. Choose the appropriate issue template:

- **Bug report**: Something works incorrectly or unexpectedly
- **Installation problem**: Something in the installation guide is wrong, unclear,
  or missing
- **Variant port**: You want to propose or discuss a port to another machine

When reporting a bug:

**Be specific about the hardware.** Which variant (A or B)? What firmware version
(check the USB CDC `t` command output — it includes the firmware version string)?
What machine serial number suffix (-11, -21, etc.)?

**Include measurements.** If the speed is wrong, what does the test tape show? What
does the USB CDC telemetry report? What is the FG period? What is the error value?
Raw numbers are far more useful than descriptions like "it runs a bit fast."

**Describe what you tried.** Have you adjusted Kp and Ki? What happened? Have you
cleaned the FG disc? Have you verified the belt is correctly installed?

**Include photographs where relevant.** A photograph of the actual wiring at the
IC601 pads or at J1 resolves ambiguity that written descriptions cannot.

---

## 4. Submitting a Variant Port

A variant port makes the DSR-1 module compatible with a different machine. This is
the contribution type most likely to have a large impact — each successful variant
opens the project to an entire community of machine owners.

### What a Variant Port Requires

A complete variant port consists of exactly three artifacts in a new directory under
`docs/variants/[machine-model]/`:

**1. `schematic-diff.md`** — A description of what changes from the reference WM-D6C
schematic. This does not require a new KiCad schematic — a clear written description
with pin numbers and signal names is sufficient. At minimum it must describe:

- The FG signal source (which component, what pin, what voltage swing)
- The motor drive point (which transistor base, what operating voltage range)
- The motor enable signal (if present, which net, what voltage)
- The speed trim potentiometers (which ones, what values, what voltage range)
- The power input (what voltage, what current, what connector)
- Any components that must be removed from the target machine

**2. `harness-pinout.md`** — A table showing how each J1 connector pin maps to a
specific point in the target machine. Must include component reference designators
from the target machine's service manual, the physical location of each connection
point (board name, component position, pad number or test point), and the wire
colour if the standard DSR-1 harness is used.

Example format:

```markdown
| J1 Pin | DSR-1 Net | Target Machine Connection |
|---|---|---|
| 1 | FG_RAW | FG sensor output — IC601 pin 13 pad on main board |
| 2 | Q601_BASE | Q601 base — R601 junction on main board |
| 3 | MOTOR_EN | IC pin X net — measure voltage during playback |
| 4 | RV601_WIPER | RV601 wiper — marked VR1 on adjustment diagram |
| 5 | RV602_WIPER | Speed tune slider wiper |
| 6 | RV603_WIPER | RV603 wiper |
| 7 | VBATT | B+1 power rail — CN301 sleeve terminal |
| 8 | GND | Chassis ground |
```

**3. `config-[machine].h`** — A complete config.h file for the target machine with
all constants calibrated for that machine. The most critical constant is
`FG_TARGET_HZ` — the FG pulse rate at exactly correct tape speed for the target
machine. This must be measured using a calibrated test tape, not estimated.

The template at `docs/variants/template/` shows the complete format for all three
files.

### How to Measure FG_TARGET_HZ

1. Install the DSR-1 module in the target machine with default WM-D6C constants as
   a starting point.
2. Insert a calibrated test tape and play it back.
3. Use the USB CDC `T` command to enable continuous telemetry output.
4. Note the `FG period` value in ticks when the test tape tone reads correct
   frequency on a spectrum analyser.
5. Calculate: `FG_TARGET_HZ = 64000000 / measured_period_ticks`
6. Alternatively, use `tools/calibration/fg-period-calculator.py` with the
   measured frequency as input.

This measurement is unique to each machine model and potentially to each individual
unit. Measurements from multiple units of the same model should be averaged, and
the variance documented in the variant notes.

### Quality Bar for Variant Ports

A variant port is accepted when it meets these criteria:

- Verified on at least one physical unit of the target machine
- FG_TARGET_HZ measured with a calibrated test tape, not estimated
- All three required files are present and complete
- The variant has been tested through at least one full tape side without speed
  anomalies
- The submitter documents the unit(s) used (serial number suffix, approximate year)

A variant port that has not been physically tested is not accepted — it can be
opened as a draft PR or an issue to solicit testing from community members who own
the target machine.

---

## 5. Firmware Contributions

### Architecture Constraints — These Are Non-Negotiable

The firmware architecture is designed around specific principles that are not open
for modification:

**No HAL.** No STM32Cube HAL, no STM32Cube LL, no vendor-supplied middleware of any
kind. Peripheral access is direct register writes only. The headers in use are the
CMSIS device headers (`stm32g0b1xx.h`) which provide register definitions and bit
masks — these are acceptable because they are documentation, not abstraction.

The reason: HAL code is large, opaque, and introduces unpredictable execution time.
The servo ISR must have deterministic timing. HAL callback chains cannot guarantee
this. Register-level code executes in a known, fixed number of cycles.

**No RTOS.** No FreeRTOS, no Zephyr, no ChibiOS. The application does not need
preemptive scheduling — the servo runs in hardware interrupts, the USB runs in the
main loop, and the priority hierarchy is simple and correct. An RTOS would add
flash consumption, RAM consumption, and scheduling overhead for zero benefit.

**No dynamic memory allocation.** No malloc, no new, no C++ containers. All
memory is statically allocated. In an embedded real-time system, dynamic allocation
risks fragmentation and non-deterministic allocation time.

**Minimal flash footprint.** The current firmware occupies approximately 28KB of
the 128KB available. Contributions that grow the firmware significantly must justify
the flash cost.

### What We Do Want

**Bug fixes**: If you find a correctness error in the control algorithm, a
timing race condition, or an incorrect register configuration, please fix it.

**New machine variants**: A new `config_[machine].h` file in `firmware/variants/`
for a ported machine, with the FG target and any machine-specific differences.

**USB CDC improvements**: Additional telemetry fields, new tuning commands, improved
output formatting. These run in the main loop and have no servo timing impact.

**Build system improvements**: Makefile improvements, better linker script
documentation, GitHub Actions improvements. These are always welcome.

**Comments and documentation**: Explaining *why* a register is configured a
particular way, or *what* an algorithm step achieves. Good comments in embedded
firmware are rare and valuable.

### Code Style

- Indentation: 4 spaces, no tabs
- Naming: `snake_case` for variables and functions, `UPPER_CASE` for constants
- Every non-obvious register write gets a comment explaining the purpose
- ISR functions must be as short as possible — no function calls, no loops
- `volatile` on every variable accessed in both ISR and main contexts

---

## 6. Hardware Contributions

### What We Want

**Verified gerber corrections**: If you find an error in the PCB layout — silkscreen
mistake, incorrect pad size, clearance violation — open an issue with a clear
description and, ideally, the corrected KiCad file.

**Variant-specific hardware modifications**: If a variant port requires a hardware
change to the DSR-1 board (a different harness connector, an additional component,
a modified footprint), document it clearly and propose it as a board revision.

**Improved footprints**: Better courtyard definitions, corrected pad sizes based
on actual manufactured boards.

### What We Do Not Accept

**Incompatible architecture changes**: Changes that break the shared PCB / dual
variant design, add significant cost, or increase board size beyond the CP304 cavity
constraint need to be discussed as issues before implementation.

**Unverified layout changes**: Hardware changes that have not been manufactured and
tested on a real board are not accepted. Open an issue instead and describe what you
are proposing.

---

## 7. Documentation Contributions

Documentation contributions are accepted with a lower bar than firmware and hardware
contributions. A document that is 90% correct and clear is vastly more useful than
no document.

### What We Want

**Translations**: The installation guide in any language. Open a PR with the
translated files in `docs/installation/[language]/`. Maintain the same structure and
heading hierarchy as the English original.

**Installation photographs**: High-quality photographs of each installation step on
a real WM-D6C. Name files descriptively: `04-removing-cx20084-desoldered.jpg`.
Images wider than 1920px should be scaled down.

**Troubleshooting additions**: If you encountered a problem during installation that
is not covered in the troubleshooting guides, document it with the symptom, the
diagnosis, and the solution.

**Theory corrections**: If something in the theory documents is technically
incorrect, open an issue or a PR with the correction and the reasoning.

---

## 8. How to Open a Pull Request

1. **Fork the repository** on GitHub.

2. **Create a branch** with a descriptive name:
   ```
   git checkout -b variant/wm-d3-port
   git checkout -b fix/fg-divider-overflow
   git checkout -b docs/installation-photos-sn72795
   ```

3. **Make your changes.** For firmware changes, verify the build:
   ```bash
   cd firmware/build
   make VARIANT=wmd6c
   ```

4. **Write a clear commit message.** One line summary, blank line, then explanation
   of *why* the change was made if it is not obvious:
   ```
   Fix FG period overflow at very low motor speeds

   At motor speeds below approximately 500 RPM, the 32-bit period counter
   can accumulate enough ticks that the period value overflows when
   multiplied by KP_Q16 in the control calculation. Add a maximum period
   guard that holds the output at DAC_MIN when period exceeds 2 × TARGET.
   ```

5. **Open the pull request** against the `main` branch. Fill in the PR template.

6. **Respond to review comments.** Changes may be requested. This is normal and
   not a rejection — it is how the project maintains quality.

---

## 9. Release Process

Releases are tagged on `main` using semantic versioning:

- `v0.x` — pre-release, design and verification phase
- `v1.0` — first stable release, hardware verified on physical unit
- `v1.x` — backwards-compatible improvements
- `v2.0` — significant hardware or firmware architecture change

Each release tag covers hardware, firmware, and documentation simultaneously. The
gerbers in `hardware/gerbers/vX.X/` are the exact files used to manufacture boards
at that version. The binary in `firmware/releases/wmd6c-servo-vX.X.bin` is compiled
from the source at that tag.

The CHANGELOG.md entry for each release is written in plain English — not git log
summaries — describing what changed, why it changed, and whether existing boards
or installations are affected.

---

## Questions?

Open an issue with the label `question`. The Discussions tab is also open for longer
conversations about design direction, variant ports, and anything else that does not
fit neatly into a bug report or pull request.
