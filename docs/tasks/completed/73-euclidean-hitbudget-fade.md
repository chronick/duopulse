---
id: 73
slug: euclidean-hitbudget-fade
title: "Euclidean K / HitBudget Fade System"
status: completed
created_date: 2026-01-21
updated_date: 2026-01-23
completed_date: 2026-01-23
branch: feature/euclidean-hitbudget-fade
spec_refs: ["06-shape", "algorithm_config"]
---

# Task 73: Euclidean K / HitBudget Fade System

## Problem

Currently, two independent systems control hit placement:

1. **Euclidean K** (`algorithm_config.h`): Defines weight field shape (WHERE peaks are)
2. **HitBudget** (`HitBudget.cpp`): Defines actual hit count (HOW MANY hits)

These systems are **disconnected**. At SHAPE=0 with low ENERGY:
- Euclidean K=4 creates 4 weighted peaks (perfect four-on-floor grid)
- But HitBudget says "place 12 hits" based on ENERGY zone
- Result: 12 hits spread around the 4 peaks, destroying the four-on-floor character

**Current Four on Floor preset (SHAPE=0, ENERGY=0.23):**
- Expected: 4 kicks on downbeats (steps 0, 8, 16, 24)
- Actual: 12 anchor hits, density 0.41

## Solution

**Fade between Euclidean K and HitBudget based on SHAPE:**

```cpp
// In HitBudget.cpp or PatternGenerator.cpp

float euclideanInfluence = getEuclideanWeight(shape);  // From AlgorithmWeights

// Fade doesn't start until SHAPE > 0.15 (pure euclidean zone)
float fadeProgress = (shape <= 0.15f) ? 0.0f : (shape - 0.15f) / 0.85f;
fadeProgress = std::min(1.0f, fadeProgress);

int euclideanK = computeEuclideanK(energy, channel);  // 4-12 based on ENERGY
int budgetK = computeHitBudget(energy, zone, ...);    // Current density logic

// Blend: at SHAPE=0-0.15, use euclideanK; at SHAPE=1.0, use budgetK
int effectiveK = static_cast<int>(
    euclideanK + fadeProgress * (budgetK - euclideanK) + 0.5f
);
```

## Behavior Matrix

| SHAPE | fadeProgress | effectiveK | Musical Result |
|-------|--------------|------------|----------------|
| 0.00 | 0.0 | euclideanK | Pure four-on-floor, ENERGY scales K |
| 0.15 | 0.0 | euclideanK | Still pure euclidean |
| 0.30 | 0.18 | mostly euclidean | Slight density freedom |
| 0.50 | 0.41 | blend | Transitioning |
| 0.70 | 0.65 | mostly budget | Density-driven, euclidean fading |
| 1.00 | 1.0 | budgetK | Pure density scaling, wild placement |

## Key Design Decisions

1. **Fade starts at SHAPE=0.15** (not 0.0)
   - SHAPE 0.00-0.15 is "pure euclidean zone"
   - Four-on-floor locked in this range regardless of ENERGY
   - User requested this threshold

2. **Per-channel euclidean K** uses existing `kAnchorKMin/Max`, `kShimmerKMin/Max`, `kAuxKMin/Max`
   - Already defined in `algorithm_config.h`
   - Scale with ENERGY within each channel's range

3. **Anchor gets priority**
   - At SHAPE=0, anchor K determines anchor hits exactly
   - Shimmer/aux may still use budget if desired, or also fade

## Implementation Steps

### Step 1: Add fade calculation helper
Location: `src/Engine/HitBudget.cpp`

```cpp
/**
 * Compute effective hit count by fading between euclidean K and budget
 * based on SHAPE parameter.
 *
 * At SHAPE <= 0.15: pure euclidean K (grid-locked hits)
 * At SHAPE = 1.0: pure budget-based (density-driven)
 */
int ComputeEffectiveHitCount(int euclideanK, int budgetK, float shape)
{
    // No fade until SHAPE > 0.15
    if (shape <= 0.15f) {
        return euclideanK;
    }

    // Linear fade from 0.15 to 1.0
    float fadeProgress = (shape - 0.15f) / 0.85f;
    fadeProgress = std::min(1.0f, fadeProgress);

    return static_cast<int>(
        euclideanK + fadeProgress * (budgetK - euclideanK) + 0.5f
    );
}
```

### Step 2: Modify ComputeAnchorBudget to use fade
Integrate the fade into existing budget computation, passing shape parameter.

### Step 3: Update PatternGenerator to use effective K
Ensure Gumbel selection uses the faded effective K, not raw budget.

### Step 4: Update tests
Add unit tests for:
- SHAPE=0 returns exactly euclideanK
- SHAPE=0.15 returns exactly euclideanK
- SHAPE=0.5 returns blend
- SHAPE=1.0 returns exactly budgetK

### Step 5: Regenerate evals and verify
- Four on Floor preset should now show ~4 anchor hits at SHAPE=0
- Pentagon metrics should improve for stable zone density/regularity

## Files to Modify

- `src/Engine/HitBudget.cpp` - Add fade logic
- `src/Engine/HitBudget.h` - Add function signature
- `src/Engine/PatternGenerator.cpp` - Use effective K in selection
- `tests/Engine/HitBudget_test.cpp` - Add fade tests

## Success Criteria

1. **Four on Floor preset** (SHAPE=0, ENERGY=0.23):
   - Anchor hits: ~4-5 (down from 12)
   - Density: ~0.15-0.20 (down from 0.41)
   - Hits land on downbeats (steps 0, 8, 16, 24 or similar)

2. **SHAPE sweep at ENERGY=0.3**:
   - SHAPE=0.0: 4-5 anchor hits, grid-locked
   - SHAPE=0.5: 6-8 anchor hits, transitioning
   - SHAPE=1.0: 10-12 anchor hits, density-driven

3. **Pentagon metrics**:
   - Stable zone density should drop into target range (0.15-0.32)
   - Stable zone regularity should improve (hits on euclidean grid = regular)

4. **Musical test**:
   - Playing SHAPE=0 sweep should sound like classic four-on-floor techno
   - Adding AXIS X should add ghost notes without moving the kick

## Related

- Task 56: Weight-based blending (created the SHAPE system)
- Task 63: Sensitivity analysis (will need re-run after this)
- `algorithm_config.h`: kAnchorKMin/Max parameters

## Notes

This is a fundamental fix to make the 4-knob interface more musical and intuitive.
The user's vision: "SHAPE=0 + ENERGY sweep means kicks on every downbeat."
