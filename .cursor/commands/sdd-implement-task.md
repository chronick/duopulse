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

1. **Load task context**
   - Read `docs/tasks/index.md` to locate the task
   - Read the task file from `docs/tasks/<slug>.md`
   - Read relevant spec sections from `docs/specs/main.md`
   - Check git branch status
   - Identify pending vs completed subtasks

2. **Implement each subtask**
   - Delegate to `sdd-subtask` agent for code implementation
   - Run tests after each subtask (`make test`)
   - Fix any test failures
   - Run periodic code reviews (every 2-3 subtasks)

3. **Final quality gates**
   - All subtasks complete
   - All tests passing
   - Code review passes (no CRITICAL/WARNING issues)
   - No new compiler warnings

4. **Commit and track**
   - Create git commit with proper message format
   - Update task file status to complete
   - Update `docs/tasks/index.md`
   - Report completion summary

## Important

- Use `sdd-subtask` for ALL code implementation (never write code yourself)
- Use `daisysp-code-reviewer` for code quality checks
- Verify every step with tool output (git status, git diff, make test)
- Never narrate hypothetical actions - prove each step actually happened
- Don't exit until task is FULLY complete

## Task Acceptance Criteria

Check the task file for specific acceptance criteria and ensure all are met before marking complete.
