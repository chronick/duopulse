# Iteration 1: Seed-Sensitive Even Spacing

**Date**: 2026-01-07
**Status**: Draft (pending clarification)
**Approach**: Inject seed into even-spacing algorithm

## Executive Summary

The simplest fix for shimmer variation is to add a seed parameter to `PlaceEvenlySpaced()`. The seed determines a phase offset that shifts where hits land within each gap. This preserves the even distribution character while making seed changes produce audibly different patterns.

Combined with raising the default DRIFT to 0.25, this should restore the expected "different seed = different pattern" behavior without any changes to the control surface.

## Technical Design

### Change 1: PlaceEvenlySpacedWithSeed

**File**: `src/Engine/VoiceRelation.cpp`
**Location**: Lines 217-227 (anonymous namespace)

**Current**:
```cpp
int PlaceEvenlySpaced(const Gap& gap, int hitIndex, int totalHits, int patternLength)
{
    if (totalHits <= 0)
    {
        return gap.start;
    }

    int offset = (gap.length * hitIndex) / std::max(1, totalHits);
    return (gap.start + offset) % patternLength;
}
```

**Proposed**:
```cpp
int PlaceEvenlySpacedWithSeed(const Gap& gap, int hitIndex, int totalHits,
                               int patternLength, uint32_t seed)
{
    if (totalHits <= 0)
    {
        return gap.start;
    }

    // Calculate base spacing
    float spacing = static_cast<float>(gap.length) / static_cast<float>(totalHits + 1);

    // Seed-based phase offset: 0.0 to 0.5 of one spacing unit
    // This shifts the entire grid without changing relative positions
    float phase = HashToFloat(seed, gap.start) * 0.5f;

    // Position = gap.start + spacing * (hitIndex + 1 + phase)
    int position = gap.start + static_cast<int>(spacing * (hitIndex + 1 + phase) + 0.5f);

    // Handle wrap-around
    if (position >= gap.start + gap.length)
    {
        position = gap.start + (position - gap.start) % gap.length;
    }

    return position % patternLength;
}
```

**Rationale**:
- `HashToFloat(seed, gap.start)` produces a stable float in [0,1) based on seed and gap position
- Multiplying by 0.5 limits phase shift to half a spacing unit (prevents overlap)
- Using `gap.start` as the hash input means different gaps get different phases
- This preserves even distribution while adding seed-dependent variation

### Change 2: Update Call Site

**File**: `src/Engine/VoiceRelation.cpp`
**Location**: Lines 378-385 (inside ApplyComplementRelationship)

**Current**:
```cpp
if (drift < 0.3f)
{
    // Low drift: evenly spaced within gap
    position = PlaceEvenlySpaced(gaps[g], j, gapShare, clampedLength);
}
```

**Proposed**:
```cpp
if (drift < 0.3f)
{
    // Low drift: evenly spaced within gap, phase varies with seed
    position = PlaceEvenlySpacedWithSeed(gaps[g], j, gapShare, clampedLength, seed);
}
```

### Change 3: Raise Default DRIFT (Optional)

**File**: `src/Engine/ControlState.h`
**Location**: Line 387

**Current**:
```cpp
drift = 0.0f;  // V5: 0% locked pattern
```

**Proposed**:
```cpp
drift = 0.25f;  // V5: 25% enables seed-varied shimmer placement
```

**Rationale**:
- At DRIFT=0.25, shimmer uses even-spacing with seed (from Change 1)
- This is still "low DRIFT" range, preserving stable feel
- Provides immediate seed variation out of the box

**Trade-off**: This is a **breaking change** for existing users who expect DRIFT=0. Consider documenting in release notes.

## Expected Behavior

### Before (Current)
```
Seed 0x1111 @ SHAPE=0.3, ENERGY=0.5, DRIFT=0.0:
  V1: ----X---X---X---X---X---X---X---X  (four-on-floor)
  V2: --X---X---X---X---X---X---X---X--  (mid-gap)

Seed 0x2222 @ SHAPE=0.3, ENERGY=0.5, DRIFT=0.0:
  V1: ----X---X---X---X---X---X---X---X  (same)
  V2: --X---X---X---X---X---X---X---X--  (same!)
```

### After (Proposed)
```
Seed 0x1111 @ SHAPE=0.3, ENERGY=0.5, DRIFT=0.0:
  V1: ----X---X---X---X---X---X---X---X  (four-on-floor)
  V2: --X---X---X---X---X---X---X---X--  (phase 0.2)

Seed 0x2222 @ SHAPE=0.3, ENERGY=0.5, DRIFT=0.0:
  V1: ----X---X---X---X---X---X---X---X  (same anchor)
  V2: -X---X---X---X---X---X---X---X---  (phase 0.4 - shifted!)
```

## Test Cases

### New Test: Seed Produces Shimmer Variation
```cpp
TEST_CASE("Seed produces shimmer variation at moderate energy", "[pattern-viz][shimmer][variation]")
{
    PatternParams params;
    params.energy = 0.50f;
    params.shape = 0.30f;
    params.drift = 0.0f;

    std::set<uint32_t> uniqueShimmerMasks;

    for (uint32_t seed : {0x11111111, 0x22222222, 0x33333333, 0x44444444})
    {
        params.seed = seed;
        PatternData pattern;
        GeneratePattern(params, pattern);
        uniqueShimmerMasks.insert(pattern.v2Mask);
    }

    // REQUIREMENT: At least 3 different shimmer patterns from 4 seeds
    REQUIRE(uniqueShimmerMasks.size() >= 3);
}
```

### Regression Test: Determinism Preserved
```cpp
TEST_CASE("Same seed produces identical shimmer", "[pattern-viz][shimmer][determinism]")
{
    PatternParams params;
    params.energy = 0.50f;
    params.shape = 0.30f;
    params.drift = 0.0f;
    params.seed = 0xDEADBEEF;

    PatternData pattern1, pattern2;
    GeneratePattern(params, pattern1);
    GeneratePattern(params, pattern2);

    REQUIRE(pattern1.v2Mask == pattern2.v2Mask);
}
```

## Risk Assessment

| Aspect | Risk Level | Mitigation |
|--------|------------|------------|
| Musical quality | Low | Phase shift is subtle, preserves groove |
| Breaking changes | Medium | Document new default DRIFT |
| Implementation bugs | Low | Small, localized change |
| Test coverage | Low | Add explicit test cases |

## Implementation Estimate

- Code changes: 1-2 hours
- Testing: 1 hour
- Documentation: 30 minutes

**Total**: ~3 hours

## Dependencies

- Requires `HashToFloat()` from GumbelSampler.h (already available)
- No new files needed
- No hardware changes

## Alternatives Considered

1. **Gumbel selection within gaps**: More complex, saves for Phase 2 if this is insufficient
2. **Nudge to weighted positions**: Adds complexity, blends approaches
3. **Pre-seed anchor jitter**: Attacks root cause but risks destabilizing anchor

Phase offset is the minimal viable fix that addresses the immediate problem.

## Open Items

- [ ] User confirmation on DRIFT semantics
- [ ] Approval to raise default DRIFT
- [ ] Review of phase offset range (0.5 sufficient?)
