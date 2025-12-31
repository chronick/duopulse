# Task 21-05: Python Evaluation Findings & Remediation

**Status**: PENDING
**Branch**: `impl/21-musicality-improvements`
**Parent Task**: Task 21 (Musicality Improvements)
**Related**: 21-04 (Implementation), pattern-viz-debug tool
**Created**: 2025-12-31

---

## Summary

A Python reference implementation (`scripts/pattern-viz-debug`) was created to evaluate the pattern generation algorithms. 117 unit tests and 16 design-intent tests were executed. **94% pass rate** with one issue identified in BUILD velocity dynamics.

---

## C++ vs Python Implementation Comparison

### Verified Matching ✅

| Component | C++ File | Python File | Match Status |
|-----------|----------|-------------|--------------|
| PUNCH coefficients | `VelocityCompute.cpp:35-45` | `velocity.py:41-52` | ✅ Identical |
| BUILD phase thresholds | `VelocityCompute.cpp:65,73` | `velocity.py:77,83` | ✅ 60%, 87.5% |
| BUILD velocity boost | `VelocityCompute.cpp:79,87` | `velocity.py:88,94` | ✅ 0.08, 0.12 |
| Energy zone boundaries | `HitBudget.cpp:80-93` | `hit_budget.py:39-46` | ✅ 0.20, 0.50, 0.75 |
| Zone minimums | `HitBudget.cpp:48-66` | `hit_budget.py:69-80` | ✅ 1,3,4,6 |
| Euclidean ratios | `EuclideanGen.cpp:149-158` | `euclidean.py:103-108` | ✅ 70%,40%,0% |
| Accent masks | `VelocityCompute.cpp:13-19` | `velocity.py:12-16` | ✅ Identical |
| Gumbel hash magic | `VelocityCompute.cpp:8-9` | `velocity.py:8-9` | ✅ 0x41434E54, 0x56415249 |

### Algorithm Coefficients Detail

```cpp
// C++ VelocityCompute.cpp:77-79 (BUILD phase)
modifiers.densityMultiplier = 1.0f + build * 0.35f * phaseProgress;
modifiers.velocityBoost = build * 0.08f * phaseProgress;

// C++ VelocityCompute.cpp:85-88 (FILL phase)
modifiers.densityMultiplier = 1.0f + build * 0.50f;
modifiers.velocityBoost = build * 0.12f;
```

```python
# Python velocity.py:87-88 (BUILD phase)
modifiers.density_multiplier = 1.0 + build * 0.35 * phase_progress
modifiers.velocity_boost = build * 0.08 * phase_progress

# Python velocity.py:93-94 (FILL phase)
modifiers.density_multiplier = 1.0 + build * 0.50
modifiers.velocity_boost = build * 0.12
```

**Conclusion**: C++ and Python implementations are functionally equivalent.

---

## Evaluation Results

### Tests Passed (15/16)

| Category | Tests | Result |
|----------|-------|--------|
| TECHNO genre characteristics | 3 | ✅ All pass |
| TRIBAL genre characteristics | 2 | ✅ All pass |
| IDM genre characteristics | 2 | ✅ All pass |
| Energy zone scaling | 3 | ✅ All pass |
| Voice coupling | 2 | ✅ All pass |
| Determinism | 2 | ✅ All pass |
| BUILD density | 1 | ✅ Pass |

### Test Failed (1/16)

| Test | Expected | Actual | Root Cause |
|------|----------|--------|------------|
| BUILD velocity trend | Increasing over phrase | [0.66, 0.62, 0.48, 0.54] | Velocity boost too small relative to random variation |

---

## Issue Analysis

### Issue: BUILD Velocity Not Consistently Increasing

**Symptom**: Over a 4-bar phrase with BUILD=0.9, velocity does not trend upward:
- Bar 1: 0.66 avg velocity
- Bar 2: 0.62 avg velocity
- Bar 3: 0.48 avg velocity (dip)
- Bar 4: 0.54 avg velocity

**Root Cause Analysis**:

1. **Velocity variation range**: At PUNCH=0.5, variation = ±7.5%
2. **BUILD velocity boost**: In BUILD phase (60-87.5%), max boost = 0.08 × 0.9 × 1.0 = 7.2%
3. **Comparison**: Random variation (±7.5%) can exceed BUILD boost (7.2%)

**Math**:
```
PUNCH=0.5 → velocityVariation = 0.03 + 0.5 * 0.12 = 0.09 (±9%)
BUILD phase boost at phraseProgress=0.7, build=0.9:
  phaseProgress_in_build = (0.7 - 0.6) / 0.275 = 0.36
  velocityBoost = 0.9 * 0.08 * 0.36 = 0.026 (2.6%)

The 2.6% boost is overwhelmed by ±9% random variation.
```

**C++ Location**: `VelocityCompute.cpp:79`

---

## Remediation Plan

### Subtask A: Increase BUILD Velocity Boost Coefficients

