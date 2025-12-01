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

- [x] **Unit tests for swing calculation** — Test genre ranges, taste interpolation, swing application to off-beats.
  - *Completed 2025-12-01*: Already implemented. Tests cover genre detection, swing ranges, CalculateSwing, IsOffBeat, CalculateSwingDelaySamples.

- [x] **Unit tests for Orbit modes** — Test interlock, free, shadow behaviors.
  - *Completed 2025-12-01*: Already implemented. Tests cover GetOrbitMode, GetInterlockModifier.

- [x] **Unit tests for Contour CV modes** — Test velocity, decay, pitch, random outputs.
  - *Completed 2025-12-01*: Added tests for GetContourMode and CalculateContourCV in all 4 modes.

- [x] **Unit tests for phrase modulation** — Test fill zones, build zones, phrase progress calculations.
  - *Completed 2025-12-01*: Added tests for CalculatePhrasePosition, fill/build zone scaling, GetPhraseFillBoost, GetPhraseAccentMultiplier.

- [x] **Clean up legacy code** — Remove old Grids-based PatternGenerator, GridsData, legacy interface (SetLowDensity, SetHighDensity, etc.), and update tests to use new DuoPulse v2 interface.
  - *Completed 2025-12-01*: Deleted PatternGenerator.h/cpp, GridsData.h/cpp. Removed legacy interface (SetLowDensity, SetHighDensity, SetLowVariation, SetHighVariation, SetStyle, SetEmphasis). Removed legacy aliases and useSkeletonPatterns flag. Updated tests to use SetAnchorDensity, SetShimmerDensity, SetFlux, SetTerrain.

- [ ] **Pattern tuning by ear** — Iterate on 16 skeleton patterns for musical feel. Tune density curves, accent intensities, swing ranges.

- [ ] **Hardware integration testing** — Verify all control mappings, CV input modulation, gate outputs, swung clock.

---

## Manual Testing Notes

### Equipment Needed
- Patch.Init module in Eurorack case
- Oscilloscope or CV meter (for verifying CV outputs)
- Audio interface or mixer (for monitoring gate outputs)
- External clock source (for testing external clock mode)
- CV sources (LFO, envelope, manual CV) for testing CV inputs
- Downstream drum modules or sampler to hear actual percussion sounds

### Test 1: Basic Boot & Operation
1. **Power on** — Module should boot with LED off initially
2. **LED default state** — In Performance Mode (switch DOWN), LED should pulse on anchor triggers
3. **Internal clock** — Should hear triggers at default 120 BPM
4. **Both outputs active** — Gate Out 1 (Anchor) and Gate Out 2 (Shimmer) should both fire

### Test 2: Control Modes (4 modes × 4 knobs = 16 parameters)

#### Performance Mode Primary (Switch DOWN, no shift)
| Knob | Parameter | Test |
|------|-----------|------|
| K1 | Anchor Density | CCW = sparse kicks, CW = busy kicks |
| K2 | Shimmer Density | CCW = sparse snares, CW = busy snares |
| K3 | FLUX | CCW = clean pattern, CW = ghost notes + fills |
| K4 | FUSE | CCW = anchor-heavy, Center = balanced, CW = shimmer-heavy |

#### Performance Mode Shift (Switch DOWN + B7 held >150ms)
| Knob | Parameter | Test |
|------|-----------|------|
| K1+Shift | Anchor Accent | CCW = even dynamics, CW = punchy accents |
| K2+Shift | Shimmer Accent | CCW = even dynamics, CW = punchy accents |
| K3+Shift | ORBIT | 0-33% = Interlock (call-response), 33-67% = Free, 67-100% = Shadow (echo) |
| K4+Shift | CONTOUR | 0-25% = Velocity CV, 25-50% = Decay CV, 50-75% = Pitch CV, 75-100% = Random S&H |

