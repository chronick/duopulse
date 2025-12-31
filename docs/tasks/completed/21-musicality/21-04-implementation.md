# Task 21: Musicality Improvements — Implementation Plan

**Status**: ✅ IMPLEMENTATION COMPLETE
**Created**: 2025-12-30
**Branch**: `impl/21-musicality-improvements`
**Depends On**: Task 22 Phase B (Balance Range) — coordinate shimmer ratio change

---

## Executive Summary

This document consolidates ideation (21-02) and critical feedback (21-03) into a prioritized, incremental implementation plan. The goal is to make DuoPulse v4 patterns feel **musical, varied, and groovy** without over-engineering or breaking determinism.

### Core Principle
**Fix contrast at the source**: Retune archetype weights and hit budgets before adding new modes or systems. Small, well-tested changes compound into significant musicality improvements.

---

## Critical Review Checklist

Feedback from spec/code review pass. Items marked with ✅ are accepted; ~~strikethrough~~ indicates rejection with rationale.

### Determinism & Architecture
- [x] **Determinism guardrail**: Euclidean blend, BUILD phases, and swing changes must stay seed-stable and avoid hidden state (`docs/specs/main.md` section 1.3: "same settings + same seed = identical output")
- [x] **Field blending**: Binary 0/1 weights collapse softmax gradients in `src/Engine/PatternField.cpp:251-252`; preserve small gradients for morphing
- [x] **Hit budget clamps**: `ComputeAnchorBudget()` caps at `patternLength/4` (line 41), `ComputeBarBudget()` clamps to `length/2` (line 191); any `/3` expansion must raise caps too
- [x] **64-step two-half generation**: Budget/eligibility applied per 32-step half in `src/Engine/Sequencer.cpp:290-410`

### Swing Pipeline
- [x] **Current swing path**: `ComputeSwing()` at `src/Engine/BrokenEffects.cpp:261` uses config knob, caps by zone (0.58/0.62/0.66), applies before jitter/displacement; start from existing pipeline, don't introduce new offset model
- ~~**flavorCV still used for jitter**~~: Flavor CV was removed in v4 per spec section 3.1; jitter now uses GENRE via archetype swing amounts

### Velocity
- [x] **Current clamp at 0.2**: `src/Engine/VelocityCompute.cpp:136` — raising to 0.35 affects PUNCH=0 dynamics
- [x] **Also affects BrokenEffects**: `GetVelocityWithVariation()` at `src/Engine/BrokenEffects.cpp:153` may need coordinated update
- [x] **Hardware check before locking**: Prototype 0.30-0.32 first per 21-02 uVCA note

### BUILD Modifiers
- [x] **BuildModifiers struct location**: `src/Engine/ControlState.h:70-93` — adding fields requires migrating callers
- [x] **Consider simpler 3-stage model**: groove → build → fill (vs 4-stage) to reduce hidden state
- [x] **Tie to phrase length**: Phase boundaries must respect configured phrase length and reset (Task 19)

### Euclidean Layer
- [x] **Fallback, not foundation**: Use Euclidean as low-Field-X blend to preserve archetype personality
- [x] **Support all pattern lengths**: 16/24/32/64, including two-half path for 64-step
- [x] **Gate by genre/zone**: Start techno-only at low Field X; don't flatten IDM/Tribal character

---

## Cross-Task Dependencies

### Task 22: Control Simplification
| This Task Phase | Task 22 Phase | Coordination |
|-----------------|---------------|--------------|
| Phase C: Balance 0-150% | Phase B | **Ship together** — same line change in `HitBudget.cpp:114` |
| Phase D: BUILD phases | Phase C (Coupling) | Complement reduced coupling modes |

**Action**: Add note to `docs/tasks/active/22-control-simplification.md`:
> Phase B coordinates with Task 21 Phase C (Balance Range Extension)

### Task 23: Immediate Field Updates
| Impact | Coordination |
|--------|--------------|
| Weight retuning makes Field X/Y changes more audible | Supports Task 23 goals |
| Archetype contrast improvements benefit immediate regeneration | No blocking dependency |

