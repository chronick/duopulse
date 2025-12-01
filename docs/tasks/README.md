# Tasks Directory

This directory contains task files for features and a global tasks index.

## Structure

- **`<slug>.md`**: Per-feature task files (e.g., `soft-pickup.md`, `password-reset.md`)
  - Each file contains context, motivation, and a checklist of concrete tasks
  - Tasks reference relevant sections in `docs/specs/main.md`
  - Format: Simple markdown with `- [ ]` checkboxes

- **`index.md`**: Global tasks index with metadata
  - Tracks all tasks across features with id, status, timestamps
  - Used by CLI scripts (`sdd-next-task`, `sdd-check-sdd`)
  - Format: Markdown table

## Legacy Directories

The following directories exist for historical tasks but are not required for new features:
- `active/`: Previously used for tasks in progress
- `backlog/`: Previously used for pending tasks
- `completed/`: Previously used for finished tasks

New features should use the simplified workflow:
1. Create feature task file: `docs/tasks/<slug>.md`
2. Register tasks in index: `./scripts/sdd-create-task <slug> "<description>"`
3. Implement tasks: Use `sdd-implement-task` command in Cursor

## Task File Format

```markdown
# Feature: Soft Pickup [soft-pickup]

## Context
Users should be able to turn knobs without abrupt parameter jumps.

## Tasks

- [ ] Update spec in `docs/specs/main.md` (section "Soft Pickup [soft-pickup]")
- [ ] Engine: add soft-pickup parameter handling
- [ ] UI: prevent parameter jumps when crossing previous value
- [ ] Tests: cover soft-pickup edge cases
```

## Index Format

The `index.md` file uses a markdown table:

| id | feature | description | status | created_at | updated_at | file |
|----|---------|-------------|--------|------------|------------|------|
| 1 | soft-pickup | Engine: add soft-pickup parameter handling | open | 2025-11-27T16:30:00Z | 2025-11-27T16:30:00Z | docs/tasks/soft-pickup.md |

Status values: `open`, `done`, `dropped`

