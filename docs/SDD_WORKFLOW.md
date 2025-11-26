# Spec-Driven Development (SDD) Workflow

This project uses a rigorous Spec-Driven Development workflow to ensure quality and traceability.

## Structure

- **`docs/specs/main.md`**: The single source of truth for the project specification. All features must be defined here before implementation.
- **`docs/tasks/`**: Contains task files for each unit of work.
  - **`active/`**: Tasks currently being worked on (limit to 1-2).
  - **`backlog/`**: Pending tasks scheduled for the future.
  - **`completed/`**: Finished and verified tasks.
  - **`template.md`**: Template for new tasks.
  - Format: `XX-task-name.md` (e.g., `01-setup-infrastructure.md`).

## Workflow Commands

### 1. Update Spec (`@update-spec`)

Use this when you want to change behavior or add features.

**Process:**
1.  **Branch**: Agent creates `spec/<feature-name>`.
2.  **Spec**: Agent updates `docs/specs/main.md` with new requirements.
3.  **Tasks**: Agent generates implementation tasks in `docs/tasks/backlog/`.
4.  **Review**: User reviews the spec diff and task list.
5.  **Merge**: Agent commits, merges to `main`, and deletes the branch.

### 2. Implement Task (`@implement-task`)

Use this to execute the planned work.

**Process:**
1.  **Discover**: 
    -   Agent checks `docs/tasks/active/`.
    -   If empty, Agent checks `docs/tasks/backlog/`, picks the next priority task, and moves it to `docs/tasks/active/`.
2.  **Loop**: For each step in the task's Implementation Plan:
    -   Implement code & tests.
    -   Verify (`make`, `make test`).
    -   **Commit**: `git commit -m "..."`.
    -   Update task file (mark checked).
3.  **Complete**: 
    -   Agent marks task as `completed`.
    -   Agent moves task to `docs/tasks/completed/`.
    -   Agent commits.

## Git Hygiene

-   **Spec Updates**: Done on short-lived `spec/*` branches.
-   **Implementation**: Can be done on `main` (if strictly following tasks) or `feat/*` branches.
-   **Commits**: Frequent, atomic commits after each successful step in a task.

## Statuses
- `pending`: Task defined but not started.
- `in_progress`: Currently being worked on.
- `completed`: Finished and verified.
- `cancelled`: Abandoned.
