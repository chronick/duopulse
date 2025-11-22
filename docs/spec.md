# Firmware Specification: Grids-like Performance Drum Controller for Patch.Init()

## Overview
This firmware turns the Daisy Patch.Init() module into a topographic drum sequencer and performance controller, inspired by Mutable Instruments Grids. It is tailored for a specific Eurorack skiff containing BIA, Tymp Legio, and other performance modules.

## Hardware Configuration (Daisy Patch SM)

### Inputs
*   **Knobs 1-4**: Primary performance controls.
*   **CV Inputs 5-8**: Modulation inputs, hard-mapped to sum with Knobs 1-4 respectively.
*   **Button (Pin B7)**: Shift / Function modifier.
*   **Switch (Pin B8)**: Mode toggle (2-position).
*   **Audio In L/R**: Available for CV modulation (if hardware permits DC coupling) or ignored if not feasible.

### Outputs
*   **Gate Out 1 (Pin B5)**: Trigger for BIA (Voice 1).
*   **Gate Out 2 (Pin B6)**: Trigger for Tymp Legio (Voice 2).
*   **CV Out 1 (Pin C10)**: Master Clock Output (Pulse).
*   **Audio Out L (Pin B1)**: Trigger Output 3 (configurable envelope/trigger).
*   **Audio Out R (Pin B2)**: Trigger Output 4 (configurable envelope/trigger).
    *   *Note*: Audio outputs are typically AC coupled. Triggers will be short pulses. Envelopes must be percussive.

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

### Output Assignment
*   **Channel 1 (Kick equivalent)** -> **Gate Out 1** (to BIA).
*   **Channel 2 (Snare equivalent)** -> **Gate Out 2** (to Tymp Legio).
*   **Channel 3 (HH equivalent)** -> **Audio Out L** (Trigger/Env).
*   **Clock / Accent** -> **CV Out 1 (C10)** (Clock Pulse).
*   **Channel 4 (or Accent 2)** -> **Audio Out R** (Trigger/Env).

### Configuration Mode (Switch B8 Toggled)
Allows adjusting the "configurable envelopes" for Audio Outputs L/R.
*   **Knob 1**: Attack/Decay shape for Out L.
*   **Knob 2**: Attack/Decay shape for Out R.
*   **Knob 3**: Probability for Out L.
*   **Knob 4**: Probability for Out R.

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
    *   Generate Audio Buffer for Out L/R (Short 1ms-5ms pulses or synthesized envelopes if acting as CV).
5.  **LED Feedback**: Pulse User LED on Beat/Downbeat.

### Libraries
*   `libDaisy`: Hardware abstraction.
*   `DaisySP`: DSP generation (Envelopes, Oscillators if needed).
*   *Grids Port*: Need to implement or port the relevant Grids logic (likely lookups and interpolation).

## Future Considerations
*   **In L/R**: Could be used as additional modulation sources if mapped to specific parameters (e.g., global accent).

