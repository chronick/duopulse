# V5 Design Critique: Shimmer Variation Problem

**Date**: 2026-01-07
**Status**: Needs Revision
**Verdict**: NOT ready to ship (critical expressiveness gap)

## Summary

Visual analysis of pattern generation reveals that **shimmer patterns are identical across different seeds at moderate energy levels**. This fundamentally undermines the module's value proposition as an algorithmic drum sequencer - the "algorithm" produces deterministic output regardless of seed at typical settings.

## Simplicity-Expressiveness Analysis

### Current Position: "One-Trick Pony" Quadrant

```
                    HIGH EXPRESSIVENESS
                           |
       "Expert's Dream"    |    "Sweet Spot" <- TARGET
       (Complex but        |
        powerful)          |
    -----------------------+--------------------
       "Useless"           |    "One-Trick Pony"
       (Hard AND limited)  |          ^
                           |          | CURRENT
                    LOW EXPRESSIVENESS
```

**Gap Analysis**:
- **Simplicity**: HIGH - Module is easy to use, defaults work
- **Expressiveness**: LOW - Seed changes produce no audible difference at moderate settings

### Path to Sweet Spot

The fix requires injecting seed variation earlier in the pipeline without sacrificing the "musically useful defaults" that give high simplicity.

## Critical Issues

### 1. Shimmer Convergence at Moderate Energy

**Location**: `src/Engine/VoiceRelation.cpp` lines 378-385
**Problem**: At DRIFT < 0.3 (including default 0.0), shimmer uses `PlaceEvenlySpaced()` which is purely deterministic - seed is never consulted.

**Impact**:
- Reseed gesture feels broken ("Is it even working?")
- Same kick-snare relationship bar after bar
- Module becomes predictable/boring

**Evidence**: In `docs/patterns.html` Seed Variation section, all four seeds produce identical V2 masks at SHAPE=0.5, ENERGY=0.6.

**Recommendation**: Inject seed-based micro-jitter into even-spacing algorithm. See recommendations.md R1.

### 2. Over-Constrained Anchor at GROOVE Zone

**Location**: `src/Engine/GuardRails.cpp`
**Problem**: Min-spacing of 4 steps plus downbeat enforcement leaves exactly one valid anchor pattern at moderate energy.

**Impact**: When anchor is locked to four-on-floor, shimmer has identical gaps to fill regardless of seed.

**Evidence**: Tests in `pattern_viz_test.cpp` document this as "by design" but it conflicts with spec promise of seed-based variation.

**Recommendation**: Consider relaxing GROOVE spacing from 4 to 2 steps. See recommendations.md R3.

### 3. Default DRIFT Bypasses Variation

**Location**: Default parameter initialization
**Problem**: DRIFT=0.0 triggers deterministic placement. Users must manually raise DRIFT to get seed-sensitive shimmer.

**Impact**: First-time users never discover that patterns can vary.

**Recommendation**: Raise default DRIFT to 0.25. See recommendations.md R2.

## Strengths

Despite the critical issue, the design has solid foundations:

1. **COMPLEMENT relationship works** - V1 and V2 never collide
2. **SHAPE zones produce different weight distributions** - Algorithm is sound
3. **Guard rails ensure musical coherence** - Patterns never feel random
4. **Velocity variation exists** - Even if masks converge, dynamics differ slightly
5. **High energy patterns vary** - The problem is localized to moderate energy

## Files in This Critique

| File | Purpose |
|------|---------|
| `pattern-analysis.md` | Observations from the HTML visualization |
| `shimmer-diagnosis.md` | Technical root cause analysis |
| `recommendations.md` | Proposed solutions with risk assessment |

## Next Actions

1. **Immediate** (Low Risk): Implement R1 (seed jitter) + R2 (raise DRIFT default)
2. **Short Term** (Medium Risk): Implement R5 (Gumbel in gaps) if R1+R2 insufficient
3. **Testing**: Add test case requiring 3+ unique shimmer masks from 4 seeds at moderate energy

## Verdict

- [ ] Ready to implement
- [x] **Needs another iteration** (seed variation for shimmer)
- [ ] Needs fundamental rethinking

The core architecture is sound. The issue is that constraints overwhelm variation at moderate energy. Recommended fixes are incremental, not structural.
