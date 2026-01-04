---
description: Check and fix SDD project consistency - verifies task status, git state, spec alignment, and overall project health
agent: sdd-manager
---

# SDD Maintenance Check

You are in **MAINTENANCE MODE**. Your mission is to audit the SDD project state and identify/fix any inconsistencies.

## Maintenance Scope

$ARGUMENTS

If no scope specified, run full audit. Otherwise, focus on:
- "tasks" - Task file consistency only
- "git" - Git state and branch hygiene
- "spec" - Spec vs implementation alignment
- "full" - Everything

## Full Audit Workflow

### 1. Task Status Audit

```bash
echo "=== TASK FILES ==="
find docs/tasks -name "*.md" -type f | grep -v index.md | sort

echo "=== TASK INDEX ==="
cat docs/tasks/index.md

echo "=== TASK IDS ==="
grep -r "^id:" docs/tasks/ | sort -t: -k3 -n
```

For each task file, verify:
- [ ] Task file exists for each index entry
- [ ] Index entry exists for each task file
- [ ] Status in file matches status in index
- [ ] Completed tasks are in `completed/` folder (if using that structure)
- [ ] Active tasks have corresponding branches
- [ ] **All tasks have numeric IDs** (no ID = CRITICAL issue)
- [ ] IDs are unique (no duplicates)

### 1b. Task Design Quality Audit

```bash
echo "=== TASKS WITH DEPENDENCIES ==="
grep -r "depends_on:" docs/tasks/ | grep -v "depends_on: \[\]" | grep -v "# depends_on"

echo "=== BLOCKED TASKS ==="
grep -r "status: blocked" docs/tasks/
```

Check for:
- [ ] **Minimal dependencies** - flag tasks with `depends_on` for review
- [ ] No circular dependencies
- [ ] Blocked tasks have clear `blocked_reason`
- [ ] Consider: Can blocked work be merged into blocking task?

**Dependency Warning Thresholds:**
- 0 dependencies: Ideal (parallel-first design)
- 1 dependency: Acceptable if unavoidable
- 2+ dependencies: Flag for redesign consideration

### 2. Git State Audit

```bash
echo "=== CURRENT BRANCH ==="
git branch --show-current

echo "=== ALL BRANCHES ==="
git branch -a

echo "=== WORKING TREE ==="
git status

echo "=== UNCOMMITTED CHANGES ==="
git diff --stat

echo "=== RECENT COMMITS ==="
git log --oneline -10

echo "=== STASHES ==="
git stash list
```

Check for:
- [ ] No uncommitted changes on main/master
- [ ] Feature branches exist for in-progress tasks
- [ ] No orphaned feature branches (task complete but branch not merged)
- [ ] No stale stashes
- [ ] Clean working directory

### 3. Task-Git Correlation

For each task marked "in-progress":
```bash
# Check if branch exists
git branch --list "feature/<slug>"

# Check if branch has commits
git log --oneline feature/<slug> 2>/dev/null | head -5
```

For each task marked "complete":
```bash
# Check if commits exist with task reference
git log --oneline --all --grep="Task <ID>" | head -5

# Check if changes are on main
git log --oneline main --grep="Task <ID>" 2>/dev/null | head -3
```

### 4. Spec Alignment Audit

```bash
echo "=== SPEC VERSION ==="
head -20 docs/specs/main.md | grep -i version

echo "=== PENDING CHANGES ==="
grep -A 20 "Pending Changes" docs/specs/main.md | head -25

echo "=== RECENT CHANGES ==="
grep -A 10 "Recent Changes" docs/specs/main.md | head -15
```

Check for:
- [ ] Completed tasks removed from "Pending Changes"
- [ ] Completed tasks added to "Recent Changes"
- [ ] Spec version incremented appropriately
- [ ] No outdated references to completed work

### 5. Build/Test Health

Delegate to `validator`:
```
Run full validation: make clean && make && make test
```

