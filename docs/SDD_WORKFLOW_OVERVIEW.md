# Spec-Driven Development (SDD) Workflow Overview

This document summarizes the current Spec-Driven Development workflow used in this project. It is intended to help other agents understand and improve the process.

## 1. Overview

The project follows a rigorous SDD workflow to ensure quality, traceability, and clear communication between the user and the AI agent. The core principle is: **No code is written without a defined specification and a planned task.**

## 2. File Hierarchy

The workflow relies on a specific directory structure:

```text
project-root/
├── .cursor/
│   └── commands/           # Instructions for AI agents (cursor rules)
│       ├── update-spec.md
│       ├── implement-task.md
│       └── ...
├── docs/
│   ├── specs/
│   │   └── main.md         # Single source of truth for project specification
│   ├── tasks/
│   │   ├── active/         # Tasks currently in progress
│   │   ├── backlog/        # Pending tasks
│   │   ├── completed/      # Finished tasks
│   │   └── template.md     # Template for new tasks
│   └── SDD_WORKFLOW.md     # High-level workflow definition
└── ... source code ...
```

## 3. Workflow Steps (End-to-End)

The process is divided into two main phases: **Spec Update** and **Implementation**.

### Phase 1: Spec Update & Task Generation
**Trigger:** User requests a new feature or change.
**Command:** `@update-spec`

1.  **Branching**: Agent creates a short-lived branch `spec/<feature-name>`.
2.  **Spec Modification**: 
    -   Agent analyzes `docs/specs/main.md`.
    -   Updates the spec to reflect the new requirements.
    -   Adds an entry to the `## Changelog` in the spec file.
3.  **Task Creation**:
    -   Agent identifies the gap between the new spec and the current codebase.
    -   Creates new task file(s) in `docs/tasks/backlog/` using `docs/tasks/template.md`.
    -   Tasks are named `XX-task-name.md` (incrementing prefix).
    -   Task includes: Context, Requirements, and detailed Implementation Plan.
4.  **Review**: User reviews the spec diff and the generated tasks.
5.  **Merge**: Upon approval, Agent commits changes, merges to `main`, and deletes the `spec/` branch.

### Phase 2: Implementation
**Trigger:** User wants to start coding.
**Command:** `@implement-task`

1.  **Task Discovery**:
    -   Agent checks `docs/tasks/active/`.
    -   If empty, promotes the highest priority task from `docs/tasks/backlog/` to `active/`.
2.  **Implementation Loop**:
    -   Agent reads the active task.
    -   Iterates through the **Implementation Plan** checklist.
    -   For each step:
        -   Write code & tests.
        -   Verify (`make`, `make test`).
        -   **Commit**: `git commit -m "..."`.
        -   Update task file (mark step as `[x]`).
3.  **Completion**:
    -   When all steps are done, Agent marks task status as `completed`.
    -   Moves task file to `docs/tasks/completed/`.
    -   Commits the documentation change.

## 4. Git Workflow

-   **Spec Branches**: `spec/<feature-name>` (Short-lived, for updating docs/specs and creating tasks).
-   **Implementation**: Directly on `main` (if strictly following atomic tasks) or `feat/<feature-name>` for larger features.
-   **Commits**: Frequent, atomic commits.
    -   *Spec updates*: "docs: update spec and add tasks for X"
    -   *Implementation*: "feat/fix: <step description> (Task XX)"
    -   *Completion*: "docs: complete task XX"

## 5. Agent Commands (.cursor/commands)

The workflow is enforced via specific cursor rules:

-   **`update-spec.md`**: Instructions for the Spec Update phase (Branch -> Update Spec -> Create Tasks -> Merge).
-   **`implement-task.md`**: Instructions for the Implementation phase (Active Task -> Code -> Test -> Commit -> Update Task).
-   **`plan-project-task.md`**: Helper to create a detailed plan for a task if not already fully defined.

## 6. Document Examples

### Specification (`docs/specs/main.md`)
The spec is a living document describing the system behavior.

```markdown
# Firmware Specification: Opinionated Drum Sequencer
...
## Functional Architecture
### Base Mode (Switch OFF)
The primary performance mode...
| Control | Function | Description |
| :--- | :--- | :--- |
| **Knob 1** | **Kick Density** | ... |
...
## Changelog
*   **2025-11-26**:
    *   **Soft Takeover**: Updated behavior...
```

### Task Template (`docs/tasks/template.md`)

```markdown
---
id: chronick/daisysp-idm-grids-{integer auto increment}"
title: "Template Task"
status: "pending" # pending, active, completed, cancelled
created_date: "YYYY-MM-DD"
last_updated: "YYYY-MM-DD"
owner: "user/ai"
---

# Task: [Task Name]

## Context
Link to relevant section in `docs/specs/main.md`.

## Requirements
- [ ] Requirement 1
- [ ] Requirement 2

## Implementation Plan
1. [ ] Step 1
2. [ ] Step 2

## Notes
- Any relevant notes.
```

### Active Task Example

```markdown
# Task: Opinionated Drum Sequencer Implementation

## Context
Implementation of the new "Opinionated Drum Sequencer" spec defined in `docs/specs/main.md`...

## Requirements
- [x] **Soft Takeover**: Implement "soft pickup"...

## Implementation Plan
### 1. Refactor Engine Architecture
- [x] Create `class Parameter`...
### 2. Implement Soft Takeover
- [ ] Create `SoftPickup` utility class...
```

