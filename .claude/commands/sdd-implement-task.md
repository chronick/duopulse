---
name: sdd-implement-task
agent: sdd-full-task
description: Implement a complete SDD task from start to finish
---

# SDD Task Implementation

You are implementing Task: {{{ ARGUMENTS }}}

## Task Context

The user has requested implementation of an SDD task. The task can be specified as:
- Task ID (e.g., "Task 22")
- Task slug (e.g., "control-simplification")
- Task file path (e.g., "@docs/tasks/active/22-control-simplification.md")

## Your Mission

Implement this task **completely from start to finish**:

1. **Setup feature branch**
   - Read task file to get specified branch name
   - Check if branch exists (`git branch --list`)
   - If exists: `git checkout <branch>`
   - If not: `git checkout -b <branch>` from current branch
   - Verify clean working directory before starting

2. **Load task context**
   - Read `docs/tasks/index.md` to locate the task
   - Read the task file from `docs/tasks/<slug>.md`
   - Read relevant spec sections from `docs/specs/main.md`
   - Check which spec sections need updates (look for task mentions in "Pending Changes")
   - Identify pending vs completed subtasks

3. **Implement each subtask**
   - Delegate to `sdd-subtask` agent for code implementation
   - Run tests after each subtask (`make test`)
   - Fix any test failures
   - Run periodic code reviews (every 2-3 subtasks)

4. **Final quality gates**
   - All subtasks complete
   - All tests passing
   - Code review passes (no CRITICAL/WARNING issues)
   - No new compiler warnings

5. **Update specification**
   - Check `docs/specs/main.md` "Pending Changes" table for this task
   - Update all spec sections mentioned for this task
   - Remove task from "Pending Changes" table
   - Add task to "Recent Changes" table with date and summary
   - Bump spec version (e.g., 4.1 → 4.2)
   - Update "Last Updated" date
   - Commit spec changes separately: `docs(spec): update for Task XX`

6. **Commit and track**
   - Create git commit with proper message format for implementation
   - Update task file status to complete, add commit hash
   - Move task file from `active/` to `completed/`
   - Update `docs/tasks/index.md` (status, date, path)
   - Commit tracking updates separately: `docs(tasks): mark Task XX as complete`
   - Report completion summary

7. **Merge to parent branch** (optional, only if requested by user)
   - Identify parent branch from git history or ask user
   - `git checkout <parent-branch>`
   - `git merge --no-ff feature/<branch>`
   - Push if requested

## Important

- Use `sdd-subtask` for ALL code implementation (never write code yourself)
- Use `daisysp-code-reviewer` for code quality checks
- Verify every step with tool output (git status, git diff, make test)
- Never narrate hypothetical actions - prove each step actually happened
- Don't exit until task is FULLY complete

## Task Acceptance Criteria

Check the task file for specific acceptance criteria and ensure all are met before marking complete.