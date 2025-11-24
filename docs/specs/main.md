# Firmware Specification: Grids-like Performance Drum Controller for Patch.Init()

## Overview
This firmware turns the Daisy Patch.Init() module into a topographic drum sequencer and performance controller, inspired by Mutable Instruments Grids. It is tailored for a specific Eurorack skiff containing BIA, Tymp Legio, and other performance modules.

## Hardware Configuration (Daisy Patch SM)

### Inputs
*   **Knobs 1-4**: Primary performance controls.
*   **CV Inputs 5-8**: Modulation inputs, hard-mapped to sum with Knobs 1-4 respectively. After summing, clamp the result to the normalized 0.0-1.0 control range to keep the Grid lookups stable even under extreme modulation swings.
*   **Button (Pin B7)**: Shift / Function modifier.
*   **Switch (Pin B8)**: Mode toggle (2-position).
*   **Audio In L/R**: Available for CV modulation (if hardware permits DC coupling) or ignored if not feasible.

### Outputs
*   **Gate Out 1 (Pin B5)**: Trigger for BIA (Voice 1).
*   **Gate Out 2 (Pin B6)**: Trigger for Tymp Legio plus shared snare/hi-hat trigger lane.
*   **CV Out 1 (Pin C10)**: Master Clock Output (Pulse).
*   **CV Out 2 (Pin C1 / Front LED)**: Mirrors the rear/User LED state for visual beat feedback on the front panel.
*   **OUT_L (Audio Out L / Pin B1)**: DC-coupled accent gate sourced from the codec output. Firmware self-attenuates the nominal ±9 V codec swing down to ±5 V per `docs/chats/daisy-audio-vs-cv.md`, so accented Kick steps land at a user-programmable gate voltage (default +5 V) while non-accented kicks stay at 0 V.
*   **OUT_R (Audio Out R / Pin B2)**: DC-coupled hi-hat gate with the same ±5 V firmware scaling. Hi-hat steps drive the configured gate voltage (default +5 V), while snare steps force the lane back to 0 V.

## Functional Architecture

### Core Engine: Topographic Drum Sequencer
The core is a port/adaptation of the Grids algorithm (Map X, Map Y, Chaos/Density).
*   **Map X**: Morphs rhythm structure (e.g., Kick/Snare relationship).
*   **Map Y**: Morphs rhythm density/fill type (e.g., Hihat patterns).
*   **Chaos/Random**: Adds probability and variations to the patterns.

### Control Mapping (Default Mode)
"Default mode always denoted by drum hits on light" (LED indication).

| Control | Parameter | Description |
| :--- | :--- | :--- |
| **Knob 1** + **CV 5** | **Map X** | Morphs drum style (Techno -> Tribal -> Glitch -> IDM). |
| **Knob 2** + **CV 6** | **Map Y** | Controls fill density and variations. |
| **Knob 3** + **CV 7** | **Chaos / Chaos Amt** | Introduces randomness/stutter to the pattern. |
| **Knob 4** + **CV 8** | **BPM / Clock Div** | Controls internal clock tempo (if master) or Clock Divider (if slaved). |
| **Button (B7)** | **Shift** | Hold to access secondary functions (e.g., Reset, Pattern Bank). |
| **Switch (B8)** | **Mode Toggle** | Switches between **Performance Mode** (Default) and **Configuration Mode**. |

### Chaos Requirements
*   **Knob 3 + CV 7** modulates a per-step random source. Low values introduce occasional ghost notes; high values perturb both the Map X/Y coordinates and each channel’s trigger probability.
*   Perturbations are evaluated once per sequencer step (never per audio sample) to retain deterministic audio callbacks.
*   Chaos modulation must never mute all voices; clamp its maximum influence so core kick/snare accents remain intelligible.

