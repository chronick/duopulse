# Spec-Driven Development (SDD) Workflow Skill

## Purpose

This skill encodes the Spec-Driven Development methodology used in this project. It provides workflow guidance, task management patterns, and quality standards.

## When to Activate

Activate this skill when:
- Planning new features or changes
- Implementing tasks from `docs/tasks/`
- Updating specs in `docs/specs/`
- Managing task lifecycle and tracking
- Discussing project workflow or methodology

## Core Principles

### 1. Specs Are Truth
- `docs/specs/main.md` is the authoritative source
- Code implements what the spec says
- If code and spec disagree, one must be updated
- Never implement behavior not in the spec

### 2. Tasks Are Contracts
- Every non-trivial change requires an indexed task
- Tasks live in `docs/tasks/` (indexed in `index.md`)
- Task defines scope, acceptance criteria, and completion conditions
- Don't exceed task scope without discussion

### 3. Small Increments
- Tasks should be 1-2 hours max
- Subtasks should be 15-30 minutes each
- Prefer multiple small PRs over one large PR
- Each increment should be testable independently

### 4. Verify Everything
- Trust tool output, not agent claims
- `git diff` after every code change
- `make test` after every subtask
- Code review before commit

## Task Lifecycle

```
PENDING → IN-PROGRESS → COMPLETE
    ↓         ↓
  BLOCKED   (fix blockers)
```

### Pending
- Task defined but not started
- No branch created yet
- Waiting for dependencies or prioritization

### In-Progress
- Feature branch created
- Active development
- Subtasks being completed

### Blocked
- Cannot proceed due to external dependency
- Document blocker in task file
- May need spec clarification or upstream changes

### Complete
- All subtasks checked off
- All tests passing
- Code review passed
- Committed to feature branch
- Task file updated, moved to `completed/`
- Spec updated if applicable

## File Structure

```
docs/
├── specs/
│   └── main.md              # The Spec (source of truth)
├── tasks/
│   ├── index.md             # Task index table
│   ├── active/              # In-progress tasks
│   │   └── 22-control-simplification.md
│   └── completed/           # Finished tasks
│       └── 21-musicality.md
└── code-review/             # Review guidelines
    ├── README.md
    ├── realtime-audio-safety.md
    └── common-pitfalls.md
```

## Task File Format

```markdown
# Task <ID>: <Title>

**Status**: pending | in-progress | blocked | complete
**Branch**: feature/<slug>
**Spec Section**: <reference to main.md section>

---

## Objective
<1-2 sentences: what this task accomplishes>

## Subtasks
- [ ] Subtask 1: <specific, testable action>
- [ ] Subtask 2: <specific, testable action>
- [ ] All tests pass (`make test`)

## Acceptance Criteria
- [ ] <Criterion 1>
- [ ] All tests pass
- [ ] No new compiler warnings
- [ ] Code review passes

## Implementation Notes
<Technical guidance, constraints>

### Files to Modify
- `path/to/file.cpp` - <description>

### Risks
<What could go wrong>
```

## Task Index Format

```markdown
# Task Index

| ID | Slug | Title | Status | Branch | Updated |
|----|------|-------|--------|--------|---------|
| 22 | control-simplification | Control Simplification | in-progress | feature/control-simplification | 2025-01-01 |
| 23 | field-updates | Field Update System | pending | - | 2024-12-30 |
```

## Commit Message Format

```
<type>(<scope>): <description>

<body - what changed and why>

<footer - references>
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code restructuring
- `test`: Test changes
- `docs`: Documentation only
- `wip`: Work in progress

Example:
```
feat(task-22-control-simplification): remove reset mode UI

- Hardcoded reset mode to STEP
- Freed Config K4 for future use
- Updated ControlProcessor to skip K4 processing

Closes Task 22 Phase A1
```

## Spec Update Protocol

When a task changes behavior documented in spec:

1. Check "Pending Changes" table in spec
2. Update relevant spec sections
3. Remove from "Pending Changes"
4. Add to "Recent Changes" with date
5. Bump spec version (e.g., 4.1 → 4.2)
6. Commit spec changes separately

## Quality Gates

Before marking a task complete:

- [ ] All subtasks checked off
- [ ] All tests pass (`make test`)
- [ ] No new compiler warnings (`make`)
- [ ] Code review passes (no CRITICAL issues)
- [ ] Acceptance criteria verified
- [ ] Task file status updated
- [ ] Index updated
- [ ] Spec updated (if applicable)

## Commands Reference

- `/sdd-feature <description>` - Plan a new feature
- `/sdd-task <id or slug>` - Implement a task
- `/sdd-maintain` - Audit project consistency
- `/wrap-session` - Clean wrap-up with commit

## Anti-Patterns

**Don't:**
- Implement without a task ("just a quick fix")
- Exceed task scope ("while I'm here...")
- Skip tests ("I'll add them later")
- Claim completion without verification
- Leave tasks in-progress indefinitely

**Do:**
- Create a task for non-trivial changes
- Stay within defined scope
- Test after every subtask
- Verify with git diff / git log
- Complete or explicitly pause tasks
