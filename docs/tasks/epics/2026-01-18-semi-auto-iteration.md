---
epic_id: 2026-01-18-semi-auto-iteration
title: "Semi-Autonomous Hill-Climbing Iteration System"
status: in_progress
created_date: 2026-01-18
branch: feature/hill-climbing-infrastructure
tasks: [53, 54, 55, 56, 57, 58, 59, 61, 62, 63]
completed_tasks: [60]
---

# Epic: Semi-Autonomous Hill-Climbing Iteration System

## Vision

Create a feedback loop using git + GitHub + PRs/issues + Actions to automatically iterate on the DuoPulse pattern generation algorithm. Claude responds to @mentions, proposes weight changes, evaluates via Pentagon metrics, and creates PRs for approval.

## Goals

1. **Make algorithm easily adjustable** - Weight-based blending with JSON config
2. **Understand parameter impacts** - Sensitivity analysis reveals high-impact levers
3. **Automate iteration cycle** - `/iterate` command orchestrates designer/critic agents
4. **Prevent regressions** - CI fails when Pentagon metrics regress
5. **Enable parallel exploration** - Ensemble search with tournament selection
6. **Track progress visually** - Timeline on website shows hill-climbing history
7. **Support user feedback** - PR approval workflow with comments

## Success Metrics

- Overall Pentagon score improves by 20%+ over 10 iterations
- Iteration cycle time < 30 minutes per proposal
- Zero undetected regressions merged to main
- Clear visibility into iteration history on website

---

## Implementation Order

### Phase 1: Foundation (Tasks 56, 59, 63)

**Goal**: Make the algorithm adjustable and understand what to adjust.

```
56 Weight-Based Blending ──► 59 Weight Config ──► 63 Sensitivity Analysis
        │                           │                      │
        │                           │                      │
   Explicit weights            JSON config            Know which
   for euclidean/             without code           weights affect
   syncopation/random         changes                which metrics
```

| Order | Task | Title | Est. |
|-------|------|-------|------|
| 1 | **56** | Weight-Based Algorithm Blending | 5-6h |
| 2 | **59** | Algorithm Weight Configuration | 3-4h |
| 3 | **63** | Parameter Sensitivity Analysis | 4-5h |

**Phase 1 Exit Criteria**:
- [ ] `pattern_viz --debug-weights` shows algorithm blend percentages
- [ ] Weights configurable via `inc/algorithm_config.h` or JSON
- [ ] Sensitivity matrix generated showing weight→metric impacts
- [ ] All tests pass

---

### Phase 2: Iteration Core (Tasks 55, 62)

**Goal**: Enable automated iteration with single and parallel search.

```
55 Iterate Command ────────────────► 62 Ensemble Search
        │                                    │
        │                                    │
   /iterate "goal"                    /iterate ensemble
   Designer proposes                  Parallel N candidates
   Critic evaluates                   Tournament selection
   PR if approved                     Faster convergence
```

| Order | Task | Title | Est. |
|-------|------|-------|------|
| 4 | **55** | Iteration Command System | 6-8h |
| 5 | **62** | Ensemble Weight Search | 5-6h |

**Phase 2 Exit Criteria**:
- [ ] `/iterate "improve X"` triggers full workflow
- [ ] `/iterate auto` suggests goal from weakest metric
- [ ] `/iterate ensemble` explores 4 candidates in parallel
- [ ] Iteration logs in `docs/design/iterations/`
- [ ] PRs created with before/after metrics

---

### Phase 3: Quality Gates (Tasks 54, 61)

**Goal**: Extend evaluation surface and prevent regressions.

```
54 Fill Gates in Evals             61 Regression Detection
        │                                    │
        │                                    │
   Fill channel added              CI fails on regression
   to Pentagon metrics             /rollback to last good
   Display on website              Baseline management
```

| Order | Task | Title | Est. |
|-------|------|-------|------|
| 6 | **54** | Fill Gates in Evals | 3-4h |
| 7 | **61** | Regression Detection + Rollback | 3-4h |

**Phase 3 Exit Criteria**:
- [ ] Fill gate patterns visible on evals dashboard
- [ ] Fill metrics included in Pentagon scoring
- [ ] `metrics/baseline.json` tracks main branch metrics
- [ ] PRs with regressions > 2% fail CI
- [ ] `/rollback` reverts to last known good baseline

---

### Phase 4: Visibility (Tasks 57, 58)

**Goal**: Enable feedback loop and visualize progress.

```
57 PR Workflow Integration         58 Website Timeline
        │                                    │
        │                                    │
   @claude mentions               Timeline page with
   Task-only PRs                  git refs + version tags
   Design iteration PRs           Score evolution graph
   Feedback via comments          Narration per iteration
```

| Order | Task | Title | Est. |
|-------|------|-------|------|
| 8 | **57** | PR Workflow Integration | 4-5h |
| 9 | **58** | Website Iteration Timeline | 4-5h |

