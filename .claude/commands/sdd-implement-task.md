# Command: SDD – Implement Task

You are implementing exactly ONE task from a feature task file in `docs/tasks/`.

Inputs (from the user):
- Either a task id from `docs/tasks/index.md`
- Or:
  - feature slug (e.g. `soft-pickup`)
  - the exact task description as it appears in `docs/tasks/<slug>.md`

Always:
- Use `docs/tasks/index.md` if a matching row exists, to read and update metadata.

Steps:

1. **Resolve the task**
   - If a task id is provided:
     - Look up the row in `docs/tasks/index.md` with that id.
     - Determine `feature`, `description`, and `file` from the row.
   - If no id is provided:
     - Use the given feature slug and description to:
       - Open `docs/tasks/<slug>.md`.
       - Confirm the checklist item exists.
       - Optionally cross-check `docs/tasks/index.md` for a matching row (same feature + description).
   - Restate the task in your own words.

2. **Understand context**
   - Read the entire feature task file.
   - Read the relevant section in `docs/specs/main.md` for this feature.
   - If the task is too large, propose a small sub-task breakdown and confirm which one to do.

3. **Plan minimal changes**
   - List the code and test files you will touch.
   - List any doc/spec updates required.

4. **Implement with tests**
   - Prefer writing or updating tests first.
   - Then implement code to satisfy tests and the spec.
   - Keep changes limited to what is necessary for this task.

5. **Run tests (conceptually)**
   - Show commands the user should run (e.g. `pytest`, `cargo test`, `npm test`, `make test`).
   - Assume the user will run them.

6. **Update the feature task file**
   - In `docs/tasks/<slug>.md`, change the corresponding checklist item from `- [ ]` to `- [x]`.
   - Optionally add a brief note under the task with important details or TODOs.

7. **Update the tasks index metadata**
   - In `docs/tasks/index.md`:
     - Find the row for this task (by id if given, otherwise by matching `feature` and `description`).
     - Set `status` to `done`.
     - Update `updated_at` to today's date in ISO format (e.g. `2025-11-27T16:30:00Z`).
     - Do not change `created_at` or the `id`.

8. **Suggest a commit message**
   - Propose a short commit message that includes the feature slug and a hint about the task.

Do not modify unrelated tasks or index rows. Stay within the spec and task description.

Usage in Cursor:

Open docs/tasks/soft-pickup.md, run sdd-implement-task, select an unchecked task, and let it go.

