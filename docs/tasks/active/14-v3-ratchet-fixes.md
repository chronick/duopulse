---
id: chronick/daisysp-idm-grids-14
title: "v3 Fixes: RATCHET + Critical Bug Fixes + Code Quality"
status: "pending"
created_date: "2025-12-17"
last_updated: "2025-12-17"
owner: "user/ai"
spec_refs:
  - "pulse-field"
  - "v3-critical-rules"
  - "ratchet-control"
  - "drift-control"
---

# Feature: v3 Ratchet & Fixes [v3-ratchet-fixes]

## Context

DuoPulse v3 is mostly complete, but hardware testing revealed two critical bugs and the need for a new RATCHET parameter to control fill intensity. This task also includes code quality improvements to keep header files minimal.

**Critical Issues:**
1. **DENSITY=0 still triggers events** — Must be absolute silence regardless of BROKEN
2. **DRIFT=0 still has beat variation** — Must be identical every loop

**New Feature:**
3. **RATCHET parameter (K4+Shift)** — Controls fill intensity during phrase transitions

**The Reference Point:**
> BROKEN=0, DRIFT=0, DENSITIES at 50%: classic 4/4 kick with snare on backbeat, repeated identically forever.

## Phase 1: Critical Bug Fixes

### DENSITY=0 Absolute Silence [v3-critical-rules]

**Root Cause Analysis**: The algorithm in `ShouldStepFire()` is mathematically correct. At density=0, threshold=1.0, and since effectiveWeight is clamped to max 1.0, nothing should fire. The bugs are upstream:

1. **Density Clamp Floor** (Sequencer.cpp:370): `Clamp(..., 0.05f, 0.95f)` floors density at 0.05
2. **FUSE Boost**: Can push 0 density up to +0.15 when FUSE is CW
3. **COUPLE Gap-Fill**: Injects triggers after algorithm, regardless of density

**Design Principle**: If user sets density=0, that voice is SILENT. No FUSE boost, no COUPLE gap-fill.

**Fixes (elegant, algorithm-preserving):**
- [x] **Fix 1**: Change clamp from `0.05f` to `0.0f` in Sequencer.cpp (lines 370, 390) *(2025-12-17)*
- [x] **Fix 2**: In `ApplyFuse()`, don't boost a voice above 0 if its base density was 0 *(2025-12-17)*
- [x] **Fix 3**: In `ApplyCouple()`, don't gap-fill for a voice if its density is 0 *(2025-12-17)*
- [x] Add unit test: density=0 produces zero triggers regardless of BROKEN/DRIFT/FUSE/COUPLE *(2025-12-17)*
- [x] *(spec: [pulse-field], [v3-critical-rules])*

### DRIFT=0 Zero Variation [v3-critical-rules]

**Root Cause Analysis**: The `ShouldStepFireWithDrift()` algorithm is correct — at DRIFT=0, all steps use `patternSeed_`. The bug is in the v2 legacy code still running in v3:

1. **ChaosModulator density bias** (Sequencer.cpp:339-370): `chaosSampleLow.densityBias` is added to density before the algorithm. ChaosModulator uses `std::minstd_rand` with internal state that's NOT deterministic per loop.
2. **flux_ = broken_** (line 123): SetBroken syncs flux_, so chaos is active whenever BROKEN>0

**Design Principle**: In v3, BROKEN controls variation (deterministic via seeds), DRIFT controls whether it changes per-loop. ChaosModulator's density bias is redundant and breaks DRIFT=0.

**Fixes (clean v3 path):**
- [x] **Fix 1**: In v3 path, skip `chaosSampleLow.densityBias` addition — just use raw densities *(2025-12-17: implemented conditional approach)*
- [x] **Fix 2**: Only add densityBias when DRIFT>0 (conditional chaos) *(2025-12-17: implemented)*
- [x] Verify at DRIFT=0, ALL steps use `patternSeed_` (algorithm already correct) *(2025-12-17: verified via tests)*
- [x] Add unit test: DRIFT=0 produces identical pattern across multiple loops *(2025-12-17)*
- [x] Add unit test: DRIFT=0 + BROKEN=100% still produces same chaos pattern every loop *(2025-12-17)*
- [x] *(spec: [drift-control], [v3-critical-rules])*

## Phase 2: RATCHET Parameter [ratchet-control]

