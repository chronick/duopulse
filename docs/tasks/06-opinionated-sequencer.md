---
id: chronick/daisysp-idm-grids-06
title: "Opinionated Drum Sequencer Implementation"
status: "pending"
created_date: "2025-11-25"
last_updated: "2025-11-25"
owner: "user/ai"
---

# Task: Opinionated Drum Sequencer Implementation

## Context
Implementation of the new "Opinionated Drum Sequencer" spec defined in `docs/specs/main.md`. This supersedes the previous Grids-port approach, focusing on performance controls (Density, Variation) and configuration (Style, Length, Emphasis).

## Requirements

### Control System
- [ ] **Soft Takeover**: Implement a "soft pickup" mechanism for all potentiometers. When switching modes, the parameter value should not jump until the physical knob crosses the stored value.
- [ ] **Mode Switching**:
    - [ ] **Base Mode** (Switch OFF): Knobs control Kick Density, Snare Density, Kick Variation, Snare Variation.
    - [ ] **Config Mode** (Switch ON): Knobs control Style, Pattern Length, Voice Emphasis, Tempo.
    - [ ] **Persistence**: Store parameter states when switching modes so they are preserved (runtime persistence, not necessarily flash yet).

### Sequencer Engine
- [ ] **Density Logic**:
    - [ ] **Kick/Low Density**: Controls density of the low-end track (from sparse to busy).
    - [ ] **Snare/High Density**: Controls density of the high-end track.
- [ ] **Variation/Chaos Logic**:
    - [ ] **Low Variation**: Controls dynamics/velocity variance and probability/fills for Low End.
    - [ ] **High Variation**: Controls dynamics/velocity variance and probability/fills for High End.
- [ ] **Pattern/Style Logic**:
    - [ ] **Style**: Selects base rhythmic feel (morphs or selects between bank of ~32 patterns/algorithms).
    - [ ] **Length**: Sets loop length (1, 2, 4, 8, 16 bars).
    - [ ] **Reset**: Gate In 2 triggers a pattern reset (to step 0).
- [ ] **Voice Emphasis**:
    - [ ] **Routing**: Implement the "Emphasis" control to mix/route tracks to Output 1 (Low) and Output 2 (High).
    - [ ] **Low Emphasis**: Prioritize low-end complexity on Out 1.
    - [ ] **High Emphasis**: Prioritize high-end complexity on Out 2.

### I/O & Hardware
- [ ] **Gate In 1**: Clock Input.
- [ ] **Gate In 2**: Pattern Reset.
- [ ] **Audio Out 1/2**: DC-coupled Trigger/Gate outputs (Velocity scaled).
- [ ] **Gate Out 1/2**: Digital Triggers (Kick, Snare).

## Implementation Plan

### 1. Refactor Engine Architecture
- [x] Create `class Parameter` or `class SoftKnob` to handle soft takeover logic (current value, stored value, locked state).
- [x] Update `Sequencer` interface to accept the new parameter set:
    - `SetLowDensity`, `SetHighDensity`
    - `SetLowVariation`, `SetHighVariation`
    - `SetStyle`, `SetLength`
    - `SetEmphasis`
- [x] Remove old `ProcessControl` method signatures that relied on Map X/Y directly.

### 2. Implement Soft Takeover
- [x] Create `SoftPickup` utility class.
    - `Update(float hardwareValue)` -> returns effective value.
    - `SnapTo(float value)` -> forces value (for mode switches).

### 3. Update Pattern Generation
- [x] Modify `PatternGenerator` to support "Style" selection (could map to different "Nodes" in the Grids data or different algorithms).
- [x] Implement `SetLoopLength(int bars)` logic in `Sequencer`.
- [x] Implement Reset trigger handling.

### 4. Implement Voice Emphasis
- [ ] Create a mixing/routing stage in `Sequencer::ProcessAudio` or a new `OutputMixer` class.
- [ ] Logic:
    - If Emphasis < 0.5: Route Tom/Perc to Out 1 (Kick).
    - If Emphasis > 0.5: Route Tom/Perc to Out 2 (Snare).
    - (Or similar blending logic as per spec).

### 5. Update Main Loop
- [ ] Instantiate `SoftPickup` objects for the 4 knobs.
- [ ] In `ProcessControls`:
    - Read hardware knobs.
    - Check Mode Switch.
    - If Mode Changed:
        - Update `SoftPickup` targets to the stored values of the new mode.
    - Feed hardware values to `SoftPickup`.
    - Apply effective values to `Sequencer`.

### 6. Cleanup
- [ ] Remove unused Grids-port code if it conflicts or is no longer needed (keep `GridsData` if used for Style).

## Notes
- "Style" can map to the original Grids `x` or `y` coordinates if we treat the 2D map as a linear list of styles, or we can implement a new lookup.
- "Density" maps well to the original Grids density parameter.


# Comments
- 2025-11-25: Completed PatternGenerator updates for Style/Density/Chaos mapping. Implemented independent ChaosModulators for Low/High variation. Implemented SetLoopLength and Reset logic. Next up: Voice Emphasis.
