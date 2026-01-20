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

## Git Hygiene & Branch Management

**YES**, `/iterate` follows strict git hygiene:
- Creates feature branch: `feature/iterate-${ITER_ID}`
- Commits all changes with structured messages
- **ALWAYS creates PR** (success or failure - for learning)
- PR description includes metrics, predictions, and rationale
- PR merge to main triggers next iteration (Task 57 CI/CD loop)
- Failed iterations close PR with `retry` label for future reference

## Automated CI/CD Loop (Task 57 Integration)

Each iteration participates in a continuous improvement loop:

```
[main] → /iterate → [feature branch] → [commit] → [PR created]
   ↑                                                      ↓
   └─────────── [PR merged] ←────────── [Review & Approve]
                                                ↓
                                         [Regression?] → [Close with "retry" label]
```

**Key behaviors**:
- PR created WHETHER OR NOT metrics improve (track all attempts)
- PR merge to main automatically triggers next iteration suggestion
- Regression PRs closed with `retry` label, logged for learning
- Successful PRs merged, baseline updated, process repeats

## Workflow Overview

Each iteration follows a **4-phase cycle** for systematic learning:

### 1. ESTIMATE Phase (NEW)
**Before changing anything**, document predictions:
- Which metrics will improve, by how much?
- What secondary effects will occur?
- What risks might prevent success?
- Confidence level in predictions

### 2. IMPLEMENT Phase
Apply proposed weight changes:
- Select high-impact levers from sensitivity analysis
- Make small adjustments (5-15% per lever)
- Verify code compiles

### 3. MEASURE Phase
Run evaluation and compare metrics:
- Generate new metrics via evals
- Compare before/after for all Pentagon metrics
- Check success criteria (target improvement + no regressions)

### 4. LOG Phase (Enhanced)
Document results with **prediction accuracy**:
- Compare predictions vs actual results
- Calculate estimate accuracy score
- Extract lessons learned for future iterations
- Narrate findings in human-readable form

**Why this matters**: By tracking prediction accuracy over time, we learn which types of changes reliably improve which metrics, making future iterations more precise.

## Critic Agent Integration

The **design-critic** agent provides critical feedback at strategic points to catch issues early:

**Invocation Points**:
1. **After Step 5 (Estimate)**: Critique predictions before implementing
   - Are estimates realistic given sensitivity data?
   - Are we missing secondary effects?
   - Risk assessment complete?

2. **After Step 10 (Compare Metrics)**: Critique results before PR creation
   - Do metric changes make sense?
   - Are there hidden regressions?
   - Is the narrative coherent?

3. **On PR Feedback**: Respond to user-requested design critique
   - User comments: `@claude /critique` on PR
   - Provides alternative approaches if current path isn't working

**Usage**:
```bash
# In iterate workflow, before committing:
Task(subagent_type="design-critic",
     prompt="Critique these weight changes and predicted impacts: [paste estimate]",
     description="Critique iteration estimates")
```

**Benefits**:
- Catches overconfident predictions
- Identifies unintended consequences
- Suggests alternative approaches
- Validates reasoning before expensive eval runs

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

# Iteration lessons learned (CRITICAL - read first!)
cat metrics/iteration-lessons.json

# Current algorithm weights
cat inc/algorithm_config.h
```

**IMPORTANT**: Review iteration lessons BEFORE proposing changes:
- Check if similar goal has been attempted before
- Look for parameter domain mismatches (ENERGY vs SHAPE confusion)
- Apply calibration lessons to predictions
- Check bootstrap heuristic confidence adjustments

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

### Step 4.5: Check Iteration Lessons (CRITICAL)

**BEFORE proposing changes, consult iteration-lessons.json for known pitfalls:**

```javascript
// From iteration-lessons.json
const lessons = iterationLessons.lessons;
const patterns = iterationLessons.patterns;

