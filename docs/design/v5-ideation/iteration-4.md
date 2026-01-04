# V5 Design Iteration 4: Final Control Layout + SHAPE Algorithm

**Date**: 2026-01-03
**Focus**: AXIS naming, BUILD removal, SHAPE algorithm specification

---

## Design Summary

V5 is a radical simplification focused on **algorithm-first** pattern generation with intuitive 2D navigation.

### Core Philosophy

- **4 performance controls, zero shift** - maximum immediacy
- **CV Law** - CV 1-4 always map to K1-K4 primaries
- **BUILD externalized** - users modulate performance controls via external CV for phrase dynamics
- **SHAPE as core innovation** - euclidean ↔ wild morphing with humanization

---

## V5 Performance Mode (Final)

```
┌─────────────────────────────────────────────────────────────┐
│  K1: ENERGY      K2: SHAPE        K3: AXIS X     K4: AXIS Y │
│  CV1             CV2              CV3            CV4        │
│                                                             │
│  Sparse ↔ Busy   Stable ↔ Wild    Grounded ↔     Simple ↔   │
│  (hit count)     (regularity)     Floating       Complex    │
└─────────────────────────────────────────────────────────────┘
```

### Parameter Definitions

| Param | Name | Domain | 0% | 100% |
|-------|------|--------|-----|------|
| K1 | **ENERGY** | Density | Sparse (few hits) | Busy (many hits) |
| K2 | **SHAPE** | Regularity | Stable (humanized euclidean) | Wild (weighted chaos) |
| K3 | **AXIS X** | Beat position | Grounded (downbeats) | Floating (offbeats) |
| K4 | **AXIS Y** | Intricacy | Simple (obvious pattern) | Complex (intricate pattern) |

### Parameter Interactions

```
                    AXIS Y
            Simple ◄─────────► Complex
               │                  │
    AXIS X     │    ENERGY        │
  Grounded ────┼──── sets ────────┼──── Floating
               │   hit count      │
               │                  │
               │    SHAPE         │
               │   sets how       │
               │   hits placed    │
               ▼                  ▼

At any ENERGY level:
- SHAPE determines regularity (euclidean ↔ weighted)
- AXIS X biases toward downbeats or offbeats
- AXIS Y biases toward simple or complex rhythms

Example: ENERGY=50%, SHAPE=0%, AXIS X=0%, AXIS Y=0%
→ Medium density, euclidean spacing, downbeat emphasis, simple pattern
→ Classic four-on-floor with humanization

Example: ENERGY=50%, SHAPE=100%, AXIS X=100%, AXIS Y=100%
→ Medium density, weighted chaos, offbeat emphasis, complex pattern
→ Broken, syncopated, intricate rhythm
```

### Why "AXIS" Works with "DuoPulse"

The name evokes:
- 2D coordinate system (X/Y navigation)
- "Pulse" positioned along two axes
- Spatial metaphor for musical exploration
- Neutral naming that doesn't imply action (vs "SEEK")

---

## SHAPE Algorithm (Specified)

SHAPE morphs between two hit placement engines using a weighted blend.

### Core Algorithm

```cpp
// SHAPE controls the blend between euclidean and weighted placement
// Range: 0.0 (pure euclidean) to 1.0 (pure weighted)

struct HitPlacement {
    bool euclidean;   // Did euclidean algorithm want this step?
    bool weighted;    // Did weighted algorithm want this step?
    float eucWeight;  // Euclidean's contribution (1.0 - shape)
    float wtdWeight;  // Weighted's contribution (shape)
};

bool ShouldStepFire(int step, float energy, float shape,
                    float axisX, float axisY, uint32_t seed) {

    int hitBudget = ComputeHitBudget(energy);  // ENERGY sets total hits

    // ═══════════════════════════════════════════════════════════════
    // STEP 1: Compute euclidean placement (SHAPE=0% contribution)
    // ═══════════════════════════════════════════════════════════════

    // Euclidean distributes hitBudget hits evenly across patternLength
    // AXIS X controls rotation (0% = no rotation, 100% = max rotation)
    // AXIS Y modifies euclidean density within budget

    int euclideanHits = hitBudget;
    int rotation = (int)(axisX * patternLength);
    bool euclideanFires = EuclideanStep(step, euclideanHits, patternLength, rotation);

    // ═══════════════════════════════════════════════════════════════
    // STEP 2: Compute weighted placement (SHAPE=100% contribution)
    // ═══════════════════════════════════════════════════════════════

    // Weighted uses archetype weight tables influenced by AXIS X/Y
    // AXIS X biases weights toward downbeats (0%) or offbeats (100%)
    // AXIS Y biases weights toward simple (0%) or complex (100%) patterns

    float baseWeight = GetBaseWeight(step);  // 0.0-1.0 per step
    float axisXBias = ComputeAxisXBias(step, axisX);  // Shift weight by beat position
    float axisYBias = ComputeAxisYBias(step, axisY);  // Shift weight by complexity

    float effectiveWeight = baseWeight * axisXBias * axisYBias;

    // Gumbel Top-K selection (deterministic given seed)
    bool weightedFires = GumbelTopK(step, effectiveWeight, hitBudget, seed);

    // ═══════════════════════════════════════════════════════════════
    // STEP 3: Blend euclidean and weighted based on SHAPE
    // ═══════════════════════════════════════════════════════════════

    // At SHAPE=0%: 100% euclidean, 0% weighted
    // At SHAPE=50%: 50% euclidean, 50% weighted (probabilistic blend)
    // At SHAPE=100%: 0% euclidean, 100% weighted

    float eucContribution = 1.0f - shape;
    float wtdContribution = shape;

    // Blend decision: weighted coin flip between the two results
    float blendRandom = HashToFloat(HashStep(seed, step));

    bool fires;
    if (blendRandom < eucContribution) {
        fires = euclideanFires;  // Use euclidean decision
    } else {
        fires = weightedFires;   // Use weighted decision
    }

    return fires;
}
```

