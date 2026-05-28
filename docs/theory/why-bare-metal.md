# Why Bare-Metal C

## Purpose of This Document

The DSR-1 firmware is written in bare-metal C — no hardware abstraction layer,
no real-time operating system, no vendor-supplied middleware. Every peripheral is
configured by writing directly to hardware registers. This is a deliberate
architectural choice, not a limitation or a shortcut. This document explains exactly
why these choices were made, what would be different if they had not been made, and
why the same principles should govern any firmware contributions or ports.

---

## 1. What "Bare-Metal" Means Precisely

### 1.1 What Is Excluded

**No STM32Cube HAL**: STMicroelectronics provides the STM32Cube Hardware Abstraction
Layer — a large library that wraps every peripheral in a layer of callback-driven
functions. `HAL_TIM_IC_Start_IT()`, `HAL_DAC_SetValue()`, `HAL_ADC_Start_DMA()`.
These functions are not used in the DSR-1 firmware. They do not appear in the source.
Their header files are not included.

**No STM32Cube LL (Low Layer)**: ST also provides a thinner abstraction called LL,
which generates inline functions for register access. `LL_TIM_EnableCounter()`,
`LL_DAC_ConvertData12RightAligned()`. These are also not used. They are wrappers
around the same register writes the DSR-1 firmware does directly.

**No RTOS**: No FreeRTOS, Zephyr, ChibiOS, or any other real-time operating system.
There is no task scheduler, no message queue, no mutex, no semaphore. The processor
runs one thing at a time — either the main loop or an interrupt service routine.

**No dynamic memory allocation**: No `malloc`, no `calloc`, no C++ `new`. There is
no heap. All memory is statically allocated — every variable is declared at fixed
addresses that the linker determines at compile time. The memory map is known before
the firmware starts.

**No generated code**: The STM32CubeMX graphical configurator can generate
initialisation code that configures peripherals based on a GUI. This generated code
is not used. Every peripheral initialisation in the DSR-1 firmware was written by
a person who understood what register they were writing and why.

### 1.2 What Is Included

**CMSIS device headers**: The file `stm32g0b1xx.h` from ARM/ST's CMSIS (Cortex
Microcontroller Software Interface Standard) is used. This file provides named
constants for every register and bit field in the STM32G0B1: `TIM2->CCR1`,
`RCC_CR_PLLON`, `DAC_DHR12R1_DACC1DHR`. These are documentation — they let the
code say `TIM2->CR1 |= TIM_CR1_CEN` instead of `*((volatile uint32_t*)0x40000000)
|= (1 << 0)`. The registers and addresses are identical; the names make the code
readable. Using CMSIS headers is not an abstraction — it is notation.

**Standard C library for non-critical functions**: `string.h` for `memcpy` and
`memset`. Nothing that touches hardware.

**The bare-metal startup file**: A small assembly file that initialises the stack
pointer, copies initialised data from flash to SRAM, zeroes the BSS section, and
calls `main()`. This is typically fewer than 50 lines.

---

## 2. The Servo Loop Requires Deterministic Timing

### 2.1 What "Deterministic" Means

A deterministic system is one where, given the same inputs, you can predict exactly
what the system will do and when it will do it. Specifically, for the DSR-1servo
loop, "deterministic" means: when the TIM2 capture interrupt fires, the ISR will
execute within a bounded, known time — always.

The servo ISR runs every time the FG sensor produces a rising edge, approximately
2500 times per second. Each execution:
1. Reads TIM2_CCR1 (the captured timestamp)
2. Computes the FG period (current capture minus previous capture)
3. Computes the error (period minus target)
4. Updates the integral accumulator
5. Computes the new DAC value
6. Writes DAC1_DHR12R1
7. Clears the TIM2 interrupt flag