### Output Assignment
*   **Channel 1 (Kick equivalent)** -> **Gate Out 1** (to BIA).
*   **Kick Accents** -> **OUT_L (Pin B1)**: Sends a 5 V gate only when the current Kick step is flagged as an accent; remains at 0 V during non-accented kicks to leave dynamics intact.
*   **Channel 2 (Snare + Hi-hat lane)** -> **Gate Out 2** (to Tymp Legio and any multed destinations). Every snare and hi-hat step produces the same 10 ms trigger here for downstream percussion.
*   **Hi-hat CV lane** -> **OUT_R (Pin B2)**: Outputs a steady 5 V gate for the duration of each hi-hat step so BIA can receive a DC-coupled control input; drops to 0 V when the sequencer schedules a snare on that lane.
*   **Clock / Accent** -> **CV Out 1 (C10)** (Clock Pulse).

### Configuration Mode (Switch B8 Toggled)
Allows editing the codec-based gate behavior for OUT_L/OUT_R without interrupting the drum engine. The sequencer, gates, and currently playing voltages continue to run while the switch is flipped, so audible transitions stay seamless.
*   **Global rule**: When a gate lane is OFF it must remain at 0 V. When it is ON, firmware maps the control range to ±5 V despite the codec’s larger ±9 V span (see `docs/chats/daisy-audio-vs-cv.md`) so external modules see predictable voltages.
*   **Mode integrity**: Toggling between Performance and Configuration modes never mutes or reinitializes OUT_L/OUT_R; they keep broadcasting the current values while you edit their parameters.
*   **Knob 1 + CV 5**: Sets the OUT_L gate-on voltage (–5 V to +5 V). Useful for dialing how hard BIA’s accent input is struck.
*   **Knob 2 + CV 6**: Sets the OUT_L gate length/shape (short pop vs sustained hold) without altering the gate-on voltage selected above.
*   **Knob 3 + CV 7**: Sets the OUT_R gate-on voltage (–5 V to +5 V) for the hi-hat CV lane.
*   **Knob 4 + CV 8**: Sets the OUT_R gate length/shape.

### Phase Alignment Notes
*   **Phase 4 scope**: CV 5-8 summing with clamps, Chaos parameterization (Knob 3 + CV 7), and the new DC-coupled accent (OUT_L) / hi-hat (OUT_R) logic are mandatory deliverables.
*   **Phase 4.1 bugfix**: Add firmware self-attenuation on the codec L/R paths so any “gate high” state lands between –5 V and +5 V regardless of the codec’s wider ±9 V swing; treat regressions here as blocking bugs for later phases.
*   **Phase 5 scope**: Configuration mode remaps the knobs to OUT_L/OUT_R gate voltages and lengths (±5 V targets, 0 V when off) without breaking the shared Gate Out 2 routing unless an explicit override is implemented.

## Clock System
*   **Internal Clock**: Default. Tempo controlled by Knob 4.
*   **External Clock**: (Optional) If Gate In 1/2 are available, one could be used as Clock In.
    *   *Assumption*: System acts as Master Clock outputting to C10.
*  **Button B7** In addition to shift, Button B7 should support tap tempo if tapped in a particular way.

## Implementation Details

### Signal Flow
1.  **Read Inputs**: ADC poll for Knobs 1-4 and CV 5-8.
2.  **Process Controls**: Apply scaling and summing (Knob + CV). Handle Button/Switch logic.
3.  **Grids Algorithm**:
    *   Update phase based on Clock.
    *   Compute drum states (Gate On/Off) based on X, Y, Chaos.
4.  **Generate Outputs**:
    *   Set Gate Pins (B5, B6) High/Low.
    *   Set DAC (C10) for Clock Pulse.
    *   Update OUT_L/OUT_R codec states by clamping OFF steps to 0 V and mapping ON steps to the configured ±5 V targets set in Configuration Mode, falling back to the +5 V defaults in Performance Mode.
5.  **LED Feedback**: Pulse User LED on Beat/Downbeat.

### Libraries
*   `libDaisy`: Hardware abstraction.
*   `DaisySP`: DSP generation (Envelopes, Oscillators if needed).
*   *Grids Port*: Need to implement or port the relevant Grids logic (likely lookups and interpolation).

## Future Considerations
*   **In L/R**: Could be used as additional modulation sources if mapped to specific parameters (e.g., global accent).

