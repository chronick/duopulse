# V5 Design Iteration 5: Algorithm Fixes + Knob Pairing

**Date**: 2026-01-04
**Focus**: Deterministic SHAPE, additive AXIS, knob pairing, fill algorithm

---

## Design Principles (Final)

1. **CV Law**: CV 1-4 always map to K1-K4 performance primaries
2. **Zero shift in performance mode**: Maximum immediacy
3. **Knob pairing**: Same physical knob = related concept across modes
4. **Deterministic algorithms**: Musicality through design, not randomness
5. **ENERGY as master modifier**: All effects scale with current energy level

---

## Knob Pairing: Performance ↔ Config

Each knob position has a related concept across modes:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           KNOB PAIRING                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Position │ Performance      │ Config           │ Shared Domain             │
├───────────┼──────────────────┼──────────────────┼───────────────────────────┤
│  K1       │ ENERGY           │ CLOCK DIV        │ Rate / Density            │
│           │ (hit density)    │ (tempo scale)    │                           │
├───────────┼──────────────────┼──────────────────┼───────────────────────────┤
│  K2       │ SHAPE            │ SWING            │ Timing Feel               │
│           │ (algorithm feel) │ (groove offset)  │                           │
├───────────┼──────────────────┼──────────────────┼───────────────────────────┤
│  K3       │ AXIS X           │ DRIFT            │ Variation / Movement      │
│           │ (grounded↔float) │ (evolution rate) │ +Shift: AUX MODE          │
├───────────┼──────────────────┼──────────────────┼───────────────────────────┤
│  K4       │ AXIS Y           │ LENGTH           │ Structure / Form          │
│           │ (simple↔complex) │ (pattern length) │                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Why This Pairing Works

**K1 - Rate/Density Domain**:
- ENERGY: How many hits per bar (sparse ↔ busy)
- CLOCK DIV: How fast the pattern runs (÷4 to ×4)
- Both control "how much is happening"

**K2 - Timing Feel Domain**:
- SHAPE: Algorithmic timing character (stable ↔ wild)
- SWING: Groove offset (straight ↔ shuffle)
- Both control "how it feels rhythmically"

**K3 - Variation Domain**:
- AXIS X: Beat position bias (grounded ↔ floating)
- DRIFT: Pattern evolution rate (locked ↔ evolving)
- Both control "how much things move/change"

**K4 - Structure Domain**:
- AXIS Y: Pattern intricacy (simple ↔ complex)
- LENGTH: Pattern length (16/24/32/64)
- Both control "form and complexity"

---

## V5 Performance Mode (Final)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  K1: ENERGY      K2: SHAPE        K3: AXIS X         K4: AXIS Y            │
│  CV1             CV2              CV3                CV4                   │
│                                                                             │
│  Sparse ↔ Busy   Stable ↔ Wild    Grounded ↔         Simple ↔              │
│  (density)       (algorithm)      Floating           Complex               │
└─────────────────────────────────────────────────────────────────────────────┘
```

**No shift layer. CV 1-4 map directly. Button triggers fill.**

---

## V5 Config Mode (Final)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  K1: CLOCK DIV   K2: SWING        K3: DRIFT          K4: LENGTH            │
│                                   +S: AUX MODE                              │
│                                                                             │
│  ÷4 to ×4        0-100%           0-100%             16/24/32/64           │
│  (tempo)         (groove)         (evolution)        (steps)               │
└─────────────────────────────────────────────────────────────────────────────┘
```

| Knob | Primary | +Shift |
|------|---------|--------|
| K1 | CLOCK DIV | - |
| K2 | SWING | - |
| K3 | DRIFT | AUX MODE (Hat/Fill) |
| K4 | LENGTH | - |

**Only 1 shift function in config mode** (AUX MODE on K3).

---

## SHAPE Algorithm: Deterministic Score Interpolation

Replaces coin-flip blending with weighted score interpolation.

### Core Algorithm (Revised)

