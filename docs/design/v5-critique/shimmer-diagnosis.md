# Technical Diagnosis: Shimmer Variation Failure

**Date**: 2026-01-07
**Files Analyzed**:
- `src/Engine/VoiceRelation.cpp` (COMPLEMENT relationship logic)
- `src/Engine/Sequencer.cpp` (GenerateBar pipeline)
- `src/Engine/HitBudget.cpp` (budget computation)
- `src/Engine/GuardRails.cpp` (pattern constraints)
- `tests/pattern_viz_test.cpp` (documented behavior)

## Core Problem Statement

Shimmer variation requires two conditions:
1. **Different anchor patterns** - Shimmer fills anchor gaps
2. **Seed-sensitive shimmer placement** - Seed affects where hits land in gaps

At moderate energy (GROOVE zone, ~0.3-0.5 energy), both conditions fail:
- Anchor converges to four-on-floor due to guard rails + spacing
- Shimmer uses even-spacing algorithm that ignores seed

## Detailed Call Flow Analysis

### Pattern Generation Pipeline

```
GenerateBar()
  |
  +-> ComputeShapeBlendedWeights()     // V1 weights
  +-> ApplyAxisBias()                   // Apply AXIS X/Y
  +-> SelectHitsGumbelTopK()            // Select anchor hits
  |     |
  |     +-> Spacing constraints (zone-based)
  |     +-> Gumbel noise (seed-dependent)
  |
  +-> ApplyHardGuardRails()             // Force downbeat
  |
  +-> ComputeShapeBlendedWeights()     // V2 weights (seed+1)
  +-> ApplyComplementRelationship()     // <-- PROBLEM HERE
        |
        +-> PlaceEvenlySpaced()         // DRIFT < 0.3 - NO SEED
        +-> PlaceWeightedBest()         // DRIFT 0.3-0.7 - Uses weights
        +-> PlaceSeedVaried()           // DRIFT > 0.7 - Uses seed
```

### Why Anchor Converges

From `GuardRails.cpp`:
```cpp
// ApplyHardGuardRails ensures step 0 has anchor hit
if ((anchorMask & 0x1) == 0)
{
    anchorMask |= 0x1;  // Force downbeat
}
```

From `GumbelSampler.cpp`:
```cpp
// GetMinSpacingForZone returns spacing constraints
case EnergyZone::GROOVE:
    return 4;  // Minimum 4 steps between anchor hits
```

With 8 anchor hits at 32 steps with min-spacing of 4:
- Only one pattern fits: 0, 4, 8, 12, 16, 20, 24, 28
- Gumbel noise becomes irrelevant - constraints determine outcome

### Why Shimmer Doesn't Vary

From `VoiceRelation.cpp` line 378-385:
```cpp
if (drift < 0.3f)
{
    // Low drift: evenly spaced within gap
    position = PlaceEvenlySpaced(gaps[g], j, gapShare, clampedLength);
}
```

`PlaceEvenlySpaced()` is purely deterministic:
```cpp
int PlaceEvenlySpaced(const Gap& gap, int hitIndex, int totalHits, int /*clampedLength*/)
{
    if (totalHits <= 1)
    {
        return (gap.start + gap.end) / 2;  // Middle of gap
    }

    int gapSize = gap.end - gap.start;
    float spacing = static_cast<float>(gapSize) / static_cast<float>(totalHits + 1);
    return gap.start + static_cast<int>(spacing * (hitIndex + 1) + 0.5f);
}
```

No seed parameter. No variation possible.

## Constraint Analysis

### Why Does This Happen?

1. **Over-constrained at moderate energy**
   - GROOVE zone (0.2-0.5 energy) has 4-step minimum spacing
   - 8 hits in 32 steps with 4-step spacing = exactly one solution

2. **Guard rails assume "musical correctness" means regularity**
   - Step 0 downbeat is mandatory
   - This is appropriate, but spacing + downbeat = over-determined

3. **DRIFT default is too low**
   - Default DRIFT=0.0 triggers `PlaceEvenlySpaced()`
   - Even if anchor varied, shimmer wouldn't

4. **COMPLEMENT is gap-dependent, not seed-dependent**
   - Shimmer placement is 100% determined by gap structure
   - Same gaps = same shimmer (at low DRIFT)

## Test Evidence

From `pattern_viz_test.cpp` lines 828-877:
```cpp
TEST_CASE("Shimmer pattern convergence at moderate energy", "[pattern-viz][shimmer][convergence]")
{
    // This test documents the KNOWN behavior that shimmer converges
    // when anchor is locked to four-on-floor by guard rails
    ...
    SECTION("Same parameters produce identical shimmer")
    {
        params.seed = 11111;
        GeneratePattern(params, pattern1);

        params.seed = 22222;
        GeneratePattern(params, pattern2);

        params.seed = 33333;
        GeneratePattern(params, pattern3);

        // DOCUMENTED BEHAVIOR: At moderate energy with low drift,
        // both V1 and V2 converge because guard rails + spacing
        // dominate the Gumbel selection.
        // This is by design - stable four-on-floor is musically appropriate.
```

The test **documents the behavior as "by design"** but this conflicts with the spec's promise of seed-based variation.

## Constraint Hierarchy (Root Cause)

```
Priority 1: Guard Rails (downbeat required)           <- Inviolable
Priority 2: Spacing Constraints (zone-based)          <- Strongly enforced
Priority 3: Gumbel Selection (seed-dependent)         <- Overwhelmed
Priority 4: COMPLEMENT (gap-filling)                  <- Deterministic
```

The problem: Priorities 1+2 fully determine the outcome at moderate energy.
Priorities 3+4 only affect the result when 1+2 leave room for choice.

## Quantitative Analysis

### At GROOVE Zone (Energy ~0.35)

- Anchor budget: ~8 hits (from `ComputeAnchorBudget`)
- Min spacing: 4 steps
- Pattern length: 32 steps
- Possible patterns: 32 / 8 = 4 starting offsets, but guard rails fix step 0

**Result: Exactly 1 valid anchor pattern**

### At PEAK Zone (Energy ~0.85)

- Anchor budget: ~12 hits
- Min spacing: 1-2 steps
- Pattern length: 32 steps
- Possible patterns: Many (combinatorics)

**Result: Seed variation is possible**

## The Fundamental Design Issue

The current design conflates two concerns:
1. **Musical stability** (four-on-floor sounds good)
2. **Algorithmic variation** (different seeds = different patterns)

These should not be mutually exclusive. You can have:
- Stable downbeat structure
- Variable syncopation/placement details

The module currently trades away #2 to maximize #1 at moderate energy.

## Related Spec Sections

### Section 5.3 (DRIFT parameter)
> "DRIFT=0.00 (Locked): Minimal variation between bars"

This is working as specified - minimal variation. But the *between seeds* variation should still exist.

### Section 6.2 (Voice Relationships)
> "COMPLEMENT: Shimmer fills anchor gaps"

The spec says *where* shimmer goes (in gaps), not *how* (deterministically or with variation).

### Section 1.1 (Core Philosophy)
> "Deterministic variation: Same settings + seed = identical output"

Implies: Different seed = different output. Currently violated at moderate energy.

## Conclusion

The shimmer variation failure is **structural**, not a simple bug. It emerges from:
1. Over-constrained anchor patterns at moderate energy
2. Deterministic shimmer placement at low DRIFT
3. Guard rails + spacing leaving no room for Gumbel selection

Fixing this requires **relaxing constraints** or **injecting variation earlier** in the pipeline.

See `recommendations.md` for proposed solutions.
