# Command: SDD – New Feature

You are helping define a new feature in a Spec-Driven Development repo.

Given:
- A short natural language feature description from the user.

Do the following steps:

1. **Choose a feature slug**
   - Create a URL- and branch-friendly slug, e.g. `password-reset`, `drum-soft-pickup`.

2. **Update or extend the spec**
   - Edit `docs/specs/main.md`.
   - Add or update a section that:
     - Describes the feature from the user's perspective.
     - Lists acceptance criteria as bullet points or a checklist.
   - Use a heading like `## Feature: <Human Title> [<slug>]`.

3. **Create a feature task file**
   - Create or update `docs/tasks/<slug>.md`.
   - Include:
     - Context / motivation (1–3 sentences).
     - A checklist of concrete tasks that can be implemented independently.
     - For each task, reference the relevant spec section.

   Example structure:

   ```markdown
   # Feature: Password Reset [password-reset]

   ## Context
   Users should be able to reset their password via email when they forget it.

   ## Tasks

   - [ ] Update spec in `docs/specs/main.md` (section "Password Reset [password-reset]") with final acceptance criteria.
   - [ ] Backend: add reset token model and persistence.
   - [ ] Backend: add "request password reset" endpoint.
   - [ ] Backend: add "confirm password reset" endpoint.
   - [ ] Email: send reset email via existing email service.
   - [ ] Frontend: add "Forgot password" form and reset form.
   - [ ] Tests: unit tests for token logic and endpoints.
   - [ ] Tests: integration tests for full reset flow.
   - [ ] Docs: briefly document password reset in README or user docs.
   ```

4. **Summarize next steps**
   - At the end of your reply, show:
     - The chosen slug.
     - The path to the tasks file.
     - A suggestion for the feature branch name: `feature/<slug>`.

After this command, the developer may use `./scripts/sdd-create-task <feature-slug> "<task description>"` to register individual tasks in `docs/tasks/index.md`. You do not need to edit `docs/tasks/index.md` in this command.

Never implement code in this command. Only edit spec and tasks.

Usage in Cursor:  
> Run **sdd-new-feature** with: "Add soft-pickup for drum sequencer parameters".

You get: updated `main.md` + `docs/tasks/soft-pickup.md` + branch name suggestion.

