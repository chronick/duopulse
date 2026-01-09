# Iteration 2: Revised Algorithm Proposals

**Date**: 2026-01-08
**Status**: Proposed
**Revision**: Addresses critique from iteration-1

---

## Summary of Changes from Iteration 1

This iteration incorporates critical feedback from the design review. The key theme is **simplicity over complexity**: preferring velocity variation and micro-displacement over pattern rotation, and giving users control over AUX style rather than random selection.

### Issues Fixed

| Issue | Iteration 1 Problem | Iteration 2 Fix |
|-------|---------------------|-----------------|
| Fitness metrics | Uniqueness != musicality | Added groove quality sub-metrics |
| Seed rotation | Breaks SHAPE contract at low values | Rotation only when SHAPE > 0.3 |
| AUX style | Hash-selected, no user control | AXIS Y zones control style |
| Beat 1 skip | Too aggressive, breaks live mixing | Only in wild zone (SHAPE >= 0.7) |
| CPU cost | Not analyzed | Explicit budget defined |
| Noise formula | Increases noise at SHAPE=0 | Fixed formula, stable zone stays stable |

### Approach Shift: Simpler is Better

The critique correctly identified that Iteration 1 swung toward "Expert's Dream" (complex but powerful) rather than "Sweet Spot" (simple surface, deep control). This revision prioritizes:

1. **Velocity variation** (quick win, no complexity)
2. **Micro-displacement** (preserves pattern shape)
3. **Two-pass generation** (guaranteed groove skeleton)
4. **Parameter partitioning** (user controls style, not randomness)

---

## Design Decisions (Locked In)

### Decision 1: Beat 1 Presence

**Rule**: Beat 1 (step 0) is ALWAYS present when SHAPE < 0.7

**Rationale**:
- DJs need predictable beat 1 for mixing
- Even IDM masters maintain 4/4 grid structure
- "Wild zone" (SHAPE >= 0.7) is explicitly experimental territory

**Implementation**:
```cpp
// In GeneratePattern, AFTER guard rails:
if (params.shape < 0.7f) {
    // Stable/syncopated zones: beat 1 always present
    result.anchorMask |= (1U << 0);
} else {
    // Wild zone: probabilistic beat 1
    float skipProb = (params.shape - 0.7f) / 0.3f * 0.4f;  // 0% at 0.7, 40% at 1.0
    if (HashToFloat(params.seed, 501) < skipProb) {
        result.anchorMask &= ~(1U << 0);
    }
}
```

### Decision 2: AUX Style Control via AXIS Y

**Rule**: User controls AUX style through AXIS Y position, not random hash

**Rationale**:
- Spec Section 1.1: "Zero shift layers: Every parameter is directly accessible"
- Users should be able to influence AUX character without reseeding
- Maintains mental model: AXIS Y = intricacy/complexity applies to AUX too

**Zones**:
| AXIS Y | AUX Style | Character |
|--------|-----------|-----------|
| 0-33% | OFFBEAT_8THS | Classic hat pattern |
| 33-66% | SYNCOPATED_16THS | Funkier, more fills |
| 66-100% | SEED_VARIED | Polyrhythmic, seed-influenced |

**Implementation**:
```cpp
enum class AuxStyle { OFFBEAT_8THS, SYNCOPATED_16THS, SEED_VARIED };

AuxStyle GetAuxStyle(float axisY) {
    if (axisY < 0.33f) return AuxStyle::OFFBEAT_8THS;
    if (axisY < 0.66f) return AuxStyle::SYNCOPATED_16THS;
    return AuxStyle::SEED_VARIED;
}

void GenerateAuxWeights(const PatternParams& params, uint32_t combinedMask, float* auxWeights) {
    AuxStyle style = GetAuxStyle(params.axisY);

    // Seed-based offset only varies within the style
    int auxOffset = static_cast<int>(HashToFloat(params.seed, 201) * 4);

    for (int i = 0; i < params.patternLength; ++i) {
        int shifted = (i + auxOffset) % params.patternLength;
        float weight;

        switch (style) {
            case AuxStyle::OFFBEAT_8THS:
                weight = (shifted % 2 == 1) ? 0.8f : 0.2f;
                break;
            case AuxStyle::SYNCOPATED_16THS:
                weight = ((shifted % 4 == 1) || (shifted % 4 == 3)) ? 0.7f : 0.35f;
                break;
            case AuxStyle::SEED_VARIED:
            default:
                // In high intricacy zone, seed determines style
                int seedStyle = static_cast<int>(HashToFloat(params.seed, 200) * 3);
                if (seedStyle == 0)
                    weight = (shifted % 3 == 0) ? 0.8f : 0.3f;  // Polyrhythm
                else if (seedStyle == 1)
                    weight = ((shifted % 4 == 2) || (shifted % 8 == 5)) ? 0.75f : 0.35f;  // Displaced
                else
                    weight = 1.0f - GetMetricWeight(shifted, params.patternLength) * 0.5f;  // Inverse
                break;
        }

        auxWeights[i] = ClampWeight(weight);
        if ((combinedMask & (1U << i)) != 0)
            auxWeights[i] *= 0.3f;
    }
}
```

