---
description: Wrap up current work session - verify build/tests, add notes, commit progress, prepare handoff
---

# Wrap Session

Cleanly wrap up the current development session with proper verification and documentation.

## Workflow

### 1. Verify Build & Tests

```bash
echo "=== BUILD CHECK ==="
make 2>&1 | tee /tmp/build.log
echo "Exit code: $?"

echo ""
echo "=== WARNINGS ==="
grep -i warning /tmp/build.log || echo "No warnings"

echo ""
echo "=== TEST CHECK ==="
make test 2>&1 | tee /tmp/test.log
echo "Exit code: $?"
```

**If build or tests fail**: 
- Note the failures
- Ask user if they want to fix before wrapping, or note as "incomplete"

### 2. Check Uncommitted Changes

```bash
echo "=== GIT STATUS ==="
git status

echo ""
echo "=== UNCOMMITTED CHANGES ==="
git diff --stat

echo ""
echo "=== STAGED CHANGES ==="
git diff --cached --stat
```

**If changes exist**:
- Summarize what changed
- Prepare commit message

### 3. Identify Current Task

```bash
echo "=== CURRENT BRANCH ==="
git branch --show-current

echo ""
echo "=== RECENT TASK CONTEXT ==="
# Try to find related task
BRANCH=$(git branch --show-current)
find docs/tasks -name "*.md" -exec grep -l "$BRANCH" {} \; 2>/dev/null | head -1
```

### 4. Update Task Notes

If a task file is identified, add session notes:

```markdown
## Session Notes - <DATE>

### Progress
- <what was accomplished>

### State
- Build: passing/failing
- Tests: X passing, Y failing
- Branch: <branch-name>
- Last commit: <hash>

### Next Steps
- <what to do next session>

### Blockers
- <any issues encountered>
```

### 5. Commit Work

If there are uncommitted changes and build passes:

```bash
# Stage all changes
git add -A

# Commit with descriptive message
git commit -m "<type>(<scope>): <description>

Session wrap-up:
- <change 1>
- <change 2>

Status: <complete | in-progress | blocked>
Next: <suggested next action>"
```

Commit types:
- `feat`: New feature work
- `fix`: Bug fixes
- `wip`: Work in progress (incomplete)
- `refactor`: Code restructuring
- `test`: Test additions/changes
- `docs`: Documentation only

### 6. Final Status Report

```
## Session Wrap-Up Complete

**Branch**: <branch-name>
**Commit**: <hash> (or "no new commits")
**Build**: passing | failing
**Tests**: X/Y passing

### This Session
- <summary of work done>

### Current State
- Task: <task ID and status>
- Subtasks: X/Y complete
- Blockers: <any issues>

### Next Session
- <recommended next steps>
- <any context to remember>

### Handoff Notes
<Anything the next session (or another developer) should know>
```

## Quick Wrap (No Commit)

If user just wants status without committing:

```bash
echo "=== QUICK STATUS ==="
git status --short
make test 2>&1 | tail -5
echo ""
echo "Changes not committed. Run /wrap-session again to commit."
```

## Error Cases

### Build Fails
```
⚠️  Build is failing. Options:
1. Fix the build issues before wrapping
2. Commit as WIP with "wip: <description> [BUILD BROKEN]"
3. Stash changes: git stash push -m "session-<date>-incomplete"

Which would you like to do?
```

### Tests Fail
```
⚠️  Tests are failing (X failures). Options:
1. Fix the test failures before wrapping
2. Commit as WIP noting the failures
3. Skip test failures and commit (not recommended)

Which would you like to do?
```

### No Task Context
```
ℹ️  No task file found for current branch.
    Creating standalone session commit.
    Consider: 
    - Is this work associated with a task?
    - Should a task be created? (/sdd-feature)
```

## Remember

The goal is to leave the repo in a known, documented state so the next session (or another developer) can pick up smoothly. Always verify before committing, and always document what's incomplete.
