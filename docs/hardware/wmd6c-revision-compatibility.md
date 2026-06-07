# WM-D6C Revision Compatibility

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/hardware/wmd6c-revision-compatibility.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Target machine identification, Sony servo-circuit revision differences, compatibility boundaries, variant requirements, and physical-unit documentation.

---

## 1. Purpose

This document defines the compatibility scope for DSR-1.

The Sony WM-D6C existed across multiple production periods, service revisions, and possible board/circuit changes. DSR-1 must not assume that every WM-D6C has the same servo circuit, power path, board layout, or component placement.

This file exists to prevent an unsafe assumption:

> “WM-D6C” is not precise enough for a hardware replacement module.

DSR-1 compatibility must be tied to actual circuit revisions, board observations, Sony service documentation, and bench measurements.

---

## 2. Compatibility Position and Production Run Scope

DSR-1 is designed for the **full WM-D6C / TC-D6C production run (1984–2002)**,
not for a single serial range or service revision. Every unit in the production
run is a potential installation target. Compatibility is earned per board revision
through measurement and validation, not assumed from the model name.

### 2.1 WM-D6C Circuit Families

Two distinct servo circuit families exist across the production run:

| Family | Servo IC | Motor drive | Approx. years | Governing reference |
|---|---|---|---|---|
| **Ver. 1.0** | CX20084 | Single Q601 motor-control path; exact device/package depends on board construction | 1984 – mid 2001 | `fb4872.pdf` |
| **Ver. 1.1** | CX-069A | Q601–Q605 (five transistors + JFET) | mid 2001 – 2002 | `sony_wm-d6c_tc-d6c_ver-1.1.pdf` |

The change is documented in the Ver. 1.1 service manual revision history as
"Change of servo circuit" under ECN-WMA00831, dated 2001.05.

Both families share: X701 34.7 kHz crystal, IC701 MSM58141RS divider, M901 motor,
FG901 optical sensor, RV601/602/603 speed pots, S601 switch, and CP304 DC-DC.

**To identify family before installation:** open the machine and read the IC at
position IC601. CX20084 = Ver. 1.0. CX-069A = Ver. 1.1.

### 2.2 Ver. 1.0 PCB Construction Sub-generations

Within Ver. 1.0, two PCB construction generations exist:

| Sub-generation | Approx. years | Construction |
|---|---|---|
| Early through-hole | 1984 – ~1994 | Through-hole PCB |
| Later SMD | ~1994 – 2001 | SMD main board |

Both sub-generations have the same CX20084-based servo circuit and the same DSR-1
electrical interface. The difference is mechanical: harness routing, test-point
location, and connector placement differ between boards. The Servo Control Board
electrical interface does not change — installation harness documentation covers both.

The 1994 service manual Supplement-2 documents the SMD board transition. Earlier
supplements (1985, 1994) do not change the servo circuit.

### 2.3 Current DSR-1 Status by Circuit Family

| Circuit family | DSR-1 status |
|---|---|
| Ver. 1.0 CX20084 boards (1984 – mid 2001) | **Primary Rev A target** |
| Ver. 1.1 CX-069A boards (mid 2001 – 2002) | **In scope — planned variant, post Rev A** |
| WM-D6 | Future variant candidate |
| WM-D3 / WM-D3C | Future variant candidate |
| TC-D5M and related Sony professional machines | Future variant candidate |

### 2.4 Known Serial Range Landmarks (WM-D6C)

Collected from community data. Uncertainty in exact transition serial numbers is
normal — these are observed data points, not guaranteed changeover boundaries.

| Serial range | Event |
|---|---|
| Before ~71236 | "Pointy" early amorphous head |
| ~71236 – ~72263 | Head change: pointy → rounded amorphous 35711 |
| After ~72263 | Rounded amorphous 35711 head (HRP901 part 35711) |
| ~114374 – ~117947 | Dolby sticker color change (spring 1988) |
| ~124264 | Documented purchase date: April 1988 |
| ~post-1994 | SMD board revision (Supplement-2) |
| ~post-2001 | CX-069A servo circuit (Ver. 1.1) |

