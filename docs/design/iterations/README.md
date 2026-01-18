# Iteration History

This directory tracks all hill-climbing iterations performed via the `/iterate` command.

## How It Works

1. User invokes `/iterate "goal"` or `/iterate auto`
2. Claude analyzes current baseline metrics
3. Claude proposes weight changes based on sensitivity analysis
4. Changes are applied, evaluated, and logged here
5. Successful iterations become PRs; failed ones inform future attempts

## File Format

Each iteration is logged as `{YYYY-MM-DD}-{NNN}.md`:

```yaml
---
iteration_id: 2026-01-18-001
goal: "improve syncopation in wild zone"
status: success | failed
started_at: ISO-8601 timestamp
completed_at: ISO-8601 timestamp
branch: feature/iterate-2026-01-18-001
commit: abc123def (if successful)
pr: "#123" (if PR created)
---
```

## Success Criteria

An iteration is considered **successful** if:
- Target metric improved >= 5% (relative)
- No other metric regressed > 2% (relative)
- All tests pass

## Cumulative Progress

Track overall improvement by comparing latest baseline to initial baseline:

```
Initial: metrics/baseline.json (commit abc123)
Current: metrics/baseline.json (commit def456)
Total Improvement: +15% on composite score
```

## Index

| ID | Goal | Status | PR |
|----|------|--------|-----|
| (iterations will be listed here) |

---

*This directory is maintained automatically by the `/iterate` command.*