### Parameter Setup
- [x] Add `ratchet_` member to Sequencer (0.0-1.0) *(2025-12-17)*
- [x] Add `SetRatchet()` / `GetRatchet()` methods *(2025-12-17)*
- [x] Map K4+Shift to RATCHET in Performance Mode *(2025-12-17)*
- [x] Add soft takeover for RATCHET parameter *(2025-12-17: via existing SoftKnob system)*
- [x] *(spec: [ratchet-control], [duopulse-controls])*

### Fill Zone Updates [phrase-modulation]
- [x] Add `isMidPhrase` (40-60%) to `PhrasePosition` struct *(2025-12-17)*
- [x] Update `GetPhraseWeightBoost()` to accept DRIFT and RATCHET parameters *(2025-12-17: GetPhraseWeightBoostWithRatchet)*
- [x] Fill density boost scales with RATCHET (0-30%) *(2025-12-17)*
- [x] DRIFT gates fill probability — at DRIFT=0, no fills occur *(2025-12-17)*
- [x] *(spec: [ratchet-control], [phrase-modulation])*

### Ratcheting (32nd Subdivisions)
- [x] Implement ratcheting logic when RATCHET > 50% *(2025-12-17: ProcessRatchet() in Sequencer)*
- [x] Ratchets occur in fill zones only *(2025-12-17: conditional on isFillZone || isMidPhrase)*
- [x] Ratchet density increases toward phrase end *(2025-12-17: fillProgress scales probability)*
- [x] *(spec: [ratchet-control])*

### Fill Resolution
- [x] Velocity ramp: fills get louder toward phrase end (1.0-1.3×) *(2025-12-17)*
- [x] Resolution accent on phrase downbeat (1.0-1.5× based on RATCHET) *(2025-12-17)*
- [x] Update `GetPhraseAccent()` to accept RATCHET parameter *(2025-12-17: GetPhraseAccentWithRatchet)*
- [x] *(spec: [ratchet-control])*

### Tests
- [x] Unit test: DRIFT=0 + any RATCHET = no fills *(2025-12-17)*
- [x] Unit test: DRIFT=100% + RATCHET=0 = subtle fills *(2025-12-17)*
- [x] Unit test: DRIFT=100% + RATCHET=100% = intense fills with ratcheting *(2025-12-17)*
- [x] Unit test: Fill zones at correct phrase positions *(2025-12-17: isMidPhrase test)*

## Phase 3: Code Quality — Header/CPP Split

### BrokenEffects Refactor
- [x] Create `src/Engine/BrokenEffects.cpp` *(2025-12-17)*
- [x] Move function implementations from `BrokenEffects.h` to `BrokenEffects.cpp` *(2025-12-17)*
- [x] Keep only function declarations and inline helpers in header *(2025-12-17)*
- [x] Verify tests still pass after refactor *(2025-12-17: all 149 tests pass)*

### PulseField Refactor
- [x] Create `src/Engine/PulseField.cpp` *(2025-12-17)*
- [x] Move `ShouldStepFire()` and `ShouldStepFireWithDrift()` implementations to cpp *(2025-12-17)*
- [x] Keep weight tables and inline helpers in header *(2025-12-17)*
- [x] Verify tests still pass after refactor *(2025-12-17: all 149 tests pass)*

### LedIndicator Refactor (Optional)
- [x] Create `src/Engine/LedIndicator.cpp` *(2025-12-17)*
- [x] Move non-trivial function implementations to cpp *(2025-12-17)*
- [x] Keep inline helpers in header *(2025-12-17)*

### Build Verification
- [x] Run `make clean && make` — verify no linker errors *(2025-12-17: firmware builds at 76.95% flash)*
- [x] Run `make test` — verify all tests pass *(2025-12-17: 149 tests, 81957 assertions)*
- [x] Verify firmware size is reasonable *(2025-12-17: 76.95% flash usage)*

## Phase 4: Documentation Updates

- [ ] Update README with RATCHET description
- [ ] Update control layout in README
- [ ] Update `docs/specs/main.md` acceptance criteria as items complete
- [ ] Mark task complete in `docs/tasks/index.md`

## Notes

### Session 2025-12-17: Phase 1 Complete

