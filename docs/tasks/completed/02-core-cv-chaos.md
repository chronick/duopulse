---
id: 2
slug: core-cv-chaos
title: Phase 4: Core CV & Chaos
status: completed
created_date: 2025-11-24
updated_date: 2025-11-26
completed_date: 2025-11-26
branch: phase-4
---

# Task: Phase 4 - Core CV & Chaos

## Context
Implement the core control summing and chaos logic as defined in `docs/specs/main.md` "Phase 4 scope".

## Requirements
- [x] **CV Summing**: Hard-map CV Inputs 5-8 to sum with Knobs 1-4 respectively.
- [x] **Clamping**: Clamp summed result to 0.0-1.0 normalized range.
- [x] **Chaos Parameter**: Implement Knob 3 + CV 7 as Chaos parameter.
- [x] **Chaos Logic**:
    - [x] Modulate per-step random source.
    - [x] Low values: ghost notes.
    - [x] High values: perturb Map X/Y and trigger probabilities.
    - [x] Evaluate once per sequencer step.
    - [x] Safety clamp: Never mute all voices.
- [x] **DC-Coupled Outputs**:
    - [x] OUT_L (Pin B1): Kick Accent gate (default +5V on accent, 0V otherwise).
    - [x] OUT_R (Pin B2): Hi-hat gate (default +5V on hi-hat, 0V on snare).

## Implementation Plan
1. [x] Create/Update `Controls` class to handle CV+Knob summing and clamping.
2. [x] Update `GridsSequencer` (or equivalent) to accept Chaos parameter.
3. [x] Implement Chaos logic in the sequencer step generation.
4. [x] Verify Chaos "safety clamp".
5. [x] Update AudioCallback to handle OUT_L/OUT_R logic based on sequencer state.
6. [x] Test with mock inputs.

## Notes
- "Perturbations are evaluated once per sequencer step (never per audio sample)".
- Marked as completed via bulk update.
