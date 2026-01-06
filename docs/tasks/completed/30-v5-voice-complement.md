---
id: 30
slug: 30-v5-voice-complement
title: "V5 Voice COMPLEMENT Relationship with DRIFT Variation"
status: completed
created_date: 2026-01-04
updated_date: 2026-01-04
completed_date: 2026-01-04
branch: feature/30-v5-voice-complement
spec_refs:
  - "v5-design-final.md#a4-voice-complement"
depends_on:
  - 27
---

# Task 30: V5 Voice COMPLEMENT Relationship with DRIFT Variation

## Objective

Implement the V5 COMPLEMENT voice relationship where shimmer fills gaps in anchor pattern, with DRIFT controlling placement variation.

## Context

V5 simplifies voice relationships to a single mode: **COMPLEMENT**. The shimmer voice fills gaps in the anchor pattern proportionally, with DRIFT controlling how placement is determined:

| DRIFT Range | Placement Strategy |
|-------------|-------------------|
| 0-30% | Evenly spaced within gaps |
| 30-70% | Weighted by shimmer step weights |
| 70-100% | Seed-varied random within gaps |

Key innovations:
- **Gap detection** including wrap-around handling
- **Proportional distribution**: larger gaps get more hits
- **Division-by-zero guards** for safety
- **Wrap-around gap combination**: tail+head gaps treated as one

This replaces V4's INTERLOCK/SHADOW coupling modes.

## Subtasks

- [x] Implement `FindGaps()` function to detect gaps in anchor mask
- [x] Handle wrap-around: combine tail+head gaps if both are gaps
- [x] Implement proportional hit distribution (`gapShare = gap.length * targetHits / totalGapLength`)
- [x] Add division-by-zero guard (`Max(1, totalGapLength)`)
- [x] Implement DRIFT-based placement:
  - [x] Low DRIFT (0-30%): `EvenlySpaced(gap, j, gapShare)`
  - [x] Medium DRIFT (30-70%): `WeightedBest(gap, shimmerWeights)`
  - [x] High DRIFT (70-100%): `SeedVaried(gap, seed, j)`
- [x] Remove V4 INTERLOCK and SHADOW modes from VoiceRelation.cpp
- [x] Wire COMPLEMENT as the only voice relationship
- [x] Add unit tests for gap detection and placement strategies
- [x] All tests pass (`make test`)

## Acceptance Criteria

- [x] Shimmer hits fill gaps in anchor pattern proportionally
- [x] Wrap-around gaps are properly combined
- [x] DRIFT=0% produces evenly spaced shimmer hits
- [x] DRIFT=50% places hits at weighted best positions
- [x] DRIFT=100% produces seed-varied placement
- [x] No division-by-zero crashes
- [x] Build compiles without errors
- [x] All tests pass

## Implementation Notes

### Algorithm (from v5-design-final.md Appendix A.4)

```cpp
uint32_t ApplyComplementRelationship(uint32_t anchorMask,
                                      const float* shimmerWeights,
                                      float drift, uint32_t seed,
                                      int patternLength, int targetHits)
{
    uint32_t shimmerMask = 0;

    // Find gaps in anchor pattern (including wrap-around)
    Gap gaps[16];
    int gapCount = FindGaps(anchorMask, patternLength, gaps);

    // Handle wrap-around: combine tail+head if both are gaps
    if (gapCount > 1 && gaps[0].start == 0 &&
        gaps[gapCount-1].end == patternLength) {
        CombineWrapAroundGaps(gaps, gapCount);
    }

    // Guard against div-by-zero and preserve DENSITY=0 invariant
    int totalGapLength = SumGapLengths(gaps, gapCount);
    if (totalGapLength == 0 || targetHits == 0) {
        return 0;  // No gaps to fill or no hits requested
    }

    // Distribute shimmer hits proportionally
    for (int g = 0; g < gapCount; ++g) {
        int gapShare = Max(1, (gaps[g].length * targetHits) /
                              Max(1, totalGapLength));

        for (int j = 0; j < gapShare; ++j) {
            int position;

            if (drift < 0.3f) {
                position = EvenlySpaced(gaps[g], j, gapShare);
            } else if (drift < 0.7f) {
                position = WeightedBest(gaps[g], shimmerWeights);
            } else {
                position = SeedVaried(gaps[g], seed, j);
            }

            shimmerMask |= (1U << position);
        }
    }

    return shimmerMask;
}
```

### Gap Structure

```cpp
struct Gap {
    int start;   // First step of gap
    int length;  // Number of steps in gap
    int end;     // start + length (may wrap)
};
```

### Placement Functions

```cpp
int EvenlySpaced(const Gap& gap, int index, int total) {
    // Place hits evenly within gap
    float spacing = gap.length / (float)(total + 1);
    return gap.start + (int)((index + 1) * spacing) % patternLength;
}

int WeightedBest(const Gap& gap, const float* weights) {
    // Find highest weighted step in gap
    int best = gap.start;
    float bestWeight = 0.0f;
    for (int i = 0; i < gap.length; ++i) {
        int step = (gap.start + i) % patternLength;
        if (weights[step] > bestWeight) {
            bestWeight = weights[step];
            best = step;
        }
    }
    return best;
}

int SeedVaried(const Gap& gap, uint32_t seed, int index) {
    // Random position within gap based on seed
    uint32_t hash = Crc32Combine(seed, index);
    return gap.start + (hash % gap.length);
}
```

### Files to Modify

- `src/Engine/VoiceRelation.cpp` - Replace INTERLOCK/SHADOW with COMPLEMENT
- `src/Engine/VoiceRelation.h` - Update function signatures
- `src/Engine/DuoPulseTypes.h` - Remove VoiceCoupling enum (done in Task 27)
- `tests/VoiceRelationTest.cpp` - Update tests for COMPLEMENT

### Constraints

- Real-time safe: fixed-size gap array (max 16 gaps)
- Bounded loops: max patternLength iterations
- Division guards: Max(1, ...) prevents /0

### Risks

- Gap detection edge cases (all gaps, no gaps, single hit)
- Wrap-around handling complexity