**Action**: Add note to `docs/tasks/active/23-immediate-field-updates.md`:
> Benefits from Task 21 archetype weight retuning (more audible field position differences)

### Task 24: Power-On Behavior
| Impact | Coordination |
|--------|--------------|
| Boot defaults at Techno [1,1] Groovy = guaranteed musical | Task 21 ensures this position is groovy |
| PUNCH=50%, BUILD=50% as defaults | Task 21 widens dynamic range at these values |

**Action**: Add note to `docs/tasks/active/24-power-on-behavior.md`:
> Boot defaults (Techno [1,1], PUNCH=50%, BUILD=50%) validated by Task 21 musicality improvements

### Task 25: VOICE Redesign (Backlog)
| Impact | Coordination |
|--------|--------------|
| Phase C Balance 0-150% provides 80% of VOICE benefit | Task 25 can wait for user feedback |

### Task 08: Bulletproof Clock (Backlog)
| Impact | Coordination |
|--------|--------------|
| Swing timing changes must not violate clock edge timing | Add regression test for external clock + swing |

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

## Phase A: Archetype Weight Retuning

### Problem
Current weight tables cluster around 0.5-0.7, creating similar patterns across archetypes.

### Subtasks

| ID | Task | File:Line | Status |
|----|------|-----------|--------|
| A0 | Add hit histogram test to benchmark current selection stats | `tests/test_generation.cpp` (new section) | ✅ |
| A1 | Tune Techno Minimal weights (4/4 with blend gradients) | `src/Engine/ArchetypeData.h:77-84` | ✅ |
| A2 | Tune Techno Groovy weights (viable ghost layers 0.50-0.60) | `src/Engine/ArchetypeData.h:190-196` | ✅ |
| A3 | Tune Techno Chaos weights (more zeros, 0.5-0.9 range) | `src/Engine/ArchetypeData.h:302-308` | ✅ |
| A4 | Tune Techno shimmer tables for complementary patterns | `src/Engine/ArchetypeData.h:86-92, 198-204, 310-316` | ✅ |
| A5 | Apply consistent changes to Tribal genre bank | `src/Engine/ArchetypeData.h:380-633` | ✅ |
| A6 | Apply consistent changes to IDM genre bank | `src/Engine/ArchetypeData.h:689-942` | ✅ |
| A7 | Run hit histogram test, compare before/after | `tests/test_generation.cpp` | ✅ |

### Feedback Integration

- [x] **Avoid pure 0/1 tables for Minimal**: Use gradients (0/0.25/0.6/0.95) so `PatternField.cpp:251` softmax blends remain meaningful
  - **Implementation**: Minimal weights use 0.0/0.2/0.95/1.0 gradient instead of pure 0/1
- [x] **Instrument selection stats**: Hit histograms per archetype/zone to prove variety gains
  - **Implementation**: Add `TEST_CASE("hit histogram by archetype")` in test_generation.cpp
- [x] **Cover all genres**: Updates across Techno/Tribal/IDM banks
  - **Implementation**: Subtasks A5, A6 cover all three banks
- [x] **Guard rails don't supply fundamentals**: Weights must produce valid patterns without rail intervention for IDM flexibility
  - **Implementation**: Ensure non-zero weights at positions 0, 8, 16, 24 for all archetypes except Chaos

### Weight Changes: Techno Minimal [0,0]

```cpp
// BEFORE (src/Engine/ArchetypeData.h:77-84):
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f,
    0.95f,0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f, ...
};

// AFTER (preserves blend gradients):
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f,  0.95f, 0.0f, 0.0f, 0.0f,  // Beats 1, 2
    0.95f,0.0f, 0.0f, 0.0f,  0.90f, 0.0f, 0.0f, 0.0f,  // Beats 3, 4
    1.0f, 0.0f, 0.0f, 0.0f,  0.95f, 0.0f, 0.0f, 0.0f,  // Bar 2
    0.95f,0.0f, 0.0f, 0.0f,  0.90f, 0.0f, 0.0f, 0.0f
};
// Gradients 0.90-1.0 preserve softmax blending while ensuring 4-on-floor
```

### Weight Changes: Techno Groovy [1,1]

