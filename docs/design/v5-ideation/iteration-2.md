# V5 Design Iteration 2: Refined Control Layout

**Date**: 2026-01-03
**Focus**: Finalized 4-control performance mode with 2D navigation

---

## Design Principles Established

From feedback round 2:

1. **CV Law**: CV 1-4 always map to K1-K4 primary functions (non-negotiable)
2. **PUNCH externalized**: VCAs handle dynamics; trigger-influencing params more valuable
3. **2D navigation required**: Two axes for full expressive control
4. **Aux simplified**: HAT + FILL BAR modes; outputs clock when unpatched

---

## V5 Performance Mode (Zero Shift)

```
┌─────────────────────────────────────────────────────────────┐
│  K1: ENERGY      K2: SHAPE        K3: SEEK X     K4: SEEK Y │
│  CV1             CV2              CV3            CV4        │
│  (density)       (regular↔chaos)  (grounded↔     (sparse↔   │
│                                    floating)      busy)     │
└─────────────────────────────────────────────────────────────┘
```

| Knob | Name | Function | Range |
|------|------|----------|-------|
| K1 | **ENERGY** | Overall hit density | 0% = minimal → 100% = maximum |
| K2 | **SHAPE** | Euclidean ↔ Irregular blend | 0% = mathematical → 100% = musical-random |
| K3 | **SEEK X** | Pattern character | 0% = grounded (downbeats) → 100% = floating (offbeats) |
| K4 | **SEEK Y** | Pattern complexity | 0% = sparse (quarters) → 100% = busy (16ths) |

### CV Modulation (The Law)

| CV Input | Modulates | Behavior |
|----------|-----------|----------|
| CV 1 | ENERGY | Bipolar ±50% |
| CV 2 | SHAPE | Bipolar ±50% |
| CV 3 | SEEK X | Bipolar ±50% |
| CV 4 | SEEK Y | Bipolar ±50% |

### Parameter Interactions

**ENERGY** sets the hit budget (how many hits per bar).

**SHAPE** controls HOW those hits are placed:
```
SHAPE 0% (Euclidean):
├── Hits are mathematically spaced
├── SEEK X rotates the euclidean pattern
├── SEEK Y controls euclidean density (hits per cycle)
└── Result: Precise, mechanical, predictable

SHAPE 100% (Irregular):
├── Hits are placed by weighted probability
├── SEEK X biases toward downbeats vs offbeats
├── SEEK Y biases toward simple vs complex rhythms
└── Result: Human, surprising, musical
```

**SEEK X/Y** navigate within the current SHAPE's paradigm:
- At SHAPE=0: Euclidean parameters (rotation, density)
- At SHAPE=100: Archetype blending (character, complexity)
- At SHAPE=50: Hybrid behavior (some mathematical, some weighted)

---

## V5 Config Mode

```
┌─────────────────────────────────────────────────────────────┐
│  K1: LENGTH      K2: SWING        K3: AUX MODE   K4: DRIFT  │
│  +S: BUILD       +S: CLOCK DIV    +S: (free)     +S: (free) │
└─────────────────────────────────────────────────────────────┘
```

| Knob | Primary | +Shift | Notes |
|------|---------|--------|-------|
| K1 | LENGTH (16/24/32/64) | BUILD (phrase arc) | BUILD sets intensity curve over phrase |
| K2 | SWING (0-100%) | CLOCK DIV (÷4 to ×4) | Genre-agnostic swing control |
| K3 | AUX MODE (Hat/Fill) | *(free)* | Simplified to 2 modes |
| K4 | DRIFT (evolution) | *(free)* | How much pattern changes per phrase |

### Config Simplifications vs v4

| Control | v4 | v5 | Rationale |
|---------|----|----|-----------|
| GENRE | Performance+Shift K2 | **Removed** | Algorithm-focused, not genre-specific |
| PUNCH | Performance+Shift K1 | **Removed** | External VCAs handle dynamics |
| BALANCE | Performance+Shift K4 | **Removed** | Voice coupling not valued |
| PHRASE LENGTH | Config+Shift K1 | Auto-derived | ~128 steps per phrase (from v4) |
| RESET MODE | Config K4 | **Hardcoded STEP** | Already done in v4 Task 22 |
| AUX DENSITY | Config+Shift K3 | **Removed** | Simplification |
| VOICE COUPLING | Config+Shift K4 | **Removed** | Not valued by user |

**Freed shift slots**: Config+Shift K3, Config+Shift K4 available for future use.

---

## Button Behavior

```
┌─────────────────────────────────────────────────────────────┐
│  TAP:        Trigger manual fill (intensity = tap duration) │
│  DOUBLE-TAP: Reseed pattern (new random seed)               │
│  HOLD:       (reserved for future use)                      │
└─────────────────────────────────────────────────────────────┘
```

