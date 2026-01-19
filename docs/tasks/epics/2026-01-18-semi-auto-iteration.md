---
epic_id: 2026-01-18-semi-auto-iteration
title: "Semi-Autonomous Hill-Climbing Iteration System"
status: completed
created_date: 2026-01-18
updated_date: 2026-01-19
completed_date: 2026-01-19
branch: feature/hill-climbing-iteration
tasks: [53, 54, 55, 56, 57, 58, 59, 61, 61a, 61b, 62, 63, 64, 65, 66, 67]
completed_tasks: [53, 54, 55, 56, 57, 58, 59, 60, 61, 61a, 61b, 62, 63, 64, 66, 67]
deferred_tasks: [65]
---

# Epic: Semi-Autonomous Hill-Climbing Iteration System

## Vision

Create a feedback loop using git + GitHub + PRs/issues + Actions to automatically iterate on the DuoPulse pattern generation algorithm. Claude responds to @mentions, proposes weight changes, evaluates via Pentagon metrics, and creates PRs for approval.

## Goals

1. **Make algorithm easily adjustable** - Weight-based blending with JSON config
2. **Understand parameter impacts** - Sensitivity analysis reveals high-impact levers
3. **Automate iteration cycle** - `/iterate` command orchestrates single-pass improvements
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

> **Note**: This order was revised 2026-01-18 based on design review feedback.
> Key changes: Task 61 split into 61a/61b prerequisites, Task 63 moved before 55,
> Task 55 simplified to single-pass, Task 56 reduced scope, Task 64 added for permissions.
> **Task 66 added** to fix config→pattern generation disconnect discovered in Task 63.

### Phase 1: Foundation + Baseline (Tasks 56, 59, 61a, 61b)

**Goal**: Make the algorithm adjustable AND establish baseline metrics infrastructure.

```
56 Weight-Based Blending ──► 59 Weight Config ──► 61a Baseline Infra
        │                           │                      │
        │                           │                      │
   Explicit weights            JSON config            Create baseline.json
   + bootstrap levers          without code           Run evals, save
                               changes
                                                           │
                                                           ▼
                                                    61b PR Metrics Compare
                                                           │
                                                      CI compares PR
                                                      to baseline, fails
                                                      if regression > 2%
```

| Order | Task | Title | Est. | Notes |
|-------|------|-------|------|-------|
| 1 | **56** | Weight-Based Algorithm Blending | 4-5h | Reduced scope: no per-section variation |
| 2 | **59** | Algorithm Weight Configuration | 3-4h | No changes |
| 3 | **61a** | Baseline Infrastructure | 1-2h | NEW: Create metrics/, baseline.json |
| 4 | **61b** | PR Metrics Comparison | 2-3h | NEW: CI workflow for PR comparison |

**Phase 1 Exit Criteria**:
- [x] `pattern_viz --debug-weights` shows algorithm blend percentages
- [x] Weights configurable via `inc/algorithm_config.h` or JSON
- [x] `metrics/baseline.json` exists with current main branch metrics
- [x] PRs show metric comparison in comments
- [x] All tests pass (373 tests)

---

### Phase 2: Iteration Core (Tasks 66, 63, 55, 64)

**Goal**: Enable sensitivity-informed iteration with proper permissions.

> **Updated**: Task 66 added as prerequisite to Task 55 to fix config→generation wiring.

```
63 Sensitivity Analysis ──► 66 PatternField Wiring ──► 55 Iterate Command ──► 64 Claude Permissions
        │                           │                           │                      │
        │                           │                           │                      │
   Infrastructure          Wire zone thresholds          /iterate "goal"        contents: write
   complete (zeros)        to actual generation          Single-pass only       pull-requests: write
                           Non-zero sensitivity          PR if improved         issues: write
```

