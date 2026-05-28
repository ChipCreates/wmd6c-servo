# Cassette Tape Mechanics — The WM-D6C Transport

## Purpose of This Document

The DSR-1 module's entire purpose is to maintain a precise tape speed. Understanding
what "precise tape speed" means mechanically — how the WM-D6C's transport achieves
it, what each component contributes, and how the components degrade with age —
provides the foundation for understanding what the servo system is controlling and
why the mechanical service is inseparable from the electronic repair.

This document covers only the WM-D6C's specific transport design and the failure
modes relevant to this project. It is not a general compact cassette primer.

---

## 1. The Fundamental Standard: 4.75 cm/s

The international standard for compact cassette playback speed is 4.75 cm/s — the
tape must move past the playback head at exactly 4.75 centimetres per second. This
is defined by IEC 60094-1 and has been universal since Philips introduced the
compact cassette format in 1963.

The consequences of deviating from 4.75 cm/s are directly audible. Tape speed
determines pitch — a tape playing back at 5% over speed reproduces music at
approximately a semitone too high. A trained ear detects speed errors of 0.5% or
less. The WM-D6C's specification of 0.05% wow and flutter means instantaneous speed
variations are so small they are inaudible under any normal conditions.

For a field recording device used by broadcast professionals — the WM-D6C's intended
application — speed accuracy was critical. A journalist recording an interview needed
the playback to match the original recording precisely, regardless of battery charge
level, operating temperature, or tape reel weight. This is why the WM-D6C has a
closed-loop servo system rather than the simple open-loop motor drive used in
consumer Walkmans.

---

## 2. The WM-D6C Capstan Drive System

### 2.1 The Capstan Shaft

The capstan is a precision-ground stainless steel shaft, approximately 2mm in
diameter, mounted vertically through the transport mechanism chassis. In playback
mode, the tape passes between the capstan shaft and the pinch roller. The tape
speed is determined entirely by the surface speed of the capstan shaft — equal to
the shaft's rotational speed multiplied by its circumference (π × diameter =
approximately 6.28mm per revolution).

At 4.75 cm/s tape speed and a 2mm capstan diameter:

```
Shaft rotational speed = tape speed / circumference
                       = 47.5 mm/s / 6.28 mm/rev
                       = 7.56 rev/s = 454 RPM
```

This is the capstan shaft's target rotational speed during playback. The servo
system's job is to maintain this speed regardless of perturbations.

### 2.2 The Flywheel

The flywheel is a precision-machined metal disc of significant mass (typically
10-15 grams in the WM-D6C) mounted concentrically on the capstan shaft. Its
function is mechanical low-pass filtering: the flywheel's rotational inertia resists
changes in rotational speed, smoothing out the small, rapid speed variations that
the capstan motor inevitably produces.

The flywheel's effectiveness is characterised by its moment of inertia J. For a
flat disc: J = ½mr² where m is the mass and r is the radius. The WM-D6C flywheel,
with a mass of approximately 12g and a radius of approximately 8mm, has:

```
J = ½ × 0.012 × (0.008)² = 3.84 × 10⁻⁷ kg·m²
```

The time constant with which the flywheel resists speed changes from a given
disturbing torque τ is J/τ. For a small disturbance torque of 1 mN·m (roughly
the variation in motor torque from brush commutation):

```
τ_flywheel = J / τ = 3.84×10⁻⁷ / 0.001 = 0.384ms
```

At 454 RPM and 7.56 Hz rotation rate, the angular variation from a 1 mN·m torque
pulse lasting 1ms is:

```
Δω = τ × Δt / J = 0.001 × 0.001 / 3.84×10⁻⁷ = 2.6 rad/s
Δω/ω = 2.6 / (2π × 7.56) = 5.5%
```

Without the flywheel, motor brush commutation would produce 5.5% instantaneous
speed variation — clearly audible. The flywheel's inertia, by integrating this torque
pulse over time rather than converting it directly to velocity, reduces the resulting
speed variation to a level measurable only with precision instruments.

