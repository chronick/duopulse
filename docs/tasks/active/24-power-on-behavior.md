# Task 24: Power-On Behavior

**Status**: PENDING
**Branch**: TBD
**Parent Task**: Task 16 (Hardware Validation)
**Related**: Task 22 (Control Simplification) - coordinate boot defaults with control changes

---

## Problem Statement

During hardware validation (Task 16 Test 7 - Config Persistence), user reported that config settings don't persist after power cycle. Rather than fixing persistence, user wants a fresh start on power-on with good defaults.

### User Feedback (from Task 16)
> "Settings do not appear to persist after power cycle. Lets defer power cycling behavior to new task. I'd like most settings to reset to good defaults after power cycle, and to read performance knob values upon power on."

---

## Current Behavior

- Config settings are written to flash
- On boot, config is loaded from flash
- Performance knobs are read continuously
- Some settings may or may not persist (inconsistent)

---

## Desired Behavior

### On Power-On:

1. **Performance Knobs (K1-K4)**: Read actual hardware position
   - Don't use saved values
   - Immediate response to knob position
   - No soft takeover needed on boot

2. **Config Settings**: Reset to musical defaults
   - Pattern Length: 32 steps
   - Swing: 50% (neutral)
   - AUX Mode: HAT
   - Reset Mode: STEP (or removed per Task 22)
   - Phrase Length: 4 bars
   - Clock Division: x1
   - AUX Density: 100%
   - Voice Coupling: INDEPENDENT (or merged per Task 22)

3. **Shift Parameters**: Reset to musical defaults
   - PUNCH: 50%
   - GENRE: Techno (0%)
   - DRIFT: 0%
   - BALANCE: 50%

---

## What Should Persist?

Consider what (if anything) should survive power cycle:
- **Nothing** - Fully fresh start every time (simplest)
- **Config only** - Performance starts fresh, config remembers
- **Selective** - Some config (like AUX mode) persists, others don't

### Recommendation: Nothing Persists

Benefits:
- Predictable boot state
- No "what state am I in?" confusion
- Module always starts in known-good configuration
- Simpler code (can remove flash persistence entirely)

---

## Implementation Tasks

- [ ] Define exact boot defaults for all parameters
- [ ] Read performance knobs on boot (no soft takeover initially)
- [ ] Reset config to defaults on boot
- [ ] Consider removing flash persistence code entirely
- [ ] Update spec with boot behavior
- [ ] Test on hardware

---

## Boot Sequence

```
1. Hardware init (DaisySP, ADC, GPIO)
2. Set all config to defaults
3. Read performance knobs (K1-K4)
4. Initialize sequencer with current knob values
5. Start audio callback
6. Begin internal clock
```

---

## Files to Modify

- `src/main.cpp` - Boot sequence
- `src/Engine/Persistence.cpp` - May simplify or remove
- `inc/config.h` - Boot defaults

---

## Success Criteria

- [ ] Power-on always starts with known-good defaults
- [ ] Performance knobs reflect actual position immediately
- [ ] No "dead zone" or soft takeover on boot
- [ ] Module is immediately playable after power-on
- [ ] Consistent behavior across power cycles

---

## Notes

This task should be coordinated with Task 22 (Control Simplification) since the config parameters may change. Complete Task 22 first to know what config parameters exist, then define their boot defaults here.

---

## Cross-Task Dependencies

**Validated by Task 21**: Task 21 (Musicality Improvements) ensures that the boot defaults (Techno genre at field position [1,1] Groovy, PUNCH=50%, BUILD=50%) produce reliably musical patterns. The archetype weight retuning and velocity contrast improvements mean these defaults will sound good immediately on power-on. Ship Task 21 before finalizing boot defaults.