| Order | Task | Title | Est. | Notes |
|-------|------|-------|------|-------|
| 5 | **63** | Parameter Sensitivity Analysis | 4-5h | **COMPLETED** |
| 5.5 | **66** | Wire Zone Thresholds into PatternField | 3-4h | **COMPLETED** |
| 6 | **55** | Iteration Command System | 4-5h | **COMPLETED** |
| 7 | **64** | Claude Permissions Update | 1h | NEW: Update claude.yml |

**Phase 2 Exit Criteria**:
- [x] Sensitivity analysis infrastructure complete (Task 63)
- [x] Zone thresholds wired to pattern generation (Task 66)
- [x] Sensitivity matrix shows weight→metric impacts (requires 66)
- [x] `/iterate "improve X"` triggers single-pass workflow (Task 55)
- [x] `/iterate auto` suggests goal from weakest metric (Task 55)
- [x] PRs created with before/after metrics (Task 55)
- [x] Claude can push branches and create PRs (Task 64)

---

### Phase 3: Quality Gates + Expansion (Tasks 54, 61, 62)

**Goal**: Extend evaluation surface, add rollback, and enable parallel search.

```
54 Fill Gates in Evals             61 Full Regression Detection
        │                                    │
        │                                    │
   Fill channel added              /rollback command
   to Pentagon metrics             Last-known-good tagging
   Display on website              Consecutive regression alerts
                                             │
                                             ▼
                                      62 Ensemble Search
                                             │
                                       Parallel candidates
                                       Tournament selection
```

| Order | Task | Title | Est. | Notes |
|-------|------|-------|------|-------|
| 8 | **54** | Fill Gates in Evals | 3-4h | No changes |
| 9 | **61** | Full Regression Detection | 2-3h | Rollback capability (61a/61b are prereqs) |
| 10 | **62** | Ensemble Weight Search | 5-6h | No changes |

**Phase 3 Exit Criteria**:
- [x] Fill gate patterns visible on evals dashboard
- [x] Fill metrics included in Pentagon scoring
- [x] `/rollback` reverts to last known good baseline
- [x] Consecutive regressions trigger alerts
- [x] `/iterate ensemble` explores 4 candidates in parallel

---

### Phase 4: Visibility (Tasks 57, 58)

**Goal**: Enable feedback loop and visualize progress.

```
57 PR Workflow Integration         58 Website Timeline
        │                                    │
        │                                    │
   @claude mentions               Timeline page with
   Feedback via comments          git refs + version tags
   /status, /compare              Score evolution graph
```

| Order | Task | Title | Est. | Notes |
|-------|------|-------|------|-------|
| 11 | **57** | PR Workflow Integration | 4-5h | No changes |
| 12 | **58** | Website Iteration Timeline | 4-5h | No changes |

**Phase 4 Exit Criteria**:
- [x] `@claude /iterate` in issue triggers workflow
- [x] PR comments update iteration with feedback
- [x] `/status`, `/compare` commands work
- [x] Timeline page shows all iterations with scores
- [x] Git commit refs and version tags displayed

---

### Phase 5: Resolution + Future (Tasks 53, 65)

**Goal**: Increase pattern resolution and enable phrase-aware weights (future).

| Order | Task | Title | Est. | Notes |
|-------|------|-------|------|-------|
| 13 | **53** | Grid Expansion to 64 Steps | 4-6h | No changes |
| 14 | **65** | Phrase-Aware Weight Modulation | TBD | FUTURE: Split from Task 56 |

