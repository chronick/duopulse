# V5 Design Iteration 6: LED Feedback, Voice Relationships, Extensibility

**Date**: 2026-01-04
**Focus**: LED behaviors, Voice 2 call/response, firmware variation architecture

---

## New in This Iteration

1. **LED Feedback Specification** - Complete brightness/pattern behaviors
2. **Voice 2 Relationship** - Call/response paradigm with variety
3. **Extensibility Architecture** - Support for percussive/bassline/ambient variations
4. **Aux Hat Burst Refinement** - ENERGY + SHAPE sensitive, pattern-aware

---

## LED Feedback Specification

Single LED output (CV Out 2), brightness only (0-5V). All feedback uses brightness levels and temporal patterns.

### Brightness Levels

```cpp
enum class LEDBrightness {
    OFF = 0,        // 0V - LED dark
    DIM = 64,       // ~1.25V - subtle indicator
    MEDIUM = 128,   // ~2.5V - normal visibility
    BRIGHT = 192,   // ~3.75V - attention-getting
    FULL = 255      // 5V - maximum brightness
};
```

### LED Behaviors by Context

#### 1. Normal Operation (Performance Mode)

**Trigger Activity** - LED pulses on hits, brightness = combined velocity

```cpp
void UpdateLEDTriggerActivity(bool voice1Hit, bool voice2Hit,
                               float v1Velocity, float v2Velocity) {
    if (voice1Hit || voice2Hit) {
        // Pulse brightness based on combined velocity
        float combinedVel = (v1Velocity + v2Velocity) / 2.0f;

        // Quick attack, medium decay
        led.SetBrightness(combinedVel * FULL);
        led.SetDecay(50);  // 50ms decay to DIM
    }

    // Idle: slow breath at DIM level synced to clock
    if (!voice1Hit && !voice2Hit) {
        led.BreatheSlow(DIM, clockPhase);
    }
}
```

**Visual**: Pulses on hits, gentle breathing between hits (shows clock sync)

---

#### 2. Fill Active

**Pattern**: Rapid strobe that accelerates, then "explodes"

```cpp
void UpdateLEDFill(float fillProgress, float intensity) {
    // fillProgress: 0.0 (fill start) to 1.0 (fill end)
    // intensity: 0.5 to 1.5 based on ENERGY

    // Phase 1 (0-80%): Accelerating strobe
    if (fillProgress < 0.8f) {
        float strobeRate = 4.0f + fillProgress * 20.0f;  // 4Hz to 24Hz
        float strobePhase = fmod(fillProgress * strobeRate * 10.0f, 1.0f);

        if (strobePhase < 0.5f) {
            led.SetBrightness(BRIGHT * intensity);
        } else {
            led.SetBrightness(DIM);
        }
    }
    // Phase 2 (80-100%): Explosion - rapid flutter then hold bright
    else {
        float explosionPhase = (fillProgress - 0.8f) / 0.2f;

        if (explosionPhase < 0.5f) {
            // Flutter: very fast random-ish
            led.SetBrightness((rand() % 2) ? FULL : MEDIUM);
        } else {
            // Hold bright
            led.SetBrightness(FULL * intensity);
        }
    }
}
```

**Visual**: Strobe speeds up → flutter → bright flash at fill end. Fun and energetic!

---

#### 3. Reseed Progress (3-second hold)

**Pattern**: Building pulse that "pops" on release

```cpp
void UpdateLEDReseedProgress(float holdProgress) {
    // holdProgress: 0.0 (just pressed) to 1.0 (3 seconds)

    // Pulse rate increases as hold progresses
    float pulseRate = 1.0f + holdProgress * 4.0f;  // 1Hz to 5Hz
    float pulsePhase = fmod(holdProgress * pulseRate * 3.0f, 1.0f);

    // Pulse amplitude increases
    float minBright = DIM;
    float maxBright = DIM + holdProgress * (FULL - DIM);

    // Sine-ish pulse
    float brightness = minBright + (maxBright - minBright) *
                       (0.5f + 0.5f * sin(pulsePhase * 2.0f * PI));

    led.SetBrightness(brightness);
}

void TriggerLEDReseedConfirm() {
    // Pop! Three quick flashes
    led.Flash(FULL, 50);   // 50ms on
    led.Pause(30);         // 30ms off
    led.Flash(FULL, 50);
    led.Pause(30);
    led.Flash(FULL, 100);  // Longer final flash
}
```

**Visual**: Gentle pulse builds faster and brighter → POP POP POP on reseed. Satisfying!

---

#### 4. Mode Switch (Performance ↔ Config)

