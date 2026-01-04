---
id: 26
slug: fix-tasks-22-24-bugs
title: "Fix Tasks 22-24 Implementation Bugs"
status: completed
created_date: 2026-01-03
updated_date: 2026-01-03
completed_date: 2026-01-03
branch: feature/26-fix-tasks-22-24-bugs
depends_on: [22, 23, 24]
---

# Task 26: Fix Tasks 22-24 Implementation Bugs

---

## Problem Statement

Pre-hardware bug hunt (using Claude Sonnet 4.5 + OpenAI Codex gpt-5.2-codex) revealed **3 critical bugs** in Tasks 22-24 that will cause incorrect behavior on hardware. All three bugs are implementation gaps where commits claim features are complete but code was only partially updated.

**Analysis Documents**:
- `docs/BUG_HUNT-Tasks-22-24.md` - Detailed analysis with line references
- `docs/QA-Tasks-22-24.md` - Hardware QA test plan (ready after fixes)

---

## Critical Bugs Found

### Bug #1: Phrase Length Not Auto-Derived (Task 22)

**Severity**: CRITICAL - Wrong phrase length calculations

**Location**: `src/Engine/DuoPulseState.h:133, 153`

**Problem**: `DuoPulseState` helper methods use legacy `controls.phraseLength` field instead of calling `controls.GetDerivedPhraseLength()`. The `phraseLength` field is never updated when pattern length changes (Task 22 made it read-only), so phrase progress calculations are wrong.

**Impact**:
- Pattern Length 32: Works (phraseLength=4, derived=4) ✅
- Pattern Length 16: **Broken** (phraseLength=4, derived=8) → phrase half as long as intended
- Pattern Length 64: **Broken** (phraseLength=4, derived=2) → phrase twice as long as intended

**Why it happened**: Task 22 made `ControlProcessor` ignore phrase length knob, but forgot to update `DuoPulseState` to use auto-derivation.

---

### Bug #2: Reset Mode Not Actually Freed (Task 22)

**Severity**: CRITICAL - Task 22 incomplete, spec violation

**Location**: `src/main.cpp:334, 505`

**Problem**: Task 22 commit message claims "Config K4 primary now FREE for future features" and "Reset mode hardcoded to STEP", but:

✅ `ControlProcessor.cpp` correctly ignores K4 input
❌ `main.cpp` still maps K4 to resetMode parameter
❌ `main.cpp` still calls `sequencer.SetResetMode()`

**Impact**:
- Config K4 is NOT actually free (user can still change reset mode)
- Contradicts Task 22 spec and commit message
- Soft-takeover still tracking freed control
- Autosave may trigger flash writes unnecessarily

---

### Bug #3: Phrase Length Not Actually Freed (Task 22)

**Severity**: CRITICAL - Task 22 incomplete, spec violation

**Location**: `src/main.cpp:343, 511, 520`

**Problem**: Task 22 commit message claims "Config+Shift K1 now FREE for future features", but:

✅ `ControlProcessor.cpp` correctly ignores Shift K1 input
❌ `main.cpp` still maps Shift K1 to phraseLengthKnob
❌ `main.cpp` still calls `sequencer.SetPhraseLength()`
❌ `main.cpp` still packs phraseLength into autosave config

**Impact**:
- Config+Shift K1 is NOT actually free
- Unnecessary flash writes on freed control
- User confusion (knob appears to do nothing)
- Contradicts Task 22 spec and commit message

---

## Medium Priority Bugs

### Bug #4: Inconsistent Default Values (Task 24)

**Severity**: MEDIUM - Unpredictable boot behavior

**Location**: `src/Engine/ControlState.h:320-324` vs `src/main.cpp:160-163`

**Problem**: `ControlState::Init()` and `MainControlState` have different defaults:

| Parameter | ControlState::Init() | MainControlState | Impact |
|-----------|---------------------|------------------|--------|
| energy | 0.5 | 0.6 | Different density on first bar |
| fieldX | 0.0 | 0.5 | Different archetype (Minimal vs Groovy) |
| fieldY | 0.0 | 0.33 | Different complexity |

**Impact**: First bar generated with wrong defaults, then abruptly changes on first control scan.

---

## Implementation Plan

### Phase A: Critical Fixes (Required Before Hardware Testing)

**Estimated time**: 15 minutes, low risk

#### A1: Fix Phrase Length Auto-Derivation ✅

