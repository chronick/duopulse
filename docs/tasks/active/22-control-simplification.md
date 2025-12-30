# Task 22: Control Simplification

**Status**: PENDING
**Branch**: TBD
**Parent Task**: Task 16 (Hardware Validation)
**Related**: Task 21 (Musicality Improvements)

---

## Problem Statement

Hardware validation (Task 16) revealed several config mode controls that are unnecessary, confusing, or could be consolidated:

1. **Reset Mode (Config K4)** - Not useful, can be removed
2. **Phrase Length vs Pattern Length** - Interaction unclear, could consolidate
3. **Voice Coupling vs Balance** - Could be merged into single control
4. **Swing Config** - Doesn't affect timing noticeably, consider removing or reworking

---

## User Feedback (from Task 16)

### Test 7D (Reset Mode)
> "Not sure this setting is necessary after going through it, I'm ok removing it and freeing up a config space. Reset can move default to step 0, but keep the code."

### Test 7E (Phrase Length)
> "Not sure how phrase length and pattern length interact, I think they can be consolidated"

### Test 7H (Voice Coupling)
> "Low and high values appear to work correctly, not sure about LOCKED, as trigs are not technically 1:1. Would appreciate a rethink of this setting, could it be combined with Anchor/Shimmer balance?"

### Test 5/7B (Swing)
> "SWING knob does not feel like it affects timing very much by contrast... For later: we may want to free up SWING for something else and leave GENRE to be the only factor affecting microtiming."

---

## Proposed Changes

### 1. Remove Reset Mode (Config K4)
- Default reset behavior to STEP (step 0)
- Keep code in place but remove from control mapping
- Frees up Config K4 for future use

### 2. Consolidate Phrase/Pattern Length
Options:
- **Option A**: Remove phrase length, pattern length controls full sequence
- **Option B**: Single "Structure" control that sets both proportionally
- **Option C**: Keep pattern length, make phrase auto-calculate (e.g., 4x pattern)

### 3. Rethink Voice Coupling + Balance
Options:
- **Option A**: Merge into single "Voice Relationship" control with continuous behavior
- **Option B**: Remove coupling entirely, keep balance
- **Option C**: INDEPENDENT/SHADOW only (remove LOCKED), balance controls shimmer density

### 4. Swing Decision
Options:
- **Option A**: Remove swing, GENRE controls all timing
- **Option B**: Make swing a multiplier (0x-2x) of GENRE's swing amount
- **Option C**: Repurpose Config K2 for something else (e.g., triplet mode)

---

## Freed Controls After Simplification

If we implement all removals:
- **Config K2**: Formerly Swing (could become triplet mode, shuffle amount, or other)
- **Config K4**: Formerly Reset Mode
- **Config+Shift K4**: Formerly Voice Coupling (merged with Balance)

---

## Implementation Tasks

- [ ] Remove Reset Mode from config mapping (keep code)
- [ ] Consolidate phrase/pattern length (choose option)
- [ ] Merge voice coupling with balance (choose option)
- [ ] Decide swing fate (choose option)
- [ ] Update spec documentation
- [ ] Update persistence (remove/rename parameters)
- [ ] Test on hardware

---

## Success Criteria

- [ ] Fewer config controls = simpler UX
- [ ] Remaining controls all have clear, audible effect
- [ ] No unused/confusing config knob positions
- [ ] Spec reflects actual control layout

---

## Notes

This task should be coordinated with Task 21 (Musicality) since some changes (like swing) affect musical output. May want to complete musicality tuning first to inform decisions here.
