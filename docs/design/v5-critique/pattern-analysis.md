# Pattern Analysis: V5 Shimmer Variation Problem

**Date**: 2026-01-07
**Analyst**: Design Critic
**Source**: `docs/patterns.html` pattern visualization

## Executive Summary

Visual analysis of the generated patterns reveals a critical expressiveness problem: **shimmer (V2) patterns are nearly identical across different seeds at moderate energy levels**. While anchor (V1) shows subtle velocity variations, the shimmer placement is deterministically locked to the same positions regardless of seed value.

## Observations from Pattern Visualization

### Seed Variation Section (Critical Finding)

Examining the "Seed Variation (same params, different patterns)" section at lines 6025-6524 of `patterns.html`:

| Seed | V1 Hits | V2 Hits | V1 Positions | V2 Positions |
|------|---------|---------|--------------|--------------|
| 0xDEADBEEF | 8 | 6 | 0,4,8,12,16,20,24,28 | 1,5,9,13,17,21 |
| 0xCAFEBABE | 8 | 6 | 0,4,8,12,16,20,24,28 | 1,5,9,13,17,21 |
| 0x12345678 | 8 | 6 | 0,4,8,12,16,20,24,28 | 1,5,9,13,17,21 |
| 0xABCD1234 | 8 | 6 | 0,4,8,12,16,20,24,28 | 1,5,9,13,17,21 |

**Result: V1 and V2 positions are IDENTICAL across all four seeds.**

The only variation is:
- Minor velocity differences (e.g., 0.97 vs 0.98 opacity on hits)
- This creates nearly imperceptible differences in the visualization

### SHAPE Sweep Observations

At SHAPE=0.00 to SHAPE=0.50 (stable through syncopated zones):
- V1 consistently locks to four-on-floor: positions 0, 4, 8, 12, 16, 20, 24, 28
- V2 consistently fills the "gap" at positions 1, 5, 9, 13, 17, 21

Only at SHAPE >= 0.70 (wild zone) do patterns begin to diverge.

### Energy Sweep Observations

At ENERGY=0.60 (the test default):
- Pattern is firmly in GROOVE zone
- Guard rails enforce downbeat hits
- Spacing constraints limit variation

## Root Cause Analysis

### The Convergence Cascade

1. **Guard Rails Lock Anchor** (Step 1)
   - `ApplyHardGuardRails()` enforces downbeat on step 0
   - At moderate energy, spacing constraints push toward regular four-on-floor

2. **COMPLEMENT Fills Deterministically** (Step 2)
   - `ApplyComplementRelationship()` finds gaps between anchor hits
   - At DRIFT=0.0 (default), uses `PlaceEvenlySpaced()` which is purely deterministic
   - Gap structure is identical -> placement is identical

3. **Low DRIFT = No Variation** (Step 3)
   - DRIFT < 0.3 means evenly spaced placement (lines 382-385 of VoiceRelation.cpp)
   - Seed is not used in this placement mode
   - Result: identical shimmer regardless of seed

### Evidence from Code

From `VoiceRelation.cpp` lines 378-396:

```cpp
if (drift < 0.3f)
{
    // Low drift: evenly spaced within gap
    position = PlaceEvenlySpaced(gaps[g], j, gapShare, clampedLength);
}
else if (drift < 0.7f)
{
    // Mid drift: weighted by shimmer weights
    position = PlaceWeightedBest(gaps[g], shimmerWeights, clampedLength, shimmerMask);
}
else
{
    // High drift: seed-varied random
    position = PlaceSeedVaried(gaps[g], rngState, clampedLength, shimmerMask);
}
```

At DRIFT=0.0 (boot default), the seed is never consulted for shimmer placement.

## Expressiveness Quadrant Assessment

Current position: **"One-Trick Pony"** (High Simplicity, Low Expressiveness)

```
                    HIGH EXPRESSIVENESS
                           |
       "Expert's Dream"    |    TARGET: "Sweet Spot"
       (Complex but        |    (Simple surface,
        powerful)          |     deep control)
    -----------------------+----------------------- <- We should be here
       "Useless"           |    "One-Trick Pony"
       (Hard to use AND    |    (Easy but limited) <- Currently here
        not expressive)    |
                           |
                    LOW EXPRESSIVENESS
```

The module is:
- Easy to use: Turn knobs, get patterns
- Limited in expression: Same patterns regardless of reseed at moderate settings

## Impact Assessment

### User Experience

1. **Reseed Gesture Feels Broken**
   - User holds button 3 seconds expecting new pattern
   - Gets identical shimmer, only subtle velocity changes
   - "Is it even working?"

2. **Loss of Musical Variety**
   - Same kick-snare relationship bar after bar
   - Cannot "find" different grooves through reseeding
   - Module becomes predictable/boring

3. **Broken Mental Model**
   - Spec says "Deterministic variation: Same settings + seed = identical output"
   - Inverse should be true: Different seed = different output
   - Current behavior violates this expectation

### Spec Compliance

From spec section 1.1:
> "Deterministic variation: Same settings + seed = identical output"

This implies different seeds should produce different outputs. At moderate energy, they don't.

## Severity Classification

**CRITICAL** - This fundamentally undermines the module's value proposition as an "algorithmic drum sequencer." If the algorithm produces the same output regardless of seed at typical settings, it's not really algorithmic variation.

## Next Steps

See `shimmer-diagnosis.md` for detailed technical diagnosis.
See `recommendations.md` for proposed solutions.
