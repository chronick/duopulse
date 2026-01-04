---
id: 33
slug: 33-v5-boot-length
title: "V5 Boot-Time Pattern Length Selection"
status: pending
created_date: 2026-01-04
updated_date: 2026-01-04
branch: feature/33-v5-boot-length
spec_refs:
  - "v5-design-final.md#boot-time-configuration"
depends_on:
  - 27
---

# Task 33: V5 Boot-Time Pattern Length Selection

## Objective

Implement boot-time pattern length selection where holding the button during power-on allows selecting 16 or 24 steps via switch position.

## Context

V5 moves pattern length from a config knob to a boot-time gesture:

| Boot Gesture | Result |
|--------------|--------|
| Normal boot | 16 steps (default) |
| Hold button + Switch UP | 16 steps (1 LED flash) |
| Hold button + Switch DOWN | 24 steps (2 LED flashes) |

Key behaviors:
- Pattern length is set at boot and cannot be changed at runtime
- Default is 16 steps (compile-time constant)
- Boot gesture provides 16/24 step options
- Compile-time options exist for 32/64 steps (custom builds)
- LED flashes confirm selection

## Subtasks

- [ ] Add PATTERN_LENGTH compile-time constant with default of 16
- [ ] Add boot detection for button-held state
- [ ] Implement boot gesture detection window (~1 second)
- [ ] Map switch UP to 16 steps, switch DOWN to 24 steps
- [ ] Add LED flash confirmation (1 flash for 16, 2 for 24)
- [ ] Store selected pattern length for runtime use
- [ ] Remove pattern length from config knobs (done in Task 27)
- [ ] Add compile-time flags for 32/64 step builds
- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] Normal boot uses 16 steps
- [ ] Hold button + Switch UP selects 16 steps with 1 LED flash
- [ ] Hold button + Switch DOWN selects 24 steps with 2 LED flashes
- [ ] Pattern length is fixed after boot
- [ ] Compile-time option works for 32/64 steps
- [ ] Build compiles without errors
- [ ] All tests pass

## Implementation Notes

### Boot Sequence

```cpp
// In main.cpp Init()
void DetectBootPatternLength()
{
    // Read initial button state before starting audio
    bool buttonHeld = hw.ReadButton();

    if (buttonHeld) {
        // Wait for switch position to stabilize
        System::Delay(100);

        bool switchUp = hw.ReadSwitch();

        if (switchUp) {
            patternLength_ = 16;
            led_.FlashCount(1);  // Single flash
        } else {
            patternLength_ = 24;
            led_.FlashCount(2);  // Double flash
        }

        // Wait for button release before continuing
        while (hw.ReadButton()) {
            System::Delay(10);
        }
    } else {
        // Normal boot - use compile-time default
        patternLength_ = PATTERN_LENGTH_DEFAULT;
    }
}
```

### Compile-Time Configuration

```cpp
// In config.h
#ifndef PATTERN_LENGTH_DEFAULT
#define PATTERN_LENGTH_DEFAULT 16
#endif

// Custom build options:
// make CFLAGS+="-DPATTERN_LENGTH_DEFAULT=32"
// make CFLAGS+="-DPATTERN_LENGTH_DEFAULT=64"
```

### LED Flash Confirmation

```cpp
void LedIndicator::FlashCount(int count)
{
    for (int i = 0; i < count; ++i) {
        SetBrightness(1.0f);
        System::Delay(100);
        SetBrightness(0.0f);
        System::Delay(100);
    }
    // Final pause before normal operation
    System::Delay(200);
}
```

### Files to Modify

- `src/main.cpp` - Add boot detection logic
- `inc/config.h` - Add PATTERN_LENGTH_DEFAULT constant
- `src/Engine/LedIndicator.cpp` - Add FlashCount method
- `src/Engine/LedIndicator.h` - Add FlashCount declaration
- `Makefile` - Document custom build options

### Constraints

- Boot detection must complete before audio starts
- Button debouncing during boot
- LED feedback must be visible before normal operation

### Risks

- Users may not discover boot gesture
- Button held during normal power-on may surprise users
- Need to document in user guide
