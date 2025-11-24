# Spec-Driven Development (SDD) Workflow

This project uses a lightweight Spec-Driven Development workflow.

## Structure

- **`docs/specs/main.md`**: The single source of truth for the project specification. All features must be defined here before implementation.
- **`docs/tasks/`**: Contains task files for each unit of work (phase, feature, or bugfix).
  - Format: `XX-task-name.md` (e.g., `01-setup-infrastructure.md`).
  - Metadata: Frontmatter with status, id, etc.

## Workflow

1.  **Update Spec**: Modify `docs/specs/main.md` to reflect the desired state of the system.
2.  **Create Task**: Create a new file in `docs/tasks/` (copy `docs/tasks/template.md`).
3.  **Plan**: Break down the task into implementation steps.
4.  **Implement**: Execute the steps, updating code and the task file.
5.  **Verify**: Ensure requirements are met.
6.  **Complete**: Update task status to `completed`.

## Cursor "Commands"

Use these phrases to trigger specific AI behaviors:

### `plan-project-task`

**Trigger**: User says "plan-project-task" (optionally with a task name/ID).

**AI Action**:
1.  Read `docs/specs/main.md`.
2.  List files in `docs/tasks/` to identify the current or next task.
3.  If a task is specified or implied, read that task file.
4.  If the task is new or empty, populate it with:
    - Context from the Spec.
    - Specific requirements.
    - A detailed implementation plan (checklist).
5.  Ask for user approval of the plan.

### `implement-task`

**Trigger**: User says "implement-task" (optionally with a task name/ID).

**AI Action**:
1.  Read the specified (or active) task file in `docs/tasks/`.
2.  Check the "Implementation Plan" section.
3.  Execute the next unchecked items in the plan.
    - This involves reading code, writing code, and running tests.
4.  Update the task file (mark items as checked) as progress is made.
5.  If blocked or finished with a logical chunk, stop and report status.

## Statuses
- `pending`: Task defined but not started.
- `active`: Currently being worked on.
- `completed`: Finished and verified.
- `cancelled`: Abandoned.