```cpp
// BEFORE (src/Engine/ArchetypeData.h:190-196):
constexpr float kGroovy_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.45f, 0.85f, 0.0f, 0.0f, 0.4f, ...
};

// AFTER (viable ghosts at 0.50-0.60):
constexpr float kGroovy_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.55f,  0.95f, 0.0f, 0.0f, 0.50f,  // Beat 1 strong, "a" ghost
    0.95f,0.0f, 0.0f, 0.55f,  0.90f, 0.0f, 0.0f, 0.50f,  // Beats 2-3
    1.0f, 0.0f, 0.0f, 0.55f,  0.95f, 0.0f, 0.0f, 0.50f,  // Bar 2
    0.95f,0.0f, 0.0f, 0.60f,  0.90f, 0.0f, 0.0f, 0.55f   // End pickup
};
// Ghost weights 0.50-0.60 can win Gumbel selection
```

---

## Phase B: Velocity Contrast Widening

### Problem
Ghost notes at 50-70% don't contrast enough with accents at 85-100%.

### Subtasks

| ID | Task | File:Line | Status |
|----|------|-----------|--------|
| B1 | Widen `velocityFloor` range (65% → 30%) | `src/Engine/VelocityCompute.cpp:30` | ✅ |
| B2 | Widen `accentBoost` range (+15% → +45%) | `src/Engine/VelocityCompute.cpp:31` | ✅ |
| B3 | Reduce `velocityVariation` range (±3% → ±15%) | `src/Engine/VelocityCompute.cpp:32` | ✅ |
| B4 | Hardware test: prototype minimum floor at 0.30 | Hardware validation | ⏭️ (deferred) |
| B5 | Raise minimum clamp to 0.30 (or 0.32 per hw test) | `src/Engine/VelocityCompute.cpp:136` | ✅ |
| B6 | Update `tests/test_timing.cpp` expectations | `tests/test_timing.cpp` | ✅ |

### Feedback Integration

- [x] **Clamp affects both locations**: `ComputeVelocity()` at line 136 and coordinate with `GetVelocityWithVariation()`
  - **Implementation**: Check if BrokenEffects has separate clamp; if so, update both
- [x] **Hardware check first**: Prototype 0.30-0.32 before locking 0.35
  - **Implementation**: Subtask B4 gates B5
- ~~**Conditional floor during fills**~~: Rejected — adds complexity; simpler to have consistent floor
  - **Rationale**: Single floor value easier to reason about; fills get velocity boost, not floor change

### Code Changes

```cpp
// src/Engine/VelocityCompute.cpp:25-33
void ComputePunch(float punch, PunchParams& params)
{
    punch = Clamp(punch, 0.0f, 1.0f);

    // UPDATED ranges:
    params.accentProbability = 0.20f + punch * 0.30f;  // 20% to 50% (was 15%-50%)
    params.velocityFloor = 0.65f - punch * 0.35f;      // 65% to 30% (was 70%-30%)
    params.accentBoost = 0.15f + punch * 0.30f;        // +15% to +45% (was +10%-35%)
    params.velocityVariation = 0.03f + punch * 0.12f;  // ±3% to ±15% (was ±5%-20%)
}

// src/Engine/VelocityCompute.cpp:136
return Clamp(velocity, 0.30f, 1.0f);  // was 0.2f
```

---

## Phase C: Hit Budget Expansion

### Problem
BUILD density increase too subtle; balance range too narrow.

### Subtasks

| ID | Task | File:Line | Status |
|----|------|-----------|--------|
| C1 | Profile current hit counts across ENERGY zones (16/24/32/64 steps) | `tests/test_generation.cpp` (new section) | ✅ |
| C2 | Raise `maxHits` from `/4` to `/3` | `src/Engine/HitBudget.cpp:41` | ✅ |
| C3 | Raise `ComputeBarBudget()` clamp from `/2` to `*2/3` | `src/Engine/HitBudget.cpp:191` | ✅ |
| C4 | Increase zone minimums (GROOVE=3, BUILD=4, PEAK=6) | `src/Engine/HitBudget.cpp:48-66` | ✅ |
| C5 | Change shimmerRatio to `balance * 1.5f` (0-150%) | `src/Engine/HitBudget.cpp:114` | ✅ |
| C6 | Add zone-aware shimmer cap (GROOVE ≤1.0, PEAK ≤1.5) | `src/Engine/HitBudget.cpp:114-120` | ✅ |
| C7 | Add regression tests for 16/24/32/64 + balance extremes | `tests/test_generation.cpp` | ✅ |

