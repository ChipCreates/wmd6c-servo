# Motor-Drive Sign Convention

**Project:** DSR-1 / wmd6c-servo
**File:** `docs/hardware/motor-drive-sign-convention.md`
**Status:** Defect note + bench procedure / pre-Rev A — **resolve before driving a motor**
**Scope:** Reconcile the three contradictory descriptions of the PWM→speed polarity,
define the single bench measurement that settles it, and give the exact code change
for each outcome. Until this is closed, the servo must not be connected to a real
capstan motor.

---

## 1. The contradiction

Three places in the firmware describe the relationship between PWM duty and motor
speed, and they do not agree.

| Source | Statement | Implication |
|---|---|---|
| `config.h` (`DAC_MIN`/`DAC_MAX` comments) | `DAC_MIN` = "Near-full motor drive", `DAC_MAX` = "Motor nearly off" | **Low PWM → fast** |
| `servo.c` (block comment) | "Higher PWM duty → Q_LS conducts more → Q601 base pulled lower → faster" | **High PWM → fast** |
| `servo.c` (control law) | `output = DAC_CENTER − (kp·error>>16) − (ki·integral>>16)` | On too-slow (positive) error, PWM **decreases** |

The control law only produces *negative* feedback (stable) if `config.h` is
correct — i.e. if **low PWM = fast**, then a too-slow condition correctly lowers
PWM to speed up. If the `servo.c` narrative is correct — **high PWM = fast** — then
lowering PWM on a too-slow condition makes it *slower*, which is **positive
feedback**: the loop drives PWM to a rail on the first FG edge and the motor either
stops or runs away.

So at most one of the two narratives is right, and which one decides whether the
shipped control-law sign is correct or inverted.

---

## 2. Why this can't be resolved from the desk

The polarity depends on the real behavior of the output stage on the target board:

```
TIM3_CH1 (PA6) ── R7/C8 RC filter ──► Q_LS (MMBT3904 NPN) base
Q_LS collector ── R9 (100k to B+1) ──► Q601 base
Q601 exact package/marking and base/emitter operating range pending on C11-494-12
```

A first-principles trace (higher PWM → more Q_LS base drive → Q_LS collector pulls
Q601 base toward GND → more drive in the working PNP model → faster) supports
the `servo.c` narrative (**high PWM = fast**), which would make the current
subtraction the *wrong* sign. But this depends on the exact bias network, where R9
returns to (B+1), the motor-drive network topology on the Sony board, and the
actual transfer curve around the operating point — none of which are confirmed
until measured. The `config.h` annotation may reflect a later, corrected
understanding, or it may be stale. The desk analysis is suggestive, not
authoritative.

---

## 3. The measurement that settles it

This slots into `docs/verification/03-motor-drive-characterization.md`. Goal: map
PWM duty → Q601 base voltage → motor speed, open-loop, on the target unit
(SN72795), and read off the sign.

Setup:

- DSR-1 driving only the PA6 PWM output through the real R7/C8/R9 + Q_LS + Q601
  network on the bench unit. **Servo loop disabled** (open-loop sweep — see §4).
- Scope/DMM on Q601 base and emitter; frequency counter on FG901; LINE OUT to an
  audio analyzer with test tape WS-48B for a speed reference.

Procedure:

1. Hold PWM at `DAC_CENTER` (2048). Record Q601 V_EB and FG frequency.
2. Step PWM up toward `DAC_MAX` (e.g. 2048 → 2560 → 3072). Record Q601 V_EB and FG
   frequency at each step.
3. Step PWM down toward `DAC_MIN` (e.g. 2048 → 1536 → 1024). Record the same.
4. Determine the sign of dFG/dPWM:
   - If **higher PWM raises FG frequency** (faster) → "high PWM = fast".
   - If **higher PWM lowers FG frequency** (slower) → "low PWM = fast".

Safety while sweeping: keep steps small, watch Q601 V_EB and motor current, and be
ready to return PWM to a known-off value. Confirm the boot-safe state first: at 0%
duty Q_LS is off and R9 should hold Q601 off (motor stopped). Verify that before
sweeping.

---

## 4. Driving the open-loop sweep safely

The current firmware has no open-loop mode. Two options:

- **Temporary test build:** add a compile-time flag that bypasses the PI law and
  drives `TIM3->CCR1` from a USB-CDC command (e.g. reuse `f±` to step raw PWM).
  Clearly mark it as a characterization-only build that must not ship.
- **External signal generator** into PA6 with the MCU's TIM3 output disabled,
  driving the RC/level-shift stage directly. Avoids touching servo firmware but
  requires lifting PA6 from the MCU drive.

The test build is usually less error-prone because it keeps the exact RC-filtered
PWM waveform the real system uses.

---

## 5. The fix, for each outcome

After the measurement, make all three sources agree. Only one of the following code
states is correct; the comments in `config.h` **and** `servo.c` must match it.

### Outcome A — low PWM = fast (config.h is correct)

The shipped control law is already right. No code change to the arithmetic. Action:

- Correct the `servo.c` block comment, which currently claims the opposite
  ("higher PWM → faster"), so the prose matches the code.

### Outcome B — high PWM = fast (servo.c narrative is correct)

The shipped control law is **inverted** and would be unstable. Flip the two signs:

```c
int32_t output = DAC_CENTER
               + ((kp * error)    >> 16)    /* was − */
               + ((ki * integral) >> 16);   /* was − */
```

Then also:

- Update the `config.h` `DAC_MIN`/`DAC_MAX` annotations (they will now mean
  `DAC_MIN` = motor nearly off, `DAC_MAX` = near-full drive), and re-check that the
  `DAC_MIN`/`DAC_MAX` clamp headroom still keeps Q601 out of full-off/full-on at the
  extremes in the new direction.
- Re-confirm the boot-safe reasoning: the motor-off state must still correspond to
  the reset condition (TIM3 at 0% duty, Q_LS off, R9 holding Q601 off). If "high
  PWM = fast", 0% duty should be the slow/off end — verify this matches the
  measured curve so power-on remains motor-off.

---

## 6. Acceptance

The sign convention is considered resolved only when:

| Requirement | Status |
|---|---|
| Open-loop PWM→Q601 V_EB→FG curve measured on SN72795 | Pending |
| Sign of dFG/dPWM recorded | Pending |
| Control-law arithmetic confirmed/corrected against the measured sign | Pending |
| `config.h` and `servo.c` comments made consistent with the code | Pending |
| Boot-safe (0% duty = motor off) verified against the measured curve | Pending |
| First closed-loop lock achieved without runaway | Pending |

---

## 7. Design rule

The servo loop sign is a property of the hardware, not the firmware. Do not close
the loop on a real motor until the open-loop sign has been measured and the three
descriptions are made to agree. A PI loop with the wrong sign is not a tuning
problem — it is guaranteed instability.
