# Spec-Driven Development (SDD) Workflow

This project uses a lightweight Spec-Driven Development workflow optimized for solo development and small projects.

## Philosophy

- **Solo dev, small projects** → optimize for low overhead, not ceremony.
- **Everything in the repo** → specs, tasks, and rules live with the code.
- **Few mental verbs** → think in 3 steps: "Describe → Plan → Implement"
- **AI as worker, you as editor** → the framework keeps the AI on rails.

## Structure

- **`docs/specs/main.md`**: The single source of truth for the project specification. All features must be defined here before implementation.
- **`docs/tasks/`**: Contains task files for each feature.
  - **`<slug>.md`**: Per-feature task files with context and checklists (e.g., `soft-pickup.md`, `password-reset.md`).
  - **`index.md`**: Global tasks index with metadata (id, feature, description, status, timestamps).
  - Legacy directories (`active/`, `backlog/`, `completed/`) may exist but are not required for new features.

## Core Commands

### 1. New Feature (`sdd-new-feature`)

Use this when you want to add a new feature or change behavior.

**Process:**
1. **Choose feature slug**: Create a URL- and branch-friendly slug (e.g., `soft-pickup`, `drum-soft-pickup`).
2. **Update spec**: Edit `docs/specs/main.md` with a new section describing the feature and acceptance criteria.
3. **Create task file**: Create `docs/tasks/<slug>.md` with context and a checklist of concrete tasks.
4. **Register tasks** (optional): Use `./scripts/sdd-create-task <slug> "<description>"` to add tasks to the index.

**Usage:**
```
In Cursor, run: sdd-new-feature
With description: "Add soft-pickup for drum sequencer parameters"
```

### 2. Implement Task (`sdd-implement-task`)

Use this to implement a single task from a feature file.

**Process:**
1. **Resolve task**: Look up task by id (from `docs/tasks/index.md`) or by feature slug + description.
2. **Understand context**: Read the feature task file and relevant spec section.
3. **Plan changes**: List code, test, and doc files to modify.
4. **Implement with tests**: Write/update tests first, then implement code.
5. **Update task file**: Mark checklist item as `[x]` in `docs/tasks/<slug>.md`.
6. **Update index**: Set status to `done` and update `updated_at` in `docs/tasks/index.md`.
7. **Suggest commit**: Propose a commit message.

**Usage:**
```
In Cursor, run: sdd-implement-task
With: task id #7 or "feature soft-pickup, task: Engine: add soft-pickup parameter handling"
```

## CLI Tools

Three shell scripts help manage tasks:

### `./scripts/sdd-create-task <feature-slug> "<task description>"`
Creates a new task entry in `docs/tasks/index.md` and ensures the checklist item exists in the feature task file.

**Example:**
```bash
./scripts/sdd-create-task soft-pickup "Engine: add soft-pickup parameter handling"
```

### `./scripts/sdd-next-task [feature-slug]`
Shows the next open task (optionally filtered by feature).

**Example:**
```bash
./scripts/sdd-next-task              # First open task across project
./scripts/sdd-next-task soft-pickup  # First open task for a feature
```

### `./scripts/sdd-check-sdd [feature-slug]`
Checks if there are any open tasks remaining (useful before merging).

**Example:**
```bash
./scripts/sdd-check-sdd              # Check all features
./scripts/sdd-check-sdd soft-pickup  # Check specific feature
```

## Standard Feature Loop (Day-to-Day Workflow)

### Adding a New Feature

1. **Create a feature branch**
   ```bash
   slug=soft-pickup
   git checkout -b "feature/$slug"
   ```

2. **Describe the feature**
   - In Cursor, run `sdd-new-feature` with a 1–3 sentence description.
   - Review/adjust `docs/specs/main.md` and `docs/tasks/<slug>.md`.

3. **Register tasks in the index** (optional but recommended)
   ```bash
   ./scripts/sdd-create-task soft-pickup "Engine: add soft-pickup parameter handling"
   ./scripts/sdd-create-task soft-pickup "UI: prevent parameter jumps when crossing previous value"
   ./scripts/sdd-create-task soft-pickup "Tests: cover soft-pickup edge cases"
   ```

### Implementing Work (Solo Day-to-Day Loop)

1. **Pick the next task**
   ```bash
   ./scripts/sdd-next-task soft-pickup
   ```

2. **Implement via Cursor**
   - Open `docs/tasks/<slug>.md` and run `sdd-implement-task` with the task id or description.
   - The command will implement tests + code, update checklists, and sync the index.

3. **Run tests & verify**
   ```bash
   make test
   # or pytest, cargo test, npm test, etc.
   ```

4. **Commit incrementally**
   ```bash
   git commit -am "soft-pickup: add engine parameter soft-pickup behavior (task #7)"
   ```

5. **Repeat** until all important tasks for the feature are done.

### Pre-Merge and Merge

1. **Check SDD state**
   ```bash
   ./scripts/sdd-check-sdd soft-pickup
   ```
   - If open tasks remain, implement them or mark as `dropped` in the index.

2. **Merge with no-ff**
   ```bash
   git checkout main
   git merge --no-ff "feature/soft-pickup"
   git branch -d "feature/soft-pickup"
   ```

## Git Hygiene

- **Feature branches**: Use `feature/<slug>` branches for all feature work.
- **Commits**: Frequent, atomic commits referencing task descriptions.
- **Merge**: Use `--no-ff` merges to preserve feature branch history.

## Task Statuses

- `open`: Task defined but not started.
- `done`: Task completed and verified.
- `dropped`: Task intentionally abandoned (optional).

The index tracks `created_at` and `updated_at` timestamps in ISO format.

## Global Rules

See `.cursor/rules/sdd-global.md` for the complete set of rules that all AI agents follow in this repo.
