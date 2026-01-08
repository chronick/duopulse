# Recommendations: Restoring Shimmer Variation

**Date**: 2026-01-07
**Priority**: CRITICAL
**Affected Files**:
- `src/Engine/VoiceRelation.cpp`
- `src/Engine/GuardRails.cpp`
- `src/Engine/GumbelSampler.cpp`

## Summary

The shimmer variation problem requires changes at multiple levels to achieve the "Sweet Spot" quadrant - simple surface with deep control. The goal is to make seed changes produce audibly different patterns even at moderate energy, while preserving musical coherence.

## Quadrant Analysis: Path to Sweet Spot

Current position: Low Expressiveness (patterns converge regardless of seed)

Target: HIGH Expressiveness while maintaining HIGH Simplicity

```
                    HIGH EXPRESSIVENESS
                           |
       "Expert's Dream"    |    TARGET: "Sweet Spot"
                           |         /
    -----------------------+--------/---------------
                           |       /
                           |    Current
                           |
                    LOW EXPRESSIVENESS
```

### What "Sweet Spot" Looks Like

1. **Simple defaults produce usable patterns** (already achieved)
2. **Reseed produces audibly different grooves** (currently broken)
3. **DRIFT provides deeper control** (working, but default bypasses it)
4. **Parameters interact musically** (needs improvement)

## Recommendations

### Recommendation 1: Inject Seed Variation into Even-Spacing (LOW RISK)

**Problem**: `PlaceEvenlySpaced()` is purely deterministic.

**Solution**: Add seed-based micro-jitter while preserving even distribution.

```cpp
int PlaceEvenlySpacedWithSeed(
    const Gap& gap,
    int hitIndex,
    int totalHits,
    int clampedLength,
    uint32_t seed)
{
    if (totalHits <= 1)
    {
        return (gap.start + gap.end) / 2;
    }

    int gapSize = gap.end - gap.start;
    float spacing = static_cast<float>(gapSize) / static_cast<float>(totalHits + 1);
    int basePosition = gap.start + static_cast<int>(spacing * (hitIndex + 1) + 0.5f);

    // Seed-based jitter: +/- 1 step maximum
    if (gapSize >= 3)
    {
        float jitter = HashToFloat(seed, hitIndex);
        int offset = (jitter < 0.33f) ? -1 : (jitter > 0.66f) ? 1 : 0;

        // Clamp to gap bounds
        int newPos = basePosition + offset;
        if (newPos > gap.start && newPos < gap.end)
        {
            basePosition = newPos;
        }
    }

    return basePosition;
}
```

**Trade-offs**:
- Low risk: Preserves overall structure
- May still converge if gap is only 2 steps wide
- Simple to implement and test

**Expressiveness Impact**: +20% (subtle variation)

---

### Recommendation 2: Raise Default DRIFT (MEDIUM RISK)

**Problem**: DRIFT=0.0 default bypasses all seed-sensitive placement.

**Solution**: Change default DRIFT from 0.0 to 0.25.

Location: `src/Engine/ControlState.cpp` or wherever defaults are set.

```cpp
void Init()
{
    drift = 0.25f;  // Was 0.0f - enables seed-varied shimmer placement
    ...
}
```

**Trade-offs**:
- Breaks backward compatibility slightly (existing patches will sound different)
- Aligns with user expectation (reseed should change pattern)
- Simple change

**Expressiveness Impact**: +40% (engages weighted placement at low DRIFT)

---

### Recommendation 3: Relax Spacing in GROOVE Zone (MEDIUM RISK)

**Problem**: Min-spacing of 4 at GROOVE zone forces four-on-floor.

**Solution**: Reduce GROOVE min-spacing from 4 to 2 steps.

Location: `src/Engine/GuardRails.cpp`

```cpp
int GetMinSpacingForZone(EnergyZone zone)
{
    switch (zone)
    {
        case EnergyZone::MINIMAL: return 4;
        case EnergyZone::GROOVE:  return 2;  // Was 4 - allows syncopation
        case EnergyZone::BUILD:   return 2;
        case EnergyZone::PEAK:    return 1;
        default: return 2;
    }
}
```

**Trade-offs**:
- Higher risk: May produce "unstable" patterns that don't feel groovy
- Opens up pattern space significantly
- May require tuning of hit budgets to compensate

**Expressiveness Impact**: +60% (unlocks many more anchor configurations)

---

### Recommendation 4: Probabilistic Downbeat (EXPERIMENTAL)

**Problem**: Guard rails force step 0 hit 100% of the time.

**Solution**: Make downbeat enforcement probabilistic based on SHAPE.

