# DSR-1 Renode Simulation

Renode-based simulation for the DSR-1 STM32G0B1KBU6 firmware.
Run from the **workspace root** (`d:/Projects/wmd6c-servo`).

## Prerequisites

**1. Install Renode** from https://renode.io. Packages are available for all
major platforms:

| Platform | Package |
|---|---|
| Windows | `.msi` installer (winget: `Antmicro.Renode`) |
| macOS | `.pkg` installer |
| Ubuntu / Debian | `.deb` — `sudo dpkg -i renode_*.deb` |
| Fedora / RHEL | `.rpm` — `sudo dnf install renode_*.rpm` |
| Any Linux | AppImage — `chmod +x renode_*.linux-portable` |

**2. Install renode-test Python dependencies.**

*Windows* — the default install path contains a space and must be quoted:
```
pip install -r "C:\Program Files\Renode\tests\requirements.txt"
```

*macOS / Debian / Ubuntu* — Renode installs to a path without spaces:
```
pip3 install -r /opt/renode/tests/requirements.txt
# macOS: /Applications/Renode.app/Contents/MacOS/tests/requirements.txt
```

*Fedora / RHEL* — same as Debian/Ubuntu but confirm the install prefix first:
```
renode --version          # shows install path
pip3 install -r /opt/renode/tests/requirements.txt
```

**3. Verify tools are on PATH** by opening a new terminal and running:
```
renode --version
renode-test --help
```

### VS Code terminal note (Windows only)

On Windows, VS Code's integrated terminal inherits the PATH from when VS Code
was launched, so tools installed afterward won't be found until VS Code is
restarted. The workspace `.vscode/settings.json` works around this for Windows
by appending the tool paths to every new VS Code terminal — open a **new
terminal** (`Ctrl+Shift+`\``) rather than reusing an existing one.

On Linux and macOS the system PATH is read fresh by each terminal and this is
not an issue.

## Quick start — interactive session

```
renode firmware/simulation/scripts/dsr1.resc
```

The Renode monitor (console) opens. The firmware is loaded and halted.

> **Do not call `start`.** The FG macros use `emulation RunFor` internally to
> advance simulation time in controlled steps. Calling `start` puts the emulation
> into free-run mode, which conflicts with `RunFor` inside a macro.

Load the FG helper and inject pulses — the macros start the CPU themselves:

```
(monitor) i @firmware/simulation/scripts/fg-gen.resc
(monitor) runMacro $fg_lock       # 10 nominal-speed pulses
(monitor) sysbus ReadDoubleWord 0x40000434   # read TIM3->CCR1 (motor PWM)
```

Each `runMacro $fg_lock` call steps the firmware through 10 FG edges, running
`clock_init` → `flash_load` → `adc_init` → `servo_init` → servo ISR on the way.
Expected TIM3->CCR1 ≈ 2048 (DAC_CENTER) when FG rate matches target exactly.

## Automated tests (Robot Framework)

```
renode-test firmware/simulation/tests/servo_loop.robot
```

Five tests:
1. **Boot completes** — firmware boots without hanging in any polling loop
2. **Nominal speed** — TIM3->CCR1 stays near DAC_CENTER at target FG rate
3. **Too slow** — motor drive increases (PWM decreases) when FG period is 110%
4. **Too fast** — motor drive decreases (PWM increases) when FG period is 90%
5. **Clamp check** — output stays within DAC_MIN/DAC_MAX at 200% slow
6. **Filter check** — sub-threshold periods (<6400 ticks) are discarded

## VS Code debugging

1. Start the Renode simulation (`dsr1.resc`), which opens GDB server on port 3333.
2. In VS Code: **Run → Start Debugging** → select **DSR-1: Debug via Renode**.
3. Set breakpoints in `servo.c`, `adc.c`, etc.
4. Use the Renode monitor alongside to inject FG pulses.

## FG injection reference