---

## 3. Governing References

Compatibility work is governed by:

1. **Sony WM-D6C / TC-D6C Service Manual, original edition (`fb4872.pdf`)**
   - Governing reference for Ver. 1.0 boards (1984 – mid 2001, CX20084 servo circuit).
   - Contains the original through-hole board layout, CX20084 schematic, and tape speed adjustment procedure.

2. **Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1, 2001.06 (`sony_wm-d6c_tc-d6c_ver-1.1.pdf`)**
   - Documents the servo circuit change (ECN-WMA00831) to CX-069A.
   - Governing reference for Ver. 1.1 boards (mid 2001 – 2002).
   - Higher-quality schematic diagrams and complete SMD parts list useful as reference even for Ver. 1.0 servicing.

3. **Physical-unit inspection**
   - Required for board revision, component presence/absence, trace routing, prior repairs, and installed circuit differences.

4. **Bench measurements**
   - Required for actual FG behavior, motor-control behavior, speed-control behavior, and power-path behavior.

The service manual is the reference starting point. The actual unit on the bench is the final authority.

If governing references conflict, the order of authority is:
1. Bench measurement on the actual target unit
2. Appropriate service manual for the confirmed board revision
3. STM32G0C1KCU6 package/device documentation
4. DSR-1 design assumptions

---

## 4. Known Unit Log

Each known test unit should be listed here.

### Unit 001

| Field | Value |
|---|---|
| Model | Sony WM-D6C |
| Serial number | 72795 |
| Serial-number evidence | Battery-compartment label photo |
| Estimated manufacture date | Late 1986 – early 1987 (most likely 1987) |
| Head type | Rounded amorphous, part number 35711 (HRP901) |
| Board construction era | Later SMD construction |
| Observed PCB marking | **C11-494-12** |
| Servo IC | **Surface-mount CX20084 confirmed** (Ver. 1.0 board) |
| Motor drive transistor | **Q601 exact part/package pending physical confirmation** |
| Crystal reference | X701 34.7 kHz + IC701 MSM58141RS (present, Ver. 1.0) |
| Board revision | C11-494-12 observed; detailed harness/pad mapping pending photos |
| Power path condition | Pending inspection |
| CN301 condition | Pending inspection |
| CP304 condition | Pending inspection |
| Prior repairs/modifications | Unknown |
| Current DSR-1 role | Primary Rev A reference unit |
| Governing service manual | `fb4872.pdf` (Ver. 1.0, original edition) |

**Dating basis for serial 72795:**
Serial 72795 falls approximately 500 units above the documented head change from
"pointy" to rounded amorphous (transition between ~71236 and ~72263). It is well
below the spring 1988 sticker-color change at ~114374–117947. A known WM-D6C with
serial 124264 was purchased April 1988, placing 72795 significantly earlier.
The rounded amorphous 35711 head (HRP901) has been physically confirmed on this unit.

---

## 5. Why Revision Compatibility Matters

DSR-1 touches or may replace signals in sensitive parts of the WM-D6C:

- FG feedback from the motor/capstan system,
- motor-control drive path,
- original servo IC area,
- speed adjustment network,
- Speed Tune controls,
- external power input,
- DC-DC / support power circuitry,
- ground references,
- and possibly case/connector geometry for USB-C.

A small board revision change may affect:

- where DSR-1 connects,
- what voltage a node carries,
- whether a node is safe for STM32 input,
- whether a component must be removed,
- whether the harness pinout is correct,
- whether the motor-control output stage is safe,
- whether USB-C placement is mechanically possible,
- and whether the original adjustment procedure still applies.

Therefore, compatibility must be proven, not assumed.

---

## 6. Compatibility Categories

