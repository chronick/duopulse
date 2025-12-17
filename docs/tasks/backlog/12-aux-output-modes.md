---
id: chronick/daisysp-idm-grids-12
title: "Auxiliary Output Modes (CV Out 1)"
status: "pending"
created_date: "2025-12-16"
last_updated: "2025-12-16"
owner: "user/ai"
spec_refs:
  - "aux-output-modes"
depends_on:
  - "08-bulletproof-clock"
---

# Feature: Auxiliary Output Modes [aux-output-modes]

## Context
When external clock is patched, outputting clock on CV_OUT_1 is redundant. Instead, CV_OUT_1 can serve as a multi-purpose auxiliary output with several useful modes: a third trigger (hi-hat/ghost), fill zone indicator, phrase position CV, accent trigger, or downbeat marker. This turns DuoPulse into a more versatile 3-voice system when using external clock.

**Depends on**: Task 08 (Bulletproof Clock) must implement context-aware K4 controls first, which exposes K4+Shift as AUX MODE selector when external clock is patched.

## Tasks

### Core Infrastructure
- [ ] Add `auxOutputMode_` parameter (0.0-1.0) to Sequencer or ControlState. *(spec: [aux-output-modes])*
- [ ] Add `SetAuxOutputMode(float value)` setter. *(spec: [aux-output-modes])*
- [ ] Add `GetAuxOutputMode()` getter that returns enum/mode. *(spec: [aux-output-modes])*
- [ ] Map K4+Shift to auxOutputMode when external clock is patched (in ProcessControls). *(spec: [aux-output-modes])*

### Mode Implementation
- [ ] **Clock Mode** (0-17%): Output swung clock (existing behavior, default). *(spec: [aux-output-modes])*
- [ ] **HiHat Mode** (17-33%): Output ghost triggers from `ChaosModulator::ghostTrigger`. *(spec: [aux-output-modes])*
  - Expose `hhTrig` state from Sequencer or create dedicated output
  - Output 5V trigger pulse on ghost events
- [ ] **Fill Gate Mode** (33-50%): Output high (5V) when in fill/build zone. *(spec: [aux-output-modes])*
  - Use `phrasePos_.isFillZone` or `phrasePos_.isBuildZone`
  - Continuous gate, not trigger
- [ ] **Phrase CV Mode** (50-67%): Output 0-5V ramp based on phrase progress. *(spec: [aux-output-modes])*
  - Use `phrasePos_.phraseProgress` (0.0-1.0) scaled to 0-5V
  - Smooth CV output, not stepped
- [ ] **Accent Mode** (67-83%): Output trigger on accent-eligible hits. *(spec: [aux-output-modes])*
  - Fire when anchor or shimmer triggers with high intensity
  - Use `IsAccentEligible()` check
- [ ] **Downbeat Mode** (83-100%): Output trigger on bar downbeats. *(spec: [aux-output-modes])*
  - Use `phrasePos_.isDownbeat`
  - Option: also fire on phrase reset (step 0 of loop)

### Output Routing
- [ ] Create `GetAuxOutputVoltage()` method that returns appropriate voltage based on mode. *(spec: [aux-output-modes])*
- [ ] Update `AudioCallback` to call `GetAuxOutputVoltage()` and write to CV_OUT_1. *(spec: [aux-output-modes])*
- [ ] Ensure sample-accurate output for trigger modes (HiHat, Accent, Downbeat).

### Internal Clock Fallback
- [ ] When internal clock (no external clock patched), CV_OUT_1 always outputs clock. *(spec: [aux-output-modes])*
- [ ] AUX MODE setting is stored but not applied until external clock is patched.

### Persistence & Feedback
- [ ] Save auxOutputMode to flash (persist across power cycles). *(spec: [aux-output-modes])*
- [ ] LED feedback when adjusting AUX MODE (flash pattern or brightness indicates mode). *(spec: [aux-output-modes])*

### Testing
- [ ] Unit tests for each aux output mode.
- [ ] Test mode switching while sequencer is running.
- [ ] Test persistence across simulated power cycle.

## Notes
- HiHat mode uses existing `ghostTrigger` from ChaosModulator—this data is already generated but currently merged into anchor/shimmer.
- Fill Gate and Phrase CV modes use existing `PhrasePosition` tracking.
- This feature makes DuoPulse a 3-voice sequencer when external clock is used: Anchor, Shimmer, and HiHat/Aux triggers.

## v3 Compatibility
- This task is compatible with v3 algorithmic pulse field changes.
- Ghost trigger source may change in v3 (from ChaosModulator to PulseField), but the output behavior remains the same.
- Phrase position tracking is reused from v2.
