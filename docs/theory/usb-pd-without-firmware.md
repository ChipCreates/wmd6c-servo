# USB Power Delivery Without Firmware — The IP2721 Hardware PD Trigger

## Purpose of This Document

This document explains how the DSR-1 Variant A module negotiates a 9V USB Power
Delivery supply contract from a USB-C charger without writing a single line of
firmware to handle it — using the IP2721 hardware PD trigger IC. It explains the
USB PD protocol at a level that makes the IP2721's operation understandable, why
this approach was chosen over using the STM32's built-in UCPD controller for power
negotiation, what happens when things go wrong, and what the STM32 monitors on the
CC lines during normal operation.

This document is grounded in the specific context of the WM-D6C: a 35-year-old
professional cassette recorder that was designed before USB existed, being given a
modern power interface that draws from the same charger sitting on your desk.

---

## 1. Why USB-C for a 1990 Cassette Recorder

The WM-D6C's original power input — CN301, the negative-centre barrel jack — is the
single most common cause of failure in every surviving unit. The failure mechanism
is simple and brutal: the connector is physically identical to a standard adapter
jack but electrically backwards, and there is no protection between the jack and the
CX20084 servo IC. One wrong adapter destroys the machine in milliseconds.

USB-C Power Delivery solves this problem not just better than the original design,
but better than any alternative:

**USB-C is physically non-destructible**: The connector is symmetrical — it can be
inserted either way. There is no polarity to get wrong.

**USB PD negotiates before delivering power**: A USB PD source does not apply full
voltage to VBUS until a successful negotiation handshake has occurred on the CC
lines. A dumb adapter with no VBUS will apply whatever voltage it produces without
negotiation — but a PD charger will not deliver 9V until the sink explicitly
requests and confirms it. This means that if the module's PD trigger fails or the
CC lines are miswired, the worst outcome is 5V default VBUS — insufficient to power
the machine, but harmless to the circuitry.

**USB-C chargers are universal**: The Sony AC-D4M, the correct original adapter for
the WM-D6C, is increasingly rare and expensive. USB-C PD chargers are on every
desk, in every laptop bag, and at every airport charging station on earth. The
Variant A module can be powered from any of them.

**USB-C supports data simultaneously**: The same connector that delivers power also
carries the USB 2.0 data lines that the STM32 uses for firmware updates and live
servo tuning. One cable replaces both the power adapter and a programming cable.

---

## 2. What USB Power Delivery Actually Is

### 2.1 The Problem USB PD Solves

Standard USB charging before PD was a mess of proprietary protocols — Apple's
divider scheme, Qualcomm Quick Charge, Samsung AFC, VOOC — each incompatible with
the others, each requiring the charger and device to be from the same ecosystem or
a compatible third party. USB Power Delivery is the USB-IF's attempt to create a
universal, open standard for high-power charging.

The key insight of USB PD is that **the supply voltage and current limit are
negotiated between two devices over a separate communication channel**, rather than
being fixed by the hardware as they are in a conventional power supply. A USB PD
charger is not a fixed-voltage supply — it is a programmable power supply that can
deliver 5V, 9V, 15V, or 20V at various current limits, depending on what the
connected device requests.

### 2.2 The Physical Layer — CC Lines

USB-C connectors have two CC pins (CC1 and CC2) that are not present in older USB
connectors. These pins serve two functions:

**Cable orientation detection**: The USB-C connector is symmetrical, but the CC
pins are on opposite sides. When a cable is connected, only one CC pin per connector
makes contact (depending on which way the plug is inserted). By detecting which CC
pin is connected, the system determines cable orientation and configures the
appropriate data lanes.

**Power delivery communication**: When a USB PD source and sink connect, they
communicate over the CC lines using a protocol called BMC (Biphase Mark Coding) at
a baud rate of 300kbit/s. This is a completely separate communication channel from
the USB data lines (D+ and D−). USB PD communication continues to function correctly
even if the USB data lines are not connected.

### 2.3 The USB PD Protocol at the Level That Matters

The USB PD negotiation between a charger (source) and a device (sink) follows this
sequence after connection:

**Step 1 — Default state**: The source applies 5V to VBUS immediately on connection
(this is the USB default). Both sides monitor the CC lines.