The servo system provides the outer correction loop that corrects for lower-frequency
speed errors (temperature drift, battery discharge, tape tension variation). The
flywheel provides the inner mechanical filter that attenuates high-frequency
disturbances that the servo loop is too slow to correct.

**Flywheel condition significance for this project**: The flywheel bearing (the
sleeve bearing in which the capstan shaft rotates) requires lubrication to operate
smoothly. In a 35-year-old machine, this lubrication has either dried out or
become a sticky gum. Bearing friction adds a load torque to the capstan shaft.
The servo loop can compensate for a steady friction load (the integral term
eliminates the steady-state error), but excessive friction increases the motor's
steady-state operating point, reduces the servo's corrective range in one direction,
and in extreme cases causes the motor to stall. Lubrication is not optional — it is
a prerequisite for correct servo operation.

### 2.3 The Capstan Belt

The capstan motor M901 does not drive the capstan shaft directly. Power is
transmitted from the motor pulley to the flywheel via a rubber drive belt — typically
a flat or round cross-section rubber belt of a specific length and tension.

The belt's role in tape speed accuracy is critical:

**Speed ratio**: The capstan shaft speed relative to the motor speed is determined
by the ratio of the motor pulley diameter to the flywheel diameter. For the WM-D6C,
this ratio is fixed at the motor's nominal operating speed — the servo system is
calibrated to lock the capstan shaft to the correct rotational speed.

**Belt slip**: A new belt in good condition transmits power with negligible slip.
An aged belt that has lost elasticity, hardened, or become glazed introduces slip
between the motor pulley and the flywheel. Belt slip means the motor can spin at
the servo's target speed while the flywheel (and therefore the capstan shaft and
tape) moves slower. The FG sensor is on the motor shaft, not the capstan shaft —
the servo locks the motor speed, which is no longer the same as the capstan speed.
Result: consistent speed error that cannot be corrected by servo tuning.

**Belt stretching**: A stretched belt has reduced tension. This increases the
tendency to slip under load and changes the effective drive ratio slightly. A
significantly stretched belt causes the capstan to run at the wrong speed even at
correct motor speed.

**Belt degradation modes in the WM-D6C**: The original belt in a 35-year-old
machine is almost certainly past its service life. The three failure modes in order
of likelihood are: hardening and glazing (reduced grip, increased slip); stretching
(reduced tension, changed drive ratio); and complete breakdown (belt becomes
crumbly, sticky, or breaks). The FixYourAudio rubber kit addresses all of these
with a belt manufactured to the correct dimensions for the WM-D6C.

### 2.4 The FG Sensor and Slotted Disc

The FG901 GP2S22AB optical sensor measures the rotational speed of the motor shaft,
not the capstan shaft. This is a critical design detail: the sensor is before the
belt in the power transmission path. If the belt is slipping, the sensor reports
the motor speed correctly but the capstan shaft is moving slower.

The slotted disc is a thin metal disc with precision-cut slots around its
circumference, mounted on the motor shaft. The FG901 sensor detects each slot
passage as the motor rotates. The relationship between the number of slots and the
FG pulse rate determines the servo's speed reference:

```
FG pulse rate = motor RPM × (slots per revolution) / 60
```

For the WM-D6C, the exact number of slots must be determined from the FG pulse rate
at correct tape speed (measured with a calibrated test tape). This is the
FG_TARGET_HZ value in `config.h`.

**FG disc contamination**: The FG disc is physically adjacent to the capstan belt.
As the belt ages and degrades, it sheds rubber particles and leaves a greasy residue
on every surface it contacts. This contamination can partially obstruct the FG901's
optical path, reducing the pulse amplitude. If the pulse amplitude drops below the
threshold where it can be detected through the level-shifting network, the servo
loses its speed reference and the motor runs open-loop — precisely the symptom of
a failed CX20084. FG disc cleaning is mandatory before diagnosing any electronic
fault.

---

## 3. The Pinch Roller

### 3.1 Function