#### Config Mode Primary (Switch UP, no shift)
| Knob | Parameter | Test |
|------|-----------|------|
| K1 | TERRAIN | 0-25% = Techno (straight), 25-50% = Tribal (shuffle), 50-75% = Trip-Hop (lazy), 75-100% = IDM (broken) |
| K2 | LENGTH | 1, 2, 4, 8, 16 bars (verify with scope) |
| K3 | GRID | Patterns 0-15 (audibly different characters) |
| K4 | TEMPO | 90-160 BPM range |

#### Config Mode Shift (Switch UP + B7 held >150ms)
| Knob | Parameter | Test |
|------|-----------|------|
| K1+Shift | SWING TASTE | Fine-tune swing within genre range |
| K2+Shift | GATE TIME | 5ms (short) to 50ms (long) gate duration |
| K3+Shift | HUMANIZE | 0% = machine-tight, 100% = ±10ms jitter |
| K4+Shift | CLOCK DIV | ÷4, ÷2, ×1, ×2, ×4 on clock output |

### Test 3: Shift Layer & Tap Tempo
1. **Tap tempo** — Short tap (<150ms) on B7 in Performance Mode triggers tap tempo
2. **Shift detection** — Hold B7 >150ms, shift LED should brighten, knobs control shift parameters
3. **Shift release** — Release B7, returns to primary layer
4. **Mode memory** — Each mode/shift combo remembers its own knob positions (soft takeover)

### Test 4: CV Input Modulation (Always Performance)
| CV Input | Modulates | Test |
|----------|-----------|------|
| CV 5 | Anchor Density | Patch LFO, verify density modulation regardless of mode |
| CV 6 | Shimmer Density | Same test |
| CV 7 | FLUX | High CV = fills/chaos, verify in Config Mode too |
| CV 8 | FUSE | Modulate balance between voices |

**Critical**: CV modulation should work in ALL modes (Performance + Config).

### Test 5: External Clock
1. Patch external clock to Gate In 1
2. Module should sync to external clock
3. Remove external clock for 2 seconds — should fall back to internal
4. Re-patch external clock — should re-sync

### Test 6: Reset Input
1. Patch gate/trigger to Gate In 2
2. Should reset to step 0 on rising edge
3. Test with slow and fast triggers

### Test 7: Swing Verification (per genre)
| Terrain | Genre | Expected Swing Range |
|---------|-------|---------------------|
| 0-25% | Techno | 52-57% (nearly straight) |
| 25-50% | Tribal | 56-62% (mild shuffle) |
| 50-75% | Trip-Hop | 60-68% (lazy, behind-beat) |
| 75-100% | IDM | 54-65% + timing jitter |

**Test**: Use oscilloscope to measure off-beat delay vs. on-beat.

### Test 8: Orbit Voice Relationship
1. **Interlock (0-33%)** — Shimmer should fill gaps when anchor silent, reduce when anchor fires
2. **Free (33-67%)** — Both voices independent, can overlap
3. **Shadow (67-100%)** — Shimmer echoes anchor with 1-step delay at 70% velocity

### Test 9: Contour CV Output Modes
| Mode | Audio Out 1/2 Behavior | Test |
|------|------------------------|------|
| Velocity | CV = hit intensity, holds between triggers | Patch to VCA, verify dynamic response |
| Decay | High velocity = high CV (decay hint) | Patch to envelope decay input |
| Pitch | Random offset ±0.2V scaled by velocity | Patch to VCO, hear pitch variation |
| Random | S&H random 0-5V each trigger | Patch to filter cutoff |

### Test 10: Clock Division Output (CV Out 1)
| Clock Div Setting | Expected Output |
|-------------------|-----------------|
| ÷4 (K4+Shift CCW) | One pulse per bar |
| ÷2 | One pulse per half-bar |
| ×1 (center) | One pulse per 16th note step |
| ×2 | Two pulses per step (if implemented) |
| ×4 | Four pulses per step (if implemented) |

**Note**: Multiplication (×2, ×4) marked as future enhancement - may not be implemented.

