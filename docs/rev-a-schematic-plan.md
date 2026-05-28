# Rev A Schematic Plan

**Project:** DSR-1 / wmd6c-servo  
**File:** `docs/hardware/rev-a-schematic-plan.md`  
**Status:** Draft / pre-Rev A  
**Scope:** Planned KiCad schematic structure, sheet responsibilities, design gates, measurement dependencies, and Rev A schematic acceptance criteria.

---

## 1. Purpose

This document defines the Rev A schematic plan for DSR-1.

The goal is to turn the project architecture into a KiCad schematic that can be reviewed, measured against, fabricated, brought up safely, and revised without ambiguity.

DSR-1 is an integrated subsystem project:

1. capstan servo,
2. power input/protection/support,
3. USB-C data/service,
4. USB-C PD or power-role strategy,
5. firmware update/debug,
6. and measurement/test access.

The schematic must reflect all of those domains. It must not be a servo-only schematic with power and USB added casually afterward.

---

## 2. Schematic Design Principles

Rev A schematic work follows these rules:

1. **No unmeasured Sony-facing node is final.**
2. **Every external input needs protection or a documented reason it does not.**
3. **Every motor-control state must fail safe.**
4. **USB-C data and power must not backfeed Sony rails unintentionally.**
5. **SWD/BOOT recovery must remain accessible.**
6. **The selected STM32 package must be proven by pin allocation before routing.**
7. **Every important signal needs a test point or measurement strategy.**
8. **Power, USB, servo, and analog references must be separated intentionally.**
9. **The schematic must encode the bring-up sequence.**
10. **Unsupported options must be marked DNP or moved out of Rev A.**

---

## 3. Required Source Documents

Before schematic acceptance, these documents must exist and be reviewed:

```text
docs/hardware/wmd6c-interface-contract.md
docs/hardware/power-usb-c-architecture.md
docs/hardware/timebase-decision.md
docs/hardware/wmd6c-revision-compatibility.md
docs/hardware/stm32g0b1-pin-allocation.md

docs/verification/00-rev-a-bringup-checklist.md
docs/verification/01-wmd6c-preinstall-measurements.md
docs/verification/02-fg901-waveform-capture.md
docs/verification/03-motor-drive-characterization.md
docs/verification/04-speed-calibration-procedure.md
docs/verification/05-wow-flutter-test-plan.md
docs/verification/06-power-fault-test-plan.md
docs/verification/07-usb-c-data-pd-test-plan.md
```

The schematic may begin before all measurements are complete, but any unmeasured area must be clearly marked as provisional.

---

## 4. Proposed KiCad Sheet Structure

Recommended schematic hierarchy:

```text
DSR-1.kicad_sch
├── 01_power_input_protection.kicad_sch
├── 02_power_rails.kicad_sch
├── 03_usb_c_data_pd.kicad_sch
├── 04_mcu_stm32g0b1.kicad_sch
├── 05_fg_input_conditioning.kicad_sch
├── 06_motor_control_output.kicad_sch
├── 07_speed_controls_adc.kicad_sch
├── 08_timebase_reference.kicad_sch
├── 09_sony_interface_connector.kicad_sch
└── 10_testpoints_debug_boot.kicad_sch
```

Sheet names may change, but responsibilities should remain separated.

---

## 5. Top-Level Sheet

### 5.1 `DSR-1.kicad_sch`

Responsibilities:

- show system-level sheet hierarchy,
- expose major nets,
- make power/servo/USB boundaries obvious,
- avoid hiding cross-domain dependencies.

Top-level net groups:

| Net group | Examples |
|---|---|
| Sony interface | `FG_IN_RAW`, `MOTOR_CTRL_OUT`, `RV601_WIPER`, `RAW_POWER`, `GND` |
| MCU local | `FG_IN`, `MOTOR_DAC_OUT`, `USB_DP`, `USB_DM`, `SWDIO`, `SWDCLK` |
| Power | `VBUS`, `RAW_POWER`, `+3V3`, `B_PLUS`, `MOTOR_SUPPLY` |
| USB-C | `USB_DP`, `USB_DM`, `CC1`, `CC2`, `USB_SHIELD` |
| Debug/test | `BOOT0`, `NRST`, `TP_*` |

Rules:

- Major domains must be readable from the top sheet.
- No hidden power nets without labels.
- Every inter-sheet net must use consistent naming.

---

## 6. Power Input / Protection Sheet

### 6.1 `01_power_input_protection.kicad_sch`

Responsibilities:

- external DC input, if retained,
- USB-C VBUS input handoff,
- battery/Sony rail interface,
- reverse-polarity protection,
- overcurrent protection,
- overvoltage protection,
- backfeed prevention.

Required blocks:

