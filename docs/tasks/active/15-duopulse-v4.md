# Feature: DuoPulse v4 Implementation [duopulse-v4]

## Context

DuoPulse v4 is a complete architectural overhaul of the algorithmic drum sequencer. Key changes from v3:

- **Control System**: New ergonomic pairings (ENERGY/PUNCH, BUILD/GENRE, FIELD X/DRIFT, FIELD Y/BALANCE)
- **Pattern System**: 2D Pattern Field with 3×3 archetype grid per genre (27 total archetypes)
- **Generation**: Hit budget system with Gumbel Top-K sampling replaces probability-per-step
- **Timing**: FLAVOR parameter (from Audio In R) controls BROKEN timing stack
- **Outputs**: Velocity as sample & hold on audio outputs, new AUX modes

This replaces the v3 implementation entirely. Previous code is preserved in git history.

## Implementation Guidelines

1. **Implementation code goes in `.cpp` files**. Header files (`.h`) should contain only:
   - Class/struct definitions
   - Method declarations
   - Constants and enums
   - Inline trivial getters/setters only

2. **All changes reference the spec** at `docs/specs/main.md`

3. **Incremental commits** after each task completion

---

## Phase 0: Cleanup (Remove v3 Code)

These files are being replaced entirely by v4 architecture:

- [x] **0.1** Remove `src/Engine/ChaosModulator.cpp` and `src/Engine/ChaosModulator.h` (replaced by generation pipeline)
- [x] **0.2** Remove `src/Engine/PatternData.h` (replaced by ArchetypeDNA)
- [x] **0.3** Remove `src/Engine/PatternSkeleton.h` (replaced by archetype grids)
- [x] **0.4** Remove `src/Engine/GenreConfig.h` (replaced by genre-specific fields)
- [x] **0.5** Remove `tests/test_pulse_field.cpp` (v3 pulse field tests, will be replaced)
- [x] **0.6** Remove `tests/test_broken_effects.cpp` (v3 broken effects tests, will be replaced)
- [x] **0.7** Remove `tests/test_sequencer.cpp` (v3 sequencer tests, will be replaced)
- [x] **0.8** Update `Makefile` to remove references to deleted source files (verified: wildcards used, no changes needed)
- [x] **0.9** Verify project still compiles (empty main, no link errors)

**Phase 0 Notes:**
- Created `src/Engine/PhrasePosition.h` to extract `PhrasePosition` struct from deleted `GenreConfig.h`
- Disabled `USE_PULSE_FIELD_V3` in `config.h` for clean v4 migration
- Stubbed `Sequencer.h/cpp` to remove v3 dependencies while maintaining interface
- Updated `BrokenEffects.h` to use `PhrasePosition.h` instead of deleted `GenreConfig.h`
- Updated `tests/test_example.cpp` to remove ChaosModulator tests
- All tests pass (177 assertions in 20 test cases)

---

## Phase 1: Foundation (Data Structures & Enums)

Create core types in headers, no implementation yet. Reference: spec sections 4.7, 10.

- [ ] **1.1** Create `src/Engine/DuoPulseTypes.h` with all enums:
  - `Genre` (TECHNO, TRIBAL, IDM)
  - `Voice` (ANCHOR, SHIMMER, AUX)
  - `EnergyZone` (MINIMAL, GROOVE, BUILD, PEAK)
  - `AuxMode` (HAT, FILL_GATE, PHRASE_CV, EVENT)
  - `AuxDensity` (SPARSE, NORMAL, DENSE, BUSY)
  - `VoiceCoupling` (INDEPENDENT, INTERLOCK, SHADOW)
  - `ResetMode` (PHRASE, BAR, STEP)

- [ ] **1.2** Create `src/Engine/ArchetypeDNA.h` with `ArchetypeDNA` struct (spec section 5.2)

- [ ] **1.3** Create `src/Engine/ControlState.h` with:
  - `PunchParams` struct
  - `BuildModifiers` struct
  - `FillInputState` struct
  - `ControlState` struct (all control parameters)