The pinch roller is a rubber-coated wheel mounted on a spring-loaded pivot. In
playback mode, the transport mechanism presses the pinch roller against the capstan
shaft, with the tape sandwiched between them. The tape is driven forward by the
friction between the capstan shaft surface and the pinch roller rubber — the
capstan pushes the tape, the pinch roller provides a compliant backing surface.

The pinch roller does not determine tape speed — that is the capstan's job. The
pinch roller's function is to maintain consistent contact pressure between the tape
and the capstan shaft so that the capstan's surface speed is reliably transferred
to the tape without slip. Insufficient contact pressure allows the tape to slip
against the capstan, producing apparent speed variation. Excessive pressure creates
high tape tension, increased tape wear, and additional load on the capstan bearing.

The WM-D6C's cassette-holding spring is adjusted to provide 130-260g (4.59-9.17 oz)
of force pressing the cassette (and with it the pinch roller) against the capstan
shaft, as specified in the service manual. This preload is designed around the
mechanical properties of new rubber on a smooth shaft.

### 3.2 Pinch Roller Degradation and Its Effects

The pinch roller rubber hardens progressively over time through oxidation and
physical set (compression creep). The original rubber in a WM-D6C pinch roller
manufactured in 1990 is now 35 years old. Under normal circumstances it will be
substantially harder than new rubber, often resembling plastic in its hardness.

**Hardened roller — reduced grip**: A hard, smooth pinch roller has reduced
coefficient of friction against the capstan shaft. Under load, the tape may slip
rather than being driven at capstan surface speed. This appears as erratic,
inconsistently-fast, or wandering playback speed that the servo loop partially
corrects but cannot eliminate because the slippage is stochastic and load-dependent.

**Hardened roller — changed geometry**: As rubber hardens, it often develops a
concave or convex wear profile from decades of contact with the capstan shaft. This
changes the contact patch geometry, potentially causing the tape to be driven at
different speeds across its width — one edge moving faster than the other — which
produces azimuth-like treble loss from poor head contact uniformity.

**Glazed roller**: A pinch roller that has been in contact with the capstan shaft
without use for extended periods (e.g., a machine in storage with a cassette
loaded and in play mode) develops a glazed contact surface with extremely low
friction. A glazed roller feels smooth and almost polished. It will not grip the
tape adequately under any conditions and must be replaced.

**The micrometer measurement**: Before installing the replacement pinch roller,
measure the original and the replacement as described in the installation notes:
outer diameter, face width, and shaft bore. The diameter is the most critical
dimension — a roller that is even 0.5mm smaller in diameter reduces the tape-to-
capstan contact pressure by changing the geometric relationship, potentially
allowing tape flutter. The replacement from a specialist supplier for the WM-D6C
should be dimensionally correct, but verification eliminates any uncertainty.

---

## 4. The Reel Drive System and Tape Tension

### 4.1 Why Tape Tension Matters for Speed

The capstan determines tape speed. But the tape must also be kept taut — if the
take-up reel doesn't pull the tape away from the capstan fast enough, the tape will
accumulate between the capstan and the reel, eventually jamming. If the supply reel
doesn't provide enough back-tension, the tape will be insufficiently taut at the
head, degrading playback quality.

The reel drive system provides:
- **Take-up torque**: A gentle forward pull on the tape after the capstan, keeping
  it wound onto the take-up reel
- **Supply back-tension**: A light braking force on the supply reel, keeping the
  tape taut between the supply reel and the head block

Both of these forces must be correctly balanced. If the take-up torque is too high,
the tape is pulled against the capstan with extra force, increasing the load torque
the servo must overcome. If the back-tension is too low or too high, the tape does
not lie flat against the playback head.

### 4.2 The Reel Drive Path in the WM-D6C

The WM-D6C uses a separate motor (not M901) or a mechanical take-off from the main
drive system to provide reel torque — the service manual shows the reel mechanism
as a separate mechanical assembly with its own idler gears and friction clutches.
The exact reel drive architecture is visible in the exploded views (figures 5-4 and
5-5 of the service manual).

