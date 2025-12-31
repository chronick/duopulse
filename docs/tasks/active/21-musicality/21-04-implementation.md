# Task 21: Musicality Improvements — Implementation Plan

**Status**: READY FOR IMPLEMENTATION
**Created**: 2025-12-30
**Branch**: `feature/musicality-v1`
**Dependencies**: Task 22 Phase B (Balance Range) should ship first or concurrently

---

## Executive Summary

This document consolidates ideation (21-02) and critical feedback (21-03) into a prioritized, incremental implementation plan. The goal is to make DuoPulse v4 patterns feel **musical, varied, and groovy** without over-engineering or breaking determinism.

### Core Principle
**Fix contrast at the source**: Retune archetype weights and hit budgets before adding new modes or systems. Small, well-tested changes compound into significant musicality improvements.

---

## Priority Matrix

| Phase | Change | Risk | Impact | LOC Est. |
|-------|--------|------|--------|----------|
| **A** | Archetype Weight Retuning | Low | High | ~200 |
| **B** | Velocity Contrast Widening | Low | Medium | ~30 |
| **C** | Hit Budget Expansion | Medium | High | ~50 |
| **D** | BUILD Phase Enhancement | Medium | Medium | ~80 |
| **E** | Swing Effectiveness | Medium | Medium | ~40 |
| **F** | Genre-Aware Euclidean Blend | High | High | ~150 |

---

## Phase A: Archetype Weight Retuning (HIGH PRIORITY)

### Problem
Current weight tables cluster around 0.5-0.7, creating similar patterns across archetypes. User feedback: "not feeling like I'm getting very musical beats, nor am I getting much variety."

### Analysis of Current Weights

From `src/Engine/ArchetypeData.h`, examining techno::kGroovy_Anchor:
```cpp
// Current: weights 0.4-1.0, many similar values
1.0f, 0.0f, 0.0f, 0.45f, 0.85f, 0.0f, 0.0f, 0.4f,
0.9f, 0.0f, 0.0f, 0.45f, 0.85f, 0.0f, 0.0f, 0.4f, ...
```

**Issues**:
1. Downbeats (0, 8, 16, 24) at 0.85-1.0 — too similar, no dynamics
2. Ghost positions (3, 7, 11...) at 0.4-0.45 — too weak to ever win selection
3. Minimal archetype nearly identical to Steady (both 4-on-floor)

### Solution: Wider Dynamic Range + Archetype Contrast

**Design philosophy per archetype position**:

| Position | Anchor Character | Weight Strategy |
|----------|------------------|-----------------|
| [0,0] Minimal | Pure 4/4, skeleton | Binary (1.0 or 0.0), no ghosts |
| [1,0] Steady | Backbeat focus | Strong 0.9-1.0, weak 0.15-0.25 ghosts |
| [2,0] Displaced | Skipped beats | Missing beat 3, emphasis on "&" |
| [0,1] Driving | Relentless 8ths | All 8ths at 0.6-0.8, uniform |
| [1,1] Groovy | Swing pocket | Swung "a" subdivisions at 0.5-0.6 |
| [2,1] Broken | Missing downbeats | Beat 1 at 0.7, beat 2 displaced |
| [0,2] Busy | Dense 16ths | Full 16ths with accent pattern |
| [1,2] Poly | 3-over-4 feel | Dotted 8th spacing |
| [2,2] Chaos | Irregular clusters | Variable 0.4-0.8, no structure |

### Code Changes: `src/Engine/ArchetypeData.h`

**Techno Minimal [0,0] — Make truly minimal**:
```cpp
// BEFORE (current):
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f,
    0.95f,0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f, ...
};

// AFTER (wider contrast, pure 4/4):
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  // Beats 1, 2
    1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  // Beats 3, 4
    1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  // Bar 2
    1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f
};
// Pure binary weights = guaranteed 4-on-floor
```

**Techno Groovy [1,1] — Add actual groove with ghost layers**:
```cpp
// BEFORE:
constexpr float kGroovy_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.45f, 0.85f, 0.0f, 0.0f, 0.4f, ...
};

// AFTER (stronger accents, viable ghosts):
constexpr float kGroovy_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.55f,  0.95f, 0.0f, 0.0f, 0.50f,  // Beat 1 strong, "a" ghost
    0.95f,0.0f, 0.0f, 0.55f,  0.90f, 0.0f, 0.0f, 0.50f,  // Beat 2-3
    1.0f, 0.0f, 0.0f, 0.55f,  0.95f, 0.0f, 0.0f, 0.50f,  // Bar 2
    0.95f,0.0f, 0.0f, 0.60f,  0.90f, 0.0f, 0.0f, 0.55f   // End pickup
};
// Ghost weights 0.50-0.60 = actually win selection sometimes
```

