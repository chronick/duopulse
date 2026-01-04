---
id: 13
slug: pulse-field-v3
title: DuoPulse v3: Algorithmic Pulse Field
status: completed
created_date: 2025-12-16
updated_date: 2025-12-17
completed_date: 2025-12-17
branch: feature/pulse-field-v3
spec_refs:
  - pulse-field
  - voice-weights
  - broken-effects
  - phrase-modulation
  - fuse-balance
  - drift-control
  - couple-interlock
  - led-feedback
---

# Feature: DuoPulse v3 — Algorithmic Pulse Field [pulse-field-v3]

## Context

DuoPulse v3 replaces the discrete 16-pattern lookup system with a continuous **algorithmic pulse field**. The core mental model is simplified to two axes:

1. **BROKEN** — How regular/irregular the pattern is (4/4 techno → IDM chaos)
2. **DRIFT** — How much the pattern evolves over time (locked → generative)

This eliminates Terrain/Grid mismatches, reduces cognitive load, and provides infinite variation along a musically coherent gradient. Genre character (Techno/Tribal/Trip-Hop/IDM) emerges naturally from the BROKEN parameter.

**Key Changes from v2:**
- FLUX removed → replaced by DRIFT
- TERRAIN removed → genre emerges from BROKEN
- ORBIT (3 modes) simplified → COUPLE (0-100%)
- Pattern skeletons → weighted pulse field algorithm
- Swing tied to BROKEN instead of separate genre settings

## Phase 1: Core Algorithm [pulse-field]

### Weight Tables & Core Logic
- [x] Create `PulseField.h` with weight tables for 32 steps. *(spec: [pulse-field])*
- [x] Implement Anchor weight profile (emphasizes downbeats 0, 8, 16, 24). *(spec: [voice-weights])*
- [x] Implement Shimmer weight profile (emphasizes backbeats 8, 24). *(spec: [voice-weights])*
- [x] Implement `ShouldStepFire(step, density, broken)` function. *(spec: [pulse-field])*
- [x] BROKEN flattens weight distribution (Lerp toward 0.5). *(spec: [pulse-field])*
- [x] Noise injection scaled by BROKEN. *(spec: [pulse-field])*
- [x] DENSITY sets threshold for firing. *(spec: [pulse-field])*
- [x] Deterministic mode using seeded RNG for reproducibility. *(spec: [pulse-field])*

### Tests
- [x] Unit tests for weight tables (correct values at positions).
- [x] Unit tests for `ShouldStepFire` at various density/broken combinations.
- [x] Verify BROKEN=0 produces regular patterns, BROKEN=1 produces random.

## Phase 2: DRIFT System [drift-control]

### Step Stability
- [x] Implement step stability values (1.0 for bar downbeats → 0.2 for 16ths). *(spec: [drift-control])*
- [x] Add `patternSeed_` (persists across loops for locked elements). *(spec: [drift-control])*
- [x] Add `loopSeed_` (regenerates on phrase reset for drifting elements). *(spec: [drift-control])*
- [x] DRIFT threshold determines which steps use locked vs. varying seed. *(spec: [drift-control])*

### Per-Voice DRIFT
- [x] Anchor uses 0.7× drift multiplier (more stable). *(spec: [drift-control])*
- [x] Shimmer uses 1.3× drift multiplier (more drifty). *(spec: [drift-control])*
- [x] At DRIFT=100%, Anchor still has some stability (effective 70%). *(spec: [drift-control])*

### Phrase Reset
- [x] `OnPhraseReset()` regenerates `loopSeed_`. *(spec: [drift-control])*
- [x] Hook phrase reset callback into Sequencer. *(spec: [drift-control])*
  - Implemented: `pulseFieldState_.OnPhraseReset()` called when `stepIndex_` wraps to 0 in `ProcessAudio()`.

