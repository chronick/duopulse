# Implement Project Task

## Action
1.  **Read Task**: Read the active task file in `docs/tasks/`.
2.  **Check Status**: Find the first unchecked item `[ ]` in the "Implementation Plan".
3.  **Execute Step**:
    - Understand the goal of the step.
    - Read necessary code files.
    - Implement the change.
    - Write good unit tests. Run tests until all pass with `make test`.
    - Verify everything builds by running `make`.
    - Fix any linting errors that are introduced.
4.  **Update Task**:
    - Check off the item `[x]` in the task file.
    - Add notes or findings if relevant.
    - Store progress and next steps as a comment in #comments

# Notes
Tasks will typically be executed one per session.

The user may ask for additional work, which should be done under this task.
If the user asks for additonal work, reflect it in the task notes, and verify it matches the spec. If it does not, update the spec after confirming with the user.

