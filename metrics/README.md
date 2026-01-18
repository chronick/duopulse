# Pentagon Metrics Tracking

This directory contains baseline metrics for the DuoPulse pattern generation algorithm.
These metrics are used by CI to detect regressions in pattern quality.

## Files

- `baseline.json` - Current main branch Pentagon metrics (source of truth)
- `.gitkeep` - Keeps directory tracked even when empty

## Usage

### Generate New Baseline

```bash
# Run evals and save as new baseline
make baseline
```

This will:
1. Build pattern_viz
2. Generate patterns across parameter space
3. Run expressiveness evaluation
4. Save results with commit hash and timestamp to `metrics/baseline.json`

### View Current Baseline

```bash
cat metrics/baseline.json | jq .
```

### Compare to Baseline

PR metrics comparison (Task 61b) will:
1. Generate metrics from PR branch
2. Compare each metric to baseline
3. Fail CI if any metric regresses > 2%

## Baseline Format

```json
{
  "version": "1.0.0",
  "generated_at": "2026-01-18T12:00:00Z",
  "commit": "abc123...",
  "metrics": {
    "overall": {
      "syncopation": 0.42,
      "density": 0.58,
      "voiceSeparation": 0.71,
      "velocityRange": 0.45,
      "regularity": 0.63
    },
    "byZone": {
      "stable": { ... },
      "syncopated": { ... },
      "wild": { ... }
    }
  }
}
```

## Updating the Baseline

The baseline should be updated when:
1. Intentional algorithm improvements are merged
2. Major releases are cut
3. Metric definitions change

To update:
```bash
git checkout main
make baseline
git add metrics/baseline.json
git commit -m "docs(metrics): update baseline after algorithm improvement"
```

## Regression Thresholds

Default threshold is 2% regression per metric. This can be adjusted in:
- `.github/workflows/pr-metrics.yml` (CI)
- `scripts/compare-metrics.js` (local)
