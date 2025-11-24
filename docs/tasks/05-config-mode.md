---
id: chronick/daisysp-idm-grids05"
title: "Phase 5: Configuration Mode"
status: "pending"
created_date: "2025-11-24"
last_updated: "2025-11-24"
owner: "user/ai"
---

# Task: Phase 5 - Configuration Mode

## Context
Implement the secondary Configuration Mode triggered by Switch B8.

## Requirements
- [ ] **Mode Switching**: Toggle between Performance (Default) and Config mode via Switch B8.
- [ ] **Seamless Audio**: Sequencer and outputs continue running during switch.
- [ ] **Config Controls**:
    - [ ] Knob 1 + CV 5: OUT_L Gate-on Voltage (-5V to +5V).
    - [ ] Knob 2 + CV 6: OUT_L Gate Length/Shape.
    - [ ] Knob 3 + CV 7: OUT_R Gate-on Voltage (-5V to +5V).
    - [ ] Knob 4 + CV 8: OUT_R Gate Length/Shape.
- [ ] **Persistence**: (Optional/Implied) Settings should apply immediately.
- [ ] **Visual Feedback**: LED indication of mode (if specified, or assume Switch state is enough).

## Implementation Plan
1. [ ] Add `SystemState` or `Config` struct to hold gate voltages and lengths.
2. [ ] Update `Controls` to interpret knobs differently based on Switch B8.
3. [ ] Update AudioCallback to use the configured voltages/lengths instead of hardcoded defaults.
4. [ ] Ensure smooth transition (no audio glitches) when toggling switch.

## Notes
- "Toggling ... never mutes or reinitializes OUT_L/OUT_R".