### SHAPE Effects Stack (Timing Layer)

In addition to hit placement, SHAPE controls timing humanization:

```cpp
struct TimingEffects {
    float jitterMs;        // Microtiming variation
    float displacementProb; // Chance of ±1-2 step displacement
    int maxDisplacement;    // Maximum steps to displace
    float velocityVar;      // Velocity variation range
};

TimingEffects GetTimingEffects(float shape) {
    TimingEffects fx;

    // ═══════════════════════════════════════════════════════════════
    // SHAPE = 0% (Humanized Euclidean)
    // Precise but alive - subtle variations
    // ═══════════════════════════════════════════════════════════════
    if (shape < 0.25f) {
        fx.jitterMs = Lerp(2.0f, 4.0f, shape / 0.25f);
        fx.displacementProb = 0.0f;
        fx.maxDisplacement = 0;
        fx.velocityVar = Lerp(0.05f, 0.08f, shape / 0.25f);
    }

    // ═══════════════════════════════════════════════════════════════
    // SHAPE = 25-50% (Loose)
    // Noticeable swing, occasional displacement
    // ═══════════════════════════════════════════════════════════════
    else if (shape < 0.50f) {
        float t = (shape - 0.25f) / 0.25f;
        fx.jitterMs = Lerp(4.0f, 6.0f, t);
        fx.displacementProb = Lerp(0.0f, 0.10f, t);
        fx.maxDisplacement = 1;
        fx.velocityVar = Lerp(0.08f, 0.12f, t);
    }

    // ═══════════════════════════════════════════════════════════════
    // SHAPE = 50-75% (Broken)
    // Significant displacement, groove destruction begins
    // ═══════════════════════════════════════════════════════════════
    else if (shape < 0.75f) {
        float t = (shape - 0.50f) / 0.25f;
        fx.jitterMs = Lerp(6.0f, 10.0f, t);
        fx.displacementProb = Lerp(0.10f, 0.25f, t);
        fx.maxDisplacement = 1;
        fx.velocityVar = Lerp(0.12f, 0.18f, t);
    }

    // ═══════════════════════════════════════════════════════════════
    // SHAPE = 75-100% (Wild)
    // Maximum chaos while respecting hit budget
    // ═══════════════════════════════════════════════════════════════
    else {
        float t = (shape - 0.75f) / 0.25f;
        fx.jitterMs = Lerp(10.0f, 12.0f, t);
        fx.displacementProb = Lerp(0.25f, 0.40f, t);
        fx.maxDisplacement = 2;
        fx.velocityVar = Lerp(0.18f, 0.25f, t);
    }

    return fx;
}
```

### Visual: SHAPE Progression

```
SHAPE = 0% (Humanized Euclidean):
Pattern:  ●···●···●···●···  (mathematically spaced)
Timing:   |   |   |   |     (±2ms jitter, grid-aligned)
Velocity: █▓█▓█▓█▓          (±5% variation)
Feel:     Precise, mechanical, but alive

SHAPE = 25%:
Pattern:  ●···●···●···●···  (still euclidean-dominant)
Timing:   |  | |   | |      (±4ms jitter)
Velocity: █░█▓█░█▓          (±8% variation)
Feel:     Slightly loose, human drummer

SHAPE = 50%:
Pattern:  ●··●····●·●·····  (50% euclidean, 50% weighted)
Timing:   | |    | |        (±6ms, 10% displacement)
Velocity: █ ░█  ▓ █         (±12% variation)
Feel:     Hybrid, some surprise

SHAPE = 75%:
Pattern:  ●·●···●·····●·●·  (weighted-dominant)
Timing:   ||   |     | |    (±10ms, 25% displacement)
Velocity: █▓  █     ░ █     (±18% variation)
Feel:     Broken, syncopated

SHAPE = 100% (Wild):
Pattern:  ·●●····●·●····●·  (pure weighted probability)
Timing:   ||    | |    |    (±12ms, 40% displacement)
Velocity: ░█    ▓ █    ░    (±25% variation)
Feel:     Chaotic but musical (hit budget preserved)
```

