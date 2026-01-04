---
id: 28
slug: 28-v5-shape-algorithm
title: "V5 SHAPE Parameter: 3-Way Blending with Crossfade Zones"
status: pending
created_date: 2026-01-04
updated_date: 2026-01-04
branch: feature/28-v5-shape-algorithm
spec_refs:
  - "v5-design-final.md#a2-shape-blending"
depends_on:
  - 27
---

# Task 28: V5 SHAPE Parameter: 3-Way Blending with Crossfade Zones

## Objective

Implement the V5 SHAPE parameter that controls pattern regularity through three character zones with smooth crossfade transitions.

## Context

SHAPE (K2) replaces the V4 BUILD parameter with entirely different semantics:

| Zone | SHAPE Range | Character |
|------|-------------|-----------|
| Zone 1 | 0-30% | Stable humanized euclidean (techno, four-on-floor) |
| Zone 2 | 30-70% | Syncopation peak (funk, displaced, tension) |
| Zone 3 | 70-100% | Wild weighted (IDM, chaos) |

Key innovations:
- **4% crossfade overlap windows** at zone boundaries (28-32%, 48-52%, 68-72%)
- **Syncopation pattern** with downbeat protection (beat 1 at 50-70% weight)
- **Three base archetypes**: Stable, Syncopated, Wild
- **Humanization** in Zone 1 (±5% jitter decreasing with shape)
- **Chaos injection** in Zone 3 (up to ±15% variation)

## Subtasks

- [ ] Create `GenerateStablePattern()` - Euclidean-based weights with energy scaling
- [ ] Create `GenerateSyncopationPattern()` - Beat suppression with anticipation boost
- [ ] Create `GenerateWildPattern()` - Weighted random with high variation
- [ ] Implement `ComputeShapeBlendedWeights()` with 7-zone logic (3 pure + 4 crossfade)
- [ ] Add humanization in Zone 1 (HashToFloat for per-step jitter)
- [ ] Add chaos injection in Zone 3 (seed-based variation)
- [ ] Update syncopation beat 1 weight to 0.50 base (P1 fix from review)
- [ ] Wire SHAPE parameter through PatternField to generation pipeline
- [ ] Add unit tests for zone transitions and crossfade behavior
- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] SHAPE=0% produces stable, humanized euclidean patterns
- [ ] SHAPE=50% produces peak syncopation with displaced downbeats
- [ ] SHAPE=100% produces wild, chaotic patterns
- [ ] Zone transitions are smooth (no audible discontinuity)
- [ ] Same seed + same SHAPE = identical output (determinism)
- [ ] Build compiles without errors
- [ ] All tests pass

## Implementation Notes

### Algorithm (from v5-design-final.md Appendix A.2)

```cpp
void ComputeShapeBlendedWeights(float shape, float energy,
                                 uint32_t seed, int patternLength,
                                 float* outWeights)
{
    // Generate three base archetypes
    float stableWeights[32];
    float syncopatedWeights[32];
    float wildWeights[32];

    GenerateStablePattern(energy, patternLength, stableWeights);
    GenerateSyncopationPattern(energy, seed, patternLength, syncopatedWeights);
    GenerateWildPattern(energy, seed, patternLength, wildWeights);

    for (int step = 0; step < patternLength; ++step) {
        if (shape < 0.28f) {
            // Pure Zone 1: stable with humanization
            float t = shape / 0.28f;
            outWeights[step] = stableWeights[step];
            float humanize = 0.05f * (1.0f - t);
            outWeights[step] += (HashToFloat(seed, step) - 0.5f) * humanize;
        }
        else if (shape < 0.32f) {
            // Crossfade Zone 1 -> Zone 2a
            float fadeT = (shape - 0.28f) / 0.04f;
            // ... interpolate between zones
        }
        // ... continue for all 7 zones

        outWeights[step] = Clamp(outWeights[step], 0.0f, 1.0f);
    }
}
```

### Syncopation Pattern (with P1 fix)

```cpp
void GenerateSyncopationPattern(float energy, uint32_t seed,
                                 int patternLength, float* weights)
{
    for (int step = 0; step < patternLength; ++step) {
        float metricStrength = GetMetricWeight(step, patternLength);

        if (metricStrength > 0.9f) {
            // Beat 1 - SUPPRESS but protect (P1 fix: increased from 0.35)
            weights[step] = 0.50f + energy * 0.20f;  // 50-70%
        }
        else if (IsAnticipationPosition(step, patternLength)) {
            // Anticipation (step before downbeat) - BOOST
            weights[step] = 0.80f + energy * 0.15f;
        }
        else if (metricStrength < 0.3f) {
            // Weak offbeats - BOOST
            weights[step] = 0.70f + energy * 0.20f;
        }
        // ...
    }
}
```

### Files to Modify

- `src/Engine/PatternField.cpp` - Add new generation functions, update blending
- `src/Engine/PatternField.h` - Add function declarations
- `src/Engine/ArchetypeData.cpp` - May need archetype weight updates
- `tests/PatternFieldTest.cpp` - Add zone transition tests

### Hash Function for Determinism

Add to Crc32.h or a new utility:
```cpp
// Simple multiplicative hash for deterministic float generation
inline float HashToFloat(uint32_t seed, int step) {
    // Fast hash combining seed and step
    uint32_t hash = seed ^ (step * 0x9E3779B9);
    hash ^= hash >> 16;
    hash *= 0x85EBCA6B;
    hash ^= hash >> 13;
    return (hash & 0xFFFF) / 65535.0f;  // 0.0 to 1.0
}
```

**Note**: This function must be added once and shared by Tasks 28, 29, and 35.

### Constraints

- Real-time safe: no allocations, bounded loops
- Deterministic: same inputs = same outputs
- All weights in 0.0-1.0 range

### Risks

- Zone boundary tuning may need hardware iteration
- Syncopation feel is subjective - may need adjustment
