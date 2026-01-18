---
id: 54
slug: fill-gates-evals
title: "Fill Gate Evaluation in Pentagon Metrics"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/fill-gates-evals
spec_refs:
  - "docs/specs/main.md#9-fill-system"
  - "docs/specs/main.md#10-aux-output-system"
---

# Task 54: Fill Gate Evaluation in Pentagon Metrics

## Objective

Add fill gate pattern generation and evaluation to the evals system. Fills should follow energy/balance parameters and be evaluated alongside anchor/shimmer patterns in the Pentagon metrics dashboard.

## Context

### Current State

- Evals generate patterns for V1 (anchor), V2 (shimmer), and AUX
- Pentagon metrics evaluate syncopation, density, velocity range, voice separation, regularity
- Fill system exists in firmware but is not evaluated
- Fill gates are triggered by button/CV but pattern content is not analyzed

### Target State

- Pattern generator outputs fill patterns for evaluation
- Fill patterns respond to ENERGY parameter (denser fills at higher energy)
- Pentagon metrics include fill quality assessment
- Website displays fill patterns alongside drum loops
- Fill timing and build characteristics are measurable

### Fill System Requirements (from spec)

From `docs/specs/main.md` section 9:
- Fills use exponential density curve
- `maxBoost = 0.6 + energy * 0.4`
- `densityMultiplier = 1.0 + maxBoost * (fillProgress^2)`
- Velocity boost: `0.10 + 0.15 * fillProgress`
- Accent probability ramp: `0.50 + 0.50 * fillProgress`
- Force accents when `fillProgress > 0.85`

## Subtasks

### Pattern Generator Updates
- [ ] Add `GenerateFillPattern()` function to pattern generator
- [ ] Add fill parameters to `PatternParams` struct (fillDuration, fillEnergy)
- [ ] Include fill mask in `GeneratedPattern` output
- [ ] Generate fills at multiple progress points (0.25, 0.5, 0.75, 1.0)

### Evaluation Metrics
- [ ] Add fill-specific metrics to Pentagon:
  - `fillDensityRamp` - Does density increase through fill?
  - `fillVelocityBuild` - Does velocity escalate toward end?
  - `fillAccentPlacement` - Are accents on strong beats near end?
- [ ] Define target ranges for fill metrics by zone
- [ ] Add fill composite score to overall alignment

### Data Generation
- [ ] Update `generate-patterns.js` to output fill patterns
- [ ] Add fill sweep: energy 0.0 to 1.0 with fill patterns
- [ ] Output fill timing data (which steps have fill hits)

### Website Integration
- [ ] Add fill pattern grid to preset visualization
- [ ] Show fill metrics in Pentagon display
- [ ] Add fill density curve visualization
- [ ] Display fill vs. main pattern overlap

### Tests
- [ ] Add unit tests for fill pattern generation
- [ ] Test fill density scaling with ENERGY
- [ ] Test fill velocity ramp
- [ ] All tests pass

## Acceptance Criteria

- [ ] Fill patterns generated for each preset/sweep
- [ ] Fill density increases with ENERGY parameter
- [ ] Fill velocity ramps toward end of fill
- [ ] Fill metrics appear in Pentagon dashboard
- [ ] Fill patterns displayed on website alongside drum loops
- [ ] Fill composite score contributes to overall alignment
- [ ] All existing tests pass
- [ ] No new compiler warnings

## Implementation Notes

### Files to Modify

Pattern Generator:
- `src/Engine/PatternGenerator.h` - Add fill generation interface
- `src/Engine/PatternGenerator.cpp` - Implement `GenerateFillPattern()`
- `tools/pattern_viz.cpp` - Add --fill flag for fill output

Evals:
- `tools/evals/generate-patterns.js` - Generate fill data
- `tools/evals/evaluate-expressiveness.js` - Add fill metrics
- `tools/evals/public/data/*.json` - Fill pattern data

Website:
- `tools/evals/public/app.js` - Render fill patterns
- `tools/evals/public/styles.css` - Fill visualization styles

### Fill Metrics Definition

```javascript
const FILL_METRICS = {
  fillDensityRamp: {
    name: 'Fill Density Ramp',
    description: 'Fills should build from sparse to dense',
    targetByZone: {
      stable: '0.60-0.85',      // Modest build
      syncopated: '0.70-0.95',  // Strong build
      wild: '0.80-1.00',        // Intense build
    },
  },
  fillVelocityBuild: {
    name: 'Fill Velocity Build',
    description: 'Velocity should crescendo through fill',
    targetByZone: {
      stable: '0.50-0.75',
      syncopated: '0.60-0.85',
      wild: '0.70-0.95',
    },
  },
  fillAccentPlacement: {
    name: 'Fill Accent Placement',
    description: 'Accents should land on strong beats, especially near end',
    targetByZone: {
      stable: '0.70-0.95',
      syncopated: '0.55-0.80',
      wild: '0.40-0.70',
    },
  },
};
```

### Fill Pattern Data Format

```json
{
  "params": { "energy": 0.5, "fillDuration": 16, ... },
  "fillSteps": [
    { "step": 0, "progress": 0.0, "velocity": 0.65, "accent": false },
    { "step": 4, "progress": 0.25, "velocity": 0.70, "accent": false },
    { "step": 8, "progress": 0.50, "velocity": 0.78, "accent": true },
    ...
  ],
  "fillMetrics": {
    "densityRamp": 0.82,
    "velocityBuild": 0.75,
    "accentPlacement": 0.88,
  }
}
```

## Test Plan

1. Build pattern_viz with fill support: `make pattern-viz`
2. Generate fill patterns:
   ```bash
   ./build/pattern_viz --energy=0.5 --fill --format=json
   ```
3. Run evals: `make evals-generate`
4. Check fill metrics in output:
   ```bash
   cat tools/evals/public/data/expressiveness.json | jq '.fillMetrics'
   ```
5. View fill patterns on website: `make evals-serve`

## Estimated Effort

3-4 hours