**Techno Chaos [2,2] — Maximum irregularity**:
```cpp
// BEFORE (still too structured):
constexpr float kChaos_Anchor[32] = {
    1.0f, 0.45f,0.0f, 0.6f,  0.0f, 0.55f,0.0f, 0.5f, ...
};

// AFTER (more zeros, less predictable):
constexpr float kChaos_Anchor[32] = {
    0.9f, 0.6f, 0.0f, 0.0f,  0.0f, 0.7f, 0.0f, 0.5f,   // Cluster then gap
    0.0f, 0.0f, 0.6f, 0.0f,  0.7f, 0.0f, 0.55f, 0.0f,  // Fragmented
    0.85f,0.0f, 0.0f, 0.6f,  0.0f, 0.0f, 0.65f, 0.0f,  // Shifted
    0.0f, 0.55f,0.0f, 0.0f,  0.6f, 0.0f, 0.0f, 0.7f    // End cluster
};
// More zeros, weight variation 0.5-0.9 range
```

### Shimmer Weight Adjustments

**Key insight**: Shimmer weights should complement, not duplicate anchor.

```cpp
// Groovy shimmer: stronger backbeat, ghost anticipations
constexpr float kGroovy_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.45f,  0.0f, 0.0f, 0.0f, 0.50f,  // Anticipations
    1.0f, 0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 0.0f,   // Strong backbeat
    0.0f, 0.0f, 0.0f, 0.45f,  0.0f, 0.0f, 0.0f, 0.50f,
    1.0f, 0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f, 0.0f    // Bar 2 backbeat
};
```

### Testing Strategy

1. Generate 100 patterns at each archetype position
2. Measure weight distribution of selected hits
3. Verify distinct character at [0,0], [1,1], [2,2] positions
4. Hardware test: sweep Field X/Y, verify audible differences

---

## Phase B: Velocity Contrast Widening (LOW RISK)

### Problem
User feedback: velocity variation feels subtle. Ghost notes at 50-70% don't contrast enough with accents at 85-100%.

### Current Implementation (`src/Engine/VelocityCompute.cpp:26-45`)

```cpp
void ComputePunch(float punch, PunchParams& params)
{
    params.accentProbability = 0.15f + punch * 0.35f;  // 15% to 50%
    params.velocityFloor = 0.70f - punch * 0.40f;      // 70% down to 30%
    params.accentBoost = 0.10f + punch * 0.25f;        // +10% to +35%
    params.velocityVariation = 0.05f + punch * 0.15f;  // ±5% to ±20%
}
```

### Analysis
- At PUNCH=50%: floor=50%, accent=32.5% boost → range 50-82%
- Insufficient contrast for audible ghost/accent difference

### Solution: Widen ranges, lower floor, higher accent

```cpp
// AFTER: Wider dynamics
void ComputePunch(float punch, PunchParams& params)
{
    params.accentProbability = 0.20f + punch * 0.30f;  // 20% to 50%
    params.velocityFloor = 0.65f - punch * 0.35f;      // 65% down to 30%
    params.accentBoost = 0.15f + punch * 0.30f;        // +15% to +45%
    params.velocityVariation = 0.03f + punch * 0.12f;  // ±3% to ±15%
}
```

**Result at PUNCH=50%**:
- Floor: 47.5% (was 50%)
- Accent boost: +30% (was +22.5%)
- Range: 47.5% - 92.5% (wider contrast)

### Minimum Velocity Floor

Per feedback: "velocity floors at 25-30% may be below VCA noise floor."

```cpp
// ComputeVelocity line 136:
// BEFORE:
return Clamp(velocity, 0.2f, 1.0f);

// AFTER: Higher minimum for audibility
return Clamp(velocity, 0.35f, 1.0f);
```

---

## Phase C: Hit Budget Expansion (MEDIUM RISK)