| Block | Status |
|---|---|
| CN301/protected external input option | Pending |
| USB-C VBUS input protection | Pending |
| Battery/Sony rail interface | Pending |
| Reverse-polarity protection | Pending |
| Overcurrent/fuse element | Pending |
| TVS/clamp strategy | Pending |
| Ideal diode / power mux strategy | Pending |
| Backfeed prevention | Pending |
| Test points | Required |

Design questions:

- Does USB-C replace CN301 or coexist?
- Does DSR-1 accept original external DC?
- Does DSR-1 support battery operation?
- Which source has priority?
- What rail powers the MCU in service-only mode?

Required test points:

- `TP_VBUS`,
- `TP_RAW_POWER`,
- `TP_PROTECTED_INPUT`,
- `TP_GND_POWER`,
- `TP_POWER_FAULT`.

---

## 7. Power Rails Sheet

### 7.1 `02_power_rails.kicad_sch`

Responsibilities:

- generate 3.3 V logic rail,
- generate or condition motor/support rail if required,
- provide rail sensing,
- provide power-good/fault status,
- ensure safe startup and shutdown.

Required blocks:

| Block | Status |
|---|---|
| 3.3 V regulator | Required |
| Motor/support rail generation or interface | Pending measurement |
| Rail sensing dividers | Recommended |
| Power-good signals | Recommended |
| Fault outputs | Recommended |
| Bulk/local decoupling | Required |
| Analog reference filtering | Required if ADC/DAC sensitive |

Rules:

- Motor/switching currents must not share sensitive ADC/FG return paths.
- Regulator dropout and thermal behavior must be reviewed.
- Rail-sense dividers must not overvoltage ADC pins.
- DSR-1 must not backfeed Sony rails when unpowered.

Required test points:

- `TP_3V3`,
- `TP_MOTOR_SUPPLY`,
- `TP_B_PLUS`,
- `TP_B_PLUS_3`,
- `TP_GND_ANALOG`,
- `TP_GND_DIGITAL`.

---

## 8. USB-C Data / PD Sheet

### 8.1 `03_usb_c_data_pd.kicad_sch`

Responsibilities:

- USB-C connector,
- USB 2.0 FS D+/D− routing,
- ESD protection,
- VBUS sense/protection,
- CC1/CC2 handling,
- native UCPD or external PD controller,
- USB shield strategy.

Required blocks:

| Block | Status |
|---|---|
| USB-C receptacle | Required |
| D+/D− ESD protection | Required |
| VBUS ESD/protection | Required |
| CC1/CC2 strategy | Required |
| Native UCPD or external PD controller | Open decision |
| VBUS sense | Required |
| USB shield termination | Required decision |
| Service/data path to MCU | Required |

Native UCPD requirements:

- CC1/CC2 routed to valid MCU pins,
- firmware responsibility documented,
- default no-contract behavior safe.

External PD controller requirements:

- controller selected,
- default behavior documented,
- status/control pins assigned,
- output rail protected.

Rules:

- USB shield must not connect accidentally.
- USB ground must not inject motor/audio noise without review.
- USB attach must not enable motor output by default.
- USB D+/D− must be routed as USB signals, not casual GPIO.

Required test points:

- `TP_VBUS`,
- `TP_CC1`,
- `TP_CC2`,
- `TP_USB_DP`,
- `TP_USB_DM`,
- `TP_USB_SHIELD`,
- `TP_PD_STATUS`, if present.

---

## 9. MCU Sheet

### 9.1 `04_mcu_stm32g0b1.kicad_sch`

Responsibilities:

- STM32G0B1 MCU,
- decoupling,
- reset,
- BOOT0,
- SWD,
- clock/timebase pins,
- all assigned I/O,
- pin labels matching `stm32g0b1-pin-allocation.md`.

Required blocks:

| Block | Status |
|---|---|
| MCU symbol/package | Pending final part |
| VDD/VDDA decoupling | Required |
| VREF+/analog reference handling | Required if used |
| NRST | Recommended |
| BOOT0 access | Required |
| SWDIO/SWDCLK | Required |
| USB pins | Required |
| UCPD pins, if native PD | Conditional |
| ADC inputs | Required |
| DAC/PWM output | Required |
| FG input | Required |
| Timebase option | Pending decision |

Rules:

- MCU package must match final pin-allocation document.
- Do not use family-level assumptions; verify exact package pins.
- SWD and BOOT access must remain available during Rev A.
- Reset/default pin states must not drive motor unsafely.

Required test points:

- `TP_SWDIO`,
- `TP_SWDCLK`,
- `TP_NRST`,
- `TP_BOOT0`,
- `TP_3V3_MCU`,
- selected spare GPIO if available.

---

## 10. FG Input Conditioning Sheet

### 10.1 `05_fg_input_conditioning.kicad_sch`