// Check for similar failed attempts
for (const lesson of lessons) {
  if (lesson.goal.includes(targetMetric) && lesson.result === "FAILED") {
    console.warn(`⚠️ Similar goal failed: ${lesson.iteration_id}`);
    console.warn(`  Problem: ${lesson.critical_error}`);
    console.warn(`  Recommendation: ${lesson.recommended_retry.rationale}`);
  }
}

// Check for parameter domain mismatches (CRITICAL!)
if (targetZone && selectedLever) {
  const leverDomain = getLeverParameterDomain(selectedLever); // ENERGY or SHAPE?
  const zoneDomain = getZoneParameterDomain(targetZone); // ENERGY or SHAPE?

  if (leverDomain !== zoneDomain) {
    console.error(`❌ DOMAIN MISMATCH: Lever operates on ${leverDomain}, zone defined by ${zoneDomain}`);
    console.error(`   This will NOT affect the target zone!`);
    console.error(`   Use ${zoneDomain}-domain levers instead.`);
    // See iteration 2026-01-19-002 for example
  }
}

// Apply confidence calibration
if (bootstrapConfidence && sensitivityRSquared < 0.5) {
  const adjustedConfidence = Math.min(bootstrapConfidence, sensitivityRSquared * 5);
  console.warn(`⚠️ Downgrading confidence: ${bootstrapConfidence}/5 → ${adjustedConfidence}/5`);
  console.warn(`   Reason: Sensitivity R²=${sensitivityRSquared} < 0.5 (weak signal)`);
}
```

**Parameter Domain Reference** (from lessons):
- **ENERGY levers**: kAnchorKMin/Max, kShimmerKMin/Max, kAuxKMin/Max
  - These scale with ENERGY parameter (0.0-1.0)
  - Affect zone-specific behavior (stable/syncopated/wild zones)

- **SHAPE levers**: kEuclidean*, kSyncopation*, kRandom*
  - These operate on SHAPE parameter (0.0-1.0)
  - Affect global algorithm blending, NOT zone-specific

**Zone Definitions** (from lessons):
- **ENERGY zones**: stable (0.0-0.3), syncopated (0.3-0.7), wild (0.7-1.0)
- **SHAPE zones**: N/A (SHAPE is a continuous parameter, not zone-based)

**Validation Checklist**:
- [ ] Lever parameter domain matches target zone domain
- [ ] Similar goal hasn't failed recently with same approach
- [ ] Bootstrap confidence adjusted for sensitivity R²
- [ ] Prediction magnitude scaled by sensitivity slope (not assumed linear)

If validation fails, STOP and select a different lever or approach.

### Step 5: Estimate Improvement (NEW)

**BEFORE making any changes, document predictions for learning:**

**5.1 Create Prediction Record**

Create prediction record in `docs/design/iterations/estimate-${ITER_ID}.md`:

```markdown
## Improvement Estimates

**Goal**: ${GOAL}
**Target Metric**: ${TARGET_METRIC}

### Predicted Changes

**Levers to adjust**:
- ${LEVER_NAME}: ${CURRENT_VALUE} → ${PROPOSED_VALUE} (${DELTA}%)
- Rationale: ${WHY_THIS_LEVER}

### Predicted Impact

**Primary Effects** (target metric):
- ${TARGET_METRIC}: Expected +${PERCENTAGE}% improvement
- Reasoning: ${EXPLAIN_MECHANISM}
- Confidence: ${HIGH|MEDIUM|LOW} (${CONFIDENCE_PERCENTAGE}%)

**Secondary Effects** (other metrics):
- ${METRIC_1}: Expected ${+/-}${PERCENTAGE}% change
- ${METRIC_2}: Expected ${+/-}${PERCENTAGE}% change
- Reasoning: ${WHY_THESE_EFFECTS}

**Risk Assessment**:
- Regression risk: ${HIGH|MEDIUM|LOW}
- Potential regressions: ${WHICH_METRICS}
- Mitigation: ${HOW_TO_PREVENT}