**Step 2 — Source capabilities advertisement**: The source transmits a
"Source_Capabilities" message over the CC line. This message is a list of
"Power Data Objects" (PDOs) — each PDO specifies a voltage and maximum current that
the source can provide. A typical phone charger might advertise:
```
PDO 1: 5V, 3A (15W)
PDO 2: 9V, 2A (18W)
PDO 3: 12V, 1.5A (18W)
```

**Step 3 — Sink request**: The sink responds with a "Request" message, selecting one
of the advertised PDOs. The request specifies the desired voltage and the maximum
current the sink needs.

**Step 4 — Source acceptance and power transition**: If the source accepts the
request, it sends an "Accept" message, then a "PS_RDY" (Power Supply Ready) message
after it has transitioned its output to the requested voltage. VBUS is now at the
negotiated voltage.

**Step 5 — Contract in force**: Both sides have agreed on a voltage and current
limit. This "contract" remains in force until the cable is disconnected or one side
requests a renegotiation.

The entire negotiation from connection to 9V VBUS takes approximately 50-100ms.

### 2.4 What Happens Without PD Negotiation

A USB-C charger that supports PD will sit at 5V VBUS until it receives a PD request.
If no request ever comes (because the sink is a simple 5V-only device), VBUS stays
at 5V and full power is available at that voltage. This backward compatibility means
that USB-C PD chargers can charge old devices that only understand 5V.

For the DSR-1 module, 5V VBUS is insufficient — the machine needs 6V B+1. Without
a successful 9V negotiation, the TPS62xx buck converter has no valid input. The
IP2721 handles this situation by failing gracefully: if no PD source responds or if
9V is not available, the IP2721 does not enable its output path, VBUS remains at 5V,
and the machine simply does not power on. No harm occurs.

---

## 3. The IP2721 Hardware PD Trigger

### 3.1 What It Does in One Sentence

The IP2721 is a USB PD sink protocol chip that negotiates a fixed voltage PDO from
a USB PD source, using a single resistor to select the target voltage, without any
microcontroller involvement.

### 3.2 The Traditional Alternative: MCU-Controlled PD

The conventional approach to USB PD in an embedded system is to use a dedicated PD
controller IC (FUSB302, HUSB238) or the MCU's built-in UCPD peripheral (which the
STM32G0C1KCU6 has) and implement the PD protocol stack in firmware. The STM32's
UCPD peripheral can send and receive BMC-encoded PD messages, perform CRC checking,
and handle all the protocol states defined in the USB PD specification.

This approach gives complete control over the negotiation — the firmware can request
any PDO, handle renegotiation, respond to charger disconnection, implement USB PD
3.0 features like Fast Role Swap (FRS), and integrate power management decisions
with the servo loop logic.

The cost is firmware complexity. A complete USB PD firmware stack is several
thousand lines of code, requires careful state machine design, must handle edge
cases and protocol timeouts, and is a potential source of bugs that could leave the
machine unpowered or with incorrect voltage. For a project where the primary goal is
a reliable, easy-to-install servo replacement, this complexity is disproportionate.

### 3.3 The IP2721 Approach: Hardware PD

The IP2721 implements the entire USB PD sink protocol in silicon — it contains a
hardwired state machine that handles all protocol states, message encoding and
decoding, retries, timeouts, and error handling. No firmware is required. The only
configuration is a single resistor on the SEL (select) pin:

| SEL Pin State | Requested Voltage |
|---|---|
| SEL = GND | 5V only (default charging) |
| SEL = floating | Up to 15V (requests highest available ≤15V) |
| SEL = VDD (high) | Up to 20V (requests highest available ≤20V) |

For the DSR-1, we need 9V. Since the IP2721 requests the highest available voltage
up to its configured maximum, and virtually all PD chargers offer 9V as their second
tier, the correct configuration is SEL pulled to a voltage that requests 9V
specifically.

**The precise SEL configuration for 9V**: The IP2721 datasheet (INJOINIC, v1.02)
describes that when SEL is pulled low to GND, only 5V is requested. When floating,
up to 15V is requested. To specifically target 9V, the recommended approach is to
use SEL = GND (5V request) combined with a configuration resistor on the CFG pin
that limits the maximum requested voltage to 9V. Alternatively, if the charger
supports 9V but not 15V or 20V (which describes essentially all phone chargers),
the floating SEL configuration will naturally land on 9V.