This sequence takes approximately 20-30 clock cycles. At 64 MHz, that is 310-470
nanoseconds. Every single execution takes between 310 and 470 nanoseconds. This
is not an approximation — it is a fact derivable from the Cortex-M0+ instruction
timing tables and the operations performed. The execution time does not vary with
system load, does not depend on what was happening in the main loop, and is not
affected by any other interrupt as long as the TIM2 ISR has the highest priority.

This determinism is what makes the servo loop work correctly. The DAC is updated
within 470ns of every FG edge. The motor sees a corrected drive voltage within 470ns
of every speed measurement. At a motor mechanical time constant of tens of
milliseconds, this is effectively instantaneous response.

### 2.2 What HAL Would Add to This

If the servo ISR used HAL functions instead of direct register writes:

```c
// HAL version (not what the firmware uses):
HAL_TIM_IC_GetCapturedValue(&htim2, TIM_CHANNEL_1);
HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, output);
```

Each HAL function includes:
- A null pointer check on the handle parameter
- A state machine check (is the peripheral in the right state?)
- A mode validity check (is 12-bit right-aligned the valid mode for this config?)
- The actual register write
- A state update

The HAL function for `HAL_DAC_SetValue` is approximately 20 lines of C in the
HAL source. It compiles to approximately 15-20 additional instructions beyond the
single-instruction register write. At 64 MHz, that is 230-310 nanoseconds of
overhead per HAL call. The ISR has two such calls (capture read + DAC write). The
overhead totals 460-620 nanoseconds — comparable to the entire execution time of the
bare-metal ISR.

More critically, the HAL function execution time is **not constant** — it depends
on whether the conditional checks branch or not, which depends on input values.
The bare-metal ISR has a predictable, bounded execution time. The HAL version does
not. For a servo control loop where timing consistency is the fundamental
requirement, non-constant execution time is not acceptable.

### 2.3 What RTOS Would Add

An RTOS manages the CPU's time through a scheduler that periodically switches
between tasks. The scheduler runs on a timer interrupt — typically SysTick at 1kHz
(every 1ms). Each scheduler tick:
- Saves the current task's register state to its stack
- Determines which task runs next (priority-based or round-robin)
- Restores that task's register state
- Returns to the selected task

This context switch takes approximately 100-300 processor cycles depending on the
RTOS and the number of tasks. More importantly, a context switch can preempt ANY
point in the main loop or a lower-priority task.

For the servo ISR, a timer interrupt-driven RTOS scheduler is not a problem —
the ISR preempts everything, including the scheduler, if it is given higher priority.
But the concern is more subtle: if the servo loop's primary logic were implemented
as an RTOS task rather than an ISR, the task's execution timing would be controlled
by the scheduler. A 2500 Hz servo loop as an RTOS task means the task must be
scheduled and execute 2500 times per second. With a 1kHz SysTick, the maximum RTOS
task rate is 1000 Hz — the task cannot run at 2500 Hz.

The only way to run the servo at 2500 Hz in an RTOS environment is as an interrupt
service routine, which is exactly what the bare-metal implementation does — without
the additional 10KB+ of RTOS flash overhead, without the stack overhead of multiple
task stacks, and without the intellectual overhead of reasoning about task
interactions and priority inversion.

---

## 3. Flash Footprint and Resource Honesty

### 3.1 Current Footprint

The bare-metal DSR-1 firmware occupies approximately 28KB of the STM32G0B1KBU6's
128KB flash:

| Component | Flash size |
|---|---|
| Startup code (assembly) | ~0.5KB |
| Clock and peripheral initialisation | ~2KB |
| Servo algorithm (servo.c) | ~3KB |
| ADC DMA scan | ~1KB |
| USB CDC stack | ~15KB |
| USB PD (UCPD monitoring) | ~2KB |
| Flash storage for settings | ~1KB |
| Interrupt handlers and glue | ~1KB |
| String constants and USB descriptors | ~2KB |
| **Total** | **~27.5KB** |

128KB total flash, 28KB used, **100KB remaining** (78% free).

### 3.2 What STM32Cube HAL Would Cost

