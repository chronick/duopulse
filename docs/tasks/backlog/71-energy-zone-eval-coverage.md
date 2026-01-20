---
id: 71
slug: energy-zone-eval-coverage
title: "Add ENERGY Zone Coverage to Evaluations"
status: backlog
created_date: 2026-01-20
updated_date: 2026-01-20
spec_refs:
  - "docs/specs/main.md#3-energy-parameter"
depends_on:
  - iteration-2026-01-20-001  # Eligibility mask wiring
---

# Task 71: Add ENERGY Zone Coverage to Evaluations

## Context

Iteration 2026-01-20-001 fixed eligibility mask wiring, but the improvement couldn't be fully measured because the evaluation sweeps don't test MINIMAL zone patterns directly.

### Current Evaluation Coverage

The sweeps in `tools/evals/generate-patterns.js`:
- **SHAPE sweep**: ENERGY=0.5 (BUILD zone), SHAPE=0.0-1.0
- **ENERGY sweep**: SHAPE=0.3, ENERGY=0.0-1.0

The Pentagon metrics are calculated by SHAPE zone (stable/syncopated/wild), but eligibility constraints operate on ENERGY zones (MINIMAL/GROOVE/BUILD/PEAK).

This creates a measurement gap:
- "Stable zone" in metrics = SHAPE < 0.3 (algorithm character)
- MINIMAL zone = ENERGY < 0.2 (eligibility constraints)

Most evaluation patterns use ENERGY=0.5 (BUILD zone), so MINIMAL zone eligibility constraints are never tested.

### Impact

- Eligibility fix for MINIMAL zone can't be verified by metrics
- Pentagon metrics show syncopation=1.0 even though MINIMAL zone is now correct
- Regression detection won't catch MINIMAL zone bugs

## Design

### Approach A: Add ENERGY Zone Sweeps

Add dedicated sweeps for each ENERGY zone:

```javascript
// generate-patterns.js additions

const energyZoneSweeps = {
  minimal: { energy: 0.10, shapes: [0.0, 0.15, 0.3, 0.5, 0.7, 1.0] },
  groove:  { energy: 0.35, shapes: [0.0, 0.15, 0.3, 0.5, 0.7, 1.0] },
  build:   { energy: 0.60, shapes: [0.0, 0.15, 0.3, 0.5, 0.7, 1.0] },
  peak:    { energy: 0.85, shapes: [0.0, 0.15, 0.3, 0.5, 0.7, 1.0] }
};
```

### Approach B: Cross-Product Grid

Generate a grid of (ENERGY, SHAPE) combinations:

```javascript
const energyValues = [0.10, 0.35, 0.60, 0.85];  // Zone centers
const shapeValues = [0.0, 0.15, 0.3, 0.5, 0.7, 1.0];

// 4 x 6 = 24 additional patterns
```

### Approach C: Zone-Aware Pentagon Calculation

Modify metric calculation to group by ENERGY zone as well as SHAPE zone:

```javascript
// New metric aggregation
const pentagonByEnergyZone = {
  minimal: { syncopation: 0.15, regularity: 0.85, ... },
  groove:  { syncopation: 0.42, regularity: 0.65, ... },
  build:   { syncopation: 0.68, regularity: 0.48, ... },
  peak:    { syncopation: 0.85, regularity: 0.32, ... }
};
```

### Recommended: Approach B + C

1. Generate cross-product grid (24 additional patterns)
2. Calculate Pentagon metrics by both SHAPE zone AND ENERGY zone
3. Add ENERGY zone metrics to baseline.json and comparison

## Subtasks

- [ ] Add cross-product pattern generation to `generate-patterns.js`
- [ ] Update `evaluate-expressiveness.js` to calculate metrics by ENERGY zone
- [ ] Add ENERGY zone metrics to `baseline.json` schema
- [ ] Update `compare-metrics.js` to compare ENERGY zone metrics
- [ ] Update dashboard to display ENERGY zone metrics
- [ ] Re-establish baseline with new metric coverage

## Acceptance Criteria

- [ ] Patterns generated at ENERGY=0.10 (MINIMAL zone)
- [ ] Pentagon metrics calculated per ENERGY zone
- [ ] Baseline includes ENERGY zone metrics
- [ ] PR comparison shows ENERGY zone deltas
- [ ] MINIMAL zone syncopation reflects eligibility constraint (should be low)

## Test Plan

1. Generate patterns with new sweeps
2. Verify MINIMAL zone (ENERGY=0.10) patterns:
   - Anchor hits only on quarter notes (0, 4, 8, ...)
   - Syncopation metric is low (< 0.3)
   - Regularity metric is high (> 0.7)
3. Verify metrics differ meaningfully across ENERGY zones

## Files to Modify

- `tools/evals/generate-patterns.js` - Add cross-product sweeps
- `tools/evals/evaluate-expressiveness.js` - Add ENERGY zone metrics
- `tools/evals/src/components/` - Update dashboard
- `metrics/baseline.json` - Add ENERGY zone metrics
- `scripts/compare-metrics.js` - Add ENERGY zone comparison

## Estimated Effort

3-4 hours

## Dashboard Mockup

```
PENTAGON BY ENERGY ZONE
========================
           MINIMAL   GROOVE    BUILD     PEAK
Sync         0.15     0.42      0.68     0.85
Dens         0.12     0.35      0.55     0.75
VelRng       0.30     0.35      0.36     0.38
VoiceSep     0.95     0.88      0.82     0.75
Reg          0.88     0.65      0.48     0.32
```

## Related

- Iteration 2026-01-20-001: Revealed the measurement gap
- Task 70: Eligibility-aware guard rails (complementary fix)
- Task 68: Preset conformance metrics (similar pattern)