Responsibilities:

- accept raw FG signal from Sony motor/FG source,
- protect MCU input,
- condition waveform for timer capture,
- reject overvoltage/noise,
- support probing.

Possible blocks:

| Block | Use if |
|---|---|
| Series resistor/clamp | Mild protection sufficient |
| Divider | FG amplitude exceeds MCU range |
| Schmitt buffer | Edges are slow/noisy |
| Comparator | Analog waveform or shifting threshold needed |
| RC filter | High-frequency noise/ringing present |
| Pull-up/down | Source requires defined idle state |

Required input:

- result of `02-fg901-waveform-capture.md`.

Rules:

- Do not finalize conditioning without waveform measurements.
- Do not assume direct MCU input is safe.
- FG input must remain safe if Sony is powered and DSR-1 is unpowered.
- FG input must not load the original circuit significantly.

Required test points:

- `TP_FG_RAW`,
- `TP_FG_CONDITIONED`,
- `TP_FG_GND`.

---

## 11. Motor-Control Output Sheet

### 11.1 `06_motor_control_output.kicad_sch`

Responsibilities:

- generate motor-control correction signal,
- protect Sony motor-control node,
- protect STM32,
- provide safe reset/default state,
- support DAC or PWM/level-shift options if needed.

**Committed output architecture for Ver. 1.0 CX20084 boards:**

PWM + RC filter + NPN level-shift (TIM3_CH1 PA6 → R7/C8 → Q_LS MMBT3904 →
R9 pullup to B+1 → Q601 base). Q601 (2SB1013 PNP) emitter is at B+3 (10.8V);
direct DAC drive cannot reach the required base voltage range.

Ver. 1.1 CX-069A board motor output architecture is not yet characterized and
is out of scope for Rev A.

Required input:

- result of `03-motor-drive-characterization.md`.

Rules:

- Motor output must be safe with MCU reset.
- Motor output must be safe in BOOT0/DFU mode.
- Motor output must be safe during USB attach/PD negotiation.
- Motor output must not backfeed Sony or MCU rails.
- Full output range must be testable with dummy load before Sony connection.

Required test points:

- `TP_MOTOR_CTRL_OUT`,
- `TP_MOTOR_CTRL_RAW`,
- `TP_MOTOR_ENABLE`,
- `TP_MOTOR_GND`.

---

## 12. Speed Controls / ADC Sheet

### 12.1 `07_speed_controls_adc.kicad_sch`

Responsibilities:

- accept RV601, RV602, RV603 wiper signals,
- accept S601 Speed Tune state,
- protect/scalefor MCU inputs,
- filter noise,
- preserve original behavior where practical.

Required signals:

| Signal | Function |
|---|---|
| `RV601_WIPER` | Base speed calibration |
| `RV602_WIPER` | User Speed Tune |
| `RV603_WIPER` | Speed Tune range |
| `S601_SPEED_TUNE` | Speed Tune enable |

Rules:

- No wiper connects directly to ADC until voltage range is measured.
- If source impedance is high, choose sampling time or buffer accordingly.
- Open/dirty pot states should fail safe in firmware and hardware where possible.
- S601 input must not float.

Required test points:

- `TP_RV601`,
- `TP_RV602`,
- `TP_RV603`,
- `TP_S601`.

---

## 13. Timebase Reference Sheet

### 13.1 `08_timebase_reference.kicad_sch`

Responsibilities:

- implement selected servo timebase strategy,
- optionally provide external/reference-derived clock,
- preserve future option if decision remains open.

Possible blocks:

| Block | Status |
|---|---|
| Internal oscillator only | Development possible, final not proven |
| External crystal | Candidate |
| External oscillator | Candidate |
| Sony-derived reference input | Candidate |
| Measurement/test point | Required for any external reference |

Required input:

- `docs/hardware/timebase-decision.md`.

Rules:

- Do not block external/reference-derived option before decision closes.
- If internal-only, document that final speed claims require measurement.
- Clock routing must avoid motor/USB noise.
- Timebase test point is strongly preferred.

Required test points:

- `TP_CLK_REF`,
- `TP_HSE_IN`, if present,
- `TP_SONY_REF`, if present.

---

## 14. Sony Interface Connector Sheet

### 14.1 `09_sony_interface_connector.kicad_sch`

Responsibilities:

- define physical/electrical connection to WM-D6C,
- collect Sony-facing harness signals,
- document direction and safety,
- support installation testing.

Required signals:

| Signal | Direction |
|---|---|
| `FG_IN_RAW` | Sony → DSR-1 |
| `MOTOR_CTRL_OUT` | DSR-1 → Sony |
| `RV601_WIPER` | Sony → DSR-1 |
| `RV602_WIPER` | Sony → DSR-1 |
| `RV603_WIPER` | Sony → DSR-1 |
| `S601_SPEED_TUNE` | Sony → DSR-1 |
| `RAW_POWER` | Power |
| `B_PLUS` | Power/sense |
| `B_PLUS_3` | Power/sense |
| `GND` | Reference |

