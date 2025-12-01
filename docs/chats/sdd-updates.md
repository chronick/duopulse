Nice, this is exactly the kind of thing that pays off on every project once it’s dialed in.

Below is a trimmed-down SDD workflow you can drop into any new repo. It keeps your current ideas (specs, tasks, Cursor commands) but:
	•	Reduces moving parts
	•	Uses just two main commands
	•	Works for any stack (Python / TS / Rust / C++)

You can copy this into a template repo and tweak per project.

⸻

0. Goals / Philosophy
	•	Solo dev, small projects → optimize for low overhead, not ceremony.
	•	Everything in the repo → specs, tasks, and rules live with the code.
	•	Few mental verbs → think in 3 steps:
“Describe → Plan → Implement”
	•	AI as worker, you as editor → the framework keeps the AI on rails.

⸻

1. Minimal Repo Layout

Use this as your baseline tree for new projects:

project-root/
├── .cursor/
│   ├── rules/
│   │   └── sdd-global.md          # High-level SDD rules for all agents
│   └── commands/
│       ├── sdd-new-feature.md     # Spec + tasks creator
│       └── sdd-implement-task.md  # Single-task implementation loop
├── docs/
│   ├── specs/
│   │   └── main.md                # Living system spec
│   └── tasks/
│       └── README.md              # How task files work
├── src/                           # Or app/, lib/, etc.
├── tests/                         # Or test/, specs/, etc. per language
└── SDD_WORKFLOW.md                # Short version of this doc for the repo

You can keep your old “Opinionated Drum Sequencer” example as an appendix or example file in docs/examples/ if you want, but don’t require it for new projects.

⸻

2. Global Rules for Agents

File: .cursor/rules/sdd-global.md

Purpose: give any agent (Cursor, Claude, etc.) a single, simple doctrine.

Example content (short and reusable):

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

That’s it. One rule file instead of a forest of instructions.

⸻

3. Two Core Commands (Cursor)

You had multiple commands (update-spec, implement-task, plan-project-task). For solo work, you can collapse that into two:

3.1 sdd-new-feature.md – Spec + Tasks

File: .cursor/commands/sdd-new-feature.md
Goal: Given a rough feature description, update the spec and create a task file.

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

	4.	Summarize next steps
	•	At the end of your reply, show:
	•	The chosen slug.
	•	The path to the tasks file.
	•	A suggestion for the feature branch name: feature/<slug>.

Never implement code in this command. Only edit spec and tasks.

Usage in Cursor:  
> Run **sdd-new-feature** with: “Add soft-pickup for drum sequencer parameters”.

You get: updated `main.md` + `docs/tasks/soft-pickup.md` + branch name suggestion.

---

### 3.2 `sdd-implement-task.md` – One Task at a Time

**File:** `.cursor/commands/sdd-implement-task.md`  
**Goal:** Take a single unchecked task from a feature file and do the full implement-with-tests loop.

