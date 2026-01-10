# Iteration 1: Algorithm Analysis and Proposals

**Date**: 2026-01-08
**Focus**: Diagnosing variation bottlenecks and proposing solutions

## Current Algorithm Analysis

### 1. Weight Generation Pipeline

Current flow:
```
PatternParams
    |
    v
ComputeShapeBlendedWeights()  ─── Uses fixed weight tables
    |                              (stable/syncopation/wild)
    v
ApplyAxisBias()               ─── Modifies weights based on AXIS X/Y
    |
    v
Seed-based noise perturbation ─── 0.4 * (1-shape) noise scale
    |
    v
SelectHitsGumbelTopK()        ─── Samples from weights
    |
    v
RotateWithPreserve()          ─── Rotates pattern but preserves step 0
    |
    v
ApplyComplementRelationship() ─── Shimmer fills anchor gaps
    |
    v
AUX generation                ─── Inverse metric weight + avoids main voices
```

### 2. Identified Bottlenecks

#### Bottleneck A: Static Base Patterns

`GenerateStablePattern()`, `GenerateSyncopationPattern()`, `GenerateWildPattern()` produce the same base weights for every seed. The only variation comes from:
- Seed-based humanization jitter (small)
- Seed offset for syncopation suppression (0.5-0.7 range)
- Chaos injection in wild zone

**Problem**: Two patterns with SHAPE=0.25 and different seeds will start with identical base weights before tiny noise is added.

#### Bottleneck B: Step 0 Preservation

`RotateWithPreserve()` always keeps step 0 in place (beat 1 kick). Combined with:
- `GenerateStablePattern()` giving step 0 weight=1.0
- Guard rails forcing beat 1 kick in techno

**Result**: 97% of patterns have anchor on step 0, destroying variation.

#### Bottleneck C: Complement Relationship Similarity

Shimmer is derived FROM anchor pattern. If anchor patterns cluster, shimmer will cluster similarly because it's filling the same gap structures.

#### Bottleneck D: AUX Fixed Formula

```cpp
float metricW = GetMetricWeight(i, params.patternLength);
auxWeights[i] = 1.0f - metricW * 0.5f;  // Always same bias
if ((combinedMask & (1U << i)) != 0)
    auxWeights[i] *= 0.3f;               // Avoids main voices
```

This produces nearly identical AUX patterns because:
1. Metric weight table is fixed
2. Only variation is from anchor+shimmer positions (which also cluster)

---

## Proposed Solutions

### Solution 1: Seed-Varied Base Pattern Generation

**Concept**: Inject seed variation into the BASE pattern generation, not just as post-processing noise.

**Implementation**:

```cpp
void GenerateStablePattern(float energy, uint32_t seed, int patternLength, float* outWeights)
{
    // NEW: Seed-based downbeat strength variation
    float beat1Strength = 0.85f + HashToFloat(seed, 0) * 0.15f;  // 0.85-1.0
    float beat3Strength = 0.75f + HashToFloat(seed, 1) * 0.15f;  // 0.75-0.9

    // NEW: Seed-based 8th note position variation
    float eighthWeight = 0.4f + HashToFloat(seed, 2) * 0.2f;     // 0.4-0.6

    // NEW: Seed-based primary pattern rotation
    int primaryRotation = static_cast<int>(HashToFloat(seed, 3) * 4);  // 0-3 steps

    for (int step = 0; step < patternLength; ++step)
    {
        int rotatedStep = (step + patternLength - primaryRotation) % patternLength;
        // ... use rotatedStep for metric position lookup
    }
}
```

**Expected Impact**: Every seed produces a fundamentally different stable pattern structure.

---

### Solution 2: Probabilistic Beat 1 Presence

**Concept**: Don't force beat 1 kick in all cases. Allow SHAPE and seed to influence it.

**Implementation**:

```cpp
// In GeneratePattern, after guard rails:
bool allowBeat1Variation = (params.shape > 0.4f) ||
                           (HashToFloat(params.seed, 500) < params.axisX * 0.4f);

if (allowBeat1Variation)
{
    // Don't use RotateWithPreserve, use regular rotation
    // OR remove step 0 from anchor with some probability
    float skipBeat1Prob = params.shape * params.axisX * 0.5f;
    if (HashToFloat(params.seed, 501) < skipBeat1Prob)
    {
        result.anchorMask &= ~(1U << 0);  // Remove beat 1
    }
}
```

**Guard Rails Modification**:
```cpp
// In ApplyHardGuardRails:
// Only force beat 1 for MINIMAL and GROOVE zones in TECHNO genre
if (genre == Genre::TECHNO && zone <= EnergyZone::GROOVE)
{
    anchorMask |= (1U << 0);  // Force beat 1
}
// For higher energy or non-techno, allow variation
```

**Expected Impact**: Reduce step 0 anchor activation from 97% to ~70-80% (still mostly present, but with variation).

---

### Solution 3: Seed-Infused Shimmer Generation

**Concept**: Instead of purely gap-filling, give shimmer its own seed-derived character.

**Implementation**:

```cpp
uint32_t ApplyComplementRelationship(
    uint32_t anchorMask,
    const float* shimmerWeights,
    float drift,
    uint32_t seed,
    int patternLength,
    int targetHits)
{
    // NEW: Compute shimmer's own "personality" from seed
    int shimmerRotation = static_cast<int>(HashToFloat(seed, 100) * 8);
    float shimmerAxisBias = HashToFloat(seed, 101) - 0.5f;  // -0.5 to +0.5

    // Apply rotation and bias to shimmer weights before gap-filling
    float adjustedWeights[kMaxSteps];
    for (int i = 0; i < patternLength; ++i)
    {
        int srcStep = (i + patternLength - shimmerRotation) % patternLength;
        adjustedWeights[i] = shimmerWeights[srcStep];

        // Apply seed-derived axis bias
        float posStrength = GetPositionStrength(i, patternLength);
        adjustedWeights[i] *= (1.0f + shimmerAxisBias * posStrength);
    }

    // Existing gap-filling logic with adjusted weights
    // ...
}
```

