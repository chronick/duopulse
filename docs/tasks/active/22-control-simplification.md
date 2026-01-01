# Task 22: Control Simplification

**Status**: READY (Refined 2025-12-30)
**Branch**: feature/control-simplification
**Parent Task**: Task 16 (Hardware Validation)
**Related**: Task 21 (Musicality), Task 23 (Field Updates), Task 24 (Power-On)

---

## Problem Statement

Hardware validation (Task 16) revealed config mode controls that are unnecessary or confusing:

1. **Reset Mode (Config K4)** - Not useful, remove
2. **Phrase Length (Config+Shift K1)** - Confusing interaction with Pattern Length, auto-derive instead
3. **Voice Coupling** - INTERLOCK mode feels broken, simplify to 2 modes
4. **Swing** - Doesn't noticeably affect timing, defer to Task 21 investigation

---

## Refined Approach: Incremental Phases

Instead of a big-bang redesign, we'll make small, safe, independently testable changes.

### Phase A: Easy Wins (Config UI Cleanup)
Remove unnecessary config knobs by defaulting internally. No code architecture changes.

| Change | Risk | Benefit |
|--------|------|---------|
| Remove Reset Mode UI | None | Frees Config K4 |
| Auto-derive Phrase Length | Low | Frees Config+Shift K1, simpler UX |

### Phase B: Balance Range Extension
Extend current balance range instead of architectural VOICE redesign.

| Current | New | Change |
|---------|-----|--------|
| Shimmer 30-100% of anchor | Shimmer 0-150% of anchor | One line in HitBudget.cpp |

### Phase C: Coupling Simplification
Remove broken INTERLOCK mode, keep INDEPENDENT and SHADOW.

| Current | New |
|---------|-----|
| 3 modes (INDEPENDENT/INTERLOCK/SHADOW) | 2 modes (INDEPENDENT/SHADOW) |

### Phase D: Swing (Deferred)
Wait for Task 21 spike to determine if swing is worth fixing or should be replaced.

---

## Phase A: Easy Wins

### A1: Remove Reset Mode from Config UI

**Current**: Config K4 primary = Reset Mode (PHRASE/BAR/STEP)
**New**: Remove from UI, hardcode to STEP

**Files to modify**:
- `src/Engine/ControlProcessor.cpp`: Skip reset mode processing, always use STEP
- `src/Engine/ControlState.h`: Keep field but don't expose in UI

**Implementation**:
```cpp
// ControlProcessor.cpp - ProcessConfigPrimary()
// BEFORE:
float resetModeRaw = configPrimaryKnobs_[3].Process(input.knobs[3]);
ResetMode newResetMode = GetResetModeFromValue(resetModeRaw);

// AFTER:
// Config K4 primary is now FREE - don't process reset mode
// Reset mode is hardcoded to STEP in ControlState::Init()
(void)input.knobs[3];  // K4 primary unused in config mode
```

**Test**: Reset input should always reset to step 0.

---

### A2: Auto-Derive Phrase Length from Pattern Length

**Current**: Pattern Length (Config K1) and Phrase Length (Config+Shift K1) are separate
**New**: Phrase Length auto-derived: `phraseBars = 8 / (patternSteps / 16)`

| Pattern Steps | Bars per Pattern | Phrase Bars | Total Steps |
|---------------|------------------|-------------|-------------|
| 16 | 1 | 8 | 128 |
| 24 | 1.5 | 5 | 120 |
| 32 | 2 | 4 | 128 |
| 64 | 4 | 2 | 128 |

**Rationale**: Keep total phrase around 128 steps (8 bars at 16th notes) for consistent phrase arc timing.

**Files to modify**:
- `src/Engine/ControlProcessor.cpp`: Remove phrase length processing
- `src/Engine/ControlState.h`: Derive `phraseLength` from `patternLength`

**Implementation**:
```cpp
// Add to ControlState
int GetDerivedPhraseLength() const {
    // Target ~128 steps total, minimum 2 bars
    switch (patternLength) {
        case 16: return 8;   // 16 × 8 = 128 steps
        case 24: return 5;   // 24 × 5 = 120 steps
        case 32: return 4;   // 32 × 4 = 128 steps
        case 64: return 2;   // 64 × 2 = 128 steps
        default: return 4;
    }
}
```

**Test**: Changing pattern length should automatically adjust phrase behavior.

---

## Phase B: Balance Range Extension

