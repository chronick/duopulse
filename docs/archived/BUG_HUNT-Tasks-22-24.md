# Bug Hunt & Implementation Validation: Tasks 22-24

**Date**: 2026-01-03
**Commits Analyzed**:
- Task 22: `04777a7` (Control Simplification)
- Task 23: `530a838` (Immediate Field Updates)
- Task 24: `e43b196` (Power-On Behavior)

---

## Executive Summary

### Critical Issues Found: 3 🔴
### Medium Issues Found: 3 🟡
### Minor Issues Found: 4 🟢

**Recommendation**: **DO NOT test on hardware until critical issues are fixed.** All three critical bugs will cause incorrect behavior:
1. Wrong phrase length calculations (broken BUILD/fill behavior)
2. Reset mode still user-controlled (Task 22 incomplete)
3. Phrase length still in control path (Task 22 incomplete)

**Impact**: Tasks 22-24 appear complete in code review but have implementation gaps. Estimated fix time: **15 minutes** for all critical issues.

---

## CRITICAL: Task 22 - Phrase Length Not Auto-Derived Everywhere

### Severity: **CRITICAL** (will cause incorrect behavior)

### Location
- `src/Engine/DuoPulseState.h:133` - `GetPhraseProgress()`
- `src/Engine/DuoPulseState.h:153` - `AdvanceStep()`

### Problem

The `DuoPulseState` helper methods use `controls.phraseLength` (the legacy field) instead of `controls.GetDerivedPhraseLength()`. This means phrase length is NOT actually auto-derived in these critical locations.

```cpp
// Line 133 - WRONG
float GetPhraseProgress() const
{
    return sequencer.GetPhraseProgress(
        controls.patternLength,
        controls.phraseLength  // ❌ Uses legacy field, not auto-derived!
    );
}

// Line 153 - WRONG
void AdvanceStep()
{
    stepSampleCounter = 0;
    sequencer.AdvanceStep(controls.patternLength, controls.phraseLength); // ❌ Wrong!
    controls.UpdateDerived(GetPhraseProgress());
}
```

### Impact

1. **Pattern Length 32** (default): Works correctly
   - `phraseLength` field = 4 (from Init())
   - `GetDerivedPhraseLength()` = 4
   - **No bug visible**

2. **Pattern Length 16**: Broken
   - `phraseLength` field = 4 (never updated)
   - `GetDerivedPhraseLength()` = 8
   - **Phrase is half as long as intended** (64 steps instead of 128)

3. **Pattern Length 64**: Broken
   - `phraseLength` field = 4 (never updated)
   - `GetDerivedPhraseLength()` = 2
   - **Phrase is twice as long as intended** (256 steps instead of 128)

### Why It Happened

`ControlProcessor::ProcessConfigShift()` was changed to skip updating `phraseLength`:

```cpp
// src/Engine/ControlProcessor.cpp:329
// Shift+K1: FREE - Phrase length is now auto-derived from pattern length
// See ControlState::GetDerivedPhraseLength()
(void)input.knobs[0];  // Mark parameter as intentionally unused
```

But `DuoPulseState` wasn't updated to call `GetDerivedPhraseLength()`.

### Verification

```bash
# Sequencer.cpp correctly uses GetDerivedPhraseLength()
grep -n "GetDerivedPhraseLength" src/Engine/Sequencer.cpp
# 1043:    int phraseLength = state_.controls.GetDerivedPhraseLength();

# But DuoPulseState.h does NOT
grep -n "controls\.phraseLength" src/Engine/DuoPulseState.h
# 133:            controls.phraseLength  ❌
# 153:        sequencer.AdvanceStep(controls.patternLength, controls.phraseLength); ❌
```

### Fix Required