In practice, for the WM-D6C application, the correct configuration is:
- SEL = floating (or pulled to mid-supply via equal resistors to VDD and GND)
- Accept the highest voltage the source offers up to 15V

The TPS62xx buck converter is rated for input up to 17V, so even if a 15V PDO is
accepted from a laptop charger, the buck handles it correctly. The firmware monitors
the VBUS voltage via an ADC divider to confirm the negotiated voltage is within the
expected range (8-15V) before enabling the servo loop.

### 3.4 The IP2721 Negotiation Sequence

From the perspective of someone monitoring the CC line with an oscilloscope, here
is what happens when a USB-C PD charger is connected to the Variant A module:

**t = 0ms**: USB-C cable inserted. The IP2721 VBUS pin detects the 5V default VBUS.
The IP2721's internal state machine begins the PD protocol.

**t = 0 to 5ms**: The IP2721 monitors the CC line for the source's
Source_Capabilities message. A USB PD source must send this message within 250ms
of connection (the tTypeCSendSourceCap timer). Most chargers send it within 20-50ms.

**t = ~20-50ms**: Source_Capabilities received. The IP2721's internal state machine
parses the PDO list, identifies the 9V PDO (or the highest available PDO matching
its configuration), and constructs a Request message.

**t = ~50ms**: IP2721 transmits the Request message over CC1.

**t = ~60ms**: Source sends Accept message.

**t = ~80ms**: Source transitions VBUS from 5V to 9V (the tSrcReady timer allows up
to 550ms for this transition; most chargers do it in 10-30ms).

**t = ~90ms**: Source sends PS_RDY message. VBUS is now 9V.

**t = ~90ms**: IP2721 asserts its PG (power good) output signal. This signal can be
used to enable the TPS62xx buck converter — holding the buck disabled until 9V VBUS
is confirmed prevents the machine from attempting to operate from an insufficient
supply.

The entire sequence from cable insertion to 9V VBUS available is approximately
90-100ms. The STM32 boot sequence (2ms to servo loop running) occurs well after
this — if power is applied from the charger before the STM32 is ready to run, the
servo simply begins its normal spin-up sequence with a fully established 9V input.

### 3.5 What If 9V Is Not Available

The IP2721 handles several failure cases:

**Source does not support PD**: The IP2721 sends PD messages but receives no
response within the protocol timeout. After the timeout, the IP2721 accepts the
default 5V VBUS. The PG output is not asserted (because 9V was not obtained). The
TPS62xx buck converter remains disabled. The machine does not power on. No damage
occurs.

**Source supports PD but not 9V**: Some older or lower-power PD chargers offer only
5V PD (a 3A PDO at 5V). The IP2721 selects the highest available PDO (5V, 3A).
Again, PG is not asserted for 9V. The machine does not power on.

**Source supports PD and 9V but at insufficient current**: If the charger only
offers 9V/0.5A (4.5W), the IP2721 will negotiate this PDO. The machine requires
approximately 9V/0.4A (3.6W) at full load, so 9V/0.5A provides marginal headroom.
In practice this scenario is rare — virtually every 9V PD source offers at least
1A (9W).

**Cable disconnected during operation**: The IP2721 detects VBUS collapsing (below
approximately 4V) and immediately disasserts PG. The TPS62xx buck converter stops.
The MCP1700 output drops. The STM32 BOR fires. The machine stops cleanly with the
motor off.

### 3.6 IP2721 External NMOS Gate Control

The IP2721 datasheet describes using an external N-channel MOSFET on the VBUSG pin
as a power path switch. When the PD contract is established, VBUSG goes high,
turning on the NMOS and connecting VBUS to the downstream circuitry. When the
contract drops (disconnection, fault), VBUSG goes low and the NMOS disconnects VBUS.

For the DSR-1, this power path switch can be implemented using an appropriate
N-channel MOSFET (Vds ≥ 20V, Id ≥ 1A) in the VBUS line before the TPS62xx input.
This provides an additional layer of protection: VBUS is physically disconnected
from the converter until the IP2721 has confirmed a valid PD contract, preventing
any voltage from reaching the converter during the negotiation window.

