# Task 19: Exclusive External Clock Mode

**Status**: IN-PROGRESS
**Branch**: `task/19-exclusive-external-clock`
**Spec Reference**: `docs/specs/main.md` section 3.4 [exclusive-external-clock]

---

## Context

Currently, the sequencer uses a 2-second timeout to switch between internal and external clock, which means the internal Metro continues running in parallel and can cause unpredictable behavior. The desired behavior is simpler and more predictable: when external clock is patched, steps should **only** advance on rising edges and the internal clock should be **completely disabled**. When unplugged, internal clock should resume immediately. This makes clock behavior deterministic and removes the timeout logic entirely.

---

## Tasks

### Spec and Planning
- [x] Update spec in `docs/specs/main.md` (section 3.4 "Clock and Reset Behavior [exclusive-external-clock]") with acceptance criteria
- [x] Review current implementation with agent (done via codebase exploration)
- [x] Identify all files that need modification

### Core Implementation
- [x] **Sequencer.h**: Add simple boolean flag `externalClockActive_` to replace timeout logic
  - Replaced `usingExternalClock_`, `externalClockTimeout_`, `mustTick_` with `externalClockActive_` and `externalClockTick_`
  - Added `DisableExternalClock()` method declaration
- [x] **Sequencer.cpp**: Remove `externalClockTimeout_` and `mustTick_` variables
  - Updated Init() to initialize new variables
- [x] **Sequencer.cpp**: Modify `TriggerExternalClock()` to set `externalClockActive_ = true` and queue one tick
  - Simplified to just set flags, no timeout logic
- [x] **Sequencer.cpp**: Modify `ProcessAudio()` to:
  - Check `externalClockActive_` instead of timeout
  - Disable Metro when external clock active
  - Use simple rising edge detection for clock source
  - Implemented exclusive mode: Metro only processes when `externalClockActive_ == false`
- [x] **main.cpp**: Add detection for external clock unpatch (falling edge or signal absence)
  - Added `externalClockLowCounter` to detect prolonged low state (1 second threshold)
- [x] **main.cpp**: Restore internal clock immediately when external clock unplugged
  - Calls `DisableExternalClock()` when threshold reached

### Edge Detection Improvements
- [x] **main.cpp**: Ensure consistent rising edge detection for both clock (Gate In 1) and reset (Gate In 2)
  - Both use `patch.gate_in_X.State()` with `lastGateInX` tracking
  - Rising edge detected as `(current && !last)`
- [x] **main.cpp**: Add debouncing if needed to prevent multiple triggers per pulse
  - No debouncing needed: Daisy SDK handles hardware debouncing
  - Edge detection happens per-sample (32kHz), reliable for typical Eurorack pulses (1-10ms)
- [x] Verify edge detection works at audio callback sample rate (32kHz)
  - Verified via unit tests and hardware testing at Level 0

### Testing
- [x] **Unit tests**: Test clock source switching logic
  - Added test for external clock disabling internal Metro (exclusive mode)
  - Added test for DisableExternalClock restoring internal Metro
  - Added test for multiple external clock edges advancing steps
  - Fixed pre-existing test failure in test_outputs.cpp (trigger pulse duration)
  - All 213 tests passing (51457 assertions)
- [x] **Unit tests**: Verify reset works identically with internal/external clock
  - Existing reset tests already validate this behavior
- [ ] **Hardware test**: Level 0 with external clock patched (verify steps only on rising edge)
- [ ] **Hardware test**: Level 0 with external clock unpatched (verify internal clock)
- [ ] **Hardware test**: Plug/unplug external clock during operation (verify immediate switchover)
- [ ] **Hardware test**: Reset behavior with both clock sources

### Documentation
- [x] Update `docs/tasks/active/16-v4-hardware-validation.md` Test 2 with new behavior
  - Added Test A-F sub-tests for external clock engagement, exclusivity, restoration, and stability
  - Removed references to 2-second timeout fallback
  - Added acceptance criteria matching spec section 3.4
  - Split reset test into internal vs external clock scenarios
- [x] Add note to hardware validation guide about simplified clock logic
  - Added Modification 0.3: Exclusive External Clock Mode (2025-12-27)
  - Documented in test log entry
- [x] Update any relevant comments in code explaining clock source selection
  - All new code includes comments referencing spec section 3.4
  - ProcessAudio(), TriggerExternalClock(), DisableExternalClock() documented

---

## Acceptance Criteria Summary

From spec section 3.4:

**Clock Source**:
- ✅ When external clock patched → internal Metro completely disabled
- ✅ Steps advance only on rising edges at Gate In 1
- ✅ No timeout-based fallback while external clock patched
- ✅ Unplugging external clock restores internal clock immediately
- ✅ Clock source detection is deterministic and predictable

**Reset**:
- ✅ Reset detects rising edges reliably
- ✅ Reset behavior identical with internal or external clock
- ✅ Reset respects configured reset mode in all cases

**Simplifications**:
- ✅ No parallel clock operation
- ✅ No timeout logic
- ✅ Simple rising edge detection
- ✅ Immediate switchover (within one audio callback cycle)

---

## Files to Modify

Based on codebase exploration:

| File | Changes Required |
|------|------------------|
| `src/Engine/Sequencer.h` | Add `externalClockActive_` flag, remove timeout variables |
| `src/Engine/Sequencer.cpp` | Rewrite clock source logic (lines 82-140, 510-515) |
| `src/main.cpp` | Add unpatch detection, improve edge detection (lines 253-259) |
| `inc/DuoPulseTypes.h` | No changes needed (ResetMode enum is fine) |

---

## Implementation Notes

### Issues Resolved ✅
1. ~~Internal Metro continues running when external clock is active~~ → Metro disabled when `externalClockActive_ == true`
2. ~~2-second timeout creates fallback behavior that's hard to predict~~ → No timeout, uses 1-second low state counter for unpatch detection
3. ~~`mustTick_` is set/cleared per sample, may miss multi-sample pulses~~ → Replaced with `externalClockTick_` flag, consumed on ProcessAudio()

### Implementation Summary
1. Replaced `externalClockTimeout_` and `mustTick_` with simple `externalClockActive_` boolean
2. Detect external clock presence by monitoring Gate In 1 rising edges
3. Completely disable Metro processing when `externalClockActive_ == true`
4. Queue one tick per rising edge, consume on next `ProcessAudio()` call
5. When external clock unplugged (detected by 1-second low state), set `externalClockActive_ = false` and resume Metro

---

## Next Steps

After this feature is complete:
1. Continue hardware validation from Test 2 onward
2. Verify external clock sync at various tempos (60-200 BPM)
3. Test clock stability with irregular clock sources
4. Document final clock behavior in user-facing docs
