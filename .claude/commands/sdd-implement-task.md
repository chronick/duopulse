# Command: SDD – Implement Task

You are implementing a complete SDD task from start to finish.

## What This Command Does

This command delegates to the **sdd-full-task** agent to:
1. Load the task file and understand requirements
2. Orchestrate subtask implementation (using sdd-subtask agent)
3. Run tests after each subtask
4. Coordinate code reviews (using daisysp-code-reviewer)
5. Fix any issues found
6. Commit when complete
7. Update task tracking files

## Your Workflow

When the user runs this command:

1. **Understand the request**
   - User provides either:
     - A task ID from `docs/tasks/index.md` (e.g., "Task 22")
     - A task slug (e.g., "control-simplification")
     - A task file path (e.g., "@docs/tasks/active/22-control-simplification.md")

2. **Delegate to sdd-full-task**
   - Use the Task tool to launch the `sdd-full-task` agent
   - Pass the task identifier to the agent
   - The sdd-full-task agent will:
     - Load task context from files
     - Delegate each subtask to sdd-subtask for implementation
     - Run tests after each subtask
     - Periodically invoke daisysp-code-reviewer
     - Fix any issues found
     - Create git commit when all subtasks complete
     - Update `docs/tasks/<slug>.md` and `docs/tasks/index.md`

3. **Report results**
   - After sdd-full-task completes, summarize:
     - What was implemented
     - Files changed
     - Test results
     - Commit hash
     - Any follow-up items

## Important Notes

- **Never implement code yourself** - delegate to sdd-full-task
- sdd-full-task will not exit until the task is fully complete
- sdd-full-task coordinates multiple sub-agents:
  - `sdd-subtask` for code implementation
  - `daisysp-code-reviewer` for code review
- The full task agent enforces quality gates (tests pass, review passes, etc.)
- If interrupted, sdd-full-task can resume from where it left off

## Task Input Formats

The user can specify a task in multiple ways:

**By ID:**
```
User: "Implement Task 22"
```

**By slug:**
```
User: "Implement control-simplification"
```

**By file reference:**
```
User: "@docs/tasks/active/22-control-simplification.md"
```

**With context:**
```
User: "Continue working on the control simplification task"
```

## Usage Example

```
User: "Implement Task 22"
Assistant: [Launches sdd-full-task agent with task ID 22]
sdd-full-task: [Reads task file, implements subtasks one by one, runs tests, reviews code]
sdd-full-task: "Task 22 complete! 4 files changed, 12 tests passing, committed as abc123f"
Assistant: "Task 22 (Control Simplification) is now complete:
- 4 subtasks implemented
- 4 files modified (ControlProcessor.cpp, ControlState.h, etc.)
- All tests passing
- Code review passed
- Committed: abc123f"
```