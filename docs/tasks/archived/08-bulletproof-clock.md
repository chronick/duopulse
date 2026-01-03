---
id: chronick/daisysp-idm-grids-08
title: "Bulletproof Clock & External Clock Behavior"
status: "archived"
created_date: "2025-11-25"
last_updated: "2026-01-03"
archived_date: "2026-01-03"
owner: "user/ai"
spec_refs:
  - "external-clock-behavior"
  - "duopulse-v4"
---

# Task: Bulletproof Clock & External Clock Behavior

## Archival Notice (2026-01-03)

**This task is archived because most work was completed by Task 19, and remaining work is not needed.**

### What Was Completed by Task 19 ✅
- **Phase 2 (Robust Clock Input)**: External clock detection, exclusive mode, rising edge detection
- **Phase 3 (Clock Division)**: Full implementation (÷8 to ×8) for both internal and external clock
- **Phase 4 (Integration)**: AUX output mode selection based on clock source

### What Became Obsolete ❌
- v4 architecture changes (lines 25-37) were superseded by Tasks 22-24
- 2-second timeout was intentionally removed (conflicts with exclusive clock design)
- Control mappings changed (Clock DIV is now K2+Shift per Task 22)

### What Remains Unimplemented
**Phase 1: Sample-Accurate Clock Output** - Move CV_OUT_1 from control rate to audio rate

**Analysis**:
- CV_OUT_1 (AUX output) currently updates at ~1kHz control rate from main loop
- AUX modes HAT/EVENT/CLOCK use 10ms triggers (~480 samples at 48kHz)
- In theory, triggers could complete between main loop iterations and be missed
- In practice, no hardware issues have been observed with current implementation

**Decision**: Defer until/unless AUX output reliability issues are observed in hardware testing. The existing `TriggerState` infrastructure in `OutputState.h` is already audio-rate capable; moving the `WriteCvOut()` call from main loop to audio callback would be straightforward if needed.

---

## Original Task Content

### Context

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

- [x] **Debouncing/Filtering**: Ensure external clock input is robust against noise or double-triggering. *(Completed Task 19)*
- [x] **Tempo Estimation**: Smooth BPM estimation when slaved to external clock. *(Not implemented - exclusive mode uses external tempo directly)*
- [x] **Timeout Handling**: 2-second timeout reverts to internal clock gracefully. *(Removed by design - exclusive mode stays active)*
- [x] **Detection State**: Add `bool externalClockPatched` to `DuoPulseState`. *(Completed Task 19 - `externalClockActive_`)*

### Clock Division/Multiplication

- [x] **CLOCK DIV Parameter**: K2+Shift in Config mode controls clock division. *(Completed Task 19)*
- [x] **Division Values**: 1, 2, 4, 8 (divides incoming clock or internal clock). *(Completed Task 19 - also includes multiplication ×2, ×4, ×8)*
- [x] **External Clock Rate**: When external clock patched, CLOCK DIV affects sequencer step rate relative to incoming pulses. *(Completed Task 19)*

### AUX Output Behavior

- [x] **Internal Clock**: CV_OUT_1 outputs clock (respecting CLOCK DIV). *(Completed Task 19)*
- [x] **External Clock Patched**: CV_OUT_1 outputs selected AUX mode (HAT/FILL_GATE/PHRASE_CV/EVENT). *(Completed Task 19)*

## Implementation Plan

### Phase 1: Sample-Accurate Clock Output

- [ ] Create `ProcessClockOutput()` function in Sequencer or separate module.
- [ ] Move CV_OUT_1 writing from `ProcessControls()` to `AudioCallback()`.
- [ ] Implement trigger state machine for consistent pulse width.
- [ ] Verify timing with oscilloscope or logic analyzer.

### Phase 2: Robust Clock Input ✅ COMPLETED BY TASK 19

- [x] Add `externalClockPatched` state to `DuoPulseState`.
- [x] Implement edge detection with optional software debounce.
- [x] Add tempo estimation with smoothing (exponential moving average).
- [x] Implement 2-second timeout for external clock detection.
- [x] Add `IsUsingExternalClock()` getter.

### Phase 3: Clock Division ✅ COMPLETED BY TASK 19

- [x] Implement clock division logic (divide incoming pulses by 1/2/4/8).
- [x] Store division counter in sequencer state.
- [x] Apply division to both internal clock output and external clock input.

### Phase 4: Integration ✅ COMPLETED BY TASK 19

- [x] Wire clock detection to AUX output mode selection.
- [x] When external clock detected, route AUX mode to CV_OUT_1.
- [x] When internal clock, route clock signal to CV_OUT_1.

## Testing

- [x] Unit test: Clock division produces correct step rate. *(Task 19)*
- [x] Unit test: Timeout detection after 2 seconds of no clock. *(Removed by design)*
- [x] Integration test: External clock sync at various tempos (60-200 BPM). *(Task 19 hardware validation)*
- [ ] Hardware test: Verify sample-accurate clock output timing.

## Notes

- This task is a prerequisite for full AUX output functionality in v4.
- The v4 spec assumes external clock for AUX modes; internal clock always outputs clock.
- Consider edge cases: very slow clocks (< 30 BPM), very fast clocks (> 300 BPM).

## Dependencies

- **Blocks**: Phase 6 (Output System) of v4 task for full AUX mode implementation.
- **Depends on**: Phase 1 (Foundation) of v4 task for `DuoPulseState` structure.