The STM32Cube HAL for the STM32G0 family is a large library. Enabling the HAL
modules needed for this application (TIM, DAC, ADC, USB, DMA, GPIO, RCC) adds
approximately:

| HAL module | Approximate flash cost |
|---|---|
| HAL Core + RCC | ~8KB |
| HAL TIM | ~12KB |
| HAL DAC | ~3KB |
| HAL ADC | ~8KB |
| HAL DMA | ~4KB |
| HAL GPIO | ~2KB |
| USB Device library | ~25KB |
| USB CDC class | ~6KB |
| **HAL total** | **~68KB** |

A HAL-based implementation of the same firmware would occupy approximately 68KB +
28KB (application code) = ~96KB — close to the device's entire flash capacity.
There would be approximately 32KB remaining.

For the initial v1.0 release this is borderline acceptable. But the DSR-1 firmware
has a roadmap: future additions (USB PD firmware negotiation, extended telemetry,
variant-specific configuration, power management features) each add flash. A HAL-
based firmware would exhaust flash within one or two major feature additions. A bare-
metal firmware has 100KB of headroom for future growth.

This is not an academic concern. STM32 flash is not expandable. When a HAL-based
project fills its flash, the only options are to strip features or to switch to a
larger device — which means a new PCB revision, new assembly, and new testing.

### 3.3 RAM Footprint

The bare-metal DSR-1 firmware uses approximately 2KB of the STM32G0B1KBU6's 144KB
SRAM:

| Variable | Size |
|---|---|
| Servo state (period, integral, DAC) | 24 bytes |
| ADC DMA buffer (3 × 16-bit) | 6 bytes |
| USB CDC receive/transmit buffers | 512 bytes |
| USB packet memory (in peripheral) | 256 bytes |
| Settings block copy | 32 bytes |
| Stack (including ISR stack) | ~1KB |
| **Total** | **~1.8KB** |

With FreeRTOS and three tasks (servo, USB, ADC), the RAM consumption would include:
- RTOS kernel: ~4KB
- Three task stacks at 256 bytes each: 768 bytes
- Task control blocks: ~200 bytes
- Message queues: ~200 bytes
- **RTOS overhead: ~5KB**

The total RTOS-based implementation would use approximately 7KB — four times the
bare-metal footprint. On a 144KB device this is not a resource crisis, but it
represents unnecessary complexity for an application that does not require preemptive
scheduling.

---

## 4. Transparency and Maintainability

### 4.1 What the Code Says vs What the Hardware Does

In bare-metal code, the relationship between source code and hardware behaviour is
direct and visible. Consider this TIM2 configuration:

```c
// Enable TIM2 clock
RCC->APBENR1 |= RCC_APBENR1_TIM2EN;

// Configure TIM2: free-running counter, no prescaler (64 MHz), auto-reload max
TIM2->PSC = 0;           // No prescaler: each tick = 15.625ns at 64MHz
TIM2->ARR = 0xFFFFFFFF;  // Maximum count — 32-bit free-running

// Configure CH1 as input capture on TI1 (PA0), rising edge, no filter
TIM2->CCMR1 = (1 << TIM_CCMR1_CC1S_Pos);  // CC1S = 01: channel 1 = TI1 input
TIM2->CCER  = TIM_CCER_CC1E;              // Enable capture, rising edge

// Enable capture interrupt
TIM2->DIER = TIM_DIER_CC1IE;

// Start counter
TIM2->CR1 = TIM_CR1_CEN;
```

Each line corresponds to a specific register write. The comment explains the
purpose. A reader with a copy of the STM32G0B1 reference manual (RM0444) can verify
every line against the register descriptions. There are no hidden operations, no
callback chains, no configuration state machines.

The equivalent STM32Cube HAL configuration:

```c
TIM_HandleTypeDef htim2 = {0};
TIM_IC_InitTypeDef sConfigIC = {0};

htim2.Instance = TIM2;
htim2.Init.Prescaler = 0;
htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
htim2.Init.Period = 0xFFFFFFFF;
htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
HAL_TIM_IC_Init(&htim2);

sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
sConfigIC.ICFilter = 0;
HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1);
HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
```

