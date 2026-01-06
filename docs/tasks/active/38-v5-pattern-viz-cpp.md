---
id: 38
slug: 38-v5-pattern-viz-cpp
title: "V5 C++ Pattern Visualization Test Tool"
status: completed
created_date: 2026-01-06
updated_date: 2026-01-06
completed_date: 2026-01-06
branch: feature/38-v5-pattern-viz-cpp
spec_refs:
  - "v5-design-final.md"
  - "hardware-test-plan.md"
depends_on:
  - 27
  - 28
  - 29
  - 30
  - 35
commits:
  - "<pending>"
---

# Task 38: V5 C++ Pattern Visualization Test Tool

## Objective

Create a C++ test utility that outputs deterministic drum patterns for given parameters and seed, enabling pattern evaluation without hardware. This extends the test framework to validate V5 generation algorithms visually.

## Context

In V4, we created `scripts/pattern-viz-debug/` - a Python replica of the firmware to evaluate patterns offline. For V5, we want the same capability but **in C++** as part of the existing test framework. This ensures:

1. **Exact algorithm match**: No translation errors between Python and C++
2. **Test integration**: Runs with `make test`, catches regressions
3. **Deterministic output**: Same params + seed = identical pattern every time
4. **Velocity visibility**: Shows CV velocity values, not just hit/no-hit

## Subtasks

- [x] Create `tests/pattern_viz_test.cpp` test file
- [x] Add pattern output formatter (ASCII grid visualization)
- [x] Implement `PrintPattern()` for Voice 1, Voice 2, Aux
- [x] Add velocity column output (0.00-1.00 range)
- [x] Create test cases for each SHAPE zone (stable/syncopated/wild)
- [x] Create test cases for AXIS X/Y extremes
- [x] Create test cases for COMPLEMENT voice relationship
- [x] Add seed-determinism verification test
- [ ] Add CLI-runnable pattern generator (optional: `./test_runner --viz`) - skipped (not needed)
- [x] All tests pass (`make test`)

## Acceptance Criteria

- [x] Pattern output shows 32 steps in readable grid format
- [x] Voice 1, Voice 2, Aux displayed with hit markers
- [x] Velocity values printed alongside hits
- [x] Same seed + params produces identical output on every run
- [x] Tests cover all 3 SHAPE zones
- [x] Tests cover AXIS X/Y full range
- [x] COMPLEMENT relationship visible in output
- [x] Build compiles without errors
- [x] All tests pass

## Output Format Specification

### Pattern Grid Format

```
=== Pattern Visualization ===
Params: ENERGY=0.50 SHAPE=0.25 AXIS_X=0.50 AXIS_Y=0.50
Config: DRIFT=0.00 ACCENT=0.50 SWING=0.50
Seed: 0xDEADBEEF
Pattern Length: 32 steps

Step  V1  V2  Aux  V1_Vel  V2_Vel  Aux_Vel  Metric
────────────────────────────────────────────────────
 0    ●   ·   ●    0.95    ----    0.70     1.00  ▌▌▌▌
 1    ·   ·   ·    ----    ----    ----     0.25  ▌
 2    ·   ·   ●    ----    ----    0.55     0.50  ▌▌
 3    ·   ●   ·    ----    0.45    ----     0.25  ▌
 4    ●   ·   ●    0.88    ----    0.65     0.75  ▌▌▌
 5    ·   ·   ·    ----    ----    ----     0.25  ▌
 6    ·   ·   ●    ----    ----    0.50     0.50  ▌▌
 7    ·   ●   ·    ----    0.42    ----     0.25  ▌
 8    ●   ·   ●    0.92    ----    0.68     0.90  ▌▌▌▌
...
31   ·   ●   ·    ----    0.40    ----     0.25  ▌

Summary:
  V1 hits: 8/32 (25%)
  V2 hits: 6/32 (19%)
  Aux hits: 16/32 (50%)
  V1 avg velocity: 0.87
  V2 avg velocity: 0.44
```

### Symbols

| Symbol | Meaning |
|--------|---------|
| `●` | Hit on this step |
| `·` | No hit |
| `----` | No velocity (no hit) |
| `▌` bars | Metric weight visualization |

## Test Cases

### 1. SHAPE Zone Tests

```cpp
TEST_CASE("SHAPE zones produce distinct patterns") {
    SECTION("Zone 1: Stable (SHAPE=0.15)") {
        // Expect: Four-on-floor, strong downbeats
        // V1 hits on 0, 8, 16, 24
    }

    SECTION("Zone 2: Syncopated (SHAPE=0.50)") {
        // Expect: Offbeat emphasis, anticipations
        // V1 may skip beat 3, hit before beat 1
    }

    SECTION("Zone 3: Wild (SHAPE=0.85)") {
        // Expect: Irregular, chaotic
        // High variation, broken feel
    }
}
```

### 2. AXIS Tests

```cpp
TEST_CASE("AXIS X biases beat position") {
    // AXIS_X=0.0: Grounded (downbeat heavy)
    // AXIS_X=1.0: Floating (offbeat heavy)
}

TEST_CASE("AXIS Y controls intricacy") {
    // AXIS_Y=0.0: Simple, sparse
    // AXIS_Y=1.0: Complex, busy
}
```

### 3. COMPLEMENT Tests

```cpp
TEST_CASE("COMPLEMENT fills anchor gaps") {
    // V2 hits should fall in V1 gaps
    // No simultaneous V1+V2 hits
}
```

### 4. Determinism Tests

```cpp
TEST_CASE("Same seed produces identical patterns") {
    auto pattern1 = GeneratePattern(params, 0xDEADBEEF);
    auto pattern2 = GeneratePattern(params, 0xDEADBEEF);
    REQUIRE(pattern1 == pattern2);
}
```

## Implementation Notes

### File Structure

```
tests/
├── catch_main.cpp         # Existing
├── test_*.cpp             # Existing tests
└── pattern_viz_test.cpp   # NEW: Pattern visualization tests
```

### Key Functions to Test

From V5 implementation:
- `ComputeShapeBlendedWeights()` - SHAPE zone blending
- `ApplyAxisBias()` - AXIS X/Y bidirectional bias
- `ApplyComplementRelationship()` - Gap-filling shimmer
- `ComputeAccentVelocity()` - Position-aware velocity
- `GenerateHatBurst()` - Aux pattern generation

### Optional: CLI Runner

If time permits, add a standalone pattern generator:

```bash
# Run pattern visualization
./build/test_runner --viz --energy=0.5 --shape=0.25 --seed=0xDEADBEEF

# Output multiple patterns for comparison
./build/test_runner --viz --sweep-shape --seed=0xDEADBEEF
```

This would be useful for rapid iteration during musicality tuning.

## Related Files

- `tests/pattern_viz_test.cpp` - New test file
- `src/Engine/GenerationPipeline.cpp` - Pattern generation
- `src/Engine/PatternField.cpp` - SHAPE/AXIS blending
- `src/Engine/VoiceRelationship.cpp` - COMPLEMENT logic
- `src/Engine/VelocityCompute.cpp` - ACCENT velocity

## References

- V4 Python equivalent: `scripts/pattern-viz-debug/`
- V5 Design: `docs/design/v5-ideation/v5-design-final.md`
- Hardware Test Plan: `docs/misc/duopulse-v5/hardware-test-plan.md`