The servo ISR (`TIM2_IRQHandler`) reads `TIM2->CCR1` on every FG rising edge
and computes `period = CCR1 - last_capture`. To simulate one FG pulse from the
Renode monitor:

```
# Write a captured timestamp into CCR1 and fire the TIM2 IRQ
sysbus WriteDoubleWord 0x40000034 <CCR1_VALUE>   # TIM2->CCR1
sysbus WriteDoubleWord 0x40000010 0x00000002      # TIM2->SR: set CC1IF
sysbus WriteDoubleWord 0xE000E200 0x00008000      # NVIC_ISPR0: set IRQ 15 pending
```

Key values (see `firmware/src/config.h`):

| Constant              | Value  | Meaning                            |
|-----------------------|--------|------------------------------------|
| TARGET_PERIOD_DEFAULT | 25600  | 2500 Hz at 64 MHz — nominal speed  |
| MIN_PERIOD_TICKS      | 6400   | periods shorter than this filtered |
| DAC_CENTER            | 2048   | TIM3 PWM at quiescent drive        |
| DAC_MIN               | 100    | maximum motor drive clamp          |
| DAC_MAX               | 3995   | minimum motor drive clamp          |

To read the current motor PWM output:
```
sysbus ReadDoubleWord 0x40000434    # TIM3->CCR1
```

## Pot trim simulation

> **Note:** `SetVoltage` is not available on all Renode versions. Check with
> `help sysbus.adc` in the monitor. If present, set voltages with:
> ```
> sysbus.adc SetVoltage 1 <voltage>   # RV601 — base speed trim (PA1/ADC_IN1)
> sysbus.adc SetVoltage 2 <voltage>   # RV602 — Speed Tune slider (PA2/ADC_IN2)
> sysbus.adc SetVoltage 3 <voltage>   # RV603 — Speed Tune range (PA3/ADC_IN3)
> ```
> Mid-scale (1.65 V) = no offset. Full deflection (3.3 V or 0 V) = ±1% (RV601)
> or ±3% (RV602 at full range) speed trim.

This is moot until finding §3.3 is resolved — `adc_get_adjusted_target()` has no
caller, so ADC values have no effect on the servo loop in the current firmware.

To simulate the S601 Speed Tune enable switch being pressed (PA7 HIGH):
```
sysbus.gpioPortA SetData 0x0080 0x0080    # set PA7 high
```

## Known simulation limits

| Item | Status | Notes |
|------|--------|-------|
| Clock init (clock_init) | Works | RCC Python model handles HSI/PLL/SW polling loops |
| Flash settings load | Works | Returns FLASH_ERR_INVALID (memory = 0x00), uses defaults |
| ADC calibration loop | Likely works | Depends on STM32G0_ADC Renode model clearing ADCAL |
| ADC ready loop | Likely works | Depends on STM32G0_ADC model setting ADRDY |
| TIM2 input capture (FG) | Manual inject | GPIO-to-timer capture path not yet verified in Renode; use fg-gen.resc |
| TIM3 PWM output | Observable | Read TIM3->CCR1 to see motor drive value |
| DMA ADC transfer | Partial | DMAMUX1 is tagged; DMA→ADC circular transfer depends on STM32G0DMA model |
| USB CDC | Stubbed | usb_cdc.c replaced with no-op stubs pending G0 USB_DRD port (finding 3.2) |
| Flash erase/write | Not tested | flash_save only called from USB CDC command; USB is stubbed |

## Platform file

`platform/stm32g0b1kbu6.repl` is derived from Renode's upstream
`platforms/cpus/stm32g0.repl` with three corrections for the G0B1 die:

- **RAM 144 KB** (upstream has 48 KB for smaller G0 variants)
- **Cortex-M0+** CPU type (upstream says cortex-m0)
- **64 MHz systick** (matching DSR-1 clock_init PLL target; upstream uses 72 MHz)

Plus a `DMAMUX1` tag at 0x40020800 to suppress access warnings from `adc_init`.
