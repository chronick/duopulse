---
id: 33
slug: 33-v5-boot-aux-mode
title: "V5 Boot-Time AUX Mode Selection"
status: completed
created_date: 2026-01-04
updated_date: 2026-01-04
completed_date: 2026-01-04
branch: feature/33-v5-boot-aux-mode
spec_refs:
  - "v5-design-final.md#boot-time-configuration"
depends_on:
  - 27
  - 32
---

# Task 33: ~~V5 Boot-Time Pattern Length Selection~~ → V5 Boot-Time AUX Mode Selection

## ~~Original Objective~~ (CANCELLED)

~~Implement boot-time pattern length selection where holding the button during power-on allows selecting 16 or 24 steps via switch position.~~

## ADDENDUM: New Objective

Implement boot-time AUX mode selection using the same Hold+Switch gesture as runtime. AUX mode is **persistent** until explicitly changed.

## Context

~~V5 moves pattern length from a config knob to a boot-time gesture:~~

~~| Boot Gesture | Result |~~
~~|--------------|--------|~~
~~| Normal boot | 16 steps (default) |~~
~~| Hold button + Switch UP | 16 steps (1 LED flash) |~~
~~| Hold button + Switch DOWN | 24 steps (2 LED flashes) |~~

### New Design: Unified AUX Mode Control

AUX mode can be set at **boot OR runtime** using the same gesture:

| When | Gesture | Result |
|------|---------|--------|
| Boot | Hold button + Switch UP | HAT mode + fancy rising flash |
| Boot | Hold button + Switch DOWN | FILL_GATE mode + grounded fade |
| Runtime | Hold button + move Switch | Same as above |
| Normal boot | No button held | Keep previous AUX mode (persistent) |

Key behaviors:
- AUX mode is **persistent** - stays set until explicitly changed
- Same gesture works at boot and runtime (Hold + Switch)
- LED feedback differentiates modes:
  - **HAT**: Fancy rising flash (∙∙∙ ● ● ●)
  - **FILL_GATE**: Grounded single fade (● → ∙)

## Subtasks

### ~~Old Pattern Length Subtasks~~ (CANCELLED)
- ~~Add PATTERN_LENGTH compile-time constant with default of 16~~
- ~~Add boot detection for button-held state~~
- ~~Implement boot gesture detection window (~1 second)~~
- ~~Map switch UP to 16 steps, switch DOWN to 24 steps~~
- ~~Add LED flash confirmation (1 flash for 16, 2 for 24)~~
- ~~Store selected pattern length for runtime use~~
- ~~Remove pattern length from config knobs (done in Task 27)~~
- ~~Add compile-time flags for 32/64 step builds~~

### New AUX Mode Boot Subtasks
- [x] Add boot-time button+switch detection in main.cpp Init()
- [x] Read switch position if button held at boot
- [x] Set AUX mode based on switch: UP=HAT, DOWN=FILL_GATE
- [x] Trigger LED feedback (FlashHatUnlock, FlashFillGateReset)
- [x] Make AUX mode persistent (don't reset on normal boot)
- [x] Wait for button release before starting audio
- [x] Add unit tests for boot AUX detection (test_boot_aux.cpp)
- [x] All tests pass (`make test`) - 303 test cases

## Acceptance Criteria

### ~~Old Criteria~~ (CANCELLED)
- ~~Normal boot uses 16 steps~~
- ~~Hold button + Switch UP selects 16 steps with 1 LED flash~~
- ~~Hold button + Switch DOWN selects 24 steps with 2 LED flashes~~
- ~~Pattern length is fixed after boot~~
- ~~Compile-time option works for 32/64 steps~~

### New Criteria
- [x] Normal boot preserves previous AUX mode (persistent)
- [x] Hold button + Switch UP at boot → HAT mode + fancy flash
- [x] Hold button + Switch DOWN at boot → FILL_GATE mode + grounded flash
- [x] Boot gesture detected before StartAudio()
- [x] LED patterns visually distinct (rising vs fade)
- [x] Build compiles without errors
- [x] All tests pass (303 test cases, 62245 assertions)

## Implementation Notes

### Boot Sequence

```cpp
// In main.cpp Init(), BEFORE audio starts
void DetectBootAuxMode(AuxMode& auxMode)
{
    // Read initial button state
    hw.ProcessAllControls();
    bool buttonHeld = !hw.gate_in_1.State();  // Active low

    if (!buttonHeld) {
        // Normal boot - keep previous AUX mode (persistent)
        return;
    }

    // Button held - wait for switch to stabilize
    System::Delay(100);
    hw.ProcessAllControls();

    bool switchUp = hw.GetSwitch().Read();

    if (switchUp) {
        auxMode = AuxMode::HAT;
        led_.FlashFancyRising();  // Task 34: ∙∙∙ ● ● ●
    } else {
        auxMode = AuxMode::FILL_GATE;
        led_.FlashGroundedFade();  // Task 34: ● → ∙
    }

    // Wait for button release
    while (!hw.gate_in_1.State()) {
        System::Delay(10);
        hw.ProcessAllControls();
    }
}
```

### Persistence

AUX mode is stored in ControlState and persists:
- Across normal boots (no button held)
- Until explicitly changed via Hold+Switch gesture
- Note: Not flash-persisted, just RAM-persistent during session

### LED Flash Patterns (from Task 34)

```cpp
// HAT mode: Fancy rising flash (increasing brightness)
void FlashFancyRising() {
    FlashSequence({0.33f, 0.66f, 1.0f}, 80);  // 3 rising pulses
}

// FILL_GATE mode: Grounded fade (single decay)
void FlashGroundedFade() {
    FadeOut(1.0f, 0.0f, 300);  // Fade from bright to dark
}
```

### Files to Modify

- `src/main.cpp` - Add boot AUX detection before audio starts
- `src/Engine/LedIndicator.cpp` - Add flash patterns (coordinate with Task 34)
- `src/Engine/LedIndicator.h` - Add pattern method declarations
- `tests/test_boot_aux.cpp` - New: boot detection tests

### Constraints

- Boot detection must complete BEFORE audio callback starts
- Flash patterns are blocking (only during boot, never in audio path)
- Reuse Task 32's AuxMode enum and ControlState.auxMode

### Risks

- Users may accidentally trigger boot gesture
- Need to document Hold+Switch gesture in user guide