This configures the same hardware. But the actual register writes are hidden inside
`HAL_TIM_IC_Init()` and `HAL_TIM_IC_ConfigChannel()`. A reader must dive into the
HAL source (approximately 1500 lines in `stm32g0xx_hal_tim.c`) to understand what
registers are written and in what order. A contributor who wants to change the
filter configuration must understand the HAL's configuration structure, find the
correct field name, and trust that the HAL translates it correctly. The direct
register code requires only the reference manual.

### 4.2 Debugging Without Abstraction Layers

When something goes wrong in an embedded system, the diagnosis begins with
inspecting hardware registers. In an IDE debugger connected via SWD, you can read
every register directly: "Is TIM2 running? Is CC1IE set? What is the current CCR1
value?" This interrogation of hardware state is fast and unambiguous in a bare-metal
system.

In a HAL-based system, you must also inspect the HAL state machine variables:
`htim2.State`, `htim2.Channel`, `htim2.hdma[TIM_DMA_ID_CC1]->State`. These are
not hardware state — they are the HAL's model of hardware state, which may or may
not reflect reality if the HAL state machine has been corrupted or has entered an
unexpected state. Debugging a HAL-based system requires understanding two layers:
the hardware behaviour and the HAL's representation of it.

For a community open source project where contributors range from professional
embedded engineers to technically capable amateurs, transparency is a feature.
The person debugging a field installation of the DSR-1 module does not need to
understand the STM32Cube HAL. They need the reference manual and the source code.
Both are freely available, both are authoritative, and both say the same thing about
what the hardware is doing.

---

## 5. The Interrupt Priority Model

### 5.1 The Priority Hierarchy

The DSR-1 firmware uses a simple, explicit priority hierarchy:

| Priority level | Interrupt | Latency constraint |
|---|---|---|
| Highest (0) | TIM2 capture (servo ISR) | Must execute within 15µs (1/65kHz) |
| Medium (1) | USB interrupt | Must complete before next SOF (1ms) |
| Lowest (2) | SysTick (if used) | No strict timing requirement |
| None (main loop) | ADC DMA callback, CDC parsing | Background only |

The servo ISR at priority 0 is guaranteed to preempt everything. On Cortex-M0+,
interrupt preemption requires that the interrupting interrupt has numerically lower
priority than the active interrupt (lower number = higher priority in ARM priority
encoding). With the servo at priority 0, it preempts all other interrupts
including USB and any future additions.

This priority assignment is unconditional and must not be changed by any firmware
contribution without explicit justification. Raising the USB interrupt priority
above the servo ISR would allow USB processing to delay servo execution — exactly
the non-determinism we are avoiding.

### 5.2 Main Loop Execution

The main loop runs at whatever speed is available after the interrupt handlers have
taken their share of CPU time. Its functions are:

1. Check for USB CDC received characters and parse servo tuning commands
2. Update any pending settings (write Kp/Ki changes to SRAM)
3. Handle settings save requests (flash write)
4. Blink the status LED if implemented
5. Loop

At 2500 Hz servo ISR, each ISR execution takes approximately 470ns, total ISR load
is 2500 × 470ns = 1.2ms per second — 0.12% of CPU time. The main loop has 99.88%
of CPU time available. USB CDC data at a baud rate of 115,200 bits/s occupies
approximately 0.1ms per second of interrupt handling. The processor is idle
(executing `WFI`, Wait For Interrupt) more than 99.5% of the time.

There is no scheduling problem to solve. An RTOS scheduler would add overhead to
manage tasks that are overwhelmingly idle. The simple "interrupts for hardware,
main loop for background" model is exactly appropriate for this workload.

---

## 6. Guidelines for Contributors

### 6.1 The Non-Negotiable Rules

These rules apply to all firmware contributions. They are not preferences — they
are architectural requirements that are enforced at review.

