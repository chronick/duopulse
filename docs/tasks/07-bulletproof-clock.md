---
id: chronick/daisysp-idm-grids-07
title: "Bulletproof Clock Tempo I/O"
status: "pending"
created_date: "2025-11-25"
last_updated: "2025-11-25"
owner: "user/ai"
---

# Task: Bulletproof Clock Tempo I/O

## Context
To ensure the sequencer syncs tightly with external equipment, we need to improve the clock handling. Currently, `CV_OUT_1` (Clock Out) is updated in `ProcessControls` (~1kHz), which introduces jitter. It should be updated in `AudioCallback` (48kHz) for sample-accurate timing. Additionally, the external clock input logic should be robust against noise and jitter.

## Requirements

### Clock Output (CV_OUT_1)
- [ ] **Sample-Accurate Output**: Move the `CV_OUT_1` update from `ProcessControls()` to `AudioCallback()`.
- [ ] **Pulse Width Control**: Ensure the output pulse width is consistent (e.g., 10ms or configurable) regardless of block size or control rate.

### Clock Input (Gate In 1)
- [ ] **Debouncing/Filtering**: Ensure the external clock input is robust against noise or double-triggering (though `gate_in_1.State()` is digital, software debouncing might be needed if not already handled by libDaisy).
- [ ] **Tempo Smoothing**: When slaved to external clock, the internal BPM estimation should be smooth to avoid jittery behavior if we use BPM-dependent logic (like LFOs).
- [ ] **Robust Auto-Switching**: Verify the timeout logic handles edge cases (e.g. very slow clocks) gracefully.

### Implementation Plan
- [ ] Refactor `AudioCallback` in `main.cpp` to handle `CV_OUT_1` writing.
- [ ] Verify `Gate In 1` handling in `AudioCallback` is efficient and correct.
- [ ] Add tests for clock output timing if possible (mocking audio callback behavior).

## Notes
- libDaisy `WriteCvOut` can be called from audio callback, but verify if it's safe/performant (it writes to DAC registers). Usually it is on STM32.