### Decision 3: Named Presets are Deterministic

**Rule**: Same preset name + same seed = identical pattern, always

**Rationale**:
- Predictability is essential for recall and live performance
- Variation should come from parameter changes, not preset instability

### Decision 4: Rotation Only Above SHAPE 0.3

**Rule**: Base pattern rotation only applies when SHAPE > 0.3

**Rationale**:
- Spec Section 5.1: "Stable = Humanized euclidean, techno, four-on-floor"
- Rotation would put downbeat on wrong step at low SHAPE
- Users setting SHAPE=0 expect canonical 4/4 structure

**Implementation**:
```cpp
// In GeneratePattern:
if (params.shape > 0.3f && params.shape < 0.7f) {
    // Syncopated zone: allow small rotation
    int maxRotation = std::max(1, params.patternLength / 8);  // Max 2 steps at 16-step
    int rotation = static_cast<int>(HashToFloat(params.seed, 2000) * maxRotation);
    result.anchorMask = RotateWithPreserve(result.anchorMask, rotation, params.patternLength, 0);
}
// SHAPE < 0.3: No rotation (stable zone)
// SHAPE >= 0.7: Rotation handled differently (wild zone chaos)
```

---

## Revised Fitness Metrics

### Composite Score (Unchanged)

```
FITNESS = 0.35*UNIQUENESS + 0.25*COHERENCE + 0.25*GENRE_AUTHENTICITY + 0.15*RESPONSIVENESS
```

### NEW: Groove Quality Sub-Metrics

Added to COHERENCE component to ensure "different" also means "groovy":

| Metric | Formula | Target | Weight |
|--------|---------|--------|--------|
| **Accent Alignment** | hits_on_beats_1_3 / expected_accent_count | >= 70% | 30% |
| **IOI Distribution** | 1 - std_dev(inter_onset_intervals) / mean | >= 60% | 25% |
| **Velocity Correlation** | corr(velocity, beat_grid) | >= 50% | 25% |
| **Density Balance** | 1 - abs(actual_hits - expected_hits) / expected | >= 80% | 20% |

**Accent Alignment**: Measures whether accented hits (velocity > 70%) land on beats 1 and 3. A pattern that randomly scatters accents scores poorly.

**IOI Distribution**: Inter-onset interval (time between consecutive hits). Too clustered (all hits together) or too sparse (huge gaps) scores poorly.

**Velocity Correlation**: Do high-velocity hits correspond to strong metric positions? Random velocity assignment scores near 0%.

**Density Balance**: Does the pattern have approximately the expected number of hits for its ENERGY setting?

### Metric Computation Example

```cpp
float ComputeGrooveQuality(const PatternResult& result, const PatternParams& params) {
    // Accent alignment: check beats 1 and 3 (steps 0, 8 for 16-step)
    int accentPositions[] = {0, 8};
    int accentsOnGrid = 0;
    int totalAccents = 0;

    for (int step = 0; step < result.patternLength; ++step) {
        if ((result.anchorMask & (1U << step)) && result.anchorVelocity[step] > 0.7f) {
            totalAccents++;
            for (int pos : accentPositions) {
                if (step == pos) accentsOnGrid++;
            }
        }
    }
    float accentScore = (totalAccents > 0) ? (float)accentsOnGrid / totalAccents : 0.5f;

    // IOI distribution: compute intervals between hits
    std::vector<int> hitSteps;
    for (int step = 0; step < result.patternLength; ++step) {
        if (result.anchorMask & (1U << step)) hitSteps.push_back(step);
    }

    float ioiScore = ComputeIOIScore(hitSteps, result.patternLength);  // Helper function

    // Velocity correlation with beat grid
    float velCorr = ComputeVelocityCorrelation(result, params.patternLength);

    // Density balance
    int expectedHits = ComputeTargetHits(params.energy, params.patternLength, Voice::ANCHOR, params.shape);
    int actualHits = __builtin_popcount(result.anchorMask);
    float densityScore = 1.0f - std::abs(actualHits - expectedHits) / (float)expectedHits;

    // Weighted combination
    return 0.30f * accentScore + 0.25f * ioiScore + 0.25f * velCorr + 0.20f * densityScore;
}
```

