# Implement Project Task

## Action

1.  **Task Discovery**:
    -   **Check Active**: Look for files in `docs/tasks/active/`.
    -   **Promote Backlog**: If no active tasks, list files in `docs/tasks/backlog/`.
        -   Select the next task (lowest number/highest priority).
        -   Move it to `docs/tasks/active/`.
    -   **Read Task**: Read the active task file.

2.  **Implementation Loop**:
    -   **Check Status**: Find the first unchecked item `[ ]` in the "Implementation Plan".
    -   **Context**: Read necessary code files to understand the current state.
    -   **Execute Step**:
        -   Implement the change in the codebase.
        -   Write/Update unit tests.
        -   Run tests: `make test`.
        -   Verify build: `make`.
        -   Fix lints.
    -   **Git Hygiene**:
        -   After a successful step (builds and tests pass), commit the changes.
        -   `git add .`
        -   `git commit -m "feat/fix: <brief description of step> (Task XX)"`
    -   **Update Task**:
        -   Mark the item as checked `[x]` in the task file.
        -   Add notes or findings if relevant.

3.  **Completion**:
    -   When all items are checked:
    -   Update task status to `completed` in the frontmatter.
    -   Move task to `docs/tasks/completed/`.
    -   `git add docs/tasks/`
    -   `git commit -m "docs: complete task XX"`

## Notes

-   Tasks should be executed systematically.
-   If additional work is requested that is NOT in the plan, update the Spec and Task first (using `@update-spec`).
-   Always verify `make` and `make test` before committing.