DSR-1 should classify compatibility in stages.

| Category | Meaning |
|---|---|
| Unknown | Machine has not been inspected or mapped |
| Candidate | Appears related but not electrically verified |
| Mapped | Schematic and connection points identified |
| Measured | Critical voltages/waveforms measured |
| Bench-compatible | Works on bench setup or simulated transport |
| Transport-validated | Installed and tested in a real machine |
| Release-supported | Documented installation and validation complete |

No machine should be called “supported” until it reaches release-supported status.

---

## 7. Required Compatibility Evidence

A compatible machine/revision must have:

| Evidence | Required? |
|---|---:|
| Model identification | Yes |
| Serial number or serial range | Yes, if available |
| Board photos | Yes |
| Servo-circuit reference comparison | Yes |
| Power-path comparison | Yes |
| FG source identified | Yes |
| Motor-control node identified | Yes |
| RV601/RV602/RV603 behavior identified | Yes |
| S601 behavior identified | Yes |
| CN301 / battery / CP304 behavior identified | Yes |
| Ground reference identified | Yes |
| Harness/interface map | Yes |
| Bench measurements | Yes |
| Firmware configuration notes | Yes |
| Real-machine validation | Required for supported status |

---

## 8. Board and Circuit Inspection Checklist

Before connecting DSR-1 to any WM-D6C, document the following.

### 8.1 Exterior Identification

- Model marking.
- Serial number.
- Region/suffix markings, if present.
- Battery-compartment label condition.
- External DC jack condition.
- Evidence of previous repair or modification.

### 8.2 Internal Board Photographs

Required photo areas:

| Area | Purpose |
|---|---|
| Full main board | General revision and condition |
| IC601 / CX20084 area | Servo circuit mapping |
| CP304 area | Power-support mapping |
| CN301 area | External input mapping |
| Motor wiring / M901 | Motor and FG identification |
| RV601 | Base speed adjustment |
| RV602 | Speed Tune control |
| RV603 | Speed Tune range |
| S601 | Speed Tune switch |
| Q601 and motor-drive transistors | Motor-control output mapping |
| Ground/chassis points | Reference and shielding strategy |

Photos should be sharp, well-lit, and taken before any components are removed.

### 8.3 Component Presence Check

Record whether the following are present and visually original:

| Reference | Present? | Notes |
|---|---:|---|
| IC601 | Confirmed | Surface-mount CX20084 on the observed C11-494-12 board |
| IC602 | Pending photo | Present on Ver. 1.1 only (NJM2904V); should be absent on Ver. 1.0 |
| IC701 | Pending | MSM58141RS divider — present on both versions |
| X701 | Pending | 34.7 kHz crystal — present on both versions |
| CP304 | Pending | DC-DC boost module |
| CN301 | Pending | External DC jack — centre-negative polarity |
| M901 | Pending | Capstan motor |
| FG901 | Pending | Optical interrupter (GP2S22AB) |
| Q601 | Pending photo | Identify package/marking on the C11-494-12 SMD board before finalizing installation docs |
| Q602–Q605 | Pending photo | Present on Ver. 1.1 only; should be absent on Ver. 1.0 |
| RV601 | Pending | 47kΩ cermet trimmer, rear panel |
| RV602 | Pending | 20kΩ carbon slider, front panel Speed Tune |
| RV603 | Pending | 47kΩ cermet trimmer |
| S601 | Pending | Speed Tune slide switch |

---

## 9. Servo Compatibility Requirements

A revision is servo-compatible only if the following are mapped and measured.

### 9.1 FG Feedback

Required:

- FG source identified.
- Connection point identified.
- Waveform captured.
- Voltage range measured.
- Frequency at correct speed measured.
- Startup and stop behavior documented.

Questions:

- Is FG safe for MCU input after conditioning?
- Does it need a comparator or Schmitt input?
- Does this revision alter FG routing or loading?
- Is FG accessible without destructive modification?

