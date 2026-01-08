---
id: 44
slug: v5-anchor-seed-variation
title: "V5 Anchor Seed Variation: Fix Zero Pattern Diversity"
status: pending
created_date: 2026-01-07
updated_date: 2026-01-07
branch: feature/44-v5-anchor-seed-variation
spec_refs: ["1.1", "4.3"]
---

# Task 44: V5 Anchor Seed Variation

## Context

Task 43 successfully improved V2 (Shimmer) seed variation from 25% to 94%. However, V1 (Anchor) remains at **0% variation** - different seeds produce identical anchor patterns.

**Current Expressiveness Metrics:**
```
Overall Seed Variation: 31%
  V1 (Anchor):  0%   <-- CRITICAL: No variation
  V2 (Shimmer): 94%  <-- Fixed in Task 43
  AUX:          0%

[FAIL] Expressiveness issues detected:
       - V1 (Anchor) variation is too low
```

This violates the core design principle from spec 1.1:
> "Deterministic variation: Same settings + seed = identical output"

The inverse must also hold: **Different seed = different output**.

## Root Cause Analysis

The anchor pattern is generated via two paths in `Sequencer.cpp:337-362`:

1. **Euclidean Path** (`euclideanRatio > 0.01`): `BlendEuclideanWithWeights()`
   - Uses seed for rotation: `rotation = seed % steps` (line 107 in EuclideanGen.cpp)
   - But rotation alone gives limited variation (only 16 positions for 16 steps)
   - Euclidean pattern itself is deterministic (no seed influence on distribution)

2. **Gumbel Path** (`euclideanRatio <= 0.01`): `SelectHitsGumbelTopK()`
   - Uses seed in `ComputeGumbelScores()` via `HashToFloat(seed, step)`
   - BUT: The weights passed are from `ComputeShapeBlendedWeights()` + `ApplyAxisBias()`
   
**The Bug:** Looking at `ComputeShapeBlendedWeights()` (PatternField.cpp:152-254):
- In Zone 1 (Stable), seed only adds tiny humanization jitter (0.05 * ...)
- The base weights come from `GenerateStablePattern()` which is **seed-independent**
- At low SHAPE values, weights are nearly identical across seeds
- When weights are identical, Gumbel sampling converges to same positions

**Why Shimmer Varies But Anchor Doesn't:**
- Task 43 added seed-based rotation to shimmer mask (`Sequencer.cpp` post-processing)
- Anchor has no such rotation applied
- Anchor weights are more deterministic (stable pattern dominates at low SHAPE)

## Design Approaches

### Approach A: Seed-Based Weight Perturbation (Recommended)

Add seed-based noise to anchor weights before Gumbel selection:

```cpp
// After ComputeShapeBlendedWeights + ApplyAxisBias for anchor
for (int step = 0; step < halfLength; ++step) {
    // Add 10-20% seed-based perturbation
    float perturbation = (HashToFloat(seed, step + 1000) - 0.5f) * 0.3f;
    anchorWeights[step] = ClampWeight(anchorWeights[step] * (1.0f + perturbation));
}
```

**Pros:**
- Directly addresses weight convergence issue
- Preserves SHAPE-based character while adding variation
- Minimal code change

**Cons:**
- May slightly weaken metrical hierarchy at very low SHAPE

### Approach B: Seed-Based Rotation (Like Shimmer)

Apply seed-based rotation to anchor mask after selection:

```cpp
// After anchorMask1 is computed
int anchorRotation = HashToInt(seed, 2000) % halfLength;
anchorMask1 = RotateBitMask(anchorMask1, anchorRotation, halfLength);
```

**Pros:**
- Simple, proven effective for shimmer
- Doesn't alter weight distribution

**Cons:**
- May shift kick off beat 1 (problematic for Techno)
- Rotation preserves pattern shape, only shifts position

### Approach C: Euclidean Phase Offset

For Euclidean-dominated patterns, vary the phase within each spacing interval:

```cpp
// In BlendEuclideanWithWeights, add phase offset to Euclidean hits
float phaseOffset = HashToFloat(seed, 500) * 0.4f; // 0-40% of spacing
```