### Tests
- [x] DRIFT=0% produces identical pattern every loop.
- [x] DRIFT=100% produces unique pattern each loop.
- [x] Verify stratified stability: downbeats lock before ghost notes.

## Phase 3: BROKEN Effects Stack [broken-effects]

### Swing from BROKEN
- [x] Implement `GetSwingFromBroken(broken)` function. *(spec: [broken-effects])*
  - 0-25%: Techno (50-54%)
  - 25-50%: Tribal (54-60%)
  - 50-75%: Trip-Hop (60-66%)
  - 75-100%: IDM (66-58% + jitter)
- [x] Apply swing to off-beat steps. *(spec: [broken-effects])*
  - Added `IsOffBeat(step)` helper function

### Micro-Timing Jitter
- [x] Implement `GetJitterMsFromBroken(broken)` function. *(spec: [broken-effects])*
  - 0-40%: 0ms
  - 40-70%: 0-3ms
  - 70-90%: 3-6ms
  - 90-100%: 6-12ms
- [x] Apply jitter per-trigger. *(spec: [broken-effects])*
  - Added `ApplyJitter(maxJitterMs, seed, step)` function

### Step Displacement
- [x] Implement `GetDisplacedStep(step, broken)` function. *(spec: [broken-effects])*
  - 0-50%: no displacement
  - 50-75%: ±1 step (0-15% chance)
  - 75-100%: ±2 steps (15-40% chance)

### Velocity Variation
- [x] Implement `GetVelocityWithVariation(baseVel, broken)` function. *(spec: [broken-effects])*
  - 0-30%: ±5%
  - 30-60%: ±10%
  - 60-100%: ±20%
  - Added `GetVelocityVariationRange(broken)` helper for testing/display

### Tests
- [x] Verify swing output at each BROKEN range.
- [x] Verify jitter scaling with BROKEN.
- [x] Verify step displacement at high BROKEN.
  - Note: All tests implemented in `tests/test_broken_effects.cpp`

## Phase 4: Phrase Awareness [phrase-modulation]

### Reuse v2 Phrase Position
- [x] Verify `PhrasePosition` struct compatibility. *(spec: [phrase-modulation])*
  - Verified in `GenreConfig.h` (lines 367-377): struct has all required fields
- [x] Ensure `phraseProgress`, `isFillZone`, `isBuildZone`, `isDownbeat` are available.
  - All fields present with correct types and default values
  - `CalculatePhrasePosition()` helper function computes all values correctly
  - `Sequencer::GetPhrasePosition()` getter already exposes the struct

### Phrase Weight Modulation
- [x] Implement `GetPhraseWeightBoost(pos, broken)` function. *(spec: [phrase-modulation])*
  - Build zone (50-75%): subtle boost (0 to 0.075)
  - Fill zone (75-100%): significant boost (0.15 to 0.25)
  - Genre scale: 0.5 + broken × 1.0
- [x] Apply weight boost in fill/build zones. *(spec: [phrase-modulation])*
  - Implemented in `BrokenEffects.h`

### Phrase BROKEN Modulation
- [x] Implement `GetEffectiveBroken(broken, pos)` function. *(spec: [phrase-modulation])*
- [x] Boost BROKEN by up to 20% in fill zone. *(spec: [phrase-modulation])*
  - fillProgress scales from 0 to 1.0, boost = fillProgress × 0.2

### Downbeat Accent
- [x] Implement `GetPhraseAccent(pos)` function. *(spec: [phrase-modulation])*
  - Phrase downbeat (stepInPhrase == 0): 1.2×
  - Bar downbeat: 1.1×
  - Otherwise: 1.0×

### Tests
- [x] Verify fill zone boost at phrase end.
- [x] Verify downbeat accent multiplier.
  - All tests in `tests/test_broken_effects.cpp` under [phrase-modulation] tag

## Phase 5: Voice Interaction [fuse-balance] [couple-interlock]