---

## Revised Algorithm Proposals

### Proposal A: Velocity Variation (PRIORITY 1 - Quick Win)

**Concept**: Keep patterns more stable, add MUCH more velocity variation. This addresses variation without pattern complexity.

**Rationale**: The research shows groove comes significantly from velocity dynamics, not just hit placement. Ghost notes (10-30% velocity) vs accents (90-100%) create perceived variety without changing pattern structure.

**Changes**:

1. Expand velocity range with ACCENT parameter:

```cpp
float ComputeAccentVelocity(float accent, int step, int patternLength, uint32_t seed) {
    float metricWeight = GetMetricWeight(step, patternLength);

    // EXPANDED: Velocity range scales more dramatically with ACCENT
    float velocityFloor = 0.85f - accent * 0.65f;   // 85% -> 20% (was 80% -> 30%)
    float velocityCeiling = 0.88f + accent * 0.12f; // 88% -> 100%

    // Map metric weight to velocity
    float baseVelocity = velocityFloor + metricWeight * (velocityCeiling - velocityFloor);

    // NEW: Seed-based ghost note injection
    // ~20% of weak-position hits become ghost notes at high ACCENT
    if (metricWeight < 0.5f && accent > 0.5f) {
        float ghostProb = (accent - 0.5f) * 0.4f;  // 0% at accent=0.5, 20% at accent=1.0
        if (HashToFloat(seed, step + 700) < ghostProb) {
            baseVelocity = 0.15f + HashToFloat(seed, step + 701) * 0.15f;  // 15-30% ghost
        }
    }

    // Micro-variation for human feel
    float variation = 0.02f + accent * 0.06f;
    baseVelocity += (HashToFloat(seed, step + 800) - 0.5f) * variation;

    return std::clamp(baseVelocity, 0.10f, 1.0f);
}
```

2. Add velocity variation to shimmer and AUX:

```cpp
// Shimmer gets independent velocity variation (different seed offset)
result.shimmerVelocity[step] = ComputeAccentVelocity(
    params.accent * 0.7f, step, params.patternLength, params.seed + 0x5000);

// AUX velocity correlates with ENERGY, independent seed
result.auxVelocity[step] = ComputeAccentVelocity(
    params.energy, step, params.patternLength, params.seed + 0xA000);
```

**Impact**: Creates perceived variation without changing hit positions. Computationally free (no extra pattern generation).

**Effort**: Low (2-3 hours)

---

### Proposal B: Micro-Displacement (PRIORITY 2 - Preserves Shape)

**Concept**: Instead of rotating entire patterns, move individual hits +/-1 step based on seed. This preserves overall pattern shape while adding variation.

**Rationale**: Users expect "low SHAPE = downbeat-focused". Full rotation violates this. Micro-displacement adds variety while maintaining the expected character.

**Implementation**:

```cpp
void ApplyMicroDisplacement(uint32_t& hitMask, float shape, uint32_t seed, int patternLength) {
    // Only apply in syncopated zone
    if (shape < 0.3f || shape >= 0.7f) return;

    // Displacement probability scales with shape
    float displacementProb = (shape - 0.3f) / 0.4f * 0.25f;  // 0% at 0.3, 25% at 0.7

    uint32_t newMask = 0;

    for (int step = 0; step < patternLength; ++step) {
        if (!(hitMask & (1U << step))) continue;

        // Don't displace beat 1
        if (step == 0) {
            newMask |= (1U << step);
            continue;
        }

        // Check displacement
        if (HashToFloat(seed, step + 600) < displacementProb) {
            // Determine direction: -1, 0, or +1
            float dirHash = HashToFloat(seed, step + 601);
            int delta = (dirHash < 0.33f) ? -1 : (dirHash > 0.66f) ? 1 : 0;

            int newStep = (step + delta + patternLength) % patternLength;

            // Don't collide with existing hits or displace onto beat 1
            if (newStep != 0 && !(newMask & (1U << newStep))) {
                newMask |= (1U << newStep);
            } else {
                newMask |= (1U << step);  // Keep original position
            }
        } else {
            newMask |= (1U << step);
        }
    }

    hitMask = newMask;
}
```