**Phase 4 Exit Criteria**:
- [ ] `@claude /iterate` in issue triggers workflow
- [ ] PR comments update iteration with feedback
- [ ] `/status`, `/retry`, `/compare` commands work
- [ ] Timeline page shows all iterations with scores
- [ ] Git commit refs and version tags displayed

---

### Phase 5: Resolution Upgrade (Task 53)

**Goal**: Increase pattern resolution for flams and micro-timing.

| Order | Task | Title | Est. |
|-------|------|-------|------|
| 10 | **53** | Grid Expansion to 64 Steps | 4-6h |

**Phase 5 Exit Criteria**:
- [ ] `kMaxSteps = 64` with `uint64_t` masks
- [ ] Pattern lengths 16/32/64 supported
- [ ] 1/4 step subdivisions for flam placement
- [ ] All tests updated and passing

---

## Dependency Graph

```
            ┌──────┐
            │  56  │ Weight-Based Blending
            └──┬───┘
               │
        ┌──────┴──────┐
        │             │
    ┌───▼───┐     ┌───▼───┐
    │  59   │     │  63   │
    │Config │     │Sensitv│
    └───┬───┘     └───┬───┘
        │             │
        └──────┬──────┘
               │
            ┌──▼───┐
            │  55  │ Iterate Command
            └──┬───┘
               │
    ┌──────────┼──────────┐
    │          │          │
┌───▼───┐  ┌───▼───┐  ┌───▼───┐
│  62   │  │  57   │  │  61   │
│Ensembl│  │PR Work│  │Regress│
└───────┘  └───┬───┘  └───────┘
               │
            ┌──▼───┐
            │  58  │ Timeline
            └──────┘

Independent:
┌───────┐  ┌───────┐
│  54   │  │  53   │
│Fill   │  │64-step│
└───────┘  └───────┘
```

---

## Task Reference

| ID | Slug | Title | Status | Phase |
|----|------|-------|--------|-------|
| 53 | grid-expansion-64 | Grid Expansion to 64 Steps | pending | 5 |
| 54 | fill-gates-evals | Fill Gates in Evals | pending | 3 |
| 55 | iterate-command | Iteration Command System | pending | 2 |
| 56 | weight-based-blending | Weight-Based Algorithm Blending | pending | 1 |
| 57 | pr-workflow-integration | PR Workflow Integration | pending | 4 |
| 58 | website-iteration-timeline | Website Iteration Timeline | pending | 4 |
| 59 | algorithm-weight-config | Algorithm Weight Configuration | pending | 1 |
| 60 | audio-preview-player | Audio Preview Player | **completed** | - |
| 61 | regression-detection | Regression Detection + Rollback | pending | 3 |
| 62 | ensemble-weight-search | Ensemble Weight Search | pending | 2 |
| 63 | parameter-sensitivity | Parameter Sensitivity Analysis | pending | 1 |

---

## How to Use This Epic

### Start Implementation

```bash
# Point SDD task manager at this epic
/sdd-task docs/tasks/epics/2026-01-18-semi-auto-iteration.md

# Or implement specific task
/sdd-task 56
```

### Check Progress

```bash
# View epic status
cat docs/tasks/epics/2026-01-18-semi-auto-iteration.md

# Check individual task
cat docs/tasks/active/56-weight-based-blending.md
```

### After Completing a Task

1. Mark task as completed in its file
2. Update this epic's `completed_tasks` list
3. Commit with message referencing epic and task
4. Move to next task in implementation order

---

## Test Cases: V5.5 Tasks (46-50)

Once the iteration system is built, use **tasks 46-50** (now in `backlog/`) to validate it works:

| Task | Title | Test Purpose |
|------|-------|--------------|
| 46 | V5.5 Noise Formula Fix | Test `/iterate` with targeted bug fix |
| 47 | V5.5 Velocity Variation | Test sensitivity analysis for velocity metrics |
| 48 | V5.5 Micro-Displacement | Test ensemble search for timing parameters |
| 49 | V5.5 AUX Style Zones | Test multi-metric optimization (fill + main) |
| 50 | V5.5 Two-Pass Generation | Test complex architectural iteration |

**Validation workflow**:
1. Complete Phase 2 (iterate command working)
2. Run `/iterate auto` → should suggest improvement based on Pentagon gaps
3. Run `/iterate "implement task 46"` → should propose noise formula changes
4. Verify PR created with before/after metrics
5. Approve PR and verify regression detection works
6. Repeat for tasks 47-50

---

## Notes

- **Tasks 46-50** moved to `backlog/` - will be implemented via the iteration system as validation
- **Task 52 (CI Release Builds)** is independent infrastructure work
- This epic focuses on the iteration feedback loop, not the algorithm improvements themselves
- Algorithm improvements will come from _using_ this system once built
