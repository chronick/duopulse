---
id: 6
slug: config-mode
title: Phase 5: Configuration Mode
status: completed
created_date: 2025-11-24
updated_date: 2025-11-26
completed_date: 2025-11-26
branch: phase-5
---

# Task: Phase 5 - Configuration Mode

## Context
Implement the secondary Configuration Mode triggered by Switch B8.

## Requirements
- [x] **Mode Switching**: Toggle between Performance (Default) and Config mode via Switch B8.
- [x] **Seamless Audio**: Sequencer and outputs continue running during switch.
- [x] **Config Controls**:
    - [x] Knob 1 + CV 5: OUT_L Gate-on Voltage (-5V to +5V).
    - [x] Knob 2 + CV 6: OUT_L Gate Length/Shape.
    - [x] Knob 3 + CV 7: OUT_R Gate-on Voltage (-5V to +5V).
    - [x] Knob 4 + CV 8: OUT_R Gate Length/Shape.
- [x] **Persistence**: (Optional/Implied) Settings should apply immediately.
- [x] **Visual Feedback**: LED indication of mode (if specified, or assume Switch state is enough).

## Implementation Plan
1. [x] Add `SystemState` or `Config` struct to hold gate voltages and lengths.
2. [x] Update `Controls` to interpret knobs differently based on Switch B8.
3. [x] Update AudioCallback to use the configured voltages/lengths instead of hardcoded defaults.
4. [x] Ensure smooth transition (no audio glitches) when toggling switch.

## Notes
- "Toggling ... never mutes or reinitializes OUT_L/OUT_R".
- Marked as completed via bulk update.