### Feedback Integration

- [x] **Profile before changing**: Measure actual hit counts before modifying formulas
  - **Implementation**: Subtask C1 creates baseline measurements
- [x] **Raise caps too**: `/3` expansion needs cap changes to avoid clipping
  - **Implementation**: Subtasks C2 and C3 raise both caps
- [x] **Zone-aware shimmer ratio**: GROOVE cap near 1.0, PEAK allows >1.0
  - **Implementation**: Subtask C6 adds conditional capping
- [x] **Regression tests for 64-step**: Avoid reintroducing blank-second-half bug
  - **Implementation**: Subtask C7 covers all lengths

### Code Changes

```cpp
// src/Engine/HitBudget.cpp:41
int maxHits = patternLength / 3;  // Was /4

// src/Engine/HitBudget.cpp:48-66 (zone minimums)
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

// src/Engine/HitBudget.cpp:114 (with zone-aware cap)
float shimmerRatio = balance * 1.5f;  // 0% to 150%
// Zone-aware cap
if (zone == EnergyZone::GROOVE || zone == EnergyZone::MINIMAL) {
    shimmerRatio = std::min(shimmerRatio, 1.0f);
}

// src/Engine/HitBudget.cpp:191
int maxHits = clampedLength * 2 / 3;  // Was /2
```

---

## Phase D: BUILD Phase Enhancement

### Problem
BUILD only affects velocity slightly; user wants audible phrase-level arc.

### Subtasks

| ID | Task | File:Line | Status |
|----|------|-----------|--------|
| D1 | Add `BuildPhase` enum (GROOVE/BUILD/FILL) | `src/Engine/ControlState.h` | ✅ |
| D2 | Add `velocityBoost` and `forceAccents` to `BuildModifiers` | `src/Engine/ControlState.h:70-93` | ✅ |
| D3 | Update `Init()` for new fields | `src/Engine/ControlState.h:87-93` | ✅ |
| D4 | Implement 3-phase BUILD logic | `src/Engine/VelocityCompute.cpp:51-79` | ✅ |
| D5 | Integrate velocityBoost into `ComputeVelocity()` | `src/Engine/VelocityCompute.cpp:103-137` | ✅ |
| D6 | Integrate forceAccents into `ShouldAccent()` | `src/Engine/VelocityCompute.cpp:85-101` | ✅ |
| D7 | Update all callers of BuildModifiers | Search for `BuildModifiers` usage | ✅ |
| D8 | Add tests for phase transitions | `tests/test_timing.cpp` | ✅ |

### Feedback Integration

- [x] **Simpler 3-stage model**: groove → build → fill (not 4-stage)
  - **Implementation**: Removed TENSION phase; now GROOVE (0-60%), BUILD (60-87.5%), FILL (87.5-100%)
- [x] **Migrate callers**: BuildModifiers struct changes require updating Sequencer and tests
  - **Implementation**: Subtask D7 covers caller migration
- [x] **Tie to phrase length**: Phase boundaries computed from `phraseProgress`, which respects configured phrase length
  - **Implementation**: No additional work needed; phraseProgress already normalized

### New Enum and Struct Updates

```cpp
// src/Engine/VelocityCompute.h (new, after line 20)
enum class BuildPhase {
    GROOVE,   // 0-60%: stable
    BUILD,    // 60-87.5%: ramping
    FILL      // 87.5-100%: climax
};

// src/Engine/ControlState.h:70-93 (updated struct)
struct BuildModifiers
{
    float densityMultiplier;
    float fillIntensity;
    bool inFillZone;
    float phraseProgress;
    // NEW FIELDS:
    BuildPhase phase;         // Current build phase
    float velocityBoost;      // +0.0 to +0.15 floor boost
    bool forceAccents;        // True = all hits accented

    void Init()
    {
        densityMultiplier = 1.0f;
        fillIntensity     = 0.0f;
        inFillZone        = false;
        phraseProgress    = 0.0f;
        phase             = BuildPhase::GROOVE;
        velocityBoost     = 0.0f;
        forceAccents      = false;
    }
};
```

