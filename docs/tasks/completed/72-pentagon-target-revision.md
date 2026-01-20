---
id: 72
slug: pentagon-target-revision
title: "Pentagon Eval Metric Target Revision"
status: completed
created_date: 2026-01-20
updated_date: 2026-01-20
completed_date: 2026-01-20
branch: feature/pentagon-target-revision
spec_refs: ["06-shape", "08-complement"]
---

# Task 72: Pentagon Eval Metric Target Revision

## Objective

Revise Pentagon evaluation metric target ranges in `evaluate-expressiveness.py` to align with actual spec design intent. Current targets expect behaviors opposite to what the algorithms are designed to produce.

## Background

Investigations from iterations 2026-01-20-006 and 2026-01-20-007 revealed fundamental misalignments:

1. **Syncopation targets misaligned**: Algorithm produces ~0.95, targets expect 0.22-0.48
   - Spec intent: Syncopated zone (SHAPE 30-70%) is designed to MAXIMIZE metric displacement
   - Current targets expect low syncopation in syncopated zone

2. **VoiceSeparation targets misaligned**: Algorithm produces ~0.92, targets expect 0.32-0.68
   - Spec intent: COMPLEMENT relationship places shimmer in gaps, preventing overlap by design
   - Current targets expect moderate overlap when none should occur

## Subtasks

### Phase 1: Target Analysis
- [x] Document current target ranges for all 5 Pentagon metrics
- [x] Map each target range to spec design intent
- [x] Identify all misalignments (not just syncopation/voiceSeparation)

### Phase 2: Syncopation Target Revision
- [x] Update `score_syncopation()` target ranges in evaluate-expressiveness.py
- [x] Recommended new targets:
  - stable (SHAPE 0-30%): 0.00-0.20 (euclidean patterns, minimal displacement)
  - syncopated (SHAPE 30-70%): 0.70-1.00 (designed for maximum displacement)
  - wild (SHAPE 70-100%): 0.60-1.00 (high displacement with chaos)

### Phase 3: VoiceSeparation Target Revision
- [x] Update `score_voice_separation()` target ranges in evaluate-expressiveness.py
- [x] Recommended new targets:
  - stable (SHAPE 0-30%): 0.75-0.95 (gaps exist, shimmer fills them cleanly)
  - syncopated (SHAPE 30-70%): 0.70-0.95 (slightly more overlap allowed)
  - wild (SHAPE 70-100%): 0.65-0.95 (chaos may cause some overlap)

### Phase 4: Comprehensive Review
- [x] Review `score_density()` targets against ENERGY/SHAPE design
- [x] Review `score_velocity_range()` targets against ACCENT design
- [x] Review `score_regularity()` targets against SHAPE zone design
- [x] Ensure all targets reflect "what the algorithm SHOULD produce" not "what sounds good in isolation"

### Phase 5: Validation and Documentation
- [x] Run full evaluation suite with revised targets
- [x] Verify baseline patterns now score appropriately
- [x] Update baseline.json with new evaluation results
- [x] Document rationale for each target change in code comments

### Phase 6: Verification
- [x] All tests pass (no code changes to test)
- [x] Baseline updated successfully
- [x] No regressions in pattern generation

## Acceptance Criteria

- [x] Syncopation targets revised to match spec's "maximize displacement" intent
- [x] VoiceSeparation targets revised to match COMPLEMENT's gap-filling design
- [x] All 5 Pentagon metrics reviewed for spec alignment
- [x] Code comments explain why each target range was chosen
- [x] baseline.json updated with new evaluation
- [x] All tests pass (no code changes, only evaluation targets)
- [x] No new warnings

## Implementation Notes

### Current Problematic Code (evaluate-expressiveness.py)

```python
# Current syncopation targets (lines 333-339)
def score_syncopation(raw: float, shape: float) -> float:
    if shape < 0.3:
        target_center, target_width = 0.11, 0.14  # expects 0.00-0.22
    elif shape < 0.7:
        target_center, target_width = 0.35, 0.16  # expects 0.22-0.48 - WRONG!
    else:
        target_center, target_width = 0.58, 0.20  # expects 0.42-0.75

# Current voice separation targets (lines 379-387)
def score_voice_separation(raw: float, shape: float) -> float:
    if shape < 0.3:
        target_center, width = 0.75, 0.16  # expects 0.62-0.88
    elif shape < 0.7:
        target_center, width = 0.65, 0.16  # expects 0.52-0.78
    else:
        target_center, width = 0.50, 0.22  # expects 0.32-0.68 - WRONG!
```

### Spec Design Intent

From `06-shape.md`:
- **Syncopated zone (30-70%)**: "Funk, displaced, tension" - designed for HIGH syncopation
- **Wild zone (70-100%)**: "IDM, chaos" - also high syncopation with randomness

From `08-complement.md`:
- COMPLEMENT relationship: "Voice 2 (shimmer) fills gaps in Voice 1"
- Gap-filling by definition prevents overlap, so voiceSeparation should be HIGH

### Files to Modify

1. `/Users/chronick-mbp/git/duopulse/scripts/pattern-viz/evaluate-expressiveness.py`
   - `score_syncopation()` function
   - `score_voice_separation()` function
   - Possibly `score_density()`, `score_velocity_range()`, `score_regularity()`

2. `/Users/chronick-mbp/git/duopulse/tools/evals/metrics/baseline.json`
   - Update after running revised evaluation

## Rationale

The Pentagon metrics are a feedback loop for pattern generation. If targets expect the wrong behavior, the iteration system will "optimize" patterns away from the spec design intent. This task ensures the evaluation system measures what we actually want the algorithms to produce.

## Related

- Iteration 2026-01-20-006: Discovered syncopation target misalignment
- Iteration 2026-01-20-007: Discovered voiceSeparation target misalignment
- Task 68: Preset Conformance Metrics (related evaluation work)
