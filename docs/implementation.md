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
Status: DONE
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
Status: DONE
**Goal:** Implement the rhythmic backbone.

1.  **Internal Clock Engine**:
    *   Implement a `Metro` or phasor-based clock.
    *   Map **Knob 4** to Tempo (e.g., 30 BPM to 200 BPM).
2.  **Basic Output Triggers**:
    *   Generate short triggers (10ms) on Gate Out 1 & 2 on every beat.
    *   Generate a distinct sound (e.g., high pitch beep) on Audio L/R on the beat.
    *   Sync both the rear User LED and the front-panel LED (CV_OUT_2/C1) to the beat.
    *   Drive **CV_OUT_1 (C10)** with a 1/16-note master clock pulse alongside the gates.
3.  **Tap Tempo**:
    *   **Button B7** Allows tap tempo if pressed in regular succession.

**Manual Testing Plan:**
*   **Tempo Control**: Turn Knob 4. Verify LED flash rate changes.
*   **Trigger Check**: Patch Gate 1 to BIA (Kick). Verify it triggers reliable kicks.
*   **Audio Check**: Listen to Audio L/R. Should hear a "tick".
*   **Clock Out**: Patch CV_OUT_1 to an external sequencer—should see a 1/16 pulse in sync with the LED/gates.


---

## Phase 3: The "Grids" Core (Algorithm Port)
Status: DONE
**Goal:** Port the topographic drum generation logic.

1.  **Data Structure**: *(done)*
    *   Full 25-node Mutable Instruments tables are embedded in `src/Engine/GridsData.*`.
2.  **Coordinate Logic**: *(done)*
    *   Knob 1/2 feed directly into bilinear interpolation inside `PatternGenerator`, matching the original Grids behavior and per-channel densities.
3.  **Sequencer Integration**: *(done)*
    *   The audio clock now queries the Grids map each step, driving gates/audio via the new pattern generator.

**Manual Testing Plan:**
*   **Static Pattern**: Set knobs to 12 o'clock. Should hear a specific breakbeat pattern.
*   **Morphing**: Slowly turn Knob 1 (Map X). Pattern should evolve (e.g., Kick placement changes).
*   **Density**: Slowly turn Knob 2 (Map Y). Pattern should get busier or simpler.
*   **Outputs**:
    *   Gate 1 (Kick) -> BIA.
    *   Gate 2 (Snare) -> Tymp Legio.
    *   Audio L (HH) -> Mixer.

---

### Manual Test Plan (Patch.Init Hardware)

#### Map X Sweep (Knob 1)

- Leave Knob 2 at noon.
- Slowly rotate Knob 1 from minimum to maximum.
- **Expect:**  
  - Kick placement morphs: starts sparse (lower-left) and evolves through four-on-the-floor to dense/jungle.
  - Gate 1 reflects these changes.
  - Gate 2 and hi-hats (HH) also shift but remain musically coherent.

#### Map Y Sweep (Knob 2)

- Set Knob 1 to approximately 0.25.
- Sweep Knob 2 from minimum to maximum.
- **Expect:**  
  - Patterns become busier with higher Y mappings (e.g., Amen/Gabber).
  - Gate 2 (snare) density follows the sweep.
  - Hi-hat output thickens.

#### Clock Sync

- Patch an external tempo CV to Knob 4 (tempo).
- Vary the external CV.
- **Expect:**  
  - LED and gates reliably follow the clock without jitter.
  - Grids map lookup occurs every metro tick.

#### Audio Consistency

- Monitor Audio L/R on a mixer.
- **Expect:**  
  - Each hi-hat strike produces a fast-decay envelope (no audible pops).
  - Rhythmic density in audio matches gate activity.

#### Edge Cases

- Rapidly turn Knob 1 or Knob 2 fully CCW/CW.  
  **Expect:** No silent zones or runaway trigger floods.
- Tap the tempo button rapidly.  
  **Expect:** BPM updates cleanly without pattern glitches.

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

## Phase 6: Verification & Manual QA
Status: TODO
**Goal:** Provide a repeatable "mini manual" for validating drum patterns, clocking, and LED behavior before release.

1.  **Setup Checklist**:
    *   Firmware flashed via `make program`.
    *   Outputs patched: Gate 1 → Kick voice, Gate 2 → Snare voice, Audio L → Mixer (HH), CV_OUT_1 → external sequencer/clock divider, CV_OUT_2 LED visible on front panel.
    *   All knobs at noon; no external CV patched unless specified.
2.  **Pattern Recipes**:
    *   **Techno / 4-on-the-floor**: Knob 1 ≈ 12 o'clock, Knob 2 ≈ 9 o'clock. Expect steady kicks on 0/4/8/12 (Gate 1), moderate hats on Audio L, sparse snares.
    *   **Breakbeat / Hip-Hop**: Knob 1 ≈ 9 o'clock, Knob 2 ≈ 12 o'clock. Expect syncopated kicks and heavy snares (Gate 2) reminiscent of node_11.
    *   **IDM / Glitch**: Knob 1 ≈ 3 o'clock, Knob 2 ≈ 3 o'clock. Expect dense hats and unpredictable fills. Verify chaos knob (Knob 3) adds further variation when raised.
3.  **Clock & LED Verification**:
    *   Patch CV_OUT_1 into an external module and confirm a 1/16 pulse aligned with both rear and front LEDs.
    *   Change tempo (Knob 4 and tap button). External device must track without drift.
4.  **Edge Tests**:
    *   Sweep Knob 1/2 extremes to ensure no silent zones or runaway triggers.
    *   Bypass the internal clock by feeding a steady tap-tempo input; confirm the pattern remains consistent.

**Manual Testing Plan**:
*   Follow each recipe, documenting audible output and LED/clock behavior.
*   If any step deviates, log knob positions, external CV, and observed outputs for debugging before release.

---

## Appendix: Grids Data Strategy
Since we are writing from scratch/porting, we will use a simplified version of the Grids map initially if the full compressed tables are too complex to import immediately.
*   *Fallback Plan*: Procedural generation (Euclidean or cellular automaton) if lookup tables are unavailable.
*   *Primary Plan*: Port `drum_pattern_generator.cc` logic from Mutable Instruments codebase (MIT license).

