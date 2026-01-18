---
id: 56
slug: weight-based-blending
title: "Weight-Based Algorithm Blending System"
status: completed
created_date: 2026-01-18
updated_date: 2026-01-18
completed_date: 2026-01-18
branch: feature/weight-based-blending
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
  - "docs/specs/main.md#6-axis-biasing"
---

# Task 56: Weight-Based Algorithm Blending System

## Objective

Make the pattern generation algorithm easier to iterate by introducing explicit weight-based blending of generation methods. This enables fine-grained control over algorithm contributions based on input parameters.

## Context

### Current State

- SHAPE parameter controls 3-zone blending (stable/syncopated/wild)
- Euclidean and weighted random are hard-blended based on SHAPE
- No per-channel parameter variation
- Algorithm weights are embedded in code, hard to adjust

### Target State

- Algorithm contributions controlled by explicit weight arrays
- Euclidean strength varies by SHAPE (low SHAPE = more euclidean)
- Per-channel euclidean parameters (different k values for anchor/shimmer)
- Weights easily adjustable via configuration
- **Bootstrap lever table** for initial `/iterate` usage

### Scope Reduction (2026-01-18)

> **Note**: Per-section variation (intro/verse/chorus weight profiles) was removed from
> this task based on design review. It requires API changes to `PatternParams` and is
> now tracked as **Task 65: Phrase-Aware Weight Modulation** (backlog).

## Design

### Blending Architecture

```
Input Parameters
     |
     v
+--------------------+
|  Weight Calculator |  <-- Computes algorithm weights from params
+--------------------+
     |
     v (weights for each algorithm)
+--------------------+     +---------------------+     +------------------+
| Euclidean Generator| --> | Syncopation Overlay | --> | Random Perturbation|
+--------------------+     +---------------------+     +------------------+
     |                           |                            |
     v                           v                            v
   weight_e                   weight_s                      weight_r
     |                           |                            |
     +---------------------------+----------------------------+
                                 |
                                 v
                    +------------------------+
                    | Weighted Blend Output  |
                    +------------------------+
```

### Weight Formulas

```cpp
// Euclidean strength: high at low SHAPE, fades at high SHAPE
float euclideanWeight = 1.0f - smoothstep(0.3f, 0.7f, shape);

// Syncopation strength: peaks in middle zone
float syncopationWeight = bell_curve(shape, center=0.5f, width=0.3f);

// Random strength: grows at high SHAPE
float randomWeight = smoothstep(0.5f, 0.9f, shape);

// Normalize weights to sum to 1.0
float total = euclideanWeight + syncopationWeight + randomWeight;
euclideanWeight /= total;
syncopationWeight /= total;
randomWeight /= total;
```

### Per-Channel Euclidean Parameters

| Voice | k Range | Description |
|-------|---------|-------------|
| Anchor | 4-12 | Lower k = more sparse, foundational |
| Shimmer | 6-16 | Higher k = more active, syncopated |
| AUX | 2-8 | Variable, often sparse |

k selection based on ENERGY:
```cpp
int anchorK = 4 + int(energy * 8);    // 4-12
int shimmerK = 6 + int(energy * 10);  // 6-16
int auxK = 2 + int(energy * 6);       // 2-8
```

### Bootstrap Lever Table

Until Task 63 (Sensitivity Analysis) runs, the `/iterate` command needs heuristic
guidance for which weights affect which metrics. This table provides initial mapping:

```cpp
// inc/algorithm_config.h - Bootstrap lever table for /iterate command
//
// Format: {metric, primary_lever, direction, secondary_lever}
// Direction: "+" = increase lever to improve metric, "-" = decrease
//
// These are manual estimates. Task 63 will generate a data-driven version.
//
namespace BootstrapLevers {
    // Syncopation improvement
    // - Primary: kSyncopationCenter (+) - shifts bell curve peak
    // - Secondary: kRandomFadeStart (-) - earlier random = more chaotic

    // Density/regularity improvement
    // - Primary: kEuclideanFadeEnd (+) - euclidean persists longer
    // - Secondary: kSyncopationWidth (-) - narrower = more predictable

    // Voice separation improvement
    // - Primary: shimmer drift (+) - more offset from anchor
    // - Secondary: kAnchorKMax (-) - sparser anchor = more gaps

    // Velocity variation improvement
    // - Primary: accent parameter (+) - direct control
    // - Secondary: kSyncopationCenter (+) - more syncopated = more accents

    // Wild zone responsiveness
    // - Primary: kRandomFadeStart (-) - random kicks in earlier
    // - Secondary: kEuclideanFadeStart (-) - euclidean fades earlier
}
```

This will be consumed by `/iterate` to make initial proposals before sensitivity
analysis provides data-driven recommendations.

## Subtasks

### Weight Calculator
- [x] Create `AlgorithmWeights` struct with euclidean/syncopation/random weights
- [x] Implement `ComputeAlgorithmWeights()` function
- [x] Add smoothstep and bell_curve utility functions
- [x] Validate weights sum to 1.0

### Euclidean Building Blocks
- [x] Parameterize euclidean generator with (n, k, rotation)
- [x] Add per-channel k calculation based on ENERGY
- [ ] Implement euclidean pattern cache for efficiency (deferred - existing system adequate)
- [x] Support AXIS X rotation of euclidean patterns