The STM32 monitors the VBUSG signal (or a divided version of VBUS) through an ADC
input to confirm supply health. If VBUS drops unexpectedly during operation, the
STM32 initiates a controlled motor shutdown before the supply fully collapses.

---

## 4. The STM32's Role in Power Management

### 4.1 Why the STM32 Has UCPD Hardware at All

The selected STM32G0C1KCU6 exposes UCPD support for the selected USB-C pins.
UCPD1 connects to PA8 (CC1) and PA9 (CC2) in the accepted pin allocation. This hardware can
implement the full USB PD protocol stack in firmware, send and receive PD messages,
negotiate PDOs, implement USB PD 3.0 features, and act as either a PD source or sink.

Since the IP2721 handles PD negotiation without firmware, why include UCPD hardware
in the design at all? Three reasons:

**CC line monitoring for contract confirmation**: Even with IP2721 handling
negotiation, the STM32 can monitor the CC lines to confirm that a valid PD contract
is in place and determine what voltage was negotiated. The UCPD peripheral can be
configured to receive PD messages in a passive "snoop" mode without transmitting,
allowing the STM32 to read the PD contract parameters after the IP2721 has
established them.

**Fallback firmware PD**: In a future firmware revision, if a specific PD charger
is identified as incompatible with the IP2721 (unusual but not impossible), the
firmware could take over PD negotiation directly. The hardware is already present.

**USB PD Status Reporting**: The STM32 could report the current power delivery
status via USB CDC telemetry — "9V PD contract established, 18W charger detected,
VBUS = 9.02V" — useful diagnostic information for an installer verifying the
system is operating correctly.

### 4.2 What the Firmware Actually Monitors

In the initial firmware implementation, the STM32 performs three power-related
monitoring functions:

**VBUS voltage monitoring**: A resistor divider from VBUS to an ADC input (R_VBUS
100kΩ + R_VBUS_LO 10kΩ to GND) scales the 9V VBUS to 0.82V at the ADC input,
within the 0-3.3V ADC range. The firmware reads this voltage every 100ms. If VBUS
drops below 7V (indicating the PD contract has dropped or the charger has been
disconnected), the firmware immediately sets the motor drive to off and holds it
off until VBUS is restored and stable above 8V for at least 100ms. This prevents
the motor from running during a supply interruption.

**MOTOR_EN monitoring**: The auto-off circuit on the WM-D6C (Q701/Q702 on the
Auto-Off board) can interrupt motor operation independently of power management.
The firmware monitors this signal on PA5 continuously.

**BOR threshold**: The STM32's Brown-Out Reset at 2.8V VDD provides a final
hardware safety net. If VDD drops below 2.8V for any reason (supply failure,
LDO dropout), the STM32 resets and all outputs return to their reset-state safe
conditions (motor off).

### 4.3 The CC Resistor Convention

