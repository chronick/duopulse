# Hardware QA: Tasks 22-24

**Date**: 2026-01-03
**Branch**: `feature/duopulse-v4`
**Tester**: _____________

---

## Quick Summary

| Task | Feature | What Changed |
|------|---------|--------------|
| **22** | Control Simplification | Removed reset mode UI, auto-derive phrase length, removed INTERLOCK coupling, extended balance range to 0-150% |
| **23** | Immediate Field Updates | Field X/Y knob changes now update pattern within 1 beat (instead of waiting for bar/reset) |
| **24** | Power-On Behavior | All settings reset to musical defaults on boot, performance knobs read from hardware (no saved state) |

---

## Task 22: Control Simplification

### What Changed

**Config Mode Cleanup**:
- Config K4 primary (Reset Mode) is now **unused** — reset always uses STEP mode
- Config+Shift K1 (Phrase Length) is now **auto-derived** from pattern length
- Voice Coupling simplified: **INTERLOCK removed**, now just INDEPENDENT (0-50%) and SHADOW (50-100%)

**Balance Range Extended** (via Task 21):
- Old: 30-100% of anchor (shimmer always audible)
- New: 0-150% of anchor (full silence to shimmer-dominant)

### Test Plan

#### T22.1: Reset Mode Hardcoded
1. Power on module
2. **Verify**: Pattern always resets to step 0 (regardless of previous behavior)
3. Try reset input multiple times
4. **Expected**: Always resets to step 0

#### T22.2: Auto-Derived Phrase Length
1. Switch to Config Mode (Switch B)
2. Turn K1 (Pattern Length) to each setting:
   - 16 steps → phrase should be ~8 bars (128 total steps)
   - 24 steps → phrase should be ~5 bars (120 total steps)
   - 32 steps → phrase should be ~4 bars (128 total steps)
   - 64 steps → phrase should be ~2 bars (128 total steps)
3. **Expected**: Phrase length adjusts automatically, BUILD arc timing feels consistent across all pattern lengths

#### T22.3: Coupling Simplified (INTERLOCK Removed)
1. Config Mode, Config+Shift K4 (Voice Coupling)
2. Turn knob slowly from 0% to 100%
3. **Expected**:
   - 0-50%: INDEPENDENT (voices generate separately)
   - 50-100%: SHADOW (shimmer copies anchor timing)
   - No in-between state (no INTERLOCK)
4. **Listen for**: Clean transition at 50% mark, no broken/sparse patterns

#### T22.4: Balance Range 0-150%
1. Performance Mode, Performance+Shift K4 (BALANCE)
2. Test extremes:
   - Balance 0%: **Only anchor should trigger** (shimmer silent)
   - Balance 50%: Balanced mix
   - Balance 100%: **Shimmer should be louder/denser than anchor**
3. **Expected**: Full range from anchor-solo to shimmer-dominant

### Things to Watch For

- Config K4 primary knob does nothing (freed for future use)
- Config+Shift K1 does nothing (freed for future use)
- Coupling knob should NOT have a "middle zone" where patterns feel broken
- Balance 0% should be **completely silent** on shimmer output (not just quieter)

---

## Task 23: Immediate Field Updates

### What Changed

Field X/Y knob movements now trigger pattern regeneration at the next **beat boundary** (every 4 steps) instead of waiting for bar/reset.

### Test Plan

#### T23.1: Immediate Field X Response
1. Performance Mode, let pattern play
2. Turn K3 (Field X) from 0% to 100% quickly
3. **Expected**: Pattern character changes within **1 beat** (max 4 steps latency)
4. **Listen for**: Syncopation density shifts immediately

#### T23.2: Immediate Field Y Response
1. Performance Mode, let pattern play
2. Turn K4 (Field Y) from 0% to 100% quickly
3. **Expected**: Pattern character changes within **1 beat**
4. **Listen for**: Complexity/ghost note density shifts immediately

#### T23.3: No Double-Regeneration at Bar Boundaries
1. Let pattern play across a bar boundary
2. Turn Field X or Y knob just before bar transition
3. **Expected**: Single regeneration (not two glitches)
4. **Listen for**: Smooth transition, no stuttering

#### T23.4: Threshold Prevents Noise
1. Leave Field X/Y knobs in middle position
2. Let pattern play for 30 seconds
3. **Expected**: Pattern should be stable (not constantly regenerating from knob noise)
4. **Threshold is 10%** — small wiggles should be ignored

#### T23.5: Interaction with BUILD
1. Set BUILD (K2) to create lots of regeneration
2. Simultaneously move Field X/Y knobs
3. **Expected**: Both systems work together (BUILD regenerations + field change regenerations)
4. **Listen for**: No timing glitches or missed regenerations

### Things to Watch For

- Audio glitches during regeneration (should be seamless)
- Constant pattern changes from noisy pots (should be stable unless knob moves >10%)
- Interaction with reset input (field changes + manual reset should both work)
- Transitions should align to beat boundaries (not mid-step)

---

## Task 24: Power-On Behavior

### What Changed

