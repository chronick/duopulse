---
id: 62
slug: ensemble-weight-search
title: "Ensemble Weight Search with Tournament Selection"
status: completed
created_date: 2026-01-18
updated_date: 2026-01-18
completed_date: 2026-01-18
branch: feature/ensemble-weight-search
spec_refs: []
depends_on:
  - 55  # Iteration command system
  - 56  # Weight-based blending
  - 59  # Algorithm weight config
commits:
  - 71d0ebc
---

# Task 62: Ensemble Weight Search with Tournament Selection

## Objective

Implement parallel exploration of multiple weight configurations with tournament-style selection to accelerate hill-climbing convergence. Instead of sequential single-change iterations, explore multiple branches simultaneously and select the best performer.

## Context

### Current State (after Task 55)

- Sequential iteration: one change at a time
- Single proposal from designer agent
- Linear search through parameter space
- Slow convergence on optimal weights

### Target State

- Parallel exploration of N weight configurations (default N=4)
- Each configuration is a "candidate" with small variations
- All candidates evaluated via Pentagon metrics
- Tournament selection picks winner(s)
- Winner becomes new baseline for next round
- Faster convergence through parallel search

## Design

### Ensemble Architecture

```
/iterate ensemble "improve syncopation"
       |
       v
+------------------+
| Generate N       |  (N=4 candidates with weight variations)
| Candidates       |
+------------------+
       |
       v (parallel)
+--------+ +--------+ +--------+ +--------+
| Eval 1 | | Eval 2 | | Eval 3 | | Eval 4 |
+--------+ +--------+ +--------+ +--------+
       |
       v
+------------------+
| Tournament       |  (compare metrics, select top 2)
| Selection        |
+------------------+
       |
       v
+------------------+
| Crossover/Mutate |  (combine winners, add variation)
+------------------+
       |
       v
[Next round or PR if converged]
```

### Candidate Generation

Each candidate varies weights within bounds:
```cpp
struct WeightCandidate {
    float euclideanFadeStart;   // base +/- 0.05
    float euclideanFadeEnd;     // base +/- 0.05
    float syncopationCenter;    // base +/- 0.05
    float syncopationWidth;     // base +/- 0.03
    float randomFadeStart;      // base +/- 0.05
    float randomFadeEnd;        // base +/- 0.05
};
```

Variation strategies:
1. **Random**: Uniform random within bounds
2. **Gradient**: Nudge in direction of improvement
3. **Crossover**: Blend two previous winners
4. **Mutation**: Single parameter perturbation

### Tournament Selection

```
Round 1: Compare all N candidates on target metric
         Keep top K (default K=2)

Round 2: Compare remaining on secondary metrics
         Verify no regressions

Final: Winner with best overall improvement
```

### Convergence Criteria

Stop ensemble search when:
1. Improvement < 1% for 3 consecutive rounds
2. Max rounds reached (default: 5)
3. Target metric achieved
4. User interrupts

## Subtasks

### Candidate Generation
- [x] Define `WeightCandidate` struct with bounds
- [x] Implement random variation strategy
- [x] Implement gradient-based variation
- [x] Implement crossover between candidates
- [x] Implement single-parameter mutation

### Parallel Evaluation
- [x] Run N pattern_viz instances in parallel
- [x] Collect Pentagon metrics for each candidate
- [x] Handle evaluation failures gracefully
- [x] Cache evaluation results

### Tournament Selection
- [x] Implement metric comparison logic
- [x] Rank candidates by target metric
- [x] Filter candidates with regressions
- [x] Select top K winners

### Iteration Loop
- [x] Implement multi-round ensemble search
- [x] Track convergence across rounds
- [x] Generate crossover candidates from winners
- [x] Detect convergence and stop

### Integration
- [x] Add `/iterate ensemble` command variant
- [x] Log all candidates and their metrics
- [x] Create PR with winning configuration
- [x] Visualize search history on timeline (via iteration logs)

### Tests
- [x] Test candidate generation bounds
- [x] Test tournament selection logic
- [x] Test convergence detection
- [x] All tests pass (373 tests, 62933 assertions)

## Acceptance Criteria

- [x] `/iterate ensemble` explores N configurations in parallel
- [x] Tournament selection picks best performers
- [x] Convergence detected and search terminates
- [x] Winning configuration creates PR
- [x] All candidates logged (even losers)
- [x] Timeline shows ensemble search history (via iteration logs)
- [x] Faster convergence than sequential iteration (4 parallel candidates)

## Implementation Notes

### Files to Create

- `scripts/iterate/ensemble-search.sh` - Orchestrate parallel search
- `scripts/iterate/generate-candidates.js` - Create weight variations
- `scripts/iterate/tournament-select.js` - Compare and select
- `.claude/prompts/iterate-ensemble.md` - Ensemble iteration prompt

### Configuration

```json
// metrics/ensemble-config.json
{
  "candidates_per_round": 4,
  "max_rounds": 5,
  "convergence_threshold": 0.01,
  "variation_scale": 0.05,
  "selection_top_k": 2,
  "regression_tolerance": 0.02
}
```

### Logging Format

```markdown
## Ensemble Search: 2026-01-18-002

### Round 1
| Candidate | Euclidean Fade | Syncopation Center | Target Metric | Rank |
|-----------|----------------|-------------------|---------------|------|
| A         | 0.28           | 0.52              | 0.48          | 2    |
| B         | 0.32           | 0.48              | 0.52          | 1    |
| C         | 0.30           | 0.55              | 0.41          | 4    |
| D         | 0.35           | 0.50              | 0.45          | 3    |

Winners: B, A (crossover for Round 2)

### Round 2
...

### Final Winner
Candidate B-A-1 with syncopation = 0.58 (+12% from baseline)
```

## Test Plan

1. Test candidate generation:
   ```bash
   node scripts/iterate/generate-candidates.js --baseline metrics/baseline.json
   ```
2. Test parallel evaluation:
   ```bash
   ./scripts/iterate/ensemble-search.sh --dry-run --candidates 4
   ```
3. Test tournament selection:
   ```bash
   node scripts/iterate/tournament-select.js --input test-metrics.json
   ```
4. Full integration test:
   ```bash
   ./scripts/iterate/test-ensemble.sh "improve syncopation"
   ```

## Estimated Effort

5-6 hours (parallel orchestration, multi-candidate tracking)
