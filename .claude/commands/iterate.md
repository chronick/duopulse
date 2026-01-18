---
description: Run a single-pass hill-climbing iteration to improve algorithm metrics
---

# Iterate: $ARGUMENTS

You are running a **single-pass hill-climbing iteration** to improve DuoPulse pattern generation metrics.

## Goal Parsing

**Input**: `$ARGUMENTS`

Parse the goal:
- If empty or "auto": Auto-detect weakest Pentagon metric and suggest goal
- Otherwise: Use as the iteration goal (e.g., "improve syncopation in wild zone")

## Workflow

### Step 1: Load Current State

Read these files to understand current baseline:

```bash
# Current metrics baseline
cat metrics/baseline.json

# Sensitivity analysis (lever recommendations)
cat metrics/sensitivity-matrix.json

# Current algorithm weights
cat inc/algorithm_config.h
```

### Step 2: Auto-Detect Goal (if needed)

If goal is "auto" or empty, analyze baseline metrics to find the weakest Pentagon metric:

```javascript
// Pentagon metrics by zone from baseline.json
const metrics = ["syncopation", "density", "velocityRange", "voiceSeparation", "regularity"];
const zones = ["stable", "syncopated", "wild"];

// Target ranges from metricDefinitions
// Find metrics furthest from their target ranges
// Suggest: "improve {metric} in {zone} zone"
```

Report the auto-detected goal before proceeding.

### Step 3: Identify Target Metric(s)

Parse the goal to identify:
1. **Target metric**: syncopation, density, velocityRange, voiceSeparation, regularity
2. **Target zone** (optional): stable, syncopated, wild
3. **Direction**: improve (default), decrease, balance

### Step 4: Find High-Impact Levers

Check sensitivity matrix for levers that affect target metric:

```javascript
// From sensitivity-matrix.json
const levers = matrix.levers[targetMetric];
// levers.primary = highest impact parameters
// levers.secondary = medium impact parameters
// levers.lowImpact = minimal effect

// If sensitivity shows zeros or no data, fall back to BootstrapLevers in algorithm_config.h
```

Select 1-2 levers to adjust based on:
- Sensitivity magnitude (higher = more impact)
- R-squared value (higher = more predictable effect)
- Direction of sensitivity (match to improvement goal)

### Step 5: Propose Weight Changes

Based on lever analysis, propose specific changes to `inc/algorithm_config.h`:

Example proposals:
```cpp
// To improve syncopation:
// kSyncopationCenter: 0.50f → 0.55f  (+10%)

// To improve regularity in stable zone:
// kEuclideanFadeStart: 0.30f → 0.35f  (+17%)
```

Guidelines:
- Small changes only: 5-15% per lever
- One lever change preferred, two maximum
- Stay within reasonable bounds (0.0 to 1.0 for floats)
- Document rationale based on sensitivity data

### Step 6: Create Iteration Branch

```bash
# Generate iteration ID
ITER_ID=$(date +%Y-%m-%d)-$(printf "%03d" $(($(ls docs/design/iterations/*.md 2>/dev/null | wc -l) + 1)))

# Create branch
git checkout -b feature/iterate-${ITER_ID}
```

### Step 7: Apply Changes

Edit `inc/algorithm_config.h` with proposed weight changes.

Verify changes compile:
```bash
make pattern-viz
```

### Step 8: Run Evaluation

Run evals to capture new metrics:

```bash
# Generate new metrics (adjust command based on project setup)
./build/pattern_viz --eval-suite 2>&1 | tee /tmp/new-metrics.txt

# Or if using Node.js evals:
npm run evals 2>&1 | tee /tmp/new-metrics.txt
```

### Step 9: Compare Metrics

Compare before (baseline.json) vs after (new metrics):

```
## Metric Comparison

| Metric | Zone | Before | After | Delta | % Change |
|--------|------|--------|-------|-------|----------|
| syncopation | total | 0.317 | 0.352 | +0.035 | +11.0% |
| ...

### Target Metric
- Goal: improve syncopation
- Before: 0.317
- After: 0.352
- Delta: +11.0% ✓ (>= 5% threshold)

### Regression Check
- Max regression: density -1.2% ✓ (<= 2% threshold)
```

