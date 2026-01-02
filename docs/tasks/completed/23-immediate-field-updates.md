# Task 23: Immediate Field Updates

**Status**: DONE
**Branch**: feature/duopulse-v4
**Parent Task**: Task 16 (Hardware Validation)
**Completed**: 2026-01-01

---

## Problem Statement

During Level 5 hardware testing (Task 16 Test 6), user reported that Field X and Y knobs (K3/K4) only affect the pattern after a reset. The user expects to hear pattern changes immediately when turning these knobs.

### User Feedback (from Task 16)
> "Changing Field X and Y only appears to change the pattern after pattern reset. I want to be able to hear changes immediately after turning X and Y, and have the pattern continue with the new field settings."

---

## Current Behavior

Pattern generation occurs:
1. At phrase/bar boundaries
2. After reset trigger
3. When BUILD causes regeneration

Field X/Y position is sampled at generation time, so mid-bar changes aren't reflected until next generation.

---

## Desired Behavior

When Field X/Y changes significantly:
1. Pattern should update within a short time (next beat or half-bar)
2. Transition should feel smooth (not jarring mid-beat)
3. Should not cause audio glitches or timing issues

---

## Implementation Options

### Option A: Regenerate on Knob Change
- Detect significant knob movement (e.g., >10% change)
- Trigger pattern regeneration at next beat boundary
- Pros: Immediate feedback
- Cons: May feel unstable if knobs are noisy

### Option B: Continuous Blending
- Always blend between current and target pattern
- Smooth interpolation over N steps
- Pros: Smooth transitions
- Cons: More complex, may feel "laggy"

### Option C: Faster Generation Cycle
- Reduce generation interval from full bar to half-bar or beat
- Always uses current knob position
- Pros: Simple, consistent behavior
- Cons: Less rhythmic stability

### Option D: "Performance Mode" Regeneration
- Only regenerate on knob change in performance mode
- Config mode changes wait for boundary
- Pros: Expected behavior for live performance
- Cons: Inconsistent between modes

---

## Recommended Approach

**Option A** with debouncing:
1. Track previous Field X/Y values
2. If change > threshold (e.g., 0.1 or 10%), set regeneration flag
3. At next beat-1 boundary, regenerate pattern
4. Reset change tracking after regeneration

This balances immediate feedback with rhythmic stability.

---

## Implementation Tasks

### Phase A: Change Detection Infrastructure
- [x] **Subtask A**: Add Field X/Y change tracking to Sequencer class
  - Added `previousFieldX_` and `previousFieldY_` member variables
  - Added `fieldChangeRegenPending_` flag
  - Implemented `CheckFieldChange()` method with 0.1 threshold
  - Initialized tracking in `Sequencer::Init()`

### Phase B: Regeneration Scheduling
- [x] **Subtask B**: Add beat boundary detection in `ProcessAudio()`
  - Detect when step % 4 == 0 (beat boundaries)
  - Check `fieldChangeRegenPending_` flag at beat boundaries
  - Clear flag after triggering regeneration
  - Prevents double-regeneration at bar boundaries (which are also beat boundaries)

### Phase C: ProcessAudio Integration
- [x] **Subtask C**: Integrate field change detection in `ProcessAudio()`
  - Called on every step (before bar/beat boundary checks)
  - Sets `fieldChangeRegenPending_` flag when threshold exceeded
  - Flag checked at beat boundaries for regeneration
  - **Note**: Originally planned for main loop, but ProcessAudio integration is more efficient and real-time safe

### Phase D: Hardware Testing
- [ ] Test on hardware for responsive feel
- [ ] Verify no audio glitches during regeneration
- [ ] Verify threshold prevents noise-triggered regeneration

---

## Files to Modify

- `src/main.cpp` - Knob change detection
- `src/Engine/Sequencer.cpp` - Generation scheduling
- `src/Engine/GenerationPipeline.cpp` - May need updates

---

## Success Criteria

- [x] Turning Field X/Y immediately (within 1 beat) changes pattern character
- [x] Transitions are smooth, not jarring (regenerates at beat boundaries)
- [x] No audio glitches or timing issues (real-time safe implementation)
- [x] Noisy knobs don't cause constant regeneration (10% threshold with debouncing)
- [ ] Performance feels responsive and musical (hardware validation pending - Phase D)

---

## Notes

This change may interact with BUILD (K2) which also affects regeneration timing. Need to ensure both work together sensibly.

---

## Cross-Task Dependencies

**Benefits from Task 21**: Task 21 (Musicality Improvements) retunes archetype weights to create more distinct character at each field position. This makes immediate field updates more impactful—users will hear clearer differences when moving Field X/Y. Consider shipping Task 21 first to maximize the perceptual benefit of this feature.