### Syncopation Overlay
- [x] Create syncopation weight array based on AXIS X/Y (via existing PatternField)
- [x] Blend syncopation weights with euclidean base (via weight normalization)
- [x] Add SHAPE-based syncopation intensity (bell curve)

### Random Perturbation
- [x] Implement noise layer with SHAPE-based intensity (via random weight)
- [x] Add seed-based deterministic randomness (via HashToFloat)
- [x] Apply random perturbation to final weights

### Weighted Blend
- [x] Combine all algorithm outputs with computed weights
- [x] Ensure smooth transitions across parameter space
- [x] Validate output falls within expected ranges

### Configuration System
- [x] Add algorithm weight configuration in `inc/algorithm_config.h`
- [x] Support compile-time weight adjustment
- [x] Add `--debug-weights` flag to pattern_viz

### Bootstrap Lever Table
- [x] Document lever→metric mappings in algorithm_config.h
- [x] Include direction hints (increase/decrease)
- [x] Note which levers interact or conflict

### Tests
- [x] Test weight normalization
- [x] Test euclidean parameter scaling
- [x] Test blend smoothness across SHAPE range
- [x] All tests pass (373 tests)

## Acceptance Criteria

- [x] Algorithm weights explicitly computed from parameters
- [x] Low SHAPE produces euclidean-dominant patterns
- [x] High SHAPE produces random-dominant patterns
- [x] Per-channel euclidean parameters scale with ENERGY
- [x] Weights are configurable without code changes
- [x] Smooth transitions across entire parameter space
- [x] Bootstrap lever table documented in algorithm_config.h
- [x] `pattern_viz --debug-weights` shows algorithm blend percentages
- [x] No performance regression in pattern generation
- [x] All existing tests pass

## Implementation Notes

### Files to Create

- `src/Engine/AlgorithmWeights.h` - Weight calculation
- `src/Engine/AlgorithmWeights.cpp` - Weight implementation
- `src/Engine/EuclideanBlocks.h` - Per-channel euclidean
- `src/Engine/EuclideanBlocks.cpp` - Euclidean implementation

### Files to Modify

- `src/Engine/PatternGenerator.cpp` - Use new weight system
- `inc/config.h` - Algorithm weight defaults
- `tools/pattern_viz/main.cpp` - Add `--debug-weights` flag
- `tools/evals/evaluate-expressiveness.js` - Weight analysis

### Weight Configuration Format

```cpp
// inc/algorithm_config.h
namespace AlgorithmConfig {
    // =======================================================================
    // EUCLIDEAN INFLUENCE CURVE
    // Controls how strongly euclidean patterns dominate at low SHAPE values
    // =======================================================================
    constexpr float kEuclideanFadeStart = 0.3f;  // Euclidean starts fading
    constexpr float kEuclideanFadeEnd = 0.7f;    // Euclidean fully faded

    // =======================================================================
    // SYNCOPATION BELL CURVE
    // Controls the middle "syncopated zone" peak
    // =======================================================================
    constexpr float kSyncopationCenter = 0.5f;   // Peak of syncopation
    constexpr float kSyncopationWidth = 0.3f;    // Width of bell curve

    // =======================================================================
    // RANDOM INFLUENCE CURVE
    // Controls how quickly randomness takes over at high SHAPE
    // =======================================================================
    constexpr float kRandomFadeStart = 0.5f;     // Random starts appearing
    constexpr float kRandomFadeEnd = 0.9f;       // Random fully dominant

    // =======================================================================
    // PER-CHANNEL EUCLIDEAN K RANGES
    // k = number of hits in euclidean(n, k) pattern
    // =======================================================================
    constexpr int kAnchorKMin = 4;
    constexpr int kAnchorKMax = 12;
    constexpr int kShimmerKMin = 6;
    constexpr int kShimmerKMax = 16;
    constexpr int kAuxKMin = 2;
    constexpr int kAuxKMax = 8;

    // =======================================================================
    // BOOTSTRAP LEVER TABLE
    // Manual heuristics for /iterate command until Task 63 runs
    // Format: metric → {primary_lever, direction, confidence}
    // =======================================================================
    // See BootstrapLevers namespace documentation above
}
```

### Evals Integration

Add weight breakdown to pattern output:
```json
{
  "params": { "shape": 0.4, ... },
  "algorithmWeights": {
    "euclidean": 0.45,
    "syncopation": 0.35,
    "random": 0.20
  },
  "channelParams": {
    "anchor": { "k": 8, "rotation": 2 },
    "shimmer": { "k": 12, "rotation": 5 },
    "aux": { "k": 4, "rotation": 0 }
  }
}
```

## Test Plan

1. Build firmware: `make clean && make`
2. Run tests: `make test`
3. Verify weight distribution:
   ```bash
   ./build/pattern_viz --shape=0.0 --debug-weights
   ./build/pattern_viz --shape=0.5 --debug-weights
   ./build/pattern_viz --shape=1.0 --debug-weights
   ```
4. Check euclidean scaling:
   ```bash
   ./build/pattern_viz --energy=0.0 --debug-euclidean
   ./build/pattern_viz --energy=1.0 --debug-euclidean
   ```
5. Run evals and compare: `make evals`

## Estimated Effort

4-5 hours (reduced from 5-6h after removing per-section variation)