```cpp
bool ShouldStepFire(int step, float energy, float shape,
                    float axisX, float axisY, uint32_t seed) {

    int hitBudget = ComputeHitBudget(energy, patternLength);

    // ═══════════════════════════════════════════════════════════════
    // STEP 1: Compute euclidean score (SHAPE=0% contribution)
    // ═══════════════════════════════════════════════════════════════
    // Score represents "how much this step wants to fire" (0.0 to 1.0)

    float euclideanScore = ComputeEuclideanScore(step, hitBudget, patternLength);
    // Returns 1.0 for steps in euclidean pattern, 0.0 otherwise
    // Includes rotation based on axisX for euclidean mode

    // ═══════════════════════════════════════════════════════════════
    // STEP 2: Compute weighted score (SHAPE=100% contribution)
    // ═══════════════════════════════════════════════════════════════

    float baseWeight = GetArchetypeBaseWeight(step);  // 0.0-1.0

    // ADDITIVE biasing (not multiplicative)
    float axisXOffset = ComputeAxisXOffset(step, axisX);  // -0.3 to +0.3
    float axisYOffset = ComputeAxisYOffset(step, axisY);  // -0.3 to +0.3

    float weightedScore = Clamp(baseWeight + axisXOffset + axisYOffset, 0.0f, 1.0f);

    // ═══════════════════════════════════════════════════════════════
    // STEP 3: DETERMINISTIC score interpolation based on SHAPE
    // ═══════════════════════════════════════════════════════════════
    // No coin flip! Smooth blend of scores.

    float blendedScore = Lerp(euclideanScore, weightedScore, shape);

    // ═══════════════════════════════════════════════════════════════
    // STEP 4: Top-K selection on blended scores
    // ═══════════════════════════════════════════════════════════════
    // Select top hitBudget steps by blended score (deterministic given seed)

    return GumbelTopK(step, blendedScore, hitBudget, seed);
}
```

### Key Change: Score Interpolation vs Coin Flip

**Before (coin flip)**:
```cpp
// Each step flips a coin to decide which algorithm to use
if (random < shape) {
    fires = weightedFires;
} else {
    fires = euclideanFires;
}
// Result: Groove varies phrase-to-phrase even at fixed parameters
```

**After (score interpolation)**:
```cpp
// Each step gets a blended score from both algorithms
float blendedScore = (1-shape) * euclideanScore + shape * weightedScore;
// Result: Consistent groove at any SHAPE value
```

### Visual: SHAPE Progression (Deterministic)

```
SHAPE = 0% (Humanized Euclidean):
Scores:   [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, ...]  (euclidean)
Pattern:  ●···●···●···●···
Feel:     Predictable, consistent every phrase

SHAPE = 50% (Hybrid):
Scores:   [0.7, 0.2, 0.15, 0.1, 0.6, 0.25, 0.1, 0.15, ...]  (blended)
Pattern:  ●·○·●··○●···○···  (○ = possible with DRIFT)
Feel:     Some euclidean structure, some weighted variation

SHAPE = 100% (Wild):
Scores:   [0.4, 0.5, 0.3, 0.2, 0.3, 0.5, 0.2, 0.3, ...]  (weighted)
Pattern:  ·●●···●·●···●·●·
Feel:     Weighted probability, but consistent per seed
```

---

## AXIS Biasing: Additive Offsets

Replaces multiplicative biasing to prevent dead zones at low ENERGY.

### AXIS X: Grounded ↔ Floating (Additive)

```cpp
float ComputeAxisXOffset(int step, float axisX) {
    // Step positions: 0.0 = downbeat, 1.0 = offbeat
    static const float kStepOffbeatness[16] = {
        0.0f,  // 0: Downbeat (bar) - most grounded
        1.0f,  // 1: 16th offbeat - most floating
        0.5f,  // 2: 8th
        0.9f,  // 3: 16th e-and
        0.25f, // 4: Quarter
        1.0f,  // 5: 16th
        0.5f,  // 6: 8th and
        0.8f,  // 7: 16th a
        0.1f,  // 8: Backbeat
        1.0f,  // 9: 16th
        0.5f,  // 10: 8th
        0.9f,  // 11: 16th
        0.25f, // 12: Quarter
        1.0f,  // 13: 16th
        0.5f,  // 14: 8th
        0.7f,  // 15: 16th (pickup)
    };

    float offbeatness = kStepOffbeatness[step % 16];

    // AXIS X centered at 0.5
    // axisX < 0.5: favor grounded (boost low offbeatness)
    // axisX > 0.5: favor floating (boost high offbeatness)

    float axisBias = (axisX - 0.5f) * 2.0f;  // -1.0 to +1.0

    // Additive offset: ±0.3 max
    // At axisX=0% (grounded): grounded steps get +0.3, floating get -0.3
    // At axisX=100% (floating): floating steps get +0.3, grounded get -0.3

    float offset = axisBias * (offbeatness - 0.5f) * 0.6f;  // -0.3 to +0.3

    return offset;
}
```