**File**: `src/Engine/DuoPulseState.h`

```diff
@@ -130,7 +130,7 @@ struct DuoPulseState
     {
         return sequencer.GetPhraseProgress(
             controls.patternLength,
-            controls.phraseLength
+            controls.GetDerivedPhraseLength()
         );
     }

@@ -150,7 +150,7 @@ struct DuoPulseState
     void AdvanceStep()
     {
         stepSampleCounter = 0;
-        sequencer.AdvanceStep(controls.patternLength, controls.phraseLength);
+        sequencer.AdvanceStep(controls.patternLength, controls.GetDerivedPhraseLength());

         // Update derived parameters based on new position
         controls.UpdateDerived(GetPhraseProgress());
```

**Test**: Verify phrase length auto-derives correctly for all pattern lengths.

---

#### A2: Remove Reset Mode from Main Loop ✅

**File**: `src/main.cpp`

```diff
@@ -331,7 +331,7 @@ float* GetParameterPtr(MainControlState& state, ControlMode mode, int knobIndex
                 case 0: return &state.patternLengthKnob;
                 case 1: return &state.swing;
                 case 2: return &state.auxMode;
-                case 3: return &state.resetMode;
+                case 3: return nullptr;  // K4 freed (Task 22)
             }
             break;

@@ -502,7 +502,8 @@ void ProcessControls(/* ... */)
     sequencer.SetPatternLength(MapToPatternLength(controlState.patternLengthKnob));
     sequencer.SetSwing(controlState.swing);
     sequencer.SetAuxMode(GetAuxModeFromValue(controlState.auxMode));
-    sequencer.SetResetMode(GetResetModeFromValue(controlState.resetMode));
+    // Reset mode hardcoded to STEP in ControlState::Init() (Task 22)
+    // Config K4 primary is now free for future features

     // Config Shift
@@ -517,7 +518,7 @@ void ProcessControls(/* ... */)
     // Create config for persistence
     PackConfig(
         currentConfig,
-        MapToPatternLength(controlState.patternLengthKnob), controlState.swing, GetAuxModeFromValue(controlState.auxMode), GetResetModeFromValue(controlState.resetMode),
+        MapToPatternLength(controlState.patternLengthKnob), controlState.swing, GetAuxModeFromValue(controlState.auxMode), ResetMode::STEP,  // Task 22: hardcoded
```

**Test**: Config K4 should have no effect. Reset behavior always STEP.

---

#### A3: Remove Phrase Length from Main Loop ✅

**File**: `src/main.cpp`

```diff
@@ -340,7 +340,7 @@ float* GetParameterPtr(MainControlState& state, ControlMode mode, int knobIndex
         case ControlMode::ConfigShift:
             switch(knobIndex)
             {
-                case 0: return &state.phraseLengthKnob;
+                case 0: return nullptr;  // Shift K1 freed (Task 22)
                 case 1: return &state.clockDivKnob;
                 case 2: return &state.auxDensity;
                 case 3: return &state.voiceCoupling;

@@ -508,7 +509,8 @@ void ProcessControls(/* ... */)
     sequencer.SetResetMode(GetResetModeFromValue(controlState.resetMode));

     // Config Shift
-    sequencer.SetPhraseLength(MapToPhraseLength(controlState.phraseLengthKnob));
+    // Phrase length auto-derived from pattern length (Task 22)
+    // Config+Shift K1 is now free for future features
     sequencer.SetClockDivision(MapToClockDiv(controlState.clockDivKnob));
     sequencer.SetAuxDensity(GetAuxDensityFromValue(controlState.auxDensity));
     sequencer.SetVoiceCoupling(GetVoiceCouplingFromValue(controlState.voiceCoupling));

@@ -518,7 +520,7 @@ void ProcessControls(/* ... */)
     PackConfig(
         currentConfig,
         MapToPatternLength(controlState.patternLengthKnob), controlState.swing, GetAuxModeFromValue(controlState.auxMode), ResetMode::STEP,
-        MapToPhraseLength(controlState.phraseLengthKnob), MapToClockDiv(controlState.clockDivKnob), GetAuxDensityFromValue(controlState.auxDensity), GetVoiceCouplingFromValue(controlState.voiceCoupling),
+        4, MapToClockDiv(controlState.clockDivKnob), GetAuxDensityFromValue(controlState.auxDensity), GetVoiceCouplingFromValue(controlState.voiceCoupling),  // Task 22: phrase auto-derived (default 4)
```

