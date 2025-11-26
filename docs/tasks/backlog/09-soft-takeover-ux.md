---
id: "chronick/daisysp-idm-grids-09"
title: "Improve Soft Takeover and Visual Feedback"
status: "pending"
created_date: "2025-11-26"
last_updated: "2025-11-26"
owner: "chronick"
---

# Task: Improve Soft Takeover and Visual Feedback

## Context
Refines the "Soft Takeover" behavior to use Value Scaling (active interpolation) instead of simple catch-up, and adds visual feedback via the CV Out 2 LED when knobs are turned.
See `docs/specs/main.md` -> "Soft Takeover" and "CV Out 2".

## Requirements
- [ ] **Value Scaling**: `SoftKnob` class should implement active scaling logic:
    - If locked, turning the knob towards the target value should scale the output value towards the knob's position proportionally.
    - Alternatively, rescale the remaining knob range to the remaining parameter range.
- [ ] **Interaction Detection**: System needs to know *when* a knob is being turned to trigger the LED override.
- [ ] **LED Feedback**:
    - When a knob is turned, `CV Out 2` LED brightness = Current Parameter Value.
    - This overrides the default (Beat Pulse / Config Mode Solid) behavior.
    - **Timeout**: 1 second after last movement, revert to default behavior.

## Implementation Plan
1. [ ] **Update `SoftKnob` Class**:
    - Add `ProcessScaling(float raw_value)` or update `Process` to handle the scaling logic.
    - Need to store `lock_reference_knob_` and `lock_reference_value_` (the point where we started turning or the initial discrepancy) to calculate the scale factor?
    - Or use a simpler approach: `value += (raw_delta * scale_factor)`.
    - Add `IsChanged()` or `GetLastActivityTime()` to help with LED timeout logic.
2. [ ] **Implement LED State Machine**:
    - In `main.cpp` or a new `UiController` class.
    - Track `last_knob_activity_time`.
    - If `now - last_knob_activity_time < 1000ms`:
        - LED Brightness = `ActiveKnob->GetValue()`.
    - Else:
        - LED Brightness = Default (Sequencer Beat or Config Status).
3. [ ] **Integrate**:
    - Update `AudioCallback` or Main Loop to feed knob values to `SoftKnobs` and update LED DAC.

## Notes
- Ableton Value Scaling formula: 
  `Value = OldValue + (NewKnob - OldKnob) * (TargetLimit - OldValue) / (KnobLimit - OldKnob)`?
  - Basically, if we are at `V` and knob is at `K`, and we are moving towards limit `L` (0 or 1), we scale our movement so that when `K` hits `L`, `V` hits `L`.
- Need to handle direction changes gracefully.

