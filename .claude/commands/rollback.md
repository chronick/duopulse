---
description: Revert algorithm configuration to last known good baseline
---

# Rollback to Last Known Good Baseline

You are performing a **rollback** to revert the algorithm configuration to the last known good baseline. This is a safety mechanism when iterations have caused regressions.

## Workflow

### Step 1: Check Prerequisites

Read the baseline file to understand current state:

```bash
cat metrics/baseline.json
```

Extract and verify these fields:
- `last_good` - Must exist and not be null
- `last_good.commit` - The commit hash to revert to
- `last_good.tag` - The version tag (e.g., "v1.0.0")
- `last_good.timestamp` - When it was marked as good
- `last_good.metrics` - The metrics at that baseline
- `consecutive_regressions` - Number of failed iterations since last good

**If `last_good` is null or missing**:
Report this error and stop:
```
ERROR: No last known good baseline exists. Cannot rollback.

The baseline.json file does not have a 'last_good' field set.
This happens when:
1. No baseline has ever been marked as "good"
2. The project is newly initialized

To establish a baseline, run a successful iteration or manually set
last_good in metrics/baseline.json.
```

**If `last_good` exists**, display the rollback info:
```
## Rollback Information

### Last Known Good Baseline
- Tag: {last_good.tag}
- Commit: {last_good.commit}
- Timestamp: {last_good.timestamp}
- Consecutive Regressions: {consecutive_regressions}

### Current State
- Commit: {current_commit}
- Tag: {current_tag or "none"}
```

If `consecutive_regressions >= 2`, note:
```
NOTE: {consecutive_regressions} consecutive regressions detected.
Rollback is recommended.
```

### Step 2: User Confirmation

Display what will happen and request confirmation:

```
## Rollback Confirmation Required

This will:
1. Create a rollback branch
2. Revert inc/algorithm_config.h to commit {last_good.commit}
3. Run evals to verify metrics are restored
4. Create a PR for review

### What Changes
- FROM: Current algorithm config (commit {current_commit})
- TO: Last good algorithm config (commit {last_good.commit}, tag {last_good.tag})

### This Will NOT
- Delete iteration logs (history is preserved)
- Affect any other files
- Push directly to main

Do you want to proceed with the rollback? (yes/no)
```

**Wait for user response.**

- If user says "yes", "y", or "proceed": Continue to Step 3
- If user says "no", "n", or "cancel": Exit with message:
  ```
  Rollback cancelled by user. No changes made.
  ```

### Step 3: Create Rollback Branch

```bash
# Get the tag for branch naming
TAG=$(jq -r '.last_good.tag // "unknown"' metrics/baseline.json)

# Create rollback branch
git checkout -b rollback/baseline-${TAG}
```

### Step 4: Revert Algorithm Config

Restore the algorithm_config.h from the last good commit:

```bash
# Get the last good commit hash
LAST_GOOD_COMMIT=$(jq -r '.last_good.commit' metrics/baseline.json)

# Restore the file from that commit
git show ${LAST_GOOD_COMMIT}:inc/algorithm_config.h > inc/algorithm_config.h
```

Verify the file changed:

```bash
# Show the diff
git diff inc/algorithm_config.h
```

Display the diff to the user:
```
## Changes to algorithm_config.h

{diff output}
```

If no diff (file unchanged):
```
WARNING: No changes detected in algorithm_config.h.
The current config may already match the last good baseline.
Continue anyway? (yes/no)
```

### Step 5: Verify Build

Ensure the reverted config compiles:

```bash
make pattern-viz
```

If build fails, report error and offer to abort:
```
ERROR: Build failed after reverting algorithm_config.h.

This may indicate:
- The last_good commit is incompatible with current code
- Other files need to be reverted as well

Options:
1. Abort rollback and restore current config
2. Continue anyway (not recommended)

Choose (1/2):
```

### Step 6: Run Evals

Run the evaluation suite to capture metrics:

```bash
# Generate patterns and evaluate
make evals-generate
make evals-evaluate
```

Compare the new metrics to the last_good baseline:

```
## Metrics Verification

| Metric | Last Good | After Rollback | Delta | Status |
|--------|-----------|----------------|-------|--------|
| syncopation | {lg_val} | {new_val} | {delta}% | {OK/WARN} |
| density | {lg_val} | {new_val} | {delta}% | {OK/WARN} |
| velocityRange | {lg_val} | {new_val} | {delta}% | {OK/WARN} |
| voiceSeparation | {lg_val} | {new_val} | {delta}% | {OK/WARN} |
| regularity | {lg_val} | {new_val} | {delta}% | {OK/WARN} |
| composite | {lg_val} | {new_val} | {delta}% | {OK/WARN} |

Tolerance: +/- 1%
```

**Verification criteria:**
- All metrics within 1% of last_good values: PASS
- Any metric > 1% different: WARN (may indicate seed or environmental variance)

If metrics are not restored within tolerance:
```
WARNING: Metrics do not match expected last_good baseline.

This may indicate:
- Changes in other code affecting pattern generation
- Different random seeds or test conditions
- The stored last_good metrics were captured differently

The file revert was successful. Do you want to:
1. Proceed with PR anyway
2. Abort rollback

Choose (1/2):
```

### Step 7: Commit Changes

```bash
# Stage the reverted file
git add inc/algorithm_config.h

# Commit with descriptive message
git commit -m "rollback: Revert to baseline ${TAG}

Reverted algorithm_config.h to last known good baseline.

- From commit: ${CURRENT_COMMIT}
- To commit: ${LAST_GOOD_COMMIT}
- Tag: ${TAG}
- Reason: ${consecutive_regressions} consecutive regressions

Metrics after rollback match baseline within tolerance."
```

