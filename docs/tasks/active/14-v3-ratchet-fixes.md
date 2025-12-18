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
- [ ] **Fix 1**: Change clamp from `0.05f` to `0.0f` in Sequencer.cpp (lines 370, 390)
- [ ] **Fix 2**: In `ApplyFuse()`, don't boost a voice above 0 if its base density was 0
- [ ] **Fix 3**: In `ApplyCouple()`, don't gap-fill for a voice if its density is 0
- [ ] Add unit test: density=0 produces zero triggers regardless of BROKEN/DRIFT/FUSE/COUPLE
- [ ] *(spec: [pulse-field], [v3-critical-rules])*

### DRIFT=0 Zero Variation [v3-critical-rules]

**Root Cause Analysis**: The `ShouldStepFireWithDrift()` algorithm is correct — at DRIFT=0, all steps use `patternSeed_`. The bug is in the v2 legacy code still running in v3:

1. **ChaosModulator density bias** (Sequencer.cpp:339-370): `chaosSampleLow.densityBias` is added to density before the algorithm. ChaosModulator uses `std::minstd_rand` with internal state that's NOT deterministic per loop.
2. **flux_ = broken_** (line 123): SetBroken syncs flux_, so chaos is active whenever BROKEN>0

**Design Principle**: In v3, BROKEN controls variation (deterministic via seeds), DRIFT controls whether it changes per-loop. ChaosModulator's density bias is redundant and breaks DRIFT=0.

**Fixes (clean v3 path):**
- [ ] **Fix 1**: In v3 path, skip `chaosSampleLow.densityBias` addition — just use raw densities
- [ ] **Fix 2**: Or: Only add densityBias when DRIFT>0 (conditional chaos)
- [ ] Verify at DRIFT=0, ALL steps use `patternSeed_` (algorithm already correct)
- [ ] Add unit test: DRIFT=0 produces identical pattern across multiple loops
- [ ] Add unit test: DRIFT=0 + BROKEN=100% still produces same chaos pattern every loop
- [ ] *(spec: [drift-control], [v3-critical-rules])*

## Phase 2: RATCHET Parameter [ratchet-control]

### Parameter Setup
- [ ] Add `ratchet_` member to Sequencer (0.0-1.0)
- [ ] Add `SetRatchet()` / `GetRatchet()` methods
- [ ] Map K4+Shift to RATCHET in Performance Mode
- [ ] Add soft takeover for RATCHET parameter
- [ ] *(spec: [ratchet-control], [duopulse-controls])*

### Fill Zone Updates [phrase-modulation]
- [ ] Add `isMidPhrase` (40-60%) to `PhrasePosition` struct
- [ ] Update `GetPhraseWeightBoost()` to accept DRIFT and RATCHET parameters
- [ ] Fill density boost scales with RATCHET (0-30%)
- [ ] DRIFT gates fill probability — at DRIFT=0, no fills occur
- [ ] *(spec: [ratchet-control], [phrase-modulation])*

### Ratcheting (32nd Subdivisions)
- [ ] Implement ratcheting logic when RATCHET > 50%
- [ ] Ratchets occur in fill zones only
- [ ] Ratchet density increases toward phrase end
- [ ] *(spec: [ratchet-control])*

### Fill Resolution
- [ ] Velocity ramp: fills get louder toward phrase end (1.0-1.3×)
- [ ] Resolution accent on phrase downbeat (1.0-1.5× based on RATCHET)
- [ ] Update `GetPhraseAccent()` to accept RATCHET parameter
- [ ] *(spec: [ratchet-control])*

### Tests
- [ ] Unit test: DRIFT=0 + any RATCHET = no fills
- [ ] Unit test: DRIFT=100% + RATCHET=0 = subtle fills
- [ ] Unit test: DRIFT=100% + RATCHET=100% = intense fills with ratcheting
- [ ] Unit test: Fill zones at correct phrase positions

## Phase 3: Code Quality — Header/CPP Split

### BrokenEffects Refactor
- [ ] Create `src/Engine/BrokenEffects.cpp`
- [ ] Move function implementations from `BrokenEffects.h` to `BrokenEffects.cpp`
- [ ] Keep only function declarations and inline helpers in header
- [ ] Verify tests still pass after refactor

### PulseField Refactor
- [ ] Create `src/Engine/PulseField.cpp` (if not already exists)
- [ ] Move `ShouldStepFire()` and `ShouldStepFireWithDrift()` implementations to cpp
- [ ] Keep weight tables and inline helpers in header
- [ ] Verify tests still pass after refactor

### LedIndicator Refactor (Optional)
- [ ] Create `src/Engine/LedIndicator.cpp`
- [ ] Move non-trivial function implementations to cpp
- [ ] Keep inline helpers in header

### Build Verification
- [ ] Run `make clean && make` — verify no linker errors
- [ ] Run `make test` — verify all tests pass
- [ ] Verify firmware size is reasonable

## Phase 4: Documentation Updates

- [ ] Update README with RATCHET description
- [ ] Update control layout in README
- [ ] Update `docs/specs/main.md` acceptance criteria as items complete
- [ ] Mark task complete in `docs/tasks/index.md`

## Notes

- RATCHET works with DRIFT, not independently:
  - DRIFT = "Do fills happen?" (probability gate)
  - RATCHET = "How intense are fills?" (intensity control)
- Header/CPP split improves compile times and reduces coupling
- All changes should maintain v2 fallback compatibility (`#ifdef USE_PULSE_FIELD_V3`)

---

### Acceptance Criteria Summary

| Requirement | Status |
|-------------|--------|
| DENSITY=0 = absolute silence | ☐ |
| DRIFT=0 = zero variation | ☐ |
| RATCHET controls fill intensity | ☐ |
| Fills target phrase boundaries | ☐ |
| Code moved to cpp files | ☐ |
| All tests pass | ☐ |