### 9.2 Motor-Control Node

Required:

- Motor-control node identified.
- Q601 / surrounding network mapped.
- Voltage range measured.
- Current or loading requirement understood.
- Safe inactive state identified.
- Original components to remove/retain documented.

**Ver. 1.0 boards (CX20084) — output topology settled:** DSR-1 uses PWM + RC filter
+ NPN level-shift (TIM3_CH1 PA6 → Q_LS MMBT3904). Direct DAC drive is not used.
On the primary C11-494-12 SMD board, Q601 exact part/package and base voltage during
playback still require bench measurement to confirm R9 sizing and PWM-to-speed sign.

**Ver. 1.1 boards (CX-069A) — not yet characterized:** Q601 is a 2SC1623 NPN
small-signal transistor driving a five-transistor network (Q601–Q605 including
a JFET Q603). Interface point and output stage design require separate
characterization before Ver. 1.1 variant support can be added.

### 9.3 Speed Controls

Required:

- RV601 wiper mapped and measured.
- RV602 wiper mapped and measured.
- RV603 wiper mapped and measured.
- S601 logic mapped and measured.
- Original speed-adjustment behavior documented.

Questions:

- Are all wipers ADC-safe?
- Is scaling required?
- Does S601 gate RV602 electrically or logically?
- Does firmware need debounce or filtering?

---

## 10. Power Compatibility Requirements

Because DSR-1 includes power modernization, compatibility must include the power path.

Required:

- CN301 polarity and wiring confirmed.
- Battery rail behavior measured.
- CP304 role documented.
- Original external-power behavior understood.
- Startup current measured.
- Steady-play current measured.
- Motor transient current measured.
- Ground return behavior understood.

Questions:

- Does the target unit have original CN301 wiring?
- Has CN301 been repaired, modified, or damaged?
- Is CP304 original, replaced, failing, or missing?
- Should DSR-1 replace, bypass, or support CP304?
- Can USB-C power safely replace or supplement the original path?
- Does battery operation remain supported?

A machine is not fully compatible with DSR-1 unless both servo and power behavior are understood.

---

## 11. USB-C Mechanical and Electrical Compatibility

USB-C adds new compatibility questions not present in the original Sony design.

Required:

- Candidate connector location identified.
- Case modification requirement documented.
- Cable insertion clearance checked.
- PCB/mechanical strain reviewed.
- USB shield/chassis strategy reviewed.
- USB service access possible after installation.

Questions:

- Can USB-C be exposed without unacceptable case modification?
- Does connector placement interfere with battery compartment, controls, transport, or service access?
- Is there a path for USB D+/D− routing that avoids motor/audio noise problems?
- Does USB ground connection disturb audio, FG, ADC, or motor behavior?
- Can USB be connected during playback?
- Can USB be connected while the Walkman is off?

---

## 12. Variant Port Requirements

A variant port is required for any machine or board revision that differs materially from the Rev A reference target.

A complete variant directory should live at:

```text
docs/variants/[machine-or-board-revision]/
```

Required files:

```text
docs/variants/[machine-or-board-revision]/
├── schematic-diff.md
├── harness-pinout.md
├── measured-signals.md
├── power-notes.md
├── usb-c-notes.md
└── config-[variant].h
```

### 12.1 `schematic-diff.md`

Must describe:

- differences from the Rev A reference circuit,
- servo IC area differences,
- FG routing differences,
- motor-drive differences,
- speed-control differences,
- power-path differences,
- and any required component removals or jumpers.

### 12.2 `harness-pinout.md`

Must include:

| DSR-1 signal | Target machine connection | Board location | Notes |
|---|---|---|---|
| `FG_IN` | Required | Required | |
| `MOTOR_CTRL` | Required | Required | |
| `RV601_WIPER` | Required | Required | |
| `RV602_WIPER` | Required | Required | |
| `RV603_WIPER` | Required | Required | |
| `S601_SPEED_TUNE` | Required | Required | |
| `RAW_POWER` | Required | Required | |
| `B_PLUS` | If used | Required | |
| `B_PLUS_3` | If used | Required | |
| `GND` | Required | Required | |