### FUSE Energy Balance
- [x] Implement `ApplyFuse(fuse, anchorDensity, shimmerDensity)`. *(spec: [fuse-balance])*
  - fuse=0.5: no change
  - fuse<0.5: boost anchor, reduce shimmer (bias = -0.15)
  - fuse>0.5: boost shimmer, reduce anchor (bias = +0.15)
  - ±15% density shift at extremes

### COUPLE Interlock
- [x] Implement `ApplyCouple(couple, anchorFires, shimmerFires, shimmerVel)`. *(spec: [couple-interlock])*
  - 0-10%: fully independent (no interaction)
  - 10-100%: collision suppression (up to 80% at max)
  - 50-100%: gap-filling boost (up to 30% at max)
- [x] Collision suppression scales with COUPLE. *(spec: [couple-interlock])*
  - suppressChance = couple × 0.8
- [x] Gap-filling boost at COUPLE > 50%. *(spec: [couple-interlock])*
  - boostChance = (couple - 0.5) × 0.6
  - Fill velocity: 0.5 to 0.8 (medium)

### Tests
- [x] Verify FUSE tilts energy between voices.
- [x] Verify COUPLE suppresses collisions at high values.
- [x] Verify COUPLE fills gaps at high values.
  - All tests in `tests/test_broken_effects.cpp` under [fuse-balance] and [couple-interlock] tags

## Phase 6: Control Integration

### Control Mapping Updates
- [x] K1 → ANCHOR DENSITY (unchanged). *(spec: control layout)*
- [x] K2 → SHIMMER DENSITY (unchanged). *(spec: control layout)*
- [x] K3 → BROKEN (replaces FLUX). *(spec: control layout)*
- [x] K4 → DRIFT (new control). *(spec: control layout)*
- [x] K1+Shift → FUSE (was K4 primary in v2). *(spec: control layout)*
- [x] K2+Shift → LENGTH (unchanged). *(spec: control layout)*
- [x] K3+Shift → COUPLE (replaces ORBIT). *(spec: control layout)*
- [x] K4+Shift → Reserved for future. *(spec: control layout)*

### CV Input Mapping Updates
- [x] CV 5 → ANCHOR DENSITY (unchanged).
- [x] CV 6 → SHIMMER DENSITY (unchanged).
- [x] CV 7 → BROKEN (was FLUX).
- [x] CV 8 → DRIFT (was FUSE).

### Remove Deprecated Controls
- [x] Remove FLUX parameter and references.
  - Kept as deprecated setter that maps to SetBroken()
- [x] Remove TERRAIN parameter (swing from BROKEN now).
  - Kept as deprecated, BROKEN now controls terrain internally
- [x] Remove GRID parameter (no pattern selection needed).
  - Kept as deprecated setter for backward compatibility
- [x] Remove ORBIT parameter (replaced by COUPLE).
  - Kept as deprecated setter that maps to SetCouple()
- [x] Remove discrete genre enum (emerges from BROKEN).
  - Genre still calculated internally from BROKEN/terrain for swing

