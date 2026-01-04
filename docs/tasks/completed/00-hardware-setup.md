---
id: 0
slug: hardware-setup
title: Phase 1: Hardware Setup & Hello World
status: completed
created_date: 2025-11-24
updated_date: 2025-11-24
completed_date: 2025-11-24
branch: phase-1
---

# Task: Phase 1 - Hardware Setup & Hello World

## Context
Verify toolchain, correct pin mappings for Patch.Init(), and basic I/O.

## Requirements
- [x] **Refactor `main.cpp`**:
    - [x] Switch include to `daisy_patch_sm.h`.
    - [x] Instantiate `DaisyPatchSM`.
    - [x] Initialize basic IO (Audio, ADC, Gate Outs).
- [x] **Test Pattern Implementation**:
    - [x] **Loop**: Blink the User LED (Pin B7/B8 or onboard) at 1Hz.
    - [x] **Audio**: Output a simple sine wave on Out L and Out R.
    - [x] **Gates**: Toggle Gate Out 1 and 2 every second (alternating).
    - [x] **CV Out**: Ramp CV Out 1 from 0V to 5V.

## Implementation Plan
- [x] Refactor `main.cpp` to use `DaisyPatchSM`.
- [x] Implement test pattern.
- [x] Verify basic I/O.

## Notes
- Completed as per `docs/implementation.md`.