The rubber parts in the reel drive system — idler tires, brake pads, and friction
elements — are included in the FixYourAudio rubber kit and must be replaced as part
of the service. Degraded reel drive rubber directly affects tape tension, which
affects the load torque presented to the capstan servo.

**Implication for servo tuning**: After replacing the rubber kit and the pinch
roller, allow the transport to run for 10-15 minutes before measuring speed accuracy
and tuning the servo PI constants. The new rubber needs brief break-in time to seat
properly, and tape tension stabilises as the reel clutches find their operating
points.

---

## 5. The Tape Path and Head Contact

### 5.1 Head Geometry

The WM-D6C uses a combined record/playback head (HRE901) mounted on an adjustable
plate. The head's position relative to the tape determines:

**Azimuth**: The angle of the head gap relative to the tape direction. Correct
azimuth (90° to tape travel) maximises high-frequency response. Incorrect azimuth
produces treble loss that increases with frequency — a 1° error at 10kHz produces
approximately 4dB of loss.

**Height**: The head must be positioned so that the gap is centered on the tape's
recording tracks. Incorrect height reduces output level.

**Tape wrap angle**: The tape wraps around the head to ensure adequate contact.
The tape guides on either side of the head control this wrap angle. The service
manual azimuth adjustment procedure (using a test tape with a 10kHz tone on both
channels simultaneously and adjusting RV101/RV201 for maximum output) sets this
correctly.

The azimuth and head adjustment procedures in the service manual are performed after
mechanical service and with a calibrated test tape — the same test tape used for
speed verification. Speed must be confirmed correct before azimuth is set, because
azimuth affects the apparent frequency of the test tone.

### 5.2 The Erase Head

HE901 is the bias-oscillator-driven erase head that clears the tape immediately
before the record head during recording. In playback mode, the erase head is
inactive. It does not affect playback mechanics.

---

## 6. What the Servo Actually Controls and What It Cannot

### 6.1 What the Servo Controls

The servo controls the rotational speed of the capstan motor shaft, measured by
the FG901 sensor. It does this by adjusting the motor drive current through Q601.

Through the mechanical chain:

```
Motor shaft speed
    → (belt) → Flywheel/capstan shaft speed
    → (capstan surface × radius) → Linear tape speed
```

If all the mechanical links are functioning correctly — belt not slipping, capstan
bearing friction within normal limits, flywheel intact — then controlling motor
shaft speed is equivalent to controlling tape speed, and the servo's speed accuracy
translates directly to tape accuracy.

### 6.2 What the Servo Cannot Fix

**Belt slip**: If the belt is slipping, the FG sensor reports correct motor speed
but the tape moves slower. The servo cannot detect this — it sees the FG signal
and declares that speed is correct. The only fix is belt replacement.

**Contaminated FG disc**: If the FG disc is partially blocked, the servo receives
corrupted or missing pulses and cannot control speed correctly. The only fix is
disc cleaning.

**Failed pinch roller**: If the pinch roller is not pressing the tape against the
capstan adequately, the tape slips regardless of capstan speed. The servo cannot
detect tape slip directly — the FG sensor is on the motor, not the tape.

**Worn capstan bearing**: If the capstan bearing has excessive play, the capstan
shaft wobbles. This creates a periodic speed variation at the capstan's rotational
frequency (454 RPM = 7.56 Hz) — a slow, regular wow that may be partially masked
by the flywheel but cannot be corrected by the servo.

This is why the service manual's prescribed sequence — mechanical service before
electronic diagnosis — is not merely procedural caution. It is the correct
diagnostic methodology. A correctly serviced machine with functioning mechanical
components provides the clean, repeatable physical plant that the servo system
assumes when its PI constants were calibrated. A mechanically degraded machine
running a correctly tuned servo will still play at the wrong speed.

---

## 7. The Measurement Standard: Wow and Flutter

### 7.1 What These Terms Mean