### AXIS Y: Simple ↔ Complex (Additive)

```cpp
float ComputeAxisYOffset(int step, float axisY) {
    // Step complexity: 0.0 = simple, 1.0 = complex
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

    // AXIS Y centered at 0.5
    float axisBias = (axisY - 0.5f) * 2.0f;  // -1.0 to +1.0

    // Additive offset: ±0.3 max
    float offset = axisBias * (complexity - 0.5f) * 0.6f;

    return offset;
}
```

### Why Additive Works Better

**Multiplicative (old)**:
```
At ENERGY=12% (2 hits from 16 steps):
- All "grounded" steps have similar high weights (1.8-2.0x)
- Top-2 selection barely changes with AXIS movement
- AXIS feels "stuck"
```

**Additive (new)**:
```
At ENERGY=12% (2 hits from 16 steps):
- AXIS X=0%: Step 0 gets +0.3, Step 1 gets -0.3
- AXIS X=100%: Step 0 gets -0.3, Step 1 gets +0.3
- Clear re-ranking even with few hits
- AXIS remains effective at all ENERGY levels
```

---

## DRIFT Algorithm (Specified)

DRIFT controls how much the pattern evolves phrase-to-phrase.

```cpp
// DRIFT modifies the seed used for hit selection each phrase
// Result: Controlled variation in which steps fire

uint32_t GetPhraseSeed(uint32_t baseSeed, int phraseNumber, float drift) {
    if (drift < 0.01f) {
        // DRIFT = 0%: Same seed every phrase (locked)
        return baseSeed;
    }

    // DRIFT > 0%: Mix phrase number into seed
    // Higher drift = more weight on phrase variation

    int driftFactor = (int)(drift * 8);  // 0-8 levels of drift
    uint32_t phraseContribution = HashInt(phraseNumber) >> (8 - driftFactor);

    return baseSeed ^ phraseContribution;
}

// Effect on pattern:
// DRIFT = 0%:   Identical pattern every phrase
// DRIFT = 25%:  ~5% of hits may differ phrase-to-phrase
// DRIFT = 50%:  ~15% of hits may differ
// DRIFT = 75%:  ~25% of hits may differ
// DRIFT = 100%: ~35% of hits may differ (still recognizable, not chaos)
```

### DRIFT + SHAPE Interaction

Both are deterministic and complementary:
- **SHAPE** controls the CHARACTER of each pattern (stable vs wild)
- **DRIFT** controls how much the pattern CHANGES between phrases

At SHAPE=0% + DRIFT=0%: Same euclidean pattern every phrase (locked)
At SHAPE=0% + DRIFT=100%: Euclidean character, but hits rotate/shift per phrase
At SHAPE=100% + DRIFT=0%: Same wild pattern every phrase (wild but locked)
At SHAPE=100% + DRIFT=100%: Wild and evolving (maximum variation)

---

## Fill Algorithm (Specified)

Fill is a "performance burst" that respects current settings.

### Fill Behavior

