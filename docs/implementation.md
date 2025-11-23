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
    *   Assert 0 V/5 V diagnostic pulses on OUT_L/OUT_R each beat to verify the DC-coupled CV path ahead of the accent/hi-hat logic.
    *   Sync both the rear User LED and the front-panel LED (CV_OUT_2/C1) to the beat.
    *   Drive **CV_OUT_1 (C10)** with a 1/16-note master clock pulse alongside the gates.
3.  **Tap Tempo**:
    *   **Button B7** Allows tap tempo if pressed in regular succession.

**Manual Testing Plan:**
*   **Tempo Control**: Turn Knob 4. Verify LED flash rate changes.
*   **Trigger Check**: Patch Gate 1 to BIA (Kick). Verify it triggers reliable kicks.
*   **CV Check**: Monitor OUT_L/OUT_R with a multimeter or scope; should see clean 0/5 V pulses in sync with the gates.
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
    *   Gate 2 (Snare + Hi-hat triggers) -> Tymp Legio (and any multed destinations).
    *   OUT_L (Kick Accents) -> BIA accent/velocity input; confirm it only rises on accented kicks.
    *   OUT_R (Hi-hat CV) -> BIA modulation input; confirm 5 V on hi-hat steps, 0 V on snare steps.

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

#### CV Consistency

- Monitor OUT_L/OUT_R with a scope or multimeter.
- **Expect:**  
  - OUT_L only rises to 5 V on accented kicks while staying at 0 V on normal kicks.
  - OUT_R outputs a steady 5 V gate for hi-hat steps and returns to 0 V on snare steps, matching Gate 2 activity.

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
    *   Drive a per-step random source that perturbs both the Map X/Y coordinates and each channel's trigger probability—low settings add ghost notes, high settings push toward glitch while clamping so primary voices never disappear.
3.  **Accent & Hi-hat CV lanes**:
    *   OUT_L becomes a DC-coupled 0‑5 V accent lane that only goes high on accented Kick steps; it must stay low on normal kicks even if Gate Out 1 fires.
    *   OUT_R becomes a DC-coupled 0‑5 V hi-hat CV lane that outputs 5 V on hi-hat steps and 0 V on snare steps so BIA can differentiate articulations.
    *   Gate Out 2 continues to emit a trigger on both snare and hi-hat steps, keeping downstream percussion locked even while OUT_R only gates the hi-hat CV.
    

**Manual Testing Plan:**
*   **Modulation Clamp Check**: Patch the Ch Svr attenuator (set to ±8 V) into CV 5, sweep fully, and confirm the summed Map X value never exceeds the musical range (sequencer should smoothly hit both extremes but never fold over).
*   **Chaos Characterization**: Raise Knob 3 while keeping CV 7 at mid. Expect subtle ghost notes up to noon, then denser fills beyond 2 o'clock without losing the primary kick/snare backbone.
*   **Accent / Hi-hat CV Routing**: Send OUT_L through an attenuator into BIA's Accent/Velocity input to confirm it only lifts on accented kicks. Route OUT_R through a second attenuator into BIA's Morph/Fold (or other CV) to confirm 5 V appears only on hi-hat steps while staying at 0 V on snares.
*   **BIA Reference Setup**: Suggested baseline—BIA in Skin mode, Pitch ~11 o'clock, Spread ~9 o'clock, Morph ~1 o'clock, Fold ~10 o'clock, Attack minimum, Decay ~1 o'clock. With this, Gate 1 keeps a solid kick while OUT_L handles dynamics and OUT_R sculpts hi-hats without clipping.

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
    *   When Switch is toggled: Knobs 1-4 remap to OUT_L/OUT_R parameters (accent probability, accent gate length, hi-hat probability, hi-hat gate length) per the spec.
3.  **Save/Load** (Optional/Time Permitting):
    *   Save config to flash.

**Manual Testing Plan:**
*   **Mode Switch**: Flip switch. Turn knobs. Verify pattern doesn't change, but OUT_L accent frequency/gate length and OUT_R hi-hat behavior respond immediately.
*   **Return**: Flip switch back. Knobs resume controlling pattern generation.

---

## Phase 6: Verification & Manual QA
Status: TODO
**Goal:** Provide a repeatable "mini manual" for validating drum patterns, clocking, and LED behavior before release.

1.  **Setup Checklist**:
    *   Firmware flashed via `make program`.
    *   Outputs patched: Gate 1 → Kick voice, Gate 2 → Snare voice (and multed to any other percussion), OUT_L → BIA Accent/Velocity input, OUT_R → BIA modulation input (Morph/Fold), CV_OUT_1 → external sequencer/clock divider, CV_OUT_2 LED visible on front panel.
    *   All knobs at noon; no external CV patched unless specified.
2.  **Pattern Recipes**:
    *   **Techno / 4-on-the-floor**: Knob 1 ≈ 12 o'clock, Knob 2 ≈ 9 o'clock. Expect steady kicks on 0/4/8/12 (Gate 1), OUT_L accents appearing on downbeats only, OUT_R sending frequent hi-hat CV bursts with occasional 0 V gaps for snares.
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

### Testing Notes: Recommended BIA & CV Patch
1.  **Primary Triggering**
    *   Mult Gate Out 1 with the Make Noise passive mult so one copy drives BIA's trigger and the other can feed an external clock/reset if needed.
    *   Gate Out 2 feeds Tymp Legio directly; the mult's spare jack can distribute the same trigger to additional voices for cohesion.
2.  **Accent & Hi-hat Distribution**
    *   Route OUT_L through Ch Svr (or similar attenuator) and into BIA's Accent/Velocity jack so 5 V pulses only on accented kicks translate to louder hits.
    *   Route OUT_R through another attenuator into BIA's Morph or Fold CV so hi-hat steps inject 5 V timbral swings while 0 V snare steps keep the tone steady.
3.  **BIA Front-Panel Reference**
    *   Mode: **Skin**, Pitch ~11 o'clock, Decay ~1 o'clock, Attack fully CCW, Spread ~9 o'clock, Harmonics ~10 o'clock, Morph ~1 o'clock, Fold ~10 o'clock.
    *   This baseline leaves headroom so the accent pulses and hi-hat CV swings clearly sculpt timbre without destabilizing the kick fundamental.
4.  **Clock & Accent Routing**
    *   Send CV Out 1 (master clock pulse) to downstream sequencers. If accent is desired, tap the passive mult to feed CV Out 2's LED mirror into external accent inputs.
5.  **Verification Goal**
    *   With the above patching, increasing Knob 3 should audibly shift BIA's tone in tandem with density changes, making it easy to confirm Chaos as well as the accent (OUT_L) and hi-hat (OUT_R) routing in a single listening pass.

---

## Appendix: Grids Data Strategy
Since we are writing from scratch/porting, we will use a simplified version of the Grids map initially if the full compressed tables are too complex to import immediately.
*   *Fallback Plan*: Procedural generation (Euclidean or cellular automaton) if lookup tables are unavailable.
*   *Primary Plan*: Port `drum_pattern_generator.cc` logic from Mutable Instruments codebase (MIT license).

