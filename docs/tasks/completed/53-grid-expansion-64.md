---
id: 53
slug: grid-expansion-64
title: "Grid Expansion to 64 Steps with Flam Resolution"
status: completed
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/grid-expansion-64
spec_refs:
  - "docs/specs/main.md#2-architecture-overview"
  - "docs/specs/main.md#5-shape-algorithm"
---

# Task 53: Grid Expansion to 64 Steps with Flam Resolution

## Objective

Expand the pattern grid from 32 steps to 64 base steps, supporting 1/4 step subdivisions for flam placement. This enables higher-resolution patterns and more expressive micro-timing.

## Context

### Current State

- Grid uses `uint32_t` bitmasks for pattern storage
- `kMaxSteps = 32` limits pattern length
- No sub-step resolution for flams or micro-timing

### Target State

- Grid uses `uint64_t` bitmasks for pattern storage
- `kMaxSteps = 64` allows full 4-bar patterns at 16th notes (or 2-bar at 32nd notes)
- Support for 16/32/64 pattern lengths via configuration
- 1/4 step resolution for flam timing (256 positions across 64 steps)

### Benefits

- Longer patterns without looping
- Sub-step flam positions for realistic drum rolls
- Compatibility with higher BPM music (64 = 2 bars at 32nd notes)
- Foundation for micro-displacement (Task 48)

## Subtasks

### Core Type Changes
- [ ] Change `uint32_t` mask types to `uint64_t` in `PatternGenerator.h`
- [ ] Update `kMaxSteps` constant from 32 to 64 in `DuoPulseTypes.h`
- [ ] Add `kMaxSubSteps = 256` constant for 1/4 step resolution
- [ ] Update `kMaxPhraseSteps` to 512 (8 bars x 64 steps)

### Mask Operations
- [ ] Update `RotateWithPreserve()` for 64-bit masks
- [ ] Update `ShiftMaskLeft()` and `ShiftMaskRight()` for 64-bit
- [ ] Update `FindLargestGap()` and `FindGapStart()` for 64-bit
- [ ] Update `CountBits()` to use `__builtin_popcountll()` for 64-bit

### Pattern Generation
- [ ] Update `PatternField.cpp` weight arrays to support 64 steps
- [ ] Update `METRIC_WEIGHTS` arrays for 64-step patterns
- [ ] Update `EuclideanGen.cpp` for 64-step patterns
- [ ] Update `GumbelSampler.cpp` score arrays for 64 steps

### Sequencer Integration
- [ ] Update `SequencerState.h` arrays from 32 to 64 elements
- [ ] Update `Sequencer.cpp` step iteration loops
- [ ] Add pattern length configuration (16/32/64)
- [ ] Update swing and jitter offset arrays

### Flam Resolution
- [ ] Add `FlamPosition` struct with step + substep (0-3)
- [ ] Add flam position array to `GeneratedPattern`
- [ ] Update velocity output to support flam timing offsets

### Evals Integration
- [ ] Update `METRIC_WEIGHTS_32` to `METRIC_WEIGHTS_64` in evals
- [ ] Add pattern length awareness to Pentagon metrics
- [ ] Update pattern JSON format for 64-step patterns

### Tests
- [ ] Update all existing tests for 64-step patterns
- [ ] Add tests for 16/32/64 pattern length switching
- [ ] Add tests for flam positioning
- [ ] All tests pass

## Acceptance Criteria

- [ ] `kMaxSteps = 64` compiles without warnings
- [ ] All mask operations work correctly with 64-bit values
- [ ] Pattern lengths of 16, 32, and 64 are selectable
- [ ] Flam positions can be specified at 1/4 step resolution
- [ ] Existing 32-step patterns continue to work
- [ ] Firmware fits within flash memory limits
- [ ] No performance regression in audio callback
- [ ] All existing tests pass with 64-step support
- [ ] Evals dashboard displays 64-step patterns correctly

## Implementation Notes

### Files to Modify

Core Types:
- `src/Engine/DuoPulseTypes.h` - Constants
- `src/Engine/PatternGenerator.h` - Mask types, array sizes

Mask Operations:
- `src/Engine/VoiceRelation.cpp` - Shift functions
- `src/Engine/VoiceRelation.h` - Function signatures
- `src/Engine/HitBudget.cpp` - CountBits
- `src/Engine/GuardRails.cpp` - Gap finding

Pattern Generation:
- `src/Engine/PatternField.cpp` - Weight arrays
- `src/Engine/EuclideanGen.cpp` - Pattern generation
- `src/Engine/GumbelSampler.cpp` - Score arrays

Sequencer:
- `src/Engine/SequencerState.h` - Per-step arrays
- `src/Engine/Sequencer.cpp` - Step iteration

Evals:
- `tools/evals/evaluate-expressiveness.js` - Metric weights
- `tools/evals/generate-patterns.js` - Pattern output

### Memory Considerations

Current (32-step):
- Pattern mask: 4 bytes x 3 voices = 12 bytes
- Velocity arrays: 32 x 4 bytes x 3 = 384 bytes
- Swing/jitter: 32 x 2 bytes x 2 = 128 bytes

After (64-step):
- Pattern mask: 8 bytes x 3 voices = 24 bytes
- Velocity arrays: 64 x 4 bytes x 3 = 768 bytes
- Swing/jitter: 64 x 2 bytes x 2 = 256 bytes

Total increase: ~500 bytes per pattern (acceptable for STM32H7)

### Backward Compatibility

- Default pattern length remains 32 for existing patches
- 16-step mode useful for simpler patterns or double-time feel
- 64-step mode enables complex arrangements

## Test Plan

1. Build firmware: `make clean && make`
2. Run tests: `make test`
3. Generate 64-step patterns:
   ```bash
   ./build/pattern_viz --length=64 --seed=123 --format=grid
   ./build/pattern_viz --length=32 --seed=123 --format=grid  # Compare
   ```
4. Verify mask operations:
   ```bash
   make test TEST_FILTER="*64*"
   ```
5. Check memory usage:
   ```bash
   make size  # Verify flash/RAM within limits
   ```

## Estimated Effort

4-6 hours (significant refactor touching many files)
