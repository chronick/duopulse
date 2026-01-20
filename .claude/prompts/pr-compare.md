---
description: Show detailed metric comparison for iteration PRs
---

# PR Metric Comparison

You are generating a detailed before/after metric comparison for an iteration PR.

## Context

**Iteration ID**: ${ITER_ID}
**Goal**: ${GOAL}
**PR Number**: #${PR_NUMBER}

## Your Task

Create a comprehensive metric comparison showing:

1. Target metric detailed breakdown (all zones)
2. All Pentagon metrics comparison
3. Regression analysis
4. Prediction accuracy
5. Visual summary

## Data Sources

- **Baseline**: `metrics/baseline.json`
- **Current**: Latest eval results or PR metrics
- **Predictions**: `docs/design/iterations/estimate-${ITER_ID}.md`

## Comparison Format

```markdown
## 📊 Detailed Metric Comparison

### Target Metric: ${TARGET_METRIC}

#### By Zone
| Zone | Before | After | Delta | % Change | Status |
|------|--------|-------|-------|----------|--------|
| Stable | 0.000 | 0.000 | +0.000 | +0.0% | ✅/⚠️/❌ |
| Syncopated | 0.000 | 0.000 | +0.000 | +0.0% | ✅ |
| Wild | 0.000 | 0.000 | +0.000 | +0.0% | ✅ |

#### Overall
| Metric | Before | After | Delta | % Change | Target | Status |
|--------|--------|-------|-------|----------|--------|--------|
| ${TARGET_METRIC} | 0.000 | 0.000 | +0.000 | +0.0% | +5.0% | ✅/❌ |

**Target Achievement**: ${ACHIEVED/NOT_ACHIEVED}

---

### All Pentagon Metrics

| Metric | Zone | Before | After | Delta | % Change | Predicted | Accuracy | Status |
|--------|------|--------|-------|-------|----------|-----------|----------|--------|
| syncopation | stable | 0.00 | 0.00 | +0.00 | +0.0% | +0.0% | 100% | ✅ |
| syncopation | syncopated | 0.00 | 0.00 | +0.00 | +0.0% | +0.0% | 100% | ✅ |
| syncopation | wild | 0.00 | 0.00 | +0.00 | +0.0% | +0.0% | 100% | ✅ |
| density | stable | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |
| density | syncopated | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |
| density | wild | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |
| velocityRange | total | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |
| voiceSeparation | total | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |
| regularity | stable | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |
| regularity | syncopated | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |
| regularity | wild | 0.00 | 0.00 | +0.00 | +0.0% | ±0.0% | 100% | ✅ |

**Legend**: ✅ Improved/Neutral | ⚠️ Acceptable Tradeoff | ❌ Regression

---

### Regression Analysis

#### Regressions Found
${COUNT} metric(s) regressed:

| Metric | Zone | Regression | Classification |
|--------|------|------------|----------------|
| ${METRIC} | ${ZONE} | -X.X% | Acceptable / Unacceptable |

#### Classification Rationale
- **${METRIC}**: ${CLASSIFICATION_REASON}

---

### Prediction Accuracy

#### Target Metric
| Aspect | Predicted | Actual | Error | Accuracy |
|--------|-----------|--------|-------|----------|
| ${TARGET_METRIC} | +X.X% | +Y.Y% | ±Z.Z% | WW% |

#### Secondary Metrics
| Metric | Predicted | Actual | Error | Accuracy |
|--------|-----------|--------|-------|----------|
| ${METRIC_1} | ±X.X% | ±Y.Y% | ±Z.Z% | WW% |
| ${METRIC_2} | ±X.X% | ±Y.Y% | ±Z.Z% | WW% |

**Overall Prediction Accuracy**: XX% (higher is better)

---

### Visual Summary

\`\`\`
TARGET: ${TARGET_METRIC}
┌─────────────────────────────────────┐
│ Baseline:  ████████░░ 0.XXX        │
│ Current:   ██████████ 0.YYY  (+Z%) │
│ Target:    ██████████ +5%          │
└─────────────────────────────────────┘
Status: ${ACHIEVED / NOT_ACHIEVED}

REGRESSIONS:
${LIST_OR_NONE}

OVERALL: ${SUCCESS / TRADEOFF / FAILED}
\`\`\`

---

### Interpretation

${NARRATIVE_INTERPRETATION_OF_RESULTS}

**Key Findings**:
1. ${FINDING_1}
2. ${FINDING_2}
3. ${FINDING_3}

**Recommendation**: ${MERGE / NEEDS_WORK / RETRY}
```

## Guidelines

1. **Complete data**: Include all Pentagon metrics across all zones
2. **Show predictions**: Compare actual vs predicted for learning
3. **Classify regressions**: Acceptable vs unacceptable with rationale
4. **Calculate accuracy**: Percentage accuracy for each prediction
5. **Provide interpretation**: Narrative explanation of what results mean
6. **Make recommendation**: Clear merge/needs-work/retry decision

## Example

See above format for structure. Fill with actual data from baseline.json and current metrics.

## Output

Provide the comparison report in markdown format, ready to be posted as a PR comment.

Do NOT include meta-commentary - just write the comparison report itself.
