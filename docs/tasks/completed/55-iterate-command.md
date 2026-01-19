---
id: 55
slug: iterate-command
title: "Iteration Command System for Hill-Climbing"
status: completed
created_date: 2026-01-18
updated_date: 2026-01-18
completed_date: 2026-01-18
branch: feature/iterate-command
commits:
  - 392d003  # Initial implementation
  - 0e4d38c  # Code review fixes
spec_refs:
  - "docs/SDD_WORKFLOW.md"
depends_on:
  - 63  # Sensitivity analysis (provides lever data)
  - 61b # PR metrics comparison (CI infrastructure)
---

# Task 55: Iteration Command System for Hill-Climbing

## Objective

Create an `/iterate` command that enables semi-autonomous algorithm improvement through weight adjustments. This is a **single-pass** iteration system - one Claude invocation proposes changes, runs evals, and creates a PR if metrics improve.

## Context

### Current State

- Manual code changes to algorithm weights
- No structured way to propose and evaluate changes
- No tracking of iteration history
- Claude can respond to @mentions but no iteration workflow

### Target State

- `/iterate` slash command triggers single-pass improvement cycle
- Claude analyzes metrics, proposes weight changes, evaluates
- All changes logged to iteration history with git refs
- Automated PR creation with before/after metrics

### Simplified Workflow

> **Note**: This task was simplified from the original design based on design review feedback.
> The original multi-agent Designer/Critic split was removed because `claude-code-action@v1`
> runs once per trigger with no mechanism for multi-step orchestration.

```
1. User invokes `/iterate` with goal (e.g., "improve syncopation in wild zone")
2. Claude reads current baseline metrics from metrics/baseline.json
3. Claude analyzes sensitivity data to identify high-impact levers
4. Claude proposes and applies weight changes to algorithm_config.h
5. Claude runs evals, captures new metrics
6. Claude compares before/after metrics
7. If improved (>= 5% on target, no regressions > 2%): Create PR
8. If not improved: Report findings, suggest manual alternatives
9. Log iteration to docs/design/iterations/
```

**Key simplification**: No retry loop. If the first attempt doesn't improve metrics,
the user can manually re-trigger with different parameters or goals.

## Subtasks

### Command Infrastructure
- [x] Create `/iterate` command in `.claude/commands/iterate.md`
- [x] Define iteration goal format and validation
- [x] Create iteration state tracking (in-progress, success, failed)
- [x] Add iteration ID generation (`scripts/iterate/generate-iteration-id.js`)

### Single-Pass Analysis & Proposal
- [x] Read baseline metrics from `metrics/baseline.json` (in command prompt)
- [x] Read sensitivity matrix from Task 63 output (in command prompt)
- [x] Use bootstrap lever table (Task 56) if sensitivity not yet run (in command prompt)
- [x] Analyze goal to identify target metric(s) (in command prompt)
- [x] Propose weight changes based on lever recommendations (in command prompt)
- [x] Apply changes to `inc/algorithm_config.h` (in command prompt)

### Evaluation
- [x] Run evals - documented in command workflow
- [x] Capture new metrics - documented in command workflow
- [x] Compare before/after metrics (`scripts/iterate/compare-metrics.js`)
- [x] Calculate deltas and percentage changes (`scripts/iterate/compare-metrics.js`)

### Success Criteria Check
- [x] Target metric improved >= 5% (relative) - in compare-metrics.js
- [x] No other metric regressed > 2% (relative) - in compare-metrics.js
- [x] All tests pass - documented in command workflow
- [x] No new warnings - documented in command workflow

### Iteration Logging
- [x] Create `docs/design/iterations/` directory structure
- [x] Define iteration log format (`docs/design/iterations/TEMPLATE.md`)
- [x] Log all proposed changes, even failed ones (in command prompt)
- [x] Include git commit refs (in command prompt)
- [x] Track cumulative improvement over time (`docs/design/iterations/README.md`)

### PR Integration
- [x] Auto-create feature branch for iteration (in command prompt)
- [x] Generate PR with iteration summary (in command prompt)
- [x] Include metric diff table in PR description (in command prompt)
- [x] Add iteration ID and goal to PR title (in command prompt)

### Auto-Suggest Mode
- [x] Implement `/iterate auto` to auto-detect weakest Pentagon metric
- [x] Analyze current metrics to identify improvement opportunities
- [x] Prioritize metrics with largest delta from target
- [x] Suggest specific goal based on metric gaps

### Tests
- [x] Test iteration command parsing - manual test with `/iterate auto`
- [x] Test proposal format validation - documented in command
- [x] Test metric comparison logic - verified with compare-metrics.js
- [x] Test auto-suggest goal detection - documented in command
- [x] All tests pass - n/a (no new code tests needed)

## Acceptance Criteria

- [x] `/iterate "goal"` triggers single-pass workflow
- [x] `/iterate auto` suggests goal based on weakest metric
- [x] Weight changes applied to `inc/algorithm_config.h`
- [x] Evals run and metrics compared
- [x] Failed iterations logged with reasons (no retry loop)
- [x] Successful iterations create PRs
- [x] Iteration history maintained in `docs/design/iterations/`
- [x] Git refs included in all iteration logs

