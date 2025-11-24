---
description: Update project specifications in specs/**/*.md or docs/specs/**/*.md based on user requirements
globs: specs/**/*.md, docs/specs/**/*.md
---

# Update Spec

This command updates the project specifications found in the `specs/` or `docs/specs/` directory.

## Steps

1.  **Analyze Request**: Understand the user's proposed changes or new feature requirements.
2.  **Locate Spec**: Identify the relevant specification file(s). If a relevant file doesn't exist, propose creating a new one.
3.  **Gap Analysis**: Compare the current spec with the requested changes. Identify what needs to be added, modified, or removed.
4.  **Clarify (Optional)**: If there are ambiguities, conflicts, or missing details, ask the user clarifying questions before proceeding.
5.  **Update Spec**:
    -   Modify the markdown content.
    -   Ensure consistent formatting.
    -   Only touch relevant sections; preserve the rest.
6.  **Update Changelog**:
    -   At the bottom of the spec file, locate or create a `## Changelog` or `## History` section.
    -   Add a new entry with the date (YYYY-MM-DD) and a brief summary of the changes.

## Rules

-   **Precision**: Only update parts of the spec that are affected by the change.
-   **Clarity**: Use clear, concise language suitable for technical documentation.
-   **Format**: Follow the existing markdown structure of the file.
-   **Changelog**: Every spec update MUST include a changelog entry.

