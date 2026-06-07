# Firmware Rev A Review Findings

**Project:** DSR-1 / wmd6c-servo
**File:** `docs/review/firmware-rev-a-findings.md`
**Status:** Review notes / pre-Rev A
**Scope:** Source-grounded review of the firmware prototype, build system, linker
script, and the firmware↔schematic interface, with severity-ordered findings and
suggested remediation order.

---

## 1. Purpose

This document records a source-level review of the DSR-1 firmware prototype as
committed. It is written to the same standard as the rest of the repository:
findings are separated into what is *demonstrably wrong in the source*, what is
*unverified and must be measured or checked against the reference manual*, and
what is *cosmetic or documentation-only*.

Nothing here contradicts the project's own honest status ("active design /
firmware prototype, not bench-validated"). The intent is to convert that general
caveat into specific, actionable items before Rev A schematic capture and first
hardware bring-up.

> **Historical review note:** this review was written against the earlier G0B1
> firmware prototype. The current hardware target is STM32G0C1KCU6; use
> `PROJECT_STATUS.md` and `docs/stm32g0c1-pin-allocation.md` for current design
> authority.

References used:

- STM32G0B1 datasheet (`stm32g0b1cc.pdf`, historical prototype reference)
- STMicroelectronics RM0444 (STM32G0x1 reference manual) — for items marked "verify"
- ST community guidance on the two USB controller generations (see §3.2)

---

## 2. Summary

| # | Finding | Severity | Type |
|---|---------|----------|------|
| 1 | Missing startup file + no vendored CMSIS — does not build | High | Build blocker |
| 2 | USB CDC PMA access uses the wrong USB-IP generation for STM32G0 | High | Defect |
| 3 | Pot/speed-trim path is dead code (`adc_get_adjusted_target` never called) | High | Defect |
| 4 | Settings flash address vs linker reservation off by one page | Medium | Latent defect |
| 5 | Motor-drive sign convention is internally contradictory + unverified | Medium | Defect / open |
| 6 | `servo_set_target_period()` / `servo_reset_integral()` never called | Low | Dead API |
| 7 | Alternate-function numbers for PA0/PA6 need verification | Low | Verify |
| 8 | ADC calibration runs before ADC clock is selected | Low | Verify |
| 9 | Makefile / license-filename inconsistencies | Low | Cleanup |
| 10 | Telemetry-struct atomicity claim is imprecise | Low | Doc/cosmetic |
| 11 | Docs lag schematic (PD controller present; signal sheet empty) | Info | Steering |

Two findings have dedicated companion documents:

- USB PMA: `docs/hardware/usb-pma-stm32g0-port.md`
- Motor sign: `docs/hardware/motor-drive-sign-convention.md`

---

## 3. High-severity findings

### 3.1 The firmware does not build as committed

At the time of this review, `firmware/build/Makefile` set `STARTUP := $(SRC_DIR)/startup_stm32g0b1xx.s` and the
legacy linker script `STM32G0B1KBUx_FLASH.ld` depended on the symbols that file provides
(`Reset_Handler`, the `.isr_vector` table, and the `.data`/`.bss` init that runs
before `main()`). That file is not present in the repository. Unlike the CMSIS
path, there is no graceful error — the build fails at the assembler step with a
missing-file error.

Separately, no translation unit includes a CMSIS device header. Every reference to
`RCC`, `GPIOA`, `TIM2`, `TIM3`, `FLASH`, `ADC1`, `DMA1_Channel1`, `DMAMUX1_Channel0`,
`NVIC_*`, `__IO`, `__WFI`, and `__enable_irq` is therefore undefined at compile
time. `firmware/include/` contains only a `.gitkeep`, and the Makefile only
*searches* for CMSIS in three external locations.

Effect: a fresh clone cannot compile or link.

Remediation (pick one and document it):

- Vendor `startup_stm32g0b1xx.s`, `system_stm32g0xx` (or a minimal replacement),
  and the STM32G0 CMSIS device + core headers under `firmware/cmsis/` (the
  Makefile already probes `$(ROOT_DIR)/cmsis`), **or**
- Add a CMSIS submodule and pin the version, **or**
- Keep them external but add a hard `$(error ...)` for the startup file mirroring
  the existing CMSIS warning, and document the exact STM32CubeG0 version in the
  firmware README.

Until this is resolved, none of the other findings are testable.

### 3.2 USB CDC PMA access targets the first-generation USB IP

`firmware/src/usb_cdc.c` models the packet memory area (PMA) the way the *first*
generation of the ST USB device peripheral works (STM32F1/L0/G4/WB): 16-bit PMA
words spread onto 32-bit address boundaries. This shows up in two places:

