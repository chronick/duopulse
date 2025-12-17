---
id: chronick/daisysp-idm-grids-13
title: "DuoPulse v3: Algorithmic Pulse Field"
status: "pending"
created_date: "2025-12-16"
last_updated: "2025-12-16"
owner: "user/ai"
spec_refs:
  - "pulse-field"
  - "voice-weights"
  - "broken-effects"
  - "phrase-modulation"
  - "fuse-balance"
  - "drift-control"
  - "couple-interlock"
  - "led-feedback"
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
- [ ] Create `PulseField.h` with weight tables for 32 steps. *(spec: [pulse-field])*
- [ ] Implement Anchor weight profile (emphasizes downbeats 0, 8, 16, 24). *(spec: [voice-weights])*
- [ ] Implement Shimmer weight profile (emphasizes backbeats 8, 24). *(spec: [voice-weights])*
- [ ] Implement `ShouldStepFire(step, density, broken)` function. *(spec: [pulse-field])*
- [ ] BROKEN flattens weight distribution (Lerp toward 0.5). *(spec: [pulse-field])*
- [ ] Noise injection scaled by BROKEN. *(spec: [pulse-field])*
- [ ] DENSITY sets threshold for firing. *(spec: [pulse-field])*
- [ ] Deterministic mode using seeded RNG for reproducibility. *(spec: [pulse-field])*

### Tests
- [ ] Unit tests for weight tables (correct values at positions).
- [ ] Unit tests for `ShouldStepFire` at various density/broken combinations.
- [ ] Verify BROKEN=0 produces regular patterns, BROKEN=1 produces random.

## Phase 2: DRIFT System [drift-control]

### Step Stability
- [ ] Implement step stability values (1.0 for bar downbeats → 0.2 for 16ths). *(spec: [drift-control])*
- [ ] Add `patternSeed_` (persists across loops for locked elements). *(spec: [drift-control])*
- [ ] Add `loopSeed_` (regenerates on phrase reset for drifting elements). *(spec: [drift-control])*
- [ ] DRIFT threshold determines which steps use locked vs. varying seed. *(spec: [drift-control])*

### Per-Voice DRIFT
- [ ] Anchor uses 0.7× drift multiplier (more stable). *(spec: [drift-control])*
- [ ] Shimmer uses 1.3× drift multiplier (more drifty). *(spec: [drift-control])*
- [ ] At DRIFT=100%, Anchor still has some stability (effective 70%). *(spec: [drift-control])*

### Phrase Reset
- [ ] `OnPhraseReset()` regenerates `loopSeed_`. *(spec: [drift-control])*
- [ ] Hook phrase reset callback into Sequencer. *(spec: [drift-control])*

### Tests
- [ ] DRIFT=0% produces identical pattern every loop.
- [ ] DRIFT=100% produces unique pattern each loop.
- [ ] Verify stratified stability: downbeats lock before ghost notes.

## Phase 3: BROKEN Effects Stack [broken-effects]

### Swing from BROKEN
- [ ] Implement `GetSwingFromBroken(broken)` function. *(spec: [broken-effects])*
  - 0-25%: Techno (50-54%)
  - 25-50%: Tribal (54-60%)
  - 50-75%: Trip-Hop (60-66%)
  - 75-100%: IDM (66-58% + jitter)
- [ ] Apply swing to off-beat steps. *(spec: [broken-effects])*

### Micro-Timing Jitter
- [ ] Implement `GetJitterMsFromBroken(broken)` function. *(spec: [broken-effects])*
  - 0-40%: 0ms
  - 40-70%: 0-3ms
  - 70-90%: 3-6ms
  - 90-100%: 6-12ms
- [ ] Apply jitter per-trigger. *(spec: [broken-effects])*

### Step Displacement
- [ ] Implement `GetDisplacedStep(step, broken)` function. *(spec: [broken-effects])*
  - 0-50%: no displacement
  - 50-75%: ±1 step (0-15% chance)
  - 75-100%: ±2 steps (15-40% chance)

### Velocity Variation
- [ ] Implement `GetVelocityWithVariation(baseVel, broken)` function. *(spec: [broken-effects])*
  - 0-30%: ±5%
  - 30-60%: ±10%
  - 60-100%: ±20%

### Tests
- [ ] Verify swing output at each BROKEN range.
- [ ] Verify jitter scaling with BROKEN.
- [ ] Verify step displacement at high BROKEN.

## Phase 4: Phrase Awareness [phrase-modulation]

### Reuse v2 Phrase Position
- [ ] Verify `PhrasePosition` struct compatibility. *(spec: [phrase-modulation])*
- [ ] Ensure `phraseProgress`, `isFillZone`, `isBuildZone`, `isDownbeat` are available.

### Phrase Weight Modulation
- [ ] Implement `GetPhraseWeightBoost(pos, broken)` function. *(spec: [phrase-modulation])*
  - Build zone (50-75%): subtle boost
  - Fill zone (75-100%): significant boost (0.15-0.25)
  - Genre scale: 0.5 + broken × 1.0
- [ ] Apply weight boost in fill/build zones. *(spec: [phrase-modulation])*

### Phrase BROKEN Modulation
- [ ] Implement `GetEffectiveBroken(broken, pos)` function. *(spec: [phrase-modulation])*
- [ ] Boost BROKEN by up to 20% in fill zone. *(spec: [phrase-modulation])*

