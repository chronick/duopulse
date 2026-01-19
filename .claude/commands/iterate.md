---
description: Run a single-pass hill-climbing iteration to improve algorithm metrics
---

# Iterate: $ARGUMENTS

You are running a **hill-climbing iteration** to improve DuoPulse pattern generation metrics.

**Usage**:
- Single-pass: `/iterate "improve syncopation"`
- Single-pass auto: `/iterate auto`
- Ensemble search: `/iterate ensemble "improve syncopation"`
- Ensemble auto: `/iterate ensemble auto`

## Mode Detection

Parse `$ARGUMENTS` to detect mode:

**Single-pass (default)**:
- Pattern: `/iterate "goal"` or `/iterate auto`
- Behavior: Generate one proposal, evaluate, create PR if improved

**Ensemble search**:
- Pattern: `/iterate ensemble "goal"` or `/iterate ensemble auto`
- Behavior: Parallel exploration of N candidates, tournament selection, multi-round search

If "ensemble" keyword detected as the first word, delegate to ensemble workflow (Step 13+).
Otherwise, continue with single-pass workflow (existing steps 1-12).

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
# Generate iteration ID using dedicated script
ITER_ID=$(node scripts/iterate/generate-iteration-id.js)

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

---

## Ensemble Search Workflow

The following steps apply only when "ensemble" keyword is detected in `$ARGUMENTS`.

### Step 13: Ensemble Search Initialization

If ensemble mode detected, run parallel weight exploration:

**13.1 Parse Ensemble Arguments**

Extract the goal from remaining arguments after "ensemble":
- `/iterate ensemble "improve syncopation"` -> goal = "improve syncopation"
- `/iterate ensemble auto` -> goal = "auto" (uses auto-detect logic from Step 2)

**13.2 Load Ensemble Config**

```bash
# Load ensemble config
cat metrics/ensemble-config.json

# Set parameters (defaults if config missing)
CANDIDATES_PER_ROUND=4
MAX_ROUNDS=5
MUTATION_RATE=0.15
CROSSOVER_RATE=0.3
```

### Step 14: Run Ensemble Search

**14.1 Generate Initial Population**

Create N candidate weight configurations:

```bash
# Generate candidates with diverse weight variations
for i in $(seq 1 $CANDIDATES_PER_ROUND); do
  # Each candidate varies 1-3 levers by 5-20%
  # Store in config/weights/candidate-${i}.json
done
```

**14.2 Parallel Evaluation**

Evaluate all candidates:

```bash
# Execute multi-round search
bash scripts/iterate/ensemble-search.sh \
  --goal "$GOAL" \
  --candidates $CANDIDATES_PER_ROUND \
  --rounds $MAX_ROUNDS

# Script outputs:
# - docs/design/iterations/ensemble-{timestamp}.md (search log)
# - config/weights/winner-{id}.json (best candidate)
```

**14.3 Tournament Selection**

For each round:
1. Evaluate all candidates against Pentagon metrics
2. Rank by target metric improvement + regression penalty
3. Select top 50% as parents
4. Generate children via crossover + mutation
5. Replace bottom 50% with children

### Step 15: Review Ensemble Results

The ensemble search will:
1. Generate N candidates with weight variations
2. Evaluate each in parallel
3. Tournament select top performers
4. Repeat for multiple rounds with crossover
5. Output final winner

Review the search log at `docs/design/iterations/ensemble-{timestamp}.md` to see:
- All candidates explored
- Tournament rankings per round
- Convergence trajectory
- Final winner selection

**15.1 Search Log Format**

```markdown
---
search_id: ensemble-{timestamp}
goal: "${GOAL}"
rounds: ${MAX_ROUNDS}
candidates_per_round: ${CANDIDATES_PER_ROUND}
total_evaluated: ${TOTAL}
---

# Ensemble Search: ${GOAL}

## Round 1
| Candidate | Levers Changed | Target Metric | Regression | Score |
|-----------|---------------|---------------|------------|-------|
| C1 | kSyncopationCenter +10% | +8.2% | -0.5% | 0.82 |
| C2 | kEuclideanFadeStart +15% | +4.1% | -1.2% | 0.41 |
...

Winners: C1, C3

## Round 2
...

## Final Winner
Candidate: C7 (Round 4)
Configuration: config/weights/winner-C7.json
Target improvement: +12.4%
Max regression: -0.8%
```

### Step 16: Create PR with Ensemble Winner

If winner improves metrics beyond threshold:

**16.1 Apply Winner Configuration**

```bash
# Winner config is in config/weights/winner-{id}.json
# Convert to algorithm_config.h format
node scripts/iterate/apply-weights.js config/weights/winner-${WINNER_ID}.json

# Verify changes compile
make pattern-viz
```

**16.2 Create Iteration Branch**

```bash
# Generate iteration ID
ITER_ID=$(node scripts/iterate/generate-iteration-id.js)

# Create branch
git checkout -b feature/iterate-${ITER_ID}
```

**16.3 Commit and Create PR**

```bash
git add inc/algorithm_config.h docs/design/iterations/
git commit -m "feat(iterate-${ITER_ID}): ${GOAL} [ensemble]

- Search: ${MAX_ROUNDS} rounds, ${TOTAL} candidates evaluated
- Winner: ${WINNER_ID}
- ${LEVER_CHANGES}
- Target metric improved ${DELTA}%
- No regressions > 2%"
```

Include in PR description:
- Number of rounds executed
- Total candidates explored
- Winning configuration details
- Before/after metrics comparison
- Link to ensemble search log

### Step 17: Handle Ensemble Outcome

**If SUCCESS**:
1. Run all tests: `make test`
2. Commit changes with ensemble metadata
3. Create PR with full search summary
4. Report success with PR link and search statistics

**If FAILED** (no candidate improved metrics):
1. Log all candidates evaluated in ensemble log
2. Identify patterns in failures (all candidates regressed same metric, etc.)
3. Suggest alternative approaches:
   - Try different levers
   - Adjust mutation/crossover rates
   - Consider manual intervention for this metric
4. Keep ensemble log for analysis

## Ensemble Output Format

For ensemble searches, end with expanded summary:

```
## Ensemble Search Summary

**Search ID**: ensemble-${TIMESTAMP}
**Goal**: ${GOAL}
**Status**: ${SUCCESS | FAILED}
**Branch**: feature/iterate-${ITER_ID}

### Search Statistics
- Rounds: ${MAX_ROUNDS}
- Candidates/round: ${CANDIDATES_PER_ROUND}
- Total evaluated: ${TOTAL}
- Convergence: Round ${CONVERGE_ROUND}

### Winner: ${WINNER_ID}
| Lever | Before | After | Delta |
|-------|--------|-------|-------|
| ${LEVER} | ${OLD} | ${NEW} | ${DELTA} |

### Metrics Impact
| Metric | Before | After | Delta |
|--------|--------|-------|-------|
| target | X | Y | +Z% |

### Search Log
See: docs/design/iterations/ensemble-${TIMESTAMP}.md

### Next Steps
${RECOMMENDED_ACTIONS}
```
