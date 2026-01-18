---
id: 56
slug: weight-based-blending
title: "Weight-Based Algorithm Blending System"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/weight-based-blending
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
  - "docs/specs/main.md#6-axis-biasing"
---

# Task 56: Weight-Based Algorithm Blending System

## Objective

Make the pattern generation algorithm easier to iterate by introducing explicit weight-based blending of generation methods. This enables fine-grained control over algorithm contributions based on input parameters and supports per-channel, per-section variation.

## Context

### Current State

- SHAPE parameter controls 3-zone blending (stable/syncopated/wild)
- Euclidean and weighted random are hard-blended based on SHAPE
- No per-channel parameter variation
- No per-section (verse/chorus) differentiation
- Algorithm weights are embedded in code, hard to adjust

### Target State

- Algorithm contributions controlled by explicit weight arrays
- Euclidean strength varies by SHAPE (low SHAPE = more euclidean)
- Per-channel euclidean parameters (different k values for anchor/shimmer)
- Per-section euclidean variation (verse vs chorus feel)
- Euclidean "building blocks" assembled based on ENERGY/SHAPE/AXIS X+Y
- Weights easily adjustable via configuration

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

### Per-Section Variation

Sections defined by phrase position:
- Intro (bars 1-2): Sparse euclidean, minimal syncopation
- Verse (bars 3-6): Standard weights
- Chorus (bars 7-8): Boosted density, more syncopation

## Subtasks

### Weight Calculator
- [ ] Create `AlgorithmWeights` struct with euclidean/syncopation/random weights
- [ ] Implement `ComputeAlgorithmWeights()` function
- [ ] Add smoothstep and bell_curve utility functions
- [ ] Validate weights sum to 1.0

### Euclidean Building Blocks
- [ ] Parameterize euclidean generator with (n, k, rotation)
- [ ] Add per-channel k calculation based on ENERGY
- [ ] Implement euclidean pattern cache for efficiency
- [ ] Support AXIS X rotation of euclidean patterns

### Syncopation Overlay
- [ ] Create syncopation weight array based on AXIS X/Y
- [ ] Blend syncopation weights with euclidean base
- [ ] Add SHAPE-based syncopation intensity

### Random Perturbation
- [ ] Implement noise layer with SHAPE-based intensity
- [ ] Add seed-based deterministic randomness
- [ ] Apply random perturbation to final weights

### Weighted Blend
- [ ] Combine all algorithm outputs with computed weights
- [ ] Ensure smooth transitions across parameter space
- [ ] Validate output falls within expected ranges

### Configuration System
- [ ] Add algorithm weight configuration (JSON or header)
- [ ] Support runtime weight adjustment for testing
- [ ] Add weight visualization to evals dashboard

### Tests
- [ ] Test weight normalization
- [ ] Test euclidean parameter scaling
- [ ] Test per-section variation
- [ ] Test blend smoothness across SHAPE range
- [ ] All tests pass

## Acceptance Criteria

- [ ] Algorithm weights explicitly computed from parameters
- [ ] Low SHAPE produces euclidean-dominant patterns
- [ ] High SHAPE produces random-dominant patterns
- [ ] Per-channel euclidean parameters scale with ENERGY
- [ ] Per-section variation produces musical phrasing
- [ ] Weights are configurable without code changes
- [ ] Smooth transitions across entire parameter space
- [ ] No performance regression in pattern generation
- [ ] All existing tests pass

## Implementation Notes

### Files to Create

- `src/Engine/AlgorithmWeights.h` - Weight calculation
- `src/Engine/AlgorithmWeights.cpp` - Weight implementation
- `src/Engine/EuclideanBlocks.h` - Per-channel euclidean
- `src/Engine/EuclideanBlocks.cpp` - Euclidean implementation

### Files to Modify

- `src/Engine/PatternGenerator.cpp` - Use new weight system
- `src/Engine/PatternField.cpp` - Per-section awareness
- `inc/config.h` - Algorithm weight defaults
- `tools/evals/evaluate-expressiveness.js` - Weight analysis

### Weight Configuration Format

```cpp
// inc/config.h
namespace AlgorithmConfig {
    // Euclidean influence curve
    constexpr float kEuclideanFadeStart = 0.3f;
    constexpr float kEuclideanFadeEnd = 0.7f;

    // Syncopation bell curve
    constexpr float kSyncopationCenter = 0.5f;
    constexpr float kSyncopationWidth = 0.3f;

    // Random influence curve
    constexpr float kRandomFadeStart = 0.5f;
    constexpr float kRandomFadeEnd = 0.9f;

    // Per-channel euclidean k ranges
    constexpr int kAnchorKMin = 4;
    constexpr int kAnchorKMax = 12;
    constexpr int kShimmerKMin = 6;
    constexpr int kShimmerKMax = 16;
    constexpr int kAuxKMin = 2;
    constexpr int kAuxKMax = 8;
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

5-6 hours (refactoring core algorithm, adding new modules)
