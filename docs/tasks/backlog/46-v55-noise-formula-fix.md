---
id: 46
slug: v55-noise-formula-fix
title: "V5.5 Noise Formula Fix: Correct Zone-Based Noise Scaling"
status: backlog
created_date: 2026-01-08
updated_date: 2026-01-08
branch: feature/v55-pattern-expressiveness
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
  - "docs/design/v5-5-iterative-patterns.md#54-change-d-fixed-noise-formula"
related:
  - 44  # V5 Anchor Seed Variation
  - 45  # Pattern Generator Extraction
---

# Task 46: V5.5 Noise Formula Fix

## Objective

Fix the noise scaling formula in pattern generation to ensure stable-zone patterns (SHAPE < 0.3) remain stable, while allowing appropriate noise injection in syncopated and wild zones.

## Context

### Problem

The current noise formula in `PatternGenerator.cpp` uses:
```cpp
const float noiseScale = 0.4f * (1.0f - params.shape);
```

This produces:
- SHAPE = 0.0: noiseScale = 0.40 (TOO HIGH for stable zone)
- SHAPE = 0.5: noiseScale = 0.20
- SHAPE = 1.0: noiseScale = 0.00 (TOO LOW for wild zone)

The formula is inverted - it adds MORE noise when SHAPE is low (stable) and LESS noise when SHAPE is high (wild).

### Design Solution

From `docs/design/v5-5-iterative-patterns/iteration-2.md`:

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

### Minor Fix: Zone Boundary Alignment

From critique-2.md: The noise formula uses 0.28/0.68 but the spec uses 0.30/0.70 for zone boundaries. Use spec boundaries for consistency:
- Stable zone: SHAPE < 0.30
- Syncopated zone: 0.30 <= SHAPE < 0.70
- Wild zone: SHAPE >= 0.70

## Subtasks

- [ ] Create `ComputeNoiseScale()` helper function in `PatternGenerator.cpp`
- [ ] Update anchor weight perturbation to use new noise formula
- [ ] Align zone boundaries with spec (0.30/0.70 instead of 0.28/0.68)
- [ ] Add unit tests for noise scaling at zone boundaries
- [ ] Verify stable zone produces consistent patterns
- [ ] All tests pass

## Acceptance Criteria

- [ ] `ComputeNoiseScale(0.0f)` returns approximately 0.0
- [ ] `ComputeNoiseScale(0.15f)` returns approximately 0.05
- [ ] `ComputeNoiseScale(0.30f)` returns approximately 0.10
- [ ] `ComputeNoiseScale(0.50f)` returns approximately 0.175
- [ ] `ComputeNoiseScale(0.70f)` returns approximately 0.25
- [ ] `ComputeNoiseScale(1.0f)` returns approximately 0.40
- [ ] Pattern viz shows more stable patterns at low SHAPE
- [ ] All existing tests pass
- [ ] No new compiler warnings

## Implementation Notes

### Files to Modify

- `src/Engine/PatternGenerator.cpp` - Add `ComputeNoiseScale()` and update usage
- `src/Engine/PatternGenerator.h` - (optional) Expose helper for testing
- `tests/test_pattern_generator.cpp` - Add noise scaling tests

### Current Code Location

In `PatternGenerator.cpp`, line ~84-91:
```cpp
// V5 Task 44: Add seed-based weight perturbation for anchor variation
{
    const float noiseScale = 0.4f * (1.0f - params.shape);
    for (int step = 0; step < params.patternLength; ++step) {
        if (step == 0 && params.shape < 0.3f) continue;
        float noise = (HashToFloat(params.seed, step + 1000) - 0.5f) * noiseScale;
        anchorWeights[step] = ClampWeight(anchorWeights[step] + noise);
    }
}
```

### New Implementation

```cpp
// Helper function
float ComputeNoiseScale(float shape) {
    if (shape < 0.30f) {
        // Stable zone: minimal noise (0 to 0.10)
        return shape / 0.30f * 0.10f;
    } else if (shape < 0.70f) {
        // Syncopated zone: moderate noise (0.10 to 0.25)
        return 0.10f + (shape - 0.30f) / 0.40f * 0.15f;
    } else {
        // Wild zone: higher noise (0.25 to 0.40)
        return 0.25f + (shape - 0.70f) / 0.30f * 0.15f;
    }
}

// Updated usage
{
    const float noiseScale = ComputeNoiseScale(params.shape);
    for (int step = 0; step < params.patternLength; ++step) {
        if (step == 0 && params.shape < 0.30f) continue;  // Also update to 0.30
        float noise = (HashToFloat(params.seed, step + 1000) - 0.5f) * noiseScale;
        anchorWeights[step] = ClampWeight(anchorWeights[step] + noise);
    }
}
```

## Test Plan

1. Build firmware: `make clean && make`
2. Run tests: `make test`
3. Generate patterns with viz tool:
   ```bash
   ./build/pattern_viz --shape=0.0 --seed=123 --format=grid  # Should be very stable
   ./build/pattern_viz --shape=0.3 --seed=123 --format=grid  # Slight variation
   ./build/pattern_viz --shape=0.7 --seed=123 --format=grid  # More variation
   ./build/pattern_viz --shape=1.0 --seed=123 --format=grid  # Maximum variation
   ```
4. Compare multiple seeds at SHAPE=0.0 - should produce near-identical patterns

## Estimated Effort

1 hour

## Relevancy Assessment (2026-01-19)

**Status**: HIGH RELEVANCE - Critical bug fix

**Current System Impact**:
- Noise formula is inverted: adds MORE noise at low SHAPE (stable) and LESS at high SHAPE (wild)
- This breaks the stable zone contract: SHAPE < 0.3 should be consistent/predictable
- Currently at SHAPE=0.0, noiseScale=0.40 (too high), at SHAPE=1.0, noiseScale=0.00 (too low)

**Why This Matters**:
- Bug is actively harming pattern stability metrics
- Should be fixed before other improvements (tasks 47-48 depend on this)
- Low implementation risk (single formula change)

## Improvement Estimates

**Pattern Stability** (stable zone SHAPE < 0.3):
- Current: Patterns show unwanted variation at low SHAPE
- Expected: +15-20% improvement in pattern consistency

**Variation Quality** (wild zone SHAPE > 0.7):
- Current: Wild patterns too similar (insufficient noise)
- Expected: +10-15% improvement in wild zone variation

**Overall Metric Impact**:
- Anchor pattern uniqueness: +10-15%
- Stable zone predictability: +20%
- Wild zone expressiveness: +10%

**Confidence**: 90% - This is a known bug with clear fix
