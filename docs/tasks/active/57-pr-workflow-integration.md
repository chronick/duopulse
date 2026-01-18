---
id: 57
slug: pr-workflow-integration
title: "PR Workflow Integration for Claude Feedback"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/pr-workflow-integration
spec_refs:
  - "docs/SDD_WORKFLOW.md"
depends_on:
  - 55  # Iteration command system
---

# Task 57: PR Workflow Integration for Claude Feedback

## Objective

Enable full PR workflow integration where Claude can receive feedback via PR comments, respond to @mentions, and iterate on PRs based on user guidance. Support both task-only PRs and design-iteration PRs.

## Context

### Current State

- `claude.yml` workflow responds to @mentions on issues/PRs
- `claude-code-review.yml` provides automated code review
- No structured feedback loop for iteration PRs
- No differentiation between task PRs and design PRs

### Target State

- User can comment with feedback on any PR
- Claude responds to @mentions with context-aware assistance
- Task PRs follow standard review workflow
- Design-iteration PRs include metric comparisons
- Feedback triggers design adjustments when appropriate
- Full conversation history maintained in PR

### PR Types

1. **Task PRs**: Implement specific tasks from `docs/tasks/`
   - Standard review workflow
   - Focus on correctness and spec compliance
   - Merge when tests pass and approved

2. **Design-Iteration PRs**: Hill-climbing improvements
   - Include before/after metrics
   - Allow feedback-driven iteration
   - May spawn follow-up iterations
   - Merge when metrics improve

## Subtasks

### PR Type Detection
- [ ] Add PR template for task PRs (`.github/PULL_REQUEST_TEMPLATE/task.md`)
- [ ] Add PR template for iteration PRs (`.github/PULL_REQUEST_TEMPLATE/iteration.md`)
- [ ] Detect PR type from labels or template usage
- [ ] Display appropriate workflow guidance in PR description

### Feedback Handling
- [ ] Parse user feedback comments for actionable items
- [ ] Categorize feedback: approve, request changes, suggest improvements
- [ ] Handle multi-turn conversations in PR comments
- [ ] Maintain context across comment threads

### Task PR Workflow
- [ ] Standard code review via `claude-code-review.yml`
- [ ] Respond to specific code questions
- [ ] Suggest fixes for test failures
- [ ] Update PR description with task checklist progress

### Iteration PR Workflow
- [ ] Include metric diff table in PR description
- [ ] Respond to metric-related questions
- [ ] Propose alternative approaches when metrics regress
- [ ] Track iteration attempts in PR body

### @Mention Handling
- [ ] Respond to `@claude` mentions in PR comments
- [ ] Support commands: `/status`, `/retry`, `/explain`, `/compare`
- [ ] Provide context-aware responses based on PR type
- [ ] Link to relevant tasks/iterations

### GitHub Actions Updates
- [ ] Update `claude.yml` for enhanced PR interaction
- [ ] Add PR-specific permissions (write access for comments)
- [ ] Handle PR review events (approve, request changes)
- [ ] Trigger evals on PR updates for iteration PRs

### Tests
- [ ] Test PR type detection
- [ ] Test feedback parsing
- [ ] Test @mention responses
- [ ] All tests pass

## Acceptance Criteria

- [ ] Claude responds to @mentions on PRs within 2 minutes
- [ ] Task PRs show task checklist in description
- [ ] Iteration PRs show metric diff table
- [ ] User feedback triggers appropriate responses
- [ ] Conversation history maintained in PR
- [ ] PR type correctly detected from template/labels
- [ ] Iteration PRs can spawn follow-up iterations
- [ ] All existing tests pass

## Implementation Notes

### Files to Create

Templates:
- `.github/PULL_REQUEST_TEMPLATE/task.md`
- `.github/PULL_REQUEST_TEMPLATE/iteration.md`

Prompts:
- `.claude/prompts/pr-respond.md` - Handle PR comments
- `.claude/prompts/pr-status.md` - Report PR status
- `.claude/prompts/pr-compare.md` - Compare metrics

### Files to Modify

- `.github/workflows/claude.yml` - Enhanced PR handling
- `.github/workflows/claude-code-review.yml` - Iteration awareness

### Task PR Template

```markdown
## Task PR: [Task Title]

**Task ID**: #XX
**Task File**: `docs/tasks/active/XX-task-slug.md`

## Summary
<!-- Brief description of changes -->

## Checklist
<!-- Auto-populated from task file -->
- [ ] Subtask 1
- [ ] Subtask 2
- [ ] All tests pass

## Test Plan
<!-- How to verify changes -->

## Related
- Task: #XX
- Spec: docs/specs/main.md#section

---
Generated with [Claude Code](https://claude.ai/code)
```

### Iteration PR Template

```markdown
## Iteration PR: [Goal]

**Iteration ID**: YYYY-MM-DD-NNN
**Branch**: feature/iterate-YYYY-MM-DD-NNN

## Goal
<!-- What improvement was targeted -->

## Metric Comparison

| Metric | Before | After | Delta |
|--------|--------|-------|-------|
| Syncopation (wild) | 0.48 | 0.58 | +0.10 |
| ... | ... | ... | ... |

## Changes
<!-- List of code changes -->

## Designer Rationale
<!-- Why these changes were proposed -->

## Critic Evaluation
<!-- Evaluation results -->

## Status
- [ ] Metrics improved >= 5%
- [ ] No regressions > 2%
- [ ] All tests pass
- [ ] Code review approved

---
Iteration #N of hill-climbing session
```

### PR Comment Commands

| Command | Description |
|---------|-------------|
| `@claude /status` | Report PR status and remaining work |
| `@claude /retry` | Retry failed iteration with new approach |
| `@claude /explain` | Explain current changes in detail |
| `@claude /compare` | Show detailed metric comparison |
| `@claude /approve` | Mark iteration as approved (owner only) |

### Conversation Example

```
User: @claude This looks good but syncopation dropped slightly. Can you try
      a different approach that preserves syncopation?

Claude: I see the syncopation regression (-0.02 in stable zone). Let me
        try an alternative approach:

        Option A: Reduce euclidean rotation in stable zone
        Option B: Increase syncopation weight floor

        Which approach would you prefer, or shall I try both?

User: Try option A first.

Claude: Creating commit with option A changes...
        [runs evals]

        Results with Option A:
        - Syncopation (stable): 0.16 -> 0.17 (+0.01)
        - Target metric unchanged
        - No regressions

        This looks promising! Shall I push this update?
```

## Test Plan

1. Create test task PR
2. Verify template is applied
3. Comment with `@claude /status`
4. Verify response with task checklist
5. Create test iteration PR
6. Verify metric diff table
7. Comment with feedback
8. Verify feedback triggers appropriate action

## Estimated Effort

4-5 hours (GitHub Actions + prompt engineering)
