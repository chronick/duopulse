---
id: 35
slug: 35-v5-accent-velocity
title: "V5 ACCENT Parameter: Musical Weight to Velocity"
status: completed
created_date: 2026-01-04
updated_date: 2026-01-04
branch: feature/35-v5-accent-velocity
spec_refs:
  - "v5-design-final.md#a7-accent"
depends_on:
  - 27
---

# Task 35: V5 ACCENT Parameter: Musical Weight to Velocity

## Objective

Implement the V5 ACCENT parameter that maps musical metric weight to velocity, replacing the V4 PUNCH parameter with position-aware dynamics.

## Context

V5 ACCENT (Config K4) replaces V4 PUNCH with fundamentally different semantics:

| ACCENT | Velocity Floor | Velocity Ceiling | Feel |
|--------|---------------|------------------|------|
| 0% | 80% | 88% | Flat (all hits equal) |
| 100% | 30% | 100% | Dynamic (ghost notes to accents) |

Key innovations:
- **Position-aware**: Velocity based on metric weight, not random
- **Wider range at high ACCENT**: 30-100% vs 65-100% in V4
- **Micro-variation**: Small human-feel variation (2-7%)
- **Deterministic**: Same seed produces same velocities

This differs from V4 PUNCH which used accent probability and random boosts.

## Subtasks

- [ ] Create `ComputeAccentVelocity()` function with metric weight mapping
- [ ] Implement velocity floor/ceiling scaling with ACCENT parameter
- [ ] Add micro-variation based on seed and step
- [ ] Clamp final velocity to 0.30-1.0 range
- [ ] Remove V4 PUNCH logic (accent probability, random boost)
- [ ] Wire ACCENT through VelocityCompute for both voices
- [ ] Update PunchParams struct to AccentParams (or remove)
- [ ] Add unit tests for velocity distribution
- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] ACCENT=0% produces flat dynamics (all hits ~80-88%)
- [ ] ACCENT=100% produces wide dynamics (30-100%)
- [ ] Strong beats (downbeats) have higher velocity
- [ ] Weak beats (offbeats) have lower velocity at high ACCENT
- [ ] Same seed + step produces same velocity
- [ ] Build compiles without errors
- [ ] All tests pass

## Implementation Notes

### Algorithm (from v5-design-final.md Appendix A.7)

```cpp
float ComputeAccentVelocity(float accent, int step,
                            int patternLength, uint32_t seed)
{
    // Get metric weight (0.0 = weak offbeat, 1.0 = strong downbeat)
    float metricWeight = GetMetricWeight(step, patternLength);

    // Velocity range scales with ACCENT
    float velocityFloor = 0.80f - accent * 0.50f;    // 80% -> 30%
    float velocityCeiling = 0.88f + accent * 0.12f;  // 88% -> 100%

    // Map metric weight to velocity
    float velocity = velocityFloor +
                     metricWeight * (velocityCeiling - velocityFloor);

    // Micro-variation for human feel
    // HashToFloat returns 0-1, subtracting 0.5 gives ±0.5 range
    // Actual variation: ±(0.5 × variation) = ±1% to ±3.5%
    float variation = 0.02f + accent * 0.05f;
    velocity += (HashToFloat(seed, step) - 0.5f) * variation;

    // Clamp to valid range
    return Clamp(velocity, 0.30f, 1.0f);
}
```

### Metric Weight Reference

```cpp
// GetMetricWeight returns 0.0-1.0 based on step position
// For 16-step pattern:
// Step 0 (beat 1): 1.0 (strongest)
// Step 4 (beat 2): 0.8
// Step 8 (beat 3): 0.9
// Step 12 (beat 4): 0.8
// Step 2, 6, 10, 14 (8th notes): 0.5
// Step 1, 3, 5... (16th notes): 0.25
```

### Migration from PunchParams

V4 PunchParams:
```cpp
struct PunchParams {
    float accentProbability;  // Remove
    float velocityFloor;      // Keep, change formula
    float accentBoost;        // Remove
    float velocityVariation;  // Keep, reduce range
};
```

V5 AccentParams:
```cpp
struct AccentParams {
    float velocityFloor;     // 0.80 - accent * 0.50
    float velocityCeiling;   // 0.88 + accent * 0.12
    float variation;         // 0.02 + accent * 0.05
};
```

### Files to Modify

- `src/Engine/VelocityCompute.cpp` - Replace PUNCH logic with ACCENT logic
- `src/Engine/VelocityCompute.h` - Update function signatures
- `src/Engine/ControlState.h` - Rename PunchParams to AccentParams
- `tests/VelocityComputeTest.cpp` - Update tests for new algorithm

### Constraints

- Velocity output range: 0.30-1.0 (30% minimum for VCA audibility)
- Deterministic with seed
- Real-time safe

### Risks

- Feel may differ significantly from V4 PUNCH
- Users accustomed to random accent placement may need adjustment
- Metric weight calculation for non-16-step patterns