### Success Criteria

- Target metric improves by >= ${MIN_THRESHOLD}%
- No metric regresses by > 2%
- All tests pass

### Learning Objectives

What will this iteration teach us about:
- ${QUESTION_1}
- ${QUESTION_2}
```

**Guidelines for estimates**:
- Be specific: Give percentage predictions, not vague "should improve"
- State confidence: HIGH (>80%), MEDIUM (50-80%), LOW (<50%)
- Explain mechanism: WHY will this change improve the metric?
- Predict secondary effects: What else might change?
- Document assumptions: What are you assuming about the system?

**5.2 Invoke Critic Agent (Optional but Recommended)**

Before proceeding, get critical feedback on predictions:

```bash
Task(subagent_type="design-critic",
     description="Critique iteration estimates",
     prompt="Review these iteration estimates for iteration ${ITER_ID}:

     [Paste estimate-${ITER_ID}.md content]

     Evaluate:
     - Are predictions realistic given sensitivity data?
     - Are secondary effects adequately considered?
     - Is the risk assessment complete?
     - Are there alternative approaches that might work better?
     - What could go wrong that wasn't predicted?

     Provide critical analysis and recommendations.")
```

**When to use critic**:
- ✅ LOW confidence predictions (< 60%)
- ✅ Complex multi-lever changes
- ✅ High-risk changes to critical metrics
- ✅ After previous iteration failed
- ⚠️ Skip for HIGH confidence bug fixes (e.g., Task 46 noise formula)

**Incorporate feedback**:
- Update estimates based on critic insights
- Adjust lever selections if critic identifies issues
- Add missed risks to risk assessment
- Document critic feedback in estimate file

### Step 6: Propose Weight Changes

Based on lever analysis and estimates, propose specific changes to `inc/algorithm_config.h`:

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

### Step 7: Create Iteration Branch

```bash
# Generate iteration ID using dedicated script
ITER_ID=$(node scripts/iterate/generate-iteration-id.js)

# Create branch
git checkout -b feature/iterate-${ITER_ID}
```

### Step 8: Apply Changes

Edit `inc/algorithm_config.h` with proposed weight changes.

Verify changes compile:
```bash
make pattern-viz
```

### Step 9: Run Evaluation

Run evals to capture new metrics:

```bash
# Generate new metrics (adjust command based on project setup)
./build/pattern_viz --eval-suite 2>&1 | tee /tmp/new-metrics.txt

# Or if using Node.js evals:
npm run evals 2>&1 | tee /tmp/new-metrics.txt
```

### Step 10: Compare Metrics

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

### Step 11: Success Criteria

Evaluate success:

```
SUCCESS if:
  target_metric_delta >= 0.05 * baseline_value  (5% relative improvement)
  AND max(other_metric_regressions) <= 0.02 * baseline_value  (2% tolerance)
  AND all_tests_pass
```

**Important**: Regression tolerance is NOT absolute. Context matters:

### Acceptable Regressions (Tradeoff Policy)

Some regressions may be acceptable as intentional tradeoffs:

**✅ Acceptable Regression Scenarios**:

1. **Planned Tradeoff**: Explicitly trading one metric for another
   - Example: Decrease regularity to improve syncopation
   - Document in estimate phase: "Willing to accept -5% regularity for +15% syncopation"
   - Justify with musical rationale

2. **Zone-Specific Improvement**: Regression in one zone, improvement in target zone
   - Example: Wild zone syncopation +20%, stable zone syncopation -3%
   - Net positive: Target zone gains outweigh collateral loss
   - Document zone priorities in estimate

3. **Temporary Regression**: Part of multi-iteration strategy
   - Example: Step 1 reduces density to improve voice separation
   - Step 2 will restore density with better separation
   - Document multi-step plan in iteration log

4. **Negligible Impact**: Regression below perceptual threshold
   - Example: Velocity range drops 1% (0.85 → 0.84)
   - Musical impact minimal
   - Focus on larger target improvement

**❌ Unacceptable Regressions**:

1. **Unplanned**: Not predicted or justified in estimate phase
2. **Critical metrics**: Beat 1 presence, test pass rate, compilation
3. **Excessive**: >5% regression without strong justification
4. **Cascading**: Multiple metrics regressing simultaneously

**PR Creation Policy**:
- **Success**: Create PR with "improvement" label → request review
- **Acceptable tradeoff**: Create PR with "tradeoff" label → explain justification → request review
- **Unacceptable regression**: Create PR with "retry" label → close without merge → log for learning

**Documentation Requirements**:
If creating "tradeoff" PR, include in description:
```markdown
## Tradeoff Justification

**Regression**: ${METRIC} decreased ${AMOUNT}%
**Gain**: ${TARGET_METRIC} increased ${AMOUNT}%

**Musical Rationale**:
${WHY_THIS_TRADEOFF_IS_WORTHWHILE}

**Plan**:
- [ ] This iteration: Accept regression for target improvement
- [ ] Next iteration: Address regression while preserving gains
```

### Step 12: Log Iteration with Estimate Accuracy

Create iteration log at `docs/design/iterations/${ITER_ID}.md` with **prediction vs actual comparison**:

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
estimate_accuracy: ${ACCURACY_SCORE}  # NEW: 0-100 scale
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

## Estimates (Prediction Phase)

### Predicted Impact
**Target Metric**: ${TARGET_METRIC}
- Predicted: ${PREDICTED_DELTA}% improvement
- Confidence: ${CONFIDENCE_LEVEL} (${CONFIDENCE_PCT}%)

**Secondary Effects**:
- ${METRIC_1}: Predicted ${PREDICTED_DELTA_1}%
- ${METRIC_2}: Predicted ${PREDICTED_DELTA_2}%

**Risks Identified**:
- ${RISK_1}
- ${RISK_2}

## Proposed Changes
${CHANGES_TO_ALGORITHM_CONFIG}

## Result Metrics
${NEW_METRICS_TABLE}

## Prediction Accuracy Analysis

### Target Metric Accuracy
| Aspect | Predicted | Actual | Error | Accuracy |
|--------|-----------|--------|-------|----------|
| ${TARGET_METRIC} | ${PRED}% | ${ACTUAL}% | ${ERROR}% | ${ACCURACY}% |

### Secondary Metrics Accuracy
| Metric | Predicted | Actual | Error | Accuracy |
|--------|-----------|--------|-------|----------|
| ${METRIC_1} | ${PRED}% | ${ACTUAL}% | ${ERROR}% | ${ACCURACY}% |
| ${METRIC_2} | ${PRED}% | ${ACTUAL}% | ${ERROR}% | ${ACCURACY}% |

**Overall Estimate Accuracy**: ${OVERALL_ACCURACY}% (higher is better)

**Calculation**:
```
accuracy = 100 - abs((predicted - actual) / predicted * 100)
overall = avg(target_accuracy, secondary_accuracies)
```

## Lessons Learned

### What We Got Right
- ${CORRECT_PREDICTION_1}
- ${CORRECT_PREDICTION_2}

### What Surprised Us
- ${UNEXPECTED_RESULT_1}
- ${UNEXPECTED_RESULT_2}

### Why Predictions Were Off
- ${REASON_FOR_ERROR_1}
- ${REASON_FOR_ERROR_2}

### Implications for Future Iterations
- Calibration adjustment: ${HOW_TO_IMPROVE_ESTIMATES}
- New hypotheses: ${NEW_THEORIES}
- Sensitivity updates: ${WHAT_TO_MEASURE}

## Evaluation
- Target metric: ${DELTA} ${PASS_FAIL}
- Max regression: ${MAX_REG} ${PASS_FAIL}
- Tests: ${PASS_FAIL}

## Decision
${SUCCESS | FAILED} - ${REASON}

## Narration

${LEGIBLE_NARRATION_OF_WHAT_HAPPENED}

This section should read like a story:
- What we tried to do and why
- What we expected to happen
- What actually happened
- Why the results make sense (or don't)
- What we learned for next time

Make it accessible to someone reading this 6 months from now.
```

**Key additions to logging**:
1. **estimate_accuracy** frontmatter field for tracking
2. **Prediction Accuracy Analysis** table comparing predictions to reality
3. **Lessons Learned** section documenting calibration insights
4. **Narration** section for human-readable summary

### Step 13: Handle Outcome

**ALWAYS create PR regardless of outcome** (for learning and transparency).

#### Outcome A: SUCCESS (Target improved, no regressions)

1. Run all tests: `make test`
2. Commit changes:
   ```bash
   git add inc/algorithm_config.h docs/design/iterations/
   git commit -m "feat(iterate-${ITER_ID}): ${GOAL}

   - ${LEVER}: ${OLD} → ${NEW}
   - Target metric improved ${DELTA}%
   - No regressions > 2%

   Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>"
   ```
3. Push branch: `git push -u origin feature/iterate-${ITER_ID}`
4. Create PR with "improvement" label:
   ```bash
   gh pr create \
     --title "Iteration ${ITER_ID}: ${GOAL}" \
     --label "improvement,iteration" \
     --body "$(cat docs/design/iterations/${ITER_ID}.md)"
   ```
5. Report success with PR link

#### Outcome B: ACCEPTABLE TRADEOFF (Target improved, acceptable regression)

1. Run all tests: `make test`
2. Update iteration log with tradeoff justification
3. Commit changes (same as success)
4. Push branch
5. Create PR with "tradeoff" label:
   ```bash
   gh pr create \
     --title "Iteration ${ITER_ID}: ${GOAL} [tradeoff]" \
     --label "tradeoff,iteration,needs-review" \
     --body "$(cat docs/design/iterations/${ITER_ID}.md)

     ## ⚠️ Tradeoff Alert

     This iteration includes an acceptable regression:
     - Regression: ${METRIC} ${DELTA}%
     - Gain: ${TARGET_METRIC} ${DELTA}%
     - Justification: ${REASON}

     Requires explicit approval before merge."
   ```
6. Report tradeoff with explanation

#### Outcome C: FAILED (No improvement OR unacceptable regression)

1. **Invoke Critic Agent** for post-mortem:
   ```bash
   Task(subagent_type="design-critic",
        description="Analyze iteration failure",
        prompt="Iteration ${ITER_ID} failed to improve metrics:

        [Paste results from Step 10]

        Analyze:
        - Why did predictions not match results?
        - What mechanism failed?
        - Alternative approaches to try?
        - Should we abort this goal or retry with different levers?")
   ```

2. Update iteration log with failure analysis and critic feedback
3. Revert algorithm changes: `git checkout inc/algorithm_config.h`
4. Commit iteration log only:
   ```bash
   git add docs/design/iterations/${ITER_ID}.md
   git commit -m "docs(iterate-${ITER_ID}): ${GOAL} [failed]

   - Attempted: ${LEVER} ${OLD} → ${NEW}
   - Result: ${WHAT_HAPPENED}
   - Lessons: ${KEY_LEARNINGS}

   Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>"
   ```
5. Push branch
6. Create PR with "retry" label:
   ```bash
   gh pr create \
     --title "Iteration ${ITER_ID}: ${GOAL} [FAILED]" \
     --label "retry,iteration,needs-analysis" \
     --body "$(cat docs/design/iterations/${ITER_ID}.md)

     ## ❌ Iteration Failed

     This iteration did not improve metrics and will not be merged.
     Preserving for learning purposes.

     **What was tried**: ${LEVER_CHANGES}
     **What happened**: ${RESULTS}
     **Critic analysis**: ${CRITIC_FEEDBACK}
     **Next steps**: ${RECOMMENDATIONS}"
   ```
7. Close PR immediately (don't leave open):
   ```bash
   gh pr close ${PR_NUMBER} --comment "Closing failed iteration. See iteration log for analysis and retry suggestions."
   ```
8. Report failure with critic analysis and retry suggestions

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
4. **Always PR**: Create PR whether success, tradeoff, or failure (learning transparency).
5. **Git refs**: All logs include commit hashes and branch names.
6. **Sensitivity first**: Prefer sensitivity data over bootstrap heuristics when available.
7. **Critic integration**: Invoke design-critic for estimates (Step 5.2) and failures (Step 13C).
8. **Regression policy**: Document acceptable tradeoffs explicitly in estimates.

## PR Commands (Task 57 Integration)

Once PR is created, users can interact via comments:

| Command | Description | Triggers |
|---------|-------------|----------|
| `@claude /status` | Report current iteration status | Status summary |
| `@claude /critique` | Request design critique | design-critic agent |
| `@claude /explain ${METRIC}` | Explain metric change | Detailed analysis |
| `@claude /retry` | Retry with different approach | New estimate + implementation |
| `@claude /compare baseline` | Show before/after comparison | Metric diff table |
| `@claude /tradeoff justify` | Explain regression tradeoff | Tradeoff rationale |

## Continuous Improvement Loop

When PR merges to main, the cycle continues:

```
PR Merged → Update baseline.json → GitHub Action triggers → Suggest next iteration

GitHub Action (.github/workflows/iterate-suggest.yml):
- Runs on PR merge to main
- Checks if PR has "iteration" label
- Updates metrics/baseline.json
- Posts issue: "Iteration ${ITER_ID} merged! Suggested next goal: [auto-detected]"
- Optional: Auto-trigger next /iterate auto if configured
```

This creates a self-improving system where each successful iteration automatically suggests the next improvement target.

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

---

## Quick Reference: Lessons Learned

### Parameter Domain Mapping (Updated from Iteration Lessons)

**ENERGY-Domain Levers** (affect zone-specific behavior):
```cpp
kAnchorKMin/Max   // Anchor (kick) hit density at different ENERGY levels
kShimmerKMin/Max  // Shimmer (hi-hat) hit density at different ENERGY levels
kAuxKMin/Max      // Aux (perc) hit density at different ENERGY levels
```
Use these when targeting **ENERGY zones** (stable/syncopated/wild).

**SHAPE-Domain Levers** (affect global algorithm blending):
```cpp
kEuclideanFadeStart/End    // Euclidean algorithm influence curve
kSyncopationCenter/Width   // Syncopation algorithm bell curve
kRandomFadeStart/End       // Random algorithm influence curve
```
Use these for **SHAPE-based goals** (e.g., "make SHAPE=0.5 more syncopated").

**Zone Definitions** (CRITICAL - two independent systems!):

**ENERGY zones** (controls generation - HitBudget, eligibility):
- MINIMAL (0.0-0.2): Sparse patterns, strong-beat eligibility only
- GROOVE (0.2-0.5): Standard density
- BUILD (0.5-0.75): Building energy
- PEAK (0.75-1.0): Maximum density

**SHAPE zones** (controls metric TARGETS in evals):
- stable (0.0-0.3): Low syncopation targets
- syncopated (0.3-0.7): Medium syncopation targets
- wild (0.7-1.0): High syncopation targets

**Key insight**: These are INDEPENDENT! A pattern at ENERGY=0.1 (MINIMAL) can have any SHAPE value. Metric targets come from SHAPE zones, but eligibility masks come from ENERGY zones.

### Common Pitfalls (from Failed Iterations)

**1. Parameter Domain Confusion** (iteration 2026-01-19-002):
```
❌ BAD:  Use kEuclideanFadeEnd to improve stable zone regularity
          (SHAPE lever for ENERGY zone - domains don't overlap!)

✅ GOOD: Use kAnchorKMin to improve stable zone regularity
          (ENERGY lever for ENERGY zone - correct domain)
```

**2. Bootstrap Overconfidence**:
```
❌ BAD:  Trust bootstrap 4/5 confidence when sensitivity R²=0.3
          (70% unexplained variance = weak signal)

✅ GOOD: Downgrade confidence = min(4, 0.3 * 5) = 1.5/5
          (adjust for low R²)
```

**3. Linear Scaling Assumption**:
```
❌ BAD:  10% lever change → predict 10% metric change
          (actual: often 25x weaker due to nonlinear relationships)

✅ GOOD: Use sensitivity slope to scale predictions
          e.g., sensitivity=0.008 → predict 0.8% change per 10% lever
```

**4. Algorithm Weight vs Pattern Characteristic** (iteration 2026-01-19-005):
```
❌ BAD:  Change kSyncopationWidth to reduce syncopation metric
          (Metric measures hit POSITIONS, not algorithm weights!)

✅ GOOD: To reduce syncopation, investigate the full causal chain:
          - Note: MINIMAL zone eligibility ALREADY constrains to strong beats!
          - Check FLAVOR param (if > 0.6, adds syncopation mask back)
          - Check multi-voice metric calculation (shimmer/aux may add syncopation)
          - Increase hit budgets (more hits → better statistical coverage)
```
The syncopation METRIC measures tension from weak-beat hits, not the syncopation ALGORITHM's influence. A pattern can be 100% euclidean and still be highly syncopated if hits land on weak beats.

**Open question**: WHY does syncopation = 1.0 when eligibility masks already constrain to strong beats? This needs root cause investigation, not more lever changes.

**5. Regularity Misconception** (iteration 2026-01-19-003):
```
❌ BAD:  Assume fewer hits = more regular patterns
          (Regularity = gap uniformity, not sparseness!)

✅ GOOD: Regularity requires uniform gap spacing
          euclidean(16,4) = gaps [4,4,4,4] = perfectly regular
          euclidean(16,3) = gaps [5,5,6] = non-uniform = less regular
```
For euclidean(n,k), regularity is highest when n/k is an integer.

**6. Euclidean k vs HitBudget Confusion** (iteration 2026-01-19-005):
```
❌ BAD:  Debug shows k=4, expect 4 hits in pattern
          (k is algorithm parameter, NOT hit count!)

✅ GOOD: Actual hit count = HitBudget.ComputeAnchorBudget()
          At ENERGY=0.10 (MINIMAL zone): budget=2 hits, not k=4
```
The euclidean k value is just an algorithm parameter for generating evenly-spaced candidate positions. The actual number of hits comes from HitBudget, which scales with ENERGY zone.

### Pre-Flight Checklist

Before proposing any change:
- [ ] Read `metrics/iteration-lessons.json` for similar failed attempts
- [ ] Verify lever parameter domain matches target zone domain
- [ ] Adjust bootstrap confidence for sensitivity R² < 0.5
- [ ] Scale predictions using sensitivity slope (not linear assumption)
- [ ] Check if similar goal failed recently - learn from mistakes!
- [ ] Verify causal chain: lever → algorithm weight → hit positions → metric
- [ ] Spot-check a single pattern BEFORE running full eval (catches issues early)
- [ ] Distinguish: Does this lever change WHERE hits land, or just WHICH algorithm decides?

### When to Consult Lessons

- **Step 1**: Load lessons alongside baseline/sensitivity
- **Step 4**: Before selecting levers, check for failed attempts
- **Step 4.5**: Validate parameter domain match (CRITICAL!)
- **Step 5**: Reference calibration lessons when making predictions
- **Step 13C**: After failure, update lessons.json with new insights
