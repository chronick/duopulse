# Common Pitfalls and Bugs from Project History

This document catalogs actual bugs found during development, to help reviewers spot similar issues.

## 1. CV Modulation Bugs

### Issue: CV Double-Application (Commit 969aa30)
**Symptom**: CV inputs had 2x the expected effect on parameters.

**Root Cause**: CV values were applied twice - once in control processing, once in parameter update.

```cpp
// ❌ WRONG: CV applied twice
ProcessControls() {
    controls.energyCV = patch.cv_in[0].Process();
    controls.energy = Clamp(controls.energyKnob + controls.energyCV, 0.0f, 1.0f);  // Applied once
}

UpdateSequencer() {
    float energy = controls.energy;
    energy += controls.energyCV;  // ❌ Applied AGAIN!
    sequencer.SetEnergy(energy);
}

// ✅ CORRECT: Single-pass CV application
ProcessControls() {
    controls.energyCV = patch.cv_in[0].Process();
    controls.energy = Clamp(controls.energyKnob + controls.energyCV, 0.0f, 1.0f);
}

UpdateSequencer() {
    sequencer.SetEnergy(controls.energy);  // ✅ Use processed value
}
```

**Review Checklist**:
- [ ] CV values applied exactly once in the control flow
- [ ] No redundant `Set*CV()` calls after control processing

### Issue: Unpatched CV Baseline Assumption (Commit 96e206c)
**Symptom**: With nothing patched to CV inputs, density became 0 and no triggers fired.

**Root Cause**: Code assumed unpatched CV inputs read 0.5V (2.5V). Actual hardware reads 0V.

```cpp
// ❌ WRONG: Assumes unpatched = 0.5V baseline
float cv = patch.cv_in[0].Process();  // Reads 0.0V when unpatched!
float modulated = knob + (cv - 0.5f);  // -0.5 offset kills parameter!

// ✅ CORRECT: Use additive modulation (MixControl pattern)
float cv = patch.cv_in[0].Process();
float modulated = Clamp(knob + cv, 0.0f, 1.0f);  // 0V = no effect
```

**Review Checklist**:
- [ ] CV modulation uses additive pattern (0V = no change)
- [ ] No assumptions about unpatched CV voltage
- [ ] Clamping applied after modulation

## 2. Configuration Parameter Bugs

### Issue: Config Parameter Ignored (Commit be7b58f)
**Symptom**: Swing config knob had no effect; timing used wrong parameter.

**Root Cause**: Function used old parameter (flavorCV) instead of new config parameter (swing).

```cpp
// ❌ WRONG: Using wrong parameter
float ComputeSwing(float flavorCV, float archetypeBase, EnergyZone zone) {
    // Function signature changed but callers not updated!
    return archetypeBase * (1.0f + flavorCV);  // ❌ Should use swing config!
}

// Called with:
ComputeSwing(controls.flavorCV, 0.5f, zone);  // ❌ Wrong variable!

// ✅ CORRECT: Use config parameter
float ComputeSwing(float swingConfig, float archetypeBase, EnergyZone zone) {
    return archetypeBase * (1.0f + swingConfig);
}

// Called with:
ComputeSwing(controls.swing, 0.5f, zone);  // ✅ Correct!
```

**Review Checklist**:
- [ ] Function signatures match intent (parameter names accurate)
- [ ] Callers pass correct variables (not leftover from refactor)
- [ ] Regression tests verify config parameters are actually used

### Issue: Wrong Signal Used for Output (Commit be7b58f)
**Symptom**: AUX output modes (HAT/EVENT) always high when clock present.

**Root Cause**: Output used `IsClockHigh()` instead of `IsAuxHigh()`.

```cpp
// ❌ WRONG: Using clock signal for aux output
if (auxMode == AUX_HAT) {
    patch.gate_out_aux.Write(sequencer.IsClockHigh());  // ❌ Wrong signal!
}

// ✅ CORRECT: Use aux-specific getter
if (auxMode == AUX_HAT) {
    patch.gate_out_aux.Write(sequencer.IsAuxHigh());  // ✅ Correct!
}
```

**Review Checklist**:
- [ ] Output signals use mode-appropriate getters
- [ ] No copy-paste errors in conditional branches
- [ ] Each output mode tested independently

## 3. Invariant Violations

### Issue: DENSITY=0 Invariant Bypassed (Commits 3ddcfc6, 0289e38, e8957ae)
**Symptom**: Even with DENSITY=0 (ENERGY=0), hits still generated.

**Root Cause**: Chaos/drift systems added hits after budget allocation, bypassing DENSITY check.

```cpp
// ❌ WRONG: Adds hits after budget check
void GeneratePattern(float density) {
    int hitCount = ComputeHitBudget(density);  // Correctly returns 0 if density=0

    // Generate hits...

    // ❌ Chaos adds hits AFTER budget allocation!
    if (chaosEnabled) {
        AddRandomHits(3);  // Violates DENSITY=0!
    }
}

// ✅ CORRECT: Chaos scales with density
void GeneratePattern(float density) {
    int hitCount = ComputeHitBudget(density);

    if (hitCount == 0) {
        return;  // ✅ Hard stop - no hits means NO HITS
    }

    // Generate hits...

    // Chaos only active if density > 0
    if (chaosEnabled && hitCount > 0) {
        ModifyExistingHits();  // ✅ Modifies, doesn't add
    }
}
```

