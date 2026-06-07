# DSR-1 Firmware

Bare-metal C firmware for the WM-D6C DSR-1 Servo Control Board. The project
hardware target is STM32G0C1KCU6 at 64 MHz; this firmware tree still carries some
legacy STM32G0B1 build filenames until the firmware migration is completed. No HAL,
no RTOS, no dynamic memory allocation.

---

## Source Files

| File | Responsibility |
|------|---------------|
| `config.h` | All tunable constants — edit here and rebuild |
| `servo.c` / `servo.h` | TIM2 input capture ISR, PI control law, TIM3 PWM output |
| `adc.c` / `adc.h` | ADC DMA scan for RV601/RV602/RV603 pot wipers |
| `flash.c` / `flash.h` | Settings save/restore with CRC-32 validation |
| `usb_cdc.c` / `usb_cdc.h` | USB CDC virtual COM port, command parser, telemetry |
| `main.c` | Clock init (HSI16 → 64 MHz PLL), peripheral sequencing, main loop |

Build files live in `firmware/build/`:

| File | Responsibility |
|------|---------------|
| `Makefile` | `arm-none-eabi-gcc` build, flash/DFU targets |
| `STM32G0B1KBUx_FLASH.ld` | Legacy prototype linker script — to be replaced for STM32G0C1KCU6 |
| `startup_stm32g0b1xx.s` | Legacy prototype CMSIS startup — to be replaced for STM32G0C1KCU6 |

---

## Prerequisites

**Toolchain:** `arm-none-eabi-gcc` version 10.0 or newer.

```bash
# macOS
brew install --cask gcc-arm-embedded

# Ubuntu / Debian
sudo apt install gcc-arm-none-eabi

# Windows
# Download from https://developer.arm.com/downloads/-/gnu-rm
```

**CMSIS headers:** The STM32G0 CMSIS device headers (`stm32g0b1xx.h` etc.) must be on the include path. The Makefile searches three standard locations automatically. If not found, either:

- Set `CMSIS_DIR` on the command line (see below), or
- Copy the headers into `firmware/cmsis/` and the Makefile will find them

The headers ship with the [STM32CubeG0 package](https://www.st.com/en/embedded-software/stm32cubeg0.html) under `Drivers/CMSIS/`.

---

## Building

```bash
cd firmware/build

# Standard build — produces firmware.elf, firmware.bin, firmware.hex
make

# With explicit CMSIS path
make CMSIS_DIR=/path/to/STM32Cube_FW_G0_V1.6.1/Drivers/CMSIS

# Flash via SWD (requires openocd)
make flash

# Flash via USB DFU (device must be in DFU mode — hold TP1/BOOT0 while connecting USB)
make dfu

# Print section sizes
make size

# Generate annotated disassembly
make disasm

# Clean all build artefacts
make clean
```

The build prints a flash usage summary on completion. At the current firmware size of ~28 KB, there is approximately 98 KB of headroom against the 126 KB firmware budget (the last 2 KB page is reserved for the settings sector).

---

## Configuration

**`config.h` is the only file you should need to edit for normal configuration.** All machine-specific values, gain constants, and hardware limits are defined there. Never scatter tunable values through source files.

### Critical: TARGET_PERIOD_DEFAULT

This constant must be measured on the bench — the placeholder value will not be correct for every unit.

1. Insert test tape WS-48B, select Play mode
2. Connect oscilloscope to LINE OUT
3. Confirm 3 kHz tone = correct speed
4. Measure FG901 pulse frequency at that speed
5. Set `TARGET_PERIOD_DEFAULT = 64000000 / measured_Hz`
6. Rebuild and flash

Alternatively, use the `f+` / `f-` USB CDC commands to tune in real time without rebuilding, then `s` to save.

### Motor Output Stage

DSR-1 uses the **PWM + RC filter + NPN level-shift** output stage exclusively.
On the primary WM-D6C serial 72795 unit, Q601 is on the surface-mount
`C11-494-12` board and its exact package/marking plus base operating range remain
pending bench verification. Direct DAC drive is not used.

TIM3 channel 1 on PA6 produces the PWM signal. There is no `SERVO_OPTION_B`
define — the firmware is committed to this single output topology.

---

## USB CDC Live Tuning

The module enumerates as a virtual COM port on any computer without driver installation. Connect with any serial terminal at any baud rate (the baud rate setting is ignored — it is a USB CDC device).

| Command | Effect |
|---------|--------|
| `p+` / `p-` | Adjust Kp ±0.01 |
| `i+` / `i-` | Adjust Ki ±0.001 |
| `f+` / `f-` | Adjust target period ±1 tick |
| `t` | Print telemetry snapshot |
| `T` | Toggle continuous telemetry |
| `s` | Save current constants to flash |
| `r` | Restore compiled-in defaults (SRAM only — does not write flash) |
| `?` | Print command reference |

On first connection the firmware sends a banner confirming whether saved flash settings were loaded or defaults are active. If the banner shows a corruption warning, calibrate the unit and send `s` before use.

### Telemetry format

```
FG:25612 Hz:2499 Tgt:25600 Err:+12 Int:-847 PWM:2034 Kp:9830(0.150) Ki:524(0.008) RV1:2048 RV2:1923 RV3:2100
```

---

## Firmware Update (DFU)

The intended STM32G0C1KCU6 target contains a factory-programmed USB DFU bootloader.
To enter DFU mode:

1. Hold BOOT0 test point (TP1) to VDD
2. Connect USB-C cable
3. Release TP1

The device enumerates as a DFU device. Flash with:

```bash
# Using dfu-util (macOS/Linux)
dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D firmware.bin

# Using STM32CubeProgrammer (Windows/macOS/Linux GUI)
# Select the .bin file, click Download
```

Pre-built `.bin` files for each release are in `firmware/releases/`.

> **Settings are preserved across DFU updates.** The settings sector (last 2 KB page at `0x0801F800`) is not erased by the update process.

---

## Architecture Notes

**Interrupt priorities** (lower number = higher priority on Cortex-M0+):

| Priority | Interrupt | Constraint |
|----------|-----------|------------|
| 0 | TIM2 (servo ISR) | Must preempt everything — non-negotiable |
| 1 | USB | Must not delay servo |

**Flash writes during playback:** The `s` command calls `servo_freeze()` before erasing flash and `servo_unfreeze()` after. The motor runs at constant open-loop drive for the ~2 ms erase window; the flywheel time constant (~200 ms) means speed does not measurably change. The servo re-locks within 1–2 FG periods after the write completes.

**No heap:** All memory is statically allocated. `_Min_Heap_Size = 0` in the linker script. `malloc` is not available and must not be added.

**Contributor rule:** All firmware contributions must use register-level C only. The STM32G0x1 Reference Manual (RM0444) and the selected STM32G0C1KCU6 datasheet are the authoritative sources — not the HAL API. See `docs/theory/why-bare-metal.md` for the full rationale.

---

## Licence

Firmware: MIT — see `LICENSE_FIRMWARE.txt`
