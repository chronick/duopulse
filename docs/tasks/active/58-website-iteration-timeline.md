---
id: 58
slug: website-iteration-timeline
title: "Website Iteration Timeline and Progress Tracking"
status: pending
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/website-iteration-timeline
spec_refs: []
depends_on:
  - 55  # Iteration command system
---

# Task 58: Website Iteration Timeline and Progress Tracking

## Objective

Add an iteration timeline to the DuoPulse website that visualizes hill-climbing progress, showing each iteration step with narration, git commit refs, version tags, and score evolution over time.

## Context

### Current State

- Website has landing page and evals dashboard
- No historical view of algorithm improvements
- No visualization of score progression
- Iteration history only in markdown files

### Target State

- Timeline page showing iteration history
- Each step has narration explaining changes
- Git commit refs and version tags linked
- Score evolution graph over time
- Visual representation of hill-climbing path
- Searchable/filterable iteration history

## Design

### Timeline Visualization

```
Score
  |
  |                                      *----* v5.2.0
  |                               *-----*
  |                         *----*
  |                    *---*
  |              *----*
  |         *---*
  |    *---*
  |---*
  +-------------------------------------------> Time

  Iteration 1   2   3   4   5   6   7   8
```

### Iteration Card

```
+--------------------------------------------------+
| Iteration 2026-01-18-001                   v5.1.4 |
|                                                   |
| Goal: Improve syncopation in wild zone            |
|                                                   |
| Changes:                                          |
| - Increased wild zone noise scale                 |
| - Adjusted syncopation weight boost               |
|                                                   |
| +10% syncopation | -2% density | Overall: +4%    |
|                                                   |
| Commit: abc123d                    [View PR #123] |
+--------------------------------------------------+
```

## Subtasks

### Data Generation
- [ ] Create script to parse `docs/design/iterations/*.md` files
- [ ] Extract metrics from iteration logs
- [ ] Generate `iterations.json` with timeline data
- [ ] Include git metadata (commits, tags, dates)

### Timeline Page
- [ ] Create `site/iterations.html` page
- [ ] Add timeline component with iteration cards
- [ ] Implement score evolution graph (Chart.js or CSS)
- [ ] Add filtering by date range, goal type
- [ ] Add search across iteration narration

### Navigation
- [ ] Add Iterations link to main navigation
- [ ] Link from evals dashboard to relevant iterations
- [ ] Add iteration badge to show improvement delta

### Iteration Cards
- [ ] Show goal and summary
- [ ] Display metric deltas with color coding (+green, -red)
- [ ] Link to git commit and PR
- [ ] Show version tag if present
- [ ] Expandable details section

### Score Evolution Graph
- [ ] Plot composite score over time
- [ ] Show individual metric trends
- [ ] Annotate version releases
- [ ] Highlight significant improvements

### Version Tracking
- [ ] Parse version tags from git history
- [ ] Associate iterations with versions
- [ ] Show version milestones on timeline
- [ ] Display changelog for each version

### CI Integration
- [ ] Auto-update `iterations.json` on iteration PRs
- [ ] Deploy updated timeline on merge
- [ ] Include iteration data in site build

### Tests
- [ ] Test iteration log parsing
- [ ] Test timeline rendering
- [ ] Test score graph accuracy
- [ ] All tests pass

## Acceptance Criteria

- [ ] Timeline page accessible at `/iterations/`
- [ ] All completed iterations displayed chronologically
- [ ] Each iteration has narration and metrics
- [ ] Git commit refs link to GitHub
- [ ] Version tags displayed for releases
- [ ] Score evolution graph updates automatically
- [ ] Filtering and search work correctly
- [ ] Responsive design for mobile
- [ ] All existing tests pass

## Implementation Notes

### Files to Create

Site:
- `site/iterations.html` - Timeline page
- `site/iterations.css` - Timeline styles
- `site/iterations.js` - Timeline interactivity

Data:
- `tools/evals/parse-iterations.js` - Log parser
- `tools/evals/public/data/iterations.json` - Timeline data

### Files to Modify

- `site/index.html` - Add Iterations nav link
- `site/styles.css` - Shared styles
- `.github/workflows/pages.yml` - Include iterations in build

### Iterations Data Format

```json
{
  "iterations": [
    {
      "id": "2026-01-18-001",
      "goal": "Improve syncopation in wild zone",
      "status": "success",
      "startedAt": "2026-01-18T10:30:00Z",
      "completedAt": "2026-01-18T11:15:00Z",
      "commit": "abc123def",
      "pr": 123,
      "version": "5.1.4",
      "changes": [
        "Increased wild zone noise scale from 0.40 to 0.50",
        "Adjusted syncopation weight boost"
      ],
      "metrics": {
        "before": {
          "syncopation": { "stable": 0.15, "syncopated": 0.42, "wild": 0.48 },
          "composite": 0.72
        },
        "after": {
          "syncopation": { "stable": 0.15, "syncopated": 0.43, "wild": 0.58 },
          "composite": 0.76
        },
        "delta": {
          "syncopation": { "stable": 0.00, "syncopated": 0.01, "wild": 0.10 },
          "composite": 0.04
        }
      }
    }
  ],
  "summary": {
    "totalIterations": 24,
    "successRate": 0.83,
    "netImprovement": 0.28,
    "latestVersion": "5.2.0"
  }
}
```

### Timeline Component

```html
<div class="timeline">
  <div class="timeline-track">
    <div class="score-graph">
      <canvas id="score-evolution"></canvas>
    </div>
    <div class="iteration-markers">
      <!-- One marker per iteration, positioned by date -->
    </div>
  </div>
  <div class="iteration-cards">
    <!-- Cards for each iteration -->
  </div>
</div>
```

### Score Evolution Graph

Using Chart.js for simplicity:

```javascript
new Chart(ctx, {
  type: 'line',
  data: {
    labels: iterations.map(i => i.id),
    datasets: [{
      label: 'Composite Score',
      data: iterations.map(i => i.metrics.after.composite),
      borderColor: '#4a90d9',
      tension: 0.3
    }]
  },
  options: {
    annotations: {
      // Version release markers
    }
  }
});
```

## Test Plan

1. Create test iteration logs in `docs/design/iterations/`
2. Run parser: `node tools/evals/parse-iterations.js`
3. Verify `iterations.json` is correct
4. Build site: `make evals`
5. View timeline at `http://localhost:3000/iterations/`
6. Verify cards display correctly
7. Verify graph renders properly
8. Test filtering and search

## Estimated Effort

4-5 hours (frontend development + data parsing)
