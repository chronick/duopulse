---
id: 43
slug: v5-shimmer-seed-variation
status: completed
---

# Task 43: V5 Shimmer Seed Variation [v5-shimmer-seed-variation]

## Context

At moderate energy levels, changing the seed produces **no audible difference** in patterns. This violates the core design principle:

> "Same settings + seed = identical output" (spec 1.1)

The inverse should hold: **Different seed = different output**. Currently this fails because:

1. `ApplyComplementRelationship()` is never called (dead code)
2. `PlaceEvenlySpaced()` ignores the seed parameter entirely
3. Both voices use independent Gumbel sampling that converges to identical patterns

**Design Decision (Model B)**: Seed should *always* affect the pattern. DRIFT controls *phrase-to-phrase evolution*, not whether seed matters. The current DRIFT=0 behavior (seed ignored) is a bug.

## Baseline Expressiveness (2026-01-07)

```
============================================================
EXPRESSIVENESS EVALUATION SUMMARY
============================================================

Overall Seed Variation: 8%
  V1 (Anchor):  0%
  V2 (Shimmer): 25%
  AUX:          0%

[FAIL] Expressiveness issues detected:
       - V2 (Shimmer) variation is too low
       - V1 (Anchor) variation is too low

LOW VARIATION ZONES:
  [CRITICAL] E=0.30 S=0.15 D=0.00: V2 score: 0%
  [CRITICAL] E=0.30 S=0.50 D=0.00: V2 score: 0%
  [CRITICAL] E=0.60 S=0.15 D=0.00: V2 score: 0%

CONVERGENCE PATTERNS: 9 found
  0x00020202: 4 seeds produce same pattern
  0x00101010: 4 seeds produce same pattern
  0x02020202: 4 seeds produce same pattern
```

**Target**: V2 variation score >= 50%

## Tasks

### Phase 1: Wire Up COMPLEMENT (Critical Bug Fix)

- [x] **43.1** In `Sequencer.cpp:392`, replace `ApplyVoiceRelationship()` with `ApplyComplementRelationship()`
  - Pass shimmer weights, drift, seed, and target hit count
  - Verify shimmer now fills gaps in anchor pattern
  - Run `make test` to ensure no regressions

- [x] **43.2** Remove deprecated `ApplyVoiceRelationship()` function from `VoiceRelation.cpp:424-435`

### Phase 2: Seed-Sensitive Placement

- [x] **43.3** Add seed parameter to `PlaceEvenlySpaced()` at `VoiceRelation.cpp:217`
  - Compute seed-based phase offset (0-50% of one spacing unit)
  - Use `HashToFloat(seed, gap.start)` for deterministic variation
  - Preserves even distribution while adding seed variation

```cpp
int PlaceEvenlySpacedWithSeed(const Gap& gap, int hitIndex, int totalHits,
                               int patternLength, uint32_t seed)
{
    if (totalHits <= 0) return gap.start;

    float spacing = static_cast<float>(gap.length) / static_cast<float>(totalHits + 1);
    float phase = HashToFloat(seed, gap.start) * 0.5f;  // 0-50% offset
    int position = gap.start + static_cast<int>(spacing * (hitIndex + 1 + phase) + 0.5f);

    return position % patternLength;
}
```

- [x] **43.4** Update call site at `VoiceRelation.cpp:378-385` to pass seed

### Phase 3: Seed-Based Rotation (Quick Win)

- [x] **43.5** After shimmer mask is computed, apply seed-based rotation
  - `int rotation = HashToInt(seed) % patternLength`
  - `shimmerMask = RotateBitMask(shimmerMask, rotation)`
  - Re-check for anchor collisions after rotation

### Phase 4: Raise Default DRIFT

- [x] **43.6** In `ControlState.h`, change default DRIFT from 0.0 to 0.25
  - This enables weighted placement that uses seed
  - Document as breaking change in release notes

## Verification

After each phase, run expressiveness evaluation:

```bash
make expressiveness-quick
```

**Success Criteria**:
- V2 variation score >= 50% (currently 25%)
- V1 variation score >= 25% (currently 0%)
- No test regressions (`make test` passes)

## Files to Modify

| File | Changes |
|------|---------|
| `src/Engine/Sequencer.cpp` | Wire up `ApplyComplementRelationship()` |
| `src/Engine/VoiceRelation.cpp` | Add seed to `PlaceEvenlySpaced()`, add rotation |
| `src/Engine/VoiceRelation.h` | Update function signature if needed |
| `src/Engine/ControlState.h` | Raise default DRIFT to 0.25 |

## Reference Documents

- `docs/design/v5-critique/` - Initial pattern analysis and diagnosis
- `docs/design/v5-iteration/` - Design alternatives and trade-offs
- `docs/design/research/expressiveness-approaches.md` - Techniques from Grids, Pam's, etc.
- `docs/design/expressiveness-evaluation.md` - How to run and interpret evaluation
- `.claude/skills/eurorack-ux/SKILL.md` - Design principles (Quadrant Test)

## Spec References

- Section 1.1: "Deterministic variation: Same settings + seed = identical output"
- Section 7: Voice Relationship (COMPLEMENT)
- Section 4.3: DRIFT parameter behavior
