# Double Down Sequencer Specification

## Overview
"Double Down" is an opinionated 2-track drum sequencer designed specifically for the Patch.Init() hardware limitations (2 Gate Outs, 2 CV Outs). It abandons the generic 3-track Grids emulation in favor of a focused "Low/Drive" vs "High/Expressive" architecture.

## Philosophy
- **Two Voices are Life**: We maximize the potential of two channels rather than compromising for three.
- **Track 1 (Low)**: The driving force. Kick, Bass pulse, Low Tom. Grounded, heavy.
- **Track 2 (High)**: The expression. Snare, Hi-hat, Percussion, Glitch. Syncopated, evolving.
- **Expressive CV**: CV outputs are not just afterthoughts; they are integral performance lanes (Velocity, Pitch, or Timbre).

## Hardware Interface
- **Switch Down (Performance Mode)**: Direct control over the rhythm and feel.
- **Switch Up (Config Mode)**: Structural settings and setup.
- **Button (Shift)**: Accesses secondary parameters for the current mode.
- **Gate In 1**: Clock
- **Gate In 2**: Reset
- **CV In 1-4**: Summed with Knobs 1-4.

## Control Mapping

### Performance Mode (Switch Down)

| Knob | Shift | Function | Description |
| :--- | :--- | :--- | :--- |
| **1** | Off | **Low Density** | Increases event density for Track 1. From sparse kick to driving pulse. |
| **2** | Off | **High Density** | Increases event density for Track 2. From backbeat to busy percussion. |
| **3** | Off | **X/Y Morph** | Traverses the rhythmic landscape (Style/Map). A macro for navigating the Grids map efficiently. |
| **4** | Off | **Chaos/Fill** | Increases probability of fills, ghost notes, and parameter drift. |
| **1** | **On** | **Low Range** | Scales the CV Output 1 range (e.g., Velocity depth or Pitch range). |
| **2** | **On** | **High Range** | Scales the CV Output 2 range (e.g., Velocity depth or Pitch range). |
| **3** | **On** | **Swing** | Global swing amount (50% to 75%). |
| **4** | **On** | **Ratchet** | Probability of ratcheting/retriggering events (mostly on High track). |

### Config Mode (Switch Up)

| Knob | Shift | Function | Description |
| :--- | :--- | :--- | :--- |
| **1** | Off | **Tempo** | Internal clock tempo (30-240 BPM). |
| **2** | Off | **Length** | Pattern loop length (1, 2, 4, 8, 16, 32 bars). |
| **3** | Off | **CV Mode** | Selects behavior for CV Outputs (see CV Modes below). |
| **4** | Off | **Map Select** | Selects different underlying drum maps/algorithms (e.g., Techno, Breakbeat, Euclidean). |
| **1** | **On** | **Clock Div** | Clock divider/multiplier for External Clock input. |
| **2** | **On** | **Humanize** | Adds micro-timing jitter to triggers. |
| **3** | **On** | **Prob. Skip** | Probability of a step being skipped (for evolving polymeters). |
| **4** | **On** | **Root Note** | Base voltage/root note for Pitch CV modes. |

## CV Output Modes (Config Knob 3)
The CV Outputs allow the sequencer to control more than just timing.
1.  **Velocity (Default)**: CV tracks step velocity/accent level.
2.  **Decay Envelope**: CV generates a trigger-synced decay envelope (internal software envelope).
3.  **Pitch Sequence**: CV outputs quantized pitch values based on the drum map's "timbre" data (simple basslines).
4.  **Random**: CV outputs a random voltage on every trigger (Sample & Hold).

## LED Feedback
- **Front LED**: Pulses on Beat 1 of the measure.
- **Gate LEDs**: Indicate active triggers.
- **Mode Indication**: When Shift is held, Front LED changes color or blink rate (if hardware allows) or CV LEDs indicate shift state.

## Implementation Details
- **Soft Takeover**: All knobs use soft pickup to prevent jumps when switching modes or shift states.
- **Persistence**: Configuration settings (Tempo, CV Mode, Map) should persist or remain stable during Performance changes.
- **Grids Engine**: The underlying engine is still Grids-based but the coordinate mapping is simplified to a single "Morph" parameter for ease of use.