**Pattern**: Distinct "signature" for each mode

```cpp
void TriggerLEDModeSwitch(Mode newMode) {
    if (newMode == PERFORMANCE) {
        // Quick double-tap: ready for action
        led.Flash(BRIGHT, 40);
        led.Pause(40);
        led.Flash(BRIGHT, 40);
    } else {  // CONFIG
        // Slow fade up: contemplative
        led.FadeUp(DIM, MEDIUM, 200);  // 200ms fade
        led.Hold(MEDIUM, 100);
        led.FadeDown(MEDIUM, DIM, 200);
    }
}
```

**Visual**:
- → Performance: Quick tap-tap (energetic)
- → Config: Slow bloom (thoughtful)

---

#### 5. Config Mode Idle

**Pattern**: Slower breathing than performance mode, different rhythm

```cpp
void UpdateLEDConfigIdle(float time) {
    // Slower, deeper breath than performance mode
    // 4-second cycle vs 2-second in performance
    float breathPhase = fmod(time / 4.0f, 1.0f);
    float brightness = DIM + (MEDIUM - DIM) *
                       (0.5f + 0.5f * sin(breathPhase * 2.0f * PI));

    led.SetBrightness(brightness);
}
```

**Visual**: Noticeably slower breathing = "I'm in config mode"

---

#### 6. Clock Sync Indicator

**Pattern**: Subtle pulse on downbeat (bar start)

```cpp
void UpdateLEDClockSync(bool isDownbeat, bool isQuarter) {
    if (isDownbeat) {
        // Downbeat: brief bright flash
        led.Boost(BRIGHT, 30);  // 30ms boost, then decay
    } else if (isQuarter) {
        // Quarter note: tiny flicker
        led.Boost(MEDIUM, 15);
    }
    // These layer ON TOP of current behavior
}
```

**Visual**: Subtle sync pulses help performer feel the grid

---

#### 7. External Clock Lock/Loss

```cpp
void TriggerLEDClockLock() {
    // Smooth: we're synced
    led.FadeUp(DIM, BRIGHT, 100);
    led.FadeDown(BRIGHT, MEDIUM, 200);
}

void TriggerLEDClockLost() {
    // Frantic: something's wrong
    for (int i = 0; i < 5; i++) {
        led.Flash(FULL, 30);
        led.Pause(30);
    }
    // Then settle to internal clock behavior
}
```

---

### LED State Priority

When multiple states are active, priority order:

1. **Reseed confirm** (highest - momentary)
2. **Fill active**
3. **Mode switch** (momentary)
4. **Clock lost** (momentary)
5. **Trigger activity**
6. **Config idle / Performance idle** (lowest)

---

## Voice 2 Relationship: Call/Response Architecture

Voice 1 = "Anchor" (call), Voice 2 = "Shimmer" (response)

### Design Goals

1. **Call/Response feel** - Voice 2 complements, doesn't compete
2. **Variety** - More options than v4's single relationship mode
3. **Extensibility** - Architecture supports non-percussive variations

### Voice Relationship Profiles

```cpp
enum class VoiceRelationship {
    SHADOW,      // Voice 2 follows Voice 1 with offset
    COMPLEMENT,  // Voice 2 fills gaps left by Voice 1
    ECHO,        // Voice 2 repeats Voice 1 with delay
    COUNTER,     // Voice 2 emphasizes opposite beats
    UNISON,      // Voice 2 mirrors Voice 1 (for bassline/ambient)
};
```

### Relationship Implementations

#### SHADOW (Default for Percussive)

Voice 2 fires shortly after Voice 1, creating "flam" or "drag" feel.

```cpp
void ApplyShadowRelationship(Pattern& v1, Pattern& v2, float shape) {
    for (int step = 0; step < patternLength; step++) {
        if (v1.hits[step]) {
            // Shadow delay: 1-3 steps based on SHAPE
            int shadowDelay = 1 + (int)(shape * 2);
            int shadowStep = (step + shadowDelay) % patternLength;

            // Voice 2 gets boosted probability at shadow position
            v2.weight[shadowStep] += 0.4f;

            // Suppress Voice 2 at Voice 1's exact position
            v2.weight[step] *= 0.3f;
        }
    }
}
```

#### COMPLEMENT

Voice 2 fills the spaces Voice 1 leaves empty.

