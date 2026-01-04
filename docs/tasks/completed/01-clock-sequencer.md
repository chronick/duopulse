---
id: 1
slug: clock-sequencer
title: Phase 2: The Clock & Simple Sequencer
status: completed
created_date: 2025-11-24
updated_date: 2025-11-24
completed_date: 2025-11-24
branch: phase-2
---

# Task: Phase 2 - The Clock & Simple Sequencer

## Context
Implement the rhythmic backbone.

## Requirements
- [x] **Internal Clock Engine**:
    - [x] Implement a `Metro` or phasor-based clock.
    - [x] Map **Knob 4** to Tempo (e.g., 30 BPM to 200 BPM).
- [x] **Basic Output Triggers**:
    - [x] Generate short triggers (10ms) on Gate Out 1 & 2 on every beat.
    - [x] Assert 0V/5V diagnostic pulses on OUT_L/OUT_R.
    - [x] Sync LEDs to the beat.
    - [x] Drive **CV_OUT_1 (C10)** with a 1/16-note master clock pulse.
- [x] **Tap Tempo**:
    - [x] **Button B7** allows tap tempo.

## Implementation Plan
- [x] Implement `Clock` class.
- [x] Map Knob 4 to tempo.
- [x] Implement basic triggers.
- [x] Implement tap tempo.

## Notes
- Completed as per `docs/implementation.md`.