**Test**: Config+Shift K1 should have no effect. Phrase length always auto-derived.

---

#### A4: Align Default Values ✅

**File**: `src/Engine/ControlState.h`

```diff
@@ -318,9 +318,9 @@ struct ControlState
     void Init()
     {
         // Performance primary
-        energy = 0.5f;
+        energy = 0.6f;  // Match MainControlState (Task 24)
         build  = 0.0f;
-        fieldX = 0.0f;
-        fieldY = 0.0f;
+        fieldX = 0.5f;  // Match MainControlState (Task 24)
+        fieldY = 0.33f; // Match MainControlState (Task 24)

         // Performance shift
         punch   = 0.5f;
```

**Test**: First bar should sound identical to subsequent bars (no abrupt change on boot).

---

### Phase B: Low Priority Fixes (Can Defer)

These issues are low-severity and can be addressed during hardware testing if needed:

- **Field change flag on reset**: Clear `fieldChangeRegenPending_` in `TriggerReset()` to avoid double-regen edge case
- **CV triggers field changes**: Consider using knob-only detection instead of `GetEffectiveFieldX/Y()` to avoid CV noise triggering regeneration
- **Freed controls in soft-takeover**: Clean up soft-knob indices 11-12 (cosmetic only)

---

## Testing Requirements

### Unit Tests

- [ ] All existing tests pass (229 tests, 51554+ assertions)
- [ ] Pattern length 16 → phrase is 8 bars (128 steps)
- [ ] Pattern length 24 → phrase is 5 bars (120 steps)
- [ ] Pattern length 32 → phrase is 4 bars (128 steps)
- [ ] Pattern length 64 → phrase is 2 bars (128 steps)
- [ ] BUILD parameter feels consistent across all pattern lengths

### Smoke Test (Pre-Hardware)

- [ ] Firmware compiles without errors
- [ ] No new compiler warnings
- [ ] No test regressions

### Hardware Validation

After fixes, proceed with `docs/QA-Tasks-22-24.md` test plan:
- [ ] Config K4 has no effect (reset always STEP)
- [ ] Config+Shift K1 has no effect (phrase auto-derived)
- [ ] Pattern length changes → phrase length adjusts correctly
- [ ] First bar sounds identical to rest (no abrupt change)
- [ ] All Tasks 22-24 features work as specified

---

## Files to Modify

**Critical fixes** (Phase A):
- `src/Engine/DuoPulseState.h` (2 lines)
- `src/main.cpp` (6 locations)
- `src/Engine/ControlState.h` (3 lines)

**Total changes**: ~11 lines across 3 files

---

## Success Criteria

- [x] All 3 critical bugs fixed (A1 ✅, A2 ✅, A3 ✅, A4 ✅)
- [x] All unit tests pass (230 tests, 51561 assertions)
- [x] Firmware compiles clean (no warnings)
- [x] Phase length auto-derivation works for all pattern lengths
- [x] Config K4 and Config+Shift K1 have no effect
- [x] Boot defaults consistent (no abrupt first-bar change)
- [x] Ready for hardware validation with QA doc

---

## Risk Assessment

**Overall Risk**: Low

All fixes are:
- Small changes (~2 lines each)
- Well-understood root causes
- Easily testable
- No architectural changes
- Covered by existing unit tests

**Biggest risk**: Missing another instance of `controls.phraseLength` somewhere (mitigated by grep search).

---

## Notes

This task is a **prerequisite for hardware validation** of Tasks 22-24. The bugs were caught by:
1. Manual code review of git diffs
2. Cross-file analysis of control flow
3. Codex gpt-5.2-codex with extended reasoning

All bugs are **implementation gaps** where Task 22 updated `ControlProcessor` but forgot to update `main.cpp` and `DuoPulseState`. This is a good reminder to grep for all usages when deprecating a field.

---

## Appendix: Analysis Process

Bug hunt methodology:
1. Read git diffs for all three tasks
2. Trace phrase length usage with `rg phraseLength`
3. Verify freed controls actually freed with grep
4. Check default value consistency across files
5. Validate with Codex gpt-5.2-codex (caught bugs #2 and #3)

Time spent on analysis: 30 minutes
Bugs found: 3 critical, 3 medium, 4 minor
Estimated fix time: 15 minutes
