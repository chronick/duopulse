---
id: 66
slug: config-patternfield-wiring
title: "Wire Runtime Zone Thresholds into PatternField"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/config-patternfield-wiring
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
depends_on:
  - 56  # Weight-based blending (provides AlgorithmConfig)
  - 63  # Sensitivity analysis (blocked by this disconnect)
---

# Task 66: Wire Runtime Zone Thresholds into PatternField

## Objective

Make PatternField zone thresholds runtime-configurable so that sensitivity analysis can vary them and observe effects on pattern output. This fixes the design disconnect where CLI weight overrides affected debug output but not actual pattern generation.

## Context

### Design Review Finding

A design review revealed a conceptual mismatch between two configuration systems:

1. **AlgorithmConfig** (`inc/algorithm_config.h`):
   - Parameters: `kEuclideanFadeStart`, `kEuclideanFadeEnd`, `kSyncopationCenter`, etc.
   - Purpose: Compute normalized blend weights (e.g., "60% euclidean + 30% syncopation")
   - Used by: `AlgorithmWeights.cpp` for `ComputeAlgorithmWeights()`
   - **Issue**: Only used in debug output (`PrintDebugWeights()`), NOT in actual pattern generation

2. **PatternField zone constants** (`src/Engine/PatternField.h`):
   - Parameters: `kShapeZone1End`, `kShapeCrossfade1End`, `kShapeZone2aEnd`, etc.
   - Purpose: Define WHICH algorithm to use at each SHAPE value
   - Used by: `PatternField.cpp` for `ComputeShapeBlendedWeights()`
   - **Issue**: Hardcoded as `constexpr`, cannot be overridden at runtime

The CLI weight override args (`--euclidean-fade-start`, etc.) update AlgorithmConfig values but never reach the actual pattern generation path. They're cosmetic only.

### Impact

1. **Sensitivity analysis produces all zeros** - varying AlgorithmConfig has no effect on patterns
2. **Lever recommendations are useless** - can't identify which weights affect metrics
3. **Task 55 (iterate command) is blocked** - needs working sensitivity data

### Target State

- PatternField zone thresholds configurable at runtime for pattern_viz tool
- Firmware continues to use compile-time constexpr (zero overhead)
- Sensitivity sweeps produce meaningful non-zero values
- CLI arg names match PatternField terminology (not AlgorithmConfig)

## Design

### Approach: PatternFieldConfig Struct with Default Parameter

```cpp
// src/Engine/PatternField.h

/**
 * Runtime-configurable zone thresholds for SHAPE parameter.
 * Default values match the original constexpr constants.
 */
struct PatternFieldConfig {
    float shapeZone1End = 0.28f;        // End of pure stable zone
    float shapeCrossfade1End = 0.32f;   // End of stable->syncopation crossfade
    float shapeZone2aEnd = 0.48f;       // End of lower syncopation zone
    float shapeCrossfade2End = 0.52f;   // End of mid syncopation crossfade
    float shapeZone2bEnd = 0.68f;       // End of upper syncopation zone
    float shapeCrossfade3End = 0.72f;   // End of syncopation->wild crossfade
    
    // Validation: thresholds must be monotonically increasing
    bool IsValid() const {
        return shapeZone1End < shapeCrossfade1End &&
               shapeCrossfade1End < shapeZone2aEnd &&
               shapeZone2aEnd < shapeCrossfade2End &&
               shapeCrossfade2End < shapeZone2bEnd &&
               shapeZone2bEnd < shapeCrossfade3End &&
               shapeCrossfade3End <= 1.0f;
    }
};

/// Default config matching original constexpr values
static constexpr PatternFieldConfig kDefaultPatternFieldConfig{};

/**
 * Compute shape-blended weights using configurable zone thresholds.
 * 
 * @param config Zone threshold configuration (default: original constexpr values)
 */
void ComputeShapeBlendedWeights(float shape, float energy,
                                 uint32_t seed, int patternLength,
                                 float* outWeights,
                                 const PatternFieldConfig& config = kDefaultPatternFieldConfig);
```

### Why This Approach

1. **Backward compatible**: Default parameter means existing callers don't change
2. **Zero overhead for firmware**: Default config is constexpr
3. **Testable**: Config struct can be validated and tested independently
4. **Clear semantics**: PatternFieldConfig is about zone boundaries, separate from AlgorithmConfig blend weights

### CLI Arg Changes

Rename pattern_viz args to match PatternField terminology:

| Old (misleading) | New (accurate) |
|------------------|----------------|
| `--euclidean-fade-start` | `--shape-zone1-end` |
| `--euclidean-fade-end` | `--shape-crossfade1-end` |
| `--syncopation-center` | `--shape-zone2a-end` |
| `--syncopation-width` | `--shape-crossfade2-end` |
| `--random-fade-start` | `--shape-zone2b-end` |
| `--random-fade-end` | `--shape-crossfade3-end` |

Also add validation when parsing:
```cpp
if (!config.IsValid()) {
    std::cerr << "Error: Zone thresholds must be monotonically increasing\n";
    return 1;
}
```

### Out of Scope: AlgorithmConfig Deprecation

