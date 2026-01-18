---
id: 64
slug: claude-permissions
title: "Claude Permissions Update"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/claude-permissions
spec_refs: []
depends_on: []
---

# Task 64: Claude Permissions Update

## Objective

Update `claude.yml` GitHub Actions workflow permissions to allow Claude to create branches, push code, and create pull requests. This is required for the `/iterate` command to work.

## Context

### Background

The design review (2026-01-18) identified that the current `claude.yml` workflow has read-only permissions, preventing Claude from:
- Pushing code changes
- Creating feature branches
- Creating pull requests
- Updating issues

### Current State

```yaml
permissions:
  contents: read
  pull-requests: read
  issues: read
  id-token: write
  actions: read
```

### Target State

```yaml
permissions:
  contents: write        # Push branches, commit code
  pull-requests: write   # Create and update PRs
  issues: write          # Update issue comments, labels
  id-token: write
  actions: read
```

### Security Considerations

This is a **privilege escalation**. The existing safeguards are:

1. **Author Association Check**: Only OWNER, MEMBER, or COLLABORATOR can trigger Claude
2. **Repository Scoped**: Permissions only apply to this repository
3. **No Force Push**: Regular push doesn't allow rewriting history
4. **PR Review Required**: Claude creates PRs, doesn't merge directly

These safeguards are sufficient for a private/small-team repo. For public repos with many contributors, consider using a dedicated GitHub App with more granular permissions.

## Subtasks

### Permission Update
- [ ] Update `contents: read` to `contents: write`
- [ ] Update `pull-requests: read` to `pull-requests: write`
- [ ] Update `issues: read` to `issues: write`
- [ ] Keep existing safeguards (author_association check)

### Documentation
- [ ] Add comment explaining permission rationale
- [ ] Document security model in workflow file
- [ ] Update CLAUDE.md if needed

### Validation
- [ ] Test that Claude can create a branch
- [ ] Test that Claude can push commits
- [ ] Test that Claude can create a PR
- [ ] Verify author_association check still works

## Acceptance Criteria

- [ ] Claude can push to feature branches
- [ ] Claude can create pull requests
- [ ] Claude can update issues
- [ ] Unauthorized users still cannot trigger Claude
- [ ] Security model documented

## Implementation Notes

### Updated Workflow

```yaml
# .github/workflows/claude.yml
name: Claude Code

on:
  issue_comment:
    types: [created]
  pull_request_review_comment:
    types: [created]
  issues:
    types: [opened, assigned]
  pull_request_review:
    types: [submitted]

jobs:
  claude:
    # Security: Only allow OWNER, MEMBER, or COLLABORATOR to trigger Claude
    # This prevents unauthorized users from burning API credits or making changes
    if: |
      (
        (github.event_name == 'issue_comment' && contains(github.event.comment.body, '@claude')) ||
        (github.event_name == 'pull_request_review_comment' && contains(github.event.comment.body, '@claude')) ||
        (github.event_name == 'pull_request_review' && contains(github.event.review.body, '@claude')) ||
        (github.event_name == 'issues' && (contains(github.event.issue.body, '@claude') || contains(github.event.issue.title, '@claude')))
      ) && (
        github.event.comment.author_association == 'OWNER' ||
        github.event.comment.author_association == 'MEMBER' ||
        github.event.comment.author_association == 'COLLABORATOR' ||
        github.event.issue.author_association == 'OWNER' ||
        github.event.issue.author_association == 'MEMBER' ||
        github.event.issue.author_association == 'COLLABORATOR' ||
        github.event.review.author_association == 'OWNER' ||
        github.event.review.author_association == 'MEMBER' ||
        github.event.review.author_association == 'COLLABORATOR'
      )
    runs-on: ubuntu-latest

    # Permissions rationale:
    # - contents: write - Required for Claude to push code changes and create branches
    # - pull-requests: write - Required for Claude to create and update PRs
    # - issues: write - Required for Claude to update issue comments and labels
    # - id-token: write - Required for OIDC authentication
    # - actions: read - Required for Claude to read CI results on PRs
    #
    # Security model:
    # - Only OWNER/MEMBER/COLLABORATOR can trigger (see `if` condition above)
    # - Claude creates PRs for review, does not merge directly
    # - No force push capability
    # - Repository-scoped permissions only
    permissions:
      contents: write
      pull-requests: write
      issues: write
      id-token: write
      actions: read

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4
        with:
          fetch-depth: 1

      - name: Run Claude Code
        id: claude
        uses: anthropics/claude-code-action@v1
        with:
          claude_code_oauth_token: ${{ secrets.CLAUDE_CODE_OAUTH_TOKEN }}
          additional_permissions: |
            actions: read
```

### Files to Modify

- `.github/workflows/claude.yml`

## Test Plan

1. Make the permission changes
2. Create a test issue: `@claude Please create a test branch and PR`
3. Verify Claude can:
   - Create branch
   - Push a small change (e.g., add comment to a file)
   - Create PR
4. Have a non-collaborator try to trigger Claude
5. Verify they are blocked by author_association check

## Estimated Effort

1 hour (simple change, but important to test carefully)
