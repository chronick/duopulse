---
id: 61
slug: regression-detection
title: "Automated Regression Detection in CI"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/regression-detection
spec_refs: []
depends_on:
  - 55  # Iteration command system
---

# Task 61: Automated Regression Detection in CI

## Objective

Implement automated regression detection in CI that flags PRs when Pentagon metrics regress beyond acceptable thresholds, preventing unintentional quality degradation.

## Context

### Current State

- Evals run but results not enforced
- No automatic comparison with baseline
- Manual review of metrics required
- Regressions can slip through

### Target State

- CI stores baseline metrics on main branch
- Every PR runs evals and compares to baseline
- Regressions beyond threshold fail CI
- Detailed regression report in PR comment
- Configurable thresholds per metric

## Subtasks

### Baseline Management
- [ ] Store baseline metrics in `metrics/baseline.json`
- [ ] Add GitHub Action to compare PR metrics to baseline
- [ ] Define regression thresholds (default: -2% per metric)
- [ ] Generate PR comment with metric diff table
- [ ] Fail CI when regression exceeds threshold
- [ ] Add manual override label for intentional regressions
- [ ] Update baseline after merge to main

### Rollback Capability
- [ ] Track "last known good" baseline with git tag
- [ ] Detect consecutive regression PRs (2+ in a row)
- [ ] Auto-suggest rollback when multiple iterations regress
- [ ] Create `/rollback` command to revert to last good baseline
- [ ] Preserve iteration history even after rollback
- [ ] Update timeline with rollback events

### Tests
- [ ] All tests pass

## Acceptance Criteria

- [ ] Baseline metrics stored and versioned
- [ ] PR evals compared to baseline automatically
- [ ] Regressions beyond threshold fail CI
- [ ] PR comment shows clear metric diff
- [ ] Override mechanism works for intentional changes
- [ ] Baseline updates after main merge
- [ ] Works with iteration PRs (different thresholds)
- [ ] Consecutive regressions trigger rollback suggestion
- [ ] `/rollback` reverts to last known good state

## Estimated Effort

3-4 hours