### Downbeat Accent
- [ ] Implement `GetPhraseAccent(pos)` function. *(spec: [phrase-modulation])*
  - Phrase downbeat: 1.2×
  - Bar downbeat: 1.1×
  - Otherwise: 1.0×

### Tests
- [ ] Verify fill zone boost at phrase end.
- [ ] Verify downbeat accent multiplier.

## Phase 5: Voice Interaction [fuse-balance] [couple-interlock]

### FUSE Energy Balance
- [ ] Implement `ApplyFuse(fuse, anchorDensity, shimmerDensity)`. *(spec: [fuse-balance])*
  - fuse=0.5: no change
  - fuse<0.5: boost anchor, reduce shimmer
  - fuse>0.5: boost shimmer, reduce anchor
  - ±15% density shift at extremes

### COUPLE Interlock
- [ ] Implement `ApplyCouple(couple, anchorFires, shimmerFires, shimmerVel)`. *(spec: [couple-interlock])*
  - 0%: fully independent
  - 50%: soft interlock (collision avoidance)
  - 100%: hard interlock (shimmer fills anchor gaps)
- [ ] Collision suppression scales with COUPLE. *(spec: [couple-interlock])*
- [ ] Gap-filling boost at COUPLE > 50%. *(spec: [couple-interlock])*

### Tests
- [ ] Verify FUSE tilts energy between voices.
- [ ] Verify COUPLE suppresses collisions at high values.
- [ ] Verify COUPLE fills gaps at high values.

## Phase 6: Control Integration

### Control Mapping Updates
- [ ] K1 → ANCHOR DENSITY (unchanged). *(spec: control layout)*
- [ ] K2 → SHIMMER DENSITY (unchanged). *(spec: control layout)*
- [ ] K3 → BROKEN (replaces FLUX). *(spec: control layout)*
- [ ] K4 → DRIFT (new control). *(spec: control layout)*
- [ ] K1+Shift → FUSE (was K4 primary in v2). *(spec: control layout)*
- [ ] K2+Shift → LENGTH (unchanged). *(spec: control layout)*
- [ ] K3+Shift → COUPLE (replaces ORBIT). *(spec: control layout)*
- [ ] K4+Shift → Reserved for future. *(spec: control layout)*

### CV Input Mapping Updates
- [ ] CV 5 → ANCHOR DENSITY (unchanged).
- [ ] CV 6 → SHIMMER DENSITY (unchanged).
- [ ] CV 7 → BROKEN (was FLUX).
- [ ] CV 8 → DRIFT (was FUSE).

### Remove Deprecated Controls
- [ ] Remove FLUX parameter and references.
- [ ] Remove TERRAIN parameter (swing from BROKEN now).
- [ ] Remove GRID parameter (no pattern selection needed).
- [ ] Remove ORBIT parameter (replaced by COUPLE).
- [ ] Remove discrete genre enum (emerges from BROKEN).

### Config Mode Updates (Low-Priority)
- [ ] K1+Shift Config → SWING TASTE (fine-tune within BROKEN's range).
- [ ] K3+Shift Config → HUMANIZE (extra jitter on top of BROKEN's).

## Phase 7: LED Feedback Updates [led-feedback]

### Mode Indication
- [ ] Performance Mode: pulse on Anchor triggers.
- [ ] Config Mode: solid ON.
- [ ] Shift held: slower breathing (500ms cycle).

### Parameter Feedback
- [ ] DENSITY: brightness = level.
- [ ] BROKEN: flash rate increases with level.
- [ ] DRIFT: pulse regularity decreases with level.

### BROKEN × DRIFT Behavior
- [ ] Low BROKEN + Low DRIFT: regular, steady pulses.
- [ ] Low BROKEN + High DRIFT: regular timing, varying intensity.
- [ ] High BROKEN + Low DRIFT: irregular timing, consistent each loop.
- [ ] High BROKEN + High DRIFT: maximum irregularity.

### Phrase Position Feedback
- [ ] Downbeat: extra bright pulse.
- [ ] Fill Zone: rapid triple-pulse pattern.
- [ ] Build Zone: gradually increasing pulse rate.

## Phase 8: Migration & Cleanup

### Coexistence During Development
- [ ] Add `#ifdef USE_PULSE_FIELD_V3` conditional compilation.
- [ ] Verify old pattern system still works when disabled.
- [ ] Remove pattern skeletons after v3 validated.

### File Structure
- [ ] Create `src/Engine/PulseField.h` and `PulseField.cpp`.
- [ ] Create `src/Engine/BrokenEffects.h` (swing, jitter, displacement).
- [ ] Update `src/Engine/Sequencer.h` and `.cpp` for v3 integration.
- [ ] Remove unused pattern data files after migration.

### Documentation
- [ ] Update control layout in spec.
- [ ] Document BROKEN × DRIFT interaction.
- [ ] Update README with new control descriptions.

## Notes

- The v3 algorithm can coexist with v2 during development using conditional compilation.
- Phrase position tracking from v2 is reused.
- Soft takeover behavior from v2 is reused (just with different parameters).
- External clock handling (Task 08) and Aux Output Modes (Task 12) are independent and compatible with v3.