### Problem
User feedback: "K2 (BUILD) effect hard to notice" — density increase is too subtle.

### Current Implementation (`src/Engine/HitBudget.cpp:34-99`)

```cpp
// ComputeAnchorBudget zone budgets:
MINIMAL: min=1, typical=patternLength/16  // 2 hits for 32 steps
GROOVE:  min=2, typical=patternLength/8   // 4 hits for 32 steps
BUILD:   min=3, typical=patternLength/6   // 5 hits for 32 steps
PEAK:    min=4, typical=patternLength/4   // 8 hits for 32 steps
```

### Solution: Increase upper bounds, steeper zone curves

```cpp
// AFTER: More hits at high energy
int ComputeAnchorBudget(float energy, EnergyZone zone, int patternLength)
{
    int maxHits = patternLength / 3;  // Was /4, now more headroom

    int minHits, typicalHits;
    switch (zone) {
        case EnergyZone::MINIMAL:
            minHits = 1;
            typicalHits = std::max(2, patternLength / 16);  // Slightly higher
            break;
        case EnergyZone::GROOVE:
            minHits = 3;  // Was 2
            typicalHits = patternLength / 6;  // Was /8
            break;
        case EnergyZone::BUILD:
            minHits = 4;  // Was 3
            typicalHits = patternLength / 4;  // Was /6
            break;
        case EnergyZone::PEAK:
            minHits = 6;  // Was 4
            typicalHits = patternLength / 3;  // Was /4
            break;
    }
    // ... rest unchanged
}
```

### Balance Range Extension (Coordinated with Task 22 Phase B)

```cpp
// HitBudget.cpp line 114:
// BEFORE:
float shimmerRatio = 0.3f + balance * 0.7f;  // 30% to 100%

// AFTER: Full range 0% to 150%
float shimmerRatio = balance * 1.5f;  // 0% to 150%
```

---

## Phase D: BUILD Phase Enhancement (MEDIUM RISK)

### Problem
BUILD only affects velocity slightly. User wants audible phrase-level arc.

### Current Implementation (`src/Engine/VelocityCompute.cpp:51-79`)

```cpp
void ComputeBuildModifiers(float build, float phraseProgress, BuildModifiers& modifiers)
{
    float rampAmount = build * phraseProgress * 0.5f;  // Up to 50% denser
    modifiers.densityMultiplier = 1.0f + rampAmount;
    modifiers.inFillZone = (phraseProgress > 0.875f);
    // ...
}
```

### Solution: Three-phase BUILD with density + velocity

**Phase structure** (tied to phraseProgress):

| Phrase % | Phase | Density Mult | Velocity Boost |
|----------|-------|--------------|----------------|
| 0-50% | GROOVE | 1.0× | 0% |
| 50-75% | BUILD | 1.0 + build×0.25 | +5% floor |
| 75-87.5% | TENSION | 1.0 + build×0.40 | +10% floor |
| 87.5-100% | FILL | 1.0 + build×0.60 | +15% floor, all accents |

```cpp
// NEW: Enhanced BUILD modifiers
void ComputeBuildModifiers(float build, float phraseProgress, BuildModifiers& modifiers)
{
    build = Clamp(build, 0.0f, 1.0f);
    phraseProgress = Clamp(phraseProgress, 0.0f, 1.0f);

    modifiers.phraseProgress = phraseProgress;

    // Phase determination
    if (phraseProgress < 0.50f) {
        // GROOVE phase: stable, no modification
        modifiers.phase = BuildPhase::GROOVE;
        modifiers.densityMultiplier = 1.0f;
        modifiers.velocityBoost = 0.0f;
    }
    else if (phraseProgress < 0.75f) {
        // BUILD phase: start ramping
        modifiers.phase = BuildPhase::BUILD;
        float phaseProgress = (phraseProgress - 0.50f) / 0.25f;
        modifiers.densityMultiplier = 1.0f + build * 0.25f * phaseProgress;
        modifiers.velocityBoost = build * 0.05f * phaseProgress;
    }
    else if (phraseProgress < 0.875f) {
        // TENSION phase: increasing energy
        modifiers.phase = BuildPhase::TENSION;
        float phaseProgress = (phraseProgress - 0.75f) / 0.125f;
        modifiers.densityMultiplier = 1.0f + build * (0.25f + 0.15f * phaseProgress);
        modifiers.velocityBoost = build * (0.05f + 0.05f * phaseProgress);
    }
    else {
        // FILL phase: maximum energy
        modifiers.phase = BuildPhase::FILL;
        modifiers.densityMultiplier = 1.0f + build * 0.60f;
        modifiers.velocityBoost = build * 0.15f;
        modifiers.forceAccents = (build > 0.5f);  // All hits accented in high BUILD
    }

    modifiers.inFillZone = (modifiers.phase == BuildPhase::FILL);
    modifiers.fillIntensity = modifiers.inFillZone ? build : 0.0f;
}
```

