# Design Iteration: Shimmer Pattern Variation

**Session**: v5-iteration
**Date**: 2026-01-07
**Status**: Pending User Clarification
**Related Critique**: `docs/design/v5-critique/`

## Problem Statement

At moderate energy levels (GROOVE zone, ~35% ENERGY), changing the seed produces no audible difference in shimmer (Voice 2) patterns. This violates the core principle from Section 1.1 of the spec:

> "Deterministic variation: Same settings + seed = identical output"

The contrapositive should hold: **Different seed = different output**. Currently, this fails at moderate energy because:

1. **Anchor converges**: Guard rails + 4-step spacing force four-on-floor
2. **Shimmer is deterministic**: At DRIFT < 0.3, `PlaceEvenlySpaced()` ignores seed entirely
3. **Default DRIFT bypasses variation**: DRIFT=0.0 ensures all shimmer uses even spacing

## Blocking Question

**Semantic clarification needed**: What is the intended relationship between DRIFT and seed?

| Model | DRIFT=0 Behavior | DRIFT=1 Behavior |
|-------|------------------|------------------|
| **A** | Seed has no effect | Seed fully controls placement |
| **B** | Seed sets initial pattern (locked) | Pattern evolves each phrase |

The spec language suggests Model B, but implementation follows Model A.

**User input required before proceeding.**

## Analysis Summary

### Files Examined
- `src/Engine/VoiceRelation.cpp` - COMPLEMENT relationship, even-spacing logic
- `src/Engine/GumbelSampler.cpp` - Anchor selection via Gumbel Top-K
- `src/Engine/GuardRails.cpp` - Downbeat enforcement, spacing constraints
- `src/Engine/ControlState.h` - Default DRIFT=0.0

### Root Cause Chain
```
                DRIFT=0.0 (default)
                      |
                      v
            PlaceEvenlySpaced() called
                      |
                      v
             No seed parameter used
                      |
                      v
         Identical shimmer for all seeds
```

### Constraint Analysis

At GROOVE zone (~35% energy) with 32-step patterns:
- Anchor budget: ~8 hits
- Min spacing: 2 steps (after Task 41 fix)
- Guard rails: Step 0 required
- Result: Multiple valid anchor patterns exist, BUT...
- Shimmer placement: Deterministic even-spacing ignores seed

Even if anchor varies, shimmer doesn't (at low DRIFT).

## Design Principles Applied

From `eurorack-ux` skill:

1. **Quadrant Test**: Currently in "One-Trick Pony" (simple but limited)
   - Target: "Sweet Spot" (simple surface, deep control)

2. **Default States**: "Defaults should be musically useful"
   - Reseed should produce audibly different patterns at defaults

3. **Simplicity-Expressiveness Dialectic**: "Maximize BOTH"
   - Fix should add expressiveness without adding control complexity

## Preliminary Recommendations

Pending clarification, the likely fix path:

### Phase 1: Minimal Viable Fix (Model B assumed)

**R1: Seed-based phase offset in even spacing**

Inject seed into `PlaceEvenlySpaced()` to determine starting phase:

```cpp
int PlaceEvenlySpacedWithSeed(const Gap& gap, int hitIndex, int totalHits,
                               int patternLength, uint32_t seed)
{
    if (totalHits <= 0) return gap.start;

    // Seed determines phase offset (0-50% of one spacing unit)
    float phase = HashToFloat(seed, 0) * 0.5f;

    float spacing = static_cast<float>(gap.length) / static_cast<float>(totalHits + 1);
    int position = gap.start + static_cast<int>(spacing * (hitIndex + 1 + phase) + 0.5f);

    return position % patternLength;
}
```

**Trade-offs**:
- (+) Preserves even distribution character
- (+) Minimal code change (add seed parameter)
- (+) No control surface changes
- (-) Variation is subtle (phase shift only)

**R2: Raise default DRIFT to 0.25**

In `src/Engine/ControlState.h` line 387:
```cpp
drift = 0.25f;  // Was 0.0f - enables weighted placement for shimmer
```

**Trade-offs**:
- (+) Engages weighted placement immediately
- (+) Simple change
- (-) Breaks backward compatibility (existing patches sound different)
- (-) May surprise users who expected locked patterns

### Phase 2: If Insufficient

**R3: Gumbel selection within gaps**

Replace even-spacing entirely with Gumbel Top-K selection constrained to each gap:

```cpp
// Instead of PlaceEvenlySpaced, use:
uint32_t gapHits = SelectHitsGumbelTopK(
    shimmerWeights,
    CreateGapMask(gap),
    hitsForGap,
    seed ^ (gapIndex * 0x1234567),
    patternLength,
    0  // No spacing within gap
);
```

**Trade-offs**:
- (+) Seed naturally varies placement
- (+) Uses existing Gumbel infrastructure
- (-) More implementation effort
- (-) Shimmer might cluster at weight peaks

## Alternative Approaches (from Codex)

See `codex-output.md` for detailed alternatives:

1. **Phase Offset** - Conservative, low risk
2. **Weighted Even (hybrid)** - Musical refinement
3. **Pre-Seed Anchor Jitter** - Attack root cause
4. **Probabilistic Blend** - Organic variation

## Success Criteria

After implementation:

1. **Seed Variation Test**: At SHAPE=0.3, ENERGY=0.5, DRIFT=0.0:
   - 4 different seeds produce at least 3 different shimmer masks

2. **Musical Coherence Test**:
   - Patterns still feel "groovy" at moderate energy
   - No jarring randomness

3. **Quadrant Position**:
   - Module in "Sweet Spot": simple defaults, deep control available

## Next Steps

1. **Receive user clarification** on DRIFT semantics
2. **Implement R1** (seed in even-spacing)
3. **Evaluate** if variation is sufficient
4. **If insufficient**, implement R2 or R3
5. **Add test case** for seed variation requirement
6. **Create SDD task** for implementation

---

## Open Questions

- [ ] DRIFT semantics: Model A or Model B?
- [ ] Backward compatibility: Acceptable to change default DRIFT?
- [ ] How much variation is "enough"? (Subtle phase vs dramatic reordering)