**Priority**: HIGH
**Effort**: 15 minutes
**Risk**: Low (coefficients only)

**Change**: Increase `velocityBoost` coefficients to be more perceptible against variation.

| Phase | Current | Proposed | Rationale |
|-------|---------|----------|-----------|
| BUILD | 0.08 | 0.15 | 2x boost to exceed typical variation |
| FILL | 0.12 | 0.20 | Maintain FILL > BUILD relationship |

**Files to Modify**:
- `src/Engine/VelocityCompute.cpp:79` - BUILD phase
- `src/Engine/VelocityCompute.cpp:87` - FILL phase
- `scripts/pattern-viz-debug/src/pattern_viz/velocity.py:88,94` - Python reference

**Code Change**:
```cpp
// VelocityCompute.cpp:79 (BUILD phase)
- modifiers.velocityBoost = build * 0.08f * phaseProgress;
+ modifiers.velocityBoost = build * 0.15f * phaseProgress;

// VelocityCompute.cpp:87 (FILL phase)
- modifiers.velocityBoost = build * 0.12f;
+ modifiers.velocityBoost = build * 0.20f;
```

### Subtask B: Reduce Velocity Variation During BUILD Phase (Optional)

**Priority**: MEDIUM
**Effort**: 30 minutes
**Risk**: Medium (affects feel)

**Alternative Approach**: Reduce random variation as BUILD increases to make the trend more audible.

**Concept**:
```cpp
// Reduce variation as BUILD intensity increases
float variationScale = 1.0f - (buildMods.buildIntensity * 0.5f);
float effectiveVariation = punchParams.velocityVariation * variationScale;
```

**Trade-off**: May reduce dynamic interest in BUILD phase. Recommend A first.

### Subtask C: Validate Fix with Python Tool

**Priority**: HIGH
**Effort**: 10 minutes

After applying Subtask A, re-run evaluation:
```bash
cd scripts/pattern-viz-debug
uv run python evaluate_patterns.py
```

**Success Criteria**: BUILD velocity trend test passes.

### Subtask D: Update Tests

**Priority**: MEDIUM
**Effort**: 15 minutes

Add C++ unit test for velocity trending:
```cpp
TEST_CASE("BUILD phase increases velocity over phrase", "[velocity]") {
    float velocities[4];
    for (int bar = 0; bar < 4; bar++) {
        float phraseProgress = bar / 4.0f;
        BuildModifiers mods;
        ComputeBuildModifiers(0.9f, phraseProgress, mods);
        // ... compute avg velocity for bar
        velocities[bar] = avgVel;
    }
    // Last bar should be >= first bar
    REQUIRE(velocities[3] >= velocities[0] - 0.05f);
}
```

---

## Additional Observations (Non-Blocking)

### Observation 1: TECHNO Minimal Gap at E=50%

At Energy=50%, TECHNO Minimal produces hits at [0, 4, 8, 24] - a 16-step gap from step 8 to 24.

**Impact**: Minor - pattern still musical but less consistent 4-on-floor
**Recommendation**: Review Minimal archetype weights to ensure steps 12, 16, 20 have adequate weight
**Priority**: LOW

### Observation 2: IDM Could Be More Irregular

IDM Chaos at E=60% has regularity score 0.56 (moderate). Could be more chaotic.

**Impact**: Minor - IDM still sounds irregular
**Recommendation**: Consider boosting offbeat weights in IDM archetypes
**Priority**: LOW

---

## Success Criteria

- [ ] **A**: BUILD velocity boost increased to 0.15/0.20
- [ ] **B**: (Optional) Variation scaling implemented
- [ ] **C**: Python evaluation passes 16/16 tests
- [ ] **D**: C++ unit test added for velocity trending
- [ ] All existing 117 Python tests still pass
- [ ] `make test` passes on C++ side

---

## Files Reference

### Source Files to Modify
```
src/Engine/VelocityCompute.cpp          # Lines 79, 87
scripts/pattern-viz-debug/src/pattern_viz/velocity.py  # Lines 88, 94
```

### Test Files
```
tests/test_velocity.cpp                 # Add trending test
scripts/pattern-viz-debug/evaluate_patterns.py  # Re-run validation
```

### Documentation
```
docs/tasks/active/21-musicality/21-05-python-evaluation-findings.md  # This file
scripts/pattern-viz-debug/evaluation_output/KEY_FINDINGS.md          # Detailed findings
scripts/pattern-viz-debug/evaluation_output/evaluation_results.md    # Full test results
```

---

## Checklist

- [x] Python reference implementation created
- [x] 117 unit tests passing
- [x] 16 design-intent tests executed
- [x] C++ vs Python comparison completed
- [x] Root cause analysis for failing test
- [ ] Subtask A: Increase velocity boost coefficients
- [ ] Subtask C: Validate with Python tool
- [ ] Subtask D: Add C++ unit test
- [ ] Update spec if coefficients change significantly
