---
id: 49
slug: v55-aux-style-beat1
title: "V5.5 AUX Style Zones and Beat 1 Enforcement"
status: pending
created_date: 2026-01-08
updated_date: 2026-01-08
branch: feature/v55-pattern-expressiveness
spec_refs:
  - "docs/specs/main.md#6-axis-biasing"
  - "docs/design/v5-5-iterative-patterns.md#32-aux-style-control-via-axis-y"
  - "docs/design/v5-5-iterative-patterns.md#31-beat-1-presence"
related:
  - 46  # V5.5 Noise Formula Fix
  - 47  # V5.5 Velocity Variation
  - 48  # V5.5 Micro-Displacement
depends_on:
  - 46  # Bug fixes first
---

# Task 49: V5.5 AUX Style Zones and Beat 1 Enforcement

## Objective

Implement two locked design decisions from v5.5:
1. **AUX Style Zones**: AXIS Y controls AUX style through zones (offbeat, syncopated, seed-varied)
2. **Beat 1 Enforcement**: Beat 1 always present when SHAPE < 0.7

## Context

### AUX Style Problem

Current AUX generation uses a fixed formula:
```cpp
auxWeights[i] = 1.0f - metricW * 0.5f;
```

This produces only 24 unique patterns from 116 tests (20% variation). Users cannot influence AUX character.

### AUX Style Solution

AXIS Y zones control style (from design doc Section 3.2):

| AXIS Y | AUX Style | Character |
|--------|-----------|-----------|
| 0-33% | OFFBEAT_8THS | Classic hat pattern |
| 33-66% | SYNCOPATED_16THS | Funkier, more fills |
| 66-100% | SEED_VARIED | Polyrhythmic, seed-influenced |

### Beat 1 Problem

Beat 1 can be missing even at low SHAPE values, breaking live DJ mixing.

### Beat 1 Solution

From design doc Section 3.1:
- SHAPE < 0.7: Beat 1 ALWAYS present
- SHAPE >= 0.7: Probabilistic skip (0% at 0.7, 40% at 1.0)

## Subtasks

### AUX Style Zones
- [ ] Create `AuxStyle` enum in `PatternGenerator.h`
- [ ] Create `GetAuxStyle(float axisY)` function
- [ ] Create `GenerateAuxWeights(params, combinedMask, auxWeights)` with style-based logic
- [ ] Implement OFFBEAT_8THS style (step % 2 == 1)
- [ ] Implement SYNCOPATED_16THS style (step % 4 in {1, 3})
- [ ] Implement SEED_VARIED style (polyrhythm or inverse, seed-selected)
- [ ] Add unit tests for each AUX style
- [ ] Verify pattern viz shows distinct AUX patterns per zone

### Beat 1 Enforcement
- [ ] Add beat 1 enforcement after pattern generation
- [ ] Implement wild zone probabilistic skip
- [ ] Add unit tests for beat 1 presence
- [ ] Verify beat 1 always present at SHAPE < 0.7

### Minor Fix: Parameterize Accent Positions
From critique-2.md: Accent positions for groove quality metrics are hardcoded for 16-step. Should use `patternLength / 2` for beat 3.

- [ ] Update any hardcoded beat 3 references (step 8) to use `patternLength / 2`

## Acceptance Criteria

- [ ] AXIS Y 0-33%: AUX hits on offbeat 8ths (steps 1, 3, 5, 7, ...)
- [ ] AXIS Y 33-66%: AUX hits on syncopated 16ths
- [ ] AXIS Y 66-100%: AUX hits vary significantly with seed
- [ ] SHAPE 0.0-0.69: Beat 1 always present (100%)
- [ ] SHAPE 0.7: Beat 1 always present (0% skip probability)
- [ ] SHAPE 0.85: Beat 1 present ~80% of time
- [ ] SHAPE 1.0: Beat 1 present ~60% of time
- [ ] AUX patterns show increased variation (target: >50% unique vs current 20%)
- [ ] All existing tests pass
- [ ] No new compiler warnings

## Implementation Notes

### Files to Modify

- `src/Engine/PatternGenerator.h` - Add `AuxStyle` enum, declarations
- `src/Engine/PatternGenerator.cpp` - Update AUX generation, add beat 1 logic
- `tests/test_pattern_generator.cpp` - Add tests

### AUX Style Implementation