**Removed**: Shift function (no shift layer in performance mode)

---

## AUX Output Behavior

| Condition | AUX Output (CV Out 1) |
|-----------|----------------------|
| **Clock unpatched** | Internal clock output |
| **Clock patched, AUX=HAT** | Third voice trigger (hi-hat pattern) |
| **Clock patched, AUX=FILL** | Gate high during fill zones |

---

## SHAPE Algorithm Concept

The core innovation of v5. SHAPE morphs between two hit placement engines:

### SHAPE = 0% (Pure Euclidean)

```cpp
// Euclidean algorithm: evenly distribute K hits across N steps
// SEEK X = rotation (0-N offset)
// SEEK Y = hit count modifier (fewer ↔ more within ENERGY budget)

for (int i = 0; i < patternLength; i++) {
    int rotated = (i + seekXRotation) % patternLength;
    hits[i] = euclideanPattern[rotated];
}
```

Visual:
```
SEEK X=0%:  ●···●···●···●···  (rotation 0)
SEEK X=25%: ·●···●···●···●··  (rotation 1)
SEEK X=50%: ··●···●···●···●·  (rotation 2)

SEEK Y=0%:  ●·······●·······  (sparse: 2 hits)
SEEK Y=50%: ●···●···●···●···  (medium: 4 hits)
SEEK Y=100%:●·●·●·●·●·●·●·●·  (dense: 8 hits)
```

### SHAPE = 100% (Pure Irregular)

```cpp
// Weighted probability: sample from archetype weight tables
// SEEK X = grounded vs floating (downbeat weight ↔ offbeat weight)
// SEEK Y = sparse vs busy (which subdivision levels eligible)

float weight = getArchetypeWeight(step, seekX, seekY);
hits[i] = gumbelTopK(weight, seed, budget);
```

Visual:
```
SEEK X=0%:  ●···●···●···●···  (grounded: downbeats emphasized)
SEEK X=100%:·●···●·●·····●··  (floating: offbeats emphasized)

SEEK Y=0%:  ●·······●·······  (sparse: only strong beats eligible)
SEEK Y=100%:●·●··●●·●●·●·●●·  (busy: all subdivisions eligible)
```

### SHAPE = 50% (Hybrid)

Blends both engines:
- Half the hits come from euclidean spacing
- Half come from weighted probability
- Creates patterns that are partially mechanical, partially human

---

## Open Questions

1. **SEEK X/Y naming**: Are "grounded/floating" and "sparse/busy" the right mental models?
   - Alternatives: "stable/unstable", "on-grid/off-grid", "simple/complex"

2. **SHAPE at extremes**: Should SHAPE=0% be PURE euclidean, or have some musical variation?
   - Pure euclidean can sound robotic
   - Could add slight humanization even at SHAPE=0%

3. **BUILD placement**: Is Config+Shift K1 the right spot, or should BUILD be Config K4 primary?
   - Current: LENGTH primary, BUILD shift
   - Alternative: Swap DRIFT and BUILD (BUILD primary, DRIFT shift)

4. **Fill button behavior**: Should fill intensity be tap duration, or velocity-sensitive?
   - Tap duration: easy to implement
   - Velocity: requires analog button (not available on Patch.Init)

---

## Comparison: v4 → v5

### Performance Mode

| Aspect | v4 | v5 |
|--------|----|----|
| Shift functions | 4 (PUNCH, GENRE, DRIFT, BALANCE) | **0** |
| Controls | ENERGY, BUILD, FIELD X, FIELD Y | ENERGY, SHAPE, SEEK X, SEEK Y |
| Core paradigm | Genre-based archetype grid | **Algorithm-based (euclidean ↔ irregular)** |
| CV mapping | K1-K4 primary | K1-K4 primary (unchanged) |

### Config Mode

| Aspect | v4 | v5 |
|--------|----|----|
| Shift functions | 4 | **2** (BUILD, CLOCK DIV) + 2 free |
| AUX modes | 4 (Hat/Fill/Phrase/Event) | **2** (Hat/Fill) |
| Removed | - | GENRE, PUNCH, BALANCE, VOICE COUPLING, AUX DENSITY |

### Button

| Aspect | v4 | v5 |
|--------|----|----|
| Functions | Shift + Fill + Reseed | **Fill (tap) + Reseed (double-tap)** |
| Shift role | Hold for shift layer | **None** (no shift in perf mode) |

---

## Next Steps

1. Validate SEEK X/Y naming and mental models
2. Detail the SHAPE blending algorithm (euclidean ↔ irregular interpolation)
3. Define archetype weight tables for irregular mode
4. Specify BUILD curve shapes (linear, exponential, S-curve)
5. Prototype and test on hardware