- [ ] **1.4** Create `src/Engine/SequencerState.h` with:
  - `DriftState` struct
  - `GuardRailState` struct
  - `SequencerState` struct (position, masks, event flags)

- [ ] **1.5** Create `src/Engine/OutputState.h` with:
  - `TriggerState` struct
  - `VelocityOutputState` struct
  - `LEDState` struct (and `LEDMode` enum)
  - `OutputState` struct

- [ ] **1.6** Create `src/Engine/DuoPulseState.h` combining all state into `DuoPulseState`

- [ ] **1.7** Add tests: `tests/test_duopulse_types.cpp` - verify enum values, struct sizes

---

## Phase 2: Pattern Field System

Implement archetype storage and blending. Reference: spec section 5.

- [ ] **2.1** Create `src/Engine/PatternField.h` with:
  - `GenreField` struct (3×3 archetype array)
  - `BlendArchetypes()` declaration
  - `SoftmaxWithTemperature()` declaration

- [ ] **2.2** Implement `src/Engine/PatternField.cpp`:
  - `BlendArchetypes()` - winner-take-more blending (spec 5.3)
  - `SoftmaxWithTemperature()` - softmax with temperature parameter
  - `GetGenreField()` - return genre's archetype grid

- [ ] **2.3** Create `src/Engine/ArchetypeData.h` with archetype weight tables:
  - Techno grid (9 archetypes)
  - Tribal grid (9 archetypes)
  - IDM grid (9 archetypes)
  - Use `constexpr` arrays for flash storage

- [ ] **2.4** Add tests: `tests/test_pattern_field.cpp`
  - Test blending at grid corners returns exact archetype
  - Test blending at center produces weighted mix
  - Test softmax with temperature sharpens weights
  - Test all 27 archetypes load correctly

---

## Phase 3: Generation Pipeline

Implement hit budget, eligibility, and Gumbel sampling. Reference: spec section 6.

- [ ] **3.1** Create `src/Engine/HitBudget.h` with:
  - `BarBudget` struct
  - `ComputeBarBudget()` declaration
  - Mask constants (kDownbeatMask, kQuarterNoteMask, etc.)

- [ ] **3.2** Implement `src/Engine/HitBudget.cpp`:
  - `ComputeBarBudget()` - budget from energy/balance/zone (spec 6.1)
  - `ComputeEligibilityMask()` - mask from energy/flavor (spec 6.2)

- [ ] **3.3** Create `src/Engine/GumbelSampler.h` with:
  - `SelectHitsGumbelTopK()` declaration
  - `HashToFloat()` declaration

- [ ] **3.4** Implement `src/Engine/GumbelSampler.cpp`:
  - `SelectHitsGumbelTopK()` - weighted selection with spacing (spec 6.3)
  - `HashToFloat()` - deterministic hash to float

- [ ] **3.5** Create `src/Engine/VoiceRelation.h` with:
  - `ApplyVoiceRelationship()` declaration
  - `GetCoupleValueFromConfig()` declaration

- [ ] **3.6** Implement `src/Engine/VoiceRelation.cpp`:
  - `ApplyVoiceRelationship()` - interlock/shadow logic (spec 6.4)

- [ ] **3.7** Create `src/Engine/GuardRails.h` with:
  - `SoftRepairPass()` declaration
  - `ApplyHardGuardRails()` declaration

- [ ] **3.8** Implement `src/Engine/GuardRails.cpp`:
  - `SoftRepairPass()` - bias rescue (spec 6.5)
  - `ApplyHardGuardRails()` - force corrections (spec 6.6)

- [ ] **3.9** Add tests: `tests/test_generation.cpp`
  - Test hit budget scales with energy
  - Test Gumbel selects correct count
  - Test spacing rules prevent clumping
  - Test guard rails force downbeat
  - Test max gap enforcement

---

## Phase 4: Timing System (BROKEN Stack)

Implement timing effects. Reference: spec section 7.