---

## AXIS X: Grounded ↔ Floating

Controls weight bias based on beat position.

### Implementation

```cpp
float ComputeAxisXBias(int step, float axisX) {
    // Step positions and their "groundedness"
    // 0.0 = pure downbeat, 1.0 = pure offbeat

    static const float kStepGroundedness[16] = {
        0.0f,  // 0: Downbeat (bar)
        1.0f,  // 1: 16th offbeat
        0.5f,  // 2: 8th offbeat
        1.0f,  // 3: 16th offbeat
        0.25f, // 4: Quarter
        1.0f,  // 5: 16th offbeat
        0.5f,  // 6: 8th offbeat
        1.0f,  // 7: 16th offbeat
        0.1f,  // 8: Half (backbeat)
        1.0f,  // 9: 16th offbeat
        0.5f,  // 10: 8th offbeat
        1.0f,  // 11: 16th offbeat
        0.25f, // 12: Quarter
        1.0f,  // 13: 16th offbeat
        0.5f,  // 14: 8th offbeat
        1.0f,  // 15: 16th offbeat
    };

    float groundedness = kStepGroundedness[step % 16];

    // AXIS X = 0%: Favor grounded steps (multiply grounded by 2x)
    // AXIS X = 100%: Favor floating steps (multiply floating by 2x)

    if (axisX < 0.5f) {
        // Favor grounded: boost low-groundedness steps
        float boost = (0.5f - axisX) * 2.0f;  // 0-1 range
        return 1.0f + boost * (1.0f - groundedness);
    } else {
        // Favor floating: boost high-groundedness steps
        float boost = (axisX - 0.5f) * 2.0f;  // 0-1 range
        return 1.0f + boost * groundedness;
    }
}
```

---

## AXIS Y: Simple ↔ Complex

Controls weight bias based on pattern intricacy.

### Implementation

```cpp
float ComputeAxisYBias(int step, float axisY) {
    // Step positions and their "complexity contribution"
    // Simple patterns use only strong beats
    // Complex patterns use unusual subdivisions

    static const float kStepComplexity[16] = {
        0.0f,  // 0: Downbeat - simple
        1.0f,  // 1: 16th - complex
        0.3f,  // 2: 8th - medium
        0.9f,  // 3: 16th e-and - complex
        0.1f,  // 4: Quarter - simple
        1.0f,  // 5: 16th - complex
        0.4f,  // 6: 8th and - medium
        0.8f,  // 7: 16th a - complex
        0.0f,  // 8: Backbeat - simple
        1.0f,  // 9: 16th - complex
        0.3f,  // 10: 8th - medium
        0.9f,  // 11: 16th - complex
        0.1f,  // 12: Quarter - simple
        1.0f,  // 13: 16th - complex
        0.5f,  // 14: 8th - medium (pre-downbeat)
        0.7f,  // 15: 16th - complex (pickup)
    };

    float complexity = kStepComplexity[step % 16];

    // AXIS Y = 0%: Favor simple steps
    // AXIS Y = 100%: Favor complex steps

    if (axisY < 0.5f) {
        // Favor simple: boost low-complexity steps
        float boost = (0.5f - axisY) * 2.0f;
        return 1.0f + boost * (1.0f - complexity);
    } else {
        // Favor complex: boost high-complexity steps
        float boost = (axisY - 0.5f) * 2.0f;
        return 1.0f + boost * complexity;
    }
}
```

---

## V5 Config Mode (Simplified)

With BUILD removed, config mode is streamlined:

```
┌─────────────────────────────────────────────────────────────┐
│  K1: LENGTH      K2: DRIFT        K3: CLOCK DIV  K4: (free) │
│  (no shift)      +S: SWING        +S: AUX MODE   (no shift) │
└─────────────────────────────────────────────────────────────┘
```

| Knob | Primary | +Shift | Notes |
|------|---------|--------|-------|
| K1 | LENGTH (16/24/32/64) | - | Pattern length |
| K2 | DRIFT (0-100%) | SWING (0-100%) | Evolution + timing feel |
| K3 | CLOCK DIV (÷4 to ×4) | AUX MODE (Hat/Fill) | Tempo + output |
| K4 | *(free)* | - | Reserved for future |

### Config Control Details

**LENGTH** (K1): Pattern length in steps
- 16 = 1 bar
- 24 = 1.5 bars (shuffle-friendly)
- 32 = 2 bars (default)
- 64 = 4 bars (long form)

