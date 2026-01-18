---
id: 55
slug: iterate-command
title: "Iteration Command System for Hill-Climbing"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/iterate-command
spec_refs:
  - "docs/SDD_WORKFLOW.md"
---

# Task 55: Iteration Command System for Hill-Climbing

## Objective

Create an `/iterate` command infrastructure that enables semi-autonomous algorithm improvement through weight adjustments, design iterations, and tracked changes. This is the core orchestration layer for hill-climbing optimization.

## Context

### Current State

- Manual code changes to algorithm weights
- No structured way to propose and evaluate changes
- No tracking of iteration history
- Claude can respond to @mentions but no iteration workflow

### Target State

- `/iterate` slash command triggers structured improvement cycle
- Designer agent proposes weight/parameter changes
- Critic agent evaluates proposals against Pentagon metrics
- All changes logged to iteration history with git refs
- Automated PR creation with before/after metrics

### Hill-Climbing Workflow

```
1. User invokes `/iterate` with goal (e.g., "improve syncopation in wild zone")
2. System runs current evals, captures baseline metrics
3. Designer agent analyzes spec + metrics, proposes changes
4. Changes applied to a feature branch
5. Evals re-run, new metrics captured
6. Critic agent compares before/after, approves or rejects
7. If approved: PR created with metric diff
8. If rejected: Designer tries alternative approach (max 3 attempts)
9. Iteration logged to docs/design/iterations/
```

## Subtasks

### Command Infrastructure
- [ ] Create `/iterate` prompt template in `.claude/prompts/`
- [ ] Define iteration goal format and validation
- [ ] Create iteration state tracking (in-progress, success, failed)
- [ ] Add iteration ID generation (timestamp-based)

### Designer Agent
- [ ] Define designer role and capabilities
- [ ] Create weight change proposal format
- [ ] Implement spec-aware change suggestions
- [ ] Add constraint validation (RT-safe, bounds checking)

### Critic Agent
- [ ] Define critic role and evaluation criteria
- [ ] Implement metric comparison (before/after)
- [ ] Define success thresholds (5% improvement, no regressions)
- [ ] Add human-readable critique output

### Iteration Logging
- [ ] Create `docs/design/iterations/` directory structure
- [ ] Define iteration log format (YAML frontmatter + markdown)
- [ ] Log all proposed changes, even rejected ones
- [ ] Include git commit refs and version tags
- [ ] Track cumulative improvement over time

### PR Integration
- [ ] Auto-create feature branch for iteration
- [ ] Generate PR with iteration summary
- [ ] Include metric diff table in PR description
- [ ] Add iteration timeline to PR body

### GitHub Actions Integration
- [ ] Update `claude.yml` to handle `/iterate` command
- [ ] Add iteration-specific permissions
- [ ] Ensure evals run in CI before PR merge

### Auto-Suggest Mode (Metric-Driven Goal Selection)
- [ ] Implement `/iterate auto` to auto-detect weakest Pentagon metric
- [ ] Analyze current metrics to identify improvement opportunities
- [ ] Suggest specific goals based on metric gaps
- [ ] Prioritize metrics with largest delta from target
- [ ] Allow user to confirm or override suggested goal

### Listening Test Integration
- [ ] Integrate with existing audio player (tools/evals/public/audio-engine.js)
- [ ] Generate A/B comparison patterns (before/after weights)
- [ ] Include subjective listening notes in iteration log
- [ ] Optional: prompt user to rate patterns 1-5 before finalizing

### Tests
- [ ] Test iteration command parsing
- [ ] Test designer proposal format validation
- [ ] Test critic evaluation logic
- [ ] Test auto-suggest goal detection
- [ ] All tests pass

## Acceptance Criteria

- [ ] `/iterate` command triggers structured workflow
- [ ] `/iterate auto` suggests goal based on weakest metric
- [ ] Designer proposes valid weight changes
- [ ] Critic evaluates proposals with metrics
- [ ] Rejected proposals logged with reasons
- [ ] Successful iterations create PRs
- [ ] Iteration history maintained in `docs/design/iterations/`
- [ ] Git refs included in all iteration logs
- [ ] Version tags (patch/rev) tracked
- [ ] Claude responds to @mentions with iteration status
- [ ] A/B audio comparison available for listening tests

## Implementation Notes

### Files to Create

Command:
- `.claude/prompts/iterate.md` - Main iterate command
- `.claude/prompts/iterate-designer.md` - Designer agent prompt
- `.claude/prompts/iterate-critic.md` - Critic agent prompt

Logging:
- `docs/design/iterations/README.md` - Iteration log index
- `docs/design/iterations/TEMPLATE.md` - Iteration log template

Scripts:
- `scripts/iterate/capture-baseline.sh` - Capture current metrics
- `scripts/iterate/compare-metrics.sh` - Compare before/after
- `scripts/iterate/create-iteration-pr.sh` - Generate PR

### Iteration Log Format

```markdown
---
iteration_id: 2026-01-18-001
goal: "Improve syncopation in wild zone"
status: success
started_at: 2026-01-18T10:30:00Z
completed_at: 2026-01-18T11:15:00Z
branch: feature/iterate-2026-01-18-001
commit: abc123def
pr: "#123"
---

# Iteration 2026-01-18-001: Improve Syncopation in Wild Zone

## Goal
Increase syncopation score for SHAPE > 0.7 patterns without regressing
other metrics.

## Baseline Metrics
| Metric | Stable | Syncopated | Wild |
|--------|--------|------------|------|
| Syncopation | 0.15 | 0.42 | 0.48 |
| ...

## Proposed Changes
1. Increase wild zone noise scale from 0.40 to 0.50
2. Adjust syncopation weight boost at high SHAPE

## Designer Rationale
[Analysis of why these changes should work...]

## Result Metrics
| Metric | Stable | Syncopated | Wild | Delta |
|--------|--------|------------|------|-------|
| Syncopation | 0.15 | 0.43 | 0.58 | +0.10 |
| ...

## Critic Evaluation
[Evaluation of changes, any concerns, approval status...]

## Decision
APPROVED - 10% improvement in target metric, no regressions.
```

### Command Invocation

User can trigger via:
- Comment: `@claude /iterate improve syncopation in wild zone`
- Issue: Create issue with `@claude /iterate` in body

### Success Criteria

An iteration is successful if:
1. Target metric improves by >= 5%
2. No other metric regresses by > 2%
3. All tests pass
4. No new warnings
5. Code review passes

## Test Plan

1. Create test iteration:
   ```bash
   # Simulate /iterate command
   ./scripts/iterate/test-iterate.sh "improve syncopation in wild zone"
   ```
2. Verify baseline capture works
3. Verify designer proposal is valid
4. Verify critic evaluation runs
5. Verify PR creation with correct format

## Estimated Effort

6-8 hours (complex orchestration, multi-agent coordination)
