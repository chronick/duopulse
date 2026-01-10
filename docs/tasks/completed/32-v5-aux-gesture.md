---
id: 32
slug: 32-v5-aux-gesture
title: "V5 Hold+Switch Gesture for AUX Mode Selection"
status: completed
created_date: 2026-01-04
updated_date: 2026-01-04
completed_date: 2026-01-04
branch: feature/32-v5-aux-gesture
spec_refs:
  - "v5-design-final.md#button--switch-behavior"
  - "v5-design-final.md#aux-mode-gesture-priority"
depends_on:
  - 27
---

# Task 32: V5 Hold+Switch Gesture for AUX Mode Selection

## Objective

Implement the V5 "secret mode" gesture where Hold + Switch selects AUX output mode (HAT vs FILL GATE), replacing the V4 config knob approach.

## Context

V5 moves AUX mode selection from a config knob to a "discoverable easter egg" gesture:

| Gesture | Effect |
|---------|--------|
| Hold + Switch UP | Set AUX to HAT ("2.5 pulse" secret) |
| Hold + Switch DOWN | Set AUX to FILL GATE (default) |

Key behaviors:
- Button hold + switch movement triggers AUX mode change
- Switch event is consumed (doesn't change Perf/Config mode)
- Pending fill is cancelled
- Button release returns to normal (no fill triggered)
- ~~AUX mode defaults to FILL GATE on boot (not persisted)~~

This is the "secret mode" that makes HAT a discoverable feature.

### ADDENDUM: AUX Mode Persistence (Task 33)

AUX mode is now **persistent** until explicitly changed:
- Same Hold+Switch gesture works at **boot** and **runtime**
- Normal boot (no button held) → keeps previous AUX mode
- See Task 33 for boot-time detection implementation

## Subtasks

- [x] Add AUX gesture state to ButtonState (auxGestureActive, switchMovedWhileHeld)
- [x] Detect switch movement while button is held
- [x] Implement gesture priority:
  - [x] Cancel pending shift mode activation
  - [x] Cancel pending live fill
  - [x] Set AUX mode based on switch direction
  - [x] Consume switch event (don't change mode)
  - [x] Mark button release as gesture-consumed (no fill)
- [~] Add LED feedback for AUX mode changes (deferred to Task 34):
  - [~] HAT mode unlock: triple rising flash (∙∙∙ ● ● ●)
  - [~] FILL GATE reset: single fade (● ∙)
- [x] Default AUX mode to FILL_GATE on boot
- [x] Remove AUX MODE from config knobs (done in Task 27)
- [x] Add unit tests for gesture detection (11 test cases)
- [x] All tests pass (`make test`)

## Acceptance Criteria

- [x] Hold + Switch UP activates HAT mode
- [x] Hold + Switch DOWN returns to FILL GATE mode
- [x] Switch movement while held doesn't change Perf/Config mode
- [x] Button release after gesture doesn't trigger fill
- [x] AUX mode is FILL_GATE on boot
- [x] Build compiles without errors
- [x] All tests pass (294 test cases, 62214 assertions)

## Implementation Notes

### Gesture Detection State Machine

```cpp
struct ButtonState {
    // ... existing fields ...

    bool auxGestureActive;       // True if Hold+Switch gesture in progress
    bool switchMovedWhileHeld;   // Track switch movement during hold
};
```

### ProcessButtonGestures Updated Signature

```cpp
bool ProcessButtonGestures(bool pressed, bool switchUp, bool prevSwitchUp,
                           uint32_t currentTimeMs, bool anyKnobMoved,
                           AuxMode& auxMode);
```

Returns `true` if switch event was consumed by AUX gesture.

### Files Modified

- `src/Engine/ControlProcessor.h` - Added auxGestureActive, switchMovedWhileHeld to ButtonState, updated signature
- `src/Engine/ControlProcessor.cpp` - Gesture detection logic, switch consumption
- `src/Engine/ControlState.h` - Changed boot default from HAT to FILL_GATE
- `tests/test_aux_gesture.cpp` - New: 11 test cases for gesture detection
- `tests/test_controls.cpp` - Updated shift toggle test
- `tests/test_duopulse_types.cpp` - Updated boot default expectation

### LED Feedback (Deferred to Task 34)

TODO comments added at gesture detection points for:
- HAT mode unlock: triple rising flash
- FILL_GATE reset: single fade

### Constraints

- Gesture must not interfere with normal button/switch operation
- Switch event consumption must be clean (no mode glitches)
- LED feedback must be non-blocking
