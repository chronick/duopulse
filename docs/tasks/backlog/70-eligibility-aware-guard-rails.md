---
id: 70
slug: eligibility-aware-guard-rails
title: "Make Guard Rails Eligibility-Aware"
status: backlog
created_date: 2026-01-20
updated_date: 2026-01-20
spec_refs:
  - "docs/specs/main.md#6-pattern-generation"
depends_on:
  - iteration-2026-01-20-001  # Eligibility mask wiring
---

# Task 70: Make Guard Rails Eligibility-Aware

## Context

Iteration 2026-01-20-001 wired eligibility masks into pattern generation, correctly constraining hit positions by ENERGY zone. However, guard rails (especially `EnforceMaxGap`) can add hits on ineligible positions, breaking the zone constraints.

### Current Behavior

When `EnforceMaxGap` detects a gap longer than the zone's maximum:
1. It finds the midpoint of the gap via `FindGapMidpoints()`
2. It adds a hit at that midpoint: `anchorMask |= (1ULL << step)`
3. The midpoint position is NOT checked against eligibility

Example at ENERGY=0.5 (BUILD zone, 8th notes only):
- Gumbel selection places hits at even positions (eligible)
- A gap of 8+ steps triggers `EnforceMaxGap`
- Guard rail adds hit at position 25 (odd = ineligible!)
- Result: Pattern violates zone constraints

### Impact

- Syncopation remains high even when eligibility should constrain to strong beats
- Zone character is broken in BUILD+ zones where gap enforcement is active
- Evaluation metrics don't reflect the intended zone behavior

## Design

### Approach: Pass Eligibility to Guard Rails

Modify `ApplyHardGuardRails` and related functions to accept eligibility masks:

```cpp
int ApplyHardGuardRails(uint64_t& anchorMask,
                        uint64_t& shimmerMask,
                        uint64_t anchorEligibility,  // NEW
                        uint64_t shimmerEligibility, // NEW
                        EnergyZone zone,
                        Genre genre,
                        int patternLength);

int EnforceMaxGap(uint64_t& anchorMask,
                  uint64_t eligibility,  // NEW
                  EnergyZone zone,
                  int patternLength);
```

When adding a fill hit, only consider eligible positions:

```cpp
// Find gaps and add hits to break them (eligibility-aware)
while (true)
{
    uint64_t gapMidpoints = FindGapMidpoints(anchorMask, maxGap + 1, clampedLength);

    // Filter to eligible positions only
    gapMidpoints &= eligibility;

    if (gapMidpoints == 0)
    {
        break;  // No eligible positions in gaps
    }

    // Add hit at first eligible midpoint
    // ...
}
```

### Alternative: Snap to Nearest Eligible Position

If exact midpoint is ineligible, snap to the nearest eligible position:

```cpp
int FindNearestEligible(int targetStep, uint64_t eligibility, int patternLength)
{
    for (int offset = 0; offset < patternLength / 2; ++offset)
    {
        int before = (targetStep - offset + patternLength) % patternLength;
        int after = (targetStep + offset) % patternLength;

        if (eligibility & (1ULL << before)) return before;
        if (eligibility & (1ULL << after)) return after;
    }
    return -1;  // No eligible position found
}
```

### Functions to Modify

1. `EnforceMaxGap()` - Main gap-filling logic
2. `EnforceDownbeat()` - Already adds at step 0, should verify eligibility
3. `SoftRepairPass()` - Rescue candidates should be eligibility-filtered
4. `FindRescueCandidate()` - Add eligibility mask parameter

## Subtasks

- [ ] Add eligibility parameter to `EnforceMaxGap()` signature
- [ ] Filter `gapMidpoints` by eligibility before adding fills
- [ ] Add eligibility parameter to `ApplyHardGuardRails()`
- [ ] Update `PatternGenerator.cpp` to pass eligibility masks
- [ ] Add eligibility to `SoftRepairPass()` and `FindRescueCandidate()`
- [ ] Update tests to verify eligibility is respected
- [ ] Verify syncopation improves in BUILD zone after fix

## Acceptance Criteria

- [ ] Guard rails only add hits on eligible positions
- [ ] Syncopation metric improves for BUILD zone patterns
- [ ] All 373+ existing tests pass
- [ ] No performance regression

## Test Plan

1. Generate BUILD zone pattern (ENERGY=0.5) with long gaps
2. Verify guard rail fills land on even positions (8th notes) only
3. Run full evaluation and check syncopation improvement
4. Verify MINIMAL zone still works (no guard rail fills due to max gap = 32)

## Files to Modify

- `src/Engine/GuardRails.h` - Update function signatures
- `src/Engine/GuardRails.cpp` - Implement eligibility filtering
- `src/Engine/PatternGenerator.cpp` - Pass eligibility to guard rails
- `tests/test_guard_rails.cpp` - Add eligibility verification tests

## Estimated Effort

2-3 hours

## Related

- Iteration 2026-01-20-001: Discovered this issue while wiring eligibility
- Task 66: PatternField config wiring (similar architecture pattern)
