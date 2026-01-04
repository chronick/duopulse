---
description: Start or continue a design review session - accepts inline feedback or file path
agent: designer
---

# Design Review Session

You are starting a design review session. Your mission is to analyze an existing implementation and iterate toward an improved design based on user feedback.

## Input Provided

$ARGUMENTS

## Input Handling

First, determine what input was provided:

### Case 1: File path provided
If `$ARGUMENTS` looks like a file path (contains `/` or ends in `.md`):
```bash
# Check if it's a feedback file
cat "$ARGUMENTS"
```
Parse this as structured feedback input.

### Case 2: Inline feedback provided
If `$ARGUMENTS` contains descriptive text about pain points, desired improvements, or design concerns, treat this as Round 1 feedback.

### Case 3: Session slug only
If `$ARGUMENTS` is a simple slug like `control-layout-v2`, check for existing session:
```bash
ls docs/design/$ARGUMENTS/
```
If session exists, continue from where it left off. If not, prompt for more context.

### Case 4: No input or unclear
If `$ARGUMENTS` is empty or unclear, use AskUserQuestion to gather:

1. **What are we designing?** (feature area, component, workflow)
2. **What's the pain point?** (current problems, user complaints)
3. **What does success look like?** (concrete outcomes)

## Session Setup

### Create or Resume Session Directory

```bash
# Create session directory if new
mkdir -p docs/design/<session-slug>/

# Initialize README if new session
cat > docs/design/<session-slug>/README.md << 'EOF'
# Design Session: <Session Name>

**Created**: YYYY-MM-DD
**Status**: Active
**Focus**: <What we're redesigning>

## Problem Statement

<Pain points and motivation for redesign>

## Constraints

<Hard limits that must be respected>

## Success Criteria

<How we'll know the design is good>
EOF
```

### Document Feedback

Write user feedback to the session:
```bash
cat >> docs/design/<session-slug>/feedback.md << 'EOF'
## Feedback Round N - YYYY-MM-DD

<User's feedback>

### Key Themes
- <theme 1>
- <theme 2>
EOF
```

## Design Workflow

Once input is gathered, follow the standard designer workflow:

### 1. Autonomous Analysis
- Read the current implementation
- Activate domain skills (`eurorack-ux`, `daisysp-review`)
- Form initial hypotheses
- Document findings in `iteration-N.md`

### 2. Codex Consultation
Query Codex for alternative perspectives:
```markdown
Context: [Implementation summary]
Problem: [What needs improvement]
User feedback: [Key themes from feedback]
Constraints: [Hard limits]

Request: Propose 2-3 alternative approaches.
```

Save exchange to `codex-input.md` and `codex-output.md`.

### 3. Synthesize Design
Create `iteration-N.md` with:
- Analysis of current state
- User feedback incorporated
- Codex alternatives considered
- Recommended approach
- Trade-offs acknowledged

### 4. Request Critique (Optional)
If the design is significant, suggest:
> "Would you like me to run the design-critic agent on this iteration?"

## Output

At the end of each session, report:
1. Session location: `docs/design/<slug>/`
2. Current iteration number
3. Key design decisions made
4. Open questions for next round
5. Suggested next steps

## Examples

### Example 1: Inline feedback
```
/design-review The control layout feels cramped and I keep hitting the wrong knob during performance
```
→ Creates session, documents feedback, begins analysis

### Example 2: Feedback file
```
/design-review docs/design/control-layout-v2/user-notes.md
```
→ Reads file, continues session

### Example 3: Resume session
```
/design-review control-layout-v2
```
→ Checks existing session, continues from last iteration

### Example 4: Fresh start
```
/design-review
```
→ Prompts for design focus and pain points