Check for:
- [ ] Build succeeds
- [ ] All tests pass
- [ ] No new warnings

## Report Format

```
# SDD Maintenance Report

**Date**: <timestamp>
**Scope**: <full | tasks | git | spec>

## Summary
- Tasks: <N total, X pending, Y in-progress, Z complete>
- Task IDs: <all numbered | X missing IDs>
- Dependencies: <none | X tasks have depends_on>
- Git: <clean | issues found>
- Spec: <aligned | needs update>
- Build: <passing | failing>

## Issues Found

### CRITICAL (must fix)
1. **<Issue>**: <description>
   - Location: <file/branch>
   - Fix: <suggested action>

### WARNING (should fix)
1. **<Issue>**: <description>
   - Fix: <suggested action>

### INFO (for awareness)
1. **<Observation>**: <description>

## Inconsistencies

| Task | Index Status | File Status | Branch | Commits | Issue |
|------|--------------|-------------|--------|---------|-------|
| 22   | in-progress  | complete    | exists | 3       | Status mismatch |

## Recommended Actions

1. <action 1>
2. <action 2>
3. <action 3>

## Auto-Fixable Issues

The following can be fixed automatically:
- [ ] Update index.md to match task file statuses
- [ ] Move completed task files to completed/ folder
- [ ] Update spec "Pending Changes" table

**Run with `--fix` to apply these automatically.**
```

## Auto-Fix Mode

If user confirms fixes (or runs with --fix intent):

### Fix Status Mismatches
```bash
# Update index.md entries
# Edit task files to correct status
```

### Fix File Organization
```bash
# Move completed tasks
git mv docs/tasks/active/<file>.md docs/tasks/completed/<file>.md
```

### Fix Spec References
```bash
# Update Pending/Recent Changes tables
# Bump version if needed
```

### Commit Fixes
```bash
git add docs/
git commit -m "docs: SDD maintenance - fix tracking inconsistencies

- Updated task statuses
- Moved completed tasks
- Aligned spec references"
```

## Common Issues & Fixes

### "Task missing numeric ID" (CRITICAL)
- All tasks MUST have a numeric ID in frontmatter
- Find next available: `grep -r "^id:" docs/tasks/ | sed 's/.*id: //' | sort -n | tail -1`
- Add `id: <next-number>` to frontmatter

### "Task has depends_on" (WARNING)
- Review if dependency is truly unavoidable
- Consider: Can the blocking work be included in this task?
- If unavoidable: Document clearly, keep chain short (max 1-2 levels)

### "Multiple tasks in dependency chain" (WARNING)
- Prefer merging tightly-coupled tasks into one larger task
- Tasks should be executable in parallel when possible

### "Task marked complete but no commits"
- Either: Task wasn't actually implemented (revert to pending)
- Or: Commits exist but don't reference task ID (add reference)

### "In-progress task with no branch"
- Create branch: `git checkout -b feature/<slug>`
- Or: Work was done on main (not recommended, but note it)

### "Completed task still in Pending Changes"
- Update spec to move to Recent Changes
- Increment spec version

### "Orphaned feature branch"
- Task complete and merged: delete branch
- Task abandoned: either delete or reopen task

### "Uncommitted changes"
- Either: commit them with appropriate message
- Or: stash them with descriptive name
- Or: discard if not needed

## Proactive Checks

Also check for:
- Tasks older than 2 weeks in "pending" (may need re-prioritization)
- Tasks in "in-progress" for more than 1 week (may be stuck)
- Feature branches with no commits in 2+ weeks
- TODOs/FIXMEs in code that reference completed tasks

```bash
# Find stale TODOs
grep -rn "TODO.*Task" src/ --include="*.cpp" --include="*.h"
grep -rn "FIXME" src/ --include="*.cpp" --include="*.h"
```

## Remember

Maintenance is about keeping the SDD system trustworthy. If task tracking doesn't match reality, the whole workflow breaks down. Be thorough, fix what you can, and clearly report what needs human decision.
