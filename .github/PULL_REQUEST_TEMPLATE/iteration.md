## Iteration PR: [Goal]

**Iteration ID**: YYYY-MM-DD-NNN
**Branch**: `feature/iterate-YYYY-MM-DD-NNN`

---

## 🎯 Goal

<!-- What improvement was targeted? -->

---

## 📊 Metric Comparison

### Target Metric

| Metric | Zone | Before | After | Delta | % Change |
|--------|------|--------|-------|-------|----------|
| ... | ... | 0.00 | 0.00 | +0.00 | +0.0% |

### All Pentagon Metrics

| Metric | Zone | Before | After | Delta | % Change | Status |
|--------|------|--------|-------|-------|----------|--------|
| syncopation | stable | 0.00 | 0.00 | +0.00 | +0.0% | ✅/⚠️/❌ |
| syncopation | syncopated | 0.00 | 0.00 | +0.00 | +0.0% | ✅ |
| syncopation | wild | 0.00 | 0.00 | +0.00 | +0.0% | ✅ |
| density | stable | 0.00 | 0.00 | +0.00 | +0.0% | ✅ |
| velocityRange | total | 0.00 | 0.00 | +0.00 | +0.0% | ✅ |
| voiceSeparation | total | 0.00 | 0.00 | +0.00 | +0.0% | ✅ |
| regularity | stable | 0.00 | 0.00 | +0.00 | +0.0% | ✅ |

**Legend**: ✅ Improved/Neutral | ⚠️ Acceptable Tradeoff | ❌ Regression

---

## 🔧 Changes

### Levers Adjusted

| Lever | Before | After | Delta | Rationale |
|-------|--------|-------|-------|-----------|
| `kExampleLever` | 0.50f | 0.55f | +10% | ... |

### Files Modified

- `inc/algorithm_config.h` - Updated lever values
- `docs/design/iterations/${ITER_ID}.md` - Iteration log

---

## 💭 Designer Rationale

### Why These Changes?

<!-- Explain the reasoning behind lever selection -->

### Predicted Impact

**Primary Effects**:
- Target metric: Expected +X% improvement
- Mechanism: ...

**Secondary Effects**:
- Other metric 1: Expected ±Y%
- Other metric 2: Expected ±Z%

**Confidence**: HIGH/MEDIUM/LOW (X%)

### Critic Feedback

<!-- If design-critic was invoked, include feedback here -->

---

## ⚠️ Tradeoff Justification

<!-- Only fill if this PR has "tradeoff" label -->

**Regression**: ${METRIC} decreased ${AMOUNT}%
**Gain**: ${TARGET_METRIC} increased ${AMOUNT}%

**Musical Rationale**:
<!-- Why is this tradeoff worthwhile? -->

**Recovery Plan**:
- [ ] This iteration: Accept regression for target improvement
- [ ] Next iteration: Address regression while preserving gains

---

## ✅ Status Checklist

- [ ] Metrics improved >= 5% (or tradeoff justified)
- [ ] No unacceptable regressions (> 2% without justification)
- [ ] All tests pass (`make test`)
- [ ] Iteration log created at `docs/design/iterations/${ITER_ID}.md`
- [ ] Prediction accuracy documented
- [ ] Lessons learned captured

---

## 📈 Results vs Predictions

### Accuracy Analysis

| Aspect | Predicted | Actual | Error | Accuracy |
|--------|-----------|--------|-------|----------|
| Target metric | +X% | +Y% | ±Z% | W% |
| Secondary 1 | ±X% | ±Y% | ±Z% | W% |
| Secondary 2 | ±X% | ±Y% | ±Z% | W% |

**Overall Estimate Accuracy**: X% (higher is better)

### Lessons Learned

**What We Got Right**:
- ...

**What Surprised Us**:
- ...

**Why Predictions Were Off**:
- ...

**Implications for Future Iterations**:
- ...

---

## 🔗 Links

- Iteration log: `docs/design/iterations/${ITER_ID}.md`
- Epic: `docs/tasks/epics/2026-01-19-v55-pattern-expressiveness.md`
- Baseline metrics: `metrics/baseline.json`

---

## 🤖 PR Commands

Comment with these commands to interact with this PR:

| Command | Description |
|---------|-------------|
| `@claude /status` | Report current iteration status |
| `@claude /critique` | Request design critique |
| `@claude /explain ${METRIC}` | Explain specific metric change |
| `@claude /retry` | Retry with different approach |
| `@claude /compare baseline` | Show detailed before/after comparison |
| `@claude /tradeoff justify` | Explain regression justification |

---

**Iteration #N** of hill-climbing session
🤖 Generated with [Claude Code](https://claude.ai/code)