**Impact**: Adds subtle variation that preserves user expectations about SHAPE zones.

**Effort**: Medium (3-4 hours)

---

### Proposal C: Two-Pass Pattern Generation (PRIORITY 3 - Guarantees Groove)

**Concept**: Generate patterns in two passes:
1. **Skeleton pass**: Place guaranteed downbeats (groove foundation)
2. **Embellishment pass**: Fill based on SHAPE/seed (variation)

**Rationale**: This guarantees the base groove is always present while allowing variation in decoration.

**Implementation**:

```cpp
void GeneratePatternTwoPass(const PatternParams& params, PatternResult& result) {
    result.anchorMask = 0;

    // PASS 1: Skeleton (guaranteed groove foundation)
    // Always include beat 1 and beat 3 for anchor
    result.anchorMask |= (1U << 0);   // Beat 1
    result.anchorMask |= (1U << 8);   // Beat 3 (for 16-step)

    int skeletonHits = 2;
    int targetHits = ComputeTargetHits(params.energy, params.patternLength, Voice::ANCHOR, params.shape);

    // PASS 2: Embellishments (varies with SHAPE/seed)
    int remainingHits = targetHits - skeletonHits;
    if (remainingHits <= 0) return;

    // Generate weights for embellishment positions
    float embellishWeights[kMaxSteps] = {0};
    ComputeShapeBlendedWeights(params.shape, params.energy, params.seed,
                               params.patternLength, embellishWeights);

    // Mask out skeleton positions
    uint32_t eligibility = ~result.anchorMask & ((1U << params.patternLength) - 1);

    // Select embellishments
    uint32_t embellishMask = SelectHitsGumbelTopK(
        embellishWeights, eligibility, remainingHits,
        params.seed, params.patternLength, GetMinSpacingForZone(GetEnergyZone(params.energy))
    );

    result.anchorMask |= embellishMask;

    // Apply micro-displacement to embellishments only
    if (params.shape > 0.3f) {
        ApplyMicroDisplacement(embellishMask, params.shape, params.seed, params.patternLength);
        result.anchorMask = (result.anchorMask & (1U << 0 | 1U << 8)) | embellishMask;
    }
}
```

**Impact**: Guarantees groove skeleton is always present, variation only in embellishments.

**Effort**: Medium (4-5 hours)

---

### Proposal D: Improved Noise Formula (PRIORITY 4 - Fix Iteration 1 Bug)

**Problem**: Iteration 1 proposed:
```cpp
const float baseNoise = 0.15f;
const float shapeNoise = 0.35f * (1.0f - params.shape * 0.5f);
const float noiseScale = baseNoise + shapeNoise;  // 0.325 to 0.50
```

This INCREASES noise at SHAPE=0 (0.50 vs current 0.40), making "stable" patterns LESS stable.

**Fixed Formula**:

```cpp
// Noise should be:
// - ZERO at SHAPE=0 (pure stable zone)
// - Small in syncopated zone (0.15-0.25)
// - Moderate in wild zone (0.25-0.40)

float ComputeNoiseScale(float shape) {
    if (shape < 0.28f) {
        // Stable zone: minimal noise, scales up toward crossfade
        return shape / 0.28f * 0.10f;  // 0 to 0.10
    } else if (shape < 0.68f) {
        // Syncopated zone: moderate noise
        return 0.10f + (shape - 0.28f) / 0.40f * 0.15f;  // 0.10 to 0.25
    } else {
        // Wild zone: higher noise
        return 0.25f + (shape - 0.68f) / 0.32f * 0.15f;  // 0.25 to 0.40
    }
}
```

**Impact**: Preserves stability in low-SHAPE patterns while still adding variation where desired.

**Effort**: Low (1 hour)

---

## Computational Cost Analysis

### Current Budget Estimate

Pattern generation occurs on bar boundaries, not in the audio callback. At 120 BPM with 32-step patterns:
- Bar length: 4 seconds
- Worst case (rapid parameter changes): regeneration up to 15x/second during sweeps

### Baseline Measurements (Estimated)

| Operation | Cycles (approx) | Count | Total |
|-----------|-----------------|-------|-------|
| HashToFloat | 5 | ~20 | 100 |
| ComputeShapeBlendedWeights | 200 | 2 | 400 |
| ApplyAxisBias | 150 | 2 | 300 |
| SelectHitsGumbelTopK | 300 | 3 | 900 |
| **Current Total** | | | **~1700 cycles** |