**Fresh Start on Every Boot**:
- All settings reset to musical defaults
- Performance knobs read from hardware (no soft takeover)
- No flash persistence (nothing saved across power cycles)

### Test Plan

#### T24.1: Boot Defaults
1. Power off module
2. Power on module
3. **Expected defaults**:
   - Pattern Length: 32 steps
   - Swing: 50% (neutral)
   - AUX Mode: HAT
   - Phrase Length: 4 bars (auto-derived from 32 steps)
   - Clock Division: x1
   - AUX Density: 100%
   - Voice Coupling: INDEPENDENT
   - PUNCH: 50%
   - GENRE: Techno (0%)
   - DRIFT: 0%
   - BALANCE: 50%

#### T24.2: Performance Knobs Read Immediately
1. Before power-on, set all performance knobs (K1-K4) to known positions (e.g., all at 50%)
2. Power on module
3. **Expected**: Knobs immediately affect output (no dead zone or soft takeover)
4. Turn K1 (ENERGY) — should respond instantly

#### T24.3: No Persistence Across Power Cycles
1. Change config settings (e.g., Pattern Length to 64, Swing to 75%)
2. Power off
3. Power on
4. **Expected**: Settings revert to defaults (Pattern Length back to 32, Swing back to 50%)

#### T24.4: Consistent Boot State
1. Power cycle module 3 times
2. **Expected**: Identical behavior each time (no randomness or variation)
3. Pattern should sound the same at boot defaults

#### T24.5: Module Immediately Playable
1. Power on module (don't touch any controls)
2. **Expected**:
   - Immediately hear musical pattern (techno kick + hat at field [1,1])
   - Reasonable tempo (default internal clock)
   - Balanced voices
   - Pattern should be danceable without adjustment

### Things to Watch For

- Any lingering state from previous session (should be fully cleared)
- Soft takeover behavior on boot (should NOT exist — knobs should work immediately)
- Non-musical boot state (defaults should sound good)
- Different behavior across power cycles (should be 100% consistent)

---

## Quick Control Reference

### Performance Mode (Switch A) — Live Control

| Knob | Primary (Always Active) | +Shift (Hold Button) |
|------|-------------------------|----------------------|
| **K1** | ENERGY (hit density) | PUNCH (velocity dynamics) |
| **K2** | BUILD (phrase arc) | GENRE (techno/breakbeat/IDM) |
| **K3** | FIELD X (syncopation ↔ driving) | DRIFT (evolution rate) |
| **K4** | FIELD Y (sparse ↔ complex) | BALANCE (anchor ↔ shimmer) |

### Config Mode (Switch B) — One-Time Setup

| Knob | Primary | +Shift |
|------|---------|--------|
| **K1** | PATTERN LENGTH (16/24/32/64) | ~~PHRASE LENGTH~~ (now auto-derived) |
| **K2** | SWING (0-100%) | CLOCK DIV (÷1/÷2/÷4/÷8) |
| **K3** | AUX MODE (Hat/Fill/Phrase/Event) | AUX DENSITY (50-200%) |
| **K4** | ~~RESET MODE~~ (now unused) | VOICE COUPLING (Indep/Shadow) |

### Outputs

| Output | Hardware | Signal |
|--------|----------|--------|
| **Anchor Trig** | Gate Out 1 | 5V trigger (kick/low voice) |
| **Shimmer Trig** | Gate Out 2 | 5V trigger (hat/high voice) |
| **Anchor Vel** | Audio Out L | 0-5V CV (velocity) |
| **Shimmer Vel** | Audio Out R | 0-5V CV (velocity) |
| **AUX** | CV Out 1 | Clock OR Hat/Fill/Phrase/Event |
| **LED** | CV Out 2 | Activity indicator |

### Inputs

| Input | Hardware | Function |
|-------|----------|----------|
| **Clock** | Gate In 1 | External clock (disables internal) |
| **Reset** | Gate In 2 | Reset to step 0 |
| **Fill CV** | Audio In L | Pressure-sensitive fill (0-5V) |

---

## General Testing Notes

### Expected Behavior (All Tasks)
- Patterns should always be rhythmically stable (no glitches or stutters)
- Control changes should feel responsive (especially Field X/Y after Task 23)
- Boot behavior should be 100% predictable (Task 24)
- Freed controls (Config K4, Config+Shift K1) should do nothing

### Common Gotchas
- **Balance 0%** should be **silent** on shimmer (not just quiet)
- **Field X/Y** should update within **1 beat** (not immediately, not full bar)
- **Coupling knob** no longer has 3 zones (just 2: Independent and Shadow)
- **Boot defaults** should sound musical immediately (Techno genre, field [1,1], 32 steps)

### If You Find Issues
1. Note exact knob positions and settings
2. Describe what you expected vs. what happened
3. Check if issue reproduces after power cycle
4. Test with both internal and external clock

---

## Sign-Off

- [ ] Task 22: All tests passed
- [ ] Task 23: All tests passed
- [ ] Task 24: All tests passed
- [ ] No regressions found
- [ ] Module feels musical and responsive

**Tester Signature**: _____________ **Date**: _______

**Notes**:
