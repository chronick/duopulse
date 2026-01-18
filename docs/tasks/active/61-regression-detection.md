---
id: 61
slug: regression-detection
title: "Full Regression Detection + Rollback"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/regression-detection
spec_refs: []
depends_on:
  - 61a  # Baseline infrastructure
  - 61b  # PR metrics comparison
  - 55   # Iteration command system
---

# Task 61: Full Regression Detection + Rollback

## Objective

Build on the baseline infrastructure (Task 61a) and PR comparison (Task 61b) to add rollback capability, consecutive regression detection, and "last known good" baseline management.

## Context

### Prerequisites (Split Tasks)

> **Note**: This task was split during design review (2026-01-18). The foundational
> work is now in two prerequisite tasks:
>
> - **Task 61a**: Baseline Infrastructure - Creates `metrics/baseline.json` and `make baseline`
> - **Task 61b**: PR Metrics Comparison - CI workflow that compares PR metrics to baseline
>
> This task (61) focuses on the advanced features that build on that foundation.

### Current State (After 61a + 61b)

- `metrics/baseline.json` exists with main branch metrics
- PRs show metric comparison and fail on regression > 2%
- Basic infrastructure in place

### Target State

- "Last known good" baseline tagging system
- Consecutive regression detection (alerts after 2+ in a row)
- `/rollback` command to revert to last good baseline
- Iteration history preserved after rollback

## Subtasks

### Last Known Good Tracking
- [ ] Add git tag for each baseline update (e.g., `baseline-v5.2.1`)
- [ ] Track baseline commit hash in `metrics/baseline.json`
- [ ] Store "last_good" pointer alongside current baseline
- [ ] Update last_good only on successful iteration (no regressions)

### Consecutive Regression Detection
- [ ] Track regression count in baseline.json metadata
- [ ] Detect 2+ consecutive regression PRs
- [ ] Add warning comment when consecutive regressions detected
- [ ] Suggest rollback when threshold exceeded

### Rollback Command
- [ ] Create `/rollback` prompt in `.claude/prompts/`
- [ ] Implement rollback to last known good baseline
- [ ] Create rollback branch with reverted changes
- [ ] Generate rollback PR with explanation
- [ ] Preserve iteration history (log rollback event, don't delete logs)

### Rollback Safety
- [ ] Require explicit confirmation for rollback
- [ ] Prevent rollback during active iteration
- [ ] Log rollback events to iteration history
- [ ] Notify on timeline (Task 58) when rollback occurs

### Tests
- [ ] Test baseline tagging
- [ ] Test consecutive regression detection
- [ ] Test rollback command parsing
- [ ] Test rollback preserves history
- [ ] All tests pass

## Acceptance Criteria

- [ ] Git tags created for each baseline update
- [ ] Consecutive regressions (2+) trigger alert
- [ ] `/rollback` reverts to last known good state
- [ ] Rollback creates PR for review (not direct push)
- [ ] Iteration history preserved after rollback
- [ ] Timeline shows rollback events

## Implementation Notes

### Baseline.json Format (Extended)

```json
{
  "version": "1.0.0",
  "generated_at": "2026-01-18T12:00:00Z",
  "commit": "abc123def",
  "tag": "baseline-v5.2.1",
  "last_good": {
    "commit": "xyz789abc",
    "tag": "baseline-v5.2.0",
    "timestamp": "2026-01-17T10:00:00Z"
  },
  "consecutive_regressions": 0,
  "metrics": {
    "overall": { ... },
    "byZone": { ... }
  }
}
```

### Rollback Workflow

```
1. User invokes `/rollback` or system auto-suggests after consecutive regressions
2. Claude reads current baseline and last_good pointer
3. Claude creates branch `rollback/baseline-v5.2.0`
4. Claude reverts algorithm_config.h to last_good commit
5. Claude runs evals to verify metrics restored
6. Claude creates PR "Rollback to baseline-v5.2.0"
7. User reviews and merges
8. Baseline updated, consecutive_regressions reset to 0
```

### Files to Create

- `.claude/prompts/rollback.md` - Rollback command

### Files to Modify

- `metrics/baseline.json` - Extended format
- `.github/workflows/pages.yml` - Baseline tagging on deploy
- `docs/design/iterations/` - Rollback event logging

## Test Plan

1. Simulate consecutive regressions:
   ```bash
   # Manually modify baseline.json to have consecutive_regressions: 2
   # Trigger CI and verify warning appears
   ```
2. Test rollback command:
   ```bash
   claude "/rollback"
   # Verify PR created with correct revert
   ```
3. Verify history preservation after rollback

## Estimated Effort

2-3 hours (reduced from original 3-4h since infrastructure is in 61a/61b)