- [ ] **4.1** Update `src/Engine/BrokenEffects.h` with v4 declarations:
  - `ComputeSwing()` - swing from flavor + zone
  - `ApplySwingToStep()` - swing offset calculation
  - `ComputeMicrotimingOffset()` - jitter from flavor + zone
  - `ComputeStepDisplacement()` - displacement from flavor + zone

- [ ] **4.2** Implement `src/Engine/BrokenEffects.cpp` (v4 version):
  - All timing functions from spec 7.2-7.4
  - Zone-bounded limits

- [ ] **4.3** Create `src/Engine/VelocityCompute.h` with:
  - `ComputePunch()` - PunchParams from punch value
  - `ComputeBuildModifiers()` - BuildModifiers from build + progress
  - `ComputeVelocity()` - velocity from punch/build/accent

- [ ] **4.4** Implement `src/Engine/VelocityCompute.cpp`:
  - All velocity functions from spec 7.5

- [ ] **4.5** Add tests: `tests/test_timing.cpp`
  - Test swing bounded by zone
  - Test jitter = 0 at low flavor
  - Test displacement only at high flavor + BUILD/PEAK zone
  - Test velocity contrast scales with punch

---

## Phase 5: DRIFT Seed System

Implement pattern evolution control. Reference: spec section 6.7.

- [ ] **5.1** Create `src/Engine/DriftControl.h` with:
  - `SelectSeed()` declaration
  - `GetStepStability()` declaration
  - `OnPhraseEnd()` declaration
  - `Reseed()` declaration

- [ ] **5.2** Implement `src/Engine/DriftControl.cpp`:
  - All drift functions from spec 6.7

- [ ] **5.3** Add tests: `tests/test_drift.cpp`
  - Test DRIFT=0 uses locked seed for all steps
  - Test DRIFT=1 uses evolving seed for all steps
  - Test downbeats use locked seed longer than ghosts
  - Test phrase seed changes on phrase boundary
  - Test reseed generates new pattern seed

---

## Phase 6: Output System

Implement trigger, velocity, and AUX outputs. Reference: spec section 8.

- [ ] **6.1** Update `src/Engine/GateScaler.h` for v4 trigger output

- [ ] **6.2** Update `src/Engine/GateScaler.cpp` with `ProcessTriggerOutput()` (spec 8.2)

- [ ] **6.3** Create `src/Engine/VelocityOutput.h` with:
  - `ProcessVelocityOutput()` declaration (sample & hold)

- [ ] **6.4** Implement `src/Engine/VelocityOutput.cpp`:
  - Sample & hold velocity output (spec 8.3)

- [ ] **6.5** Create `src/Engine/AuxOutput.h` with:
  - `ComputeAuxOutput()` declaration
  - `ComputeAuxModeOutput()` declaration

- [ ] **6.6** Implement `src/Engine/AuxOutput.cpp`:
  - All AUX modes from spec 8.4 (HAT, FILL_GATE, PHRASE_CV, EVENT)

- [ ] **6.7** Add tests: `tests/test_outputs.cpp`
  - Test trigger pulse width
  - Test velocity sample & hold behavior
  - Test AUX modes produce correct signals

---

## Phase 7: LED Feedback System

Implement LED state machine. Reference: spec section 9.

- [ ] **7.1** Update `src/Engine/LedIndicator.h` for v4 LED modes

- [ ] **7.2** Update `src/Engine/LedIndicator.cpp`:
  - LED state machine from spec 9.2
  - Parameter change detection from spec 9.3

- [ ] **7.3** Add tests: `tests/test_led.cpp`
  - Test trigger brightness levels
  - Test mode change flash
  - Test fill pulse pattern

---

## Phase 8: Control Processing

Implement control reading and CV modulation. Reference: spec sections 3, 4.

- [ ] **8.1** Update `src/Engine/ControlUtils.h` for v4:
  - `ProcessCVModulation()` declaration
  - `ProcessFillInput()` declaration
  - `ProcessFlavorCV()` declaration

- [ ] **8.2** Create `src/Engine/ControlProcessor.h` with:
  - `ProcessControls()` declaration
  - `ProcessButtonGestures()` declaration

