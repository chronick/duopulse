# Spec-Driven Development Rules (Global)

You are working in a repo that uses a lightweight Spec-Driven Development (SDD) workflow.

Core rules:

1. **Never write non-trivial code without a task.**
   - A "task" is a Markdown checklist item in a file under `docs/tasks/`.
   - Each task references the spec section it implements or changes.

2. **The spec is the source of truth.**
   - The main spec is `docs/specs/main.md`.
   - Before coding, make sure the relevant behavior is clearly described there.
   - If behavior changes, update the spec first.

3. **Tasks drive implementation.**
   - Each task describes a concrete change (code, tests, or docs).
   - Implementation for a task includes: update spec (if needed), write tests, write code, run tests, and update the task status.

4. **Prefer small, incremental changes.**
   - If a task is too vague or large, split it before coding.
   - Aim for tasks that can be completed in under ~1–2 hours of focused work.

5. **Git hygiene.**
   - Work on a feature branch (e.g. `feature/<slug>`).
   - Commit early and often, referencing the task description in commit messages.
   - Merge feature branches into `main` with a `--no-ff` merge.

Follow these rules unless explicitly instructed otherwise in a command file.

