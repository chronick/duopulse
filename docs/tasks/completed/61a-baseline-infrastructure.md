---
id: 61a
slug: baseline-infrastructure
title: "Baseline Infrastructure"
status: completed
created_date: 2026-01-18
updated_date: 2026-01-18
completed_date: 2026-01-18
branch: feature/weight-based-blending
spec_refs: []
depends_on:
  - 56  # Weight-based blending (evals need this for weight debugging)
---

# Task 61a: Baseline Infrastructure

## Objective

Create the foundational infrastructure for tracking Pentagon metrics on main branch. This enables PR comparison and regression detection in subsequent tasks.

## Context

### Background

This task was split from Task 61 during design review (2026-01-18). The original Task 61 tried to do too much at once. This task focuses only on creating the baseline infrastructure.

### Current State

- Evals generate `tools/evals/public/expressiveness.json`
- No persistent baseline tracking
- No `metrics/` directory

### Target State

- `metrics/baseline.json` stores main branch Pentagon metrics
- `make baseline` target generates/updates baseline
- Baseline includes commit hash for traceability
- Simple, reliable foundation for Tasks 61b and 61

## Subtasks

### Directory Structure
- [x] Create `metrics/` directory
- [x] Add `metrics/.gitkeep` (directory tracked even if empty initially)
- [x] Add `metrics/README.md` explaining purpose

### Baseline Generation
- [x] Add `make baseline` target to Makefile
- [x] Target runs evals and copies result to `metrics/baseline.json`
- [x] Add current commit hash to baseline metadata
- [x] Add generation timestamp

### Baseline Format
- [x] Define JSON schema for baseline.json
- [x] Include Pentagon metrics (overall and by-zone)
- [x] Include generation metadata (commit, timestamp, version)

### Initial Baseline
- [x] Run `make baseline` to create initial baseline
- [x] Commit baseline.json to repository
- [x] Verify baseline matches current evals output

### Tests
- [x] Test `make baseline` generates valid JSON
- [x] Test baseline includes required fields
- [x] All tests pass (373 tests)

## Acceptance Criteria

- [x] `metrics/` directory exists
- [x] `metrics/baseline.json` contains Pentagon metrics
- [x] `make baseline` target works
- [x] Baseline includes commit hash and timestamp
- [x] Baseline matches current evals output
- [x] Baseline is committed to repository

## Implementation Notes

### Baseline Format

```json
{
  "version": "1.0.0",
  "generated_at": "2026-01-18T12:00:00Z",
  "commit": "f6e2631...",
  "metrics": {
    "overall": {
      "syncopation": 0.42,
      "density": 0.58,
      "voiceSeparation": 0.71,
      "velocityRange": 0.45,
      "regularity": 0.63
    },
    "byZone": {
      "stable": {
        "syncopation": 0.15,
        "density": 0.45,
        ...
      },
      "syncopated": {
        "syncopation": 0.42,
        ...
      },
      "wild": {
        "syncopation": 0.68,
        ...
      }
    }
  }
}
```

### Makefile Changes

```makefile
# Generate baseline metrics from current main branch
.PHONY: baseline
baseline: build/pattern_viz
	@echo "Generating baseline metrics..."
	cd tools/evals && npm run generate-patterns
	cd tools/evals && npm run evaluate
	@mkdir -p metrics
	@echo '{"version":"1.0.0","generated_at":"'$$(date -u +%Y-%m-%dT%H:%M:%SZ)'","commit":"'$$(git rev-parse HEAD)'","metrics":' > metrics/baseline.json
	@cat tools/evals/public/expressiveness.json >> metrics/baseline.json
	@echo '}' >> metrics/baseline.json
	@echo "Baseline saved to metrics/baseline.json"
```

### Files to Create

- `metrics/.gitkeep`
- `metrics/README.md`
- `metrics/baseline.json` (generated)

### Files to Modify

- `Makefile` - Add baseline target

## Test Plan

1. Run baseline generation:
   ```bash
   make baseline
   ```
2. Verify output:
   ```bash
   cat metrics/baseline.json | jq .
   ```
3. Verify commit hash matches:
   ```bash
   git rev-parse HEAD
   ```
4. Verify metrics match evals:
   ```bash
   diff -y <(jq .metrics metrics/baseline.json) <(jq . tools/evals/public/expressiveness.json)
   ```

## Estimated Effort

1-2 hours (small, focused task)
