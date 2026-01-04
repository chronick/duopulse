---
id: 7
slug: opinionated-sequencer
title: Opinionated Drum Sequencer Implementation
status: completed
created_date: 2025-11-25
updated_date: 2025-11-25
completed_date: 2025-11-25
branch: feature/opinionated-sequencer
---

# Task: Opinionated Drum Sequencer Implementation

## Context
Implementation of the new "Opinionated Drum Sequencer" spec defined in `docs/specs/main.md`. This supersedes the previous Grids-port approach, focusing on performance controls (Density, Variation) and configuration (Style, Length, Emphasis).

## Requirements

### Control System
- [x] **Soft Takeover**: Implement a "soft pickup" mechanism for all potentiometers. When switching modes, the parameter value should not jump until the physical knob crosses the stored value.
- [x] **Mode Switching**:
    - [x] **Base Mode** (Switch OFF): Knobs control Kick Density, Snare Density, Kick Variation, Snare Variation.
    - [x] **Config Mode** (Switch ON): Knobs control Style, Pattern Length, Voice Emphasis, Tempo.
    - [x] **Persistence**: Store parameter states when switching modes so they are preserved (runtime persistence, not necessarily flash yet).

### Sequencer Engine
- [x] **Density Logic**:
    - [x] **Kick/Low Density**: Controls density of the low-end track (from sparse to busy).
    - [x] **Snare/High Density**: Controls density of the high-end track.
- [x] **Variation/Chaos Logic**:
    - [x] **Low Variation**: Controls dynamics/velocity variance and probability/fills for Low End.
    - [x] **High Variation**: Controls dynamics/velocity variance and probability/fills for High End.
- [x] **Pattern/Style Logic**:
    - [x] **Style**: Selects base rhythmic feel (morphs or selects between bank of ~32 patterns/algorithms).
    - [x] **Length**: Sets loop length (1, 2, 4, 8, 16 bars).
    - [x] **Reset**: Gate In 2 triggers a pattern reset (to step 0).
- [x] **Voice Emphasis**:
    - [x] **Routing**: Implement the "Emphasis" control to mix/route tracks to Output 1 (Low) and Output 2 (High).
    - [x] **Low Emphasis**: Prioritize low-end complexity on Out 1.
    - [x] **High Emphasis**: Prioritize high-end complexity on Out 2.

### I/O & Hardware
- [x] **Gate In 1**: Clock Input.
- [x] **Gate In 2**: Pattern Reset.
- [x] **Audio Out 1/2**: DC-coupled Trigger/Gate outputs (Velocity scaled).
- [x] **Gate Out 1/2**: Digital Triggers (Kick, Snare).

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
- [x] Create a mixing/routing stage in `Sequencer::ProcessAudio` or a new `OutputMixer` class.
- [x] Logic:
    - If Emphasis < 0.5: Route Tom/Perc to Out 1 (Kick).
    - If Emphasis > 0.5: Route Tom/Perc to Out 2 (Snare).
    - (Or similar blending logic as per spec).

### 5. Update Main Loop
- [x] Instantiate `SoftPickup` objects for the 4 knobs.
- [x] In `ProcessControls`:
    - [x] Read hardware knobs.
    - [x] Check Mode Switch.
    - [x] If Mode Changed:
        - [x] Update `SoftPickup` targets to the stored values of the new mode.
    - [x] Feed hardware values to `SoftPickup`.
    - [x] Apply effective values to `Sequencer`.

### 6. Cleanup
- [x] Remove unused Grids-port code if it conflicts or is no longer needed (keep `GridsData` if used for Style).

## Notes
- "Style" can map to the original Grids `x` or `y` coordinates if we treat the 2D map as a linear list of styles, or we can implement a new lookup.
- "Density" maps well to the original Grids density parameter.


# Comments
- 2025-11-25: Completed PatternGenerator updates for Style/Density/Chaos mapping. Implemented independent ChaosModulators for Low/High variation. Implemented SetLoopLength and Reset logic. Next up: Voice Emphasis.
- 2025-11-25: Implemented Voice Emphasis routing. HH/Perc now routes to Low (Kick) or High (Snare) based on Emphasis control. Audio Outs now output velocity-scaled pulses for all triggered events on their respective channels.
- 2025-11-25: Implemented Main Loop with Soft Pickup/Takeover logic. Mode switching now preserves parameter values. Removed obsolete ConfigMapper and legacy gate control logic. Hooked up Reset trigger (Gate In 2). Verified build and tests.
- 2025-11-25: Implemented External Clock support (Gate In 1) with auto-switching and timeout logic. Added unit tests for external clock integration.