```cpp
void ApplyHardGuardRails(uint32_t& anchorMask, uint32_t& shimmerMask,
                          EnergyZone zone, Genre genre, int patternLength,
                          float shape, uint32_t seed)
{
    // Step 0 downbeat: always at low SHAPE, probabilistic at high SHAPE
    if ((anchorMask & 0x1) == 0)
    {
        if (shape < 0.5f || HashToFloat(seed, 0) < (1.0f - shape))
        {
            anchorMask |= 0x1;  // Force downbeat
        }
    }
    ...
}
```

**Trade-offs**:
- High risk: May produce "wrong-feeling" patterns
- Aligns with SHAPE philosophy (Wild = less constrained)
- Requires careful testing across energy zones

**Expressiveness Impact**: +30% (only affects high SHAPE)

---

### Recommendation 5: Shimmer-Specific Gumbel Stage (STRUCTURAL)

**Problem**: Shimmer placement is gap-dependent, not Gumbel-selected.

**Solution**: After finding gaps, use Gumbel selection within each gap.

```cpp
uint32_t ApplyComplementRelationship(
    uint32_t anchorMask,
    const float* shimmerWeights,
    float drift,
    uint32_t seed,
    int patternLength,
    int targetHits)
{
    // Step 1: Find gaps (unchanged)
    Gap gaps[kMaxSteps];
    int numGaps = FindGaps(anchorMask, patternLength, gaps);

    // Step 2: Distribute hits across gaps proportionally
    int hitsPerGap[kMaxSteps];
    DistributeHitsToGaps(gaps, numGaps, targetHits, hitsPerGap);

    // Step 3: NEW - Use Gumbel selection WITHIN each gap
    uint32_t shimmerMask = 0;
    for (int g = 0; g < numGaps; ++g)
    {
        // Create eligibility mask for this gap only
        uint32_t gapEligibility = CreateGapMask(gaps[g]);

        // Use Gumbel to select hit positions within gap
        uint32_t gapHits = SelectHitsGumbelTopK(
            shimmerWeights,
            gapEligibility,
            hitsPerGap[g],
            seed ^ (g * 0x1234567),  // Per-gap seed variation
            patternLength,
            0  // No spacing within gap
        );

        shimmerMask |= gapHits;
    }

    return shimmerMask;
}
```

**Trade-offs**:
- Medium-high implementation complexity
- Structurally sound: Uses existing Gumbel infrastructure
- Preserves gap-filling semantics
- Seed naturally varies shimmer placement

**Expressiveness Impact**: +80% (shimmer becomes truly seed-dependent)

---

## Recommended Implementation Order

### Phase 1: Quick Wins (Low Risk)

1. **Implement Recommendation 1** (seed jitter in even-spacing)
   - 1-2 hour task
   - Immediate improvement
   - No breaking changes

2. **Implement Recommendation 2** (raise default DRIFT)
   - 15 minute change
   - Document breaking change
   - Consider user notification on first boot

### Phase 2: Structural Improvement (Medium Risk)

3. **Implement Recommendation 5** (Gumbel selection in gaps)
   - 4-6 hour task
   - Major expressiveness improvement
   - Requires comprehensive testing

### Phase 3: Parameter Tuning (Testing Required)

4. **Consider Recommendation 3** (relax GROOVE spacing)
   - Only if Phase 1+2 insufficient
   - Requires musical testing
   - May need hitbudget retuning

5. **Consider Recommendation 4** (probabilistic downbeat)
   - Only if SHAPE feels too constrained at high values
   - Experimental - may not ship

## Success Criteria

After implementation, the following should hold:

1. **Seed Variation Test**: At SHAPE=0.3, ENERGY=0.5, DRIFT=0.0, four different seeds should produce at least 3 different shimmer masks.

2. **Musical Coherence Test**: Patterns should still feel "groovy" - not random.

3. **Expressiveness Quadrant**: Module should be in "Sweet Spot" quadrant:
   - Novice can get good patterns with default settings
   - Different seeds produce audibly different grooves
   - DRIFT provides deeper control when desired

## Test Cases to Add

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

## Risk Assessment

| Recommendation | Risk | Impact | Effort | Priority |
|----------------|------|--------|--------|----------|
| R1: Seed jitter | Low | +20% | 2h | HIGH |
| R2: Raise DRIFT default | Medium | +40% | 15min | HIGH |
| R3: Relax spacing | Medium | +60% | 1h | MEDIUM |
| R4: Prob. downbeat | High | +30% | 2h | LOW |
| R5: Gumbel in gaps | Medium | +80% | 6h | HIGH |

**Recommended First Action**: Implement R1 + R2 together, test, then evaluate if R5 is needed.

## Conclusion

The shimmer variation failure is fixable without major architectural changes. The key insight is that the current system over-constrains patterns at moderate energy, and relaxing these constraints through seed-sensitive placement will restore the expected "same settings, different seed = different pattern" behavior.

The recommendations are ordered by risk and effort. Quick wins (R1 + R2) can be shipped immediately; structural improvements (R5) provide the most comprehensive solution if needed.
