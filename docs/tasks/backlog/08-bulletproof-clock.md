---
id: chronick/daisysp-idm-grids-08
title: "Bulletproof Clock & External Clock Behavior"
status: "pending"
created_date: "2025-11-25"
last_updated: "2025-12-16"
owner: "user/ai"
spec_refs:
  - "external-clock-behavior"
---

# Task: Bulletproof Clock & External Clock Behavior

## Context
To ensure the sequencer syncs tightly with external equipment, we need to improve the clock handling. Currently, `CV_OUT_1` (Clock Out) is updated in `ProcessControls` (~1kHz), which introduces jitter. It should be updated in `AudioCallback` (48kHz) for sample-accurate timing. Additionally, when external clock is patched, K4 controls should become context-aware: TEMPO becomes CLOCK DIV (for sequencer rate), and K4+Shift becomes AUX MODE (freeing up for auxiliary output selection).

## Requirements

### Clock Output (CV_OUT_1)
- [ ] **Sample-Accurate Output**: Move the `CV_OUT_1` update from `ProcessControls()` to `AudioCallback()`. *(spec: [external-clock-behavior])*
- [ ] **Pulse Width Control**: Ensure the output pulse width is consistent (e.g., 10ms or configurable) regardless of block size or control rate.

### Clock Input (Gate In 1)
- [ ] **Debouncing/Filtering**: Ensure the external clock input is robust against noise or double-triggering (though `gate_in_1.State()` is digital, software debouncing might be needed if not already handled by libDaisy). *(spec: [external-clock-behavior])*
- [ ] **Tempo Smoothing**: When slaved to external clock, the internal BPM estimation should be smooth to avoid jittery behavior if we use BPM-dependent logic (like LFOs). *(spec: [external-clock-behavior])*
- [ ] **Robust Auto-Switching**: Verify the timeout logic handles edge cases (e.g. very slow clocks) gracefully.

### Context-Aware K4 Controls
- [ ] **Expose `usingExternalClock_` state**: Add getter `bool IsUsingExternalClock()` to Sequencer. *(spec: [external-clock-behavior])*
- [ ] **K4 Primary context switch**: When external clock patched, K4 (Config Primary) controls CLOCK DIV instead of TEMPO. *(spec: [external-clock-behavior])*
- [ ] **K4+Shift context switch**: When external clock patched, K4+Shift (Config Shift) controls AUX MODE instead of CLOCK DIV. *(spec: [external-clock-behavior], [aux-output-modes])*
- [ ] **External clock rate division**: CLOCK DIV affects sequencer step rate when external clock is used (÷4 to ×4 relative to incoming clock). *(spec: [external-clock-behavior])*

### Implementation Plan
- [ ] Refactor `AudioCallback` in `main.cpp` to handle `CV_OUT_1` writing.
- [ ] Verify `Gate In 1` handling in `AudioCallback` is efficient and correct.
- [ ] Add `IsUsingExternalClock()` getter to `Sequencer` class.
- [ ] Update `ProcessControls()` to check external clock state and route K4 appropriately.
- [ ] Implement external clock rate division in `Sequencer::ProcessAudio()`.
- [ ] Add tests for clock output timing if possible (mocking audio callback behavior).
- [ ] Add tests for context-aware K4 behavior.

## Notes
- libDaisy `WriteCvOut` can be called from audio callback, but verify if it's safe/performant (it writes to DAC registers). Usually it is on STM32.
- The AUX MODE functionality (what CV_OUT_1 outputs in different modes) is handled in a separate task: `12-aux-output-modes.md`.

