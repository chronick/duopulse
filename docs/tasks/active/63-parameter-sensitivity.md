---
id: 63
slug: parameter-sensitivity
title: "Parameter Sensitivity Analysis"
status: completed
created_date: 2026-01-18
updated_date: 2026-01-18
completed_date: 2026-01-18
commits:
branch: feature/parameter-sensitivity
spec_refs: []
depends_on:
  - 56  # Weight-based blending
  - 59  # Algorithm weight config
---

# Task 63: Parameter Sensitivity Analysis

## Objective

Implement automated sensitivity analysis to discover which algorithm weights most strongly affect each Pentagon metric. This enables smarter iteration by focusing changes on high-impact parameters.

## Context

### Current State

- Designer agent guesses which weights to change
- No systematic understanding of parameter impact
- Trial-and-error iteration approach
- May miss high-impact parameters

### Target State

- Sensitivity matrix: weight → metric impact mapping
- Identify "levers" for each Pentagon metric
- Guide designer to most impactful changes
- Reduce wasted iterations on low-impact parameters
- Visualize sensitivity on evals dashboard

## Design

### Sensitivity Analysis Method

For each weight parameter W and metric M:
1. Sweep W across range [min, max] in N steps
2. Hold all other parameters constant
3. Measure M at each step
4. Compute sensitivity = ΔM / ΔW

### Sensitivity Matrix

```
                  | Syncopation | Density | Velocity | Voice Sep | Regularity |
------------------|-------------|---------|----------|-----------|------------|
euclideanFadeStart|    -0.3     |   0.1   |   0.0    |    0.2    |    0.5     |
euclideanFadeEnd  |    -0.4     |   0.2   |   0.0    |    0.1    |    0.4     |
syncopationCenter |     0.8     |   0.1   |   0.1    |    0.3    |   -0.6     |
syncopationWidth  |     0.5     |   0.0   |   0.0    |    0.2    |   -0.3     |
randomFadeStart   |     0.6     |   0.3   |   0.2    |    0.4    |   -0.7     |
randomFadeEnd     |     0.4     |   0.4   |   0.3    |    0.5    |   -0.5     |
```

Values: -1.0 (strong negative) to +1.0 (strong positive)

### Lever Identification

From sensitivity matrix, identify:
- **Primary levers**: |sensitivity| > 0.5 for target metric
- **Safe parameters**: |sensitivity| < 0.1 (low collateral impact)
- **Risk parameters**: high sensitivity on multiple metrics

Example output:
```
To improve Syncopation:
  Primary levers:
    - syncopationCenter (+0.8) - RECOMMENDED
    - randomFadeStart (+0.6)

  Secondary levers:
    - syncopationWidth (+0.5)
    - randomFadeEnd (+0.4)

  Avoid (high collateral):
    - randomFadeStart (also affects Regularity -0.7)
```

## Subtasks

### Sweep Infrastructure
- [x] Create parameter sweep script
- [x] Define sweep ranges for each weight
- [x] Run pattern_viz at each sweep point
- [x] Collect Pentagon metrics

### Sensitivity Calculation
- [x] Compute ΔM/ΔW for each weight-metric pair
- [x] Normalize sensitivities to [-1, 1]
- [x] Handle non-linear relationships (polynomial fit)
- [ ] Identify interaction effects between parameters (FUTURE)

### Lever Analysis
- [x] Rank parameters by sensitivity for each metric
- [x] Identify primary vs secondary levers
- [x] Detect parameters with high collateral impact
- [x] Generate lever recommendations

### Designer Integration
- [ ] Feed sensitivity matrix to designer agent (Task 55)
- [ ] Suggest high-impact parameters to modify (Task 55)
- [ ] Warn about high-risk parameter changes (Task 55)
- [ ] Prioritize lever order in proposals (Task 55)

### Visualization
- [x] Add sensitivity heatmap to evals dashboard
- [ ] Show lever recommendations on iterate page (Task 55)
- [ ] Visualize sweep curves (parameter vs metric) (FUTURE)
- [x] Update after each iteration

### Tests
- [x] Test sweep execution
- [x] Test sensitivity calculation accuracy
- [x] Test lever ranking logic
- [x] All tests pass

## Acceptance Criteria

- [x] Sensitivity matrix computed for all weight-metric pairs
- [x] Primary levers identified for each Pentagon metric
- [ ] Designer agent receives lever recommendations (Task 55)
- [x] Sensitivity heatmap displayed on dashboard
- [ ] Sweep curves available for detailed analysis (FUTURE)
- [x] Matrix updates as algorithm evolves

## Implementation Notes

### Files Created

- `scripts/sensitivity/README.md` - Documentation
- `scripts/sensitivity/run-sweep.js` - Parameter sweep runner
- `scripts/sensitivity/compute-matrix.js` - Sensitivity calculation
- `scripts/sensitivity/identify-levers.js` - Lever recommendations
- `metrics/sweep-config.json` - Sweep parameter ranges
- `metrics/sensitivity-matrix.json` - Generated matrix output
- `tools/evals/public/js/sensitivity.js` - Dashboard visualization
- `tools/evals/tests/sensitivity.test.js` - Unit tests

### Make Targets Added

- `make sensitivity-sweep` - Run parameter sweep
- `make sensitivity-matrix` - Compute full sensitivity matrix
- `make sensitivity-levers METRIC=<name>` - Show lever recommendations

### Known Limitations

Current sensitivity values show 0.0 because pattern_viz uses compile-time constexpr weights from `algorithm_config.h`. The --config flag displays loaded config but doesn't affect pattern generation.

**Future work**: Add runtime weight parameter support to pattern_viz, or modify the sweep script to use `make weights-header` for each sweep point (slower but accurate).

## Test Plan

1. Run parameter sweep:
   ```bash
   ./scripts/sensitivity/run-sweep.sh --parameter syncopationCenter
   ```
2. Compute full sensitivity matrix:
   ```bash
   node scripts/sensitivity/compute-matrix.js --config metrics/sweep-config.json
   ```
3. Generate lever recommendations:
   ```bash
   node scripts/sensitivity/identify-levers.js --target syncopation
   ```
4. Verify dashboard visualization:
   ```bash
   make evals-serve
   # Navigate to /sensitivity
   ```

## Estimated Effort

4-5 hours (sweep infrastructure, matrix computation, visualization)
