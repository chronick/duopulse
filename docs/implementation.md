# Implementation Plan: Grids for Patch.Init()

This plan outlines the iterative development of the Grids-style sequencer for the Daisy Patch.Init() module. Each phase is designed to be implemented and manually tested in isolation before moving to the next.

**Documentation**
- Refer to all files in docs/ as needed
- Refer to doc/spec.md for overall firmware spec
- Refer to daisysp docs online when necessary.

Online resources:
- https://forum.electro-smith.com/
- https://daisy.audio/
- https://electro-smith.github.io/DaisySP/index.html
- https://github.com/electro-smith/libDaisy
- https://github.com/electro-smith/DaisySP

**Constraint Checklist & Confidence Score:**
1. Hardware: Daisy Patch.Init() (using `DaisyPatchSM`)? Yes.
2. No Oscilloscope? Yes (Plan relies on LED/Audio feedback).
3. Manual Testing? Yes (Eurorack context).
4. Confidence Score: 5/5

**Testing**
- Create comprehensive unit tests for each feature
- Use Test-driven-development (TDD) as much as is reasonably possible.
- Ensure all tests pass by iteratively running tests and fixing issues.
- Attempt to compile a build frequently after each iteration.
- When necessary, you may prompt me to flash the firmware and test.

**Naming Conventions**
- Implement most functionality in src/main.cpp
- Don't create "phase_one, phase_two" nomenclature, build off what exists now.

---

## Phase 1: Hardware Setup & "Hello World"
Status: TODO
**Goal:** Verify toolchain, correct pin mappings for Patch.Init(), and basic I/O.
*Note: The current `main.cpp` uses `DaisyPatch` (Big module) instead of `DaisyPatchSM`. This phase fixes that.*

1.  **Refactor `main.cpp`**:
    *   Switch include to `daisy_patch_sm.h`.
    *   Instantiate `DaisyPatchSM`.
    *   Initialize basic IO (Audio, ADC, Gate Outs).
2.  **Test Pattern Implementation**:
    *   **Loop**: Blink the User LED (Pin B7/B8 or onboard) at 1Hz.
    *   **Audio**: Output a simple sine wave on Out L and Out R.
    *   **Gates**: Toggle Gate Out 1 and 2 every second (alternating).
    *   **CV Out**: Ramp CV Out 1 from 0V to 5V.

**Manual Testing Plan (No Scope):**
*   **Visual**: Verify User LED blinks. Connect LED patch cable to Gate Out 1 & 2; observe alternating blinking.
*   **Audio**: Patch Out L & R to mixer; hear constant tone.
*   **Controls**: Patch CV Out 1 to a VCO pitch input; hear rising siren sound.

---

## Phase 2: The Clock & Simple Sequencer
Status: TODO
**Goal:** Implement the rhythmic backbone.

1.  **Internal Clock Engine**:
    *   Implement a `Metro` or phasor-based clock.
    *   Map **Knob 4** to Tempo (e.g., 30 BPM to 200 BPM).
2.  **Basic Output Triggers**:
    *   Generate short triggers (10ms) on Gate Out 1 & 2 on every beat.
    *   Generate a distinct sound (e.g., high pitch beep) on Audio L/R on the beat.
    *   Sync User LED to the beat.
3.  **Tap Tempo**:
    *   **Button B7** Allows tap tempo if pressed in regular succession.

**Manual Testing Plan:**
*   **Tempo Control**: Turn Knob 4. Verify LED flash rate changes.
*   **Trigger Check**: Patch Gate 1 to BIA (Kick). Verify it triggers reliable kicks.
*   **Audio Check**: Listen to Audio L/R. Should hear a "tick".

---

## Phase 3: The "Grids" Core (Algorithm Port)
Status: TODO
**Goal:** Port the topographic drum generation logic.

1.  **Data Structure**:
    *   Import/Create the Grids lookup tables (Map X/Y data). *Note: This is the hardest part, requires porting the compressed data tables.*
2.  **Coordinate Logic**:
    *   Implement the logic to read Map X (Knob 1) and Map Y (Knob 2).
    *   Implement the interpolation/read logic to determine drum density for 3 channels (Kick, Snare, HH).
3.  **Sequencer Integration**:
    *   Replace simple metronome with Grids coordinate lookup.
    *   On clock tick: Read Grid state -> Trigger Outputs.

**Manual Testing Plan:**
*   **Static Pattern**: Set knobs to 12 o'clock. Should hear a specific breakbeat pattern.
*   **Morphing**: Slowly turn Knob 1 (Map X). Pattern should evolve (e.g., Kick placement changes).
*   **Density**: Slowly turn Knob 2 (Map Y). Pattern should get busier or simpler.
*   **Outputs**:
    *   Gate 1 (Kick) -> BIA.
    *   Gate 2 (Snare) -> Tymp Legio.
    *   Audio L (HH) -> Mixer.

---

## Phase 4: Full Control Mapping & Chaos
Status: TODO
**Goal:** Complete the control surface as per spec.

1.  **CV Integration**:
    *   Map CV 5-8 to sum with Knobs 1-4.
    *   Ensure values clamp correctly (0.0 - 1.0).
2.  **Chaos Implementation**:
    *   Implement Knob 3 (Chaos).
    *   Add random perturbation to the X/Y readout coordinates or trigger probabilities.
3.  **Audio Output Envelopes**:
    *   Instead of simple "ticks" on Audio L/R, implement configurable AD envelopes (or simple decay) for Hi-Hats.

**Manual Testing Plan:**
*   **Modulation**: Patch an LFO to CV 5 (Map X). Hear the beat constantly morphing.
*   **Chaos**: Turn Knob 3 up. Pattern should have "ghost notes" or variations.
*   **Audio Quality**: Listen to Audio L/R. Should sound like percussive clicks/hats rather than digital pops.

---

## Phase 5: Secondary Modes & Polish
Status: TODO
**Goal:** Add "Shift" functions and configuration.

1.  **UI Logic**:
    *   Implement Button (B7) and Switch (B8) reading. 
    *   State machine: Performance Mode vs. Config Mode.
    *   Tap-Tempo: Retain support for tap-tempo introduced in Phase 2 while in performance mode.
    *   LED Light feedback: button remains lit while in config mode. In performance mode, blinks on every hit
2.  **Config Mode**:
    *   When Switch is toggled: Knobs 1-4 control Envelope params (Decay, Pitch, etc.) for Audio L/R.
3.  **Save/Load** (Optional/Time Permitting):
    *   Save config to flash.

**Manual Testing Plan:**
*   **Mode Switch**: Flip switch. Turn knobs. Verify pattern doesn't change, but sound character (decay) of Audio L/R changes.
*   **Return**: Flip switch back. Knobs resume controlling pattern generation.

---

## Appendix: Grids Data Strategy
Since we are writing from scratch/porting, we will use a simplified version of the Grids map initially if the full compressed tables are too complex to import immediately.
*   *Fallback Plan*: Procedural generation (Euclidean or cellular automaton) if lookup tables are unavailable.
*   *Primary Plan*: Port `drum_pattern_generator.cc` logic from Mutable Instruments codebase (MIT license).

