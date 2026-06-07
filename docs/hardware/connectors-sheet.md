# DSR-1 Connectors Sheet — WM-D6C Machine Interface

## Reference version

This sheet targets the **former-type** WM-D6C servo board in the primary unit:
serial `72795`, observed PCB marking `C11-494-12`, with a **surface-mount CX20084**
at IC601. Q601 remains a physical-identification and bench-verification item on
this SMD board; do not assume the older through-hole transistor package. Do **not**
take pin detail from the Supplement-4 new-type CX-069A sheets; that is a different
circuit and a planned post-Rev A variant.

## Principle: the original machine has no single servo connector

The original WM-D6C uses point-to-point colored flying leads plus a small 3-terminal
DC-DC daughterboard (CP304). The "8-pin JST J1 carrying all signals" in the legacy module
datasheet is a consolidation abstraction — it does not exist as one physical part,
and it omits B+3. This sheet replaces that abstraction with the three connections
the Servo Control Board actually makes to the machine.

Two things are **not** Servo Control Board connectors — they stay on the machine, and the
Servo Control Board only drives into their nodes:

- **M901 (capstan motor)** — remains driven by Q601 / Q703 / Q704 on the main board.
  The Servo Control Board's only influence on the motor is (a) the **B+3 rail** it supplies
  (J1 pin 3) and (b) the **Q601 base drive** it produces (J2 pin 2 → CX20084 pad 15).
  The motor's two leads never come to the Servo Control Board.
- **Optical FG sensor (PH701, GP2S22AB)** — remains on the machine with its LED
  supply and B+1 pull-up. The Servo Control Board taps only its **output node** at CX20084
  pad 13 (J2 pin 1). The sensor's four leads never come to the Servo Control Board.

---

## J1 — Power / DC-DC cavity connector (reuses the CP304 socket)

3-terminal, confirmed directly from the original DC-DC board schematic. Orientation
note: on the sheet the **output (10.8V) is on the left (pin 3)** and the
**input (6V) is on the right (pin 1)** — go by pin number, not physical side.

| Pin | Net | Direction | Machine node | Notes |
|-----|------------|-----------------|--------------------------------|-------|
| 1 | B+1 (6V) | Servo Control Board → machine | CP304 cavity pin 1 | Powers machine logic/audio. Machine-side filter C317 220µF 10V. In the original this was the 6V *input* to CP304; the Servo Control Board now *supplies* it. |
| 2 | GND | shared | CP304 cavity pin 2 | Common return. |
| 3 | B+3 (10.8V) | Servo Control Board → machine | CP304 cavity pin 3 → Q703/Q704 → M901 | Motor rail. MT3608 boost output replaces CP304 here. Machine-side filter C316 100µF 16V. |

Series Schottky (1N5819) on the Servo Control Board's B+1 feed is recommended (≈0.3V drop →
machine sees ~5.7V, within spec) to block reverse current on power sequencing.

---

## J2 — Servo flying leads (to the removed CX20084 / IC601 pads)

IC601 is desoldered; these leads land on its cleaned pads at board position **G-12**.
Pad numbers below are from the legacy module docs — **verify on the physical board before
soldering.**

| Pin | Net | Direction | CX20084 pad | Conditioning on Servo Control Board | Notes |
|-----|-----------|-----------------|-------------|------------------------|-------|
| 1 | FG_RAW | machine → Servo Control Board | pad 13 | R3/R4 divider + BAT54 clamps + 10pF | Open-collector, pulled to B+1 on machine. Swings ~0 to ~B+1. Confirm swing → sets R3/R4. |
| 2 | Q601_BASE | Servo Control Board → machine | pad 15 | PWM→RC→level-shift NPN | Drive to Q601 base. Direct DAC drive is not used; bench measurement confirms R9 sizing and PWM-to-speed sign. |
| 3 | MOTOR_EN | machine → Servo Control Board | pad 7 | R10/R11 divider to GPIO | ~4.4V in playback, ~0.1–0.5V at auto-off. Divider → ~3.03V at PA5. |
| 4 | GND | shared | pad 14 | — | Servo ground reference. |
| 5 | (VDD / B+1) | n/c or tie | pad 16 | — | Original CX20084 supply. Module already supplies B+1 on J1 — leave open or tie to B+1, do not double-drive. |

---

## J3 — Speed trim pots + Speed Tune switch

