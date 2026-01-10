# Task 37: V5 Code Review Critical Fixes

---
id: 37
feature: v5-code-review-fixes
status: completed
priority: critical
created_at: 2026-01-04
updated_at: 2026-01-04
completed_date: 2026-01-04
commits:
  - 019b1fd
---

## User Notes
- Important: Include tests for each bug where relevant.

## Summary

Fix critical and high-severity issues identified during pre-merge code review of feature/duopulse-v5 branch. These issues were found by parallel code-reviewer agents examining +6736/-1245 lines across 43 files.

## Context

- **Branch**: feature/duopulse-v5
- **Build Size**: 98.95% FLASH utilization (dangerously close to limit)
- **Tests**: 338 test cases, 62350 assertions passing
- **Build**: Fixed LedLayer::FLASH → FLASH_EVT rename (STM32 macro conflict)

## Critical Issues (Must Fix)

### 1. VoiceRelation: Buffer over-read in shimmerWeights

**File**: `src/Engine/VoiceRelation.cpp:246`

**Issue**: `PlaceWeightedBest()` accesses `shimmerWeights[step]` without validating bounds. Array is sized `kMaxSteps` (32), but if caller passes larger `patternLength`, `step` could exceed bounds.

**Fix**: Add bounds check or assert at function entry:
```cpp
assert(patternLength <= kMaxSteps && "patternLength exceeds array bounds");
```

### 2. VoiceRelation: Null pointer not validated

**File**: `src/Engine/VoiceRelation.cpp:211-251`

**Issue**: `shimmerWeights` pointer used without null check. Crash if nullptr passed.

**Fix**: Add defensive check:
```cpp
if (!shimmerWeights) return -1;  // Or appropriate error handling
```

### 3. HatBurst: Trigger count invariant violation

**File**: `src/Engine/HatBurst.cpp:144-185`

**Issue**: Documentation states `burst.count` is in range [2, 12], but when `fillDuration` is very small (1-2 steps) and collisions occur, the loop can skip triggers, resulting in `burst.count < 2`.

**Fix**: Either:
- Clamp minimum `fillDuration` to ensure at least 2 triggers can be placed
- Or update documentation to reflect actual behavior

## High Issues (Should Fix)

### 4. VelocityCompute hardcodes 16-step pattern length

**File**: `src/Engine/VelocityCompute.cpp:151,193,228`

**Issue**: `GetMetricWeight(step, 16)` hardcodes 16 instead of using actual `patternLength`. For 32-step patterns, step 16 is treated as step 0.

**Fix**: Pass `patternLength` parameter to velocity compute functions.

### 5. HatBurst proximity check uses wrong modulo

**File**: `src/Engine/HatBurst.cpp:68-78`

**Issue**: `CheckProximity` uses `kMaxSteps` (32) instead of actual `patternLength`. Wrong positions checked in mainPattern bitmask.

**Fix**: Pass `patternLength` to `GenerateHatBurst()` and use it in `CheckProximity()`.

### 6. RNG seed correlation

**File**: `src/Engine/VoiceRelation.cpp:205`

**Issue**: `seed ^ 0xDEADBEEF` causes seeds 0 and 0xDEADBEEF to produce identical sequences.

**Fix**: Use a better mixing function or validate seed input.

### 7. Gap distribution can exceed target

**File**: `src/Engine/VoiceRelation.cpp:267-274`

**Issue**: `std::max(1, ...)` for gapShare can sum to more than `targetHits` across multiple gaps.

**Fix**: Track remaining budget and break early when exhausted.

## Design Debt (Track for Later)

### 8. ControlState reference aliases

**File**: `src/Engine/ControlState.h:492-516`

**Issue**: Reference member aliases (`float& build = shape`) break copy semantics. Currently safe because struct is never copied, but fragile.

**Recommendation**: Replace with getter methods in future refactor.

### 9. LED layer system unused

**File**: `src/main.cpp` vs `src/Engine/LedIndicator.cpp`

**Issue**: V5 layer system (`SetLayer`, `ComputeFinalBrightness`) implemented but not used in production code.

**Recommendation**: Either integrate or remove to reduce binary size (we're at 98.95% FLASH).

## Verification

- [x] All critical issues have unit tests
- [x] `make test` passes (348 test cases, 62476 assertions)
- [x] `make` builds without warnings
- [x] FLASH usage stays under 100% (98.96%)

## Subtasks

- [x] Fix shimmerWeights bounds check (Issue #1)
- [x] Fix shimmerWeights null check (Issue #2)
- [x] Fix HatBurst trigger count guarantee (Issue #3)
- [x] Add patternLength to VelocityCompute (Issue #4)
- [x] Add patternLength to HatBurst proximity (Issue #5)
- [x] Fix RNG seed mixing (Issue #6)
- [x] Fix gap distribution overflow (Issue #7)

## References

- Code review session: 2026-01-04
- Reviewers: 4 parallel code-reviewer agents