- `pma_write16()` / `pma_read16()` compute `USB_PMA_BASE + pma_off * 2` — the ×2
  stride is the doubled-access scheme.
- `PMA_BufDesc` inserts a `uint16_t _pad` after every real 16-bit field.

The STM32G0 uses the **second-generation** USB device IP, in which the PMA and
buffer descriptor table are accessed **linearly (1:1)** with 16-bit-wide entries
and no gap words. The ×2 stride and the descriptor padding are therefore incorrect
for this MCU, and enumeration will not work as written.

This is a confirmed defect, not a "verify" — it is documented behavior of the two
IP generations. Full details and a corrected linear-access model are in
`docs/hardware/usb-pma-stm32g0-port.md`. Note that the endpoint/channel register
layout and naming also differ between the two generations and must be checked
against RM0444 at the same time.

### 3.3 The speed-trim path is dead code

`adc.c` fully implements `adc_get_adjusted_target()`, and its header and comments
state it is "called from `TIM2_IRQHandler` (servo ISR) on each FG edge." It has no
caller anywhere in the firmware. The servo ISR in `servo.c` uses `g_target_period`
directly:

```c
uint32_t target = g_target_period;   /* RV601/RV602/RV603/S601 never consulted */
```

`servo.c` does not even include `adc.h`. As committed, the three speed-control pots
and the S601 Speed Tune switch have **no effect** on tape speed. The README's
caveat ("the adjusted-target path must be verified end-to-end") understates this:
the path is not wired at all, not merely unverified.

Remediation: have the ISR derive its working target from
`adc_get_adjusted_target()` instead of reading `g_target_period` raw. Be deliberate
about timing and integral interaction:

- `adc_get_adjusted_target()` reads three volatile DMA values and does several
  multiplies/divides. Confirm the added ISR cost still fits comfortably inside the
  FG period budget at maximum plausible FG rate (the divides are by constants and
  should be cheap, but measure on `make disasm`).
- The adjusted target now moves with the pots, so a large pot change produces a
  step in `error`. The existing anti-windup clamp covers this, but consider whether
  `servo_reset_integral()` should fire on large target steps (see finding 6).

---

## 4. Medium-severity findings

### 4.1 Settings flash address and linker reservation are off by one page

The linker reserves only the **last** 2 KB page:

```
FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 126K   /* usable ends at 0x0801F800 */
```

That leaves the usable code region as `0x08000000–0x0801F7FF` (pages 0–62
inclusive) and reserves page 63 (`0x0801F800–0x0801FFFF`).

But `config.h` / `flash.c` place the settings block at page **62**:

```c
#define FLASH_SETTINGS_ADDR 0x0801F000UL          /* page 62, inside code region */
#define SETTINGS_PAGE ((0x0801F000 - 0x08000000) / 2048)   /* = 62 */
```