**DRIFT** (K2): Pattern evolution rate
- 0% = locked (identical every phrase)
- 50% = subtle evolution
- 100% = significant variation each phrase

**CLOCK DIV** (K3): Clock division/multiplication
- ÷4, ÷2, ×1, ×2, ×4

**SWING** (K2+Shift): Timing feel
- 0% = straight
- 50% = subtle shuffle
- 100% = heavy triplet

**AUX MODE** (K3+Shift): Aux output function
- HAT: Third voice trigger
- FILL: Gate high during fills

### Why BUILD Was Removed

1. **External automation**: Users can patch LFO → CV inputs for phrase dynamics
2. **Reduces config complexity**: One less thing to configure
3. **Performance control CV is more flexible**: Modulating ENERGY or AXIS Y achieves similar effect
4. **Pattern length less critical**: Without BUILD's phrase arc, exact length matters less

---

## Button Behavior (Final)

```
┌─────────────────────────────────────────────────────────────┐
│  TAP:           Trigger fill (on/off gate)                  │
│  HOLD 3 SEC:    Reseed pattern (new random seed)            │
│                 LED feedback indicates reseed progress      │
│                 (LED behavior TBD in LED feedback spec)     │
└─────────────────────────────────────────────────────────────┘
```

### Button State Machine

```cpp
enum class ButtonState {
    IDLE,
    PRESSED,
    FILL_ACTIVE,
    RESEED_PENDING,  // Held > threshold, awaiting release
    RESEED_COMPLETE
};

void ProcessButton(bool pressed, float holdTimeMs) {
    const float RESEED_THRESHOLD_MS = 3000.0f;

    if (pressed && holdTimeMs >= RESEED_THRESHOLD_MS) {
        // LED feedback: indicate reseed ready (TBD)
        state = RESEED_PENDING;
    }
    else if (pressed) {
        // LED feedback: indicate fill active or reseed progress (TBD)
        TriggerFill();
        state = FILL_ACTIVE;
    }
    else if (!pressed && state == RESEED_PENDING) {
        // Release after 3 seconds = reseed
        ReseedPattern();
        state = RESEED_COMPLETE;
    }
    else {
        state = IDLE;
    }
}
```

---

## AUX Output Behavior (Unchanged)

| Condition | AUX Output (CV Out 1) |
|-----------|----------------------|
| **Clock unpatched** | Internal clock output |
| **Clock patched, AUX=HAT** | Third voice trigger |
| **Clock patched, AUX=FILL** | Gate high during fills |

---

## LED Feedback Requirements (TBD)

The following LED behaviors need to be specified in a future LED feedback document:

1. **Normal operation**: Brightness indicates trigger activity
2. **Fill active**: Distinct indication when fill is triggered
3. **Reseed progress**: Visual feedback during 3-second hold
4. **Reseed complete**: Confirmation flash when reseed occurs
5. **Config mode**: Distinguish from performance mode
6. **Clock sync**: Pulse on clock to show sync status

---

## Summary: V4 → V5 Changes

### Performance Mode

| Aspect | v4 | v5 |
|--------|----|----|
| Parameters | ENERGY, BUILD, FIELD X, FIELD Y | ENERGY, SHAPE, AXIS X, AXIS Y |
| Shift functions | 4 (PUNCH, GENRE, DRIFT, BALANCE) | **0** |
| Paradigm | Genre-based archetype grid | **Algorithm-based (euclidean ↔ wild)** |
| BUILD | Performance control | **Removed** (external CV) |
| Navigation naming | FIELD X/Y | **AXIS X/Y** |

### Config Mode

| Aspect | v4 | v5 |
|--------|----|----|
| Shift functions | 4 | **2** |
| Controls | 8 total | **5 total** (3 primary + 2 shift) |
| BUILD | Config+Shift K1 | **Removed** |
| Free knob | None | **K4** |

### Removed Parameters

| Parameter | v4 Location | Reason for Removal |
|-----------|-------------|-------------------|
| GENRE | Perf+Shift K2 | Algorithm-focused, not genre-specific |
| PUNCH | Perf+Shift K1 | External VCAs handle dynamics |
| BALANCE | Perf+Shift K4 | Voice coupling not valued |
| BUILD | Config K2 | External CV automation |
| VOICE COUPLING | Config+Shift K4 | Not valued |
| AUX DENSITY | Config+Shift K3 | Over-complication |
| RESET MODE | Config K4 | Hardcoded to STEP |
| PHRASE LENGTH | Config+Shift K1 | Auto-derived |

---

## Open Items

1. **LED feedback specification**: Full document needed for all LED behaviors
2. **DRIFT algorithm**: How exactly does pattern evolution work per phrase?
3. **Fill algorithm**: How does button-triggered fill modify the pattern?
4. **K4 config usage**: Reserved for future - what might go here?