```cpp
struct FillState {
    bool active;
    float intensityMultiplier;  // Based on ENERGY at fill start
    int remainingSteps;         // Fill duration
};

void TriggerFill(float currentEnergy, float currentShape, AuxMode auxMode) {
    fill.active = true;

    // ═══════════════════════════════════════════════════════════════
    // ENERGY scales fill intensity
    // ═══════════════════════════════════════════════════════════════
    // Low ENERGY = subtle fill, High ENERGY = intense fill

    fill.intensityMultiplier = 0.5f + currentEnergy * 1.0f;  // 0.5x to 1.5x

    // Fill duration: 1 bar (patternLength steps)
    fill.remainingSteps = patternLength;

    // ═══════════════════════════════════════════════════════════════
    // Trigger aux output based on AUX MODE and SHAPE
    // ═══════════════════════════════════════════════════════════════

    if (auxMode == AUX_HAT) {
        // Hat burst: rapid triggers scaled by SHAPE
        // SHAPE low = steady 16th hat burst
        // SHAPE high = irregular hat flurry
        TriggerHatBurst(currentShape);
    } else {  // AUX_FILL
        // Fill gate: hold high for fill duration
        SetFillGateHigh();
    }
}

float GetFillModifiedEnergy(float baseEnergy, FillState& fill) {
    if (!fill.active) return baseEnergy;

    // Boost energy during fill, scaled by intensity multiplier
    float boostedEnergy = baseEnergy + 0.3f * fill.intensityMultiplier;
    return Clamp(boostedEnergy, 0.0f, 1.0f);
}

float GetFillModifiedShape(float baseShape, FillState& fill) {
    if (!fill.active) return baseShape;

    // Push shape toward wild during fill
    float shapeBoost = 0.25f * fill.intensityMultiplier;
    return Clamp(baseShape + shapeBoost, 0.0f, 1.0f);
}

float GetFillModifiedAxisY(float baseAxisY, FillState& fill) {
    if (!fill.active) return baseAxisY;

    // Push toward complex during fill
    float complexityBoost = 0.2f * fill.intensityMultiplier;
    return Clamp(baseAxisY + complexityBoost, 0.0f, 1.0f);
}
```

### Fill Effect Summary

| ENERGY at fill | Fill Intensity | Effect |
|----------------|----------------|--------|
| 0% (sparse) | 0.5x | Subtle: +15% energy, +12% shape, +10% complexity |
| 50% (medium) | 1.0x | Moderate: +30% energy, +25% shape, +20% complexity |
| 100% (busy) | 1.5x | Intense: +45% energy, +37% shape, +30% complexity |

### Hat Burst Behavior (when AUX = HAT)

```cpp
void TriggerHatBurst(float shape) {
    // Number of hat triggers in burst
    int burstCount = 4 + (int)(shape * 8);  // 4-12 triggers

    // Timing between triggers (shaped by SHAPE)
    if (shape < 0.3f) {
        // Steady 16th notes
        ScheduleHatBurst(burstCount, SIXTEENTH_NOTE_INTERVAL);
    } else if (shape < 0.7f) {
        // Mixed 16ths and 32nds
        ScheduleHatBurstMixed(burstCount);
    } else {
        // Irregular flurry (32nds with gaps)
        ScheduleHatBurstIrregular(burstCount);
    }
}
```

---

## Button State Machine (Fixed)

Addresses the continuous-fill bug from previous iteration.

```cpp
enum class ButtonState {
    IDLE,
    FILL_ACTIVE,
    RESEED_PENDING
};

class ButtonHandler {
    ButtonState state = IDLE;
    float holdStartTime = 0;
    bool fillTriggered = false;

    static constexpr float DEBOUNCE_MS = 20.0f;
    static constexpr float RESEED_THRESHOLD_MS = 3000.0f;

public:
    void Process(bool pressed, float currentTimeMs) {
        float holdDuration = pressed ? (currentTimeMs - holdStartTime) : 0;

        // ═══════════════════════════════════════════════════════════════
        // BUTTON RELEASED
        // ═══════════════════════════════════════════════════════════════
        if (!pressed) {
            if (state == RESEED_PENDING) {
                // Release after 3 seconds = reseed
                ReseedPattern();
                // LED: confirmation flash (TBD)
            } else if (state == FILL_ACTIVE) {
                // Release before 3 seconds = end fill normally
                EndFill();
            }
            state = IDLE;
            fillTriggered = false;
            return;
        }

        // ═══════════════════════════════════════════════════════════════
        // BUTTON PRESSED
        // ═══════════════════════════════════════════════════════════════
        if (state == IDLE) {
            holdStartTime = currentTimeMs;
        }

        if (holdDuration >= RESEED_THRESHOLD_MS) {
            // Held 3+ seconds: ready to reseed on release
            state = RESEED_PENDING;
            // LED: reseed ready indicator (TBD)
        }
        else if (holdDuration > DEBOUNCE_MS) {
            // Held past debounce: this is a fill
            if (!fillTriggered) {
                TriggerFill();  // Only trigger ONCE
                fillTriggered = true;
            }
            state = FILL_ACTIVE;
            // LED: fill active + progress toward reseed (TBD)
        }
    }
};
```