**Review Checklist**:
- [ ] DENSITY=0 → zero hits (hard invariant)
- [ ] DRIFT=0 → no evolution (hard invariant)
- [ ] No subsystem bypasses critical invariants
- [ ] Early returns when invariant violated

## 4. Array/Buffer Bounds

### Issue: Pattern Generation Only for 32 Steps (Commit 1288d21)
**Symptom**: Steps 32-63 never had hits, even with 64-step pattern length.

**Root Cause**: Loop only iterated to 32, not full pattern length.

```cpp
// ❌ WRONG: Hardcoded loop limit
void GeneratePattern(int patternLength) {
    for (int step = 0; step < 32; step++) {  // ❌ Should be patternLength!
        pattern[step] = ComputeHit(step);
    }
}

// ✅ CORRECT: Use actual pattern length
void GeneratePattern(int patternLength) {
    for (int step = 0; step < patternLength; step++) {
        pattern[step] = ComputeHit(step);
    }
}
```

**Review Checklist**:
- [ ] Loops use parameter/variable, not magic number
- [ ] Buffer size matches actual data length
- [ ] No off-by-one errors (< vs <=)

## 5. Hardware Mapping Errors

### Issue: Clock Division Mapping Off-Center (Commit be7b58f)
**Symptom**: ×1 clock division wasn't centered at 12 o'clock (50%).

**Root Cause**: Threshold ranges didn't match spec.

```cpp
// ❌ WRONG: ×1 at 30-40% (not centered)
int MapToClockDivision(float knobValue) {
    if (knobValue < 0.30f) return 1;  // ÷8
    if (knobValue < 0.40f) return 2;  // ÷4
    if (knobValue < 0.60f) return 4;  // ÷2
    // ×1 range is too small!
    return 8;  // ×1
}

// ✅ CORRECT: ×1 centered at 42-58% per spec
int MapToClockDivision(float knobValue) {
    if (knobValue < 0.25f) return 1;   // ÷8
    if (knobValue < 0.42f) return 2;   // ÷4
    if (knobValue < 0.58f) return 4;   // ÷2 (intentional bias)
    if (knobValue < 0.75f) return 8;   // ×1 (CENTERED!)
    return 16;  // ×2
}
```

**Review Checklist**:
- [ ] Detent positions match spec (especially 12 o'clock = "normal")
- [ ] Ranges tested at boundaries (49%, 50%, 51%)
- [ ] User-facing defaults feel natural (×1, straight timing, etc.)

## 6. Testing Blind Spots

### Lesson: Regression Tests for Config Parameters (Commit be7b58f)
**Problem**: Swing config bug wasn't caught because no test verified the parameter was actually used.

```cpp
// ❌ Insufficient test: only checks output range
TEST_CASE("ComputeSwing returns reasonable values") {
    float result = ComputeSwing(0.5f, 0.5f, EnergyZone::PEAK);
    REQUIRE(result >= 0.5f);
    REQUIRE(result <= 0.7f);
    // ✅ Passes even if swing parameter ignored!
}

// ✅ Better test: verifies parameter effect
TEST_CASE("ComputeSwing uses swing config parameter") {
    float low  = ComputeSwing(0.0f, 0.5f, EnergyZone::PEAK);
    float high = ComputeSwing(1.0f, 0.5f, EnergyZone::PEAK);

    REQUIRE(low < high);  // ✅ Fails if parameter ignored!
}
```

**Review Checklist**:
- [ ] Tests verify parameter is *used*, not just that function compiles
- [ ] Edge cases tested (0%, 50%, 100%)
- [ ] Regression tests document bug fixes with `[regression]` tag

## Quick Reference: Bug Commits

| Bug | Commit | Fix |
|-----|--------|-----|
| CV double-application | 969aa30 | Remove redundant SetCV calls |
| CV baseline assumption | 96e206c | Use additive MixControl pattern |
| Swing config ignored | be7b58f | Update function signature and callers |
| Clock mapping off-center | be7b58f | Adjust threshold ranges |
| AUX wrong signal | be7b58f | Use IsAuxHigh() not IsClockHigh() |
| DENSITY=0 bypass | 3ddcfc6 | Early return, no chaos/drift |
| Pattern length hardcoded | 1288d21 | Use patternLength variable |
| Flash write in audio | 1ccd75f | Defer to main loop |

## Prevention Strategies

1. **Write the test first**: If adding a config parameter, test that changing it has an effect
2. **Test invariants**: DENSITY=0, DRIFT=0, valid ranges
3. **Test at boundaries**: 0%, 25%, 50%, 75%, 100%
4. **Regression tag**: `TEST_CASE("...", "[regression]")` for bug fixes
5. **Code review focus**: Parameter usage, signal routing, invariant preservation
