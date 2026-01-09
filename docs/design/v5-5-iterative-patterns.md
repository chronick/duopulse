# DuoPulse v5.5: Pattern Generation Algorithm Iteration

**Version**: 5.5 (Revised)
**Status**: Design Phase - Iteration 2
**Author**: Design Iteration Session
**Date**: 2026-01-08
**Last Revised**: 2026-01-08

---

## Executive Summary

The current v5 pattern generation algorithm achieves only 30.7% overall variation score across 116 test patterns. This document proposes algorithmic improvements to achieve >60% variation while maintaining musical coherence and genre authenticity.

### Revision 2 Key Changes

Iteration 2 addresses critical feedback from design review:

1. **Simpler solutions preferred**: Velocity variation and micro-displacement over complex pattern rotation
2. **User control over AUX**: AXIS Y zones select AUX style, not random hash
3. **Beat 1 guaranteed**: Always present when SHAPE < 0.7 (live performance safety)
4. **Groove quality metrics**: Uniqueness != musicality; now measuring accent alignment and IOI
5. **SHAPE contract honored**: No rotation in stable zone (SHAPE < 0.3)

---

## Table of Contents

1. [Problem Analysis](#1-problem-analysis)
2. [Research Synthesis](#2-research-synthesis)
3. [Design Decisions (Locked)](#3-design-decisions-locked)
4. [Fitness Metrics Framework](#4-fitness-metrics-framework)
5. [Proposed Algorithm Changes](#5-proposed-algorithm-changes)
6. [Implementation Roadmap](#6-implementation-roadmap)
7. [Testing Strategy](#7-testing-strategy)
8. [Appendices](#8-appendices)

---

## 1. Problem Analysis

### 1.1 Current State

From `docs/patterns.summary.txt`:

| Voice | Unique Patterns | Total | Variation |
|-------|-----------------|-------|-----------|
| V1 (Anchor) | 36 | 116 | 30.4% |
| V2 (Shimmer) | 49 | 116 | 41.7% |
| AUX | 24 | 116 | 20.0% |
| **Overall** | - | - | **30.7%** |

Named presets score 64.4%, indicating the algorithm CAN produce variation when parameters differ significantly.

### 1.2 Per-Step Activation Analysis

The per-step frequency table reveals problematic clustering:

**V1 (Anchor) Hot Spots**:
- Step 0: 97% activation (beat 1 lock)
- Step 4: 84% activation
- Step 3: 77% activation
- Step 26: 72% activation

**Low Entropy Steps** (>80% or <20% activation):
- Steps 0, 2, 4: Near-guaranteed V1
- Steps 1, 2, 5, 6, 7, 9, 12, 13, 14: Near-guaranteed no V1

This means V1 patterns are largely predictable from just a few step positions.

### 1.3 Root Causes Identified

1. **Static Base Pattern Generators**
   - `GenerateStablePattern()` uses fixed metric weight table
   - Seed variation only added as small post-processing noise
   - Two patterns with same SHAPE but different seeds start nearly identical

2. **Step 0 Preservation**
   - `RotateWithPreserve()` always keeps step 0 in place
   - Guard rails force beat 1 kick in techno
   - Combined effect: 97% step 0 activation

3. **Shimmer Dependency on Anchor**
   - COMPLEMENT relationship fills gaps in anchor pattern
   - Similar anchor patterns produce similar gap structures
   - Shimmer variation limited by anchor variation

4. **AUX Fixed Formula**
   - Uses fixed `1.0 - metricWeight * 0.5f` formula
   - Only variation from avoiding main voice positions
   - Produces only 24 unique patterns from 116

---

## 2. Research Synthesis

### 2.1 Genre-Specific Pattern Characteristics

Research compiled in `resources/drum-patterns-research.md` identifies key differentiators:

| Genre | Kick Pattern | Hi-Hat Emphasis | Key Variation Source |
|-------|--------------|-----------------|----------------------|
| Four on Floor | Every quarter | 8ths | Velocity, ghost notes |
| House | Every quarter | Offbeat opens | Hat articulation |
| Breakbeat | Displaced | Ride cymbal | Step displacement |
| Dub Techno | Soft 4/4 | Offbeat | Processing, not pattern |
| IDM | Irregular | Irregular | Everything varies |
| Gabber | Melodic distorted | Minimal | Pitch modulation |

### 2.2 Variation Techniques from Research

1. **Velocity Dynamics**: Ghost notes vs. accents (house, funk) - **PRIORITY**
2. **Micro-Displacement**: Moving individual hits +/-1 step (breakbeat) - **PRIORITY**
3. **Swing Amount**: 0% to ~70% (all genres)
4. **Pattern Rotation**: Shifting entire pattern (polyrhythm) - **RESTRICTED**
5. **Polyrhythmic Layering**: 3 over 4, 5 over 4, etc. (IDM, tribal)

### 2.3 Key Insight: IDM in 4/4

> "Classic '90s IDM is often held up as an example of odd time signatures, but actually, a great deal of it is in plain old 4/4. Producers were just very good at programming MIDI in a way that makes the underlying sequencer grid extremely hard to perceive."

This suggests we can achieve IDM-level variation WITHOUT abandoning 4/4 structure - we just need to make the grid less predictable through velocity and micro-timing.

---

## 3. Design Decisions (Locked)

These decisions are finalized and should not be revisited without explicit user request.

### 3.1 Beat 1 Presence

**Rule**: Beat 1 (step 0) is ALWAYS present when SHAPE < 0.7

| Zone | Beat 1 Behavior |
|------|-----------------|
| Stable (0-30%) | Always present |
| Syncopated (30-70%) | Always present |
| Wild (70-100%) | Probabilistic skip (up to 40% at SHAPE=1.0) |

**Rationale**: DJs need predictable beat 1 for mixing. Wild zone is explicitly experimental.

### 3.2 AUX Style Control

**Rule**: AXIS Y controls AUX style through zones

| AXIS Y | AUX Style | Character |
|--------|-----------|-----------|
| 0-33% | OFFBEAT_8THS | Classic hat pattern |
| 33-66% | SYNCOPATED_16THS | Funkier, more fills |
| 66-100% | SEED_VARIED | Polyrhythmic, seed-influenced |

**Rationale**: Spec says "Zero shift layers: Every parameter is directly accessible." Users should control AUX character, not random hash.

### 3.3 Named Preset Determinism

**Rule**: Same preset name + same seed = identical pattern, always

**Rationale**: Predictability essential for recall and live performance.

### 3.4 Rotation Restrictions

**Rule**: Base pattern rotation only in syncopated zone (SHAPE 0.3-0.7)

| Zone | Rotation Allowed |
|------|------------------|
| Stable (0-30%) | NO - preserves four-on-floor |
| Syncopated (30-70%) | YES - up to 2 steps |
| Wild (70-100%) | Handled by chaos, not rotation |

**Rationale**: Spec Section 5.1 promises "Stable = four-on-floor". Rotation violates this.

---

## 4. Fitness Metrics Framework

### 4.1 Composite Score

```
FITNESS = 0.35*UNIQUENESS + 0.25*COHERENCE + 0.25*GENRE_AUTHENTICITY + 0.15*RESPONSIVENESS
```

### 4.2 UNIQUENESS (35% weight)

Measures pattern diversity:

| Metric | Formula | Target |
|--------|---------|--------|
| Unique Ratio | unique_patterns / total_patterns | >= 60% |
| Avg Distance | avg(hamming_distance(Pi, Pj)) | >= 25% |
| Step Entropy | avg(binary_entropy(step_prob)) | >= 60% |

### 4.3 COHERENCE with Groove Quality (25% weight)

**NEW in v5.5r2**: Coherence now includes groove quality sub-metrics:

| Metric | Description | Target | Weight |
|--------|-------------|--------|--------|
| Accent Alignment | Accents on beats 1/3 | >= 70% | 30% |
| IOI Distribution | Even inter-onset intervals | >= 60% | 25% |
| Velocity Correlation | High velocity on strong beats | >= 50% | 25% |
| Density Error | Actual vs expected hits | < 20% | 20% |

**Why Added**: Iteration 1 metrics measured "different" but not "groovy". A random generator scores 100% on uniqueness but is useless. These metrics ensure patterns are musically valid.

### 4.4 GENRE_AUTHENTICITY (25% weight)

- Each genre has expected step probability profile
- Pattern scored against profile
- Tolerance varies by genre (tighter for techno, looser for IDM)

### 4.5 RESPONSIVENESS (15% weight)

- Sensitivity gradient: pattern changes proportionally with params
- Dead zone detection: no ranges where 10%+ sweep produces no change
- Monotonicity: ENERGY up -> density up, etc.

### 4.6 Targets

| Level | Composite | Uniqueness | Coherence |
|-------|-----------|------------|-----------|
| Current | ~0.50 | 0.31 | ~0.65 |
| Minimum Viable | 0.55 | 0.50 | 0.60 |
| Good | 0.60 | 0.55 | 0.70 |
| Target | 0.65 | 0.60 | 0.75 |

---

## 5. Proposed Algorithm Changes

### 5.1 Change A: Enhanced Velocity Variation (PRIORITY 1)

**Concept**: Add more velocity variation (ghost notes, accents) without changing pattern structure.

**Rationale**: Research shows groove comes from velocity dynamics. This is computationally free and addresses variation without complexity.

**Current**:
```cpp
float velocityFloor = 0.80f - accent * 0.50f;    // 80% -> 30%
float velocityCeiling = 0.88f + accent * 0.12f;  // 88% -> 100%
```

**Proposed**:
```cpp
float velocityFloor = 0.85f - accent * 0.65f;    // 85% -> 20%
float velocityCeiling = 0.88f + accent * 0.12f;  // 88% -> 100%

// NEW: Ghost note injection for weak positions
if (metricWeight < 0.5f && accent > 0.5f) {
    float ghostProb = (accent - 0.5f) * 0.4f;  // Up to 20% ghost chance
    if (HashToFloat(seed, step + 700) < ghostProb) {
        baseVelocity = 0.15f + HashToFloat(seed, step + 701) * 0.15f;
    }
}
```

**Impact**: Perceived variation without pattern complexity. No added CPU cost.

### 5.2 Change B: Micro-Displacement (PRIORITY 2)

**Concept**: Move individual hits +/-1 step based on seed. Preserves overall pattern shape.

**Current**: Full pattern rotation via `RotateWithPreserve()`

**Proposed**:
```cpp
void ApplyMicroDisplacement(uint32_t& hitMask, float shape, uint32_t seed, int patternLength) {
    if (shape < 0.3f || shape >= 0.7f) return;  // Only syncopated zone

    float displacementProb = (shape - 0.3f) / 0.4f * 0.25f;  // 0% at 0.3, 25% at 0.7

    for (int step = 0; step < patternLength; ++step) {
        if (!(hitMask & (1U << step))) continue;
        if (step == 0) continue;  // Never displace beat 1

        if (HashToFloat(seed, step + 600) < displacementProb) {
            float dirHash = HashToFloat(seed, step + 601);
            int delta = (dirHash < 0.33f) ? -1 : (dirHash > 0.66f) ? 1 : 0;
            int newStep = (step + delta + patternLength) % patternLength;

            // No collision, no beat 1
            if (newStep != 0 && !(hitMask & (1U << newStep))) {
                hitMask ^= (1U << step);
                hitMask |= (1U << newStep);
            }
        }
    }
}
```

**Impact**: Adds variation while preserving user expectation that "low SHAPE = downbeat-focused".

### 5.3 Change C: AUX Style Zones (PRIORITY 3)

**Concept**: AXIS Y selects AUX style, giving user control.

**Current**:
```cpp
auxWeights[i] = 1.0f - metricW * 0.5f;  // Always same formula
```

**Proposed**:
```cpp
AuxStyle GetAuxStyle(float axisY) {
    if (axisY < 0.33f) return AuxStyle::OFFBEAT_8THS;
    if (axisY < 0.66f) return AuxStyle::SYNCOPATED_16THS;
    return AuxStyle::SEED_VARIED;
}

// Style implementation varies weight generation
switch (style) {
    case OFFBEAT_8THS:    weight = (step % 2 == 1) ? 0.8f : 0.2f; break;
    case SYNCOPATED_16THS: weight = ((step % 4 == 1) || (step % 4 == 3)) ? 0.7f : 0.35f; break;
    case SEED_VARIED:      /* polyrhythm or inverse, seed-selected */ break;
}
```

**Impact**: User controls AUX character via AXIS Y. Matches "Zero shift layers" principle.

### 5.4 Change D: Fixed Noise Formula (PRIORITY 4)

**Problem**: Iteration 1 formula increased noise at SHAPE=0, making stable patterns less stable.

**Proposed**:
```cpp
float ComputeNoiseScale(float shape) {
    if (shape < 0.28f) {
        return shape / 0.28f * 0.10f;  // 0 to 0.10 (minimal in stable)
    } else if (shape < 0.68f) {
        return 0.10f + (shape - 0.28f) / 0.40f * 0.15f;  // 0.10 to 0.25
    } else {
        return 0.25f + (shape - 0.68f) / 0.32f * 0.15f;  // 0.25 to 0.40
    }
}
```

**Impact**: Preserves stability in low-SHAPE patterns.

### 5.5 Change E: Beat 1 Enforcement (PRIORITY 5)

**Concept**: Implement locked design decision about beat 1.

```cpp
// In GeneratePattern, after all other processing:
if (params.shape < 0.7f) {
    result.anchorMask |= (1U << 0);  // Beat 1 always present
} else {
    // Wild zone: probabilistic
    float skipProb = (params.shape - 0.7f) / 0.3f * 0.4f;
    if (HashToFloat(params.seed, 501) >= skipProb) {
        result.anchorMask |= (1U << 0);
    }
}
```

---

## 6. Implementation Roadmap

### Phase 1: Bug Fixes (1 hour)

1. **Change D**: Fix noise formula (prevents regression)

### Phase 2: Quick Wins (3-4 hours)

2. **Change A**: Enhanced Velocity Variation
   - Highest impact, lowest risk
   - No pattern structure changes

3. **Change E**: Beat 1 Enforcement
   - Simple guard, locked decision

### Phase 3: Core Changes (5-6 hours)

4. **Change C**: AUX Style Zones
   - User control over AUX
   - Medium complexity

5. **Change B**: Micro-Displacement
   - Replaces full rotation
   - Preserves SHAPE contract

### Phase 4: Validation (2-3 hours)

6. Run fitness evaluator with groove quality metrics
7. Generate pattern visualizations
8. A/B listening tests
9. Verify CPU budget (<1ms generation)

**Total Estimated Effort**: 13-16 hours

---

## 7. Testing Strategy

### 7.1 Automated Tests

```cpp
TEST_CASE("Pattern variation meets targets")
{
    std::vector<PatternResult> patterns;
    GenerateTestPatterns(patterns, 116);

    FitnessResult fitness = EvaluateFitness(patterns);

    REQUIRE(fitness.uniqueness >= 0.50);
    REQUIRE(fitness.coherence >= 0.60);
    REQUIRE(fitness.grooveQuality >= 0.60);
}

TEST_CASE("Beat 1 presence by SHAPE zone")
{
    for (float shape = 0.0f; shape < 0.7f; shape += 0.1f) {
        PatternParams params = DefaultParams();
        params.shape = shape;
        PatternResult result;
        GeneratePattern(params, result);

        REQUIRE((result.anchorMask & 1U) != 0);  // Beat 1 always present
    }
}

TEST_CASE("AUX style varies with AXIS Y")
{
    std::set<uint32_t> lowYPatterns, highYPatterns;

    for (int i = 0; i < 50; ++i) {
        PatternParams params = DefaultParams();
        params.seed = i;
        params.axisY = 0.1f;
        PatternResult result;
        GeneratePattern(params, result);
        lowYPatterns.insert(result.auxMask);
    }

    for (int i = 0; i < 50; ++i) {
        PatternParams params = DefaultParams();
        params.seed = i;
        params.axisY = 0.9f;
        PatternResult result;
        GeneratePattern(params, result);
        highYPatterns.insert(result.auxMask);
    }

    // Should have different distributions
    REQUIRE(lowYPatterns != highYPatterns);
}
```

### 7.2 A/B Listening Test Protocol

1. Generate 20 pattern pairs (old vs new algorithm, same params)
2. Present to 3+ testers, random order, blind
3. Ask: "Which is more groovy/musical?"
4. Success: NEW preferred >= 60%

### 7.3 Regression Checks

- Build compiles without warnings
- No heap allocations in audio path
- Same seed + params = same output (determinism)
- CPU usage < 1ms per generation

---

## 8. Appendices

### 8.1 Files Created

| File | Purpose |
|------|---------|
| `docs/design/v5-5-iterative-patterns/` | Session directory |
| `docs/design/v5-5-iterative-patterns/README.md` | Session overview |
| `docs/design/v5-5-iterative-patterns/iteration-1.md` | First iteration |
| `docs/design/v5-5-iterative-patterns/iteration-2.md` | Second iteration (current) |
| `docs/design/v5-5-iterative-patterns/critique-1.md` | Design review feedback |
| `docs/design/v5-5-iterative-patterns/resources/` | Research documents |
| `docs/design/v5-5-iterative-patterns.md` | This document |

### 8.2 Key Source Files to Modify

| File | Changes |
|------|---------|
| `src/Engine/VelocityCompute.cpp` | Change A (velocity variation) |
| `src/Engine/PatternGenerator.cpp` | Changes B, E (displacement, beat 1) |
| `src/Engine/PatternField.cpp` | Change D (noise formula) |
| `src/Engine/AuxGenerator.cpp` | Change C (AUX style zones) |
| `tools/pattern_viz/main.cpp` | Groove quality metrics |

### 8.3 Computational Cost

| Operation | Current | Added | Total |
|-----------|---------|-------|-------|
| HashToFloat calls | ~20 | ~45 | ~65 |
| Pattern generation | ~1700 cycles | ~620 cycles | ~2320 cycles |
| Time at 480 MHz | ~3.5us | ~1.3us | ~4.8us |

**Budget**: < 1ms. Actual: ~5us. **Headroom**: 99.5%

### 8.4 Trade-offs Acknowledged

1. **Beat 1 predictability limits IDM**: Wild zone only allows probabilistic skip
2. **AUX zones add mental model**: One more thing to learn (AXIS Y controls AUX style)
3. **Micro-displacement is subtle**: Less dramatic than full rotation

### 8.5 References

- [LANDR - 808 Beats](https://blog.landr.com/808-beats-patterns/)
- [MusicRadar - Jeff Mills 909](https://www.musicradar.com/how-to/program-jeff-mills-style-909-drums-1)
- [Splice - Aphex Twin Drums](https://splice.com/blog/programming-drums-aphex-twin-style/)
- [Attack Magazine - Basic Channel Dub Techno](https://www.attackmagazine.com/technique/beat-dissected/basic-channel-style-dub-techno/)
- [MusicRadar - Amen Break](https://www.musicradar.com/tuition/tech/how-to-program-an-amen-style-break-637374)

---

*End of Design Document - Iteration 2*