### Added Cost from Proposals

| Proposal | Added Operations | Cycles Added |
|----------|------------------|--------------|
| A: Velocity Variation | +10 HashToFloat per voice | +150 |
| B: Micro-Displacement | +32 HashToFloat (worst case) | +160 |
| C: Two-Pass | +1 SelectHitsGumbelTopK | +300 |
| D: Noise Formula | Branch logic | +10 |
| **Total Added** | | **~620 cycles** |

### CPU Budget

**Budget**: Generation must complete in < 1ms

At 480 MHz (STM32H7):
- 1ms = 480,000 cycles
- Current estimate: ~1700 cycles
- With proposals: ~2320 cycles
- **Headroom**: ~99.5% remaining

**Conclusion**: Computational cost is negligible. All proposals are safe.

---

## Implementation Priority Order

| Order | Proposal | Effort | Impact | Rationale |
|-------|----------|--------|--------|-----------|
| 1 | D: Noise Formula Fix | 1 hr | Low | Bug fix, prevents regression |
| 2 | A: Velocity Variation | 2-3 hr | High | Quick win, no pattern complexity |
| 3 | B: Micro-Displacement | 3-4 hr | Medium | Adds variation, preserves SHAPE |
| 4 | AUX Style Zones | 2 hr | High | User control over AUX |
| 5 | Beat 1 Rules | 1 hr | High | Locked design decision |
| 6 | C: Two-Pass | 4-5 hr | Medium | More complex, do if needed |

**Total Estimated Effort**: 13-16 hours

---

## Success Criteria

### Quantitative

| Metric | Current | Minimum | Target |
|--------|---------|---------|--------|
| V1 Unique Patterns | 30.4% | 50% | 65% |
| V2 Unique Patterns | 41.7% | 55% | 70% |
| AUX Unique Patterns | 20.0% | 50% | 65% |
| Overall Variation | 30.7% | 50% | 60% |
| Groove Quality | N/A | 60% | 75% |
| CPU Overhead | 0% | <5% | <2% |

### Qualitative

- [ ] Low-SHAPE patterns sound stable (downbeats present)
- [ ] High-SHAPE patterns sound chaotic (IDM-like)
- [ ] AUX character changes with AXIS Y (user control)
- [ ] Velocity variation creates ghost/accent dynamics
- [ ] Beat 1 reliable for DJ mixing (SHAPE < 0.7)
- [ ] Named presets are deterministic

---

## A/B Listening Test Protocol

To validate that changes improve musicality, not just uniqueness:

### Setup

1. Generate 20 patterns with OLD algorithm (matched parameters)
2. Generate 20 patterns with NEW algorithm (same seeds/params)
3. Create 20 A/B pairs (random order)

### Test Procedure

1. Present pair to tester (blind to which is old/new)
2. Play each pattern at 120 BPM for 8 bars
3. Ask: "Which pattern is more groovy/musical?"
4. Record preference and confidence (1-5)

### Success Threshold

- NEW preferred >= 60% of the time
- No category where OLD strongly preferred (>70%)

---

## Trade-offs & Limitations

### What This Doesn't Solve

1. **Fundamental pattern generator diversity**: Base patterns still use same metric weight tables. True diversity would require more generator algorithms.

2. **COMPLEMENT relationship similarity**: Shimmer still fills anchor gaps. If anchor patterns cluster, shimmer follows.

3. **Real genre variety**: This is pattern variation within techno/IDM space. True genre switching (house vs. drum & bass) would require different approaches.

### Accepted Trade-offs

1. **Complexity for AUX**: AXIS Y zones add a mental model to learn, but it's one parameter = one effect, not hidden randomness.

2. **Beat 1 predictability**: Wild zone (SHAPE >= 0.7) is the only place beat 1 varies. This limits IDM potential but ensures live usability.

3. **CPU overhead**: ~35% more cycles for pattern generation. Acceptable given large headroom.

---

## Next Steps

1. Implement Proposal D (noise formula fix) immediately
2. Implement Proposal A (velocity variation) as proof of concept
3. Run fitness evaluator, compare to baseline
4. If targets not met, implement Proposal B (micro-displacement)
5. Implement AUX style zones
6. Run A/B listening tests
7. Document results in iteration-3.md or final.md

---

*End of Iteration 2*