```markdown
# Command: SDD – Implement Task

You are implementing exactly ONE task from a feature task file in `docs/tasks/`.

Given:
- The path to a task file (e.g. `docs/tasks/password-reset.md`).
- The user may also paste or reference a specific unchecked task line.

Follow this loop:

1. **Understand the task**
   - Read the entire task file and the referenced section(s) of `docs/specs/main.md`.
   - Restate the task in your own words.
   - If the task is too large or vague, propose a short list of sub-tasks and ask the user to pick one (or confirm all are OK).

2. **Plan minimal changes**
   - Before editing files, list the concrete changes you will make:
     - Code files to modify or create.
     - Test files to modify or create.
     - Any spec or docs updates, if required.

3. **Implement with tests**
   - Write or update tests first, when practical.
   - Then implement the code to satisfy the tests and the spec.
   - Keep changes limited to what is necessary for this task.
   - Use existing patterns and style from the repo.

4. **Run tests (conceptually)**
   - Assume the user will run tests locally.
   - Print a short command list for them to run, e.g.:
     - `pytest`
     - `cargo test`
     - `npm test`

5. **Update the task file**
   - Edit `docs/tasks/<slug>.md`:
     - Mark the implemented task as `[x]`.
     - If you created sub-tasks, mark the completed ones.
     - Add a one-line note under the task with any relevant details or TODOs.

6. **Suggest a commit message**
   - Propose a short, descriptive commit message that references the feature slug and the task.

Do not edit unrelated tasks. Do not add speculative features. Stay within the spec.

Usage in Cursor:

Open docs/tasks/soft-pickup.md, run sdd-implement-task, select an unchecked task, and let it go.

⸻

4. Standard Feature Loop (Your Day-to-Day Checklist)

This is the workflow you can memorize and apply everywhere:
	1.	Create a branch
	•	Decide feature name → slug.
	•	git checkout -b feature/<slug>
	2.	Describe the feature
	•	In Cursor, run sdd-new-feature with a 1–3 sentence description.
	•	Review/adjust docs/specs/main.md and docs/tasks/<slug>.md.
	3.	Implement tasks one by one
	•	Pick a task from docs/tasks/<slug>.md.
	•	Run sdd-implement-task in Cursor.
	•	Review diffs, run tests, tweak as needed.
	•	Repeat until all essential tasks are [x].
	4.	Evaluate the feature
	•	Manually run the app / script.
	•	Verify against the acceptance criteria in the spec.
	5.	Merge cleanly
	•	git status → ensure only relevant files changed.
	•	git commit with clear messages (AI can propose).
	•	git checkout main
	•	git merge --no-ff feature/<slug>
	•	git branch -d feature/<slug>
	6.	Keep spec as source of truth
	•	If behavior changed during implementation, update docs/specs/main.md now.
	•	Optionally add a tiny “Changelog” section in the spec for major features.

⸻

5. Lightweight Project Init Checklist

For a new repo, your personal setup flow can be:
	1.	Scaffold project
	•	Create language-specific boilerplate (or use cargo new, npm init, etc.).
	•	Add src/, tests/ as appropriate.
	2.	Drop in SDD scaffold
	•	Create directories and files:

mkdir -p .cursor/rules .cursor/commands docs/specs docs/tasks
touch docs/specs/main.md docs/tasks/README.md .cursor/rules/sdd-global.md
touch .cursor/commands/sdd-new-feature.md .cursor/commands/sdd-implement-task.md


	•	Paste the templates above into the relevant files.
	•	Add a short SDD_WORKFLOW.md that explains the 4-step loop.

	3.	Write a minimal system spec
	•	In docs/specs/main.md, write:

# System Spec

This document describes the expected behavior of this project.

## Current Features

(empty – to be filled by `sdd-new-feature`)

## Non-Functional Requirements

- Single developer workflow.
- Primary languages: (Python / TypeScript / Rust / C++).
- Testing: (pytest / jest / cargo test / etc.).


	4.	Commit the scaffold
	•	git add .
	•	git commit -m "Initialize SDD workflow scaffold"

Now any new feature is just: branch → sdd-new-feature → sdd-implement-task loops → merge.

⸻

6. Optional “Nice to Have” Extensions (When You Feel Like It)

You can layer these on later without changing the core loop:
	•	Per-language snippets in .cursor/rules (e.g. python-style.md, ts-style.md) and include them in your Cursor config for style consistency.
	•	A tiny CLI script (sdd Python or Node script) that:
	•	Creates docs/tasks/<slug>.md if missing.
	•	Opens the file and prints next unchecked task.
	•	A FEATURES.md index that lists all docs/tasks/*.md with links (can be AI-generated occasionally).
	•	A check-sdd script that fails CI if there are unchecked tasks in a feature that was merged (later, if you add simple CI).

But you don’t need any of that to get started.

⸻

If you’d like, next step I can help you turn this into an actual starter repo layout (e.g. for Python or TS) with all these files pre-filled, so you can just git clone / degit and go.

Love this direction – this is exactly the level of “just enough structure” that will actually get used.

Below is an updated version of your SDD workflow with:
	•	A tasks index (docs/tasks/index.md) that holds metadata.
	•	Three shell scripts:
	•	scripts/sdd-create-task
	•	scripts/sdd-next-task
	•	scripts/sdd-check-sdd
	•	Updated command specs so the AI keeps the index in sync.

I’ll show:
	1.	Updated repo layout
	2.	Tasks index format + metadata
	3.	Shell scripts (POSIX sh)
	4.	Tweaked Cursor commands
	5.	Updated day-to-day workflow

You can just paste these into your starter template.

⸻

1. Updated Repo Layout

project-root/
├── .cursor/
│   ├── rules/
│   │   └── sdd-global.md          # High-level SDD rules for all agents
│   └── commands/
│       ├── sdd-new-feature.md     # Spec + tasks creator
│       └── sdd-implement-task.md  # Single-task implementation loop
├── docs/
│   ├── specs/
│   │   └── main.md                # Living system spec
│   └── tasks/
│       ├── index.md               # GLOBAL tasks index with metadata
│       └── <slug>.md             # Per-feature task files
├── scripts/
│   ├── sdd-create-task            # CLI: create indexed task
│   ├── sdd-next-task              # CLI: show next open task
│   └── sdd-check-sdd              # CLI: check for open tasks
├── src/
├── tests/
└── SDD_WORKFLOW.md                # Short explainer for the repo


⸻

2. Tasks Index + Metadata

File: docs/tasks/index.md

This is your single source of metadata for tasks: status, created_at, updated_at.

# Tasks Index

| id | feature | description | status | created_at | updated_at | file |
|----|---------|-------------|--------|------------|------------|------|

	•	id: integer, unique across the whole project.
	•	feature: feature slug (e.g. soft-pickup, password-reset).
	•	description: short description matching the checklist item in the feature’s task file.
	•	status: "open" | "in_progress" | "done" | "dropped" (you can start with just open/done if you like).
	•	created_at / updated_at: ISO UTC timestamps, e.g. 2025-11-27T16:30:00Z.
	•	file: path to the feature task file, e.g. docs/tasks/soft-pickup.md.

The CLI scripts will append and read from this table. The AI (via sdd-implement-task) will update status + timestamps when it completes a task.

Per-feature task files (e.g. docs/tasks/soft-pickup.md) stay human-friendly Markdown: context + checklist.

⸻

3. Shell Scripts (POSIX sh)

Put these in scripts/ and chmod +x them:

chmod +x scripts/sdd-create-task scripts/sdd-next-task scripts/sdd-check-sdd

3.1 scripts/sdd-create-task

Usage:

./scripts/sdd-create-task <feature-slug> <task description...>
# e.g.
./scripts/sdd-create-task soft-pickup "Engine: add soft-pickup parameter handling"

Script:

#!/usr/bin/env sh
set -eu

INDEX="docs/tasks/index.md"
TASKS_DIR="docs/tasks"

if [ $# -lt 2 ]; then
  echo "Usage: $0 <feature-slug> <task description...>" >&2
  exit 1
fi

feature="$1"
shift
desc="$*"
task_file="$TASKS_DIR/$feature.md"
now="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

# Ensure tasks dir
mkdir -p "$TASKS_DIR"

# Ensure index with header
if [ ! -f "$INDEX" ]; then
  cat >"$INDEX" <<'EOF'
# Tasks Index

| id | feature | description | status | created_at | updated_at | file |
|----|---------|-------------|--------|------------|------------|------|
EOF
fi

# Compute next id
last_id="$(
  awk -F'|' '
    NR>2 && $2 ~ /[0-9]/ {
      gsub(/ /, "", $2);
      if ($2 > max) max = $2;
    }
    END {
      if (max == "") print 0; else print max;
    }
  ' "$INDEX"
)"
id=$((last_id + 1))

# Append row to index
echo "| $id | $feature | $desc | open | $now | $now | $task_file |" >>"$INDEX"

# Ensure feature task file exists
if [ ! -f "$task_file" ]; then
  cat >"$task_file" <<EOF
# Feature: $feature [$feature]

## Context
TODO: describe the motivation and behavior for this feature.

## Tasks

EOF
fi

# Append checklist item if not already present
if ! grep -Fq "$desc" "$task_file"; then
  printf '%s\n' "- [ ] $desc" >>"$task_file"
fi

echo "Created task #$id for feature '$feature'"
echo "  description: $desc"
echo "  file:        $task_file"


⸻

3.2 scripts/sdd-next-task

Usage:

./scripts/sdd-next-task          # first open task across project
./scripts/sdd-next-task soft-pickup  # first open task for a feature

Script:

#!/usr/bin/env sh
set -eu

INDEX="docs/tasks/index.md"

feature_filter=""
if [ $# -ge 1 ]; then
  feature_filter="$1"
fi

if [ ! -f "$INDEX" ]; then
  echo "No tasks index at $INDEX" >&2
  exit 1
fi

awk -v feature="$feature_filter" -F'|' '
  NR <= 2 { next }  # skip header lines
  {
    # columns: 1="", 2=id, 3=feature, 4=description, 5=status, 6=created_at, 7=updated_at, 8=file, 9=""
    id=$2; f=$3; desc=$4; status=$5; created=$6; updated=$7; file=$8;
    gsub(/^ +| +$/, "", id);
    gsub(/^ +| +$/, "", f);
    gsub(/^ +| +$/, "", desc);
    gsub(/^ +| +$/, "", status);
    gsub(/^ +| +$/, "", created);
    gsub(/^ +| +$/, "", updated);
    gsub(/^ +| +$/, "", file);

    if (status == "open" && (feature == "" || f == feature)) {
      print id "\n" f "\n" desc "\n" status "\n" created "\n" updated "\n" file;
      exit;
    }
  }
' "$INDEX" | {
  if read -r id &&
     read -r f &&
     read -r desc &&
     read -r status &&
     read -r created &&
     read -r updated &&
     read -r file; then

    echo "Next task:"
    echo "  id:          $id"
    echo "  feature:     $f"
    echo "  description: $desc"
    echo "  status:      $status"
    echo "  created_at:  $created"
    echo "  updated_at:  $updated"
    echo "  file:        $file"
    echo
    echo "Suggested: open '$file' and run the SDD 'implement task' command in your AI editor."
  else
    if [ -n "$feature_filter" ]; then
      echo "No open tasks found for feature '$feature_filter'."
    else
      echo "No open tasks found."
    fi
    exit 1
  fi
}


⸻

3.3 scripts/sdd-check-sdd

Usage (ideal for pre-merge or CI):

./scripts/sdd-check-sdd
./scripts/sdd-check-sdd soft-pickup  # optional: check only a feature

Script:

#!/usr/bin/env sh
set -eu

INDEX="docs/tasks/index.md"

feature_filter=""
if [ $# -ge 1 ]; then
  feature_filter="$1"
fi

if [ ! -f "$INDEX" ]; then
  echo "No tasks index at $INDEX – treating as no tasks."
  exit 0
fi

if [ -n "$feature_filter" ]; then
  open_count="$(
    awk -v feature="$feature_filter" -F'|' '
      NR>2 {
        f=$3; status=$5;
        gsub(/^ +| +$/, "", f);
        gsub(/^ +| +$/, "", status);
        if (f == feature && status == "open") c++;
      }
      END { print (c == "" ? 0 : c) }
    ' "$INDEX"
  )"
else
  open_count="$(
    awk -F'|' '
      NR>2 {
        status=$5;
        gsub(/^ +| +$/, "", status);
        if (status == "open") c++;
      }
      END { print (c == "" ? 0 : c) }
    ' "$INDEX"
  )"
fi

if [ "$open_count" -gt 0 ]; then
  if [ -n "$feature_filter" ]; then
    echo "SDD check failed: $open_count open task(s) remain for feature '$feature_filter' in $INDEX."
  else
    echo "SDD check failed: $open_count open task(s) remain in $INDEX."
  fi
  echo "Use './scripts/sdd-next-task${feature_filter:+ $feature_filter}' to see the next task."
  exit 1
else
  if [ -n "$feature_filter" ]; then
    echo "SDD check passed: no open tasks for feature '$feature_filter'."
  else
    echo "SDD check passed: no open tasks in $INDEX."
  fi
  exit 0
fi


⸻

4. Tweaked Cursor Commands (to work with metadata)

4.1 sdd-new-feature.md (minor tweak)

You can keep most of what we had – just mention the index so the agent knows it exists, but you still create index rows via the CLI for simplicity.

Add a note like this at the end of step 3:

After this command, the developer may use ./scripts/sdd-create-task <feature-slug> "<task description>" to register individual tasks in docs/tasks/index.md. You do not need to edit docs/tasks/index.md in this command.

That keeps sdd-new-feature focused on spec + per-feature tasks file, with you deciding which bullets deserve indexed tracking.

4.2 sdd-implement-task.md (new “sync index” step)

Update the command to know about the index metadata.

Key changes:
	•	Allow a task id (from docs/tasks/index.md) or a description + feature slug.
	•	At the end, update the index row: status → done, updated_at → today.

You can adapt your earlier sdd-implement-task to something like:

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
   - Show commands the user should run (e.g. `pytest`, `cargo test`, `npm test`).
   - Assume the user will run them.

6. **Update the feature task file**
   - In `docs/tasks/<slug>.md`, change the corresponding checklist item from `- [ ]` to `- [x]`.
   - Optionally add a brief note under the task with important details or TODOs.

7. **Update the tasks index metadata**
   - In `docs/tasks/index.md`:
     - Find the row for this task (by id if given, otherwise by matching `feature` and `description`).
     - Set `status` to `done`.
     - Update `updated_at` to today’s date in ISO format (e.g. `2025-11-27T16:30:00Z`).
     - Do not change `created_at` or the `id`.

8. **Suggest a commit message**
   - Propose a short commit message that includes the feature slug and a hint about the task.

Do not modify unrelated tasks or index rows. Stay within the spec and task description.

That’s enough for the agent to keep status and updated_at in sync with the code.

⸻

5. Updated Day-to-Day Workflow (with Scripts + Index)

Here’s the version you can drop into SDD_WORKFLOW.md as the “how I work” section.

5.1 Adding a New Feature
	1.	Create a feature branch

slug=soft-pickup
git checkout -b "feature/$slug"

	2.	Describe the feature

In Cursor, run the SDD – New Feature command:

“Add soft-pickup behavior for drum sequencer parameters to avoid abrupt jumps when turning knobs.”

It will:
	•	Update docs/specs/main.md with a ## Feature: Soft Pickup [soft-pickup] section + acceptance criteria.
	•	Create docs/tasks/soft-pickup.md with context and a checklist of tasks.

	3.	Register tasks in the index (with metadata)

For each checklist item you actually want to track:

./scripts/sdd-create-task soft-pickup "Engine: add soft-pickup parameter handling"
./scripts/sdd-create-task soft-pickup "UI: prevent parameter jumps when crossing previous value"
./scripts/sdd-create-task soft-pickup "Tests: cover soft-pickup edge cases"

This:
	•	Appends rows to docs/tasks/index.md with status=open, created_at, updated_at.
	•	Ensures the bullet exists in docs/tasks/soft-pickup.md.

Now index.md is your metadata snapshot for all tasks.

⸻

5.2 Implementing Work (Solo Day-to-Day Loop)
	1.	Pick the next task

./scripts/sdd-next-task soft-pickup
# or just:
./scripts/sdd-next-task

This prints something like:

Next task:
  id:          7
  feature:     soft-pickup
  description: Engine: add soft-pickup parameter handling
  status:      open
  created_at:  2025-11-27T16:30:00Z
  updated_at:  2025-11-27T16:30:00Z
  file:        docs/tasks/soft-pickup.md

	2.	Implement via Cursor

Open docs/tasks/soft-pickup.md and run your SDD – Implement Task command with the task id or description:

“Implement task #7 (Engine: add soft-pickup parameter handling) for feature soft-pickup.”

The command will:
	•	Read the spec and task file.
	•	Implement tests + code.
	•	Mark the checklist item checked in docs/tasks/soft-pickup.md.
	•	Update the matching row in docs/tasks/index.md to status=done and update updated_at.

	3.	Run tests & try it

# whatever is appropriate:
pytest
# or
cargo test
# or
npm test

If it’s a UI feature, actually click through and sanity-check against the spec’s acceptance criteria.
	4.	Commit incrementally

Use the commit message suggested by the command, or write your own, e.g.:

git commit -am "soft-pickup: add engine parameter soft-pickup behavior (task #7)"

Repeat the next-task → implement → test → commit cycle until all important tasks for the feature are done.

⸻

5.3 Pre-Merge and Merge

Before merging the feature branch:
	1.	Check SDD state

./scripts/sdd-check-sdd soft-pickup
# or, if you want the whole project clean:
./scripts/sdd-check-sdd

	•	If it reports open tasks, you can:
	•	Implement them, or
	•	Mark them dropped manually in the index (and update checkboxes in the task file), if they’re intentionally out of scope.

	2.	Merge with no-ff

git checkout main
git merge --no-ff "feature/soft-pickup"
git branch -d "feature/soft-pickup"

Now main contains:
	•	Spec updated in docs/specs/main.md.
	•	Tasks completed (and indexed) in docs/tasks/soft-pickup.md + index.md.
	•	Code + tests.

⸻

This gives you:
	•	Lightweight core loop (branch → spec/tasks → implement tasks → merge).
	•	CLI glue that keeps tasks discoverable (sdd-next-task) and enforceable (sdd-check-sdd).
	•	Structured metadata (status, created_at, updated_at) without adding a ton of ceremony.

If you want, next step we can:
	•	Turn this into a small init-sdd.sh that scaffolds docs/, .cursor/, and scripts/ in any new repo.
	•	Or produce a sample repo layout for one of your main stacks (Python or TS) with this baked in.