### New Enum for VelocityCompute.h

```cpp
enum class BuildPhase {
    GROOVE,   // 0-50%: stable
    BUILD,    // 50-75%: ramping
    TENSION,  // 75-87.5%: high energy
    FILL      // 87.5-100%: climax
};

struct BuildModifiers {
    BuildPhase phase;
    float phraseProgress;
    float densityMultiplier;
    float velocityBoost;
    float fillIntensity;
    bool inFillZone;
    bool forceAccents;
};
```

---

## Phase E: Swing Effectiveness (MEDIUM RISK)

### Problem
User feedback: "swing does not appear to change timings very much."

### Current Implementation
Swing is applied in BrokenEffects, but the range may be too narrow (50%-66%).

### Analysis
From spec section 7.1:
> Swing: Config K2 (SWING) → 0-100% (straight → heavy triplet)

The timing displacement should be noticeable. Possible issues:
1. Swing timing offset not large enough
2. Swing only applied to certain subdivisions
3. Swing conflicting with GENRE jitter

### Solution: Wider swing range, clearer application

```cpp
// BrokenEffects.cpp - swing computation
float ComputeSwingOffset(int step, float swing, float genreSwing)
{
    // Only swing odd 16th notes (steps 1, 3, 5, 7...)
    if ((step & 1) == 0) return 0.0f;

    // Combine config swing with genre base
    // swing = 0.0: straight (50% position)
    // swing = 1.0: heavy triplet (66% position = +33% offset)
    float totalSwing = swing * (genreSwing + 0.5f);  // Genre amplifies
    totalSwing = Clamp(totalSwing, 0.0f, 1.0f);

    // Convert to timing offset (as fraction of step duration)
    // 0% swing = 0 offset, 100% swing = +0.33 step offset (triplet feel)
    float offset = totalSwing * 0.33f;

    return offset;
}
```

### Swing as Genre Modifier (User Answer from 21-02)

Per user feedback: "SWING should act as a modifier against existing swing settings in GENRE, from 0× to 2× of GENRE swing."

```cpp
// New swing model
float effectiveSwing = genreBaseSwing * (1.0f + configSwing);
// At configSwing=0: 1× genre swing
// At configSwing=1: 2× genre swing
```

---

## Phase F: Genre-Aware Euclidean Blend (HIGH RISK, HIGH REWARD)

### Problem
Probabilistic generation can produce non-musical patterns. User wants guaranteed "safe" patterns at certain positions.

### User Answer (from 21-02)
> "Euclidean/probabilistic dichotomy can be different between each genre (Techno full Euclidean, Tribal middle, IDM mostly probabilistic)."

### Solution: Genre-specific Euclidean blend factor

**Euclidean as fallback/foundation per genre**:

| Genre | Euclidean Weight | Probabilistic Weight |
|-------|------------------|----------------------|
| Techno | 70-100% | 0-30% |
| Tribal | 40-60% | 40-60% |
| IDM | 10-30% | 70-90% |

### Implementation Approach

**New file**: `src/Engine/EuclideanGen.h`

```cpp
#pragma once
#include <cstdint>

namespace daisysp_idm_grids
{

// Euclidean rhythm generator
// E(k, n) distributes k hits evenly across n steps
uint32_t GenerateEuclidean(int hits, int steps);

// Rotate pattern by offset steps
uint32_t RotatePattern(uint32_t pattern, int offset, int steps);

// Blend Euclidean foundation with probabilistic weights
// Returns mask with guaranteed Euclidean hits + probabilistic additions
uint32_t BlendEuclideanWithWeights(
    int budget,
    int steps,
    const float* weights,
    float euclideanRatio,  // 0.0 = pure probabilistic, 1.0 = pure Euclidean
    uint32_t seed
);

} // namespace daisysp_idm_grids
```

