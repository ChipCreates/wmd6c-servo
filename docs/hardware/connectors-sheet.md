# DSR-1 Connectors Sheet — WM-D6C Machine Interface

## Reference version

This sheet targets the **former-type** WM-D6C servo board (1-611-494-11/-12,
1-651-545-12): servo IC **IC601 = CX20084**, motor transistor **Q601 = 2SB733**.
This matches the unit and the module design docs. Do **not** take pin detail from
the Supplement-4 sheets (pages 36–41) — those are the new-type -14 board (CX-069A),
a different circuit.

## Principle: the original machine has no single servo connector

The original WM-D6C uses point-to-point colored flying leads plus a small 3-terminal
DC-DC daughterboard (CP304). The "8-pin JST J1 carrying all signals" in the module
datasheet is a consolidation abstraction — it does not exist as one physical part,
and it omits B+3. This sheet replaces that abstraction with the three connections
the module actually makes to the machine.

Two things are **not** module connectors — they stay on the machine, and the module
only drives into their nodes:

- **M901 (capstan motor)** — remains driven by Q601 / Q703 / Q704 on the main board.
  The module's only influence on the motor is (a) the **B+3 rail** it supplies
  (J1 pin 3) and (b) the **Q601 base drive** it produces (J2 pin 2 → CX20084 pad 15).
  The motor's two leads never come to the module.
- **Optical FG sensor (PH701, GP2S22AB)** — remains on the machine with its LED
  supply and B+1 pull-up. The module taps only its **output node** at CX20084
  pad 13 (J2 pin 1). The sensor's four leads never come to the module.

---

## J1 — Power / DC-DC cavity connector (reuses the CP304 socket)

3-terminal, confirmed directly from the original DC-DC board schematic. Orientation
note: on the sheet the **output (10.8V) is on the left (pin 3)** and the
**input (6V) is on the right (pin 1)** — go by pin number, not physical side.

| Pin | Net | Direction | Machine node | Notes |
|-----|------------|-----------------|--------------------------------|-------|
| 1 | B+1 (6V) | module → machine | CP304 cavity pin 1 | Powers machine logic/audio. Machine-side filter C317 220µF 10V. In the original this was the 6V *input* to CP304; the module now *supplies* it. |
| 2 | GND | shared | CP304 cavity pin 2 | Common return. |
| 3 | B+3 (10.8V) | module → machine | CP304 cavity pin 3 → Q703/Q704 → M901 | Motor rail. MT3608 boost output replaces CP304 here. Machine-side filter C316 100µF 16V. |

Series Schottky (1N5819) on the module's B+1 feed is recommended (≈0.3V drop →
machine sees ~5.7V, within spec) to block reverse current on power sequencing.

---

## J2 — Servo flying leads (to the removed CX20084 / IC601 pads)

IC601 is desoldered; these leads land on its cleaned pads at board position **G-12**.
Pad numbers below are from the module docs — **verify on the physical board before
soldering.**

| Pin | Net | Direction | CX20084 pad | Conditioning on module | Notes |
|-----|-----------|-----------------|-------------|------------------------|-------|
| 1 | FG_RAW | machine → module | pad 13 | R3/R4 divider + BAT54 clamps + 10pF | Open-collector, pulled to B+1 on machine. Swings ~0 to ~B+1. Confirm swing → sets R3/R4. |
| 2 | Q601_BASE | module → machine | pad 15 | Option A: DAC→R5→base, R6 pullup. Option B: PWM→RC→level-shift NPN | Drive to Q601 (2SB733, PNP). Option chosen by measured base-voltage range. |
| 3 | MOTOR_EN | machine → module | pad 7 | R10/R11 divider to GPIO | ~4.4V in playback, ~0.1–0.5V at auto-off. Divider → ~3.03V at PA5. |
| 4 | GND | shared | pad 14 | — | Servo ground reference. |
| 5 | (VDD / B+1) | n/c or tie | pad 16 | — | Original CX20084 supply. Module already supplies B+1 on J1 — leave open or tie to B+1, do not double-drive. |

---

## J3 — Speed trim pots + Speed Tune switch

Wipers read by the module ADC; the pots themselves are unchanged on the machine.
Reference rails (B+1 top, GND bottom) are shared with J1 — only the wipers and the
switch are unique signals here.

| Pin | Net | Direction | Machine node | Notes |
|-----|-------------|-----------------|----------------------------------|-------|
| 1 | RV601_WIPER | machine → module | RV601 wiper (47kΩ cermet, base speed) | ADC. Verify wiper ≤3.3V over full travel; add divider if not. |
| 2 | RV602_WIPER | machine → module | RV602 wiper (20kΩ carbon, Speed Tune) | ADC. Verify ≤3.3V. |
| 3 | RV603_WIPER | machine → module | RV603 wiper (47kΩ cermet, range) | ADC. Verify ≤3.3V. |
| 4 | S601 | machine → module | Speed Tune slide switch | GPIO (PA7). |
| 5 | B+1 ref | module → machine | pot top ends | Shared with J1 pin 1; pots reference this. |
| 6 | GND | shared | pot bottom ends | Shared with J1 pin 2. |

---

## CX20084 (IC601) pad quick reference

The module's entire servo interface lands on these pads after the IC is removed:

| Pad | Function | Module net | Module connector |
|-----|----------------------|------------|------------------|
| 7 | Motor enable input | MOTOR_EN | J2 pin 3 |
| 13 | FG signal input | FG_RAW | J2 pin 1 |
| 14 | Ground | GND | J2 pin 4 |
| 15 | Motor drive output | Q601_BASE | J2 pin 2 |
| 16 | Supply (VDD / B+1) | B+1 | J2 pin 5 (optional) |

---

## Verification checklist before finalizing

1. **CP304 cavity pinout** — continuity-check pins 1/2/3 on the physical socket to
   confirm the left/right (output/input) orientation matches this sheet.
2. **Q601 base voltage range** — measure during playback. ≤3.3V → Option A (DAC).
   >3.3V → Option B (PWM + level shift). Decides the J2 pin 2 output stage.
3. **RV601/602/603 wiper max voltage** — over full travel. >3.3V anywhere → add a
   divider ahead of the ADC.
4. **CX20084 pad numbers** — confirm 7 / 13 / 14 / 15 / 16 on the actual board (G-12)
   before committing the J2 harness.
5. **FG swing voltage** — at pad 13 (AC). Sets R3/R4 divider values for J2 pin 1.
6. **Q601 health** — diode-mode test B-E and B-C (~0.6–0.7V each). Replace
   (2SA1015 / BC327-40 / MMBT3906) if open, since it may share the reverse-polarity
   damage.

## Notes

- Suggested refdes (J1/J2/J3) — remap to your schematic's numbering as needed.
- CN301 on the original sheet is drawn ⊕–c–⊖ (negative-center barrel jack): the
  documented reverse-polarity hazard. Both module variants eliminate it.
