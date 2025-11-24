---
id: chronick/daisysp-idm-grids01"
title: "Phase 4: Core CV & Chaos"
status: "pending"
created_date: "2025-11-24"
last_updated: "2025-11-24"
owner: "user/ai"
---

# Task: Phase 4 - Core CV & Chaos

## Context
Implement the core control summing and chaos logic as defined in `docs/specs/main.md` "Phase 4 scope".

## Requirements
- [ ] **CV Summing**: Hard-map CV Inputs 5-8 to sum with Knobs 1-4 respectively.
- [ ] **Clamping**: Clamp summed result to 0.0-1.0 normalized range.
- [ ] **Chaos Parameter**: Implement Knob 3 + CV 7 as Chaos parameter.
- [ ] **Chaos Logic**:
    - [ ] Modulate per-step random source.
    - [ ] Low values: ghost notes.
    - [ ] High values: perturb Map X/Y and trigger probabilities.
    - [ ] Evaluate once per sequencer step.
    - [ ] Safety clamp: Never mute all voices.
- [ ] **DC-Coupled Outputs**:
    - [ ] OUT_L (Pin B1): Kick Accent gate (default +5V on accent, 0V otherwise).
    - [ ] OUT_R (Pin B2): Hi-hat gate (default +5V on hi-hat, 0V on snare).

## Implementation Plan
1. [ ] Create/Update `Controls` class to handle CV+Knob summing and clamping.
2. [ ] Update `GridsSequencer` (or equivalent) to accept Chaos parameter.
3. [ ] Implement Chaos logic in the sequencer step generation.
4. [ ] Verify Chaos "safety clamp".
5. [ ] Update AudioCallback to handle OUT_L/OUT_R logic based on sequencer state.
6. [ ] Test with mock inputs.

## Notes
- "Perturbations are evaluated once per sequencer step (never per audio sample)".

