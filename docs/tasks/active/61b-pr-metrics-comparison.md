---
id: 61b
slug: pr-metrics-comparison
title: "PR Metrics Comparison"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/pr-metrics-comparison
spec_refs: []
depends_on:
  - 61a  # Baseline infrastructure
---

# Task 61b: PR Metrics Comparison

## Objective

Add a GitHub Actions workflow that compares PR metrics to baseline and posts results as a PR comment. Fails CI if any metric regresses beyond threshold.

## Context

### Background

This task was split from Task 61 during design review (2026-01-18). It builds on the baseline infrastructure from Task 61a to add CI comparison.

### Current State (After 61a)

- `metrics/baseline.json` exists with main branch metrics
- No PR-specific metrics comparison
- Pages workflow runs evals but doesn't compare

### Target State

- PR workflow builds pattern_viz with PR changes
- Runs evals against PR branch
- Compares to `metrics/baseline.json`
- Posts comparison table as PR comment
- Fails if any metric regresses > 2%
- Manual override label for intentional regressions

## Subtasks

### CI Workflow
- [ ] Create or modify workflow for PR metrics
- [ ] Build pattern_viz on PR branches
- [ ] Run evals with PR code
- [ ] Compare against baseline.json

### Comparison Script
- [ ] Create `scripts/compare-metrics.js` (or bash)
- [ ] Read baseline from `metrics/baseline.json`
- [ ] Read PR metrics from evals output
- [ ] Calculate deltas (absolute and percentage)
- [ ] Determine pass/fail based on thresholds

### PR Comment
- [ ] Post metric comparison table as PR comment
- [ ] Show before/after values
- [ ] Highlight regressions in red
- [ ] Include overall pass/fail status
- [ ] Update existing comment instead of creating new ones

### Regression Threshold
- [ ] Default threshold: 2% regression
- [ ] Configurable via workflow input or file
- [ ] Different thresholds for different metrics (optional)

### Override Mechanism
- [ ] Create `allow-regression` label
- [ ] When label present, regression doesn't fail CI
- [ ] Comment notes that override was used

### Tests
- [ ] Test comparison script with sample data
- [ ] Test pass/fail logic
- [ ] Test PR comment formatting
- [ ] All tests pass

## Acceptance Criteria

- [ ] PRs trigger metrics comparison
- [ ] Comparison posted as PR comment
- [ ] Regressions > 2% fail CI
- [ ] Override label allows intentional regressions
- [ ] Comment updates on push (no duplicates)
- [ ] Works for iteration PRs and regular PRs

## Implementation Notes

### Workflow Structure

```yaml
# .github/workflows/pr-metrics.yml
name: PR Metrics

on:
  pull_request:
    branches: [main]
    paths:
      - 'src/**'
      - 'inc/**'
      - 'tools/pattern_viz/**'

jobs:
  compare-metrics:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      pull-requests: write

    steps:
      - uses: actions/checkout@v4

      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: '20'

      - name: Build pattern_viz
        run: make build/pattern_viz

      - name: Generate PR patterns
        working-directory: tools/evals
        run: npm run generate-patterns

      - name: Run evaluations
        working-directory: tools/evals
        run: npm run evaluate

      - name: Compare to baseline
        id: compare
        run: node scripts/compare-metrics.js
        env:
          BASELINE_PATH: metrics/baseline.json
          PR_METRICS_PATH: tools/evals/public/expressiveness.json
          REGRESSION_THRESHOLD: 0.02

      - name: Post PR comment
        uses: actions/github-script@v7
        with:
          script: |
            const fs = require('fs');
            const comparison = JSON.parse(fs.readFileSync('metrics-comparison.json'));
            // ... post comment logic

      - name: Check for regressions
        run: |
          if [ "${{ steps.compare.outputs.has_regression }}" == "true" ]; then
            if [[ "${{ contains(github.event.pull_request.labels.*.name, 'allow-regression') }}" == "true" ]]; then
              echo "Regression allowed by label"
            else
              echo "::error::Metric regression detected"
              exit 1
            fi
          fi
```

### Comparison Script Output

```json
{
  "baseline_commit": "abc123",
  "pr_commit": "def456",
  "threshold": 0.02,
  "has_regression": false,
  "metrics": [
    {
      "name": "syncopation",
      "zone": "overall",
      "baseline": 0.42,
      "pr": 0.44,
      "delta": 0.02,
      "delta_pct": 4.76,
      "status": "improved"
    },
    {
      "name": "density",
      "zone": "wild",
      "baseline": 0.65,
      "pr": 0.63,
      "delta": -0.02,
      "delta_pct": -3.08,
      "status": "regression"
    }
  ]
}
```

### PR Comment Format

```markdown
## Pentagon Metrics Comparison

**Baseline**: abc123 (2026-01-17)
**PR**: def456 (this PR)
**Threshold**: 2% regression

| Metric | Zone | Baseline | PR | Delta | Status |
|--------|------|----------|-----|-------|--------|
| syncopation | overall | 0.42 | 0.44 | +4.76% | ✅ |
| density | wild | 0.65 | 0.63 | -3.08% | ⚠️ |
| ... | ... | ... | ... | ... | ... |

**Result**: ⚠️ 1 regression detected

> Note: Add `allow-regression` label to bypass regression check.
```

### Files to Create

- `.github/workflows/pr-metrics.yml` (or add job to existing workflow)
- `scripts/compare-metrics.js`

### Files to Modify

- (none required, but may integrate with pages.yml)

## Test Plan

1. Create a test PR with no algorithm changes
2. Verify metrics match baseline (no regression)
3. Create a test PR with intentional regression
4. Verify CI fails with regression
5. Add `allow-regression` label
6. Verify CI passes with override

## Estimated Effort

2-3 hours