### Step 10: Success Criteria

Evaluate success:

```
SUCCESS if:
  target_metric_delta >= 0.05 * baseline_value  (5% relative improvement)
  AND max(other_metric_regressions) <= 0.02 * baseline_value  (2% tolerance)
  AND all_tests_pass
```

### Step 11: Log Iteration

Create iteration log at `docs/design/iterations/${ITER_ID}.md`:

```markdown
---
iteration_id: ${ITER_ID}
goal: "${GOAL}"
status: success | failed
started_at: ${TIMESTAMP}
completed_at: ${TIMESTAMP}
branch: feature/iterate-${ITER_ID}
commit: ${COMMIT_HASH}
pr: null
---

# Iteration ${ITER_ID}: ${GOAL}

## Goal
${GOAL_DESCRIPTION}

## Baseline Metrics
(from metrics/baseline.json)

## Lever Analysis
Based on sensitivity matrix, selected:
- Primary: ${LEVER} (sensitivity: ${VALUE})
- Rationale: ${WHY}

## Proposed Changes
${CHANGES_TO_ALGORITHM_CONFIG}

## Result Metrics
${NEW_METRICS_TABLE}

## Evaluation
- Target metric: ${DELTA} ${PASS_FAIL}
- Max regression: ${MAX_REG} ${PASS_FAIL}
- Tests: ${PASS_FAIL}

## Decision
${SUCCESS | FAILED} - ${REASON}
```

### Step 12: Handle Outcome

**If SUCCESS**:
1. Run all tests: `make test`
2. Commit changes:
   ```bash
   git add inc/algorithm_config.h docs/design/iterations/
   git commit -m "feat(iterate-${ITER_ID}): ${GOAL}

   - ${LEVER}: ${OLD} → ${NEW}
   - Target metric improved ${DELTA}%
   - No regressions > 2%"
   ```
3. Create PR with metric comparison in description
4. Report success with PR link

**If FAILED**:
1. Log failure reason in iteration file
2. Revert algorithm changes: `git checkout inc/algorithm_config.h`
3. Commit only the iteration log
4. Report failure with analysis:
   - What lever was tried
   - Why it didn't work
   - Suggestions for manual intervention

## Auto-Suggest Mode

When goal is "auto", analyze current metrics to suggest improvement:

```javascript
// Find metric with largest gap from target
const gaps = [];
for (const zone of zones) {
  for (const metric of metrics) {
    const current = baseline.metrics.pentagonStats[zone][metric];
    const target = getTargetRange(metric, zone);
    const gap = targetGap(current, target);
    gaps.push({ metric, zone, current, target, gap });
  }
}

// Sort by gap size, suggest largest
const suggestion = gaps.sort((a, b) => b.gap - a.gap)[0];
console.log(`Suggested goal: improve ${suggestion.metric} in ${suggestion.zone} zone`);
```

## Output Format

Always end with a structured summary:

```
## Iteration Summary

**ID**: ${ITER_ID}
**Goal**: ${GOAL}
**Status**: ${SUCCESS | FAILED}
**Branch**: feature/iterate-${ITER_ID}

### Changes Made
- ${LEVER}: ${OLD} → ${NEW}

### Metrics Impact
| Metric | Before | After | Delta |
|--------|--------|-------|-------|
| target | X | Y | +Z% |

### Next Steps
${RECOMMENDED_ACTIONS}
```

## Important Notes

1. **Single-pass only**: No retry loops. If first attempt fails, user re-triggers manually.
2. **Small changes**: 5-15% adjustments per lever to avoid overshooting.
3. **Always log**: Both successes and failures get logged for learning.
4. **Git refs**: All logs include commit hashes and branch names.
5. **Sensitivity first**: Prefer sensitivity data over bootstrap heuristics when available.
