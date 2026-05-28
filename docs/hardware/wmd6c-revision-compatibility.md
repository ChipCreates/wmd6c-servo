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

## 2. Current Compatibility Position

**Current primary target:** Sony WM-D6C / TC-D6C Ver. 1.1 service-manual servo circuit.

| Machine / board | Current status |
|---|---|
| WM-D6C / TC-D6C Ver. 1.1 new servo circuit | Primary reference target |
| Earlier WM-D6C servo-circuit revisions | Not assumed compatible |
| Other WM-D6C regional/suffix variants | Not assumed compatible until checked |
| WM-D6 | Future variant candidate |
| WM-D3 / WM-D3C | Future variant candidate |
| TC-D5M and related Sony professional machines | Future variant candidate |

The project may eventually support multiple variants, but Rev A should target one known reference circuit first.

---

## 3. Governing References

Compatibility work is governed by:

1. **Sony WM-D6C / TC-D6C Service Manual, Ver. 1.1, 2001.06**
   - Primary reference for the currently targeted servo circuit, board layout, service notes, and documented servo-circuit change.

2. **Physical-unit inspection**
   - Required for board revision, component presence/absence, trace routing, prior repairs, and installed circuit differences.

3. **Bench measurements**
   - Required for actual FG behavior, motor-control behavior, speed-control behavior, and power-path behavior.

The service manual is the reference starting point. The actual unit on the bench is the final authority.

---

## 4. Known Unit Log

Each known test unit should be listed here.

### Unit 001

| Field | Value |
|---|---|
| Model | Sony WM-D6C |
| Serial number | 72795 |
| Serial-number evidence | Battery-compartment label photo |
| Exterior condition | Pending full documentation |
| Board revision | Pending internal inspection |
| Servo circuit revision | Pending schematic comparison |
| Power path condition | Pending inspection |
| CN301 condition | Pending inspection |
| CP304 condition | Pending inspection |
| Prior repairs/modifications | Unknown |
| Current DSR-1 role | Primary candidate for measurement and Rev A validation |

The serial number identifies the machine, but it does not establish board revision. Internal photographs and circuit comparison are required.

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
| IC601 / CX20084 | Pending | |
| IC701 | Pending | |
| X701 | Pending | |
| CP304 | Pending | |
| CN301 | Pending | |
| M901 | Pending | |
| FG901 | Pending | |
| Q601 | Pending | |
| Q603–Q605 | Pending | |
| RV601 | Pending | |
| RV602 | Pending | |
| RV603 | Pending | |
| S601 | Pending | |

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

Questions:

- Can DSR-1 use DAC output?
- Is PWM/level shift required?
- Does the motor-control node exceed MCU voltage range?
- Does this revision change motor-drive polarity or operating point?

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

1. Which main board revision is installed?
2. Does the unit match the Ver. 1.1 new servo circuit?
3. Is IC601 original, replaced, failed, or missing?
4. Is CP304 original and functional?
5. Is CN301 original, damaged, or modified?
6. What is the FG901 waveform?
7. What is the Q601/motor-control operating voltage?
8. Are RV601/RV602/RV603 ADC-safe?
9. What is S601 logic polarity?
10. Where can USB-C physically exit the case?
11. Can USB-C be installed without weakening the shell or interfering with controls?
12. Does USB ground affect audio/servo behavior?
13. Can this unit serve as the Rev A reference build?

---

## 15. Documentation Table

As compatibility work proceeds, update this table.

| Unit | Serial | Board revision | Servo revision | Power path | USB-C mechanical status | DSR-1 status |
|---|---:|---|---|---|---|---|
| Unit 001 | 72795 | Pending | Pending | Pending | Pending | Candidate |

---

## 16. Design Rule

Do not write “compatible with all WM-D6C units.”

Use one of these phrases instead:

- “Primary target: WM-D6C / TC-D6C Ver. 1.1 new servo circuit.”
- “Compatibility with earlier board revisions is pending inspection and measurement.”
- “Variant support requires schematic comparison, harness mapping, bench measurements, and physical validation.”

Compatibility is earned per board/revision, not assumed from the model name.
