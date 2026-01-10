---
id: 31
slug: 31-v5-hat-burst
title: "V5 Hat Burst: Pattern-Aware Fill Triggers"
status: completed
created_date: 2026-01-04
updated_date: 2026-01-04
completed_date: 2026-01-04
branch: feature/31-v5-hat-burst
spec_refs:
  - "v5-design-final.md#aux-output-modes"
  - "v5-design-final.md#a5-hat-burst"
depends_on:
  - 27
---

# Task 31: V5 Hat Burst: Pattern-Aware Fill Triggers

## Objective

Implement the V5 Hat Burst feature that produces pattern-aware hat triggers during fills, with density following ENERGY and regularity following SHAPE.

## Context

V5 introduces HAT mode as a "secret 2.5 pulse" feature:
- AUX output produces pattern-aware hat burst during fills
- Burst density follows ENERGY (2-12 triggers)
- Burst regularity follows SHAPE
- Pre-allocated arrays (max 12 triggers, no heap)
- Velocity ducking: 70% reduction near main pattern hits

This is the **main new feature** in V5 (only ~20% exists in V4).

**Note**: Uses SHAPE value to determine regularity but doesn't require Task 28's blending algorithm to be complete. SHAPE is just read as a parameter value (0-1).

## Subtasks

- [x] Create `HatBurst` struct with pre-allocated trigger array (12 max)
- [x] Implement `GenerateHatBurst()` with ENERGY-based trigger count
- [x] Add SHAPE-based timing patterns:
  - [x] Low SHAPE (0-30%): Even spacing
  - [x] Medium SHAPE (30-70%): Euclidean with jitter
  - [x] High SHAPE (70-100%): Random steps
- [x] Implement collision detection with nearest-empty nudging
- [x] Add velocity ducking (30% when near main hits)
- [~] Wire hat burst to AUX output during fill zones (deferred to Task 32)
- [~] Wire hat burst to AUX output when AuxMode == HAT (deferred to Task 32)
- [x] Note: LED feedback for HAT mode unlock is implemented in Task 34
- [x] Add unit tests for burst generation and collision detection
- [x] All tests pass (`make test`)

## Acceptance Criteria

- [x] Hat burst generation algorithm implemented
- [x] Trigger count scales with ENERGY (2 at 0%, 12 at 100%)
- [x] Timing regularity follows SHAPE
- [x] No trigger collisions (nearest-empty nudging works)
- [x] Velocity is 30% when near main pattern hits
- [x] No heap allocations in audio path
- [x] Build compiles without errors
- [x] All tests pass

## Implementation Notes

### Pre-allocated Structure (from v5-design-final.md Appendix A.5)

```cpp
struct HatBurst {
    struct Trigger {
        uint8_t step;      // Step position within fill
        float velocity;    // 0.0-1.0
    };

    Trigger triggers[12];  // Max 12 triggers
    uint8_t count;         // Actual trigger count
    uint8_t fillStart;     // Fill zone start step
    uint8_t fillDuration;  // Fill zone length
    uint8_t _pad;          // Alignment
};
// Size: 68-96 bytes depending on alignment
```

### Generation Algorithm

```cpp
void GenerateHatBurst(float energy, float shape,
                      uint32_t mainPattern, int fillStart,
                      int fillDuration, uint32_t seed,
                      HatBurst& burst)
{
    // Determine trigger count (with div-by-zero guard)
    int triggerCount = Max(1, (int)(2 + energy * 10));  // 2-12 triggers

    // Track used steps for collision detection
    uint32_t usedSteps = 0;
    burst.count = 0;
    burst.fillStart = fillStart;
    burst.fillDuration = fillDuration;

    for (int i = 0; i < triggerCount && burst.count < 12; ++i) {
        int step;

        if (shape < 0.3f) {
            // Even spacing
            step = (i * fillDuration) / Max(1, triggerCount);
        } else if (shape < 0.7f) {
            // Euclidean with jitter
            step = EuclideanWithJitter(i, fillDuration, shape, seed);
        } else {
            // Random
            step = RandomStep(fillDuration, seed, i);
        }

        // Collision detection: nudge to nearest empty
        if (usedSteps & (1U << step)) {
            step = FindNearestEmpty(step, fillDuration, usedSteps);
        }

        if (step >= 0) {  // Valid position found
            usedSteps |= (1U << step);
            burst.triggers[burst.count].step = fillStart + step;
            burst.count++;
        }
    }

    // Velocity ducking near main hits
    for (int i = 0; i < burst.count; ++i) {
        bool nearMainHit = CheckProximity(burst.triggers[i].step,
                                          mainPattern, 1);
        float baseVelocity = 0.65f + 0.35f * energy;
        burst.triggers[i].velocity = nearMainHit ?
            baseVelocity * 0.30f : baseVelocity;
    }
}
```

### Collision Detection

```cpp
int FindNearestEmpty(int step, int fillDuration, uint32_t usedSteps) {
    // Search outward from step for empty position
    for (int offset = 1; offset < fillDuration; ++offset) {
        int left = (step - offset + fillDuration) % fillDuration;
        int right = (step + offset) % fillDuration;

        if (!(usedSteps & (1U << left))) return left;
        if (!(usedSteps & (1U << right))) return right;
    }
    return -1;  // No empty position
}
```

### Files Created/Modified

- `src/Engine/HatBurst.h` - New: HatBurst struct and generation functions
- `src/Engine/HatBurst.cpp` - New: Implementation
- `tests/test_hat_burst.cpp` - New: Unit tests (584 lines, 13 test cases)

### Deferred to Task 32

- `src/Engine/AuxOutput.cpp` - Wire hat burst to AUX output
- `src/Engine/AuxOutput.h` - Add HatBurst member

### Constraints

- **No heap allocation**: Fixed 12-trigger array
- **Bounded loops**: Max 12 triggers, fillDuration iterations
- **Real-time safe**: All computation deterministic

### Risks

- Velocity ducking threshold (1 step proximity) may need tuning
- Euclidean with jitter implementation complexity
