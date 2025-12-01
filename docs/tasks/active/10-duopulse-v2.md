---
id: chronick/daisysp-idm-grids-10
title: "DuoPulse v2: 2-Voice Percussive Sequencer"
status: "todo"
created_date: "2025-12-01"
last_updated: "2025-12-01"
owner: "user/ai"
supersedes: "07-opinionated-sequencer"
---

# Feature: DuoPulse v2 [duopulse-v2]

## Context

DuoPulse is an expansive update to the existing opinionated drum sequencer, evolving it into a genre-aware, phrase-conscious 2-voice percussion engine. This update introduces abstract terminology (Anchor/Shimmer), genre-based swing, voice relationship modes, CV expression modes, phrase structure awareness, and a shift-layer control scheme that provides 16 distinct parameters across 4 control modes.

The implementation builds on the existing codebase (`Sequencer`, `PatternGenerator`, `SoftKnob`, `GateScaler`, `ChaosModulator`) rather than rewriting from scratch.

## Tasks

### Phase 1: Control System Overhaul

- [x] **Update ControlState structure** — Add all 16 parameters (4 modes × 4 knobs) to ControlState in `main.cpp`. Rename existing parameters to new vocabulary (anchorDensity, shimmerDensity, flux, fuse, etc.). Reference: `docs/specs/main.md` section "Control System [duopulse-controls]".
  - *Completed 2025-12-01*: Added ControlMode enum, 16-parameter ControlState, expanded SoftKnob array to 16 slots, CV always modulates performance params.

- [x] **Implement shift layer detection** — Track button hold duration to distinguish tap (<150ms = fill trigger) from hold (>150ms = shift active). Manage 4 separate SoftKnob sets for each mode/shift combination. Reference: `docs/specs/main.md` section "Control System [duopulse-controls]".
  - *Completed 2025-12-01*: Added timing state vars, shift detection logic. Short tap = tap tempo, hold = shift layer.

- [x] **Update CV routing for always-performance** — CV inputs 5-8 always modulate performance parameters (Anchor Density, Shimmer Density, Flux, Fuse) regardless of current mode. Additive modulation centered at 2.5V. Reference: `docs/specs/main.md` section "CV Input Behavior [duopulse-cv]".
  - *Completed 2025-12-01*: Implemented in Task 1 - CV always modulates performance params with 0.5 center.

### Phase 2: Sequencer Engine Updates

- [x] **Expand Sequencer interface** — Add new setters: `SetAnchorDensity`, `SetShimmerDensity`, `SetFlux`, `SetFuse`, `SetAnchorAccent`, `SetShimmerAccent`, `SetOrbit`, `SetContour`, `SetTerrain`, `SetSwingTaste`, `SetGateTime`, `SetHumanize`, `SetClockDiv`. All apply immediately. Reference: `docs/specs/main.md` section "Parameter Change Behavior [duopulse-immediate]".
  - *Completed 2025-12-01*: Full DuoPulse v2 interface with 16 setters. Legacy aliases for backward compat.

- [x] **Update tempo range** — Change kMinTempo from 30 to 90, kMaxTempo from 200 to 160. Reference: `docs/specs/main.md` Technical Specifications.
  - *Completed 2025-12-01*: Updated in Sequencer.h. Tests updated accordingly.

- [x] **Implement genre-aware swing** — Create SwingRange struct with min/max per genre (Techno 52-57%, Tribal 56-62%, Trip-Hop 60-68%, IDM 54-65%+jitter). Calculate effective swing from terrain + swingTaste. Reference: `docs/specs/main.md` section "Genre-Aware Swing [duopulse-swing]".
  - *Completed 2025-12-01*: Added GenreConfig.h with swing ranges. Anchor gets 70% swing, Shimmer 100%.

- [x] **Implement swung clock output** — Clock output (CV Out 1) respects swing timing. Fire clock triggers at swing-adjusted positions. Reference: `docs/specs/main.md` section "Genre-Aware Swing [duopulse-swing]".
  - *Completed 2025-12-01*: Clock triggers queued with swing delay alongside voice triggers.

- [x] **Implement Orbit voice relationship modes** — Interlock (0-33%): shimmer fills gaps. Free (33-67%): independent. Shadow (67-100%): shimmer echoes anchor with delay. Reference: `docs/specs/main.md` section "Orbit Voice Relationships [duopulse-orbit]".
  - *Completed 2025-12-01*: Interlock modifies shimmer prob ±30%, Shadow echoes at 70% velocity.

- [x] **Implement Humanize micro-timing jitter** — Add random ±0-10ms jitter to trigger timing based on humanize parameter. IDM terrain adds up to 30% extra. Reference: `docs/specs/main.md` section "Humanize Timing [duopulse-humanize]".
  - *Completed 2025-12-01*: Xorshift RNG for jitter, combined with swing delay. IDM adds 30% extra.

- [x] **Implement fill system** — High FLUX values (via knob or CV) increase fill probability. Fill zones at phrase boundaries get additional boost. Reference: `docs/specs/main.md` section "CV-Driven Fills [duopulse-fills]".
  - *Completed 2025-12-01*: Fills trigger at FLUX>50%, up to 30% probability at FLUX=100%.