**Phase 5 Exit Criteria**:
- [x] `kMaxSteps = 64` with `uint64_t` masks
- [x] Pattern lengths 16/32/64 supported
- [x] 1/4 step subdivisions for flam placement
- [x] All tests updated and passing

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
    │  59   │     │ 61a   │
    │Config │     │Baseln │
    └───┬───┘     └───┬───┘
        │             │
        │         ┌───▼───┐
        │         │ 61b   │
        │         │PR Cmp │
        │         └───┬───┘
        │             │
        └──────┬──────┘
               │
            ┌──▼───┐
            │  63  │ Sensitivity Analysis (infra only)
            └──┬───┘
               │
            ┌──▼───┐
            │  66  │ PatternField Config Wiring (NEW)
            └──┬───┘
               │
            ┌──▼───┐
            │  55  │ Iterate Command (simplified)
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
┌───────┐  ┌───────┐  ┌───────┐
│  54   │  │  53   │  │  64   │
│Fill   │  │64-step│  │Perms  │
└───────┘  └───────┘  └───────┘
```

---

## Task Reference

| ID | Slug | Title | Status | Phase |
|----|------|-------|--------|-------|
| 53 | grid-expansion-64 | Grid Expansion to 64 Steps | **completed** | 5 |
| 54 | fill-gates-evals | Fill Gates in Evals | **completed** | 3 |
| 55 | iterate-command | Iteration Command System | **completed** | 2 |
| 56 | weight-based-blending | Weight-Based Algorithm Blending | **completed** | 1 |
| 57 | pr-workflow-integration | PR Workflow Integration | pending | 4 |
| 58 | website-iteration-timeline | Website Iteration Timeline | pending | 4 |
| 59 | algorithm-weight-config | Algorithm Weight Configuration | **completed** | 1 |
| 60 | audio-preview-player | Audio Preview Player | **completed** | - |
| 61 | regression-detection | Regression Detection + Rollback | **completed** | 3 |
| 61a | baseline-infrastructure | Baseline Infrastructure | **completed** | 1 |
| 61b | pr-metrics-comparison | PR Metrics Comparison | **completed** | 1 |
| 62 | ensemble-weight-search | Ensemble Weight Search | **completed** | 3 |
| 63 | parameter-sensitivity | Parameter Sensitivity Analysis | **completed** | 2 |
| 64 | claude-permissions | Claude Permissions Update | **completed** | 2 |
| 65 | phrase-aware-weights | Phrase-Aware Weight Modulation | backlog | 5 |
| 66 | config-patternfield-wiring | Wire Zone Thresholds into PatternField | **completed** | 2 |
| 67 | dashboard-improvements | Evaluation Dashboard UX Improvements | **completed** | - |

---

## Design Review Changes (2026-01-18)

This epic was revised based on design review feedback. Key issues addressed:

| Issue | Resolution |
|-------|------------|
| Claude can't push/create PRs | New Task 64 updates permissions |
| Designer/Critic can't communicate | Simplified to single-pass iteration |
| Sensitivity runs after iterate | Reordered: 63 before 55 |
| No baseline exists | New Task 61a creates infrastructure |
| No CI comparison | New Task 61b adds workflow |
| Task 56 over-scoped | Split per-section to future Task 65 |
| Task 55 listening tests | Removed from scope (future work) |
| **Sensitivity produces zeros** | **New Task 66 wires config to generation** |

---

## Code Review Fixes (2026-01-18)

Post-implementation code review of Tasks 55 and 64 identified and fixed these issues:

| Issue | Resolution | Commit |
|-------|------------|--------|
| Workflow comment referenced wrong path | Changed `active/` → `completed/` in claude.yml | `0e4d38c` |
| Inconsistent iteration ID generation | Use `generate-iteration-id.js` script in command | `0e4d38c` |
| Direction-unaware regression detection | Added target range awareness to compare-metrics.js | `0e4d38c` |

### Deferred Enhancements

The following were noted but deferred for future work:

- **Script unit tests** - compare-metrics.js and generate-iteration-id.js lack automated tests
- **`--json` output mode** - For machine-parseable compare-metrics output
- **`--dry-run` flag** - Preview changes before applying in /iterate
- **Auto-generated iteration index** - Currently manual maintenance
- **Concurrent ID collision handling** - No locking in generate-iteration-id.js

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

---

## Design Limitation Discovered (Task 63) - RESOLVED BY TASK 66

During Task 63 implementation, a disconnect was discovered between the configuration system and pattern generation:

**Problem**: Sensitivity analysis showed all zeros because:
1. `AlgorithmConfig` weights (Task 56) define blend parameters, but
2. `PatternField.cpp` uses separate hardcoded zone thresholds (`kShapeZone1End`, etc.)
3. These zone constants weren't connected to the CLI args

**Resolution**: Task 66 implemented `PatternFieldConfig` struct with runtime-configurable zone thresholds.

**Key insight from design review**: AlgorithmConfig and PatternField zones are conceptually different:
- AlgorithmConfig: proportional blend weights ("60% euclidean")
- PatternField zones: SHAPE value thresholds ("use syncopation at SHAPE > 0.28")

**Result**: Sensitivity matrix now produces non-zero values:
- `shapeCrossfade1End`: -0.038 sensitivity for syncopation
- `shapeZone1End`: 0.009 sensitivity for voiceSeparation

CLI args renamed to match PatternField terminology (`--shape-zone1-end`, etc.).

---

## Task 53 Implementation (2026-01-19)

**Status**: Completed on branch `feature/grid-expansion-64`

Task 53 (Grid Expansion to 64 Steps) was implemented as part of Phase 5. While not strictly required for the core iteration system (Phases 1-4), it provides foundation for higher-resolution patterns and future enhancements.

### Implementation Summary

**Commits**:
- `05e9d35` - Initial implementation (28 files, 449 insertions, 443 deletions)
- `f67106b` - Task documentation update
- `03497b5` - Critical 64-bit conversion fixes from code review

**Key Changes**:
- `kMaxSteps`: 32 → 64
- `kMaxPhraseSteps`: 256 → 512
- All pattern masks: `uint32_t` → `uint64_t`
- All bit operations: `1U <<` → `1ULL <<`
- Added `kMaxSubSteps = 256` for future flam resolution

**Code Review Findings**:
Three parallel code-reviewer agents identified 6 critical issues (all fixed):
1. VoiceRelation.cpp:154 - UB from 32-bit shift (fixed: 1U → 1ULL)
2. GumbelSampler.h:133 - Type mismatch header/impl (fixed: uint32_t → uint64_t)
3. HatBurst.cpp/.h - Incomplete migration (fixed: full 64-bit conversion)
4. VelocityCompute.cpp:131 - Step aliasing bug (fixed: removed & 31 mask)
5. Sequencer.cpp:409 - Missing patternLength parameter (fixed)
6. Sequencer.cpp:444 - Missing patternLength parameter (fixed)

**Validation**:
- ✓ All 373 tests pass (62,942 assertions)
- ✓ Clean build (no warnings)
- ✓ Memory: 92.12% flash, 5.81% SRAM (minimal increase)
- ✓ Pattern generation verified at 16/32/64 steps
- ✓ Hill-climbing system compatible

---

## Epic Completion (2026-01-19)

All tasks from Phases 1-4 are complete and functional:
- Phase 1: Foundation + Baseline ✓
- Phase 2: Iteration Core ✓
- Phase 3: Quality Gates + Expansion ✓
- Phase 4: Visibility ✓
- Phase 5: Task 53 complete ✓, Task 65 deferred to backlog

**Deferred**: Task 65 (Phrase-Aware Weight Modulation) remains in backlog as a future enhancement.

The semi-autonomous hill-climbing iteration system is now operational and ready for production use.

---

## Dashboard Improvements (2026-01-19)

**Task 67** added post-completion to improve evaluation dashboard UX based on user feedback:

| Issue | Resolution |
|-------|------------|
| No progress timeline | New script extracts metrics from baseline tags, timeline chart on overview |
| Fill section unclear | Added parameter displays, explanatory text, configuration section |
| Fill state not visible | Added fill state indicator row (high/low line) to fill patterns |
| Sensitivity data missing | Added sensitivity-matrix and metrics-history steps to CI workflow |

The dashboard now provides better visibility into algorithm evolution and clearer understanding of the evaluation data.
