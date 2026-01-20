---
id: 69
slug: iteration-detail-page
title: "Iteration Detail Page with Timeline and Logs"
status: in_progress
created_date: 2026-01-19
updated_date: 2026-01-19
branch: feature/iteration-detail-page
spec_refs: []
depends_on: []
---

# Task 69: Iteration Detail Page with Timeline and Logs

## Objective

Create a dedicated iterations page that shows:
1. Timeline chart (same as overview page)
2. Detailed iteration cards/logs from `/iterate` command
3. Link from overview timeline to detail page

This integrates the existing timeline chart (Task 67) with detailed iteration logging (Task 58).

## Context

### Current State

- Timeline chart exists on overview page (`tools/evals/public/js/timeline.js`)
- Chart shows historical metrics from baseline tags
- `/iterate` command specified to log to `docs/design/iterations/*.md`
- No dedicated page for iteration details

### Target State

- Standalone `/iterations/` page with:
  - Timeline chart at top
  - Iteration cards below showing all logged iterations
  - Filtering, sorting, search capabilities
- Overview page links to detail page
- CI parses iteration logs and generates `iterations.json`

## Implementation Plan

### 1. Iteration Log Parser

**Create**: `scripts/iterate/parse-iterations.js`

Parse markdown files in `docs/design/iterations/`:
- Extract YAML frontmatter (iteration_id, goal, status, dates, branch, commit, pr)
- Extract sections (Goal, Baseline Metrics, Lever Analysis, Result Metrics, Evaluation, Decision)
- Generate `tools/evals/public/data/iterations.json` with structured data

**Format**:
```json
{
  "iterations": [
    {
      "id": "2026-01-18-001",
      "goal": "improve syncopation in wild zone",
      "status": "success",
      "startedAt": "2026-01-18T10:30:00Z",
      "completedAt": "2026-01-18T11:15:00Z",
      "branch": "feature/iterate-2026-01-18-001",
      "commit": "abc123def",
      "pr": "#123",
      "lever": {
        "primary": "kSyncopationCenter",
        "oldValue": 0.50,
        "newValue": 0.55,
        "delta": "+10%"
      },
      "metrics": {
        "target": { "before": 0.317, "after": 0.352, "delta": "+11.0%" },
        "maxRegression": "-1.2%"
      }
    }
  ],
  "summary": {
    "total": 24,
    "successful": 20,
    "failed": 4,
    "successRate": 0.83
  }
}
```

### 2. Iterations Detail Page

**Create**: `tools/evals/public/iterations/index.html`

Page structure:
```html
<!DOCTYPE html>
<html>
<head>
  <title>DuoPulse - Iteration History</title>
  <link rel="stylesheet" href="../css/app.css">
  <link rel="stylesheet" href="../css/iterations.css">
</head>
<body>
  <nav><!-- Same nav as main dashboard --></nav>

  <main>
    <h1>Iteration History</h1>

    <!-- Summary stats -->
    <div class="iteration-stats">
      <span>Total: 24</span>
      <span>Success Rate: 83%</span>
      <span>Net Improvement: +28%</span>
    </div>

    <!-- Timeline chart (reuse existing component) -->
    <section class="timeline-section">
      <h2>Metrics Progress</h2>
      <div id="timeline-chart"></div>
    </section>

    <!-- Filters and search -->
    <section class="iteration-filters">
      <input type="search" placeholder="Search iterations..." />
      <select name="status">
        <option value="all">All Status</option>
        <option value="success">Success</option>
        <option value="failed">Failed</option>
      </select>
      <select name="sort">
        <option value="newest">Newest First</option>
        <option value="oldest">Oldest First</option>
        <option value="impact">Highest Impact</option>
      </select>
    </section>

    <!-- Iteration cards -->
    <section class="iteration-cards">
      <!-- Generated from iterations.json -->
    </section>
  </main>

  <script type="module" src="../js/iterations.js"></script>
</body>
</html>
```

### 3. Iteration Card Component

**Create**: `tools/evals/public/css/iterations.css`

Card design:
```
┌─────────────────────────────────────────────────────┐
│ ✓ #001 - 2026-01-18 10:30                   SUCCESS │
│                                                      │
│ Improve syncopation in wild zone                    │
│                                                      │
│ Lever: kSyncopationCenter (0.50 → 0.55, +10%)      │
│                                                      │
│ Target Metric: +11.0%  |  Max Regression: -1.2%    │
│                                                      │
│ [View PR #123]  [abc123d]  [Branch ⎘]              │
│                                                      │
│ [▼ Show Details]                                    │
└─────────────────────────────────────────────────────┘
```

Expandable details show:
- Full baseline metrics table
- Lever analysis/rationale
- Result metrics table
- Evaluation criteria
- Decision notes

### 4. Timeline Integration

**Modify**: `tools/evals/public/js/timeline.js`

Add click handler to timeline chart points:
```javascript
// When clicking a point on timeline
onClick(dataPoint) {
  // Navigate to iterations page, scroll to matching iteration
  window.location.href = `/iterations/#${dataPoint.iterationId}`;
}
```

**Modify**: `tools/evals/public/app.js` (overview view)

Add "View All Iterations →" link below timeline chart

### 5. CI Integration

**Modify**: `.github/workflows/pages.yml`

Add steps:
```yaml
- name: Parse iteration logs
  run: node scripts/iterate/parse-iterations.js

- name: Deploy iterations page
  run: |
    cp -r tools/evals/public/iterations _site/iterations
    cp tools/evals/public/data/iterations.json _site/data/
```

## Subtasks

- [x] Create feature branch
- [x] Create task file and plan
- [ ] Create `scripts/iterate/parse-iterations.js`
- [ ] Create `tools/evals/public/iterations/index.html`
- [ ] Create `tools/evals/public/css/iterations.css`
- [ ] Create `tools/evals/public/js/iterations.js`
- [ ] Add link from overview page to iterations page
- [ ] Add CI workflow steps
- [ ] Test with sample iteration logs
- [ ] All tests pass

## Acceptance Criteria

- [ ] `/iterations/` page accessible from dashboard nav
- [ ] Timeline chart displays on iterations page
- [ ] Iteration cards render for all logged iterations
- [ ] Clicking timeline points navigates to matching iteration
- [ ] "View All Iterations" link on overview page works
- [ ] Filters and search work correctly
- [ ] Expandable cards show full details
- [ ] CI generates iterations.json automatically
- [ ] All existing tests pass

## Test Plan

1. Create sample iteration logs in `docs/design/iterations/`
2. Run parser: `node scripts/iterate/parse-iterations.js`
3. Verify `iterations.json` structure
4. Open `/iterations/` page locally
5. Verify timeline chart renders
6. Verify iteration cards display
7. Test filtering and search
8. Test expandable details
9. Test navigation from overview
10. Verify CI workflow updates data

## Estimated Effort

4-5 hours

## Notes

This task combines:
- Task 58's original vision (standalone iterations page with detailed cards)
- Task 67's implementation (timeline chart on overview)

Result: Best of both - summary chart on overview, detailed page for exploration.