### Phase 3: Pattern Generator Rewrite

- [x] **Define new PatternSkeleton structure** — 32-step patterns with anchor/shimmer skeletons, accent mask, genre affinity. Reference: `docs/specs/main.md` section "Pattern Generation [duopulse-patterns]".
  - *Completed 2025-12-01*: Created `PatternSkeleton.h` with packed 4-bit intensity format, helper functions (GetStepIntensity, ShouldStepFire, etc.), GenreAffinity bitfield, and IntensityLevel classification.

- [x] **Create 16 skeleton patterns** — Optimized for 2-voice output. Include techno four-on-floor, minimal, driving, tribal interlocking, breakbeat, trip-hop sparse, IDM irregular patterns. Reference: Implementation Roadmap Phase 3.
  - *Completed 2025-12-01*: Created `PatternData.h` with 16 patterns: 4 Techno (four-on-floor, minimal, driving, pounding), 4 Tribal (clave, interlocking, poly, circular), 4 Trip-Hop (sparse, lazy, heavy, groove), 4 IDM (broken, glitch, irregular, chaos).

- [x] **Implement density threshold system** — Density controls threshold against pattern intensity. Low density = only high-intensity steps. High density = all steps including ghosts. Reference: `docs/specs/main.md` section "Pattern Generation [duopulse-patterns]".
  - *Completed 2025-12-01*: Added `GetSkeletonTriggers()` method using `ShouldStepFire()` with density threshold. Pattern index from `grid_` parameter. Legacy Grids system kept for backward compat via `SetUseSkeletonPatterns()`.

- [x] **Implement FLUX probabilistic variation** — FLUX adds fill chance (30% max), ghost chance (50% max), velocity jitter (20%), timing jitter (IDM only). Reference: `docs/specs/main.md` section "Pattern Generation [duopulse-patterns]".
  - *Completed 2025-12-01*: Added `CalculateGhostProbability()`, `ShouldTriggerGhost()`, `ApplyVelocityJitter()`. Ghost-level steps (1-4 intensity) can fire via FLUX even below density threshold.

- [x] **Implement FUSE energy tilt** — CCW boosts anchor density, CW boosts shimmer density. Reference: `docs/specs/main.md` section "Control System [duopulse-controls]".
  - *Completed 2025-12-01*: Already implemented. fuseBias = (fuse - 0.5) * 0.3 applies ±15% density tilt. CCW boosts anchor, CW boosts shimmer.

### Phase 4: Phrase Structure

- [x] **Implement PhrasePosition tracking** — Track currentBar, stepInBar, stepInPhrase, phraseProgress, isLastBar, isFillZone, isBuildZone, isDownbeat. Reference: `docs/specs/main.md` section "Phrase Structure [duopulse-phrase]".
  - *Completed 2025-12-01*: Full phrase tracking with fill/build zones scaling by pattern length.

- [x] **Implement phrase modulation parameters** — Modulate fill probability, ghost probability, syncopation, accent intensity, velocity variance based on phrase position. Reference: `docs/specs/main.md` section "Phrase Structure [duopulse-phrase]".
  - *Completed 2025-12-01*: Fill/ghost boost + accent multiplier based on phrase position and genre.

- [x] **Implement genre-specific phrase scaling** — Techno: 50% (subtle), Tribal: 120% (pronounced), Trip-Hop: 70% (sparse), IDM: 150% (extreme). Reference: `docs/specs/main.md` section "Phrase Structure [duopulse-phrase]".
  - *Completed 2025-12-01*: Genre scaling in GetPhraseFillBoost() matches spec percentages.

- [x] **Implement pattern length zone scaling** — Fill zone and build zone lengths scale with pattern length (1-16 bars). Reference: `docs/specs/main.md` section "Phrase Structure [duopulse-phrase]".
  - *Completed 2025-12-01*: Fill zone = 4 steps/bar, build zone = 8 steps/bar, capped at 32/64.

### Phase 5: CV Expression (Contour)

- [x] **Implement Velocity contour mode** — CV = hit intensity (0-5V), slight hold between triggers. Reference: `docs/specs/main.md` section "Contour CV Modes [duopulse-contour]".
  - *Completed 2025-12-01*: Pass-through velocity with 95% decay between triggers.

- [x] **Implement Decay contour mode** — CV hints decay time, applies decay envelope to output. Reference: `docs/specs/main.md` section "Contour CV Modes [duopulse-contour]".
  - *Completed 2025-12-01*: High velocity = high CV (long decay hint), 85% decay rate.

- [x] **Implement Pitch contour mode** — CV = random pitch offset scaled by velocity. Reference: `docs/specs/main.md` section "Contour CV Modes [duopulse-contour]".
  - *Completed 2025-12-01*: ±0.2V range at max velocity, centered at 2.5V.

- [x] **Implement Random contour mode** — Sample & Hold random voltage on each trigger. Reference: `docs/specs/main.md` section "Contour CV Modes [duopulse-contour]".
  - *Completed 2025-12-01*: S&H random 0-5V on trigger, holds until next.

