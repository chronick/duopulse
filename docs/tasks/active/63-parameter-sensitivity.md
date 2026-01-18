---
id: 63
slug: parameter-sensitivity
title: "Parameter Sensitivity Analysis"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
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
- [ ] Create parameter sweep script
- [ ] Define sweep ranges for each weight
- [ ] Run pattern_viz at each sweep point
- [ ] Collect Pentagon metrics

### Sensitivity Calculation
- [ ] Compute ΔM/ΔW for each weight-metric pair
- [ ] Normalize sensitivities to [-1, 1]
- [ ] Handle non-linear relationships (polynomial fit)
- [ ] Identify interaction effects between parameters

### Lever Analysis
- [ ] Rank parameters by sensitivity for each metric
- [ ] Identify primary vs secondary levers
- [ ] Detect parameters with high collateral impact
- [ ] Generate lever recommendations

### Designer Integration
- [ ] Feed sensitivity matrix to designer agent
- [ ] Suggest high-impact parameters to modify
- [ ] Warn about high-risk parameter changes
- [ ] Prioritize lever order in proposals

### Visualization
- [ ] Add sensitivity heatmap to evals dashboard
- [ ] Show lever recommendations on iterate page
- [ ] Visualize sweep curves (parameter vs metric)
- [ ] Update after each iteration

### Tests
- [ ] Test sweep execution
- [ ] Test sensitivity calculation accuracy
- [ ] Test lever ranking logic
- [ ] All tests pass

## Acceptance Criteria

- [ ] Sensitivity matrix computed for all weight-metric pairs
- [ ] Primary levers identified for each Pentagon metric
- [ ] Designer agent receives lever recommendations
- [ ] Sensitivity heatmap displayed on dashboard
- [ ] Sweep curves available for detailed analysis
- [ ] Matrix updates as algorithm evolves
- [ ] Iteration speed improves (measured by # iterations to target)

## Implementation Notes

### Files to Create

- `scripts/sensitivity/run-sweep.sh` - Execute parameter sweeps
- `scripts/sensitivity/compute-matrix.js` - Calculate sensitivities
- `scripts/sensitivity/identify-levers.js` - Rank and recommend
- `tools/evals/public/sensitivity.js` - Dashboard visualization
- `metrics/sensitivity-matrix.json` - Cached matrix

### Sweep Configuration

```json
// metrics/sweep-config.json
{
  "parameters": {
    "euclideanFadeStart": { "min": 0.1, "max": 0.5, "steps": 10 },
    "euclideanFadeEnd": { "min": 0.5, "max": 0.9, "steps": 10 },
    "syncopationCenter": { "min": 0.3, "max": 0.7, "steps": 10 },
    "syncopationWidth": { "min": 0.1, "max": 0.5, "steps": 10 },
    "randomFadeStart": { "min": 0.3, "max": 0.7, "steps": 10 },
    "randomFadeEnd": { "min": 0.7, "max": 1.0, "steps": 10 }
  },
  "baseline_params": {
    "energy": 0.5,
    "shape": 0.5,
    "axisX": 0.5,
    "axisY": 0.5
  }
}
```

### Sensitivity Output Format

```json
// metrics/sensitivity-matrix.json
{
  "generated_at": "2026-01-18T12:00:00Z",
  "baseline_commit": "abc123",
  "matrix": {
    "euclideanFadeStart": {
      "syncopation": -0.32,
      "density": 0.12,
      "velocityRange": 0.05,
      "voiceSeparation": 0.18,
      "regularity": 0.48
    },
    ...
  },
  "levers": {
    "syncopation": {
      "primary": ["syncopationCenter", "randomFadeStart"],
      "secondary": ["syncopationWidth", "randomFadeEnd"],
      "avoid": []
    },
    ...
  }
}
```

### Dashboard Visualization

Heatmap showing:
- Rows: weight parameters
- Columns: Pentagon metrics
- Color: blue (negative) → white (neutral) → red (positive)
- Intensity: sensitivity magnitude

Click on cell to see sweep curve for that parameter-metric pair.

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
