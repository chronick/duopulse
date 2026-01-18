---
id: 47
slug: v55-velocity-variation
title: "V5.5 Velocity Variation: Ghost Notes and Accent Dynamics"
status: backlog
created_date: 2026-01-08
updated_date: 2026-01-08
branch: feature/v55-pattern-expressiveness
spec_refs:
  - "docs/specs/main.md#10-velocity-dynamics"
  - "docs/design/v5-5-iterative-patterns.md#51-change-a-enhanced-velocity-variation"
related:
  - 35  # V5 Accent Velocity
  - 46  # V5.5 Noise Formula Fix
depends_on:
  - 46  # Noise fix should be done first (bug fix)
---

# Task 47: V5.5 Velocity Variation

## Objective

Enhance velocity variation to create more expressive patterns through ghost notes (very quiet hits) and stronger accent dynamics, without changing pattern structure.

## Context

### Problem

Current velocity computation produces relatively flat dynamics:
- Velocity floor: 80% -> 30% (with ACCENT)
- Velocity ceiling: 88% -> 100% (with ACCENT)
- Micro-variation: 2% -> 7%

This creates patterns that sound "same-y" even when the hit positions vary. Research shows groove comes significantly from velocity dynamics.

### Design Solution

From `docs/design/v5-5-iterative-patterns/iteration-2.md` Proposal A:

1. **Expand velocity range**: Floor from 85% -> 20% (was 80% -> 30%)
2. **Ghost note injection**: Weak positions can become very quiet (15-30% velocity)
3. **Seed-based variation**: Different voices get independent velocity patterns

### Musical Impact

- Ghost notes create "feel" without changing hit positions
- Wider velocity range = more perceived variety
- Computational cost: Zero (same number of operations)

## Subtasks

- [ ] Update `ComputeAccentVelocity()` velocity floor to 0.85 - accent * 0.65
- [ ] Add ghost note injection for weak metric positions at high ACCENT
- [ ] Add independent seed offsets for shimmer and AUX velocity
- [ ] Add unit tests for velocity range at ACCENT extremes
- [ ] Add unit test for ghost note probability
- [ ] Verify pattern viz shows velocity variation
- [ ] All tests pass

## Acceptance Criteria

- [ ] At ACCENT=0: velocity range is ~80-88% (minimal variation)
- [ ] At ACCENT=1: velocity range is ~20-100% (wide variation)
- [ ] Ghost notes (velocity < 30%) appear at weak positions when ACCENT > 0.5
- [ ] Ghost note probability ~20% at max ACCENT for weak positions
- [ ] Shimmer velocity varies independently from anchor (different seed)
- [ ] AUX velocity varies independently from both voices
- [ ] All existing tests pass
- [ ] No new compiler warnings
- [ ] CPU overhead negligible (< 1% increase)

## Implementation Notes

### Files to Modify

- `src/Engine/VelocityCompute.cpp` - Update `ComputeAccentVelocity()`
- `src/Engine/VelocityCompute.h` - Add ghost note constants if needed
- `src/Engine/PatternGenerator.cpp` - Update velocity calls with seed offsets
- `tests/test_velocity_compute.cpp` - Add new tests

### Current Implementation

In `VelocityCompute.cpp`:
```cpp
float ComputeAccentVelocity(float accent, int step, int patternLength, uint32_t seed)
{
    accent = Clamp(accent, 0.0f, 1.0f);
    float metricWeight = GetMetricWeight(step, patternLength);
    
    float velocityFloor   = 0.80f - accent * 0.50f;   // 80% -> 30%
    float velocityCeiling = 0.88f + accent * 0.12f;   // 88% -> 100%
    
    float velocity = velocityFloor + metricWeight * (velocityCeiling - velocityFloor);
    
    // Micro-variation
    float variation = 0.02f + accent * 0.05f;
    uint32_t varHash = HashStep(seed ^ kVelVariationHashMagic, step);
    velocity += (HashToFloat(varHash) - 0.5f) * variation;
    
    return Clamp(velocity, 0.30f, 1.0f);
}
```

### New Implementation

```cpp
float ComputeAccentVelocity(float accent, int step, int patternLength, uint32_t seed)
{
    accent = Clamp(accent, 0.0f, 1.0f);
    float metricWeight = GetMetricWeight(step, patternLength);
    
    // EXPANDED: Velocity range scales more dramatically with ACCENT
    float velocityFloor   = 0.85f - accent * 0.65f;   // 85% -> 20% (was 80% -> 30%)
    float velocityCeiling = 0.88f + accent * 0.12f;   // 88% -> 100%
    
    // Map metric weight to velocity
    float velocity = velocityFloor + metricWeight * (velocityCeiling - velocityFloor);
    
    // NEW: Ghost note injection for weak positions at high ACCENT
    // ~20% of weak-position hits become ghost notes at high ACCENT
    if (metricWeight < 0.5f && accent > 0.5f) {
        float ghostProb = (accent - 0.5f) * 0.4f;  // 0% at accent=0.5, 20% at accent=1.0
        uint32_t ghostHash = HashStep(seed ^ 0x47485354, step);  // "GHST"
        if (HashToFloat(ghostHash) < ghostProb) {
            // Ghost note: 15-30% velocity
            uint32_t ghostVelHash = HashStep(seed ^ 0x47565F31, step);  // "GV_1"
            velocity = 0.15f + HashToFloat(ghostVelHash) * 0.15f;
        }
    }
    
    // Micro-variation for human feel
    float variation = 0.02f + accent * 0.06f;  // Slightly increased
    uint32_t varHash = HashStep(seed ^ kVelVariationHashMagic, step);
    velocity += (HashToFloat(varHash) - 0.5f) * variation;
    
    return Clamp(velocity, 0.10f, 1.0f);  // Lower floor to allow ghost notes
}
```

### PatternGenerator.cpp Updates

Update velocity computation to use independent seeds:
```cpp
// Anchor velocity (original seed)
result.anchorVelocity[step] = ComputeAccentVelocity(
    params.accent, step, params.patternLength, params.seed);

// Shimmer velocity (different seed offset for independence)
result.shimmerVelocity[step] = ComputeAccentVelocity(
    params.accent * 0.7f, step, params.patternLength, params.seed ^ 0x5000);

// AUX velocity (another different seed)
result.auxVelocity[step] = ComputeAccentVelocity(
    params.energy, step, params.patternLength, params.seed ^ 0xA000);
```

## Test Plan

1. Build firmware: `make clean && make`
2. Run tests: `make test`
3. Generate patterns with viz tool:
   ```bash
   ./build/pattern_viz --accent=0.0 --format=grid  # Flat dynamics
   ./build/pattern_viz --accent=0.5 --format=grid  # Moderate dynamics
   ./build/pattern_viz --accent=1.0 --format=grid  # Wide dynamics with ghosts
   ```
4. Verify ghost notes appear in output (velocity < 30 shown in grid)
5. Compare anchor vs shimmer velocities - should be different

## Estimated Effort

2-3 hours
