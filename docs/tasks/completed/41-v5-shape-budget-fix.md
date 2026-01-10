---
id: 41
slug: 41-v5-shape-budget-fix
title: "V5 SHAPE Budget Fix: Zone Boundaries and Anchor/Shimmer Ratio"
status: completed
created_date: 2026-01-06
updated_date: 2026-01-06
completed_date: 2026-01-06
branch: feature/41-v5-shape-budget-fix
commits:
  - 841fa40
spec_refs:
  - "5.2 Three-Way Blending"
  - "5.4 SHAPE-Modulated Hit Budget"
---

# Task 41: V5 SHAPE Budget Fix

## Objective

Fix two bugs in HitBudget.cpp where SHAPE zone boundaries and anchor/shimmer ratio modulation don't match the V5 spec.

## Background

**Bug 1 - Zone Boundaries**: Implementation uses 0.33/0.66 thresholds instead of spec's 0.30/0.70.

```cpp
// Current (WRONG)
if (shape < 0.33f)      // Stable
else if (shape < 0.66f) // Syncopated
else                    // Wild

// Spec requires
if (shape < 0.30f)      // Stable
else if (shape < 0.70f) // Syncopated
else                    // Wild
```

**Bug 2 - Anchor/Shimmer Ratio**: Spec section 5.4 requires separate multipliers:

| Zone | Anchor Budget | Shimmer Budget |
|------|---------------|----------------|
| Stable (0-30%) | 100% of base | 100% of base |
| Syncopated (30-70%) | 90-100% | 110-130% |
| Wild (70-100%) | 80-90% | 130-150% |

Current implementation uses single `GetShapeBudgetMultiplier()` for both voices.

## Subtasks

- [ ] Fix zone boundary constants: 0.33→0.30, 0.66→0.70 in `GetShapeBudgetMultiplier()`
- [ ] Create `GetAnchorBudgetMultiplier(float shape)` with spec values (100%→80%)
- [ ] Create `GetShimmerBudgetMultiplier(float shape)` with spec values (100%→150%)
- [ ] Update `ComputeAnchorBudget()` to use anchor-specific multiplier
- [ ] Update `ComputeShimmerBudget()` to use shimmer-specific multiplier
- [ ] Add unit tests for zone boundary behavior (test at 0.29, 0.30, 0.31, 0.69, 0.70, 0.71)
- [ ] Add unit tests verifying anchor/shimmer ratio diverges in Wild zone
- [ ] Verify pattern_viz tool output reflects changes
- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] Zone boundaries at exactly 0.30 and 0.70 (matching spec 5.2)
- [ ] At SHAPE=0.85 (Wild): anchor ~85% of base, shimmer ~140% of base
- [ ] At SHAPE=0.50 (Syncopated): anchor ~95% of base, shimmer ~120% of base
- [ ] At SHAPE=0.15 (Stable): anchor=100%, shimmer=100%
- [ ] All existing tests pass
- [ ] No new compiler warnings

## Implementation Notes

### Files to Modify

- `src/Engine/HitBudget.cpp` - Split multiplier function, fix boundaries
- `src/Engine/HitBudget.h` - Add new function declarations
- `tests/test_hit_budget.cpp` - Add zone boundary and ratio tests (create if needed)

### New Function Signatures

```cpp
// Replace GetShapeBudgetMultiplier() with these two:
float GetAnchorBudgetMultiplier(float shape);
float GetShimmerBudgetMultiplier(float shape);
```

### Multiplier Curves (per spec 5.4)

**Anchor multiplier:**
- Stable (0-0.30): 1.0
- Syncopated (0.30-0.70): lerp(1.0, 0.90) = 1.0 - progress * 0.10
- Wild (0.70-1.0): lerp(0.90, 0.80) = 0.90 - progress * 0.10

**Shimmer multiplier:**
- Stable (0-0.30): 1.0
- Syncopated (0.30-0.70): lerp(1.0, 1.30) = 1.0 + progress * 0.30
- Wild (0.70-1.0): lerp(1.30, 1.50) = 1.30 + progress * 0.20

### Constraints

- No heap allocation (real-time audio path)
- Multiplier functions must be branch-free or low-branch for performance
- Maintain backward compatibility with existing parameter ranges

### Risks

- Changing zone boundaries may affect existing patterns users have tuned
- Need to verify pattern_viz tool uses same boundaries after fix