USB-C devices identify their role (source/sink/DRP) and capability using pull
resistors on the CC pins. The IP2721 implements the sink role identification by
connecting the CC lines to GND through internal 5.1kΩ resistors (the USB-C
specification's Rd resistors for sink devices). This is required for the charger
to recognise that a sink device is connected.

In Variant A, the CC lines are connected to the IP2721 for power negotiation. They
are also connected to the STM32 UCPD1 pins (PA8, PA9) for monitoring. The IP2721's
internal Rd resistors are sufficient — no external CC resistors are needed. The
STM32 UCPD pins are configured as high-impedance inputs during normal operation,
presenting negligible load on the CC lines.

**What NOT to do**: In Variant A, do not place the passive 5.1kΩ CC resistors to
GND that are commonly used for simple 5V USB-C power — these would also present Rd
on the CC lines, but without PD negotiation. A charger seeing Rd on CC without any
PD communication would assume the device is a simple 5V consumer and never offer
9V. The IP2721's active PD implementation replaces the passive resistors.

---

## 5. USB PD in Practice — Real-World Behaviour

### 5.1 Charger Compatibility

The vast majority of USB-C PD chargers on the market will deliver 9V to the DSR-1
module without any issues. This includes:

- Apple USB-C power adapters (5W, 20W, 30W, 61W, 87W, 96W, 140W)
- Apple MagSafe USB-C adapters
- Google USB-C chargers (Pixel, Chromebook)
- Samsung USB-C chargers (25W, 45W, 65W variants)
- Anker, Belkin, Aukey, UGREEN branded USB-C PD chargers
- Laptop USB-C chargers (Dell, HP, Lenovo, Microsoft, Razer)
- Most USB-C power banks with PD output

**Chargers that may not work**:

*Legacy USB-A chargers with a USB-A to USB-C cable*: These do not support PD.
VBUS will be 5V only. The machine will not power on, but no damage occurs.

*Very cheap USB-C chargers marked "5V only"*: These apply 5V to VBUS without PD
negotiation. The IP2721 will not be able to negotiate 9V. The machine will not
power on.

*Qualcomm Quick Charge adapters without USB PD*: Early QC adapters used a
proprietary protocol over the USB data lines, not over CC. They are incompatible
with USB PD. The machine will not power on with these adapters.

### 5.2 The Cable Matters

USB-C cables are not all equal for Power Delivery. The requirements:

**Any USB-C to USB-C cable rated for 5A (E-Marker cable)** supports full USB PD up
to 100W. These cables contain an electronically marked IC that identifies them to
the charger as 5A-capable. For 9V/1A operation (9W), an E-marker cable is not
required but is always acceptable.

**USB-C to USB-C cables rated for 3A** support USB PD at the current levels needed
for the WM-D6C (maximum approximately 0.5A at 9V = 4.5W). These are widely
available and inexpensive.

**USB-A to USB-C cables** typically have USB-A on the charger end and cannot
negotiate USB PD voltages above 5V. These will not power the Variant A module at
9V.

**Cable length**: Longer cables have higher resistance, which causes VBUS to sag
under load. At 0.5A load through a 3m USB-C cable with 0.2Ω per conductor
resistance: V_sag = 0.5 × 0.4Ω = 200mV. This reduces 9V VBUS to 8.8V at the
module connector — still well above the 8V minimum VBUS monitoring threshold.
Standard cables (1m to 2m) present negligible sag.

### 5.3 Hot-Plug and Re-Plug Behaviour

The IP2721 handles repeated connect/disconnect cycles gracefully. Each connection
triggers a new negotiation sequence from the beginning. The PG output de-asserts
immediately on disconnection and re-asserts only after a new successful negotiation
— the machine is held off during the negotiation window, which takes approximately
100ms.

For the WM-D6C user, this means:
- Unplugging and replugging the USB-C cable stops the machine momentarily (motor
  stops, servo resets) then restarts it after approximately 200ms (100ms for PD
  negotiation + 100ms firmware startup)
- The tape remains loaded and the transport position is unchanged
- The next play button press restarts tape transport normally

### 5.4 Power Banks and Portable Operation

A USB-C power bank with PD output can power the Variant A WM-D6C in the field —
without batteries, without an AC adapter, anywhere a charged power bank is available.
This is a significant enhancement to the machine's operational flexibility compared
to the original battery-dependent design.

A 10,000mAh power bank at 9V (90Wh) powering the WM-D6C at approximately 3-4W
provides approximately 22-30 hours of operation — far exceeding the battery life
of the original four AA cells (approximately 5-8 hours with quality batteries).

The only practical consideration is that some power banks disable their 9V PD output
when they detect very low load currents — a behaviour called "auto-off" or "low
current protection" designed to prevent the bank from draining against phantom loads.
The WM-D6C in pause mode draws approximately 10-15mA from 9V VBUS (0.09-0.135W),
which may trigger some power banks' auto-off circuitry. If this is observed (the
machine loses power after a few minutes in pause), keep a cassette in the machine
and periodically cycle to play mode, or use a different power bank that does not
implement aggressive auto-off.

---

## 6. The UCPD Peripheral for Future Firmware Features

### 6.1 What UCPD1 Can Do When Enabled

The STM32G0C1's UCPD1 peripheral can be fully programmed in a future firmware
revision to take over PD negotiation from the IP2721, or to work alongside it for
enhanced monitoring and control. The UCPD peripheral handles:

