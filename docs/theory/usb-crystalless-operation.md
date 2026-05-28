# USB Crystal-Less Operation

## Purpose of This Document

Every USB device requires a precise 48 MHz clock to generate and decode USB Full
Speed signals. The conventional approach is an external quartz crystal — a small,
highly accurate resonator that vibrates at a known frequency. The DSR-1 module uses
no external crystal anywhere on the board. This document explains how USB 2.0 Full
Speed operation is achieved without one, why this matters specifically for the
WM-D6C project, and what the tradeoffs are.

---

## 1. Why USB Needs a Precise 48 MHz Clock

### 1.1 USB Full Speed Timing Requirements

USB Full Speed operates at 12 Mbit/s — each bit occupies 83.3 nanoseconds. The USB
2.0 specification requires that the transmitter's bit rate be accurate to ±2500 ppm
(parts per million) relative to the nominal 12 MHz bit clock, which is derived from
the 48 MHz USB clock by dividing by four. 2500 ppm represents 0.25% frequency
accuracy.

This sounds lenient, but consider the context. A quartz crystal oscillator typically
achieves ±20-100 ppm accuracy over temperature — ten to a hundred times better than
USB requires. RC oscillators in silicon, by contrast, are typically accurate to only
±1-5% without calibration — ten to fifty times worse than USB requires. The gap
between what an untrimmed RC oscillator delivers and what USB requires is the
problem that crystal-less operation must bridge.

The consequence of clock inaccuracy in USB is not graceful degradation — USB frames
will fail CRC checks, the host will retransmit, latency increases, and eventually
the device will enumerate unreliably or not at all. At 0.25% maximum error, there
is almost no margin: an RC oscillator at ±3% error is twelve times too inaccurate.

### 1.2 The Traditional Solution: External Crystal

In conventional USB device designs, a 12 MHz or 48 MHz quartz crystal is connected
to the microcontroller's oscillator input pins. The crystal's mechanical resonance
provides frequency accuracy of ±20-50 ppm, twenty to a hundred times better than
USB requires. This approach is reliable, well-understood, and cheap — a 12 MHz
crystal costs approximately $0.15 in quantity.

The WM-D6C presents a specific reason to avoid the crystal beyond general preference:
the machine has already suffered damage from improper handling and long-term storage.
Quartz crystals are mechanically fragile — a sharp mechanical shock can crack the
resonating element. A 35-year-old machine that has been dropped, stored poorly, or
subjected to the shock of incorrect adapter damage is exactly the scenario where a
crystal is at risk. Eliminating the crystal eliminates that failure mode from the
replacement module.

---

## 2. The STM32G0B1's Crystal-Less USB Architecture

### 2.1 The HSI48 Internal Oscillator

The STM32G0B1KBU6 contains an internal 48 MHz RC oscillator called HSI48 (High
Speed Internal 48 MHz). This is a dedicated oscillator for USB, separate from the
HSI16 that drives the system PLL. From DS13560 Rev 6, Table 45:

| Parameter | Value |
|---|---|
| Nominal frequency | 48 MHz |
| Factory calibration accuracy | ±3% at VDD = 3.0-3.6V, TA = -15 to 85°C |
| User trimming step | 0.11% to 0.18% per step |
| User trimming coverage | ±6% to ±7% total |
| Start-up time | 2.5µs typical, 6µs maximum |
| Power consumption | 340-380µA |

The ±3% factory calibration accuracy is far outside the USB requirement of ±0.25%.
Left untrimmed, the HSI48 cannot produce a clock accurate enough for USB. This is
where the Clock Recovery System (CRS) peripheral becomes essential.

### 2.2 The Clock Recovery System (CRS)

The CRS peripheral is a hardware frequency measurement and correction system built
into the STM32G0B1. It works by continuously comparing the HSI48 frequency against
a reference signal and automatically adjusting the HSI48 trim register to keep the
oscillator on target.

The reference signal can be sourced from several inputs. For USB crystal-less
operation, the reference is the USB Start-of-Frame (SOF) packet — a special USB
packet that the USB host (the computer or hub) sends exactly once per millisecond,
every 1.000ms ± 250ppm.

The CRS measures the number of HSI48 clock ticks between successive SOF packets.
If the USB clock were exactly 48 MHz and the SOF interval were exactly 1ms, there
would be exactly:

```
48,000,000 Hz × 0.001s = 48,000 ticks per SOF interval
```

If the HSI48 is running fast (say 48.5 MHz), the tick count between SOFs will be:

```
48,500,000 × 0.001 = 48,500 ticks  (500 more than expected)
```

The CRS detects this 500-tick excess (representing a +1.04% frequency error) and
decrements the HSI48 trim register to slow the oscillator down. On the next SOF
interval, the tick count is closer to 48,000. The CRS continues this correction
on every SOF packet — 1000 corrections per second.