### 3-Phase BUILD Logic

```cpp
// src/Engine/VelocityCompute.cpp:51-79 (replacement)
void ComputeBuildModifiers(float build, float phraseProgress, BuildModifiers& modifiers)
{
    build = Clamp(build, 0.0f, 1.0f);
    phraseProgress = Clamp(phraseProgress, 0.0f, 1.0f);
    modifiers.phraseProgress = phraseProgress;

    if (phraseProgress < 0.60f) {
        // GROOVE phase: stable
        modifiers.phase = BuildPhase::GROOVE;
        modifiers.densityMultiplier = 1.0f;
        modifiers.velocityBoost = 0.0f;
        modifiers.forceAccents = false;
    }
    else if (phraseProgress < 0.875f) {
        // BUILD phase: ramping density and velocity
        modifiers.phase = BuildPhase::BUILD;
        float phaseProgress = (phraseProgress - 0.60f) / 0.275f;  // 0-1 within phase
        modifiers.densityMultiplier = 1.0f + build * 0.35f * phaseProgress;
        modifiers.velocityBoost = build * 0.08f * phaseProgress;
        modifiers.forceAccents = false;
    }
    else {
        // FILL phase: maximum energy
        modifiers.phase = BuildPhase::FILL;
        modifiers.densityMultiplier = 1.0f + build * 0.50f;
        modifiers.velocityBoost = build * 0.12f;
        modifiers.forceAccents = (build > 0.6f);
    }

    modifiers.inFillZone = (modifiers.phase == BuildPhase::FILL);
    modifiers.fillIntensity = modifiers.inFillZone ? build : 0.0f;
}
```

---

## Phase E: Swing Effectiveness

### Problem
Swing doesn't noticeably affect timing perception.

### Subtasks

| ID | Task | File:Line | Status |
|----|------|-----------|--------|
| E1 | Audit current swing path with logging | `src/Engine/BrokenEffects.cpp:261-280` | ✅ |
| E2 | Change swing to multiply archetype `swingAmount` | `src/Engine/BrokenEffects.cpp:261-280` | ✅ |
| E3 | Widen zone caps (0.60/0.65/0.68/0.70) | `src/Engine/BrokenEffects.cpp:270-280` | ✅ |
| E4 | Add external clock + swing regression test | `tests/test_timing.cpp` | ✅ |

### Feedback Integration

- [x] **Start from existing pipeline**: Use `ComputeSwing()` at `BrokenEffects.cpp:261`, don't create new offset model
  - **Implementation**: Modify existing function, don't add parallel path
- [x] **Multiply archetype swingAmount**: Use blended archetype swing from PatternField instead of separate formula
  - **Implementation**: `effectiveSwing = archetypeSwing * (1.0 + configSwing)` with zone capping
- [x] **External clock regression**: Ensure triplet swing doesn't violate rising-edge timing
  - **Implementation**: Subtask E4 adds test for Task 08 compatibility

### Code Changes

```cpp
// src/Engine/BrokenEffects.cpp:261-280 (updated)
float ComputeSwing(float configSwing, float archetypeSwing, EnergyZone zone)
{
    // Config swing multiplies archetype base (0×-2× multiplier)
    float effectiveSwing = archetypeSwing * (1.0f + configSwing);

    // Zone-aware caps (widened from 0.58/0.62/0.66)
    float maxSwing;
    switch (zone) {
        case EnergyZone::MINIMAL: maxSwing = 0.60f; break;
        case EnergyZone::GROOVE:  maxSwing = 0.65f; break;
        case EnergyZone::BUILD:   maxSwing = 0.68f; break;
        case EnergyZone::PEAK:    maxSwing = 0.70f; break;
        default:                  maxSwing = 0.65f; break;
    }

    return Clamp(effectiveSwing, 0.50f, maxSwing);
}
```