- [ ] **8.3** Implement `src/Engine/ControlProcessor.cpp`:
  - Full control processing from spec 11.4
  - Performance mode handling
  - Config mode handling
  - CV modulation

- [ ] **8.4** Reuse `src/Engine/SoftKnob.cpp/h` (should work as-is)

- [ ] **8.5** Add tests: `tests/test_controls.cpp`
  - Test CV modulation clamping
  - Test fill input gate detection
  - Test mode switching
  - Test button gestures

---

## Phase 9: Core Sequencer

Implement main sequencer logic. Reference: spec section 11.

- [ ] **9.1** Update `src/Engine/Sequencer.h` for v4:
  - New method declarations for v4 pipeline
  - Remove v3-specific methods

- [ ] **9.2** Implement `src/Engine/Sequencer.cpp`:
  - `GenerateBar()` - full bar generation (spec 11.2)
  - `ProcessStep()` - step processing (spec 11.3)
  - `AdvanceStep()` - step advancement
  - Clock processing

- [ ] **9.3** Add tests: `tests/test_sequencer.cpp`
  - Test bar generation produces valid masks
  - Test step advancement wraps correctly
  - Test phrase boundary detection

---

## Phase 10: Persistence

Implement auto-save and config loading. Reference: spec section 12.

- [ ] **10.1** Create `src/Engine/Persistence.h` with:
  - `PersistentConfig` struct
  - `AutoSaveState` struct
  - `MarkConfigDirty()` declaration
  - `ProcessAutoSave()` declaration
  - `LoadConfig()` declaration

- [ ] **10.2** Implement `src/Engine/Persistence.cpp`:
  - Auto-save with debouncing (spec 12.1)
  - Flash read/write
  - Config validation

- [ ] **10.3** Add tests: `tests/test_persistence.cpp`
  - Test config serialization round-trip
  - Test checksum validation
  - Test debounce timing

---

## Phase 11: Main Integration

Wire everything together in main.cpp.

- [ ] **11.1** Update `src/main.cpp`:
  - Initialize DuoPulseState
  - Load config from flash
  - Set up audio callback
  - Wire all outputs

- [ ] **11.2** Verify full build compiles without errors

- [ ] **11.3** Hardware smoke test:
  - Triggers fire on gate outputs
  - Velocity outputs sample & hold
  - LED responds to triggers
  - Knobs affect parameters

---

## Phase 12: Archetype Tuning

Create and tune the 27 archetype weight tables.

- [ ] **12.1** Design Techno archetypes (9 patterns)
- [ ] **12.2** Design Tribal archetypes (9 patterns)
- [ ] **12.3** Design IDM archetypes (9 patterns)
- [ ] **12.4** Musical validation: sweep FIELD X/Y, verify distinct character

---

## Notes

### Files to Keep (Reusable from v3)
- `SoftKnob.cpp/h` - Soft takeover logic is reusable
- `GateScaler.cpp/h` - Trigger output logic is reusable (minor updates)
- `LedIndicator.cpp/h` - LED logic needs updates but structure is reusable
- `ControlUtils.h` - Some utilities may be reusable

### Files Being Replaced Entirely
- `PulseField.cpp/h` → `PatternField.cpp/h` (different algorithm)
- `BrokenEffects.cpp/h` → New version (timing-only, not pattern generation)
- `Sequencer.cpp/h` → Major rewrite for new pipeline
- `ChaosModulator.cpp/h` → Removed (replaced by generation pipeline)
- `PatternData.h`, `PatternSkeleton.h`, `GenreConfig.h` → Removed

### Key Architectural Differences from v3
| v3 | v4 |
|----|-----|
| BROKEN controls pattern + timing | FLAVOR controls timing only; FIELD X/Y control pattern |
| Probability per step | Hit budget with Gumbel sampling |
| 32-step weight table | 3×3 archetype grid per genre |
| COUPLE as continuous interlock | VoiceCoupling as 3 discrete modes |
| Single DENSITY per voice | ENERGY + BALANCE split |
| DRIFT for pattern evolution | Same concept, better stratified stability |