### 2.3 The Math: CRS Accuracy

The USB host sends SOF packets at 1.000ms ± 250ppm intervals. The CRS uses this as
its frequency reference. The USB specification requires the device clock to be within
±2500 ppm of 48 MHz. The CRS achieves accuracy limited by the reference accuracy
plus the quantisation of the HSI48 trim step:

**Reference accuracy contribution**: The USB host SOF timing is accurate to ±250ppm
by specification. The CRS tracks this reference, so the resulting USB clock accuracy
inherits ±250ppm from the reference.

**Trim step quantisation**: The HSI48 trim step is 0.11% to 0.18% (1100-1800 ppm).
After CRS correction, the oscillator is within ±1 trim step of the target, so the
quantisation error is approximately ±900 ppm worst case.

**Combined accuracy**: The total error budget is ±250ppm (reference) plus ±900ppm
(quantisation) = approximately ±1150ppm. This is within the USB requirement of
±2500ppm with 2.2× margin.

ST Application Note AN4879 confirms that crystal-less USB device operation using
the CRS and SOF reference achieves reliable enumeration and sustained operation.
DS13560 Rev 6, Section 3.24 explicitly states: "The synchronization for this
oscillator can be taken from the USB data stream itself (SOF signalization) which
allows crystal-less operation in USB device mode."

### 2.4 The Chicken-and-Egg Problem and Its Solution

A careful reader will notice a circularity: the CRS needs the USB SOF signal to
trim the HSI48, but USB requires the HSI48 to be accurate enough to send and receive
USB packets in the first place. How does the device establish USB communication if
its clock is initially ±3% — well outside USB specification?

The answer lies in USB enumeration behaviour. When a USB device first connects to
a host, the host attempts to communicate at the standard 12 Mbit/s rate. If the
device's clock is off by ±3% (6× the USB specification limit), the initial
communication packets will likely fail — the device may not respond correctly to
the host's setup transactions. However, the USB host retries failed transactions
multiple times before giving up.

The key is that the STM32 receives the first USB SOF packet as soon as the host
detects the device connection (within the first few milliseconds). Even if the
initial clock accuracy is poor and some early packets fail, the CRS begins trimming
immediately from the first received SOF. After 10-20 SOF packets (10-20ms), the
CRS has corrected the HSI48 to within the USB accuracy specification. By the time
the host has completed its initial setup transactions (which take 50-200ms total),
the clock is accurate and stable.

In practice, the STM32G0B1 crystal-less USB implementation enumerates reliably with
virtually all USB hosts. The ±3% initial accuracy creates a brief window of potential
communication errors immediately after connection, but the CRS trims fast enough
that enumeration completes successfully. This has been validated in production
devices by STMicroelectronics and confirmed by the community.

---

## 3. Clock Configuration for the DSR-1 Module

### 3.1 Two Independent Clocks for Two Independent Functions

The DSR-1 uses two clock domains that must not interfere with each other:

**System clock (64 MHz)**: Drives the Cortex-M0+ core, TIM2 input capture, DAC,
ADC, and all peripheral buses. Generated by HSI16 (16 MHz internal RC) multiplied
by the PLL. Must be stable and accurate for servo loop timing.

**USB clock (48 MHz)**: Drives the USB Full Speed PHY exclusively. Generated by
HSI48 trimmed by CRS. Must meet USB accuracy requirements.

These two clocks are completely independent in the STM32G0B1. The USB peripheral
clock is sourced from HSI48 through a dedicated clock path that bypasses the system
PLL entirely. Changes to the system clock (from PLL adjustments, power mode changes,
or dynamic frequency scaling) do not affect the USB clock, and CRS trimming
adjustments to HSI48 do not affect the system clock.

This independence is essential for the servo loop. The TIM2 input capture measures
FG pulse periods in units of system clock ticks (64 MHz, 15.625 ns per tick). If
USB clock trimming could perturb the system clock, it would introduce jitter into
the FG period measurements and degrade servo accuracy. The separate clock
architecture eliminates this coupling entirely.

### 3.2 System Clock Startup Sequence

From power-on, the clock initialisation sequence in `main.c`:

```c
// Step 1: Enable HSI16 and wait for it to be ready
RCC->CR |= RCC_CR_HSION;
while (!(RCC->CR & RCC_CR_HSIRDY));  // Wait ~50µs

// Step 2: Configure PLL: HSI16 × 8 / 2 = 64 MHz
// PLLM = /1, PLLN = ×8, PLLR = /2
RCC->PLLCFGR = RCC_PLLCFGR_PLLSRC_HSI
             | (1 << RCC_PLLCFGR_PLLM_Pos)   // /1
             | (8 << RCC_PLLCFGR_PLLN_Pos)   // ×8 → 128 MHz VCO
             | (2 << RCC_PLLCFGR_PLLR_Pos)   // /2 → 64 MHz
             | RCC_PLLCFGR_PLLREN;            // Enable PLLR output

// Step 3: Enable PLL
RCC->CR |= RCC_CR_PLLON;
while (!(RCC->CR & RCC_CR_PLLRDY));   // Wait ~200µs for PLL lock

// Step 4: Set flash latency for 64 MHz operation (2 wait states)
FLASH->ACR = (2 << FLASH_ACR_LATENCY_Pos) | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN;

// Step 5: Switch system clock to PLL
RCC->CFGR = RCC_CFGR_SW_PLLRCLK;
while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLLRCLK);

// System is now running at 64 MHz
// Total time from power-on: approximately 300µs
```

### 3.3 HSI48 and CRS Startup Sequence

The HSI48 and CRS are enabled as part of USB initialisation, which happens after
the system clock is stable:

```c
// Enable HSI48
RCC->CR |= RCC_CR_HSI48ON;
while (!(RCC->CR & RCC_CR_HSI48RDY));  // ~3µs

// Enable CRS peripheral clock
RCC->APBENR1 |= RCC_APBENR1_CRSEN;

// Configure CRS: USB SOF reference, 48000 target count, auto-trim enabled
CRS->CFGR = (47999 << CRS_CFGR_RELOAD_Pos)  // Target: 48000 ticks per SOF
           | (34 << CRS_CFGR_FELIM_Pos)      // Frequency error limit
           | CRS_CFGR_SYNCSRC_USB;           // Use USB SOF as reference

// Enable CRS automatic trimming
CRS->CR = CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

// Configure USB clock source to HSI48
RCC->CCIPR = (3 << RCC_CCIPR_CLK48SEL_Pos); // Select HSI48 for USB
```

The RELOAD value of 47999 corresponds to 48000 - 1, because the counter counts from
RELOAD down to zero (48000 ticks per period). The FELIM value of 34 represents the
frequency error detection threshold in timer counts — approximately 34/48000 =
0.07% frequency error causes a CRS correction event.

After enabling CRS, the HSI48 begins trimming as soon as USB SOF packets arrive.
The CRS interrupt (ESYNCF flag) fires on each SOF, and the automatic trimming
hardware adjusts the HSI48 trim register without software involvement. The entire
USB clock calibration is hardware-automated.

---

## 4. Implications for the DSR-1 Design

### 4.1 Components Eliminated

Removing the external crystal eliminates from the BOM and PCB:

- **Crystal resonator**: Typically 3.2×2.5mm package, 2 pads, ±20-100ppm accuracy,
  approximately $0.15 in quantity
- **Two load capacitors**: Typically 12-18pF 0402 capacitors required by the crystal
  circuit, approximately $0.05 total
- **Crystal routing constraints**: The crystal traces must be short, isolated from
  digital signals, and free from parasitic coupling. On a dense 30×22mm board, these
  routing constraints consume significant PCB real estate

Total savings: approximately $0.20 in components, 2-3mm² of PCB area, and
significant routing effort.

### 4.2 Reliability Improvement

For the WM-D6C application specifically, eliminating the crystal improves module
reliability in two ways relevant to the machine's history and use:

**Mechanical shock immunity**: The WM-D6C is a portable device. It will be carried,
dropped, and subjected to mechanical shock. Quartz crystals are fragile resonating
elements — sharp impacts can crack the crystal blank, causing the oscillator to stop
or shift frequency. An RC oscillator on silicon has no fragile mechanical element
and is immune to mechanical shock.

**Aging immunity**: Quartz crystals age over decades, gradually shifting their
resonant frequency as the crystal structure relaxes from manufacturing stress. This
aging is typically 1-5 ppm per year — small but measurable over the lifetime of a
device. A 30-year-old crystal may have drifted 30-150 ppm from its nominal
frequency. For the servo clock (HSI16/PLL at 64 MHz), this is irrelevant — the
servo calibration accommodates the HSI16's ±1% tolerance. For the USB clock, the
CRS automatically corrects for any HSI48 drift on every SOF packet, making the USB
clock ageless.

### 4.3 The Constraint: USB Must Be Connected for CRS to Work

Crystal-less USB has one operational constraint that a crystal-based design does
not: the CRS needs USB SOF packets to trim HSI48, and SOF packets only arrive when
a USB connection is active.

For the DSR-1, this means:

**USB CDC live tuning**: When a computer is connected and the USB CDC interface is
active, the CRS is receiving SOF packets and the HSI48 is accurately trimmed.
Serial data transfers are reliable.

**DFU firmware update**: When the module is in DFU mode (BOOT0 held, USB-C
connected), the DFU bootloader receives SOF packets and the HSI48 is trimmed.
Firmware updates are reliable.