---

## Phase F: Genre-Aware Euclidean Blend

### Problem
Probabilistic generation can produce non-musical patterns at low complexity.

### Subtasks

| ID | Task | File:Line | Status |
|----|------|-----------|--------|
| F1 | Create `EuclideanGen.h` with `GenerateEuclidean()` | `src/Engine/EuclideanGen.h` (new) | ✅ |
| F2 | Create `EuclideanGen.cpp` implementation | `src/Engine/EuclideanGen.cpp` (new) | ✅ |
| F3 | Add `RotatePattern()` for seed-based rotation | `src/Engine/EuclideanGen.cpp` | ✅ |
| F4 | Add `BlendEuclideanWithWeights()` | `src/Engine/EuclideanGen.cpp` | ✅ |
| F5 | Add `GetGenreEuclideanRatio()` | `src/Engine/EuclideanGen.cpp` | ✅ |
| F6 | Integrate into `GenerateBar()` for Techno only | `src/Engine/Sequencer.cpp:290-410` | ✅ |
| F7 | Gate by ENERGY zone (only MINIMAL/GROOVE use Euclidean) | `src/Engine/Sequencer.cpp` | ✅ |
| F8 | Support 16/24/32/64 patterns (including two-half path) | `src/Engine/Sequencer.cpp` | ✅ |
| F9 | Add unit tests for Euclidean generation | `tests/test_generation.cpp` | ✅ |
| F10 | Add tests for Euclidean ratio tapering with Field X | `tests/test_generation.cpp` | ✅ |

### Feedback Integration

- [x] **Fallback, not foundation**: Use Euclidean as low-Field-X blend, not hard override
  - **Implementation**: Blend ratio reduces with Field X; at X>0.7, Euclidean ratio near 0
- [x] **Start techno-only**: Gate by genre to preserve IDM/Tribal character
  - **Implementation**: Subtask F6 only applies to Genre::TECHNO
- [x] **Rotation deterministic**: Rotation derived from seed, not random
  - **Implementation**: `int rotation = (seed % steps)` in `BlendEuclideanWithWeights()`
- [x] **Support all pattern lengths**: 16/24/32 direct, 64 via two-half path
  - **Implementation**: Subtask F8 handles all cases

### New Files

**`src/Engine/EuclideanGen.h`**:
```cpp
#pragma once
#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/// Generate Euclidean rhythm: k hits distributed evenly across n steps
uint32_t GenerateEuclidean(int hits, int steps);

/// Rotate pattern by offset steps (deterministic)
uint32_t RotatePattern(uint32_t pattern, int offset, int steps);

/// Blend Euclidean foundation with probabilistic weights
/// @param euclideanRatio 0.0 = pure probabilistic, 1.0 = pure Euclidean
uint32_t BlendEuclideanWithWeights(
    int budget,
    int steps,
    const float* weights,
    uint32_t eligibility,
    float euclideanRatio,
    uint32_t seed);

/// Get genre-specific Euclidean blend ratio
/// @param fieldX Syncopation axis (0-1), higher = less Euclidean
float GetGenreEuclideanRatio(Genre genre, float fieldX, EnergyZone zone);

} // namespace daisysp_idm_grids
```

---

## Spec Updates

### Section 4.3 (PUNCH Parameter)

**Add after current content**:
> **Velocity ranges** (v4.1):
> - velocityFloor: 65% down to 30% (widened from 70%-30%)
> - accentBoost: +15% to +45% (widened from +10%-35%)
> - Minimum output velocity: 30% (raised from 20% for VCA audibility)

### Section 4.4 (BUILD Parameter)

**Replace current content with**:
> BUILD controls the narrative arc of each phrase—how much tension builds toward the end.
>
> BUILD operates in three phases:
> 1. **GROOVE (0-60%)**: Stable pattern, no density/velocity modification
> 2. **BUILD (60-87.5%)**: Density +0-35%, velocity floor +0-8%
> 3. **FILL (87.5-100%)**: Density +35-50%, velocity floor +8-12%, all hits accented (at BUILD>60%)
>
> Phase boundaries are computed from phrase progress, which respects configured phrase length.

