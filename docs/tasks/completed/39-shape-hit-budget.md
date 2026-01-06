---
id: 39
slug: 39-shape-hit-budget
title: "SHAPE-Modulated Hit Budget for Pattern Variation"
status: completed
created_date: 2026-01-06
updated_date: 2026-01-06
completed_date: 2026-01-06
branch: feature/39-shape-hit-budget
spec_refs:
  - "main.md#section-6.1"
  - "main.md#section-6.2"
related:
  - 28  # V5 SHAPE algorithm
  - 38  # Pattern viz tests (discovered issue)
---

# Task 39: SHAPE-Modulated Hit Budget for Pattern Variation

## Objective

Add SHAPE parameter influence to hit budget calculation so that "stable" patterns are sparser and "wild" patterns are denser. This breaks the mathematical convergence that currently locks patterns to four-on-floor at moderate energy.

## Problem Statement

Currently at moderate energy (GROOVE zone, ~0.5 ENERGY):
- Anchor budget = exactly 8 hits for 32 steps
- Spacing constraint = ~4 steps minimum
- 8 hits with 4-step spacing = mathematically forced to every 4th step
- Result: **all patterns converge to four-on-floor regardless of SHAPE/seed**

SHAPE affects weights but not hit count, so Gumbel selection always picks the same 8 positions.

## Solution

Multiply hit budget by SHAPE zone factor:
- **Stable zone** (SHAPE < 0.33): 0.75x-0.85x multiplier (6-7 hits)
- **Syncopated zone** (0.33-0.66): 1.0x multiplier (8 hits, unchanged)
- **Wild zone** (SHAPE > 0.66): 1.15x-1.25x multiplier (9-10 hits)

More hits = more positions to fill = more room for variation.
Fewer hits = more sparse = different groove character.

## Subtasks

- [x] Add `float shape` parameter to `ComputeAnchorBudget()` signature
- [x] Add `float shape` parameter to `ComputeShimmerBudget()` signature
- [x] Implement SHAPE zone detection in budget functions (`GetShapeBudgetMultiplier()`)
- [x] Apply multiplier: stable=0.75-0.85x, syncopated=1.0x, wild=1.15-1.25x
- [x] Update `ComputeBarBudget()` to accept and pass SHAPE
- [x] Update `Sequencer::GenerateBar()` to pass SHAPE to budget computation
- [x] Add unit tests: different SHAPE values produce different hit counts
- [x] Update pattern_viz_test.cpp to verify budget variation with SHAPE
- [x] All tests pass (`make test`) - 362 tests, 62749 assertions

## Acceptance Criteria

- [x] `ComputeAnchorBudget(0.5, BUILD, 32, 0.15)` returns 3 hits (stable)
- [x] `ComputeAnchorBudget(0.5, BUILD, 32, 0.50)` returns 4 hits (syncopated)
- [x] `ComputeAnchorBudget(0.5, BUILD, 32, 0.85)` returns 5 hits (wild)
- [x] Pattern viz budget varies with SHAPE (guard rails may add hits to enforce max gap)
- [x] Shimmer budget scales proportionally
- [x] No regression in existing tests (362 tests pass)
- [x] No new compiler warnings

**Note**: At energy=0.5, the zone is BUILD (not GROOVE). Guard rails enforce max gap of 6 steps, which may add hits beyond the budget to maintain musical groove.

## Implementation Notes

### Files to Modify

- `src/Engine/HitBudget.h` - Update function signatures
- `src/Engine/HitBudget.cpp` - Add SHAPE multiplier logic
- `src/Engine/Sequencer.cpp` - Pass shape to ComputeBarBudget
- `tests/test_hit_budget.cpp` - Add SHAPE budget tests (new test case)
- `tests/pattern_viz_test.cpp` - Update convergence tests

### SHAPE Zone Detection

```cpp
float GetShapeBudgetMultiplier(float shape)
{
    if (shape < 0.33f)
    {
        // Stable zone: sparse patterns
        return 0.75f + (shape / 0.33f) * 0.10f;  // 0.75-0.85
    }
    else if (shape < 0.66f)
    {
        // Syncopated zone: normal density
        return 1.0f;
    }
    else
    {
        // Wild zone: denser patterns
        float wildProgress = (shape - 0.66f) / 0.34f;
        return 1.15f + wildProgress * 0.10f;  // 1.15-1.25
    }
}
```

### Constraints

- Multiplier must be applied BEFORE zone-based clamping
- Shimmer budget derives from anchor, so it will scale automatically
- RT-safe: no allocations, simple math only

### Risks

- Changing hit counts could affect musicality at edge cases
- Need to ensure stable zone still sounds good with fewer hits
- Wild zone with more hits might be too chaotic - may need tuning

## Test Cases

```cpp
TEST_CASE("SHAPE affects anchor budget", "[hit-budget][shape]")
{
    SECTION("Stable zone reduces hits")
    {
        int stable = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.15f);
        int normal = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.50f);
        REQUIRE(stable < normal);
    }

    SECTION("Wild zone increases hits")
    {
        int normal = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.50f);
        int wild = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.85f);
        REQUIRE(wild > normal);
    }
}
```

## References

- Issue discovered in Task 38 pattern viz tests
- V5 SHAPE algorithm: Task 28
- Hit budget spec: docs/specs/main.md section 6.1-6.2