### 12.3 `measured-signals.md`

Must include raw measurements for:

- FG waveform,
- FG target at correct speed,
- motor-control voltage range,
- speed-control wiper ranges,
- S601 logic,
- rail voltages,
- motor current,
- USB-connected behavior if installed.

### 12.4 `power-notes.md`

Must include:

- external power behavior,
- battery behavior,
- CP304 behavior,
- protection requirements,
- USB-C power compatibility,
- and backfeed risks.

### 12.5 `usb-c-notes.md`

Must include:

- connector location,
- mechanical modification,
- USB shield/ground behavior,
- data access after installation,
- PD/power behavior,
- and service-mode limitations.

### 12.6 `config-[variant].h`

Must include:

- measured FG target data,
- gain starting points,
- ADC scaling,
- output-stage selection,
- power behavior flags,
- USB/service behavior flags if needed.

---

## 13. Supported Status Requirements

A machine/revision may be marked supported only when:

| Requirement | Status |
|---|---|
| Schematic comparison complete | Required |
| Harness map complete | Required |
| Servo signals measured | Required |
| Power signals measured | Required |
| USB-C/mechanical notes complete | Required if USB-C installed |
| Firmware configuration complete | Required |
| Bench test complete | Required |
| Real transport install complete | Required |
| Sony speed-adjustment procedure completed | Required |
| Wow/flutter or equivalent performance measurement complete | Required |
| Power-fault testing complete | Required |
| USB data/service test complete | Required |
| Documentation/photos complete | Required |

Anything less is a candidate, not a supported variant.

---

## 14. Current Open Questions for Unit 72795

**Confirmed:**
- IC601 = CX20084 (Ver. 1.0 board) ✓
- Q601 exact part/package pending; motor drive NPN level-shift required ✓
- Manufacture date: ~late 1986 – early 1987 ✓
- Head: rounded amorphous 35711 (HRP901) ✓
- Surface-mount board construction; PCB marking C11-494-12 ✓
- Governing manual: `fb4872.pdf` ✓

**Still pending:**
1. Internal board photographs — confirm board layout vs service manual
2. Q601 physical confirmation by reading marking/package on the C11-494-12 board
3. CP304 condition — original and functional?
4. CN301 condition — original, damaged, or modified?
5. FG901 waveform at correct tape speed
6. Q601 base voltage during playback (needed for R9 sizing confirmation)
7. RV601/RV602/RV603 wiper voltage ranges (needed for ADC conditioning decision)
8. S601 logic polarity
9. USB-C mechanical exit point
10. Ground noise behavior with USB connected

---

## 15. Documentation Table

As compatibility work proceeds, update this table.

| Unit | Serial | Board revision | Servo IC | Motor drive | Power path | USB-C status | DSR-1 status |
|---|---:|---|---|---|---|---|---|
| Unit 001 | 72795 | SMD, C11-494-12 | Surface-mount CX20084 ✓ | Q601 package/marking pending | Pending measurement | Pending mechanical | Candidate |

---

## 16. Design Rule

Do not write “compatible with all WM-D6C units” without qualification.

Use one of these phrases instead:

- “Primary Rev A target: WM-D6C / TC-D6C Ver. 1.0 CX20084 boards (1984–mid 2001).”
- “Ver. 1.1 CX-069A boards (mid 2001–2002) are a planned variant, pending separate characterization.”
- “Full production run (1984–2002) is the design scope; each board revision requires individual measurement and validation.”
- “Variant support requires schematic comparison, harness mapping, bench measurements, and physical validation.”

Compatibility is earned per board revision, not assumed from the model name.
