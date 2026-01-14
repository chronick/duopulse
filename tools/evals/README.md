# Pattern Evaluation Dashboard

Interactive visualization and analysis tools for DuoPulse v5 pattern generation.

## Quick Start

```bash
# From project root
make evals          # Build pattern_viz, generate data, setup dashboard
make evals-serve    # Start local server at http://localhost:3000
```

## Overview

This dashboard provides tools for evaluating pattern musicality across various metrics, enabling hill-climbing optimization of the pattern generation algorithms.

### Features

- **Preset Visualization**: Pre-configured parameter combinations with pattern grids
- **Parameter Sweeps**: See how patterns evolve across parameter ranges
- **Pentagon of Musicality**: 5 orthogonal metrics for pattern evaluation
- **Seed Variation Analysis**: Test pattern uniqueness across random seeds

## Architecture

```
tools/evals/
├── generate-patterns.js    # Node script: runs pattern_viz, outputs JSON
├── evaluate-expressiveness.js  # Node script: computes Pentagon metrics
├── package.json
├── public/
│   ├── index.html          # Static frontend
│   ├── styles.css          # Ableton-inspired dark theme
│   ├── app.js              # Client-side JavaScript
│   └── data/               # Generated JSON (git-ignored)
│       ├── metadata.json
│       ├── presets.json
│       ├── preset-metrics.json
│       ├── sweeps.json
│       ├── sweep-metrics.json
│       ├── seed-variation.json
│       └── expressiveness.json
└── README.md
```

## Make Targets

| Target | Description |
|--------|-------------|
| `make evals` | Full build: pattern_viz + generate data + setup npm |
| `make evals-generate` | Regenerate JSON data only |
| `make evals-serve` | Start local dev server on port 3000 |
| `make evals-deploy` | Copy to docs/evals/ for GitHub Pages |

## Pentagon of Musicality

Five orthogonal metrics measuring pattern quality:

1. **Syncopation** - Tension from metric displacement (LHL model)
2. **Density** - Overall activity level (hits per step)
3. **Velocity Range** - Dynamic contrast between loudest/quietest hits
4. **Voice Separation** - Independence between voices (1 - overlap)
5. **Regularity** - Temporal predictability (1 - coefficient of variation)

Each metric is scored against zone-appropriate targets based on SHAPE:
- **Stable** (SHAPE < 0.3): Regular, on-beat patterns
- **Syncopated** (0.3 <= SHAPE < 0.7): Off-beat emphasis, moderate complexity
- **Wild** (SHAPE >= 0.7): Chaotic, unpredictable patterns

## GitHub Pages Deployment

The dashboard auto-deploys via GitHub Actions when changes are pushed to:
- `tools/evals/**`
- `src/Engine/**`
- `tools/pattern_viz.cpp`

Manual deployment:
```bash
make evals-deploy   # Copy to docs/evals/
git add docs/evals
git commit -m "Update evals dashboard"
git push
```

## Development

```bash
# Install dependencies
cd tools/evals
npm install

# Generate pattern data
PATTERN_VIZ=../../build/pattern_viz node generate-patterns.js

# Compute expressiveness metrics
node evaluate-expressiveness.js

# Serve locally with hot reload
npx serve public -l 3000
```

## Data Format

### Pattern JSON

```json
{
  "params": {
    "energy": 0.5,
    "shape": 0.3,
    "axisX": 0.5,
    "axisY": 0.5,
    "drift": 0.0,
    "accent": 0.5,
    "seed": 3735928559,
    "length": 32
  },
  "steps": [
    { "step": 0, "v1": true, "v2": false, "aux": false, "v1Vel": 0.85, ... },
    ...
  ],
  "masks": { "v1": 286331153, "v2": 143165576, "aux": 35791394 },
  "hits": { "v1": 8, "v2": 6, "aux": 4 }
}
```

### Pentagon Metrics JSON

```json
{
  "raw": {
    "syncopation": 0.15,
    "density": 0.28,
    "velocityRange": 0.35,
    "voiceSeparation": 0.72,
    "regularity": 0.85
  },
  "scores": {
    "syncopation": 0.92,
    "density": 0.88,
    ...
  },
  "composite": 0.86,
  "zone": "stable"
}
```
