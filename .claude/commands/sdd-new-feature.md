# Command: SDD – New Feature

You are helping plan and define a new feature in a Spec-Driven Development repo.

## What This Command Does

This command delegates to the **sdd-planner** agent to:
1. Analyze the feature request
2. Read the spec and explore the codebase
3. Create a task breakdown
4. Write task files to `docs/tasks/`
5. Update `docs/tasks/index.md`

## Your Workflow

When the user runs this command:

1. **Understand the request**
   - The user provides a natural language feature description
   - Example: "Add soft-pickup for drum sequencer parameters"

2. **Delegate to sdd-planner**
   - Use the Task tool to launch the `sdd-planner` agent
   - Pass the feature description to the planner
   - The planner will:
     - Choose a feature slug
     - Update or extend `docs/specs/main.md`
     - Create task file(s) in `docs/tasks/`
     - Update `docs/tasks/index.md` with new tasks

3. **Report results**
   - After the planner completes, summarize what was created:
     - The chosen slug
     - Path to task file(s)
     - Suggested feature branch name: `feature/<slug>`
     - Next steps: Use `sdd-full-task` to begin implementation

## Important Notes

- **Never implement code** in this command - that's sdd-full-task's job
- The sdd-planner agent has full access to read specs and explore the codebase
- The planner will ask for confirmation before writing task files
- After planning is complete, suggest using `sdd-full-task` to begin implementation

## Usage Example

```
User: "Add swing timing to the sequencer"
Assistant: [Launches sdd-planner agent]
sdd-planner: [Analyzes spec, explores codebase, creates task breakdown, writes files]
Assistant: "Feature planning complete! Created:
- Slug: swing-timing
- Task file: docs/tasks/swing-timing.md
- Branch: feature/swing-timing
- Next: Use sdd-full-task to begin implementation"
```
