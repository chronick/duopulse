---
description: Update project specifications and generate implementation tasks based on user requirements.
globs: specs/**/*.md, docs/specs/**/*.md
---

# Update Spec

This command updates the project specifications found in the `specs/` or `docs/specs/` directory, following a strict SDD workflow.

## Workflow

1.  **Branching**:
    -   Determine a short, descriptive name for the update (e.g., `chaos-mode`, `clock-fix`).
    -   Checkout a new branch: `git checkout -b spec/<name-of-update>`.

2.  **Update Spec**:
    -   **Analyze Request**: Understand the user's proposed changes or new feature requirements.
    -   **Locate Spec**: Identify the relevant specification file(s) (usually `docs/specs/main.md`).
    -   **Gap Analysis**: Compare the current spec with the requested changes.
    -   **Modify Spec**: Update the markdown content. Ensure consistent formatting and clarity.
    -   **Update Changelog**: Add an entry to the `## Changelog` section in the spec file.

3.  **Generate Tasks**:
    -   Create new task file(s) in `docs/tasks/backlog/` to implement the differences between the current codebase and the new spec.
    -   Use `docs/tasks/template.md` as a base.
    -   Naming convention: `XX-task-name.md` (increment the prefix).
    -   Fill in the `Context`, `Requirements`, and a detailed `Implementation Plan`.

4.  **Review**:
    -   Present the changes (Spec diff + New Tasks) to the user for review.
    -   Ask: "Does this spec update and task list look correct?"

5.  **Commit & Merge**:
    -   Once approved by the user:
    -   `git add docs/specs/ docs/tasks/`
    -   `git commit -m "docs: update spec and add tasks for <name-of-update>"`
    -   `git checkout main`
    -   `git merge spec/<name-of-update>`
    -   `git branch -d spec/<name-of-update>`

## Rules

-   **Precision**: Only update parts of the spec that are affected by the change.
-   **Clarity**: Use clear, concise language suitable for technical documentation.
-   **Changelog**: Every spec update MUST include a changelog entry.
-   **Traceability**: Ensure every new spec requirement maps to a task in `docs/tasks/backlog/`.