### Phase 6: Soft Pickup Enhancement

- [x] **Enhance SoftKnob gradual interpolation** — Implement 10% per cycle interpolation toward target. Cross-detection for immediate catchup. Prevent jumps on mode/shift switch. Reference: `docs/specs/main.md` section "Soft Takeover [duopulse-soft-pickup]".
  - *Completed 2025-12-01*: Rewrote SoftKnob with 10% per cycle interpolation, cross-detection for immediate unlock, configurable rate via SetInterpolationRate(). Tests updated.

- [x] **Create 16-slot parameter storage** — 4 knobs × 4 mode/shift combinations, each with independent soft pickup state. Reference: Implementation Roadmap Phase 5.
  - *Completed 2025-12-01*: Already implemented in main.cpp. 16-element SoftKnob array, mode-change detection loads targets, all 16 initialized at startup.

### Phase 7: LED Feedback

- [x] **Implement LED state machine** — Performance mode: pulse on anchor. Config mode: solid. Shift: brighter. Knob interaction: show value for 1s. Fill active: rapid flash. Reference: `docs/specs/main.md` section "LED Visual Feedback [duopulse-led]".
  - *Completed 2025-12-01*: Updated main.cpp LED logic with all states: anchor pulse, config solid, shift brightness, 1s knob feedback, high FLUX rapid flash (50ms/20Hz).

### Phase 8: Gate Time Control

- [x] **Implement configurable gate time** — Range 5-50ms controlled by K2+Shift in Config Mode. Apply to next trigger. Reference: `docs/specs/main.md` section "Control System [duopulse-controls]".
  - *Completed 2025-12-01*: Already implemented. SetGateTime() maps 0-1 to 5-50ms, wired up in main.cpp as K2+Shift in Config Mode.

### Phase 9: Clock Division

- [x] **Implement clock output division/multiplication** — ÷4, ÷2, ×1, ×2, ×4 based on K4+Shift in Config Mode. Reference: `docs/specs/main.md` section "Control System [duopulse-controls]".
  - *Completed 2025-12-01*: Added GetClockDivisionFactor() and clock division logic in TriggerClock(). Division (÷2, ÷4) works. Multiplication noted as future enhancement (requires sub-step timing).

### Phase 10: Testing & Tuning

- [ ] **Unit tests for swing calculation** — Test genre ranges, taste interpolation, swing application to off-beats.

- [ ] **Unit tests for Orbit modes** — Test interlock, free, shadow behaviors.

- [ ] **Unit tests for Contour CV modes** — Test velocity, decay, pitch, random outputs.

- [ ] **Unit tests for phrase modulation** — Test fill zones, build zones, phrase progress calculations.

- [ ] **Pattern tuning by ear** — Iterate on 16 skeleton patterns for musical feel. Tune density curves, accent intensities, swing ranges.

- [ ] **Hardware integration testing** — Verify all control mappings, CV input modulation, gate outputs, swung clock.

## Files to Modify

- `src/main.cpp` — Control routing, shift layer, fill trigger, CV routing
- `src/Engine/Sequencer.h` — New interface, phrase position, swing, orbit
- `src/Engine/Sequencer.cpp` — Implementation of all new features
- `src/Engine/PatternGenerator.h` — New hybrid structure
- `src/Engine/PatternGenerator.cpp` — New generation algorithm, 16 patterns
- `src/Engine/SoftKnob.h` — Enhanced gradual interpolation
- `src/Engine/SoftKnob.cpp` — Interpolation implementation
- `src/Engine/ChaosModulator.h/cpp` — Integration with FLUX
- `tests/test_sequencer.cpp` — New test cases

## Files to Create (Optional)

- `src/Engine/GenreConfig.h` — Swing ranges and genre parameters
- `src/Engine/PatternData.h` — Skeleton pattern tables (or inline)

## Estimated Effort

| Phase | Description | Hours |
|-------|-------------|-------|
| 1 | Control System | 4-6 |
| 2 | Sequencer Engine | 10-14 |
| 3 | Pattern Generator | 6-8 |
| 4 | Phrase Structure | 3-4 |
| 5 | CV Expression | 2-3 |
| 6 | Soft Pickup | 1-2 |
| 7 | LED Feedback | 1-2 |
| 8 | Gate Time | 0.5-1 |
| 9 | Clock Division | 0.5-1 |
| 10 | Testing/Tuning | 10-15 |
| **Total** | | **~40-56 hours** |

## Success Criteria

1. Abstract terminology throughout (Anchor/Shimmer, Terrain, Orbit, etc.)
2. CV inputs always affect performance parameters
3. Genre-aware swing with taste fine-tuning
4. CV-driven fills via high FLUX values
5. Swung clock output
6. Gradual soft pickup on all knobs
7. Visual LED feedback for all states
8. Patterns optimized for 2-voice output
9. Musically appropriate for techno/tribal/trip-hop/IDM
10. All 16 controls accessible (4 modes × 4 knobs)

# Comments
- 2025-12-01: Created task from second-iteration specification synthesis. Supersedes 07-opinionated-sequencer.