```cpp
void ApplyComplementRelationship(Pattern& v1, Pattern& v2, float energy) {
    for (int step = 0; step < patternLength; step++) {
        if (v1.hits[step]) {
            // Suppress Voice 2 where Voice 1 hits
            v2.weight[step] *= 0.1f;

            // Boost Voice 2 on adjacent steps
            int prev = (step - 1 + patternLength) % patternLength;
            int next = (step + 1) % patternLength;
            v2.weight[prev] += 0.2f;
            v2.weight[next] += 0.2f;
        } else {
            // Boost Voice 2 in Voice 1's gaps
            v2.weight[step] += 0.15f * energy;
        }
    }
}
```

#### ECHO

Voice 2 repeats Voice 1's pattern with a delay.

```cpp
void ApplyEchoRelationship(Pattern& v1, Pattern& v2, float shape) {
    // Echo delay: 2-6 steps based on SHAPE
    int echoDelay = 2 + (int)(shape * 4);

    for (int step = 0; step < patternLength; step++) {
        int echoStep = (step + echoDelay) % patternLength;

        if (v1.hits[step]) {
            // Copy hit to echo position with reduced velocity
            v2.hits[echoStep] = true;
            v2.velocity[echoStep] = v1.velocity[step] * 0.7f;
        }
    }
}
```

#### COUNTER

Voice 2 emphasizes beats Voice 1 avoids.

```cpp
void ApplyCounterRelationship(Pattern& v1, Pattern& v2, float axisX) {
    // Calculate Voice 1's emphasis pattern
    float v1Emphasis[16];
    for (int step = 0; step < patternLength; step++) {
        v1Emphasis[step] = v1.hits[step] ? 1.0f : 0.0f;
    }

    // Voice 2 gets inverse emphasis, modulated by AXIS X
    for (int step = 0; step < patternLength; step++) {
        float counterWeight = 1.0f - v1Emphasis[step];
        v2.weight[step] = counterWeight * (0.5f + axisX * 0.5f);
    }
}
```

#### UNISON (for Bassline/Ambient variations)

Voice 2 mirrors Voice 1 exactly (both outputs fire together).

```cpp
void ApplyUnisonRelationship(Pattern& v1, Pattern& v2) {
    for (int step = 0; step < patternLength; step++) {
        v2.hits[step] = v1.hits[step];
        v2.velocity[step] = v1.velocity[step];
    }
}
```

### Relationship Selection

For v5 percussive mode, relationship is **automatically selected** based on AXIS Y:

```cpp
VoiceRelationship GetRelationshipFromAxisY(float axisY, float shape) {
    // AXIS Y = Simple (0%) → Complex (100%)
    // Simple patterns use SHADOW (tight)
    // Complex patterns use COMPLEMENT or COUNTER (interlocking)

    if (axisY < 0.25f) {
        return SHADOW;
    } else if (axisY < 0.5f) {
        return COMPLEMENT;
    } else if (axisY < 0.75f) {
        // Mix: sometimes ECHO, sometimes COUNTER
        return (shape < 0.5f) ? ECHO : COUNTER;
    } else {
        return COUNTER;
    }
}
```

**Result**: As AXIS Y increases complexity, voice relationship becomes more intricate.

---

## Extensibility Architecture: Firmware Variations

### Variation Profiles

The engine can load different "variation profiles" that reinterpret the abstract controls.

```cpp
struct VariationProfile {
    const char* name;

    // How to interpret controls
    ControlMapping energyMapping;    // What ENERGY controls
    ControlMapping shapeMapping;     // What SHAPE controls
    ControlMapping axisXMapping;     // What AXIS X controls
    ControlMapping axisYMapping;     // What AXIS Y controls

    // Voice behavior
    VoiceRelationship defaultRelationship;
    bool voicesIndependent;          // If true, Voice 2 is fully separate

    // Output interpretation
    OutputMode voice1Output;         // TRIGGER, GATE, CV_PITCH, CV_MOD
    OutputMode voice2Output;
    OutputMode velocityOutput;       // What velocity CV represents

    // Pattern generation
    PatternGenerator* generator;     // Pluggable pattern engine
};
```

### Built-in Variation Profiles

#### 1. PERCUSSIVE (Default)

```cpp
VariationProfile PERCUSSIVE = {
    .name = "Percussive",

    .energyMapping = { .target = HIT_DENSITY, .range = {0.0f, 1.0f} },
    .shapeMapping = { .target = ALGORITHM_BLEND, .range = {0.0f, 1.0f} },
    .axisXMapping = { .target = BEAT_POSITION, .range = {0.0f, 1.0f} },
    .axisYMapping = { .target = COMPLEXITY, .range = {0.0f, 1.0f} },

    .defaultRelationship = SHADOW,
    .voicesIndependent = false,

    .voice1Output = TRIGGER,     // Gate Out 1 = trigger
    .voice2Output = TRIGGER,     // Gate Out 2 = trigger
    .velocityOutput = CV_LEVEL,  // Audio Out = 0-5V velocity

    .generator = &PercussivePatternGenerator,
};
```

