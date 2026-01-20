---
id: 57
slug: pr-workflow-integration
title: "PR Workflow Integration for Claude Feedback"
status: active
created_date: 2026-01-18
updated_date: 2026-01-19
branch: feature/pr-workflow-integration
completion_percentage: 65
spec_refs:
  - "docs/SDD_WORKFLOW.md"
depends_on:
  - 55  # Iteration command system (completed)
---

# Task 57: PR Workflow Integration for Claude Feedback

## Objective

Enable full PR workflow integration where Claude can receive feedback via PR comments, respond to @mentions, and iterate on PRs based on user guidance. Support both task-only PRs and design-iteration PRs. Create CI/CD loop where PR merge triggers next iteration suggestion for continuous improvement.

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

### Iteration Command Integration (2026-01-19)
- [x] Update `/iterate` command to ALWAYS create PRs (success/tradeoff/failure)
- [x] Add git hygiene documentation (feature branches, structured commits)
- [x] Add regression tradeoff policy (acceptable vs unacceptable)
- [x] Integrate design-critic agent at strategic points
- [x] Add PR commands table (@claude /status, /critique, /retry, etc.)
- [x] Document continuous improvement loop (PR merge → suggest next iteration)
- [ ] Create GitHub Action for iteration suggestion on PR merge
- [ ] Add PR label-based workflow (improvement/tradeoff/retry)

### PR Type Detection
- [x] Add PR template for task PRs (`.github/PULL_REQUEST_TEMPLATE/task.md`)
- [x] Add PR template for iteration PRs (`.github/PULL_REQUEST_TEMPLATE/iteration.md`)
- [ ] Detect PR type from labels or template usage (requires claude.yml implementation)
- [x] Display appropriate workflow guidance in PR description (templates include guidance)

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
- [x] Include metric diff table in PR description
- [x] Respond to metric-related questions
- [x] Propose alternative approaches when metrics regress
- [x] Track iteration attempts in PR body
- [x] Create PRs even for failed iterations (with "retry" label)
- [x] Support "tradeoff" PRs for acceptable regressions
- [x] Invoke design-critic for failed iterations
- [ ] Implement PR command handlers (/status, /critique, /retry, etc.)
- [ ] Auto-close failed iteration PRs with analysis
- [ ] Track prediction accuracy across iterations

### @Mention Handling
- [ ] Respond to `@claude` mentions in PR comments
- [x] Support commands: `/status`, `/retry`, `/explain`, `/compare`, `/critique`, `/tradeoff`
- [ ] Provide context-aware responses based on PR type
- [ ] Link to relevant tasks/iterations
- [x] Invoke design-critic agent on `/critique` command

### GitHub Actions Updates
- [ ] Update `claude.yml` for enhanced PR interaction
- [ ] Add PR-specific permissions (write access for comments)
- [ ] Handle PR review events (approve, request changes)
- [ ] Trigger evals on PR updates for iteration PRs
- [ ] Create `iterate-suggest.yml` workflow:
  - Triggers on PR merge to main (with "iteration" label)
  - Updates metrics/baseline.json
  - Posts issue suggesting next iteration goal
  - Optional: Auto-trigger `/iterate auto`

### Tests
- [ ] Test PR type detection
- [ ] Test feedback parsing
- [ ] Test @mention responses
- [ ] All tests pass

## Acceptance Criteria

**Phase 2 (Artifacts) - ✅ COMPLETED**:
- [x] PR templates created (task.md, iteration.md)
- [x] Task PRs show task checklist in description (via template)
- [x] Iteration PRs show metric diff table (via template)
- [x] Claude prompts created for PR interactions
- [x] PR commands documented and specified

**Phase 3 (Implementation) - ⏳ REMAINING**:
- [ ] Claude responds to @mentions on PRs within 2 minutes (needs claude.yml)
- [ ] User feedback triggers appropriate responses (needs claude.yml)
- [ ] Conversation history maintained in PR (GitHub native)
- [ ] PR type correctly detected from template/labels (needs claude.yml)
- [ ] Iteration PRs can spawn follow-up iterations (needs iterate-suggest.yml)
- [ ] All existing tests pass (after implementation)

## Implementation Notes

### Files to Create

Templates:
- [x] `.github/PULL_REQUEST_TEMPLATE/task.md`
- [x] `.github/PULL_REQUEST_TEMPLATE/iteration.md`