**Changes Made:**
1. `Sequencer.cpp` — Changed density clamp floor from `0.05f` to `0.0f` (was preventing true silence)
2. `Sequencer.cpp` — Added conditional: skip ChaosModulator `densityBias` when `drift_ == 0` (was breaking DRIFT=0 determinism)
3. `BrokenEffects.h` `ApplyFuse()` — Added zero-preservation logic: if voice density was 0, FUSE cannot boost it above 0
4. `BrokenEffects.h` `ApplyCouple()` — Added optional `shimmerDensity` param; gap-fill disabled when density ≤ 0
5. `test_broken_effects.cpp` — Added 9 v3-critical-rules tests for DENSITY=0 invariant
6. `test_pulse_field.cpp` — Added 7 v3-critical-rules tests for DENSITY=0 and DRIFT=0 invariants, including "The Reference Point" test

**Key Design Decisions:**
- Made `ApplyCouple(shimmerDensity)` optional with default `-1.0f` for backward compatibility
- Used `densityBias` conditional on `drift_ > 0` rather than removing it entirely (cleaner, maintains v2 fallback)

**Test Results:** All 139 tests pass (81,936 assertions). Firmware builds at 76% flash.

### Session 2025-12-17: Phase 2 Complete

**Changes Made:**
1. `Sequencer.h` — Added `ratchet_` member, `SetRatchet()`, `GetRatchet()` methods
2. `Sequencer.cpp` — Added `ProcessRatchet()` for 32nd note subdivision triggers
3. `Sequencer.cpp` — Ratchet scheduling in ProcessAudio when RATCHET > 50% in fill zones
4. `Sequencer.cpp` — Updated `GetPulseFieldTriggers()` to use RATCHET-aware functions
5. `GenreConfig.h` — Added `isMidPhrase` (40-60%) to `PhrasePosition` struct
6. `BrokenEffects.h` — Added `GetPhraseWeightBoostWithRatchet()` (DRIFT × RATCHET interaction)
7. `BrokenEffects.h` — Added `GetPhraseAccentWithRatchet()` (resolution accent boost, fill zone velocity ramp)
8. `main.cpp` — Mapped K4+Shift to RATCHET (replaced 'reserve'), updated soft knob init
9. `test_broken_effects.cpp` — Added 10 RATCHET tests for fill probability, accent, and isMidPhrase

**Key Design Decisions:**
- DRIFT gates fill probability (DRIFT=0 → no fills, regardless of RATCHET)
- RATCHET scales fill intensity (density boost 0-30%, velocity ramp 1.0-1.3×)
- Ratcheting (32nd subs) fires at RATCHET > 50%, in fill/mid-phrase zones only
- Resolution accent on phrase downbeat scales 1.2-1.5× with RATCHET
- Ratchet triggers fire at 70% velocity of primary trigger

**Test Results:** All 149 tests pass (81,957 assertions). Firmware builds at 76.80% flash.

### Session 2025-12-17: Phase 3 Complete

**Changes Made:**
1. `BrokenEffects.cpp` — Created new file, moved all function implementations from header (13 functions)
2. `BrokenEffects.h` — Converted inline functions to declarations, kept only magic constants and documentation
3. `PulseField.cpp` — Created new file, moved `ShouldStepFire()` and `ShouldStepFireWithDrift()` implementations
4. `PulseField.h` — Converted core algorithm functions to declarations, kept weight tables and inline helpers
5. `LedIndicator.cpp` — Created new file, moved all Process* methods and Init() implementation
6. `LedIndicator.h` — Converted methods to declarations, kept only simple inline helpers

**Key Design Decisions:**
- Kept simple inline helpers (Lerp, Clamp, HashStep, HashToFloat) in PulseField.h for performance
- Kept weight tables and constants in headers for easy access
- All function implementations moved to .cpp files to reduce compile-time coupling
- Maintained backward compatibility — no API changes

**Test Results:** All 149 tests pass (81,957 assertions). Firmware builds successfully at 76.95% flash usage.

---

- RATCHET works with DRIFT, not independently:
  - DRIFT = "Do fills happen?" (probability gate)
  - RATCHET = "How intense are fills?" (intensity control)
- Header/CPP split improves compile times and reduces coupling
- All changes should maintain v2 fallback compatibility (`#ifdef USE_PULSE_FIELD_V3`)

---

### Acceptance Criteria Summary

| Requirement | Status |
|-------------|--------|
| DENSITY=0 = absolute silence | ☑ *(2025-12-17)* |
| DRIFT=0 = zero variation | ☑ *(2025-12-17)* |
| RATCHET controls fill intensity | ☑ *(2025-12-17)* |
| Fills target phrase boundaries | ☑ *(2025-12-17)* |
| Code moved to cpp files | ☐ |
| All tests pass | ☑ *(149 tests, 81957 assertions)* |