### B1: Extend Shimmer Ratio Range

**Current** (`HitBudget.cpp:113-114`):
```cpp
float shimmerRatio = 0.3f + balance * 0.7f;  // 30% to 100%
```

**New**:
```cpp
float shimmerRatio = balance * 1.5f;  // 0% to 150%
```

| Balance | Current Shimmer | New Shimmer |
|---------|-----------------|-------------|
| 0% | 30% of anchor | 0% (silent) |
| 50% | 65% of anchor | 75% of anchor |
| 100% | 100% of anchor | 150% of anchor |

**Files to modify**:
- `src/Engine/HitBudget.cpp`: Change shimmer ratio formula

**Test**:
- Balance 0% should produce no shimmer triggers
- Balance 100% should produce more shimmer than anchor

**Dependency**: Task 21 Phase C (Hit Budget Expansion) ships first and implements this change along with zone-aware shimmer capping. This phase is **complete when Task 21 ships**.

---

## Phase C: Coupling Simplification

### C1: Remove INTERLOCK Mode

User feedback: "LOCKED... trigs are not technically 1:1"

**Current**: INDEPENDENT (0-33%), INTERLOCK (33-67%), SHADOW (67-100%)
**New**: INDEPENDENT (0-50%), SHADOW (50-100%)

**Files to modify**:
- `src/Engine/DuoPulseTypes.h`: Remove INTERLOCK from enum (or keep but don't use)
- `src/Engine/VoiceRelation.cpp`: Remove INTERLOCK case
- `src/Engine/ControlProcessor.cpp`: Map knob to 2 modes instead of 3

**Implementation**:
```cpp
// DuoPulseTypes.h
inline VoiceCoupling GetVoiceCouplingFromValue(float value) {
    if (value < 0.5f) return VoiceCoupling::INDEPENDENT;
    return VoiceCoupling::SHADOW;
}
```

**Test**: Coupling knob should switch cleanly between independent and shadow modes.

---

## Phase D: Swing (Deferred to Task 21)

Task 21 is investigating why swing doesn't feel impactful. Possible outcomes:

1. **Swing is broken** → Fix implementation
2. **Swing range too narrow** → Widen 50-66% to 50-75%
3. **GENRE already handles it** → Remove swing, let GENRE control timing

**Decision**: Wait for Task 21 findings before changing swing.

---

## Freed Controls Summary

After Phase A-C:

| Freed Control | Previous Use | Available For |
|---------------|--------------|---------------|
| Config K4 Primary | Reset Mode | Future feature or leave empty |
| Config+Shift K1 | Phrase Length | Future feature or leave empty |

---

## Implementation Order

1. **A1**: Remove Reset Mode UI (safest, no behavior change) ✓ **COMPLETE**
2. **A2**: Auto-derive Phrase Length (low risk, simplifies UX) ✓ **COMPLETE**
3. **B1**: Extend Balance Range (medium risk, changes musical behavior)
4. **C1**: Remove INTERLOCK (low risk, removes broken feature) ✓ **COMPLETE**

Each phase should be a separate commit, tested on hardware before proceeding.

---

## Success Criteria

- [x] Config mode has fewer confusing options (A1: Reset Mode removed, A2: Phrase Length auto-derived)
- [x] Balance 0% produces anchor-only patterns (B1: Balance range 0-150% via Task 21)
- [x] Balance 100% produces shimmer-heavy patterns (B1: Balance range 0-150% via Task 21)
- [x] Coupling switches cleanly between INDEPENDENT and SHADOW (C1: INTERLOCK removed)
- [x] No regressions in existing functionality (All 229 tests pass with 51554 assertions)
- [ ] All changes tested on hardware (Requires physical device - TODO after commit)

---

## Appendix: Original Critical Review (2025-12-30)

The original Task 22 proposal had these issues that led to this refined approach:

1. **VOICE Control was too ambitious**: Required architectural rewrite of HitBudget. Refined to simple balance range extension.

2. **Pattern/Phrase math was wrong**: Original said "64 steps = 2 bars". Corrected: 64 steps = 4 bars at 16th notes.

3. **Phase ordering would break build**: Original removed controls before updating code. Refined to update code in each phase.

4. **VoiceCoupling had no migration plan**: Original merged coupling into VOICE. Refined to just remove broken INTERLOCK mode.

5. **Sub-tasks 25-41 were too granular**: Created tracking confusion. Refined to 4 logical phases (A-D).
