---
description: Plan a new feature using SDD workflow - analyzes spec, explores codebase, creates task files
agent: sdd-manager
---

# Plan New Feature: $ARGUMENTS

You are in **PLANNING MODE**. Your mission is to create a well-structured task breakdown for this feature.

## Feature Request

$ARGUMENTS

## Planning Workflow

### 1. Understand the Request
- What problem does this solve?
- What user need does it address?
- Is this a new feature, enhancement, or fix?

### 2. Research Context
```bash
# Read the main spec
cat docs/specs/main.md

# Check existing tasks
ls docs/tasks/
cat docs/tasks/index.md

# Explore related code
grep -r "relevant_term" src/ --include="*.cpp" --include="*.h" | head -20
```

### 3. Analyze Scope
- Is this one task or multiple?
- What are the dependencies?
- What's the risk level?
- Estimate: How many 1-2 hour tasks?

### 4. Create Task Breakdown

For each task, define:
- **Slug**: kebab-case identifier
- **Title**: Clear, descriptive name
- **Subtasks**: 3-8 items, each 15-30 minutes
- **Acceptance Criteria**: How to verify completion
- **Files to Modify**: Expected changes
- **Risk**: Low/Medium/High

### 5. Present Plan for Confirmation

Before creating files, present:
```
## Feature: <name>

### Analysis
<1-2 paragraphs on what you learned>

### Task Breakdown
1. **<slug-1>** (~Xh): <description>
   - Key files: <list>
   - Risk: <level>

2. **<slug-2>** (~Xh): <description> [depends: slug-1]
   - Key files: <list>
   - Risk: <level>

### Ready to create task files?
```

### 6. Create Task Files

After confirmation:
1. Create `docs/tasks/active/<id>-<slug>.md` for each task
2. Update `docs/tasks/index.md` with new entries
3. Report created files and suggested branch names

## Task File Template

```markdown
# Task <ID>: <Title>

**Status**: pending
**Branch**: feature/<slug>
**Spec Section**: <reference>

---

## Objective
<What this task accomplishes>

## Subtasks
- [ ] Subtask 1: <specific action>
- [ ] Subtask 2: <specific action>
- [ ] Subtask 3: <specific action>
- [ ] All tests pass (`make test`)

## Acceptance Criteria
- [ ] <Criterion 1>
- [ ] All tests pass
- [ ] No new compiler warnings
- [ ] Code review passes

## Implementation Notes

### Files to Modify
- `path/to/file.cpp` - <expected changes>

### Constraints
<RT audio rules, style requirements, etc.>

### Risks
<What could go wrong>
```

## Important

- Do NOT implement any code in planning mode
- Do NOT delegate to code-writer
- DO create complete, actionable task definitions
- DO suggest using `/sdd-task <slug>` to begin implementation after planning
