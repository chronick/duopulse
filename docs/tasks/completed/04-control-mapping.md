---
id: 4
slug: control-mapping
title: Phase 4: Full Control Mapping & Chaos
status: completed
created_date: 2025-11-24
updated_date: 2025-11-24
completed_date: 2025-11-24
branch: phase-4
---

# Task: Phase 4 - Full Control Mapping & Chaos

## Context
Complete the control surface as per spec.

## Requirements
- [x] **CV Integration**:
    - [x] Map CV 5-8 to sum with Knobs 1-4.
    - [x] Ensure values clamp correctly (0.0 - 1.0).
- [x] **Chaos Implementation**:
    - [x] Implement Knob 3 (Chaos).
    - [x] Drive a per-step random source.
- [x] **Accent & Hi-hat CV lanes**:
    - [x] OUT_L: DC-coupled accent lane (5V on accent, 0V otherwise).
    - [x] OUT_R: DC-coupled hi-hat CV lane (5V on hi-hat, 0V on snare).
    - [x] Gate Out 2: Triggers on snare and hi-hat.

## Implementation Plan
- [x] Implement CV summing and clamping.
- [x] Implement Chaos logic.
- [x] Update outputs for Accent/Hi-hat CV.

## Notes
- Marked as DONE in `docs/implementation.md`.
- Note: Phase 4.1 (bugfix) is still pending as a separate task.