### Key Fixes

1. **Fill triggers ONCE** when entering FILL_ACTIVE state (not every frame)
2. **Reseed happens on RELEASE** after 3 seconds (not while held)
3. **Clear state separation**: IDLE → FILL_ACTIVE → RESEED_PENDING
4. **LED feedback points marked** for future specification

---

## Hit Budget Scaling (Clarified)

ENERGY always means "this percentage of the pattern is filled."

```cpp
int ComputeHitBudget(float energy, int patternLength) {
    // ENERGY 0% = 1 hit minimum (never silent)
    // ENERGY 100% = all steps eligible

    int minHits = 1;
    int maxHits = patternLength;

    int budget = minHits + (int)(energy * (maxHits - minHits));

    return Clamp(budget, minHits, maxHits);
}
```

| LENGTH | ENERGY 0% | ENERGY 25% | ENERGY 50% | ENERGY 75% | ENERGY 100% |
|--------|-----------|------------|------------|------------|-------------|
| 16 | 1 hit | 4 hits | 8 hits | 12 hits | 16 hits |
| 32 | 1 hit | 8 hits | 16 hits | 24 hits | 32 hits |
| 64 | 1 hit | 16 hits | 32 hits | 48 hits | 64 hits |

---

## Timing Effects (Unchanged from v4)

Still scales with SHAPE, applied after hit placement:

| SHAPE Range | Jitter | Displacement | Max Displace | Velocity Var |
|-------------|--------|--------------|--------------|--------------|
| 0-25% | ±2-4ms | 0% | 0 steps | ±5-8% |
| 25-50% | ±4-6ms | 0-10% | 1 step | ±8-12% |
| 50-75% | ±6-10ms | 10-25% | 1 step | ±12-18% |
| 75-100% | ±10-12ms | 25-40% | 2 steps | ±18-25% |

---

## LED Feedback Requirements (TBD)

Deferred to separate LED specification document:

1. **Normal operation**: Brightness = trigger activity
2. **Fill active**: Distinct pattern during fill
3. **Reseed progress**: 0-3s hold shows progress (pulse rate?)
4. **Reseed confirm**: Flash on reseed
5. **Config mode**: Distinguish from performance mode
6. **Clock sync**: Pulse on clock beats

---

## Summary: Changes from Iteration 4

| Aspect | Iteration 4 | Iteration 5 |
|--------|-------------|-------------|
| SHAPE blending | Coin-flip per step | **Deterministic score interpolation** |
| AXIS biasing | Multiplicative (dead zones) | **Additive offsets** |
| Knob pairing | Not specified | **Related concepts across modes** |
| Config K1 | LENGTH | **CLOCK DIV** (pairs with ENERGY) |
| Config K2 | DRIFT + SWING shift | **SWING** (pairs with SHAPE) |
| Config K3 | CLOCK DIV + AUX shift | **DRIFT + AUX shift** |
| Config K4 | Free | **LENGTH** (pairs with AXIS Y) |
| DRIFT algorithm | Unspecified | **Phrase seed mixing** |
| Fill algorithm | Unspecified | **Multi-attribute boost + aux output** |
| Button state machine | Buggy | **Fixed: trigger once, reseed on release** |
| Array naming | kStepGroundedness | **kStepOffbeatness** (clearer) |

---

## Open Items

1. **LED feedback specification** - Full document needed
2. **Aux Hat burst timing** - Exact implementation of mixed/irregular bursts
3. **Voice 2 relationship** - How does voice 2 pattern relate to voice 1? (TBD or inherited from v4)