**Wow** refers to slow speed variations, typically below 6 Hz. It is perceived as
a slow pitch wavering, most noticeable on sustained piano notes or vocal held tones.
Wow is caused by eccentric reels, uneven tape tension cycling at the reel rotation
rate, or slow servo oscillation.

**Flutter** refers to fast speed variations, typically 6 Hz to 200 Hz. It is
perceived as a roughness or "warble" on sustained tones. Flutter is caused by
capstan bearing irregularities, pinch roller eccentricity, or motor brush
commutation noise that passes through the flywheel.

**WRMS** (Weighted Root Mean Square) is the measurement method defined by IEC 60386
that applies frequency-weighting to the speed variations to reflect their audible
significance — human hearing is most sensitive to speed variations in the 1-6 Hz
range, and the weighting function reflects this.

### 7.2 The WM-D6C Specification in Context

The WM-D6C's specification of 0.05% WRMS wow and flutter is exceptional for a
portable cassette machine. For comparison:

| Machine type | Typical W&F spec |
|---|---|
| Consumer Walkman | 0.2-0.3% WRMS |
| Decent consumer cassette deck | 0.05-0.10% WRMS |
| WM-D6C | 0.05% WRMS |
| Professional broadcast tape machine | 0.03-0.08% WRMS |

The WM-D6C achieves broadcast-quality tape transport in a jacket pocket. This
specification was achieved through the combination of the precision flywheel
(mechanical flutter rejection), the CX20084 servo system (electronic speed control),
and high-precision mechanical components. The DSR-1 module replaces the electronic
portion with a system that is inherently more accurate — digital period measurement
vs analog phase comparison — while preserving the mechanical components that provide
the flywheel filtering.

### 7.3 How to Measure It

The test tape included in the preparation for this project (used for speed
verification and azimuth setting) can also be used to verify wow and flutter
performance after installation. The method:

**Using a smartphone spectrum analyser** (free app): Play the 3150 Hz test tone
through the headphone output (monitored at the LINE OUT jack, not through the
volume control). Observe the frequency display. A properly functioning machine
shows a stable, narrow peak at 3150 Hz. Wow appears as a slow oscillation of the
peak's frequency. Flutter appears as sidebands symmetrically around the peak.

**Using a proper flutter meter**: Connect to LINE OUT at the specified level
(approximately -20 dBu for the WM-D6C standard output). Measure to IEC 60386
weighted standard.

The smartphone method is less precise but sufficient for confirming that no gross
speed instability is present after installation. The proper flutter meter measurement
is the standard for confirming the 0.05% specification is met.

---

## 8. Summary: The Mechanical Prerequisites for Correct Servo Operation

Before the DSR-1 module can be correctly calibrated and before its speed performance
can be evaluated, the following mechanical conditions must be met:

| Condition | Check | Action if failed |
|---|---|---|
| Capstan belt | Not stretched, not glazed, correct tension | Replace with kit belt |
| Pinch roller | Not hardened, correct diameter | Replace with measured replacement |
| FG slotted disc | Clean, not contaminated | Clean with IPA |
| Capstan bearing | Smooth rotation, no play | Clean and lubricate with clock oil |
| Reel drive rubber | Idler tires not glazed, brakes functional | Replace with kit parts |
| Transport mechanism | All levers spring correctly, mechanism engages | Lubricate pivot points |

A machine that passes all six checks provides the mechanical foundation on which
the DSR-1 servo can achieve the WM-D6C's 0.05% specification. A machine with any
of these conditions unmet will not meet specification regardless of how precisely
the servo is calibrated.

---

## See Also

- [Original Servo Circuit](original-servo-circuit.md) — how the FG sensor's signal
  is used by the servo system
- [Digital PLL Servo](digital-pll-servo.md) — how the STM32 measures and controls
  motor speed
- [Signal Chain Analysis](signal-chain-analysis.md) — FG signal conditioning from
  sensor to STM32 pin
- [Installation Guide](../installation/) — mechanical service procedure
- [Bench Measurements](../bench-measurements/) — speed measurement procedure and
  expected results