#### 2. BASSLINE (Future)

```cpp
VariationProfile BASSLINE = {
    .name = "Bassline",

    .energyMapping = { .target = NOTE_DENSITY, .range = {0.0f, 1.0f} },
    .shapeMapping = { .target = MELODIC_CONTOUR, .range = {0.0f, 1.0f} },
    .axisXMapping = { .target = PITCH_RANGE, .range = {0.0f, 1.0f} },
    .axisYMapping = { .target = RHYTHMIC_COMPLEXITY, .range = {0.0f, 1.0f} },

    .defaultRelationship = UNISON,
    .voicesIndependent = true,

    .voice1Output = GATE,        // Gate Out 1 = note gate
    .voice2Output = CV_PITCH,    // Gate Out 2 = pitch CV (repurposed!)
    .velocityOutput = CV_MOD,    // Audio Out = filter/mod CV

    .generator = &BasslinePatternGenerator,
};
```

**Control Reinterpretation for Bassline**:
- ENERGY = Note density (how many notes per bar)
- SHAPE = Melodic contour (stepwise ↔ leapy)
- AXIS X = Pitch range (narrow ↔ wide)
- AXIS Y = Rhythmic complexity (straight ↔ syncopated)

#### 3. AMBIENT (Future)

```cpp
VariationProfile AMBIENT = {
    .name = "Ambient",

    .energyMapping = { .target = EVENT_DENSITY, .range = {0.0f, 0.3f} },  // Slower
    .shapeMapping = { .target = TEXTURE_DENSITY, .range = {0.0f, 1.0f} },
    .axisXMapping = { .target = MOVEMENT_RATE, .range = {0.0f, 1.0f} },
    .axisYMapping = { .target = HARMONIC_RICHNESS, .range = {0.0f, 1.0f} },

    .defaultRelationship = COMPLEMENT,
    .voicesIndependent = true,

    .voice1Output = CV_MOD,      // Slow modulation CV
    .voice2Output = CV_MOD,      // Second modulation CV
    .velocityOutput = CV_MOD,    // Third modulation CV

    .generator = &AmbientPatternGenerator,
};
```

### Pattern Generator Interface

```cpp
class PatternGenerator {
public:
    virtual void Generate(
        Pattern& v1, Pattern& v2,
        float energy, float shape, float axisX, float axisY,
        uint32_t seed, int patternLength
    ) = 0;

    virtual void ApplyRelationship(
        Pattern& v1, Pattern& v2,
        VoiceRelationship relationship,
        float energy, float shape, float axisX, float axisY
    ) = 0;
};
```

This allows completely different pattern generation strategies per variation.

---

## Aux Hat Burst: ENERGY + SHAPE Sensitive

Refined to follow the same principles as main pattern generation.

### Hat Burst Algorithm

