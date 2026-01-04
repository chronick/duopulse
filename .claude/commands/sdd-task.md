---
description: Implement an SDD task from start to finish - delegates to code-writer, runs tests, commits when complete
agent: sdd-manager
---

# Implement Task: $ARGUMENTS

You are in **IMPLEMENTATION MODE**. Your mission is to complete this task fully, with verified code changes, passing tests, and proper commits.

## Task Reference

$ARGUMENTS

This can be:
- Task ID: "22" or "Task 22" (preferred - all tasks are numbered)
- Task slug: "22-control-simplification"
- File path: "docs/tasks/active/22-control-simplification.md"

**Note**: All tasks have numeric IDs. Use the ID for unambiguous reference.

## Implementation Workflow

### 1. Load Context

```bash
# Find and read the task file
find docs/tasks -name "*$ARGUMENTS*" -o -name "*control-simplification*" 2>/dev/null
cat docs/tasks/active/<task-file>.md

# Check current branch
git branch --show-current
git status

# Read relevant spec sections
cat docs/specs/main.md | head -200
```

### 2. Setup Branch

```bash
# Get branch name from task file, then:
git checkout <branch> 2>/dev/null || git checkout -b <branch>
git branch --show-current  # Verify
```

### 3. Identify Work

- Which subtasks are pending (`[ ]`)?
- What order should they be done?
- Are there dependencies between them?

### 4. For Each Subtask

#### 4a. Delegate to code-writer
Use Task tool to invoke `code-writer` with:
```
Goal: <exact subtask description>
Files: <specific files to modify>
Context: <relevant spec excerpt, existing patterns>
Constraints: <RT audio rules if applicable>
```

#### 4b. Verify Changes (CRITICAL)
**Immediately** after code-writer returns:
```bash
git diff --stat
git diff
git status
```

If no files changed → delegation FAILED. Retry or report error.

#### 4c. Run Tests
Delegate to `validator`:
```
Run standard validation: make && make test
```

Read the output. If tests fail:
1. Delegate fix to code-writer with error details
2. Re-validate
3. Max 3 attempts before escalating

#### 4d. Update Task Checkbox
Edit the task file to mark subtask complete:
```markdown
- [x] Subtask description
```

### 5. Periodic Review

After every 2-3 subtasks, delegate to `code-reviewer`:
```
Review recent changes for Task <ID>
Files: <list of changed files>
Focus: <specific concerns>
```

If CRITICAL issues found:
1. Delegate fixes to code-writer
2. Re-validate
3. Re-review

### 6. Final Verification

Before committing:
```bash
# All tests pass
make test

# No warnings
make 2>&1 | grep -i warning

# Changes look correct
git diff --stat
git diff
```

### 7. Commit

```bash
# Stage changes
git add -A

# Commit with proper message
git commit -m "feat(task-<id>-<slug>): <brief description>

- <subtask 1 completed>
- <subtask 2 completed>
- <subtask 3 completed>

Closes Task <ID>"

# Verify commit
git log -1 --stat
```

### 8. Update Tracking

#### 8a. Update Task File Frontmatter

Edit the YAML frontmatter at the top of the task file:

```yaml
---
# Update these fields:
status: completed                    # was: in-progress
updated_date: <today YYYY-MM-DD>     # update to today
completed_date: <today YYYY-MM-DD>   # add this field
commits:                             # add commit hashes
  - <commit-hash-1>
  - <commit-hash-2>
---
```

#### 8b. Update Task Index

Update `docs/tasks/index.md`:
- Set status to `completed`
- Set updated_at to today

#### 8c. Move Task File

Move to completed folder:
```bash
git mv docs/tasks/active/<file>.md docs/tasks/completed/<file>.md
```

#### 8d. Commit Tracking Updates

```bash
git add docs/tasks/
git commit -m "docs(tasks): mark Task <ID> complete"
```

### 9. Update Spec (if applicable)

Check if spec needs updating:
```bash
grep -i "task <id>" docs/specs/main.md
grep -i "pending" docs/specs/main.md
```

If task is mentioned in "Pending Changes":
1. Update relevant spec sections
2. Remove from "Pending Changes"
3. Add to "Recent Changes"
4. Bump version
5. Commit: `docs(spec): update for Task <ID>`

### 10. Report Completion

```
## Task <ID> Complete

**Branch**: <branch-name>
**Commits**: <list commit hashes>
**Files Changed**: <count>
**Tests**: All passing

### Summary
<What was implemented>

### Next Steps
<Suggested follow-up or "Ready for merge">
```

## Critical Rules

1. **Never write code directly** - always delegate to code-writer
2. **Always verify with git diff** - after every code-writer call
3. **Always run tests** - via validator, read the actual output
4. **Never skip review** - at least one code-reviewer pass before commit
5. **Never claim success without proof** - show git log, test output

## Error Recovery

**code-writer returns but no changes:**
- Check tool permissions
- Retry with more explicit instructions
- Report failure after 2 retries

**Tests keep failing:**
- Capture full error output
- Send to code-writer with context
- Max 3 fix cycles, then escalate

**Review finds CRITICAL issues:**
- Must fix before commit
- Delegate to code-writer
- Re-validate and re-review
