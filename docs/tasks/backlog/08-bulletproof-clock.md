---
id: chronick/daisysp-idm-grids-08
title: "Bulletproof Clock & External Clock Behavior"
status: "pending"
created_date: "2025-11-25"
last_updated: "2025-12-19"
owner: "user/ai"
spec_refs:
  - "external-clock-behavior"
  - "duopulse-v4"
---

# Task: Bulletproof Clock & External Clock Behavior

## Context

To ensure the sequencer syncs tightly with external equipment, we need robust clock handling. This task covers:

1. **Sample-accurate clock output** — Move CV_OUT_1 update from control rate to audio rate
2. **Robust external clock input** — Debouncing, tempo smoothing, timeout handling
3. **Clock division/multiplication** — For both internal and external clock

**Note**: In v4, the control mapping changes. K2+Shift (Config) controls CLOCK DIV. When external clock is patched, AUX output mode becomes available (K3 in Config mode).

## v4 Architecture Changes

| v2/v3 | v4 |
|-------|-----|
| K4 Config = TEMPO | No dedicated TEMPO control (internal clock assumed) |
| K4+Shift Config = CLOCK DIV | K2+Shift Config = CLOCK DIV |
| AUX MODE only when ext clock patched | K3 Config = AUX MODE (always available) |
| Internal clock has TEMPO knob | Internal clock inferred from external, or fixed default |

**Decision needed**: How does v4 handle internal tempo? Options:
1. Fixed internal tempo (e.g., 120 BPM)
2. Derive from external clock when available, use last-known or default otherwise
3. Add TEMPO to a different control slot

## Requirements

### Clock Output (CV_OUT_1)

- [ ] **Sample-Accurate Output**: Move CV_OUT_1 update from `ProcessControls()` to `AudioCallback()`. *(spec: [external-clock-behavior])*
- [ ] **Pulse Width**: Consistent trigger pulse width (5ms per v4 spec `kTriggerLengthMs`).
- [ ] **Swing Timing**: Clock output respects swing timing when applicable.

### Clock Input (Gate In 1)

- [ ] **Debouncing/Filtering**: Ensure external clock input is robust against noise or double-triggering. *(spec: [external-clock-behavior])*
- [ ] **Tempo Estimation**: Smooth BPM estimation when slaved to external clock. *(spec: [external-clock-behavior])*
- [ ] **Timeout Handling**: 2-second timeout reverts to internal clock gracefully. *(spec: [external-clock-behavior])*
- [ ] **Detection State**: Add `bool externalClockPatched` to `DuoPulseState`. *(spec: v4 section 10.1)*

### Clock Division/Multiplication

- [ ] **CLOCK DIV Parameter**: K2+Shift in Config mode controls clock division. *(spec: v4 section 4.5)*
- [ ] **Division Values**: 1, 2, 4, 8 (divides incoming clock or internal clock).
- [ ] **External Clock Rate**: When external clock patched, CLOCK DIV affects sequencer step rate relative to incoming pulses.

### AUX Output Behavior

- [ ] **Internal Clock**: CV_OUT_1 outputs clock (respecting CLOCK DIV).
- [ ] **External Clock Patched**: CV_OUT_1 outputs selected AUX mode (HAT/FILL_GATE/PHRASE_CV/EVENT). *(spec: v4 section 8.3)*

## Implementation Plan

### Phase 1: Sample-Accurate Clock Output

- [ ] Create `ProcessClockOutput()` function in Sequencer or separate module.
- [ ] Move CV_OUT_1 writing from `ProcessControls()` to `AudioCallback()`.
- [ ] Implement trigger state machine for consistent pulse width.
- [ ] Verify timing with oscilloscope or logic analyzer.

### Phase 2: Robust Clock Input

- [ ] Add `externalClockPatched` state to `DuoPulseState`.
- [ ] Implement edge detection with optional software debounce.
- [ ] Add tempo estimation with smoothing (exponential moving average).
- [ ] Implement 2-second timeout for external clock detection.
- [ ] Add `IsUsingExternalClock()` getter.

### Phase 3: Clock Division

- [ ] Implement clock division logic (divide incoming pulses by 1/2/4/8).
- [ ] Store division counter in sequencer state.
- [ ] Apply division to both internal clock output and external clock input.

### Phase 4: Integration

- [ ] Wire clock detection to AUX output mode selection.
- [ ] When external clock detected, route AUX mode to CV_OUT_1.
- [ ] When internal clock, route clock signal to CV_OUT_1.

## Testing

- [ ] Unit test: Clock division produces correct step rate.
- [ ] Unit test: Timeout detection after 2 seconds of no clock.
- [ ] Integration test: External clock sync at various tempos (60-200 BPM).
- [ ] Hardware test: Verify sample-accurate clock output timing.

## Notes

- This task is a prerequisite for full AUX output functionality in v4.
- The v4 spec assumes external clock for AUX modes; internal clock always outputs clock.
- Consider edge cases: very slow clocks (< 30 BPM), very fast clocks (> 300 BPM).

## Dependencies

- **Blocks**: Phase 6 (Output System) of v4 task for full AUX mode implementation.
- **Depends on**: Phase 1 (Foundation) of v4 task for `DuoPulseState` structure.