This task does NOT address the AlgorithmConfig system. A separate audit should determine if:
- AlgorithmConfig should be deprecated (since it's not used in actual generation)
- AlgorithmWeights.cpp provides value beyond debug output
- The two systems should be unified

This is left for future work to avoid scope creep.

## Subtasks

### PatternField Refactoring
- [ ] Create `PatternFieldConfig` struct in `PatternField.h`
- [ ] Add `IsValid()` validation method
- [ ] Add default parameter to `ComputeShapeBlendedWeights()` signature
- [ ] Replace hardcoded `kShapeZone*` constants with `config.*` members
- [ ] Keep original `constexpr` constants for reference (maybe rename to `kDefaultShapeZone*`)

### pattern_viz Updates
- [ ] Rename CLI args to match PatternField terminology
- [ ] Create `PatternFieldConfig` from CLI args
- [ ] Validate config before use
- [ ] Pass config to `GeneratePattern()` call chain
- [ ] Update `--help` output with new arg names

### GeneratePattern Integration
- [ ] Add `PatternFieldConfig` parameter to `GeneratePattern()` (or pass via PatternParams)
- [ ] Wire config through to `ComputeShapeBlendedWeights()` calls
- [ ] Ensure firmware path still uses default (zero overhead)

### Sensitivity Analysis Verification
- [ ] Update sweep scripts to use new CLI arg names
- [ ] Re-run sensitivity sweep
- [ ] Verify non-zero sensitivity values
- [ ] Update sensitivity matrix with real data

### Tests
- [ ] Add unit tests for `PatternFieldConfig::IsValid()`
- [ ] Test that default config produces identical output to old code
- [ ] Test that modified config produces different output
- [ ] Verify all 373+ existing tests pass unchanged (via default parameter)

### Documentation
- [ ] Update Task 63 known limitations section (remove or mark resolved)
- [ ] Update epic file to note this task's completion
- [ ] Document CLI arg mapping in pattern_viz help

## Acceptance Criteria

- [ ] `PatternFieldConfig` struct exists with validation
- [ ] `ComputeShapeBlendedWeights()` accepts optional config parameter
- [ ] Default config produces identical output to current behavior
- [ ] pattern_viz CLI args renamed to `--shape-zone*-end` format
- [ ] Sensitivity sweep produces non-zero values for zone parameters
- [ ] All existing tests pass without modification
- [ ] No performance regression in firmware path

## Implementation Notes

### Files to Modify

- `src/Engine/PatternField.h` - Add config struct, update function signature
- `src/Engine/PatternField.cpp` - Use config members instead of constants
- `src/Engine/PatternGenerator.h` - Optionally add config to PatternParams
- `src/Engine/PatternGenerator.cpp` - Pass config through call chain
- `tools/pattern_viz.cpp` - Rename CLI args, create config, validate
- `scripts/sensitivity/run-sweep.js` - Update CLI arg names

### Files to Create

- `tests/test_patternfield_config.cpp` - Config validation tests

### Backward Compatibility

The default `PatternFieldConfig{}` MUST produce identical patterns to the current code:
```cpp
// These must be equivalent:
ComputeShapeBlendedWeights(shape, energy, seed, length, weights);
ComputeShapeBlendedWeights(shape, energy, seed, length, weights, PatternFieldConfig{});
```

This is verified by running all existing tests without modification.

## Test Plan

1. **Baseline capture**: Before changes, capture pattern output for test cases
2. **Refactor**: Apply changes with default config
3. **Regression test**: Verify identical output with default config
4. **Config variation test**: Verify different output with modified config
5. **Sensitivity re-run**: Run sweep, verify non-zero values

```bash
# Baseline
./build/pattern_viz --shape=0.3 --seed=42 > baseline.txt

# After refactor (should be identical)
./build/pattern_viz --shape=0.3 --seed=42 > refactored.txt
diff baseline.txt refactored.txt  # Should be empty

# With config override (should differ)
./build/pattern_viz --shape=0.3 --seed=42 --shape-zone1-end=0.35 > modified.txt
diff baseline.txt modified.txt  # Should show differences

# Sensitivity sweep
node scripts/sensitivity/run-sweep.js
cat metrics/sensitivity-matrix.json  # Verify non-zero values
```

## Dependencies

- **Task 56** (Weight-Based Blending): Completed - provides AlgorithmConfig
- **Task 63** (Sensitivity Analysis): Completed but data is zeros - this task unblocks it

## Blocking

- **Task 55** (Iterate Command): Needs lever recommendations from working sensitivity analysis

## Estimated Effort

3-4 hours
- PatternField refactoring: 1h
- pattern_viz CLI updates: 45min
- Test verification: 1h
- Sensitivity re-run: 30min
- Documentation: 15min

## Related Epic

This task is part of **Phase 2** of the Semi-Auto Iteration epic (2026-01-18-semi-auto-iteration.md).
It addresses a design limitation discovered during Task 63 implementation.

## Design Review Notes

This task was revised based on design review feedback:

1. **Clarified scope**: Focus ONLY on PatternField zone thresholds, not AlgorithmConfig integration
2. **Renamed CLI args**: Use PatternField terminology (`--shape-zone*-end`) not AlgorithmConfig terms
3. **Added validation**: Zone thresholds must be monotonically increasing
4. **Out of scope**: AlgorithmConfig audit/deprecation is separate future work
5. **Default parameter**: Maintains backward compatibility for all existing callers