```cpp
enum class AuxStyle {
    OFFBEAT_8THS,       // Classic hi-hat: 8th note offbeats
    SYNCOPATED_16THS,   // Funkier: 16th note syncopation
    SEED_VARIED         // Polyrhythmic or inverse, seed-selected
};

inline AuxStyle GetAuxStyle(float axisY) {
    if (axisY < 0.33f) return AuxStyle::OFFBEAT_8THS;
    if (axisY < 0.66f) return AuxStyle::SYNCOPATED_16THS;
    return AuxStyle::SEED_VARIED;
}

void GenerateAuxWeights(const PatternParams& params, uint32_t combinedMask, float* auxWeights) {
    AuxStyle style = GetAuxStyle(params.axisY);
    
    // Seed-based offset only varies within the style
    int auxOffset = static_cast<int>(HashToFloat(params.seed, 201) * 4);
    
    for (int i = 0; i < params.patternLength; ++i) {
        int shifted = (i + auxOffset) % params.patternLength;
        float weight;
        
        switch (style) {
            case AuxStyle::OFFBEAT_8THS:
                // Classic: high weight on odd steps (8th note offbeats)
                weight = (shifted % 2 == 1) ? 0.8f : 0.2f;
                break;
                
            case AuxStyle::SYNCOPATED_16THS:
                // Funkier: emphasis on 16th note anticipations
                weight = ((shifted % 4 == 1) || (shifted % 4 == 3)) ? 0.7f : 0.35f;
                break;
                
            case AuxStyle::SEED_VARIED:
            default:
                // Seed determines sub-style
                int seedStyle = static_cast<int>(HashToFloat(params.seed, 200) * 3);
                if (seedStyle == 0) {
                    // Polyrhythm: every 3rd step
                    weight = (shifted % 3 == 0) ? 0.8f : 0.3f;
                } else if (seedStyle == 1) {
                    // Displaced: unusual positions
                    weight = ((shifted % 4 == 2) || (shifted % 8 == 5)) ? 0.75f : 0.35f;
                } else {
                    // Inverse of metric weight
                    weight = 1.0f - GetMetricWeight(shifted, params.patternLength) * 0.5f;
                }
                break;
        }
        
        auxWeights[i] = ClampWeight(weight);
        
        // Reduce weight where main voices are hitting
        if ((combinedMask & (1U << i)) != 0) {
            auxWeights[i] *= 0.3f;
        }
    }
}
```

### Beat 1 Enforcement Implementation

```cpp
// In GeneratePattern(), after all pattern generation:
void EnforceBeat1(uint32_t& anchorMask, float shape, uint32_t seed) {
    if (shape < 0.70f) {
        // Stable and syncopated zones: beat 1 ALWAYS present
        anchorMask |= (1U << 0);
    } else {
        // Wild zone: probabilistic skip
        // 0% skip at shape=0.7, 40% skip at shape=1.0
        float skipProb = (shape - 0.70f) / 0.30f * 0.40f;
        float roll = HashToFloat(seed, 501);
        if (roll >= skipProb) {
            anchorMask |= (1U << 0);  // Keep beat 1
        }
        // If roll < skipProb, beat 1 is NOT added (skip)
    }
}
```

### Edge Case: Zone Boundaries

From critique-2.md: When AXIS Y is exactly at 0.33 or 0.66, the `<` comparisons mean:
- 0.33 -> SYNCOPATED_16THS
- 0.66 -> SEED_VARIED

This is correct behavior. Consider adding hysteresis if users report flickering.

### Beat 1 Logic Consistency

From critique-2.md: The skip logic uses `>= skipProb` to keep beat 1 (higher hash = keep), which is inverted from typical probability logic. Both patterns work, but document clearly:
- `roll < skipProb` -> skip beat 1
- `roll >= skipProb` -> keep beat 1

## Test Plan

1. Build firmware: `make clean && make`
2. Run tests: `make test`
3. AUX style verification:
   ```bash
   ./build/pattern_viz --axisY=0.1 --seed=123 --format=grid  # Offbeat 8ths
   ./build/pattern_viz --axisY=0.5 --seed=123 --format=grid  # Syncopated
   ./build/pattern_viz --axisY=0.9 --seed=123 --format=grid  # Seed-varied
   ```
4. Beat 1 verification:
   ```bash
   ./build/pattern_viz --shape=0.3 --sweep=seed:1:100 --format=summary
   # All should have beat 1
   
   ./build/pattern_viz --shape=1.0 --sweep=seed:1:100 --format=summary
   # ~60% should have beat 1
   ```

## Estimated Effort

3 hours
