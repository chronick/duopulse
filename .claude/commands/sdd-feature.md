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

### 3. Analyze Scope (Parallel-First Design)

**Goal**: Create self-contained tasks that can be executed in any order.

- Can all related work fit in one task? (Prefer larger self-contained tasks)
- Are there genuinely independent pieces? (Only split if truly unrelated)
- **Avoid dependencies** - if work is coupled, keep it in one task
- Tasks can be epic-sized; don't artificially split tightly-coupled work

#### Large Implementation Guidance

For very large features (multi-week scope), dependencies between tasks are acceptable when:

1. **Each task produces a working product** - The system must be functional after each task completes. No task should leave the codebase in a broken or half-implemented state.

2. **Tasks are independently valuable** - Each task delivers meaningful functionality on its own, not just "setup" for later tasks.

3. **Clear boundaries exist** - The dependency is at a well-defined interface (e.g., "Task B adds UI for the API that Task A creates").

**Anti-patterns to avoid:**
- Task A: "Add data structures" → Task B: "Add logic" → Task C: "Add tests"
  - ❌ Task A alone is useless; group into one task
- Task A: "Implement feature (broken)" → Task B: "Fix bugs from Task A"
  - ❌ Task A doesn't produce a working product

**Acceptable dependency example:**
- Task A: "Core pattern generation with basic UI" (fully working)
- Task B: "Advanced pattern visualization" (depends on A, adds value)
  - ✅ Both tasks produce working products; B enhances A

### 4. Create Task Breakdown

**All tasks MUST have a numeric ID.** Find next ID:
```bash
grep -r "^id:" docs/tasks/ | sed 's/.*id: //' | sort -n | tail -1
```

For each task, define:
- **ID**: Next sequential number (REQUIRED)
- **Slug**: kebab-case identifier
- **Title**: Clear, descriptive name
- **Subtasks**: Group all related work together
- **Acceptance Criteria**: How to verify completion
- **Files to Modify**: Expected changes

**Dependency Check**: Before adding `depends_on`:
1. Can this work be included in the blocking task instead?
2. Can the task be restructured to avoid the dependency?
3. Is the dependency truly unavoidable (different codebase areas)?
4. **Does each task produce a working product?** (Required for dependencies)
5. **Is each task independently valuable?** (Not just "setup")

### 5. Present Plan for Confirmation

Before creating files, present:
```
## Feature: <name>

### Analysis
<1-2 paragraphs on what you learned>

### Task Breakdown (Parallel-Executable)

1. **Task <ID>: <slug-1>**: <description>
   - Key files: <list>
   - Self-contained: Yes / Depends on: <only if unavoidable>

2. **Task <ID+1>: <slug-2>**: <description>
   - Key files: <list>
   - Self-contained: Yes

### Parallelization
- Tasks can be executed in any order: [Yes/No]
- If No, explain unavoidable dependencies

### Ready to create task files?
```

### 6. Create Task Files

After confirmation:
1. Create `docs/tasks/active/<id>-<slug>.md` for each task
2. Update `docs/tasks/index.md` with new entries
3. Report created files and suggested branch names

## Task File Template

**IMPORTANT**: All task files MUST use YAML frontmatter for metadata.

```markdown
---
id: <next-sequential-number>        # REQUIRED - must be unique
slug: <id>-<kebab-case-name>        # e.g., "27-voice-redesign"
title: "<Descriptive Title>"
status: pending
created_date: <YYYY-MM-DD>
updated_date: <YYYY-MM-DD>
branch: feature/<slug>
spec_refs:
  - "<spec-section-reference>"
# depends_on: []            # AVOID - group related work instead
# related: []               # Optional - non-blocking references only
---

# Task <ID>: <Title>

## Objective

<What this task accomplishes - 1-2 sentences>

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

### Frontmatter Field Reference

| Field | Required | Description |
|-------|----------|-------------|
| `id` | **Yes** | Unique task number (sequential) |
| `slug` | **Yes** | `<id>-<kebab-case-name>`, matches filename |
| `title` | **Yes** | Human-readable title |
| `status` | **Yes** | pending, in-progress, blocked, completed, archived |
| `created_date` | **Yes** | ISO date (YYYY-MM-DD) |
| `updated_date` | **Yes** | ISO date, update on any change |
| `branch` | **Yes** | Git branch name (`feature/<slug>`) |
| `spec_refs` | No | List of spec section references |
| `depends_on` | **Avoid** | Only if truly unavoidable |
| `related` | No | Non-blocking references |

## Important

- Do NOT implement any code in planning mode
- Do NOT delegate to code-writer
- DO assign numeric IDs to ALL tasks
- DO create self-contained, parallel-executable tasks
- DO suggest using `/sdd-task <id>` to begin implementation after planning
- AVOID creating dependency chains - group coupled work instead
