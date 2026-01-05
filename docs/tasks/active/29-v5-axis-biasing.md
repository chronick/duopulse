---
id: 29
slug: 29-v5-axis-biasing
title: "V5 AXIS X/Y: Bidirectional Biasing with Broken Mode"
status: completed
created_date: 2026-01-04
updated_date: 2026-01-04
completed_date: 2026-01-04
branch: feature/29-v5-axis-biasing
spec_refs:
  - "v5-design-final.md#a3-axis-biasing"
depends_on:
  - 27
---

# Task 29: V5 AXIS X/Y: Bidirectional Biasing with Broken Mode

## Objective

Implement the V5 AXIS X/Y parameters as bidirectional biasing controls with an emergent "broken mode" at high SHAPE + high AXIS X.

## Context

AXIS X/Y replace the V4 FIELD X/Y with enhanced semantics:

| Parameter | 0% | 50% (neutral) | 100% |
|-----------|-----|---------------|------|
| AXIS X | Grounded (downbeats) | Neutral | Floating (offbeats) |
| AXIS Y | Simple | Neutral | Complex (intricate) |

Key innovations:
- **Bidirectional effect**: 0.5 = neutral, moving either direction has effect
- **Increased Y range**: ±50% effect (up from ±30% in V4)
- **Broken mode**: Emergent behavior when SHAPE > 0.6 AND AXIS X > 0.7
- **No dead zones**: Continuous effect across full range

## Subtasks

- [x] Convert AXIS X from unipolar (0-1 → offbeats) to bipolar (0-1 → downbeats to offbeats)
- [x] Implement bidirectional X bias: suppress AND boost based on direction
- [x] Convert AXIS Y from unipolar to bipolar with increased ±50% range
- [x] Implement "broken mode" detection (SHAPE > 0.6 AND AXIS X > 0.7)
- [x] Add downbeat suppression in broken mode (seed-based 60% chance at brokenIntensity)
- [x] Document "broken mode" interaction zone in user-facing notes
- [x] Ensure minimum weight floor of 0.05 (no complete step elimination)
- [x] Add unit tests for bidirectional behavior
- [x] All tests pass (`make test`)

## Acceptance Criteria

- [x] AXIS X at 0% emphasizes downbeats, at 100% emphasizes offbeats
- [x] AXIS X at 50% has no bias effect (neutral)
- [x] AXIS Y at 0% simplifies pattern, at 100% adds complexity
- [x] Broken mode activates at high SHAPE + high AXIS X
- [x] Minimum step weight is 0.05 (never zero)
- [x] Build compiles without errors
- [x] All tests pass

## Implementation Notes

### Algorithm (from v5-design-final.md Appendix A.3)

```cpp
void ApplyAxisBias(float* baseWeights, float axisX, float axisY,
                   float shape, uint32_t seed, int patternLength)
{
    // Compute bipolar biases (0.5 = neutral)
    float xBias = (axisX - 0.5f) * 2.0f;  // [-1.0, 1.0]
    float yBias = (axisY - 0.5f) * 2.0f;  // [-1.0, 1.0]

    // Apply X bias (bidirectional)
    for (int step = 0; step < patternLength; ++step) {
        float positionStrength = GetPositionStrength(step, patternLength);
        float xEffect;

        if (xBias > 0.0f) {
            // Moving toward offbeats: suppress downbeats, boost offbeats
            if (positionStrength < 0.0f) {
                xEffect = -xBias * fabsf(positionStrength) * 0.45f;
            } else {
                xEffect = xBias * positionStrength * 0.60f;
            }
        } else {
            // Moving toward downbeats: boost downbeats, suppress offbeats
            if (positionStrength < 0.0f) {
                xEffect = -xBias * fabsf(positionStrength) * 0.60f;
            } else {
                xEffect = xBias * positionStrength * 0.45f;
            }
        }

        weights[step] = Clamp(baseWeights[step] + xEffect, 0.05f, 1.0f);
    }

    // Apply Y bias (intricacy) - increased to ±50%
    for (int step = 0; step < patternLength; ++step) {
        bool isWeakPosition = GetMetricWeight(step, patternLength) < 0.5f;
        float yEffect;

        if (yBias > 0.0f) {
            yEffect = yBias * (isWeakPosition ? 0.50f : 0.15f);
        } else {
            yEffect = yBias * (isWeakPosition ? 0.50f : -0.25f);
        }

        weights[step] = Clamp(weights[step] + yEffect, 0.05f, 1.0f);
    }

    // "Broken" mode at high SHAPE + high AXIS X
    // NOTE: downbeatPositions must be a fixed ordered array for determinism
    // e.g., static const int downbeatPositions[] = {0, 4, 8, 12} for 16-step
    if (shape > 0.6f && axisX > 0.7f) {
        float brokenIntensity = (shape - 0.6f) * 2.5f * (axisX - 0.7f) * 3.33f;
        for (int i = 0; i < numDownbeats; ++i) {
            int step = downbeatPositions[i];  // Ordered array, not set
            if (HashToFloat(seed ^ 0xDEADBEEF, step) < brokenIntensity * 0.6f) {
                weights[step] *= 0.25f;
            }
        }
    }
}
```

### Position Strength Function

```cpp
// Returns -1.0 (strong downbeat) to +1.0 (strong offbeat)
float GetPositionStrength(int step, int patternLength) {
    float metricWeight = GetMetricWeight(step, patternLength);
    // Invert: high metric weight = strong downbeat = negative value
    return 1.0f - 2.0f * metricWeight;
}
```

### Files to Modify

- `src/Engine/PatternField.cpp` - Add ApplyAxisBias function
- `src/Engine/PatternField.h` - Add function declaration
- `tests/PatternFieldTest.cpp` - Add bidirectional and broken mode tests

### Constraints

- Real-time safe: no allocations
- Minimum weight floor of 0.05 prevents complete step elimination
- Broken mode is emergent, not explicitly selectable

### User Documentation Note

Add to user guide:
> **Hidden Feature**: When SHAPE is above 60% and AXIS X is above 70%, an emergent "broken mode" activates that randomly suppresses downbeats for a more fractured, IDM-style feel.

### Risks

- Broken mode threshold tuning may need adjustment
- Interaction between SHAPE and AXIS X could be confusing if not documented
