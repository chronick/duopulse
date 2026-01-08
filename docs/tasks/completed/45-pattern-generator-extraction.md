---
id: 45
slug: 45-pattern-generator-extraction
title: "Pattern Generator Extraction: Shared Module for Firmware & Viz Tool"
status: completed
created_date: 2026-01-08
updated_date: 2026-01-08
completed_date: 2026-01-08
branch: feature/45-pattern-generator-extraction
commits:
  - c9fffb5
  - de07ca6
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
  - "docs/specs/main.md#6-axis-biasing"
  - "docs/specs/main.md#7-voice-relationship-complement"
related:
  - 38  # V5 Pattern Viz Tool
  - 40  # V5 Pattern Generator Extraction (partial)
  - 44  # V5 Anchor Seed Variation
---

# Task 45: Pattern Generator Extraction

## Objective

Extract pattern generation logic into a shared module that both firmware (`Sequencer.cpp`) and visualization tool (`pattern_viz.cpp`) use, ensuring identical pattern output and enabling easier algorithm iteration.

## Context

### Problem

Pattern generation code is **duplicated** between firmware and viz tool (~200 lines):

| Component | Location | Issue |
|-----------|----------|-------|
| Firmware | `Sequencer::GenerateBar()` | Full-featured, integrated with sequencer state |
| Viz Tool | `GeneratePattern()` | Simplified copy, may drift from firmware |

Both implement:
- `RotateWithPreserve()` - identical
- Anchor noise perturbation (Task 44) - identical
- Gumbel hit selection + guard rails
- COMPLEMENT shimmer placement
- Aux metric-based weight generation
- Velocity computation

### Solution

Create `PatternGenerator` module with pure functions that:
1. Take pattern parameters as input
2. Return hit masks and velocities as output
3. Have no dependency on sequencer state
4. Are usable by both firmware and tools

### Design Constraints

- **RT-safe**: No heap allocations (firmware runs in audio callback context)
- **Deterministic**: Same inputs → identical outputs
- **Testable**: Pure functions enable comprehensive unit testing
- **Minimal coupling**: Generator doesn't know about Sequencer class

## Subtasks

### Phase A: Create PatternGenerator Module

- [ ] Create `src/Engine/PatternGenerator.h` with:
  - `PatternParams` struct (energy, shape, axisX, axisY, drift, accent, seed, length)
  - `PatternResult` struct (v1Mask, v2Mask, auxMask, velocities[3][kMaxSteps])
  - `GeneratePattern()` pure function declaration
  - `RotateWithPreserve()` utility function (move from Sequencer.cpp)

- [ ] Create `src/Engine/PatternGenerator.cpp` with:
  - Core pattern generation algorithm extracted from `Sequencer::GenerateBar()`
  - Anchor weight generation with noise perturbation
  - Shimmer COMPLEMENT application
  - Aux weight generation with metric avoidance
  - Velocity computation for all voices

### Phase B: Update Firmware to Use Module

- [ ] Modify `Sequencer::GenerateBar()` to call `GeneratePattern()`
- [ ] Keep firmware-specific logic in Sequencer:
  - Long pattern handling (>32 steps, two-half generation)
  - Euclidean blending (genre-aware)
  - Fill boost modifiers
  - Soft repair pass
  - State machine integration
- [ ] Verify all existing tests pass

### Phase C: Update Viz Tool to Use Module

- [ ] Modify `tools/pattern_viz.cpp` to use shared `GeneratePattern()`
- [ ] Remove duplicated code:
  - Local `GeneratePattern()` function
  - Local `RotateWithPreserve()` function
  - Local `ComputeTargetHits()` helper
- [ ] Keep viz-specific code:
  - CLI argument parsing
  - Output formatters (grid, CSV, mask)
  - Sweep functionality

### Phase D: Add Verification Tests

- [ ] Create `tests/test_pattern_generator.cpp`:
  - Test `GeneratePattern()` produces valid masks
  - Test determinism (same params → same output)
  - Test parameter sweeps don't crash
  - Test edge cases (energy=0, energy=1, etc.)

- [ ] Add integration test verifying firmware and viz produce identical patterns:
  - Generate pattern via `GeneratePattern()` directly
  - Compare with pattern from Sequencer (for simple cases)

- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] `PatternGenerator.h/.cpp` exists with clean API
- [ ] `Sequencer::GenerateBar()` uses `GeneratePattern()` for core logic
- [ ] `pattern_viz.cpp` uses `GeneratePattern()` (no duplication)
- [ ] Firmware and viz tool produce **identical** patterns for same inputs
- [ ] All existing tests pass
- [ ] No new compiler warnings
- [ ] Code review passes (RT-safe, no heap allocs)

## Implementation Notes

### Files to Create

- `src/Engine/PatternGenerator.h` - Public API
- `src/Engine/PatternGenerator.cpp` - Implementation
- `tests/test_pattern_generator.cpp` - Unit tests

### Files to Modify

- `src/Engine/Sequencer.cpp` - Use new module in `GenerateBar()`
- `tools/pattern_viz.cpp` - Use new module, remove duplication
- `Makefile` - Add new source files to build

### API Design

```cpp
namespace daisysp_idm_grids {

/// Input parameters for pattern generation
struct PatternParams {
    float energy = 0.50f;
    float shape = 0.30f;
    float axisX = 0.50f;
    float axisY = 0.50f;
    float drift = 0.00f;
    float accent = 0.50f;
    uint32_t seed = 0xDEADBEEF;
    int patternLength = 32;
};

/// Output from pattern generation
struct PatternResult {
    uint32_t anchorMask = 0;
    uint32_t shimmerMask = 0;
    uint32_t auxMask = 0;
    float anchorVelocity[kMaxSteps] = {0};
    float shimmerVelocity[kMaxSteps] = {0};
    float auxVelocity[kMaxSteps] = {0};
};

/// Generate a complete pattern from parameters
/// @param params Input parameters
/// @param result Output masks and velocities
void GeneratePattern(const PatternParams& params, PatternResult& result);

/// Rotate bitmask while preserving a specific step
/// @param mask Input bitmask
/// @param rotation Steps to rotate
/// @param length Pattern length
/// @param preserveStep Step to keep in place
/// @return Rotated mask with preserved step
uint32_t RotateWithPreserve(uint32_t mask, int rotation, int length, int preserveStep);

} // namespace daisysp_idm_grids
```

### Firmware Integration Strategy

The firmware has additional complexity not in the viz tool:
- Long patterns (>32 steps)
- Euclidean blending
- Fill modifiers

Strategy: `GeneratePattern()` handles core 32-step generation. `Sequencer::GenerateBar()` calls it twice for long patterns, applies Euclidean blending and fill modifiers as post-processing.

### Constraints

- **RT Audio Safety**: All allocations must be stack-based or pre-allocated
- **No std::vector**: Use fixed-size arrays only
- **No exceptions**: Use return values for error handling
- **Deterministic**: Hash-based randomness only (no rand())

### Risks

1. **Subtle algorithm differences** - Careful line-by-line comparison needed
2. **Long pattern edge cases** - Second-half generation may have firmware-specific quirks
3. **Fill modifier integration** - May need callback or hook for fill boost

## Test Plan

1. Build firmware: `make clean && make`
2. Run tests: `make test`
3. Build viz tool: `make -C tools` or rebuild via main Makefile
4. Compare outputs:
   ```bash
   # Generate pattern with viz tool
   ./build/pattern_viz --energy=0.5 --shape=0.3 --seed=0xDEADBEEF --format=mask

   # Should match firmware behavior (verify via logging or test)
   ```