```diff
--- a/src/Engine/DuoPulseState.h
+++ b/src/Engine/DuoPulseState.h
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

### Testing

After fix:
1. Set pattern length to 16 → verify phrase is ~128 steps (8 bars)
2. Set pattern length to 64 → verify phrase is ~128 steps (2 bars)
3. BUILD parameter should feel consistent across all pattern lengths

---

## CRITICAL: Task 22 - Reset Mode NOT Actually Freed

### Severity: **CRITICAL** (Task 22 incomplete)

### Location
- `src/main.cpp:334` - `GetParameterPtr()` still maps K4 to resetMode
- `src/main.cpp:505` - `ProcessControls()` still calls `SetResetMode()`

### Problem

Task 22 Phase A1 claims:
> "Config K4 primary no longer controls reset mode... Config K4 primary now FREE for future features"

**But this is only half-implemented:**

✅ `ControlProcessor.cpp:323` - Correctly ignores K4 input
❌ `main.cpp:334` - Still maps K4 to resetMode parameter
❌ `main.cpp:505` - Still calls `sequencer.SetResetMode()`

```cpp
// main.cpp line 334 - GetParameterPtr()
case ControlMode::ConfigPrimary:
    switch(knobIndex) {
        case 0: return &state.patternLengthKnob;
        case 1: return &state.swing;
        case 2: return &state.auxMode;
        case 3: return &state.resetMode;  // ❌ Still mapped!
    }

// main.cpp line 505 - ProcessControls()
sequencer.SetResetMode(GetResetModeFromValue(controlState.resetMode)); // ❌ Still called!
```

### Impact

1. **Config K4 is NOT free** - User can still change reset mode
2. **Contradicts Task 22 spec** - Reset mode is NOT hardcoded to STEP
3. **Soft-takeover still tracking** - Unnecessary overhead
4. **Autosave may trigger** - Flash writes for freed control

### Fix Required

Remove reset mode from main.cpp control path entirely:

```diff
--- a/src/main.cpp
+++ b/src/main.cpp
@@ -331,7 +331,7 @@ float* GetParameterPtr(MainControlState& state, ControlMode mode, int knobIndex
                 case 0: return &state.patternLengthKnob;
                 case 1: return &state.swing;
                 case 2: return &state.auxMode;
-                case 3: return &state.resetMode;
+                case 3: return nullptr;  // K4 freed (Task 22)
             }
             break;

@@ -502,7 +502,7 @@ void ProcessControls(/* ... */)
     sequencer.SetPatternLength(MapToPatternLength(controlState.patternLengthKnob));
     sequencer.SetSwing(controlState.swing);
     sequencer.SetAuxMode(GetAuxModeFromValue(controlState.auxMode));