**No USB connection**: When the module is running the servo loop without a USB
cable connected, the CRS has no reference. HSI48 runs at its untrimmed ±3%
accuracy. This does not matter because the USB peripheral is not used — there is
nothing to clock accurately. The system clock (HSI16/PLL at 64 MHz) is completely
independent of HSI48 and is unaffected.

**Transition moment**: When a USB cable is first connected, the HSI48 begins
untrimmed and the CRS starts trimming from the first SOF. During the first 10-20ms
after connection, USB communication may be unreliable until the CRS brings HSI48
into specification. In practice this is invisible to the user — USB enumeration
takes 50-200ms total, well longer than the CRS convergence time.

### 4.4 Interaction with the Servo Loop

The servo loop runs on the system clock (64 MHz, from HSI16/PLL) and uses TIM2
input capture for FG period measurement. The USB CDC interface runs in the main
loop on the same 64 MHz system clock.

When a USB interrupt fires (SOF received, data available, DFU request), the interrupt
handler preempts the main loop but cannot preempt the TIM2 capture ISR (which is
set to higher priority). The servo loop timing is therefore not affected by USB
activity. The CRS trimming adjustments to HSI48 operate on a completely separate
clock and have zero effect on the 64 MHz system clock used by the servo loop.

This isolation is a deliberate consequence of the two-clock architecture. The servo
loop is shielded from all USB-related activity — clock trimming, interrupt handling,
data transfers — by both hardware architecture and interrupt priority assignment.

---

## 5. Crystal-Less Operation in the Context of a 35-Year-Old Machine

There is a philosophical alignment between crystal-less operation and the DSR-1
project's goals that is worth making explicit.

The WM-D6C's original servo circuit failed partly because its components — the
X701 crystal oscillator, the IC701 divider IC, the CX20084 ASIC — are aging physical
objects. They drift, degrade, and eventually fail. The DSR-1 replaces them with
components that are either more stable (the TARGET_PERIOD integer in flash, the
HSI16/PLL system clock), more self-correcting (the HSI48/CRS combination that
retrains itself on every SOF packet), or more insensitive to aging (the STM32 die,
which will long outlast any quartz crystal).

A crystal on the DSR-1 PCB would be a new fragile component on a board installed in
a decades-old machine that will continue to be handled, carried, and potentially
subjected to mechanical stress for decades to come. Eliminating it removes a future
failure mode before it can be introduced.

The CRS's continuous self-calibration also aligns with the philosophy behind the
servo loop itself: rather than relying on a fixed hardware reference (a crystal),
the system continuously measures reality (SOF packet timing from the USB host) and
corrects itself. This is the same principle as the PI servo loop measuring actual
tape speed through the FG signal and continuously correcting the motor drive. Both
are self-referencing, self-correcting systems — the right approach for a device that
must maintain precision over decades of operation.

---

## 6. Firmware Configuration Summary

For completeness, the complete USB peripheral configuration relevant to crystal-less
operation:

```c
// USB clock: HSI48 trimmed by CRS via USB SOF
// System clock: HSI16 × PLL = 64 MHz (independent of USB clock)

// Enable clocks
RCC->APBENR1 |= RCC_APBENR1_USBEN | RCC_APBENR1_CRSEN;
RCC->CR |= RCC_CR_HSI48ON;
while (!(RCC->CR & RCC_CR_HSI48RDY));

// Set USB clock source to HSI48
RCC->CCIPR = (3 << RCC_CCIPR_CLK48SEL_Pos);

// Configure CRS for USB SOF synchronisation
CRS->CFGR = (47999 << CRS_CFGR_RELOAD_Pos)  // 48000 - 1 ticks per SOF
           | (34   << CRS_CFGR_FELIM_Pos)    // Error limit: ~70ppm
           | CRS_CFGR_SYNCSRC_USB;           // SOF as reference source

// Enable auto-trimming (hardware manages HSI48 trim register automatically)
CRS->CR = CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

// Enable USB peripheral and internal DP pull-up (signals connection to host)
USB->CNTR &= ~USB_CNTR_PDWN;                 // Power down off
// ... additional USB endpoint configuration follows
USB->BCDR |= USB_BCDR_DPPU;                  // Enable DP pull-up = connect to host
```

The CRS runs autonomously after this configuration. No periodic software tasks are
required for clock maintenance.

---

## See Also

- [Digital PLL Servo](digital-pll-servo.md) — how the system clock drives TIM2 and
  the servo loop
- [Why Bare Metal](why-bare-metal.md) — the architecture philosophy that keeps the
  servo loop isolated from USB handling
- [Signal Chain Analysis](signal-chain-analysis.md) — USB data path routing and ESD
  protection
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — clock configuration
  in Section 3.3
