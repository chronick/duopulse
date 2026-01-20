---
description: Report PR status and progress
---

# PR Status Reporter

You are generating a status report for a GitHub Pull Request.

## Context

**PR Type**: ${PR_TYPE} (task | iteration)
**PR Number**: #${PR_NUMBER}
**PR Title**: ${PR_TITLE}

## Task-Specific Context

**For Task PRs**:
- Task file: `${TASK_FILE}`
- Task ID: #${TASK_ID}

**For Iteration PRs**:
- Iteration ID: ${ITER_ID}
- Iteration log: `docs/design/iterations/${ITER_ID}.md`
- Goal: ${GOAL}

## Your Task

Provide a concise status report answering:

1. What has been completed?
2. What remains to be done?
3. Are there any blockers?
4. What is the current assessment (ready to merge / needs work)?

## Status Report Format

### For Task PRs

```markdown
## 📊 Task Status: #${TASK_ID}

### Completed ✅
- [x] Subtask 1
- [x] Subtask 2
- [x] Tests added

### Remaining ⏳
- [ ] Subtask 3
- [ ] Documentation update

### Blockers 🚧
<!-- List any blockers, or "None" -->

### Assessment
<!-- Ready to merge | Needs work | Waiting for tests | etc. -->

### Next Steps
1. Action item 1
2. Action item 2
```

### For Iteration PRs

```markdown
## 📊 Iteration Status: ${ITER_ID}

### Goal
${GOAL}

### Metrics Summary
| Metric | Target | Status |
|--------|--------|--------|
| ${TARGET_METRIC} | +X% | ✅ Achieved: +Y% |
| Others | No regression | ⚠️ ${METRIC} regressed Z% |

### Current State
- **Success criteria**: Met / Not met / Partial
- **Tests**: Passing / Failing
- **Tradeoffs**: None / Acceptable / Unacceptable

### Assessment
<!-- Success (ready to merge) | Tradeoff (needs justification) | Failed (close with retry) -->

### Next Steps
1. Action item 1
2. Action item 2

### Links
- Iteration log: `docs/design/iterations/${ITER_ID}.md`
- Metrics baseline: `metrics/baseline.json`
```

## Guidelines

1. **Be factual**: Base status on actual file contents and test results
2. **Be concise**: Keep the report scannable
3. **Use emojis**: ✅ ⏳ 🚧 ⚠️ ❌ for visual clarity
4. **Highlight blockers**: Call out anything preventing progress
5. **Provide next steps**: Give 1-3 concrete action items

## Examples

### Example 1: Task PR Nearly Complete

```markdown
## 📊 Task Status: #46

### Completed ✅
- [x] Created ComputeNoiseScale() helper function
- [x] Updated anchor weight perturbation logic
- [x] Aligned zone boundaries with spec (0.30/0.70)
- [x] Added unit tests for noise scaling
- [x] All tests passing

### Remaining ⏳
- [ ] Verify stable zone produces consistent patterns
- [ ] Update pattern viz examples in task file

### Blockers 🚧
None

### Assessment
**Ready to merge** after verification testing

### Next Steps
1. Run pattern_viz sweep at SHAPE < 0.3 to verify consistency
2. Update task file with test results
3. Request review
```

### Example 2: Iteration PR with Tradeoff

```markdown
## 📊 Iteration Status: 2026-01-19-001

### Goal
Improve syncopation in wild zone

### Metrics Summary
| Metric | Target | Status |
|--------|--------|--------|
| Syncopation (wild) | +10% | ✅ Achieved: +12.4% |
| Regularity (stable) | No regression | ⚠️ Regressed -3.2% |

### Current State
- **Success criteria**: Partial (target met but regression present)
- **Tests**: ✅ All passing
- **Tradeoffs**: Acceptable (regularity regression within threshold)

### Assessment
**Tradeoff PR** - Target achieved with acceptable regression.

The regularity regression is within the 5% acceptable tradeoff range
and was partially predicted in estimates (predicted -2%, actual -3.2%).

This is a zone-specific improvement where wild zone gains (+12.4%)
outweigh stable zone loss (-3.2%).

### Next Steps
1. Add tradeoff justification to PR description
2. Document recovery plan for next iteration
3. Request review and approval
4. Consider follow-up iteration to restore regularity while preserving gains

### Links
- Iteration log: `docs/design/iterations/2026-01-19-001.md`
- Metrics baseline: `metrics/baseline.json`
```

## Output

Provide the status report in markdown format, ready to be posted as a PR comment.

Do NOT include meta-commentary - just write the status report itself.