### Step 8: Create PR

Create a pull request with full context:

```bash
gh pr create \
  --title "rollback: Revert to baseline ${TAG}" \
  --body "## Rollback PR

### Reason
${consecutive_regressions} consecutive iteration regressions triggered this rollback.

### Changes
Reverted \`inc/algorithm_config.h\` to commit ${LAST_GOOD_COMMIT} (tag: ${TAG}).

### Metrics Verification
Ran evals after revert. Metrics match last_good baseline within 1% tolerance.

| Metric | Expected | Actual | Delta |
|--------|----------|--------|-------|
| syncopation | X | Y | Z% |
| ... | ... | ... | ... |

### History Preserved
Iteration logs in \`docs/design/iterations/\` are NOT deleted.
Failed iterations can be analyzed for future reference.

### Next Steps
After merge:
1. Update \`metrics/baseline.json\` to reset \`consecutive_regressions\` to 0
2. Investigate why recent iterations caused regressions
3. Consider adjusting iteration parameters or sensitivity matrix" \
  --label "rollback" \
  --label "metrics"
```

If `gh` CLI is not available or fails:
```
PR creation failed. Please create PR manually:

Branch: rollback/baseline-${TAG}
Title: rollback: Revert to baseline ${TAG}
Labels: rollback, metrics

Include the metrics comparison table in the PR description.
```

### Step 9: Log Rollback Event

Create a rollback log entry:

```bash
TIMESTAMP=$(date +%Y-%m-%d-%H%M)
```

Create file `docs/design/iterations/rollback-${TIMESTAMP}.md`:

```markdown
---
type: rollback
timestamp: ${TIMESTAMP}
from_commit: ${CURRENT_COMMIT}
to_commit: ${LAST_GOOD_COMMIT}
to_tag: ${TAG}
branch: rollback/baseline-${TAG}
pr: ${PR_NUMBER or "pending"}
---

# Rollback: ${TIMESTAMP}

## Reason

Rollback triggered after ${consecutive_regressions} consecutive iteration regressions.

## State Before Rollback

- Commit: ${CURRENT_COMMIT}
- Consecutive regressions: ${consecutive_regressions}

## Reverted To

- Commit: ${LAST_GOOD_COMMIT}
- Tag: ${TAG}
- Original timestamp: ${last_good.timestamp}

## Metrics Comparison

### Expected (Last Good)
| Metric | Total | Stable | Syncopated | Wild |
|--------|-------|--------|------------|------|
| syncopation | X | X | X | X |
| ... | ... | ... | ... | ... |

### After Rollback
| Metric | Total | Stable | Syncopated | Wild |
|--------|-------|--------|------------|------|
| syncopation | X | X | X | X |
| ... | ... | ... | ... | ... |

## Files Changed

- `inc/algorithm_config.h` - Reverted to ${LAST_GOOD_COMMIT}

## PR

${PR_URL or "Pending creation"}

## Notes

{Any observations about why the regressions occurred}

---

*Generated by `/rollback` command on ${TIMESTAMP}*
```

Commit the log file:
```bash
git add docs/design/iterations/rollback-${TIMESTAMP}.md
git commit --amend --no-edit
git push -u origin rollback/baseline-${TAG}
```

### Step 10: Report Summary

```
## Rollback Summary

**Status**: COMPLETE
**Branch**: rollback/baseline-${TAG}
**PR**: ${PR_URL}

### What Was Done
1. Created rollback branch from current HEAD
2. Reverted inc/algorithm_config.h to commit ${LAST_GOOD_COMMIT}
3. Verified build compiles
4. Ran evals - metrics match baseline within tolerance
5. Created PR with labels: rollback, metrics
6. Logged rollback event to docs/design/iterations/

### Metrics After Rollback
| Metric | Value | vs Last Good |
|--------|-------|--------------|
| syncopation | X | +/-Y% |
| ... | ... | ... |

### Next Steps
1. Review and merge PR: ${PR_URL}
2. After merge, reset consecutive_regressions in baseline.json
3. Analyze failed iterations to understand regression cause
4. Consider adjusting lever bounds or sensitivity thresholds

### Preserved History
Iteration logs in docs/design/iterations/ are preserved for analysis.
No historical data was deleted.
```

## Error Handling

### last_good is null
```
ERROR: No last known good baseline exists.

The metrics/baseline.json file has last_good: null.
This means no baseline has been marked as "good" yet.

To establish a last_good baseline:
1. Run an iteration that passes all metrics
2. Or manually set last_good in baseline.json to current state

Cannot proceed with rollback.
```

### Git command fails
```
ERROR: Git command failed: {command}
Output: {error_output}

Rollback aborted. No changes have been committed.
Your working directory may have uncommitted changes.

To clean up:
  git checkout inc/algorithm_config.h
  git checkout main
  git branch -D rollback/baseline-${TAG}
```

### Metrics don't restore
```
WARNING: Post-rollback metrics differ from expected baseline.

| Metric | Expected | Actual | Delta |
|--------|----------|--------|-------|
| {metric} | {expected} | {actual} | {delta}% |

This may be acceptable if:
- Delta is small (< 5%)
- Changes are in other code affecting generation
- Test conditions differ from original baseline capture

Options:
1. Proceed anyway - file revert was successful
2. Abort - investigate differences first

Choose (1/2):
```

## Important Notes

1. **User confirmation required**: Always wait for explicit "yes" before making changes
2. **History preserved**: Iteration logs are never deleted
3. **PR workflow**: Never push directly to main
4. **Verification**: Always run evals after revert to confirm restoration
5. **Logging**: Every rollback is documented in iterations directory