## What Was Removed (Compared to Original Design)

The following features were removed from this task scope based on design review:

1. **Designer/Critic Agent Split** - The original design had separate "Designer" and "Critic" agents. This is not implementable with current `claude-code-action@v1` which runs once per trigger.

2. **Retry Loop** - The original "max 3 attempts" retry logic requires multi-step orchestration. Instead, users can manually re-trigger `/iterate` with refined goals.

3. **Listening Test Integration** - Moved to future task. Requires human-in-the-loop timing that doesn't fit automated workflow.

4. **Version Tags** - Deferred to Task 61 (regression detection) which handles baseline management.

## Implementation Notes

### Files Created

Command:
- `.claude/commands/iterate.md` - Main iterate command

Logging:
- `docs/design/iterations/README.md` - Iteration log index
- `docs/design/iterations/TEMPLATE.md` - Iteration log template

Scripts:
- `scripts/iterate/compare-metrics.js` - Compare before/after metrics
- `scripts/iterate/generate-iteration-id.js` - Generate unique iteration IDs

### Iteration Log Format

```markdown
---
iteration_id: 2026-01-18-001
goal: "Improve syncopation in wild zone"
status: success | failed
started_at: 2026-01-18T10:30:00Z
completed_at: 2026-01-18T10:45:00Z
branch: feature/iterate-2026-01-18-001
commit: abc123def
pr: "#123"
---

# Iteration 2026-01-18-001: Improve Syncopation in Wild Zone

## Goal
Increase syncopation score for SHAPE > 0.7 patterns without regressing
other metrics.

## Baseline Metrics (from metrics/baseline.json)
| Metric | Stable | Syncopated | Wild |
|--------|--------|------------|------|
| Syncopation | 0.15 | 0.42 | 0.48 |
| ...

## Proposed Changes
Based on sensitivity analysis, increasing syncopationCenter should improve
syncopation scores in the wild zone.

Changes to `inc/algorithm_config.h`:
- `kSyncopationCenter`: 0.5 → 0.55

## Result Metrics
| Metric | Stable | Syncopated | Wild | Delta |
|--------|--------|------------|------|-------|
| Syncopation | 0.15 | 0.43 | 0.58 | +0.10 |
| ...

## Evaluation
- Target metric (syncopation/wild): +20.8% ✓
- Max regression: density/stable -1.2% ✓
- Tests: PASS ✓

## Decision
SUCCESS - PR created.
```

### Command Invocation

User triggers via:
- Issue comment: `@claude /iterate improve syncopation in wild zone`
- Issue body: Create issue with `@claude /iterate` in body

### Success Criteria (Precise Definition)

```
SUCCESS if:
  target_metric_delta >= 0.05 * baseline_value  (5% relative improvement)
  AND max(other_metric_regressions) <= 0.02 * baseline_value  (2% tolerance)
  AND all_tests_pass
  WHERE baseline = metrics/baseline.json (main branch metrics)
```

## Test Plan

1. Create test iteration:
   ```bash
   # Manually test the iterate prompt
   claude "/iterate improve syncopation in wild zone"
   ```
2. Verify baseline reading works
3. Verify weight changes are valid syntax
4. Verify evals run and metrics captured
5. Verify PR creation with correct format

## Dependencies

- **Task 63** (Parameter Sensitivity): Provides lever recommendations
- **Task 61b** (PR Metrics Comparison): CI infrastructure for PR evaluation
- **Task 56** (Weight-Based Blending): Bootstrap lever table for initial iterations

## Estimated Effort

4-5 hours (simplified from original 6-8h estimate)

---

## Post-Implementation Code Review (2026-01-18)

### Fixes Applied

| Issue | File | Fix | Commit |
|-------|------|-----|--------|
| Wrong task path in workflow comment | `.github/workflows/claude.yml:49` | Changed `active/` → `completed/` | `0e4d38c` |
| Inconsistent iteration ID generation | `.claude/commands/iterate.md:99` | Use `generate-iteration-id.js` instead of inline shell | `0e4d38c` |
| Direction-unaware regression detection | `scripts/iterate/compare-metrics.js` | Added direction-aware logic using `metricDefinitions.targetByZone` | `0e4d38c` |

### Direction-Aware Regression Logic

The compare-metrics.js script now uses target ranges from baseline.json to determine if a change is a regression:

- If metric is **above** target max → decrease is improvement (not regression)
- If metric is **below** target min → increase is improvement
- Example: `regularity` in wild zone (0.606) is above target (0.12-0.48), so a decrease would be good

### Deferred Items (Future Enhancements)

1. **Automated unit tests for scripts** - compare-metrics.js and generate-iteration-id.js have no automated tests. Manual verification was performed. Consider adding Jest tests in future.

2. **`--json` output mode** - compare-metrics.js only outputs markdown. A JSON mode would enable downstream automation.

3. **`--dry-run` flag** - The iterate command could benefit from a dry-run mode that shows proposed changes without applying them.

4. **Auto-generated README index** - The `docs/design/iterations/README.md` index table must be manually maintained. A script could auto-populate it.

5. **Concurrent iteration ID collision** - generate-iteration-id.js has no locking for concurrent execution. Unlikely in practice but noted.
