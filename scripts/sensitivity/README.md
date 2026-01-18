# Parameter Sensitivity Analysis

Scripts for computing sensitivity matrix and identifying high-impact "levers" for each Pentagon metric.

## Overview

The sensitivity analysis measures how changes in algorithm weight parameters affect Pentagon metrics:
- **Syncopation** - Groove and forward motion
- **Density** - Overall pattern fullness
- **Velocity Range** - Dynamic variation
- **Voice Separation** - Clarity vs overlap
- **Regularity** - Predictability/danceability

## Usage

```bash
# Full sensitivity sweep (creates metrics/sensitivity-matrix.json)
make sensitivity-matrix

# Quick sweep for single parameter
node scripts/sensitivity/run-sweep.js --parameter syncopationCenter

# View lever recommendations for a metric
node scripts/sensitivity/identify-levers.js --target syncopation
```

## Output Files

- `metrics/sweep-config.json` - Sweep parameter ranges
- `metrics/sensitivity-matrix.json` - Full sensitivity matrix with lever recommendations
- `tools/evals/public/data/sensitivity.json` - Dashboard visualization data

## Algorithm

For each weight parameter W and metric M:
1. Sweep W across [min, max] in N steps
2. Hold all other parameters at baseline
3. Measure Pentagon metric M at each step
4. Compute sensitivity = normalized(ΔM / ΔW)

Sensitivity values range from -1.0 (strong negative correlation) to +1.0 (strong positive correlation).