Prompts:
- [x] `.claude/prompts/pr-respond.md` - Handle PR comments
- [x] `.claude/prompts/pr-status.md` - Report PR status
- [x] `.claude/prompts/pr-compare.md` - Compare metrics

GitHub Actions:
- [ ] `.github/workflows/iterate-suggest.yml` - Auto-suggest next iteration on merge

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

## Progress Log (2026-01-19)

### Completed (2026-01-19)

**Phase 1: Specification & Design**
- ✅ Updated `/iterate` command with full CI/CD loop workflow
- ✅ Added git hygiene confirmation (feature branches, structured commits)
- ✅ Implemented ALWAYS-CREATE-PR policy (success/tradeoff/failure)
- ✅ Added regression tradeoff policy with acceptable scenarios
- ✅ Integrated design-critic agent at strategic points
- ✅ Documented PR commands (@claude /status, /critique, /retry, etc.)
- ✅ Added continuous improvement loop vision (PR merge → suggest next)

**Phase 2: Artifacts Creation**
- ✅ Created PR template for task PRs (`.github/PULL_REQUEST_TEMPLATE/task.md`)
- ✅ Created PR template for iteration PRs (`.github/PULL_REQUEST_TEMPLATE/iteration.md`)
- ✅ Created pr-respond.md prompt (handles PR comments and commands)
- ✅ Created pr-status.md prompt (reports PR status for task/iteration PRs)
- ✅ Created pr-compare.md prompt (detailed metric comparison)
- ✅ Updated Task 57 with progress tracking

### Remaining (Phase 3: GitHub Actions Implementation)

**Critical Path**:
- ⏳ Create `.github/workflows/iterate-suggest.yml`:
  - Trigger on PR merge to main with "iteration" label
  - Update metrics/baseline.json
  - Post issue suggesting next iteration goal
  - Optional: Auto-trigger `/iterate auto`

**Enhanced PR Handling**:
- ⏳ Update `claude.yml` for PR command handling:
  - Parse commands: /status, /critique, /explain, /retry, /compare, /tradeoff
  - Load appropriate prompt (pr-respond, pr-status, pr-compare)
  - Invoke design-critic for /critique command
  - Post response as PR comment

**Automation**:
- ⏳ Add PR label-based automation (improvement/tradeoff/retry)
- ⏳ Auto-close failed iteration PRs with analysis
- ⏳ Track prediction accuracy across iterations

**Testing**:
- ⏳ Test full workflow end-to-end
- ⏳ Verify PR type detection
- ⏳ Verify command parsing
- ⏳ Verify @mention responses

### Key Design Decisions

**Always Create PR**:
- Even failed iterations create PR (closed immediately with "retry" label)
- Transparency: All attempts logged and visible
- Learning: Failed iterations teach us what doesn't work

**Regression Tradeoff Policy**:
- Acceptable: Planned tradeoffs, zone-specific improvements, multi-step strategies
- Unacceptable: Unplanned regressions, critical metric failures, excessive drops
- Requires explicit justification in estimate phase

**Critic Agent Integration**:
- Step 5.2: Optional pre-flight check on estimates (recommended for LOW confidence)
- Step 13C: Mandatory post-mortem on failures
- PR command: User can request critique via `@claude /critique`

**Continuous Loop**:
```
[main] → /iterate → [branch] → [commit] → [PR]
   ↑                                        ↓
   └──────── [merge] ←────── [review] ──── [label: improvement/tradeoff]
                                        ↓
                              [label: retry] → [close, log, suggest retry]
```

## Current Status Summary

**Task is 65% complete**:

✅ **DONE** (Phase 1 & 2):
- Specification complete
- `/iterate` command fully documented
- PR templates created and ready to use
- Claude prompts created and ready for integration
- Design decisions documented

⏳ **TODO** (Phase 3):
- GitHub Actions implementation (`iterate-suggest.yml`)
- `claude.yml` updates for PR command handling
- End-to-end testing

**Next Steps**:
1. Implement `iterate-suggest.yml` workflow
2. Update `claude.yml` to parse PR commands and invoke prompts
3. Test with a real iteration PR
4. Mark task complete

**Estimated Remaining Effort**: 2-3 hours (GitHub Actions implementation)
