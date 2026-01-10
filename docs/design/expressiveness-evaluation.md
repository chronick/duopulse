# Expressiveness Evaluation System

**Purpose**: Continuously evaluate and iterate on pattern expressiveness by sweeping parameters and measuring variation.

## Overview

The expressiveness evaluation system provides a feedback loop for pattern generation development. After each algorithmic change, run the evaluation to verify:

1. **Improvement**: Variation increased where expected
2. **No Regression**: Existing expressiveness not degraded
3. **Coverage**: All parameter regions behave appropriately

## Quick Start

```bash
# Build the pattern_viz tool (if not already built)
make pattern-viz

# Run full evaluation
python scripts/evaluate-expressiveness.py

# Run quick evaluation (fewer combinations)
python scripts/evaluate-expressiveness.py --quick

# Generate JSON data for further analysis
python scripts/evaluate-expressiveness.py --json evaluation-data.json
```

## What It Measures

### 1. Seed Variation (Critical)

The most important metric: **do different seeds produce different patterns?**

For each combination of (energy, shape, drift), the system generates patterns with 8 different seeds and counts how many unique V1, V2, and AUX masks result.

**Pass Criteria**: V2 (shimmer) variation score >= 50%

A variation score of 50% means at least half the seeds produce unique patterns. Below this threshold, the reseed gesture feels "broken" to users.

### 2. Parameter Sweeps

Evaluates how patterns change across parameter ranges:

- **SHAPE sweep**: Stable -> Syncopated -> Wild
- **ENERGY sweep**: Sparse -> Dense
- **DRIFT sweep**: Even-spaced -> Weighted -> Random

### 3. Problem Zone Detection

Identifies parameter regions where:
- Multiple seeds converge to the same pattern
- V2 (shimmer) doesn't vary with seed
- Patterns are unexpectedly similar

## Output Files

| File | Purpose |
|------|---------|
| `docs/design/expressiveness-report.md` | Human-readable analysis |
| `evaluation-data.json` (optional) | Machine-readable data for plotting |

## Interpreting Results

### Variation Scores

| Score | Interpretation |
|-------|----------------|
| 100% | Perfect: every seed produces unique pattern |
| 75-99% | Good: high variation |
| 50-74% | Acceptable: adequate variation |
| 25-49% | Warning: low variation |
| 0-24% | Critical: effectively no variation |

### Example Output

```
============================================================
EXPRESSIVENESS EVALUATION SUMMARY
============================================================

Overall Seed Variation: 45%
  V1 (Anchor):  62%
  V2 (Shimmer): 28%
  AUX:          45%

[FAIL] Expressiveness issues detected:
       - V2 (Shimmer) variation is too low

LOW VARIATION ZONES:
  [CRITICAL] E=0.50 S=0.30 D=0.00
             V2 score: 12%
  [WARNING]  E=0.60 S=0.50 D=0.00
             V2 score: 37%
```

## Integration with Development

### After Each Algorithm Change

1. Run the evaluation: `python scripts/evaluate-expressiveness.py`
2. Compare scores to baseline (saved in git)
3. If V2 score dropped, investigate the change
4. If V2 score improved, document the improvement

### Baseline Tracking

Save evaluation JSON after reaching a known-good state:

```bash
python scripts/evaluate-expressiveness.py --json baseline.json
git add baseline.json
git commit -m "chore: update expressiveness baseline"
```

### CI Integration (Future)

The script returns exit code 1 if V2 expressiveness < 50%, making it suitable for CI:

```yaml
- name: Check expressiveness
  run: python scripts/evaluate-expressiveness.py --quick
```

## Metrics Reference

### Pattern Metrics

| Metric | Description |
|--------|-------------|
| `v1_hits` | Number of V1 (anchor) hits |
| `v2_hits` | Number of V2 (shimmer) hits |
| `aux_hits` | Number of AUX hits |
| `regularity_score` | 0=irregular, 1=perfectly regular (based on gap variance) |
| `syncopation_ratio` | Ratio of offbeat hits to total hits |
| `v1_velocity_range` | Difference between max and min velocity |

### Seed Variation Metrics

| Metric | Description |
|--------|-------------|
| `unique_v1_masks` | Count of distinct V1 bitmasks across seeds |
| `unique_v2_masks` | Count of distinct V2 bitmasks across seeds |
| `v1_variation_score` | (unique - 1) / (total - 1), normalized 0-1 |
| `v2_variation_score` | Same for V2 (shimmer) - the critical metric |

## Known Issues

### Current Problem: V2 Convergence

As documented in `docs/design/v5-critique/pattern-analysis.md`, the shimmer voice converges to the same pattern regardless of seed at moderate energy with DRIFT=0. This is the primary expressiveness issue.

**Root Cause**: `PlaceEvenlySpaced()` in VoiceRelation.cpp is purely deterministic and doesn't consult the seed.

**Impact**: At DRIFT < 0.3 (including default 0.0), shimmer placement is identical across all seeds.

### Workarounds

1. Increase DRIFT parameter (>= 0.3) to engage weighted placement
2. Use higher SHAPE values (>= 0.7) for more variation
3. Combine with ENERGY changes for different hit counts

## Architecture

```
scripts/evaluate-expressiveness.py
    |
    +-- run_pattern_viz()     # Calls C++ pattern_viz binary
    |
    +-- compute_metrics()     # Calculates pattern metrics
    |
    +-- test_seed_variation() # Tests variation across seeds
    |
    +-- sweep_parameter()     # Sweeps a parameter range
    |
    +-- find_low_variation_zones()  # Identifies problems
    |
    +-- generate_markdown_report()  # Creates human-readable output
```

The script leverages the existing `pattern_viz` C++ tool to generate patterns using the real firmware algorithms, ensuring evaluation matches actual hardware behavior.

## Future Enhancements

1. **Historical Tracking**: Store evaluations over time to track trends
2. **Visualization**: Generate heatmaps of variation across parameter space
3. **Targeted Tests**: Add specific tests for known problem patterns
4. **Musical Interest Score**: Quantify how "interesting" patterns are beyond just variation