Rules:

- Connector pinout must match interface contract.
- Harness direction must be obvious.
- Provide keyed connector if practical.
- Avoid pinout that can swap power and signal easily.
- Installation points must be documented with photos later.

Required test points:

- all Sony-facing signals should be accessible during Rev A bring-up.

---

## 15. Testpoints / Debug / Boot Sheet

### 15.1 `10_testpoints_debug_boot.kicad_sch`

Responsibilities:

- SWD,
- BOOT0,
- reset,
- test pads,
- manufacturing/bring-up access,
- optional jumpers/configuration straps.

Required access:

| Signal | Required |
|---|---:|
| SWDIO | Yes |
| SWDCLK | Yes |
| GND | Yes |
| 3.3 V reference | Yes |
| NRST | Strongly recommended |
| BOOT0 | Yes |
| UART debug | Optional |
| Power test points | Yes |
| FG test points | Yes |
| Motor output test points | Yes |
| USB/PD test points | Yes |

Rules:

- Debug access must remain possible on Rev A.
- BOOT0 access must be clearly labeled.
- Configuration jumpers must have safe defaults.
- Test points must not create accidental shorts during installation.

---

## 16. ERC / Review Requirements

Before PCB layout:

| Review | Required |
|---|---:|
| KiCad ERC clean or explained | Yes |
| Power net names reviewed | Yes |
| USB-C pins reviewed | Yes |
| MCU pin allocation reviewed | Yes |
| Sony-facing nets reviewed | Yes |
| Protection devices reviewed | Yes |
| Reset/safe states reviewed | Yes |
| Backfeed paths reviewed | Yes |
| Test points reviewed | Yes |
| DNP options documented | Yes |
| Measurement dependencies marked | Yes |

Every ERC waiver must be documented.

---

## 17. DNP and Option Strategy

Rev A may include optional footprints, but options must be controlled.

Possible DNP options:

| Option | Purpose |
|---|---|
| Variant A (USB-C PD) vs Variant B (barrel jack) | Power input selection |
| External oscillator footprint | Preserve timebase option |
| Native UCPD vs external PD controller | Evaluate PD architecture |
| Rail-sense dividers | Populate as needed |
| USB shield termination variants | Evaluate noise/ESD behavior |
| Pull-up/down variants | Match measured Sony signals |

Rules:

- DNP options must not create unsafe default behavior.
- Default population must be documented.
- Mutually exclusive options must be clearly marked.
- Jumpers/solder bridges must fail safe.

---

## 18. Rev A Schematic Acceptance Criteria

The Rev A schematic is accepted when:

| Requirement | Status |
|---|---|
| Sheet hierarchy complete | Pending |
| MCU package selected | Pending |
| Pin allocation accepted | Pending |
| Power input/protection complete | Pending |
| USB-C data/PD architecture complete | Pending |
| 3.3 V rail complete | Pending |
| FG input conditioning complete or measurement-pending marker present | Pending |
| Motor output complete or measurement-pending marker present | Pending |
| Speed-control input conditioning complete | Pending |
| Timebase strategy represented | Pending |
| Sony interface connector defined | Pending |
| SWD/BOOT/test access complete | Pending |
| Safe reset states reviewed | Pending |
| Backfeed prevention reviewed | Pending |
| Test points present | Pending |
| ERC passed or waived with notes | Pending |
| BOM generated | Pending |
| Schematic review logged | Pending |

---

## 19. Open Questions

1. Is STM32G0B1KBU6 sufficient, or does Rev A need a larger package?
2. Is native UCPD feasible, or should Rev A use an external PD controller?
3. Does USB-C replace CN301 or coexist with it?
4. Does DSR-1 support battery-only operation?
5. Does DSR-1 generate motor/support rails or only interface with Sony rails?
6. ~~Is direct DAC motor output safe?~~ **Resolved: DAC is not used. PWM + NPN level-shift is the committed motor output for Ver. 1.0 boards.**
7. What FG conditioning is required? (Pending FG waveform measurement)
8. Is an external/reference-derived timebase required?
9. Where is USB-C mechanically mounted?
10. What connector/harness strategy is safest for Sony-facing signals?
11. Which optional footprints are worth carrying on Rev A?
12. What is the exact Q601 base voltage during playback? (Needed to confirm R9 sizing)

---

## 20. Final Rule

The Rev A schematic is not complete because every block has symbols.

It is complete only when every symbol reflects a measured requirement, a datasheet requirement, a safe default state, or a clearly marked validation assumption.