So the page that gets erased by `flash_save()` (62) is *inside* the region the
linker is free to fill with `.text`/`.rodata`. The protection the linker comment
claims ("the compiler can never place code there … a `flash_save()` erase would
destroy that code") is not actually in force. It is harmless at the current ~28 KB
image because page 62 happens to be empty, but it is a latent corruption path.

Remediation (either is correct; the first is least disruptive):

- Move settings to the true last page: `#define FLASH_SETTINGS_ADDR 0x0801F800UL`
  (page 63), which matches the existing 126 KB reservation, **or**
- Reserve two pages in the linker: `LENGTH = 124K`, keeping settings at page 62.

After the change, re-check the `make` flash-usage print, which currently computes
against `126976` (4 KB reserved) while the linker reserves 2 KB — pick one number.

### 4.2 Motor-drive sign convention is contradictory and unverified

Three sources disagree on the PWM→speed polarity:

- `config.h`: `DAC_MIN` is annotated "Near-full motor drive", `DAC_MAX` "Motor
  nearly off" — i.e. **low PWM = fast**.
- `servo.c` block comment: "Higher PWM duty → Q_LS conducts more → Q601 base
  pulled lower → faster" — i.e. **high PWM = fast**.
- The control law subtracts both P and I terms from `DAC_CENTER` on positive
  (too-slow) error.

The control-law sign is correct *only* if `config.h` is right (low PWM = fast). If
the `servo.c` physical narrative is right (high PWM = fast), the loop is positive
feedback and will drive PWM to a rail on the first FG edge.

This must be resolved by bench measurement of Q601 base voltage versus motor speed
before any motor is driven, and the three descriptions made consistent. Procedure
and the exact code change for each outcome are in
`docs/hardware/motor-drive-sign-convention.md`.

---

## 5. Low-severity findings

### 5.1 Unused servo API

`servo_set_target_period()` and `servo_reset_integral()` have no callers. The
`f+`/`f-` commands modify `g_target_period` directly in `usb_cdc.c`, and there is
no play/stop transition that would call `servo_reset_integral()`. Consequence: the
integral is never explicitly zeroed at capstan start, so initial-lock overshoot is
bounded only by `INTEGRAL_LIMIT`. Either route the `f±` commands through
`servo_set_target_period()` for a single source of truth, or remove the unused
function; and decide whether a start-of-play integral reset is wanted once
play/stop is detectable.

### 5.2 Alternate-function numbers for PA0 / PA6

`servo_init()` programs AF1 for both PA0 (TIM2_CH1) and PA6 (TIM3_CH1). On
STM32G0, TIM3_CH1 on PA6 is AF1, but TIM2_CH1 on PA0 is **AF2** — so PA0 is most
likely wrong. Confirm both against the alternate-function table in the datasheet
(`stm32g0b1cc.pdf`) you already have, and fix the `AFR[0]` nibble for PA0. The pin
allocation doc already flags these as "verify AF".

### 5.3 ADC calibration ordering

`adc_init()` performs self-calibration (`ADCAL`) *before* it sets `CKMODE` to
PCLK/4. Per RM0444 the ADC clock should be selected before calibration; as written,
calibration runs on the reset-default async ADC clock. Reorder so `CKMODE` is set
first, then enable `ADVREGEN`, wait, then calibrate. Verify against RM0444's ADC
start-up sequence.

### 5.4 Build and license cleanup

- The Makefile defines `-DUSE_FULL_LL_DRIVER`, but the firmware is deliberately
  register-level (no LL/HAL). Remove it to avoid implying a dependency.
- "Optimisation: -O2 as specified in datasheet §6.4" — a datasheet does not
  specify compiler optimization; drop or correct the citation.
- License filename: `firmware/src/README.md` points to `LICENSE_FW.txt`, the root
  README maps firmware to `LICENSE`, and the repo ships `LICENSE_FIRMWARE.txt`.
  Pick one canonical filename and update both READMEs.

### 5.5 Telemetry atomicity claim

`servo.h` states telemetry reads are atomic on Cortex-M0+. Each 32-bit field read
is atomic, but the multi-field `g_telemetry` struct read in the main loop can tear
if `TIM2_IRQHandler` fires mid-read (some fields old, some new). This is harmless
for human-readable telemetry, but the comment overstates the guarantee. Either
relax the wording, or double-buffer/seqlock the snapshot if a coherent frame is
ever required.

---

## 6. Informational — docs lag the schematic

The README lists the USB-C PD architecture as "open (native UCPD vs external
controller)", but the `Power input zone` schematic sheet already instantiates an
external CC/PD controller (IP2721), an MT3608 boost, and a USBLC6 ESD device — so
the hardware is already exploring the external-controller answer. Meanwhile the
`Signal conditioning` sheet (FG input conditioning + the PWM/RC/level-shift motor
output the firmware depends on) is still an empty stub, even though the firmware
has committed to its nodes (PA0 capture, PA6 PWM, Q_LS MMBT3904, R7/C8/R9).

Not a defect — the schematics are in development — but the signal-conditioning
sheet is the natural next thing to populate, because the firmware is already ahead
of it and finding 4.2 (motor sign) cannot be closed without it.

---

## 7. Suggested remediation order

1. Make it build — vendor or pin the startup file and CMSIS (3.1). Nothing is
   testable until this is done.
2. Fix the USB PMA model for the G0 IP (3.2) and wire the trim path (3.3); both are
   silently broken paths that would otherwise surface only on the bench.
3. Resolve the motor sign (4.2) before driving a real motor — this is the one that
   can run away.
4. Correct the settings-page/linker mismatch (4.1) and the PA0 AF (5.2) before Rev
   A schematic capture, so the pin/memory map is honest.
5. Everything else is cleanup and can follow.

---

## 8. What this review did not cover

- USB enumeration correctness beyond the PMA model (descriptors, endpoint setup,
  control-transfer state machine) — deferred until the PMA port lands.
- Servo loop tuning (Kp/Ki values) — these are explicitly simulation-derived
  placeholders to be bench-tuned.
- Absolute speed accuracy — fully covered by `docs/hardware/timebase-decision.md`;
  the current HSI16-only timebase is acknowledged as development-grade.
- Any hardware not yet captured in the KiCad sheets.
