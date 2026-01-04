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
- Every non-trivial change requires a **numbered** task
- Tasks live in `docs/tasks/` (indexed in `index.md`)
- Task defines scope, acceptance criteria, and completion conditions
- All tasks MUST have a numeric ID (sequential, unique)

### 3. Self-Contained Tasks (Parallel-First Design)
- **Tasks should have zero or minimal dependencies**
- Group related work within the same task (can be epic-sized)
- Tasks can be executed in parallel or any order
- Dependencies (`depends_on`) are **discouraged** but allowed when unavoidable
- Prefer larger self-contained tasks over many interdependent small ones
- Each task should be completable without waiting on other tasks

### 4. Verify Everything
- Trust tool output, not agent claims
- `git diff` after every code change
- `make test` after every subtask
- Code review before commit

## Task Lifecycle

```
PENDING → IN-PROGRESS → COMPLETE
              ↓
           BLOCKED (rare - avoid if possible)
```

### Pending
- Task defined but not started
- No branch created yet
- Ready to start at any time (no dependencies blocking it)

### In-Progress
- Feature branch created
- Active development
- Subtasks being completed

### Blocked (Discouraged)
- Cannot proceed due to **unavoidable** external dependency
- Document blocker clearly in task file
- Consider: Can the blocking work be included in this task instead?
- Consider: Can the task be restructured to avoid the dependency?

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

All task files MUST use YAML frontmatter for metadata. This enables tooling, automation, and consistent tracking.

### Frontmatter Schema (Required Fields)

```yaml
---
id: <number>                    # Task ID (unique, sequential) - REQUIRED
slug: <id>-<kebab-case-name>    # e.g., "27-voice-redesign" - includes ID
title: <string>                 # Human-readable title
status: <enum>                  # pending | in-progress | blocked | completed | archived
created_date: <YYYY-MM-DD>      # When task was created
updated_date: <YYYY-MM-DD>      # Last modification date
branch: <string>                # Git branch name (feature/<slug>)
---
```

### Frontmatter Schema (Optional Fields)

```yaml
---
# Completion tracking
completed_date: <YYYY-MM-DD>    # When task was completed
commits: [<hash>, ...]          # Key commit hashes

# Archival tracking
archived_date: <YYYY-MM-DD>     # When task was archived
superseded_by: <task-id>        # If replaced by another task

# Relationships (use sparingly - prefer self-contained tasks)
parent_task: <task-id>          # Parent task if subtask
depends_on: [<task-id>, ...]    # DISCOURAGED: Blocking dependencies
related: [<task-id>, ...]       # Non-blocking references

# Spec references
spec_refs: [<section>, ...]     # Spec sections this implements

# Blocking info (rare - avoid if possible)
blocked_reason: <string>        # Why task is blocked
blocked_date: <YYYY-MM-DD>      # When it became blocked
---
```

### Task Numbering Rules

1. **All tasks MUST have a numeric ID** - no exceptions
2. IDs are sequential and unique across all tasks
3. ID determines filename: `<id>-<slug>.md`
4. Never reuse IDs from archived/completed tasks
5. Next ID = highest existing ID + 1

### Complete Task File Template

```markdown
---
id: 27
slug: example-feature
title: "Example Feature Implementation"
status: pending
created_date: 2026-01-03
updated_date: 2026-01-03
branch: feature/example-feature
spec_refs:
  - "section-3.2"
  - "appendix-a"
---

# Task 27: Example Feature Implementation

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

### Status Transitions & Required Fields

| From | To | Required Updates |
|------|----|------------------|
| pending | in-progress | `updated_date`, verify `branch` |
| in-progress | completed | `completed_date`, `commits`, move to `completed/` |
| in-progress | blocked | `blocked_reason`, `blocked_date` |
| blocked | in-progress | clear `blocked_*`, `updated_date` |
| any | archived | `archived_date`, `superseded_by` (if applicable), move to `archived/` |

### Frontmatter Validation Rules

1. **id** must be unique across all tasks (REQUIRED - no exceptions)
2. **slug** must include id and match filename: `<id>-<name>.md` (e.g., `27-voice-redesign`)
3. **branch** must follow pattern: `feature/<slug>` or `fix/<slug>`
4. **dates** must be ISO 8601 format (YYYY-MM-DD)
5. **status** must be one of: `pending`, `in-progress`, `blocked`, `completed`, `archived`
6. **commits** should include merge commit when completed
7. **depends_on** should be empty or minimal (parallel-first design)

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
- Create tasks without numeric IDs
- Create chains of dependent tasks when they could be combined
- Split tightly-coupled work into separate tasks

**Do:**
- Create a task for non-trivial changes
- Stay within defined scope
- Test after every subtask
- Verify with git diff / git log
- Complete or explicitly pause tasks
- Assign sequential numeric IDs to all tasks
- Group related work into single self-contained tasks
- Design tasks that can be executed in parallel

## Task Design Guidelines

### When to Combine Work into One Task

Combine into a single task when:
- Work touches the same files/modules
- Changes are tightly coupled (A can't work without B)
- Features build on each other within the same area
- Testing requires both pieces to be present

### When to Split into Separate Tasks

Split into separate tasks when:
- Work is in completely different areas of the codebase
- Either piece can be shipped independently
- Different expertise or focus required
- Genuinely no coupling between the work

### Handling Unavoidable Dependencies

If a dependency truly cannot be avoided:
1. Document clearly in `blocked_reason`
2. Consider if the blocking work can be included in this task
3. Keep the dependency chain as short as possible (max 1-2 levels)
4. Never create circular dependencies