### Test 11: LED Feedback
| State | Expected LED Behavior |
|-------|----------------------|
| Performance Mode | Pulses on anchor trigger |
| Config Mode | Solid ON |
| Shift Held | Brighter |
| Knob Turn | Shows parameter value for 1 second |
| High FLUX (>70%) | Rapid flash at 20Hz |

### Test 12: Soft Takeover
1. Set K1 to 50% in Performance Mode
2. Switch to Config Mode (K1 now controls Terrain)
3. Move K1 — value should interpolate gradually, not jump
4. Cross the stored value — should "catch up" and track directly
5. Switch back to Performance — K1 should interpolate back to 50%

### Test 13: Pattern Character by Genre (GRID knob)
| Index | Pattern | Character |
|-------|---------|-----------|
| 0-3 | Techno | Four-on-floor, minimal, driving, pounding |
| 4-7 | Tribal | Clave, interlocking, polyrhythmic, circular |
| 8-11 | Trip-Hop | Sparse, lazy, heavy, groove |
| 12-15 | IDM | Broken, glitch, irregular, chaos |

**Listen for**: Distinct character per pattern, appropriate for genre.

### Test 14: Phrase Awareness (Long Pattern)
1. Set LENGTH to 4 bars
2. Listen through complete phrase
3. **Bar 1-2**: Steady pattern
4. **Bar 3**: Building (more ghost notes, syncopation)
5. **Bar 4 (last 8 steps)**: Fill zone (increased activity)
6. **Loop point**: Clear phrase reset

### Test 15: Full Integration Patch
1. Patch Gate Out 1 → Kick drum
2. Patch Gate Out 2 → Snare drum
3. Patch Audio Out 1 → Kick VCA CV
4. Patch Audio Out 2 → Snare VCA CV
5. Patch CV Out 1 → Clock for another module
6. Set Terrain to Trip-Hop (50-75%)
7. Set FLUX to moderate (40-60%)
8. Set ORBIT to Interlock (0-33%)
9. **Verify**: Musical groove with call-response interaction

### Known Limitations / Future Enhancements
- Clock multiplication (×2, ×4) requires sub-step timing — not implemented
- Pattern tuning needs ear-based iteration on the 16 skeletons
- Contour CV modes apply to both voices equally (no per-voice contour)

### Pass/Fail Checklist
- [ ] All 16 knob mappings work in correct modes
- [ ] CV inputs modulate performance params in ALL modes
- [ ] Shift layer activates/deactivates correctly
- [ ] Tap tempo works (short tap)
- [ ] External clock syncs properly
- [ ] Reset input returns to step 0
- [ ] Swing varies by genre
- [ ] Orbit modes create audible voice interaction
- [ ] Contour modes produce expected CV shapes
- [ ] Clock division works (÷4, ÷2, ×1)
- [ ] LED feedback reflects all states
- [ ] Soft takeover prevents jumps
- [ ] Patterns have distinct genre character
- [ ] Phrase awareness creates musical arc
- [ ] No crashes, lockups, or unexpected behavior

## Files Modified

- `src/main.cpp` — Control routing, shift layer, fill trigger, CV routing
- `src/Engine/Sequencer.h` — DuoPulse v2 interface, phrase position, swing, orbit
- `src/Engine/Sequencer.cpp` — All new features implementation
- `src/Engine/SoftKnob.h/cpp` — Enhanced gradual interpolation
- `src/Engine/ChaosModulator.h/cpp` — Integration with FLUX
- `tests/test_sequencer.cpp` — New test cases

## Files Created

- `src/Engine/GenreConfig.h` — Swing ranges, phrase modulation, contour modes
- `src/Engine/PatternSkeleton.h` — 32-step pattern structure with helpers
- `src/Engine/PatternData.h` — 16 skeleton patterns for all genres

## Files Deleted (Legacy Cleanup)

- `src/Engine/PatternGenerator.h/cpp` — Replaced by PatternSkeleton
- `src/Engine/GridsData.h/cpp` — No longer needed

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

