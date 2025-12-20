# Task 15: DuoPulse v4 Hardware QA & Debugging

**Status**: 🔴 IN PROGRESS  
**Created**: 2025-12-19  
**Spec Reference**: `docs/specs/main.md` [duopulse-controls]

---

## Problem Statement

First hardware test of v4 (v3 Pulse Field) implementation revealed:
- Triggers feel "very irregular and slow"
- Not able to control predictably with knobs

## Root Cause Analysis

### Confirmed Issues

1. **Conservative defaults at startup**:
   - `anchorDensity = 0.5` → Only ~25% of positions fire (downbeats/quarters)
   - `shimmerDensity = 0.5` → Only 2 hits per 32 steps (backbeats only!)
   - `broken = 0.0` → No swing, no jitter, completely straight
   - `drift = 0.0` → Pattern identical every loop (feels static)

2. **COUPLE suppression at default 0.5**:
   - When Anchor fires, Shimmer has 40% chance of suppression
   - Backbeats (step 8, 24) where both fire → Shimmer often suppressed

3. **Sparse Shimmer pattern**:
   - At density=0.5, threshold=0.5
   - Shimmer weights >0.5: only steps 4, 8, 20, 24 qualify
   - With COUPLE suppression on 8 and 24: may only get 2 hits!

### Why It Feels "Slow"

At 120 BPM with 16 steps per bar, 32 steps = 4 seconds.  
With density=0.5, ~8 Anchor hits + ~2 Shimmer hits = 10 total hits in 4 seconds.  
That's only **2.5 hits per second** — very sparse!

---

## Hardware Testing Method

### Phase 1: Timing Verification (qa-simple)

**Build and flash:**
```bash
make qa-simple && make program
```

**Expected behavior:**
- Anchor (Gate Out 1): Fires ~8 times per bar (all quarter/8th note positions)
- Shimmer (Gate Out 2): Fires ~8 times per bar
- Tempo: Exactly 120 BPM (8 steps/second)
- Swing: None (straight timing)
- Loop: 1 bar (repeats every 2 seconds)

**Verification checklist:**
- [ ] Gate Out 1 produces regular triggers (~4 per second on strong beats)
- [ ] Gate Out 2 produces regular triggers
- [ ] Timing feels steady (no irregular gaps)
- [ ] LED flashes on triggers
- [ ] CV Out 1 shows clock output

---

### Phase 2: Progressive Feature Enablement

Work through each level, verifying behavior before moving to the next:

#### Level 0: Anchor Only (Clock Verification)
```bash
make qa-level0 && make program
```
- **Gate Out 1**: Fires every step (16 per bar)
- **Gate Out 2**: Silent
- **Verify**: Clock is stable at 120 BPM (8 Hz)

#### Level 1: Both Voices at Max
```bash
make qa-level1 && make program
```
- **Gate Out 1**: Fires on Anchor-weighted positions
- **Gate Out 2**: Fires on Shimmer-weighted positions
- **Verify**: Both outputs active, many triggers per bar

#### Level 2: Variable Density
```bash
make qa-level2 && make program
```
- **Density**: 0.7 (more hits than production default)
- **Verify**: Still feels active, but some positions don't fire

#### Level 3: Add BROKEN
```bash
make qa-level3 && make program
```
- **BROKEN**: 0.4 (Tribal shuffle feel)
- **Verify**: Swing is audible on off-beats
- **Verify**: Slightly less regular timing

#### Level 4: Add DRIFT
```bash
make qa-level4 && make program
```
- **DRIFT**: 0.5 (pattern evolves each loop)
- **Verify**: Pattern changes slightly each time through
- **Verify**: Downbeats stay consistent

#### Level 5: Full Features
```bash
make qa-level5 && make program
```
- **COUPLE**: 0.4 (some voice interlock)
- **RATCHET**: 0.3 (light fills at phrase end)
- **Verify**: Shimmer fills gaps when Anchor is silent
- **Verify**: Fills occur near phrase boundaries

#### Production Defaults
```bash
make qa-production && make program
```
- Return to standard defaults
- **Verify**: Now understand what the sparse defaults sound like
- **Recommendation**: Adjust production defaults based on findings

---

### Phase 3: Knob Response Verification

With `qa-lively` or `qa-production` flashed:

1. **K1 (Anchor Density)**:
   - Full CCW: Gate Out 1 should be mostly silent
   - Full CW: Gate Out 1 should fire on every position
   - **Verify**: Smooth gradient between extremes

2. **K2 (Shimmer Density)**:
   - Full CCW: Gate Out 2 should be mostly silent
   - Full CW: Gate Out 2 should fire frequently
   - **Verify**: Smooth gradient between extremes

3. **K3 (BROKEN)**:
   - Full CCW: Straight timing, regular pattern
   - Full CW: Swung timing, irregular pattern
   - **Verify**: Swing increases, pattern becomes chaotic

4. **K4 (DRIFT)**:
   - Full CCW: Pattern identical every loop
   - Full CW: Pattern evolves significantly each loop
   - **Verify**: Downbeats stay stable even at max DRIFT

---

## Expected Outcomes

### If Timing Is Wrong
- Check `metro_.SetFreq()` calculation in `Sequencer::SetBpm()`
- Verify sample rate is 48000 Hz

### If Pattern Is Too Sparse
- Increase default `anchorDensity_` and `shimmerDensity_` to 0.6-0.7
- Consider lowering default `couple_` to 0.3

### If Swing Sounds Wrong
- Check `GetSwingFromBroken()` output values
- Verify `CalculateSwingDelaySamples()` calculation

### If DRIFT Has No Effect
- Verify `PulseFieldState::OnPhraseReset()` is being called
- Check that `loopSeed_` is changing

---

## Recommended Default Changes

Based on analysis, consider updating production defaults:

```cpp
// Proposed new production defaults
anchorDensity_  = 0.6f;  // Was 0.5 (too sparse)
shimmerDensity_ = 0.6f;  // Was 0.5 (too sparse)
broken_         = 0.15f; // Was 0.0 (add subtle swing)
drift_          = 0.1f;  // Was 0.0 (subtle evolution)
couple_         = 0.3f;  // Was 0.5 (less suppression)
```

This would give a more musical starting point while still being conservative.

---

## Checklist

- [x] Add debug baseline compile flags
- [x] Add QA make targets
- [x] Verify tests still pass
- [ ] Hardware test: qa-simple
- [ ] Hardware test: qa-level0 through qa-level5
- [ ] Hardware test: knob response
- [ ] Determine optimal production defaults
- [ ] Update production defaults if needed
- [ ] Document findings
