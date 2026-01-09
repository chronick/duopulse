---
id: 50
slug: v55-two-pass-generation
title: "V5.5 Two-Pass Generation: Skeleton + Embellishment (Conditional)"
status: pending
created_date: 2026-01-08
updated_date: 2026-01-08
branch: feature/v55-pattern-expressiveness
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
  - "docs/design/v5-5-iterative-patterns.md#55-change-e-beat-1-enforcement"
  - "docs/design/v5-5-iterative-patterns/iteration-2.md#proposal-c-two-pass-pattern-generation"
related:
  - 46  # V5.5 Noise Formula Fix
  - 47  # V5.5 Velocity Variation
  - 48  # V5.5 Micro-Displacement
  - 49  # V5.5 AUX Style and Beat 1
depends_on:
  - 46
  - 47
  - 48
  - 49
---

# Task 50: V5.5 Two-Pass Generation (Conditional)

## Objective

Implement two-pass pattern generation if targets are not met after Tasks 46-49. The skeleton pass places guaranteed downbeats, the embellishment pass adds variation.

## Context

### Conditional Implementation

From the design critique (critique-2.md):
> "Skip Proposal C (Two-Pass) unless still below target after B"

This task should only be implemented if:
1. Tasks 46-49 are complete
2. Pattern variation still below 50% target
3. Groove quality metrics still below 60% target

### Design Concept

Two-pass generation guarantees groove foundation:

1. **Skeleton Pass**: Place beats 1 and 3 unconditionally
2. **Embellishment Pass**: Fill remaining hits based on SHAPE/seed

This ensures musical coherence while allowing variation.

### Minor Fix: Beat 3 Parameterization

From critique-2.md: Beat 3 is hardcoded as step 8 for 16-step patterns. Should use `patternLength / 2` for variable pattern lengths.

## Prerequisites Check

Before implementing this task, verify:
```bash
# Run pattern analysis
./build/pattern_viz --sweep=all --format=summary

# Check targets:
# - V1 unique patterns: target >= 50%
# - V2 unique patterns: target >= 55%
# - AUX unique patterns: target >= 50%
# - Overall variation: target >= 50%
```

If targets are MET, this task can be marked as DROPPED (not needed).

## Subtasks

- [ ] Verify Tasks 46-49 complete
- [ ] Run pattern analysis to check if targets are met
- [ ] **Decision Point**: If targets met, mark task as DROPPED
- [ ] If targets NOT met, continue:
- [ ] Create `GeneratePatternTwoPass()` function
- [ ] Implement skeleton pass (beats 1 and 3)
- [ ] Implement embellishment pass with Gumbel selection
- [ ] Integrate with existing `GeneratePattern()` flow
- [ ] Add unit tests for two-pass logic
- [ ] Verify groove quality improvement
- [ ] All tests pass

## Acceptance Criteria

**If implemented:**
- [ ] Skeleton hits (beats 1 and 3) always present in anchor
- [ ] Embellishment hits vary with SHAPE and seed
- [ ] Micro-displacement applies to embellishments only (not skeleton)
- [ ] Pattern variation target (50%) achieved
- [ ] Groove quality target (60%) achieved
- [ ] All existing tests pass
- [ ] No new compiler warnings
- [ ] CPU overhead < 5%

**If dropped:**
- [ ] Documented reason (targets already met)
- [ ] Metrics showing current state meets goals

## Implementation Notes

### Files to Modify

- `src/Engine/PatternGenerator.cpp` - Add two-pass logic
- `src/Engine/PatternGenerator.h` - (optional) New declarations
- `tests/test_pattern_generator.cpp` - Add tests

### Two-Pass Implementation

```cpp
void GeneratePatternTwoPass(const PatternParams& params, PatternResult& result)
{
    result.anchorMask = 0;
    
    // PASS 1: Skeleton (guaranteed groove foundation)
    // Always include beats 1 and 3 for anchor
    result.anchorMask |= (1U << 0);                         // Beat 1
    result.anchorMask |= (1U << (params.patternLength / 2)); // Beat 3 (parameterized!)
    
    int skeletonHits = 2;
    int targetHits = ComputeTargetHits(params.energy, params.patternLength, 
                                        Voice::ANCHOR, params.shape);
    
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
        embellishWeights,
        eligibility,
        remainingHits,
        params.seed,
        params.patternLength,
        GetMinSpacingForZone(GetEnergyZone(params.energy))
    );
    
    result.anchorMask |= embellishMask;
    
    // Apply micro-displacement to embellishments only
    if (params.shape > 0.30f && params.shape < 0.70f) {
        uint32_t skeletonMask = (1U << 0) | (1U << (params.patternLength / 2));
        embellishMask = result.anchorMask & ~skeletonMask;
        ApplyMicroDisplacement(embellishMask, params.shape, params.seed, params.patternLength);
        result.anchorMask = skeletonMask | embellishMask;
    }
}
```

### Integration Strategy

Option A: Replace current generation entirely
Option B: Use two-pass only when needed (e.g., low SHAPE)

Recommended: Option A for consistency, but make it a compile-time flag during testing.

## Test Plan

1. Run prerequisites check (see above)
2. If implementing:
   - Build firmware: `make clean && make`
   - Run tests: `make test`
   - Verify skeleton hits:
     ```bash
     ./build/pattern_viz --shape=0.3 --sweep=seed:1:100 --format=summary
     # All patterns should have beats 1 AND 3
     ```
   - Compare before/after metrics

## Estimated Effort

4-5 hours (if implemented)
0 hours (if dropped due to targets already met)
