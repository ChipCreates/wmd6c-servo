# Power Rev A — Required Changes

**Project:** DSR-1 / wmd6c-servo
**File:** `docs/review/power-rev-a-findings.md`
**Status:** Review against requirement: barrel input 5 / 6 / 9 V, polarity-agnostic,
protected; must deliver 6.0 V B+1, 10.8 V B+3, 3.3 V.
**Scope:** Only deltas from the current `Power input zone` and `Power management`
sheets. Wiring-completion items that ERC will catch after the connectors sheet are
out of scope except where they are genuine design/topology errors.

---

## 1. What exists today

**Power input zone**
- USB-C PD front end: J1 (USB-C), U1 (IP2721), D1 (USBLC6 ESD), CC1/CC2 routed,
  output net `9V_PD`.
- Barrel input: J2 with a 4-diode Schottky **full bridge** (D2/D3 cathodes = +rail,
  D4/D5 anodes = −rail). Polyfuse F1, TVS D6 (SMBJ12A).
- Regulator: **U3 = AP63203WU-7** with divider R1 = 649k / R2 = 100k, inductor L1,
  bootstrap C3, output cap C4 → net `B+1`.

**Power management**
- Boost: **U2 = MT3608**, L2, D7 (SS14), divider R3 = 169k / R4 = 10k, output cap
  C8 = 47 µF/16 V → net `B+3`.
- LDO: **U4 = MCP1700-3302**, C6/C7/C9/C10 → net `+3.3V`.

---

## 2. Required changes — Power input zone

### 2.1 Replace U3 with a buck-boost  *(core change)*
- **Current:** AP63203WU-7 — a **fixed 3.3 V buck**. A buck cannot reach 6 V from a
  5 V or 6 V input, and this part is not adjustable regardless.
- **Change:** **TPS63070** (adjustable buck-boost, 2–16 V in, 0.8 V FB ref, 2 A).
  It crosses through unity automatically — boosts the 5/6 V cases, bucks the 9 V
  case — holding 6.0 V across the whole input range.
- **Implications:**
  - New footprint (15-pin VQFN, not SOT-23-6) and a redrawn converter block.
  - The single inductor now connects **between the TPS63070's two switch pins
    (L1/L2)**, not SW→VOUT as in a buck. L1's placement/connection changes.
  - Add input/output caps per the TPS63070 datasheet; EN has a precise 0.8 V
    rising threshold (use an RC for soft-start; do not leave floating).
- **Keep:** R1 = 649k / R2 = 100k. At the TPS63070's 0.8 V reference,
  0.8 × (1 + 649/100) = 6.0 V — the values carry over unchanged.

### 2.2 Fix the feedback divider bottom leg
- **Current:** R2 is wired B+1 → GND. The FB node sees only R1 from B+1 with no leg
  to ground, so there is no division.
- **Change:** move R2's top terminal onto the **FB node** (junction with R1 and the
  regulator FB pin). Required for any adjustable regulator.

### 2.3 Separate the rectified positive rail from GND
- **Current:** the bridge negative rail carries **both** a `GND` and a `9V_PD`
  label on one net (they are shorted), and the regulator VIN is not on `9V_PD`.
- **Change:** bridge − rail = `GND` only. Bridge + rail (through F1) = the raw input
  net (`9V_PD` / call it `VRAW`), routed to the regulator VIN. Remove the stray
  `9V_PD` label from the GND rail.

### 2.4 Put the polyfuse in series, not across the rails
- **Current:** F1 connects the + rail to the − rail (across the bridge output).
- **Change:** F1 in **series** with the positive rectified rail feeding VRAW
  (or in series ahead of the bridge). It must not bridge + to −.

### 2.5 Resize the TVS
- **Current:** D6 = SMBJ12A, which clamps around ~20 V — above the TPS63070's 16 V
  absolute max, so a transient could exceed the regulator before the TVS clamps
  hard. Netlist also shows both D6 pins on one node.
- **Change:** use an **SMBJ10A/11A-class** part (standoff above the 9 V worst case,
  clamp below 16 V), placed rail-to-GND (one pin on VRAW, one on GND) on the
  rectified side (single polarity → unidirectional is correct).

---

## 3. Required changes — Power management

### 3.1 Correct the MT3608 boost topology  *(verify against symbol pinout)*
- **Current (from netlist):** FB (pin 3) tied to the inductor; SW (pin 1) tied to
  the R3/R4 divider; D7 reversed (cathode→inductor, anode→B+3); the inductor's
  input side is not on B+1.
- **Change to the standard boost:**
  - B+1 → inductor L2 → SW node.
  - SW node = MT3608 SW pin **and** D7 **anode**.
  - D7 **cathode** → B+3 (with output cap C8 and the divider top R3).
  - R3/R4 divider **midpoint** → FB pin; R4 bottom → GND.
- If the MT3608 symbol's pin numbering is nonstandard, fix the symbol rather than
  the wiring — either way the fabricated netlist must match the chip's pads.
- **Keep:** R3 = 169k / R4 = 10k (→ 10.74 V) and C8 = 47 µF/16 V.

---

## 4. Confirmed correct — do not change

- R1 / R2 = 649k / 100k → 6.0 V (valid on the TPS63070).
- R3 / R4 = 169k / 10k → 10.8 V B+3.
- C8 = 47 µF / 16 V output cap (DC-bias-derating correction already applied).
- MCP1700 (U4) path: B+1 → VI, VO → +3.3V, with C6/C7/C9/C10 decoupling.
- USB-C PD front end (J1, U1, D1): unchanged. Its 9 V output simply joins VRAW at
  the buck-boost input.
- The barrel bridge itself (D2–D5) is the cross-polarity protection and stays.
  Schottky is fine — the ~0.7 V bridge drop is now harmless because the buck-boost
  works down to 2 V input.

---

## 5. Open decisions

- **Over-voltage from a wrong adapter.** The bridge handles polarity, not voltage.
  A 12 V adapter survives (16 V rating); a 19 V laptop adapter does not, and a TVS
  alone will not survive sustained 19 V. If that misuse is plausible, add an input
  **OVP load switch / eFuse** that disconnects above ~10–11 V.
- **Bridge type.** Keep the Schottky bridge (recommended — simple, robust) vs. an
  ideal-diode MOSFET bridge (lower loss, more parts). No need to change unless
  efficiency on wall power matters.

---

## 6. Net effect

With 2.1 + 2.2 in place, the barrel input delivers a regulated 6.0 V B+1 across all
of 5 / 6 / 9 V and either polarity. 3.1 restores the 10.8 V B+3 that replaces CP304.
Everything in §4 stays as-is.
