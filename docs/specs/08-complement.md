# Voice Relationship (COMPLEMENT)

[← Back to Index](00-index.md) | [← AXIS Biasing](07-axis.md)

---

Voice 2 (shimmer) fills gaps in Voice 1 (anchor) pattern. DRIFT controls placement variation.

## 1. Gap Finding

```
gaps = FindGaps(anchorMask, patternLength)

// Handle wrap-around: combine tail+head if both are gaps
IF gaps[0].start == 0 AND gaps[last].end == patternLength:
  CombineWrapAroundGaps(gaps)
```

---

## 2. Shimmer Placement by DRIFT

| DRIFT Range | Placement Strategy |
|-------------|-------------------|
| 0-30% | Evenly spaced within gaps |
| 30-70% | Weight-based selection (best metric positions) |
| 70-100% | Seed-varied (controlled randomness) |

```
FOR each gap:
  gapShare = max(1, round(gap.length * targetHits / totalGapLength))

  FOR j = 0 TO gapShare - 1:
    IF drift < 0.3:
      position = EvenlySpaced(gap, j, gapShare)
    ELSE IF drift < 0.7:
      position = WeightedBest(gap, shimmerWeights)
    ELSE:
      position = SeedVaried(gap, seed, j)

    PlaceShimmerHit(position)
```

---

[Next: ACCENT Velocity →](09-accent.md)
