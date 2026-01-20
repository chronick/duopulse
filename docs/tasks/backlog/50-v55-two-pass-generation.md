---
id: 50
slug: v55-two-pass-generation
title: "V5.5 Two-Pass Generation: Skeleton + Embellishment (Conditional)"
status: backlog
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

## Relevancy Assessment (2026-01-19)

**Status**: CONDITIONAL - Implement only if needed

**Current System Impact**:
- Single-pass generation can miss groove fundamentals
- No guaranteed skeleton hits (except beat 1 enforcement from task 49)
- Embellishments and foundation not separated

**Why This Is Conditional**:
- Design critique recommends: "Skip Proposal C unless still below target after B"
- Tasks 46-49 may be sufficient to meet variation targets
- Two-pass adds complexity and CPU overhead
- Should measure first, implement only if needed

**Decision Criteria**:
After tasks 46-49 complete, check metrics:
- Pattern variation >= 50%? ✓ Drop this task
- Groove quality >= 60%? ✓ Drop this task
- Either metric below target? → Implement

## Improvement Estimates (IF IMPLEMENTED)

**Pattern Stability** (skeleton guarantee):
- Current: Variable groove foundation
- Expected: +15-20% improvement in groove quality

**Groove Quality** (musical coherence):
- Current: Patterns can lack structure
- Expected: +10-15% improvement with guaranteed downbeats

**Variation Quality** (embellishment diversity):
- Current: Whole pattern varies
- Expected: +5-10% improvement (skeleton stays, fills vary)

**Overall Metric Impact** (IF NEEDED):
- Groove quality: +10-15%
- Pattern stability: +15-20%
- Variation: +5-10%

**Confidence**: 70% IF implemented - Adds structure but may reduce variation

**Priority**: LOW - Measure first, implement last

---

## 64-Step Grid Compatibility (Added 2026-01-19)

**Impact**: Major - Skeleton concept needs bar awareness

### Current Implementation Assumption

The task assumes "beats 1 and 3" means steps 0 and `patternLength / 2`. At 32 steps:
- Step 0 = beat 1
- Step 16 = beat 3

This works for 2-bar patterns at 16th-note resolution.

### 64-Step Implications

At 64 steps, the pattern can represent:
- **4 bars of 16ths**: Steps 0, 16, 32, 48 are downbeats (beats 1 of each bar)
- **2 bars of 32nds**: Steps 0, 32 are beats 1 and 3
- **1 bar of 64ths**: Only step 0 is beat 1

### Required Updates

1. **Multi-bar skeleton**: For 64-step 4-bar patterns, skeleton should include all downbeats:
   ```cpp
   std::vector<int> skeletonSteps;
   int stepsPerBar = patternLength / 4;  // Assume 4 bars
   for (int bar = 0; bar < 4; bar++) {
       skeletonSteps.push_back(bar * stepsPerBar);  // Beat 1 of each bar
   }
   ```

2. **Add pattern structure parameter**: Consider adding `barsPerPattern` or `resolution` to `PatternParams`:
   ```cpp
   struct PatternParams {
       // ...
       int patternLength;      // 16, 32, or 64
       int barsPerPattern;     // Derived: patternLength / 16
       PatternResolution res;  // SIXTEENTHS, THIRTY_SECONDS, etc.
   };
   ```

3. **Update acceptance criteria**: Test skeleton at all pattern lengths:
   - 16 steps: beats 1, 3 (steps 0, 8)
   - 32 steps: beats 1, 3 (steps 0, 16)
   - 64 steps: beats 1, 5, 9, 13 (steps 0, 16, 32, 48) for 4-bar pattern

### Opportunities

With 64-step support, two-pass generation can create more complex phrase structures:
- Pass 1: Downbeats (4 hits in 64 steps)
- Pass 2: Fill based on ENERGY and SHAPE

This creates true multi-bar skeleton patterns instead of just 2-beat.