**Implementation**: `src/Engine/EuclideanGen.cpp`

```cpp
uint32_t GenerateEuclidean(int hits, int steps)
{
    if (hits <= 0 || steps <= 0) return 0;
    if (hits >= steps) return (steps >= 32) ? 0xFFFFFFFF : ((1U << steps) - 1);

    uint32_t pattern = 0;
    int bucket = 0;

    // Bresenham's line algorithm for even distribution
    for (int i = 0; i < steps; ++i) {
        bucket += hits;
        if (bucket >= steps) {
            bucket -= steps;
            pattern |= (1U << i);
        }
    }

    return pattern;
}

uint32_t BlendEuclideanWithWeights(
    int budget, int steps, const float* weights,
    float euclideanRatio, uint32_t seed)
{
    // Euclidean portion
    int euclideanHits = static_cast<int>(budget * euclideanRatio + 0.5f);
    uint32_t euclideanMask = GenerateEuclidean(euclideanHits, steps);

    // Optional rotation based on seed
    int rotation = (seed % steps);
    euclideanMask = RotatePattern(euclideanMask, rotation, steps);

    // Probabilistic portion fills remaining budget
    int remainingBudget = budget - __builtin_popcount(euclideanMask);
    if (remainingBudget <= 0) return euclideanMask;

    // Use existing Gumbel Top-K for remaining hits, excluding Euclidean positions
    uint32_t eligibility = ((1U << steps) - 1) & ~euclideanMask;
    uint32_t additionalHits = GumbelTopK(weights, eligibility, remainingBudget, seed);

    return euclideanMask | additionalHits;
}
```

### Integration with Genre

```cpp
// PatternField.cpp or GenerationPipeline.cpp
float GetGenreEuclideanRatio(Genre genre, float fieldX)
{
    // Base ratios per genre
    float baseRatio;
    switch (genre) {
        case Genre::TECHNO:
            baseRatio = 0.85f;  // Mostly Euclidean
            break;
        case Genre::TRIBAL:
            baseRatio = 0.50f;  // Balanced
            break;
        case Genre::IDM:
            baseRatio = 0.20f;  // Mostly probabilistic
            break;
        default:
            baseRatio = 0.50f;
    }

    // Field X reduces Euclidean (more syncopation = more probabilistic)
    return baseRatio * (1.0f - fieldX * 0.5f);
}
```

---

## Consistency with Other Tasks

### Task 22: Control Simplification
- **Phase B (Balance Range)**: This task assumes 0-150% range ships concurrently
- **Phase C (Coupling)**: BUILD velocity changes complement reduced coupling modes

### Task 23: Immediate Field Updates
- Archetype weight changes make Field X/Y more impactful
- Regeneration triggers will immediately show new archetype character

### Task 24: Power-On Behavior
- Boot defaults should land at Techno [1,1] Groovy — guaranteed musical
- PUNCH=50%, BUILD=50% = good starting dynamics

### Task 25: VOICE Redesign (Backlog)
- Balance range extension (Phase C here) provides 80% of VOICE benefit
- Full VOICE redesign can wait for user feedback on these changes

---

## Spec Updates Required

### Section 7.2 (Velocity Computation)

**Current**:
> - velocityFloor: Minimum velocity for non-accented hits (70% down to 30%)
> - accentBoost: How much louder accents are (+10% to +35%)

**Updated**:
> - velocityFloor: Minimum velocity for non-accented hits (65% down to 30%)
> - accentBoost: How much louder accents are (+15% to +45%)
> - Minimum output velocity: 35% (for VCA audibility)

### Section 4.4 (BUILD Parameter)

**Add**:
> BUILD operates in four phases across the phrase:
> 1. **GROOVE (0-50%)**: Stable pattern, no modification
> 2. **BUILD (50-75%)**: Density +0-25%, velocity +0-5%
> 3. **TENSION (75-87.5%)**: Density +25-40%, velocity +5-10%
> 4. **FILL (87.5-100%)**: Density +40-60%, velocity +10-15%, all accents

### Section 6 (Generation Pipeline)