**Pros:**
- Affects Euclidean path specifically
- Maintains even distribution character

**Cons:**
- Only affects Euclidean path, not Gumbel
- Limited variation range

### Recommended: Approach A + C Combined

1. Add weight perturbation for all paths (A)
2. Add phase offset for Euclidean path (C)
3. **Protect beat 1**: Never perturb step 0 weight, or apply rotation mod that preserves beat 1

## Subtasks

### Phase 1: Research & Baseline

- [ ] **44.1** Add anchor pattern logging to expressiveness evaluation
  - Log anchor weights and final mask for debugging
  - Confirm that anchor weights are identical across seeds at low SHAPE

### Phase 2: Weight Perturbation

- [ ] **44.2** Add seed-based weight perturbation to anchor weights in `Sequencer.cpp`
  - Apply after `ApplyAxisBias()` completes
  - Use 10-20% perturbation scaled by (1 - SHAPE) so variation decreases as SHAPE increases
  - Protect step 0 (beat 1) from perturbation at SHAPE < 0.3 (Techno range)
  - Run `make test` to verify no regressions

- [ ] **44.3** Run expressiveness evaluation and verify V1 variation improved
  - Target: V1 variation >= 25%
  - Run `make expressiveness-quick`

### Phase 3: Euclidean Phase Variation (if needed)

- [ ] **44.4** If V1 still < 25%, add phase offset to Euclidean generation
  - In `BlendEuclideanWithWeights()`, compute seed-based phase offset
  - Apply to Euclidean hit positions before combining with Gumbel
  - Verify Euclidean path now varies with seed

### Phase 4: Verification

- [ ] **44.5** Run full expressiveness evaluation
  - All three voices should show variation
  - No critical convergence patterns
  - Run `make test` for regressions

- [ ] **44.6** Update `docs/design/expressiveness-report.md` with new metrics

## Acceptance Criteria

- [ ] V1 (Anchor) variation score >= 25%
- [ ] V2 (Shimmer) variation score remains >= 50%
- [ ] No test regressions (`make test` passes)
- [ ] No new compiler warnings
- [ ] Beat 1 (step 0) remains stable in Techno zone (SHAPE < 0.3)
- [ ] Code review passes

## Files to Modify

| File | Changes |
|------|---------|
| `src/Engine/Sequencer.cpp` | Add weight perturbation after ApplyAxisBias |
| `src/Engine/EuclideanGen.cpp` | Optional: Add phase offset to BlendEuclideanWithWeights |
| `docs/design/expressiveness-report.md` | Update with new metrics |

## Implementation Notes

### Weight Perturbation Location

In `Sequencer.cpp`, after line 307 (ApplyAxisBias call):

```cpp
// V5 Task 44: Add seed-based weight perturbation for anchor variation
// Scaled by (1 - shape) so variation is strongest at low SHAPE
float perturbScale = 0.2f * (1.0f - state_.controls.shape);
for (int step = 0; step < halfLength; ++step) {
    // Protect beat 1 at low SHAPE (Techno kick stability)
    if (step == 0 && state_.controls.shape < 0.3f) continue;
    
    float perturb = (HashToFloat(seed, step + 1000) - 0.5f) * 2.0f * perturbScale;
    anchorWeights[step] = ClampWeight(anchorWeights[step] * (1.0f + perturb));
}
```

### HashToFloat Signature

Already available in `GumbelSampler.h`:
```cpp
float HashToFloat(uint32_t seed, int step);
```

### ClampWeight Location

Already available in `PatternField.h`:
```cpp
inline float ClampWeight(float w) { return std::max(0.05f, std::min(1.0f, w)); }
```

## Reference Documents

- `docs/tasks/completed/43-v5-shimmer-seed-variation.md` - What worked for shimmer
- `docs/design/expressiveness-report.md` - Current metrics
- `docs/design/v5-iteration/` - V5 design decisions
- `src/Engine/PatternField.cpp` - Weight generation code

## Spec References

- Section 1.1: "Deterministic variation: Same settings + seed = identical output"
- Section 4.3: DRIFT parameter behavior (seed selection)
