# Firmware Specification: Opinionated Drum Sequencer for Patch.Init()

## Overview
This firmware transforms the Daisy Patch.Init() module into an opinionated, performance-oriented drum sequencer. It departs from a direct Grids port to focus on playable, expressive control over drum patterns using the specific constraints of the Patch.Init hardware. The core design philosophy emphasizes smooth transitions, evolving rhythms, and "soft takeover" controls to prevent parameter jumps.

## Hardware Configuration (Daisy Patch SM)

### Inputs
*   **Knobs 1-4**: Multifunction performance controls (depend on Switch state).
*   **CV Inputs 5-8**: Modulation inputs for Knobs 1-4 respectively. CV adds to the knob position, clamped to safe ranges.
*   **Button (Pin B7)**: Shift / Modifier (Exact function TBD).
*   **Switch (Pin B8)**: Mode toggle (2-position: OFF = Base Mode, ON = Config Mode).
*   **Gate In 1 (Pin B3)**: Clock Input (Rising Edge).
*   **Gate In 2 (Pin B4)**: Pattern Reset (Rising Edge).
*   **Audio In L/R**: Ignored.

### Outputs
*   **Audio Out 1 (L / Pin B1)**: Kick/Low-End Output (Trigger/Gate/CV).
*   **Audio Out 2 (R / Pin B2)**: Snare/High-End Output (Trigger/Gate/CV).
*   **Gate Out 1 (Pin B5)**: Kick Trigger (Digital 5V).
*   **Gate Out 2 (Pin B6)**: Snare/Hihat Trigger (Digital 5V).
*   **CV Out 1 (Pin C10)**: TBD (Default: Clock Out?).
*   **CV Out 2 (Pin C1 / Front LED)**: LED Visual Feedback.
    *   **Behavior**:
        *   **Default**: Pulses on beats (Base Mode) or Solid ON (Config Mode).
        *   **Interaction**: When any knob is turned, the LED brightness reflects the *actual parameter value* being modified (dim to bright).
        *   **Timeout**: After 1 second of inactivity, reverts to Default behavior.

## Functional Architecture

The system operates in two distinct modes controlled by the **Switch**. State variables for each mode are persisted when switching, ensuring no parameters are lost.

**Soft Takeover**: When switching modes or startup, turning a knob does not immediately jump the parameter. The system uses "Value Scaling" (similar to Ableton Live): as the user turns the knob, the parameter value actively interpolates towards the physical knob position, scaling the remaining range to ensure smooth convergence without needing to "cross" the current value first.

### Base Mode (Switch OFF)
The primary performance mode, optimized for in-set manipulation of the drum voices.

| Control | Function | Description |
| :--- | :--- | :--- |
| **Knob 1** | **Kick/Tom Density** | Controls the "frequency" or "content" of the low-end track. Moves from sparse to dense/busy. |
| **Knob 2** | **Snare/Hat Density** | Controls the "frequency" or "content" of the high-end track. Moves from sparse to dense/busy. |
| **Knob 3** | **Kick/Tom Variation** | Controls dynamics and chaos for the low-end. Low values = steady velocity. High values = velocity variation + chaos (random ghost notes/fills). |
| **Knob 4** | **Snare/Hat Variation** | Controls dynamics and chaos for the high-end. Low values = steady velocity. High values = velocity variation + chaos. |

*   **CV Inputs** 1-4 modulate their respective Knobs 1-4.

### Config Mode (Switch ON)
Setup, Style, and Routing configuration.

| Control | Function | Description |
| :--- | :--- | :--- |
| **Knob 1** | **Style / Time Sig** | Selects the overall rhythmic feel and time signature (e.g., 4/4 Techno -> Breakbeat -> IDM). |
| **Knob 2** | **Pattern Length** | Sets pattern loop length: 1, 2, 4, 8, or 16 bars. |
| **Knob 3** | **Voice Emphasis** | Controls voice allocation/routing balance (Low/High Emphasis). See below. |
| **Knob 4** | **Tempo** | Sets internal clock tempo (ignored if external clock present). |

### Voice Emphasis (Config Knob 3)
This control adjusts how internal generator tracks are mapped to the two physical outputs, allowing the user to tailor the sequencer to the connected modules (e.g., BIA vs. Tymp Legio).

*   **Low Emphasis (CCW)**: Prioritizes low-end complexity on Output 1. May mix Tom tracks or additional low-perc elements into the Kick channel.
*   **Balanced (Center)**: Standard mapping. Kick -> Out 1, Snare/Hat -> Out 2.
*   **High Emphasis (CW)**: Prioritizes high-end complexity on Output 2. Ensures rich Snare/Hat/Percussion patterns on Out 2, possibly simplifying the Kick track or moving mid-range elements to Out 2.

### Signal Flow & Audio Logic
1.  **Clocking**:
    *   If **Gate In 1** receives pulses, system syncs to External Clock.
    *   Otherwise, runs on Internal Clock (set by Config Knob 4).
2.  **Sequencer Engine**:
    *   Generates triggers based on **Style** (Config K1) and **Density** (Base K1/K2).
    *   Applies **Variation/Chaos** (Base K3/K4) to modify velocity and probability.
    *   Applies **Voice Emphasis** (Config K3) to route triggers to channels.
3.  **Output Generation**:
    *   **Audio Outs**: Generates DC-coupled pulses/gates.
        *   Voltage Level corresponds to Velocity/Accent level.
        *   Config Mode attenuation controls (from previous spec) are removed in favor of Emphasis/Style, assuming fixed 5V or max range output unless re-added. *Correction: User removed explicit attenuation knobs from Config, assuming standard Eurorack levels or fixed scaling.*
    *   **Gate Outs**: Digital mirror of the main triggers (always 0V/5V).

## Implementation Details

### Data Structures
*   **Pattern Bank**: ~32 patterns stored as bitmaps or step sequences.
*   **State Machine**:
    *   `CurrentMode` (Base/Config).
    *   `Params_Base` (KickDensity, SnareDensity, KickVar, SnareVar).
    *   `Params_Config` (Style, Length, Emphasis, Tempo).
    *   `SoftPickup` state for each knob.

## Changelog
*   **2025-11-26**:
    *   **Soft Takeover**: Updated behavior to "Value Scaling" for immediate, smooth response without parameter jumps.
    *   **Visual Feedback**: Added interaction behavior to CV Out 2 LED (reflects knob value on turn, 1s timeout).
*   **2025-11-24**:
    *   Refined control scheme for "in-set" performance.
    *   **Base Mode**: Now split into Low-End (K1/K3) and High-End (K2/K4) pairs controlling Density and Variation/Chaos.
    *   **Config Mode**: Moved Style and Length to Config K1/K2. Added **Voice Emphasis** on K3.
    *   Defined Emphasis logic (Low vs High priority routing).