**Add after 6.3**:
> ### 6.3.1 Euclidean Foundation (Genre-Dependent)
>
> Before Gumbel Top-K selection, an optional Euclidean foundation guarantees even hit distribution:
> - **Techno**: 70-100% Euclidean, ensures four-on-floor at low complexity
> - **Tribal**: 40-60% Euclidean, balances structure with polyrhythm
> - **IDM**: 10-30% Euclidean, allows maximum irregularity
>
> Field X (syncopation) reduces Euclidean ratio by up to 50%.

---

## Implementation Order

### Sprint 1: Low-Risk, High-Impact (Days 1-2)
1. **A1**: Tune Techno Minimal weights (pure 4/4)
2. **A2**: Tune Techno Groovy weights (ghost layers)
3. **A3**: Tune Techno Chaos weights (more zeros)
4. **B1**: Widen velocity floor/boost ranges
5. **B2**: Raise minimum velocity to 35%

### Sprint 2: Medium-Risk Changes (Days 3-4)
1. **C1**: Increase hit budget upper bounds
2. **C2**: Balance range extension (with Task 22 B)
3. **D1**: Implement BuildPhase enum
4. **D2**: Three-phase BUILD density ramp
5. **D3**: BUILD velocity boost

### Sprint 3: Architecture (Days 5-7)
1. **E1**: Swing range widening
2. **E2**: Swing as genre modifier
3. **F1**: EuclideanGen module
4. **F2**: Genre-aware blend integration

### Hardware Validation
After each sprint:
- [ ] Generate patterns at all 9 field positions per genre
- [ ] Verify distinct character audibly
- [ ] Sweep ENERGY 0-100%, verify zone transitions
- [ ] Sweep BUILD 0-100%, verify phrase arc
- [ ] Test swing at 0%, 50%, 100%

---

## Files Modified

| File | Changes |
|------|---------|
| `src/Engine/ArchetypeData.h` | Weight table retuning (~200 lines) |
| `src/Engine/VelocityCompute.h` | Add BuildPhase enum, modify BuildModifiers |
| `src/Engine/VelocityCompute.cpp` | Widen velocity ranges, BUILD phases |
| `src/Engine/HitBudget.cpp` | Budget expansion, balance range |
| `src/Engine/BrokenEffects.cpp` | Swing effectiveness |
| `src/Engine/EuclideanGen.h` | New file |
| `src/Engine/EuclideanGen.cpp` | New file |
| `src/Engine/GenerationPipeline.cpp` | Euclidean integration |
| `docs/specs/main.md` | Spec updates per above |

---

## Success Criteria

### Musicality
- [ ] Techno [0,0] Minimal produces reliable 4-on-floor
- [ ] Techno [1,1] Groovy produces head-nodding groove
- [ ] Techno [2,2] Chaos produces interesting irregularity
- [ ] Clear audible difference across all 9 field positions

### Dynamics
- [ ] PUNCH 0% sounds flat/hypnotic
- [ ] PUNCH 100% sounds punchy with pop
- [ ] Ghost notes audible but clearly softer than accents

### Phrase Arc
- [ ] BUILD 0% = flat, no variation
- [ ] BUILD 50% = subtle build to phrase end
- [ ] BUILD 100% = dramatic arc, obvious fills

### Swing
- [ ] SWING 0% = straight feel
- [ ] SWING 100% = obvious triplet swing

---

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Weight changes break existing patterns | Keep old weights as fallback, A/B test |
| Euclidean layer fights field morphing | Tune genre ratios conservatively |
| BUILD phases too complex | Start with 2 phases, expand if needed |
| Velocity changes too aggressive | Test on hardware with real VCA |

---

## Appendix: Reference Patterns

### Target: Techno [1,1] Groovy Anchor

```
Step:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
Hit:   K   -   -   g   K   -   -   g   K   -   -   g   K   -   -   g
Vel:  100  -   -  60  95   -   -  55  95   -   -  60  90   -   -  55

Legend: K = kick (accent), g = ghost note, - = rest
Pattern: Four-on-floor with "a" subdivision ghosts for shuffle feel
```

### Target: Techno [1,1] Groovy Shimmer

```
Step:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
Hit:   -   -   -   a   -   -   -   a   S   -   -   -   -   -   -   a
Vel:   -   -   -  50   -   -   -  55 100   -   -   -   -   -   -  50

Legend: S = snare (accent), a = anticipation ghost, - = rest
Pattern: Backbeat with anticipation notes creating push/pull
```

---

*End of Implementation Plan*
