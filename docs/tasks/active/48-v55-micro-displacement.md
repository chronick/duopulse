---
id: 48
slug: v55-micro-displacement
title: "V5.5 Micro-Displacement: Per-Hit Position Variation"
status: pending
created_date: 2026-01-08
updated_date: 2026-01-08
branch: feature/v55-pattern-expressiveness
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
  - "docs/design/v5-5-iterative-patterns.md#52-change-b-micro-displacement"
related:
  - 44  # V5 Anchor Seed Variation
  - 46  # V5.5 Noise Formula Fix
  - 47  # V5.5 Velocity Variation
depends_on:
  - 46  # Noise fix should be done first
---

# Task 48: V5.5 Micro-Displacement

## Objective

Replace full pattern rotation with per-hit micro-displacement. Individual hits can move +/-1 step based on seed, preserving overall pattern shape while adding subtle variation.

## Context

### Problem

Current rotation implementation shifts the entire pattern:
```cpp
if (params.shape < 0.7f) {
    int maxRotation = std::max(1, params.patternLength / 4);
    int rotation = static_cast<int>(HashToFloat(params.seed, 2000) * maxRotation);
    result.anchorMask = RotateWithPreserve(result.anchorMask, rotation, ...);
}
```

This can move ALL hits, which at low SHAPE values violates the "stable = downbeat-focused" contract.

### Design Solution

From `docs/design/v5-5-iterative-patterns/iteration-2.md` Proposal B:

1. **Zone restriction**: Only apply in syncopated zone (0.3 <= SHAPE < 0.7)
2. **Per-hit displacement**: Each hit has independent chance to move +/-1
3. **Beat 1 protection**: Never displace step 0
4. **Collision avoidance**: Don't create overlapping hits

### Key Design Decisions

- **Stable zone (SHAPE < 0.3)**: No displacement - patterns are deterministic
- **Syncopated zone (0.3-0.7)**: Up to 25% of hits displaced at SHAPE=0.7
- **Wild zone (SHAPE >= 0.7)**: No displacement (chaos handled differently)

## Subtasks

- [ ] Create `ApplyMicroDisplacement()` function in `PatternGenerator.cpp`
- [ ] Remove or replace existing rotation code for anchor
- [ ] Add zone guards (only syncopated zone)
- [ ] Protect beat 1 from displacement
- [ ] Add collision detection
- [ ] Add unit tests for displacement logic
- [ ] Add test for beat 1 protection
- [ ] Verify pattern viz shows displacement effects
- [ ] All tests pass

## Acceptance Criteria

- [ ] SHAPE < 0.3: Zero hits displaced
- [ ] SHAPE = 0.5: Some hits displaced (+/-1 step)
- [ ] SHAPE = 0.7: Up to 25% of hits displaced
- [ ] Beat 1 (step 0) is NEVER displaced
- [ ] No two hits end up on the same step (collision prevention)
- [ ] Displaced hits don't wrap to step 0
- [ ] All existing tests pass
- [ ] No new compiler warnings

## Implementation Notes

### Files to Modify

- `src/Engine/PatternGenerator.cpp` - Add `ApplyMicroDisplacement()`, update `GeneratePattern()`
- `src/Engine/PatternGenerator.h` - (optional) Expose for testing
- `tests/test_pattern_generator.cpp` - Add displacement tests

### Current Rotation Code (to replace)

In `PatternGenerator.cpp` around line 158-164:
```cpp
// V5 Task 44: Apply seed-based rotation for anchor variation (AFTER guard rails)
if (params.shape < 0.7f) {
    int maxRotation = std::max(1, params.patternLength / 4);
    int rotation = static_cast<int>(HashToFloat(params.seed, 2000) * maxRotation);
    result.anchorMask = RotateWithPreserve(result.anchorMask, rotation,
                                            params.patternLength, 0);
}
```

### New Implementation

```cpp
void ApplyMicroDisplacement(uint32_t& hitMask, float shape, uint32_t seed, int patternLength)
{
    // Only apply in syncopated zone
    if (shape < 0.30f || shape >= 0.70f) return;
    
    // Displacement probability scales with shape
    // 0% at shape=0.3, 25% at shape=0.7
    float displacementProb = (shape - 0.30f) / 0.40f * 0.25f;
    
    uint32_t newMask = 0;
    
    for (int step = 0; step < patternLength; ++step) {
        if (!(hitMask & (1U << step))) continue;
        
        // Never displace beat 1
        if (step == 0) {
            newMask |= (1U << step);
            continue;
        }
        
        // Check if this hit should be displaced
        float roll = HashToFloat(seed, step + 600);
        if (roll < displacementProb) {
            // Determine direction: -1, 0, or +1
            float dirHash = HashToFloat(seed, step + 601);
            int delta = (dirHash < 0.33f) ? -1 : (dirHash > 0.66f) ? 1 : 0;
            
            int newStep = (step + delta + patternLength) % patternLength;
            
            // Collision prevention: don't move onto existing hit or beat 1
            if (newStep != 0 && !(newMask & (1U << newStep)) && !(hitMask & (1U << newStep))) {
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

// In GeneratePattern(), REPLACE the rotation code with:
// Apply micro-displacement (syncopated zone only)
ApplyMicroDisplacement(result.anchorMask, params.shape, params.seed, params.patternLength);
```

### Relationship to Rotation

The design critique suggests clarifying that micro-displacement REPLACES rotation, not supplements it. The `RotateWithPreserve()` function can remain for other uses but should not be called for anchor patterns in the main generation path.

## Test Plan

1. Build firmware: `make clean && make`
2. Run tests: `make test`
3. Generate patterns with viz tool:
   ```bash
   # Stable zone - no displacement
   ./build/pattern_viz --shape=0.1 --seed=123 --format=mask
   ./build/pattern_viz --shape=0.1 --seed=456 --format=mask
   # Same patterns expected
   
   # Syncopated zone - displacement visible
   ./build/pattern_viz --shape=0.5 --seed=123 --format=grid
   ./build/pattern_viz --shape=0.5 --seed=456 --format=grid
   # Different patterns, but similar structure
   
   # Verify beat 1 always present
   ./build/pattern_viz --shape=0.5 --sweep=seed:1:100 --format=summary
   # All patterns should have beat 1
   ```

## Estimated Effort

3-4 hours