- BMC encoding and decoding of PD messages at 300kbit/s
- CRC generation and checking for message integrity
- 4b5b encoding and decoding
- Ordered set detection (SOP, SOP', SOP'')
- Hard Reset and Fast Role Swap detection
- GoodCRC auto-reply (configurable)
- Direct Memory Access for message payload transfer

The STM32Cube firmware package includes an X-CUBE-TCPP middleware layer for USB PD,
though the DSR-1 firmware explicitly avoids Cube HAL. A bare-metal UCPD
implementation requires writing to the UCPD_CR, UCPD_IMR, and UCPD_TXPAYSZ
registers directly, and implementing the protocol state machine in software.

### 6.2 The Power Profile Manager Concept

A future firmware feature could replace the IP2721 entirely and implement a
"power profile manager" that:

1. Negotiates the optimal voltage from the charger (9V if available, but falls back
   to the highest available PDO that the TPS62xx can accept)
2. Reports the negotiated voltage and current limit via USB CDC telemetry
3. Implements USB PD 3.0 programmable power supply (PPS) for fine voltage control
4. Monitors the PD contract health and gracefully handles charger disconnection
5. Implements USB PD power role swap if a DRP charger is connected

This is an enhancement for a future major revision, not a requirement for v1.0.
The IP2721 approach is correct and sufficient for the initial release — it keeps
the power system completely independent of firmware correctness.

### 6.3 Why Keeping Power Independent of Firmware Is the Right Choice for v1.0

The servo loop, the USB CDC interface, and the ADC scanning are all firmware
functions. If a bug in any of these functions causes the STM32 to crash or hang,
the machine simply stops — the servo halts, the motor coasts to a stop. This is a
benign failure.

If the power negotiation were also a firmware function, a bug in the servo loop
could hang the MCU and cause the power delivery to fail — VBUS drops, the entire
machine loses power, and the user has no indication of what went wrong. Worse,
if the firmware is being updated via USB DFU, the MCU is executing bootloader code,
not application code. If the application code were responsible for PD negotiation,
updating the firmware would cause the power to drop in the middle of the update —
potentially corrupting the flash.

By placing PD negotiation entirely in the IP2721 hardware, the power supply remains
stable regardless of what the firmware is doing. The STM32 can be in DFU mode,
crashed, executing a hard fault handler, or simply not yet loaded — the 9V supply
continues to be delivered correctly.

This is the same philosophy that motivates keeping the servo loop in hardware-
triggered interrupts rather than a software main loop: the most critical functions
are independent of software correctness.

---

## 7. Variant B and the Parallel Question

Variant B retains the barrel jack, eliminates reverse polarity through the LTC4359
ideal diode bridge, and adds protection against overvoltage and overcurrent. It does
not use USB PD at all.

For the user who wants to keep using their original Sony AC-D4M adapter, Variant B
is the correct choice. The AC-D4M delivers a nominally unregulated 6V output. At
no load, an unregulated AC adapter may measure 7-8V due to the transformer's poor
regulation. The SMBJ7.0A TVS clamp handles this — it clamps the input to
approximately 7.8V maximum before the downstream circuits see it.

For the user who has lost their AC-D4M and uses a generic 6V adapter, Variant B
accepts any polarity and adds protection the generic adapter cannot provide.

For the user who wants the cleanest, most modern solution with the most powerful
features (USB data, power bank compatibility, universal charger compatibility),
Variant A is the correct choice.

Both variants produce identical B+1, B+3, and 3.3V rails to the downstream
circuitry. From the STM32's perspective and the machine's perspective, the two
variants are electrically identical at and beyond the rail outputs.

---

## See Also

- [Power Supply Design](power-supply-design.md) — MT3608, MCP1700, TPS62xx, and
  LTC4359 design in depth
- [Signal Chain Analysis](signal-chain-analysis.md) — VBUS monitoring via ADC,
  USB data path, CC line routing
- [Why This Failed](why-this-failed.md) — the original failure mode that motivated
  the USB-C power input
- [Module Datasheet](../datasheet/WMD6C_Module_Datasheet.pdf) — complete electrical
  specification for both variants
