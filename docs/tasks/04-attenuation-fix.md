---
id: chronick/daisysp-idm-grids04"
title: "Phase 4.1: Attenuation Bugfix"
status: "pending"
created_date: "2025-11-24"
last_updated: "2025-11-24"
owner: "user/ai"
---

# Task: Phase 4.1 - Attenuation Bugfix

## Context
Ensure firmware self-attenuation for codec L/R paths per `docs/specs/main.md`.

## Requirements
- [ ] **Attenuation**: Map logical "Gate High" (+5V target) to correct codec output value to achieve +5V given the ±9V swing.
- [ ] **Range**: Ensure output is strictly within -5V to +5V.
- [ ] **Validation**: Verify using `docs/chats/daisy-audio-vs-cv.md` formulas if available, or theoretical scaling (5/9 approx 0.55).

## Implementation Plan
1. [ ] Implement helper function `VoltageToCodec(float voltage)` which maps -5V..+5V to -1.0..+1.0 (assuming ±5V is the desired range, but codec is ±9V, so ±5V is approx ±0.555 amplitude).
2. [ ] Apply this helper to OUT_L and OUT_R in the audio callback.
3. [ ] Verify 0V maps to 0.0.

## Notes
- "Firmware self-attenuates the nominal ±9V codec swing down to ±5V".