Wipers are read by the Servo Control Board ADC; the pots themselves are unchanged on the machine.
Reference rails (B+1 top, GND bottom) are shared with J1 — only the wipers and the
switch are unique signals here.

| Pin | Net | Direction | Machine node | Notes |
|-----|-------------|-----------------|----------------------------------|-------|
| 1 | RV601_WIPER | machine → Servo Control Board | RV601 wiper (47kΩ cermet, base speed) | ADC. Verify wiper ≤3.3V over full travel; add divider if not. |
| 2 | RV602_WIPER | machine → Servo Control Board | RV602 wiper (20kΩ carbon, Speed Tune) | ADC. Verify ≤3.3V. |
| 3 | RV603_WIPER | machine → Servo Control Board | RV603 wiper (47kΩ cermet, range) | ADC. Verify ≤3.3V. |
| 4 | S601 | machine → Servo Control Board | Speed Tune slide switch | GPIO (PA7). |
| 5 | B+1 ref | Servo Control Board → machine | pot top ends | Shared with J1 pin 1; pots reference this. |
| 6 | GND | shared | pot bottom ends | Shared with J1 pin 2. |

---

## CX20084 (IC601) pad quick reference

The Servo Control Board's entire servo interface lands on these pads after the IC is removed:

| Pad | Function | Servo Control Board net | Servo Control Board connector |
|-----|----------------------|------------|------------------|
| 7 | Motor enable input | MOTOR_EN | J2 pin 3 |
| 13 | FG signal input | FG_RAW | J2 pin 1 |
| 14 | Ground | GND | J2 pin 4 |
| 15 | Motor drive output | Q601_BASE | J2 pin 2 |
| 16 | Supply (VDD / B+1) | B+1 | J2 pin 5 (optional) |

---

## J4 — VU/Battery LED-board harness

The front-panel VU/battery level switch and LED bar are routed to the MCU in the
battery build. `S801_BATT` is the MCU enable flag for battery-gauge ownership.
`BATT_LED1-5` drive D801-D805 only while `S801_BATT` is asserted; in VU/non-BATT
operation the MCU outputs must be released/high-Z so they do not contend with the
CX10043 VU circuitry.

| Pin | Net | Direction | Machine node | Notes |
|-----|-----|-----------|--------------|-------|
| 1 | BATT_LED1 | Servo Control Board → machine | D801 node | Drive only when S801 is in BATT |
| 2 | BATT_LED2 | Servo Control Board → machine | D802 node | Drive only when S801 is in BATT |
| 3 | BATT_LED3 | Servo Control Board → machine | D803 node | Drive only when S801 is in BATT |
| 4 | BATT_LED4 | Servo Control Board → machine | D804 node | Drive only when S801 is in BATT |
| 5 | BATT_LED5 | Servo Control Board → machine | D805 node | Drive only when S801 is in BATT |
| 6 | S801_BATT | machine → Servo Control Board | S801 BATT-position pole | MCU enable flag; polarity pending bench verification |
| 7 | GND | shared | LED-board ground | Return/reference |

---

## Verification checklist before finalizing

1. **CP304 cavity pinout** — continuity-check pins 1/2/3 on the physical socket to
   confirm the left/right (output/input) orientation matches this sheet.
2. **Q601 identity and base voltage range** — identify package/marking and measure
   during playback. The output stage is committed PWM + RC + NPN level shift; the
   measurement confirms R9 sizing and PWM-to-speed polarity.
3. **RV601/602/603 wiper max voltage** — over full travel. >3.3V anywhere → add a
   divider ahead of the ADC.
4. **CX20084 pad numbers** — confirm 7 / 13 / 14 / 15 / 16 on the actual board (G-12)
   before committing the J2 harness.
5. **FG swing voltage** — at pad 13 (AC). Sets R3/R4 divider values for J2 pin 1.
6. **Q601 health** — diode-mode test the device after identifying its package and
   pinout. Replace with an electrically suitable part only after the actual SMD
   footprint and ratings are known.
7. **S801/LED harness** — confirm S801_BATT polarity, D801-D805 polarity/current
   limiters, and whether CX10043 LED outputs are high-Z/released in BATT mode before
   connecting MCU LED outputs.

## Notes

- Suggested refdes (J1/J2/J3) — remap to your schematic's numbering as needed.
- CN301 on the original sheet is drawn ⊕–c–⊖ (negative-center barrel jack): the
  documented reverse-polarity hazard. Both DSR-1 variants eliminate it.