### Section 6.3 (Gumbel Top-K Selection)

**Add new subsection 6.3.1**:
> ### 6.3.1 Euclidean Foundation (Genre-Dependent)
>
> Before Gumbel Top-K selection, an optional Euclidean foundation guarantees even hit distribution:
>
> | Genre | Base Euclidean Ratio | Notes |
> |-------|---------------------|-------|
> | Techno | 70% at Field X=0 | Ensures four-on-floor at low complexity |
> | Tribal | 40% at Field X=0 | Balances structure with polyrhythm |
> | IDM | 0% (disabled) | Allows maximum irregularity |
>
> - Field X reduces Euclidean ratio by up to 70% (at X=1.0, ratio ≈ 0.3× base)
> - Only active in MINIMAL and GROOVE zones
> - Rotation derived from seed for determinism

---

## Implementation Order

### Sprint 1: Instrumentation + Low-Risk (Days 1-2)

| Subtask | Description | Blocking? |
|---------|-------------|-----------|
| A0 | Add hit histogram test | No |
| A1-A4 | Techno weight retuning | No |
| B1-B3 | Velocity range widening | No |
| B4 | Hardware velocity floor test | **Yes** (gates B5) |

### Sprint 2: Medium-Risk + Cross-Task (Days 3-4)

| Subtask | Description | Blocking? |
|---------|-------------|-----------|
| B5-B6 | Apply velocity floor (after hw test) | No |
| C1-C7 | Hit budget expansion | No |
| D1-D8 | BUILD phase enhancement | No |
| **Task 22 B** | Ship balance range together | **Coordinate** |

### Sprint 3: Architecture (Days 5-7)

| Subtask | Description | Blocking? |
|---------|-------------|-----------|
| A5-A7 | Tribal/IDM weight tuning + validation | No |
| E1-E4 | Swing effectiveness | No |
| F1-F10 | Euclidean blend (techno-only) | No |

### Hardware Validation Checkpoints

After each sprint:
- [ ] Generate patterns at all 9 field positions for Techno
- [ ] Verify distinct character audibly at corners ([0,0], [2,2])
- [ ] Sweep ENERGY 0-100%, verify zone transitions
- [ ] Sweep BUILD 0-100%, verify phrase arc
- [ ] Test swing at 0%, 50%, 100% with external clock

---

## Files Modified Summary

| File | Phase(s) | Changes |
|------|----------|---------|
| `src/Engine/ArchetypeData.h` | A | Weight table retuning (~200 lines) |
| `src/Engine/VelocityCompute.h` | D | Add BuildPhase enum |
| `src/Engine/VelocityCompute.cpp` | B, D | Velocity ranges, BUILD phases |
| `src/Engine/ControlState.h` | D | Extend BuildModifiers struct |
| `src/Engine/HitBudget.cpp` | C | Budget expansion, balance range |
| `src/Engine/BrokenEffects.cpp` | E | Swing effectiveness |
| `src/Engine/EuclideanGen.h` | F | New file |
| `src/Engine/EuclideanGen.cpp` | F | New file |
| `src/Engine/Sequencer.cpp` | E, F | Swing integration, Euclidean blend |
| `tests/test_generation.cpp` | A, C, F | Hit histograms, budget tests, Euclidean tests |
| `tests/test_timing.cpp` | B, D, E | Velocity, BUILD, swing tests |
| `docs/specs/main.md` | All | Spec updates per above |

---

## Success Criteria

### Musicality
- [ ] Techno [0,0] Minimal produces reliable 4-on-floor
- [ ] Techno [1,1] Groovy produces head-nodding groove with ghost notes
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
- [ ] External clock timing not violated by swing

---

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Weight changes break existing patterns | Subtask A0 baselines before changes; A/B test |
| Euclidean layer fights field morphing | Only active at low Field X; genre-gated |
| BUILD phases add hidden state | Simplified to 3 phases; all derived from phraseProgress |
| Velocity floor too high | Hardware test (B4) before locking value |
| Cross-task coordination failure | Explicit coordination with Task 22 Phase B |

---

*End of Implementation Plan*