### Config Mode Updates (Low-Priority)
- [x] K1+Shift Config → SWING TASTE (fine-tune within BROKEN's range).
- [x] K3+Shift Config → HUMANIZE (extra jitter on top of BROKEN's).

## Phase 7: LED Feedback Updates [led-feedback]

### Mode Indication
- [x] Performance Mode: pulse on Anchor triggers.
- [x] Config Mode: solid ON.
- [x] Shift held: slower breathing (500ms cycle).

### Parameter Feedback
- [x] DENSITY: brightness = level.
- [x] BROKEN: flash rate increases with level.
- [x] DRIFT: pulse regularity decreases with level.

### BROKEN × DRIFT Behavior
- [x] Low BROKEN + Low DRIFT: regular, steady pulses.
- [x] Low BROKEN + High DRIFT: regular timing, varying intensity.
- [x] High BROKEN + Low DRIFT: irregular timing, consistent each loop.
- [x] High BROKEN + High DRIFT: maximum irregularity.

### Phrase Position Feedback
- [x] Downbeat: extra bright pulse.
- [x] Fill Zone: rapid triple-pulse pattern.
- [x] Build Zone: gradually increasing pulse rate.

> **Implementation Note** (2025-12-17): LED feedback fully implemented in `LedIndicator.h` with `LedState` struct for state management. Tests in `tests/test_led_indicator.cpp`.

## Phase 8: Migration & Cleanup

### Coexistence During Development
- [x] Add `#ifdef USE_PULSE_FIELD_V3` conditional compilation.
  - Added to `inc/config.h` with documentation
- [x] Verify old pattern system still works when disabled.
  - Tests pass with v3 both enabled and disabled
- [ ] ~~Remove pattern skeletons after v3 validated.~~ **DEFERRED**
  - Intentionally deferred: keep v2 code for fallback until hardware validation complete
  - The `#ifdef USE_PULSE_FIELD_V3` conditional allows easy switching between v2/v3

### File Structure
- [x] Create `src/Engine/PulseField.h` and `PulseField.cpp`.
  - PulseField.h implemented (header-only)
- [x] Create `src/Engine/BrokenEffects.h` (swing, jitter, displacement).
  - BrokenEffects.h implemented (header-only)
- [x] Update `src/Engine/Sequencer.h` and `.cpp` for v3 integration.
  - Added PulseFieldState, GetPulseFieldTriggers(), ApplyBrokenEffects()
  - Conditional compilation throughout ProcessAudio()
  - UpdateSwingParameters() uses GetSwingFromBroken() for v3
- [ ] ~~Remove unused pattern data files after migration.~~ **DEFERRED**
  - Intentionally deferred: keep PatternSkeleton/PatternData for v2 fallback mode

### Documentation
- [x] Update control layout in spec.
  - Acceptance criteria updated for pulse-field, broken-effects, drift-control, fuse-balance, couple-interlock
- [x] Document BROKEN × DRIFT interaction.
  - Added to README and spec
- [x] Update README with new control descriptions.
  - Added comprehensive DuoPulse v3 section to README

## Notes

- The v3 algorithm can coexist with v2 during development using conditional compilation.
- Phrase position tracking from v2 is reused.
- Soft takeover behavior from v2 is reused (just with different parameters).
- External clock handling (Task 08) and Aux Output Modes (Task 12) are independent and compatible with v3.

> **Implementation Note** (2025-12-17): Task 13 complete:
> - v3 algorithm integrated into Sequencer via `#ifdef USE_PULSE_FIELD_V3`
> - Phase 2 phrase reset callback hooked in Sequencer.cpp (calls `OnPhraseReset()` when step wraps to 0)
> - Both v2 and v3 modes pass all 124 test cases (76,532 assertions)
> - Documentation updated (spec acceptance criteria, README)
> - Pattern skeleton removal intentionally deferred until hardware validation complete

---

### Session Notes (2025-12-17)

**Phrase Reset Callback Implementation:**
- Added `pulseFieldState_.OnPhraseReset()` call in `Sequencer::ProcessAudio()` when `stepIndex_` wraps to 0
- Wrapped in `#ifdef USE_PULSE_FIELD_V3` for conditional compilation
- This enables DRIFT-affected steps to produce different patterns each loop (loopSeed_ regeneration)

**Tests Added:**
- `test_sequencer.cpp`: "Phrase reset triggers loopSeed regeneration in v3" - verifies pattern behavior with DRIFT
- `test_sequencer.cpp`: "Phrase reset at step 0 verified via phrase position" - verifies loop wrap detection

**Files Modified:**
- `src/Engine/Sequencer.cpp` - phrase reset callback
- `tests/test_sequencer.cpp` - integration tests
- `docs/tasks/completed/13-pulse-field-v3.md` - task updates
- `docs/tasks/index.md` - status update
