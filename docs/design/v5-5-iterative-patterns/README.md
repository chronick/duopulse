# Design Session: Pattern Generation v5.5 Iteration

**Session ID**: v5-5-iterative-patterns
**Status**: Iteration 2 Complete - Awaiting Review
**Created**: 2026-01-08
**Last Updated**: 2026-01-08
**Current Iteration**: 2

## Problem Statement

The current v5 pattern generation algorithm produces insufficient variation across the parameter space. Analysis of 116 patterns (default seed) shows:

| Metric | Current | Target |
|--------|---------|--------|
| V1 (Anchor) unique patterns | 30.4% (36/116) | >= 70% |
| V2 (Shimmer) unique patterns | 41.7% (49/116) | >= 70% |
| AUX unique patterns | 20.0% (24/116) | >= 70% |
| Overall variation score | 30.7% | >= 65% |

**Root Cause Hypothesis**: The algorithm relies too heavily on:
1. Fixed metric weight tables (same weights for all seeds/params)
2. Deterministic stable/syncopation/wild pattern generators with insufficient variation
3. COMPLEMENT relationship producing similar shimmer patterns from similar anchor patterns
4. AUX generation using inverted metric weights (same bias every time)

## Goals

1. **Maximize expressiveness**: Unique patterns should emerge from different parameter combinations
2. **Maintain musicality**: Patterns must still sound coherent within their genre context
3. **Parameter responsiveness**: Every control should meaningfully affect output
4. **Genre authenticity**: Named presets should sound like their declared style

## Constraints

- Real-time safe (no heap allocation in audio path)
- Deterministic (same seed + params = same output)
- Backward compatible (existing workflow preserved)
- Hardware constraints (Daisy Patch.init(), limited memory)

## Session Files

| File | Purpose |
|------|---------|
| `README.md` | This file - session overview |
| `resources/drum-patterns-research.md` | Research on classic drum patterns |
| `resources/fitness-metrics-framework.md` | Quantifiable metrics for optimization |
| `iteration-1.md` | First iteration analysis and proposal |
| `critique-1.md` | Design review feedback on iteration 1 |
| `iteration-2.md` | **CURRENT** - Revised proposals addressing critique |
| `codex-input.md` | Prompts for alternative perspective (if needed) |
| `codex-output.md` | Responses from alternative analysis |
| `final.md` | Final recommendation (when complete) |

## Key Findings

### Current Algorithm Issues

1. **Step 0 Anchor Lock**: 97% of patterns have V1 hit on step 0 - nearly no variation on beat 1
2. **AUX Clustering**: Only 24 unique AUX patterns from 116 - too uniform
3. **Per-step frequency analysis** shows predictable clustering:
   - V1 heavily biased to steps 0, 3, 4, 10, 26, 31
   - V2 heavily biased to steps 8, 19, 27
   - AUX locked to specific offbeat positions

### Design Principles (from eurorack-ux skill)

- Every parameter adjustment should produce audible change
- No dead zones in parameter ranges
- Maximize both simplicity AND expressiveness (not trade-offs)
- 3-second rule: understand state, make change, recover from error

## Iteration 2 Summary

Iteration 2 addresses critical feedback from the design review:

### Locked Design Decisions
1. **Beat 1**: ALWAYS present when SHAPE < 0.7
2. **AUX Style**: Controlled by AXIS Y zones (user has control)
3. **Named Presets**: MUST be deterministic
4. **Rotation**: Only in syncopated zone (SHAPE 0.3-0.7)

### Priority Order (Simplicity First)
1. Fix noise formula (bug from iteration 1)
2. Velocity variation (quick win, no complexity)
3. Beat 1 enforcement (locked decision)
4. AUX style zones (user control)
5. Micro-displacement (replaces rotation)
6. Two-pass generation (if needed)

### New Metrics
- Added groove quality sub-metrics to COHERENCE
- Accent alignment, IOI distribution, velocity correlation
- Prevents optimizing for "different" without "groovy"

## Next Steps

1. Review iteration-2.md proposals
2. Approve or request changes to locked decisions
3. Implement Phase 1 (bug fixes) and Phase 2 (quick wins)
4. Run fitness evaluator with groove quality metrics
5. A/B listening tests to validate musicality