**Expected Impact**: Shimmer patterns vary independently of anchor clustering.

---

### Solution 4: Dynamic AUX Weight Generation

**Concept**: Give AUX its own seed-derived pattern logic instead of fixed inverse-metric.

**Implementation**:

```cpp
void GenerateAuxWeights(
    const PatternParams& params,
    uint32_t combinedMask,
    float* auxWeights)
{
    // NEW: Seed-derived AUX character
    int auxStyle = static_cast<int>(HashToFloat(params.seed, 200) * 4);  // 0-3
    float auxOffset = HashToFloat(params.seed, 201) * 8;  // 0-8 step offset

    for (int i = 0; i < params.patternLength; ++i)
    {
        float weight;

        switch (auxStyle)
        {
            case 0:  // Offbeat 8ths
                weight = (i % 2 == 1) ? 0.8f : 0.2f;
                break;

            case 1:  // Syncopated 16ths
                weight = ((i % 4 == 1) || (i % 4 == 3)) ? 0.7f : 0.4f;
                break;

            case 2:  // Every 3rd (polyrhythm)
                weight = (i % 3 == 0) ? 0.8f : 0.3f;
                break;

            case 3:  // Inverse anchor (current behavior)
            default:
                weight = 1.0f - GetMetricWeight(i, params.patternLength) * 0.5f;
                break;
        }

        // Apply offset rotation
        int rotatedI = (i + static_cast<int>(auxOffset)) % params.patternLength;
        auxWeights[i] = ClampWeight(weight);

        // Still avoid main voices
        if ((combinedMask & (1U << i)) != 0)
            auxWeights[i] *= 0.3f;
    }
}
```

**Expected Impact**: AUX patterns vary significantly based on seed, not just minor position shifts.

---

### Solution 5: Named Preset Seed Derivation

**Concept**: Each named preset gets a characteristic seed derived from its name/parameters, ensuring distinct starting points.

**Implementation**:

```cpp
uint32_t ComputePresetSeed(const char* presetName, const PatternParams& baseParams)
{
    // Hash the preset name for base seed
    uint32_t nameSeed = HashString(presetName);

    // XOR with parameter-derived values for additional variation
    uint32_t paramSeed = static_cast<uint32_t>(baseParams.shape * 1000) ^
                         static_cast<uint32_t>(baseParams.energy * 10000) ^
                         static_cast<uint32_t>(baseParams.axisX * 100000);

    return nameSeed ^ paramSeed;
}
```

This ensures "Four on Floor" and "Berlin Loop" (both low SHAPE) get different seeds even with similar parameters.

---

### Solution 6: Per-Step Probability Injection

**Concept**: Add a small random probability offset to each step based on seed.

**Current** (in PatternGenerator.cpp):
```cpp
const float noiseScale = 0.4f * (1.0f - params.shape);
```

**Proposed**:
```cpp
// Increase noise contribution, make it less SHAPE-dependent
const float baseNoise = 0.15f;  // Always some variation
const float shapeNoise = 0.35f * (1.0f - params.shape * 0.5f);  // Less suppression
const float noiseScale = baseNoise + shapeNoise;  // 0.15 to 0.50

// Also: vary the noise SEED per step more distinctly
for (int step = 0; step < params.patternLength; ++step) {
    // Use step * prime to spread hashes more
    float noise = (HashToFloat(params.seed, step * 31 + 1000) - 0.5f) * noiseScale * 2.0f;
    anchorWeights[step] = ClampWeight(anchorWeights[step] + noise);
}
```

---

## Implementation Priority

| Priority | Solution | Effort | Impact |
|----------|----------|--------|--------|
| 1 | Solution 4: Dynamic AUX | Low | High (AUX only 20% unique) |
| 2 | Solution 1: Seed-Varied Base | Medium | High (fixes root cause) |
| 3 | Solution 6: Noise Injection | Low | Medium |
| 4 | Solution 2: Beat 1 Variation | Medium | Medium |
| 5 | Solution 3: Shimmer Personality | Medium | Medium |
| 6 | Solution 5: Preset Seeds | Low | Low (presets already work better) |

---

## Testing Plan

1. **Before changes**: Run fitness evaluator on current algorithm, record baseline
2. **After each solution**: Re-run fitness evaluator, compare to baseline
3. **Success criteria**:
   - Uniqueness ratio > 60% for all voices
   - Overall variation > 55%
   - No regression in coherence or genre authenticity

---

## Questions for User

Before proceeding with implementation, I'd like to clarify:

1. **Beat 1 kick importance**: How critical is it that beat 1 almost always has a kick? Should IDM presets allow beat 1 to be empty?

2. **AUX voice role**: Is AUX meant to be hi-hat-like (offbeat emphasis) or more flexible? Can it adopt different rhythmic roles per seed?

3. **Named preset determinism**: Should the same named preset always produce EXACTLY the same pattern, or can it vary slightly?

4. **Performance tolerance**: These changes add more hash computations. Is ~5-10% more CPU acceptable?

---

## Next Steps

1. Get user feedback on questions above
2. Implement Solution 4 (Dynamic AUX) as proof of concept
3. Measure impact with fitness evaluator
4. Iterate on remaining solutions
