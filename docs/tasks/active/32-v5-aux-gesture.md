---
id: 32
slug: 32-v5-aux-gesture
title: "V5 Hold+Switch Gesture for AUX Mode Selection"
status: pending
created_date: 2026-01-04
updated_date: 2026-01-04
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
- AUX mode defaults to FILL GATE on boot (not persisted)

This is the "secret mode" that makes HAT a discoverable feature.

## Subtasks

- [ ] Add AUX mode state to ButtonState (HAT or FILL_GATE only)
- [ ] Detect switch movement while button is held
- [ ] Implement gesture priority:
  - [ ] Cancel pending shift mode activation
  - [ ] Cancel pending live fill
  - [ ] Set AUX mode based on switch direction
  - [ ] Consume switch event (don't change mode)
  - [ ] Mark button release as gesture-consumed (no fill)
- [ ] Add LED feedback for AUX mode changes:
  - [ ] HAT mode unlock: triple rising flash (∙∙∙ ● ● ●)
  - [ ] FILL GATE reset: single fade (● ∙)
- [ ] Default AUX mode to FILL_GATE on boot
- [ ] Remove AUX MODE from config knobs (done in Task 27)
- [ ] Add unit tests for gesture detection
- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] Hold + Switch UP activates HAT mode with LED feedback
- [ ] Hold + Switch DOWN returns to FILL GATE mode with LED feedback
- [ ] Switch movement while held doesn't change Perf/Config mode
- [ ] Button release after gesture doesn't trigger fill
- [ ] AUX mode is FILL_GATE on boot
- [ ] Build compiles without errors
- [ ] All tests pass

## Implementation Notes

### Gesture Detection State Machine

```cpp
struct ButtonState {
    // ... existing fields ...

    bool auxGestureActive;       // True if Hold+Switch gesture in progress
    bool switchMovedWhileHeld;   // Track switch movement during hold
    AuxMode pendingAuxMode;      // Mode to set on gesture completion
};

void ProcessButtonGestures(bool pressed, bool switchUp, bool prevSwitchUp,
                           uint32_t currentTimeMs, bool anyKnobMoved)
{
    // Detect switch movement while button is held
    if (buttonState_.pressed && (switchUp != prevSwitchUp)) {
        // Switch moved while button held - this is the AUX gesture
        buttonState_.auxGestureActive = true;
        buttonState_.switchMovedWhileHeld = true;

        // Cancel pending operations
        buttonState_.liveFillActive = false;
        buttonState_.tapDetected = false;

        // Set AUX mode based on switch direction
        if (switchUp) {
            buttonState_.pendingAuxMode = AuxMode::HAT;
            // Queue triple rising flash for LED
        } else {
            buttonState_.pendingAuxMode = AuxMode::FILL_GATE;
            // Queue single fade for LED
        }

        // Consume switch event - don't pass to mode selection
        return;  // Don't process normal switch logic
    }

    // On button release after gesture
    if (!pressed && buttonState_.auxGestureActive) {
        // Apply AUX mode change
        state.auxMode = buttonState_.pendingAuxMode;

        // Reset gesture state
        buttonState_.auxGestureActive = false;
        buttonState_.switchMovedWhileHeld = false;

        // Don't trigger fill (gesture consumed the press)
        buttonState_.tapDetected = false;
        buttonState_.doubleTapDetected = false;
    }
}
```

### LED Feedback Patterns

```cpp
// HAT mode unlock: triple rising flash (increasing brightness)
void QueueHatUnlockFlash(LedIndicator& led) {
    led.QueueFlash(0.33f, 50);   // First dim
    led.QueueFlash(0.66f, 50);   // Second medium
    led.QueueFlash(1.00f, 100);  // Third bright
}

// FILL GATE reset: single fade
void QueueFillGateResetFlash(LedIndicator& led) {
    led.QueueFade(1.0f, 0.0f, 200);  // Fade out
}
```

### Files to Modify

- `src/Engine/ControlProcessor.cpp` - Add gesture detection in ProcessButtonGestures
- `src/Engine/ControlProcessor.h` - Add auxGestureActive to ButtonState
- `src/Engine/LedIndicator.cpp` - Add flash patterns for AUX mode changes
- `src/Engine/LedIndicator.h` - Add QueueFlash/QueueFade methods
- `src/main.cpp` - Wire switch state to button processor
- `tests/ControlProcessorTest.cpp` - Add gesture detection tests

### Constraints

- Gesture must not interfere with normal button/switch operation
- Switch event consumption must be clean (no mode glitches)
- LED feedback must be non-blocking

### Risks

- Gesture timing window may need tuning
- Users may accidentally trigger gesture
- Switch debouncing interaction with gesture detection