```cpp
struct HatBurst {
    int triggerCount;
    float* triggerTimes;  // Relative times within burst window
    float* velocities;
};

HatBurst GenerateHatBurst(float energy, float shape, float burstDurationMs,
                          const Pattern& mainPattern, int currentStep) {
    HatBurst burst;

    // ═══════════════════════════════════════════════════════════════
    // ENERGY determines burst density
    // ═══════════════════════════════════════════════════════════════
    int minTriggers = 2;
    int maxTriggers = 12;
    burst.triggerCount = minTriggers + (int)(energy * (maxTriggers - minTriggers));

    burst.triggerTimes = new float[burst.triggerCount];
    burst.velocities = new float[burst.triggerCount];

    // ═══════════════════════════════════════════════════════════════
    // SHAPE determines regularity of timing
    // ═══════════════════════════════════════════════════════════════

    float intervalBase = burstDurationMs / burst.triggerCount;

    for (int i = 0; i < burst.triggerCount; i++) {
        if (shape < 0.3f) {
            // LOW SHAPE: Regular spacing (like euclidean)
            burst.triggerTimes[i] = i * intervalBase;
        }
        else if (shape < 0.7f) {
            // MID SHAPE: Slight variation
            float jitter = (RandomFloat() - 0.5f) * intervalBase * 0.3f * shape;
            burst.triggerTimes[i] = i * intervalBase + jitter;
        }
        else {
            // HIGH SHAPE: Irregular/clustered
            // Tend toward front-loaded burst with trailing gaps
            float t = (float)i / burst.triggerCount;
            float curve = pow(t, 0.5f + shape);  // Steeper at high shape
            burst.triggerTimes[i] = curve * burstDurationMs;

            // Add micro-jitter
            float jitter = (RandomFloat() - 0.5f) * intervalBase * 0.5f;
            burst.triggerTimes[i] += jitter;
        }

        // ═══════════════════════════════════════════════════════════════
        // Velocity: accented start, decay toward end
        // ═══════════════════════════════════════════════════════════════
        float position = (float)i / burst.triggerCount;
        burst.velocities[i] = 1.0f - position * 0.4f;  // 1.0 → 0.6 decay

        // Add velocity variation based on SHAPE
        burst.velocities[i] += (RandomFloat() - 0.5f) * shape * 0.3f;
        burst.velocities[i] = Clamp(burst.velocities[i], 0.3f, 1.0f);
    }

    // ═══════════════════════════════════════════════════════════════
    // Pattern sensitivity: avoid main pattern hits
    // ═══════════════════════════════════════════════════════════════
    AdjustBurstForMainPattern(burst, mainPattern, currentStep);

    return burst;
}

void AdjustBurstForMainPattern(HatBurst& burst, const Pattern& mainPattern,
                                int currentStep) {
    // Find main pattern hits within burst window
    // Reduce hat velocity near main hits (let kicks/snares punch through)

    float stepDurationMs = 60000.0f / bpm / 4.0f;  // 16th note duration

    for (int i = 0; i < burst.triggerCount; i++) {
        float burstTimeMs = burst.triggerTimes[i];
        int nearestStep = currentStep + (int)(burstTimeMs / stepDurationMs);
        nearestStep = nearestStep % patternLength;

        // Check if main pattern has a hit near this burst trigger
        if (mainPattern.v1.hits[nearestStep] || mainPattern.v2.hits[nearestStep]) {
            // Reduce hat velocity to not compete with main hit
            burst.velocities[i] *= 0.5f;
        }
    }
}
```

### Visual: Hat Burst Patterns

```
ENERGY=25%, SHAPE=0% (sparse, regular):
Time: |-----|-----|-----|-----|-----|
Hats: x     x     x
      (3 hats, evenly spaced)

ENERGY=75%, SHAPE=0% (dense, regular):
Time: |-----|-----|-----|-----|-----|
Hats: x  x  x  x  x  x  x  x  x
      (9 hats, evenly spaced, machine gun)

ENERGY=25%, SHAPE=100% (sparse, irregular):
Time: |-----|-----|-----|-----|-----|
Hats: xx          x
      (3 hats, clustered then sparse)

ENERGY=75%, SHAPE=100% (dense, irregular):
Time: |-----|-----|-----|-----|-----|
Hats: xxxx    x x   xxx     x
      (9 hats, clusters and gaps, organic flam)
```

---

## Button Behavior Update

Fill now triggers hat burst OR sets fill gate based on AUX MODE.

```cpp
void TriggerFill(float energy, float shape, AuxMode auxMode,
                 const Pattern& mainPattern, int currentStep) {

    fill.active = true;
    fill.intensityMultiplier = 0.5f + energy * 1.0f;
    fill.remainingSteps = patternLength;

    // Apply fill modifications to pattern
    ApplyFillModifications(energy, shape);

    // ═══════════════════════════════════════════════════════════════
    // AUX output based on mode
    // ═══════════════════════════════════════════════════════════════

    if (auxMode == AUX_HAT) {
        // Generate and schedule hat burst
        float burstDurationMs = (60000.0f / bpm) * 4;  // 1 bar
        HatBurst burst = GenerateHatBurst(
            energy, shape, burstDurationMs,
            mainPattern, currentStep
        );
        ScheduleHatBurst(burst);
    }
    else {  // AUX_FILL
        // Simple: gate high for fill duration
        SetFillGateHigh();
    }
}
```

---

## Summary: Changes from Iteration 5

| Aspect | Iteration 5 | Iteration 6 |
|--------|-------------|-------------|
| LED feedback | TBD | **Fully specified** (8 behaviors) |
| Voice relationship | Unspecified | **Call/response with 5 relationship types** |
| Relationship selection | None | **Automatic based on AXIS Y** |
| Extensibility | Not considered | **Variation profiles architecture** |
| Hat burst | Basic | **ENERGY + SHAPE sensitive, pattern-aware** |

---

## Open Items

1. **Variation profile selection** - How to switch between percussive/bassline/ambient? (boot-time? config menu?)
2. **Pattern generator implementations** - Full code for bassline/ambient generators
3. **Velocity curve tuning** - May need adjustment after hardware testing
4. **LED timing constants** - May need adjustment for visual appeal