-    sequencer.SetResetMode(GetResetModeFromValue(controlState.resetMode));
+    // Reset mode hardcoded to STEP in ControlState::Init() (Task 22)
```

---

## CRITICAL: Task 22 - Phrase Length NOT Actually Freed

### Severity: **CRITICAL** (Task 22 incomplete)

### Location
- `src/main.cpp:343` - `GetParameterPtr()` still maps Shift K1 to phraseLength
- `src/main.cpp:511` - `ProcessControls()` still calls `SetPhraseLength()`
- `src/main.cpp:520` - Autosave still checks `phraseLength` for changes

### Problem

Task 22 Phase A2 claims:
> "Config+Shift K1 now FREE for future features"

**But this is only half-implemented:**

✅ `ControlProcessor.cpp:329` - Correctly ignores Shift K1 input
❌ `main.cpp:343` - Still maps Shift K1 to phraseLengthKnob
❌ `main.cpp:511` - Still calls `sequencer.SetPhraseLength()`
❌ `main.cpp:520` - Still packs phraseLength into autosave config

### Impact

1. **Config+Shift K1 is NOT free** - Still mapped and processed
2. **Unnecessary flash writes** - Autosave triggers on freed control
3. **User confusion** - Knob appears to do nothing (SetPhraseLength is no-op)
4. **Contradicts Task 22 spec** - Control is NOT actually freed

### Fix Required

Remove phrase length from main.cpp control path entirely:

```diff
--- a/src/main.cpp
+++ b/src/main.cpp
@@ -340,7 +340,7 @@ float* GetParameterPtr(MainControlState& state, ControlMode mode, int knobIndex
         case ControlMode::ConfigShift:
             switch(knobIndex)
             {
-                case 0: return &state.phraseLengthKnob;
+                case 0: return nullptr;  // Shift K1 freed (Task 22)
                 case 1: return &state.clockDivKnob;
                 case 2: return &state.auxDensity;
                 case 3: return &state.voiceCoupling;
@@ -508,7 +508,7 @@ void ProcessControls(/* ... */)
     sequencer.SetResetMode(GetResetModeFromValue(controlState.resetMode));

     // Config Shift
-    sequencer.SetPhraseLength(MapToPhraseLength(controlState.phraseLengthKnob));
+    // Phrase length auto-derived from pattern length (Task 22)
     sequencer.SetClockDivision(MapToClockDiv(controlState.clockDivKnob));
     sequencer.SetAuxDensity(GetAuxDensityFromValue(controlState.auxDensity));
     sequencer.SetVoiceCoupling(GetVoiceCouplingFromValue(controlState.voiceCoupling));
@@ -517,7 +517,7 @@ void ProcessControls(/* ... */)
     // Create config for persistence
     PackConfig(
         currentConfig,
-        MapToPatternLength(controlState.patternLengthKnob), controlState.swing, GetAuxModeFromValue(controlState.auxMode), GetResetModeFromValue(controlState.resetMode),
-        MapToPhraseLength(controlState.phraseLengthKnob), MapToClockDiv(controlState.clockDivKnob), GetAuxDensityFromValue(controlState.auxDensity), GetVoiceCouplingFromValue(controlState.voiceCoupling),
+        MapToPatternLength(controlState.patternLengthKnob), controlState.swing, GetAuxModeFromValue(controlState.auxMode), ResetMode::STEP,  // Task 22: hardcoded
+        4, MapToClockDiv(controlState.clockDivKnob), GetAuxDensityFromValue(controlState.auxDensity), GetVoiceCouplingFromValue(controlState.voiceCoupling),  // Task 22: phrase auto-derived
```

---

## MEDIUM: Task 24 - Soft-Takeover Initialization Issue

### Severity: **MEDIUM** (UX degradation on boot)

### Location
- `src/main.cpp:722-725` - Soft-takeover initialization
- `src/main.cpp:658-665` - Boot defaults comment

### Problem

The boot code intentionally does NOT initialize performance primary knobs (energy/build/fieldX/fieldY), expecting them to read from hardware. However, the soft-takeover knobs ARE initialized with struct defaults:

```cpp
// Struct initialization (line 160-163) - has defaults
float energy = 0.6f;  // K1: Hit density - mid-GROOVE zone
float build  = 0.0f;  // K2: Phrase arc
float fieldX = 0.5f;  // K3: Center position
float fieldY = 0.33f; // K4: Between minimal and driving

// Boot code (line 658-665) - intentionally skips these
// "These are NOT initialized here - they will read from hardware knobs"

// But soft-takeover Init() (line 722-725) - uses struct defaults!
softKnobs[0].Init(controlState.energy);    // Inits with 0.6
softKnobs[1].Init(controlState.build);     // Inits with 0.0
softKnobs[2].Init(controlState.fieldX);    // Inits with 0.5
softKnobs[3].Init(controlState.fieldY);    // Inits with 0.33
```

### Impact

If hardware knob position differs from struct defaults:
- **Soft-takeover will require user to sweep knob through default position** before it responds
- **Defeats the purpose of Task 24**: "read performance knob values upon power on"

Example:
1. Power on with K1 (energy) at 100%
2. Soft-takeover initialized with 60% (struct default)
3. User must turn K1 down past 60% before it responds
4. **Expected**: Immediate response at 100%

### Potential Fix Options

**Option A**: Read hardware before soft-takeover init
```cpp
// Before soft-takeover init, read actual hardware positions
patch.ProcessAnalogControls();
patch.ProcessDigitalControls();
controlState.energy = patch.GetKnobValue(KNOB_1);
controlState.build = patch.GetKnobValue(KNOB_2);
controlState.fieldX = patch.GetKnobValue(KNOB_3);
controlState.fieldY = patch.GetKnobValue(KNOB_4);

// Then init soft-takeover with actual positions
softKnobs[0].Init(controlState.energy);
// ...
```

**Option B**: Initialize soft-takeover with sentinel "no previous value"
- Might require changes to SoftKnob implementation
- First read would immediately adopt hardware position

**Option C**: Accept current behavior as reasonable default
- Struct defaults are "musical" (task 24 comment line 158)
- User only needs to sweep through default once
- Still better than flash persistence (which could be wildly different)

### Recommendation

- **Option A** is most aligned with Task 24 intent
- Requires adding hardware read before soft-takeover init
- Low risk, clear benefit

---

## MEDIUM: Task 24 - Inconsistent Default Values

### Severity: **MEDIUM** (unpredictable boot behavior)

### Location
- `src/Engine/ControlState.h:320-324` - ControlState::Init() defaults
- `src/main.cpp:160-163` - MainControlState struct defaults

### Problem

Task 24 removed flash loading, so boot behavior depends on initial defaults. But defaults differ between ControlState and MainControlState:

| Parameter | ControlState::Init() | MainControlState | Impact |
|-----------|---------------------|------------------|--------|
| energy | **0.5** | **0.6** | Different density on first bar |
| fieldX | **0.0** | **0.5** | Different archetype (Minimal vs Groovy) |
| fieldY | **0.0** | **0.33** | Different complexity |

### Sequence of Events

1. **Boot** (line 636): Flash loading disabled
2. **Sequencer Init** (line 680): Calls `ControlState::Init()`
   - Sets energy=0.5, fieldX=0.0, fieldY=0.0
3. **First bar generated**: Uses ControlState defaults
   - Pattern starts with Minimal archetype (0,0)
4. **First control scan** (line 460): Reads MainControlState
   - Overwrites with energy=0.6, fieldX=0.5, fieldY=0.33
5. **Next bar**: Pattern abruptly changes to Groovy archetype

### Impact

- **Unpredictable boot state**: First 4 seconds sound different than rest
- **Not "immediately playable"**: Contradicts Task 24 goal
- **User confusion**: Module starts one way, then shifts

### Fix Required

Align ControlState::Init() with MainControlState defaults:

```diff
--- a/src/Engine/ControlState.h
+++ b/src/Engine/ControlState.h
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

### Testing

After fix, verify first bar sounds identical to subsequent bars (no abrupt change).

---

## MEDIUM: Task 23 - Field Change Flag Cleared at Bar Boundaries

### Severity: **MEDIUM** (edge case, unlikely)

### Location
- `src/Engine/Sequencer.cpp:202` - Flag cleared at bar boundary

### Problem

The `fieldChangeRegenPending_` flag is cleared both after regeneration AND at bar boundaries:

```cpp
// Line 191-197: Regenerate at beat boundaries when field changed
if (fieldChangeRegenPending_ && isBeatBoundary && !state_.sequencer.isBarBoundary)
{
    BlendArchetype();
    GenerateBar();
    ComputeTimingOffsets();
    fieldChangeRegenPending_ = false;  // Cleared here
}

// Line 199-204: Generate new bar at bar boundary
if (state_.sequencer.isBarBoundary || isFirstStep)
{
    BlendArchetype();
    GenerateBar();
    ComputeTimingOffsets();
    fieldChangeRegenPending_ = false;  // Also cleared here ← Potential issue
}
```

### Impact

**Edge case**: Field change detected just before bar boundary
1. Step 63 (last step before bar): Field change detected, flag set
2. Step 0 (bar boundary): `isBarBoundary=true`, regeneration happens
3. Flag cleared
4. **Result**: Works correctly (regenerated)

**Potential issue**: Field change at step 60
1. Step 60: Field change detected, flag set
2. Step 60-63: Waiting for beat boundary (step 0 or 4)
3. Step 0: Bar boundary regeneration happens, **flag cleared**
4. **Field change is acknowledged** (regeneration happened)
5. **Result**: Works correctly

After analysis, this appears to be **intentional and correct**:
- Bar boundaries are also beat boundaries
- Regeneration always happens at bar boundaries
- Clearing flag at bar boundary prevents duplicate regeneration
- The `!state_.sequencer.isBarBoundary` check ensures no double-regen

### Verdict

**Not a bug**, but code comment would help clarify intent:

```cpp
fieldChangeRegenPending_ = false;  // Clear flag (regeneration happened at bar boundary)
```

---

## MINOR: Task 22 - Freed Controls Still Processed by Soft-Takeover

### Severity: **MINOR** (cosmetic, no functional impact)

### Location
- `src/main.cpp:737` - Phrase length soft-takeover init
- `src/main.cpp:735` - Reset mode soft-takeover init

### Problem

Config K4 and Config+Shift K1 are marked as unused in `ControlProcessor`, but their soft-takeover knobs are still initialized:

```cpp
// These controls are freed (no-op)
softKnobs[11].Init(controlState.resetMode);        // K4 - freed
softKnobs[12].Init(controlState.phraseLengthKnob); // Shift K1 - freed
```

### Impact

- Soft-takeover tracking happens but does nothing
- Minimal CPU overhead (a few comparisons per frame)
- No functional bug

### Recommendation

- Leave as-is for now (backward compatibility)
- If these knobs are repurposed, soft-takeover is already wired up
- Can clean up in future refactoring

---

## MINOR: Task 23 - Threshold Might Be Too Sensitive

### Severity: **MINOR** (user preference)

### Location
- `src/Engine/Sequencer.cpp:1147` - Field change threshold

### Problem

The threshold is hardcoded to 10% (0.1):

```cpp
static constexpr float kFieldChangeThreshold = 0.1f;
```

### Considerations

**Pro 10%**:
- Responsive to deliberate knob movements
- Unit tests verify it works

**Con 10%**:
- Might be too sensitive for noisy pots
- User wiggling knob slightly could trigger regeneration

### Recommendation

- **Test on hardware** with actual pots before changing
- If pots are noisy, consider 15% (0.15) threshold
- Could make configurable in future (config mode parameter)

---

## Task 23: Race Condition Analysis

### Location
`src/Engine/Sequencer.cpp:174-216` - ProcessAudio() step advancement

### Analysis

The field change detection and regeneration logic runs entirely within `ProcessAudio()`:

```cpp
// Line 179: Check for field changes
CheckFieldChange();  // Sets fieldChangeRegenPending_ flag

// Line 187: Beat boundary regeneration
static constexpr int kStepsPerBeat = 4;
const bool isBeatBoundary = (state_.sequencer.currentStep % kStepsPerBeat == 0);

if (fieldChangeRegenPending_ && isBeatBoundary && !state_.sequencer.isBarBoundary)
{
    BlendArchetype();
    GenerateBar();
    // ...
}
```

**No race condition** because:
1. All reads/writes happen in audio thread (single-threaded context)
2. Control updates from main loop happen before ProcessAudio() call
3. Flag is only read/written within ProcessAudio()
4. No concurrent access from multiple threads

### Verdict

**Safe** - Real-time audio implementation is correct.

---

## Task 24: Default Value Consistency

### Analysis

Checked all default values across:
- Struct initialization (`MainControlState` lines 155-181)
- Boot sequence (lines 640-665)
- `ControlState::Init()` (ControlState.h:326-348)

### Findings

All defaults are **consistent** except performance primary knobs (intentional):

| Parameter | Struct Default | Boot Override | ControlState::Init() | Status |
|-----------|---------------|---------------|----------------------|--------|
| energy | 0.6 | (not set) | 0.0 | ⚠️ Struct unused |
| build | 0.0 | (not set) | 0.0 | ⚠️ Struct unused |
| fieldX | 0.5 | (not set) | 0.5 | ⚠️ Struct unused |
| fieldY | 0.33 | (not set) | 0.33 | ⚠️ Struct unused |
| punch | 0.5 | 0.5 | 0.5 | ✅ Consistent |
| genre | 0.0 | 0.0 | Techno | ✅ Consistent |
| drift | 0.0 | 0.0 | 0.0 | ✅ Consistent |
| balance | 0.5 | 0.5 | 0.5 | ✅ Consistent |
| patternLength | - | 0.625f (32) | 32 | ✅ Consistent |
| swing | 0.5 | 0.5 | 0.5 | ✅ Consistent (Task 24 fix) |

The performance primary struct defaults are **intentionally unused** (see MEDIUM issue above).

### Verdict

Defaults are consistent where they need to be. Performance primary defaults are an architectural choice (see soft-takeover issue).

---

## Codex Analysis (gpt-5.2-codex)

**Model**: OpenAI Codex with xhigh reasoning effort
**Completed**: 2026-01-03, 5:55 PM

### Additional Critical Issues Found

Codex identified **two additional CRITICAL issues** that I missed:

#### CRITICAL #2: Reset Mode Still User-Controlled

**Location**: `src/main.cpp:334`, `src/main.cpp:505`

Task 22 claims Config K4 is "FREE" and reset mode is "hardcoded to STEP", but:
- `GetParameterPtr()` still maps Config K4 to `resetMode`
- `ProcessControls()` still calls `sequencer.SetResetMode()`
- User can still change reset mode via Config K4 knob

**Contradiction**: `ControlProcessor.cpp` ignores K4, but `main.cpp` still processes it!

#### CRITICAL #3: Phrase Length Still in Main Loop

**Location**: `src/main.cpp:343`, `src/main.cpp:511`, `src/main.cpp:520`

Config+Shift K1 is claimed "FREE", but:
- Still mapped in `GetParameterPtr()`
- Still processed and sent to `sequencer.SetPhraseLength()`
- Can trigger autosave flash writes (unnecessary wear)
- Creates user confusion (knob appears to do nothing)

### Additional Medium Issue

#### MEDIUM #3: Inconsistent Default Values

**Location**: `src/Engine/ControlState.h:320-324` vs `src/main.cpp:160-163`

`ControlState::Init()` and `MainControlState` have different defaults:

| Parameter | ControlState::Init() | MainControlState |
|-----------|---------------------|------------------|
| energy | 0.5 | 0.6 |
| fieldX | 0.0 | 0.5 |
| fieldY | 0.0 | 0.33 |

**Impact**: First bar is generated with `ControlState` defaults (0.0, 0.0) before main loop reads knobs, then abruptly changes on first control update. Creates unpredictable boot behavior.

### Additional Low Issues

#### LOW #3: Field Change Flag Not Cleared on Reset

**Location**: `src/Engine/Sequencer.cpp:671-679`

If user triggers reset while `fieldChangeRegenPending_` is set:
1. `TriggerReset()` regenerates immediately
2. Flag is NOT cleared
3. Next beat boundary triggers another regeneration
4. **Double regeneration** (especially visible in STEP reset mode)

#### LOW #4: CV Modulation Can Trigger Field Changes

**Location**: `src/Engine/Sequencer.cpp:1139-1143`

`CheckFieldChange()` uses `GetEffectiveFieldX/Y()` which includes CV modulation:
- CV noise can cause repeated regenerations every beat
- User didn't move knob, but CV wiggle triggers change
- May not be intended behavior (Task 23 focused on knob changes)

### Codex Questions

1. Should phrase progress everywhere use `GetDerivedPhraseLength()`, or keep `phraseLength` in sync when `patternLength` changes?
2. Do you want Config K4 and Config+Shift K1 fully removed from `main.cpp` UI path?
3. Should CV modulation count as a "field change" trigger, or knob-only detection?

---

## Summary Table

| Issue | Severity | Component | Impact | Fix Required |
|-------|----------|-----------|--------|--------------|
| Phrase length not auto-derived (DuoPulseState) | CRITICAL | DuoPulseState.h:133,153 | Wrong phrase length when pattern ≠ 32 | Yes - 2 lines |
| Reset mode still user-controlled | CRITICAL | main.cpp:334,505 | Config K4 NOT actually freed | Yes - remove mapping |
| Phrase length still in main loop | CRITICAL | main.cpp:343,511,520 | Config+Shift K1 NOT actually freed | Yes - remove mapping |
| Soft-takeover init with defaults | MEDIUM | main.cpp:719-725 | UX: knobs need sweep on boot | Consider fix |
| Inconsistent defaults (ControlState vs Main) | MEDIUM | ControlState.h:320 / main.cpp:160 | Unpredictable first bar | Yes - align defaults |
| Field change flag clearing | MEDIUM | Sequencer.cpp:202 | Intentional design | No - add comment |
| Field change flag on reset | LOW | Sequencer.cpp:671 | Double-regen edge case | Consider clearing |
| CV triggers field changes | LOW | Sequencer.cpp:1139 | Unintended regenerations | Design decision |
| Freed controls in soft-takeover | MINOR | main.cpp:737,735 | Cosmetic only | No |
| Threshold sensitivity | MINOR | Sequencer.cpp:1147 | User preference | Test first |

---

## Recommendations

### CRITICAL FIXES REQUIRED Before Hardware Testing

**Found 3 critical bugs that will cause incorrect behavior:**

1. **FIX CRITICAL #1**: Update `DuoPulseState.h` to use `GetDerivedPhraseLength()`
   - **File**: `src/Engine/DuoPulseState.h`
   - **Lines**: 133, 153
   - **Change**: Replace `controls.phraseLength` → `controls.GetDerivedPhraseLength()`
   - **Time**: 2 minutes
   - **Risk**: Very low (simple find/replace)

2. **FIX CRITICAL #2**: Remove reset mode from main.cpp control path
   - **File**: `src/main.cpp`
   - **Lines**: 334 (GetParameterPtr), 505 (ProcessControls)
   - **Action**: Remove reset mode mapping and sequencer call
   - **Time**: 5 minutes
   - **Risk**: Low (control already no-op in ControlProcessor)

3. **FIX CRITICAL #3**: Remove phrase length from main.cpp control path
   - **File**: `src/main.cpp`
   - **Lines**: 343 (GetParameterPtr), 511, 520 (ProcessControls)
   - **Action**: Remove phrase length mapping and sequencer call
   - **Time**: 5 minutes
   - **Risk**: Low (control already no-op in ControlProcessor)

4. **FIX MEDIUM #3**: Align default values between ControlState and MainControlState
   - **Files**: `src/Engine/ControlState.h:320-324`, `src/main.cpp:160-163`
   - **Action**: Set ControlState::Init() defaults to match MainControlState
   - **Time**: 2 minutes
   - **Risk**: Very low (alignment only)

5. **TEST**: Verify all pattern lengths work correctly
   - Use unit tests or basic hardware test
   - Ensure phrase arc feels consistent across 16/24/32/64 steps

### During Hardware Testing

3. **EVALUATE**: Soft-takeover boot behavior
   - Test with knobs at various positions on power-on
   - If frustrating, implement Option A (read hardware before init)

4. **MONITOR**: Field change threshold
   - Watch for unwanted regenerations from knob noise
   - If problematic, increase to 0.15 (15%)

### Post-Testing Cleanup

5. **DOCUMENT**: Add comments for freed controls and field change flag clearing
6. **CONSIDER**: Make threshold configurable if needed

---

## Files Requiring Changes

### Critical Fix
- `src/Engine/DuoPulseState.h` - Lines 133, 153

### Potential Improvements
- `src/main.cpp` - Boot sequence (if fixing soft-takeover)
- `src/Engine/Sequencer.cpp` - Add comment (line 202)

---

## Testing Checklist

After applying critical fix:
- [ ] Compile firmware (`make`)
- [ ] Run unit tests (`make test`)
- [ ] Test pattern length 16: Phrase should be 8 bars (128 steps)
- [ ] Test pattern length 24: Phrase should be 5 bars (120 steps)
- [ ] Test pattern length 32: Phrase should be 4 bars (128 steps)
- [ ] Test pattern length 64: Phrase should be 2 bars (128 steps)
- [ ] BUILD parameter should feel consistent across all pattern lengths

---

**Next Steps**:
1. Fix critical issue
2. Re-run tests
3. Proceed to hardware validation with QA doc