**Rule 1 — No HAL, no LL**: All peripheral access is direct register writes using
CMSIS constants. If you want to enable TIM3, write:
```c
RCC->APBENR1 |= RCC_APBENR1_TIM3EN;
```
Not:
```c
__HAL_RCC_TIM3_CLK_ENABLE();
```

**Rule 2 — No RTOS**: No scheduler, no task, no semaphore. If a function needs to
run periodically, it runs in the main loop or on an interrupt.

**Rule 3 — No malloc**: All memory is statically allocated. Arrays have fixed sizes
declared at compile time. If a feature requires dynamic memory, find a way to do
it statically or the feature does not belong in this firmware.

**Rule 4 — ISRs are short**: Interrupt service routines do only what must be done
at interrupt time. Anything that can be deferred to the main loop is deferred.
An ISR that sets a flag and returns is better than an ISR that processes data.

**Rule 5 — Servo ISR has highest priority**: Nothing can preempt the TIM2 capture
ISR. No other interrupt may be assigned priority 0.

**Rule 6 — Every register write has a comment**: Obvious to the person who wrote
it; not obvious to the person reading it three years later.

### 6.2 What "Improved Performance" Looks Like in Bare Metal

Improving the servo loop's performance — tighter speed control, faster lock-in
time, better disturbance rejection — means improving the algorithms in `servo.c`,
not adding infrastructure. Valid improvements include:

- Tuning Kp and Ki based on bench measurements of a specific motor
- Adding a derivative term (PID rather than PI) if bench testing shows benefit
- Adding a feed-forward term that anticipates systematic load variations
- Implementing per-variant FG target constants in `firmware/variants/`
- Improving the USB CDC command interface with new tuning commands

None of these require HAL, RTOS, or dynamic memory.

### 6.3 The Reference Manual Is the Authority

The STM32G0B1 Reference Manual (RM0444) is the authoritative description of every
peripheral register in the device. It is freely available at st.com. For every
register write in the DSR-1 firmware, the corresponding section in RM0444 explains
what that register does and what the valid values are.

A contributor who understands RM0444 can understand the firmware. A contributor who
only understands the STM32Cube HAL API cannot. The reference manual is therefore
the baseline knowledge requirement for firmware contributions — not IDE proficiency,
not HAL familiarity, not CubeMX configuration experience. Read the manual; write the
registers; explain in comments.

---

## 7. The Philosophical Alignment with the WM-D6C Project

The WM-D6C is an instrument. It was designed with precision, maintained by
professionals, and repaired by people who understood exactly what they were working
with. The service manual is a complete document of every component, every circuit,
and every adjustment procedure — nothing hidden, nothing magical.

The DSR-1 firmware aspires to the same standard. Every design decision is
documented. Every line of code that touches hardware explains what it is doing and
why. The person who installs this module in their machine can read the source code
and understand exactly what is running inside it. They can modify it, improve it,
and verify it against the hardware documentation without any intermediary.

This is not nostalgia for assembly language or hostility toward modern frameworks.
STM32Cube HAL is appropriate for many applications — prototyping, applications with
stringent certification requirements where the vendor-qualified code path matters,
teams that need to move quickly across many STM32 variants. It is not appropriate
here, where the application is small, the timing constraints are strict, the
community is mixed-skill, and the documentation philosophy demands transparency.

Bare-metal C is not a limitation of the DSR-1 project. It is an expression of its
values.

---

## See Also

- [Digital PLL Servo](digital-pll-servo.md) — the servo algorithm that makes these
  timing constraints necessary
- [Fixed-Point Arithmetic](fixed-point-arithmetic.md) — why single-cycle integer
  arithmetic replaces floating-point
- [USB Crystal-Less Operation](usb-crystalless-operation.md) — the hardware that
  makes crystal-less USB possible alongside the servo loop
- `firmware/src/servo.c` — the ISR code that embodies these principles
- `firmware/build/Makefile` — the build system that requires no IDE
