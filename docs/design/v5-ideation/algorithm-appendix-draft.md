# V5 Algorithm Appendix (Draft)

**Date**: 2026-01-04
**Status**: Draft for Review
**Related**: v5-design-final.md

---

## Purpose

This appendix expands the Algorithm Summary from the V5 design document into detailed pseudocode specifications suitable for implementation. Each section includes:

1. **V5 Design Intent** - What the algorithm should accomplish
2. **V4 Current State** - What exists in the codebase today
3. **Delta Analysis** - Differences between V4 and V5
4. **Algorithm Specification** - Detailed pseudocode
5. **Real-Time Constraints** - DaisySP/STM32 safety considerations

---

## 1. SHAPE Blending: Deterministic Score Interpolation

### V5 Design Intent

SHAPE (K2) controls pattern regularity from "stable humanized euclidean" (0%) to "wild weighted" (100%). This is a redesign of the V4 GENRE/Field system into a single continuous parameter.

### V4 Current State

V4 uses a 2D Pattern Field (FIELD X/Y) with 3x3 archetypes per genre:
- `PatternField.cpp`: `GetBlendedArchetype()` uses bilinear interpolation + softmax
- `SoftmaxWithTemperature()` sharpens weights for "winner-take-more" behavior
- Continuous properties (step weights, swing) interpolate; discrete properties (masks) come from dominant archetype

**Key existing code** (`PatternField.cpp:241-264`):
```cpp
void GetBlendedArchetype(const GenreField& field,
                         float fieldX, float fieldY,
                         float temperature,
                         ArchetypeDNA& outArchetype)
{
    float weights[4];
    int gridX0, gridX1, gridY0, gridY1;
    ComputeGridWeights(fieldX, fieldY, weights, gridX0, gridX1, gridY0, gridY1);
    SoftmaxWithTemperature(weights, temperature);
    // ... blend 4 surrounding archetypes
}
```

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| Dimensions | 2D (X syncopation, Y complexity) | 1D (SHAPE regularity) |
| Control | FIELD X/Y knobs | Single SHAPE knob |
| Blending | Bilinear + softmax | Linear interpolation between 2 endpoints |
| Archetypes | 9 per genre | 2 per voice (stable, wild) |

**Key Insight**: V5 simplifies from 2D 9-archetype blend to 1D 2-archetype blend, but the core interpolation mechanism remains similar.

### Algorithm Specification

```
ALGORITHM: ComputeShapeBlendedWeights(shape, energy, patternLength)

  INPUT:
    shape: float [0.0, 1.0] - SHAPE parameter (0=stable, 1=wild)
    energy: float [0.0, 1.0] - ENERGY parameter (affects density)
    patternLength: int [16, 24, 32] - Steps per pattern

  OUTPUT:
    weights[patternLength]: float[] - Per-step hit weights [0.0, 1.0]

  // --- Step 1: Define endpoint archetypes ---

  stableWeights[patternLength] = GenerateEuclideanBase(energy, patternLength)
  wildWeights[patternLength] = GenerateWeightedBase(energy, patternLength)

  // --- Step 2: Linear interpolation ---

  FOR step = 0 TO patternLength - 1:
    weights[step] = (1.0 - shape) * stableWeights[step] + shape * wildWeights[step]

  // --- Step 3: Apply deterministic humanization ---
  // Small per-step variation to avoid mechanical feel

  IF shape < 0.3:  // Only humanize stable patterns
    humanizeAmount = 0.05 * (1.0 - shape / 0.3)
    FOR step = 0 TO patternLength - 1:
      noise = DeterministicNoise(seed, step) * humanizeAmount
      weights[step] = Clamp(weights[step] + noise, 0.0, 1.0)

  RETURN weights
```

```
ALGORITHM: GenerateEuclideanBase(energy, patternLength)

  INPUT:
    energy: float [0.0, 1.0]
    patternLength: int

  OUTPUT:
    weights[patternLength]: float[] - Euclidean-distributed weights

  // Determine hit count from energy
  hitCount = MapEnergyToHitCount(energy, patternLength)

  // Compute Euclidean pattern (evenly distributed)
  // Using Bjorklund's algorithm conceptually
  euclideanMask = ComputeEuclideanPattern(hitCount, patternLength)

  // Convert mask to weights (strong hits on euclidean positions)
  FOR step = 0 TO patternLength - 1:
    IF euclideanMask & (1 << step):
      weights[step] = 0.9 + 0.1 * GetMetricStrength(step, patternLength)
    ELSE:
      weights[step] = 0.1 * GetMetricStrength(step, patternLength)

  RETURN weights
```

```
ALGORITHM: GenerateWeightedBase(energy, patternLength)

  INPUT:
    energy: float [0.0, 1.0]
    patternLength: int

  OUTPUT:
    weights[patternLength]: float[] - Archetype-style weighted pattern

  // Use existing archetype data from ArchetypeData.h
  // Select archetype based on energy (chaos at high, steady at low)

  archetypeIndex = MapEnergyToArchetypeIndex(energy)

  FOR step = 0 TO patternLength - 1:
    weights[step] = GetArchetypeWeight(TECHNO, archetypeIndex, step)

  RETURN weights
```

### Real-Time Constraints

- **No heap allocation**: All weight arrays pre-allocated (max 32 floats = 128 bytes)
- **No exp() in audio path**: Softmax removed from V5 SHAPE (linear blend only)
- **Deterministic**: Same SHAPE + ENERGY + seed = identical output

---

## 2. AXIS Biasing: Additive Offsets with No Dead Zones

### V5 Design Intent

AXIS X (K3) and AXIS Y (K4) provide continuous control over pattern character:
- AXIS X: Beat position (grounded downbeats -> floating offbeats)
- AXIS Y: Intricacy (simple -> complex)

Unlike V4 FIELD X/Y, these are **additive biases** applied to the base pattern, not selection axes into a grid.

### V4 Current State

V4 FIELD X/Y select position in the 3x3 archetype grid:
- X (syncopation): straight -> syncopated -> broken
- Y (complexity): sparse -> medium -> dense

The actual "biasing" happens through archetype selection, not post-processing.

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| Mechanism | Grid position selection | Additive weight modification |
| Center point | Grid center [1,1] | No center - 50% means neutral |
| Dead zone | Grid snapping | None - continuous |
| Effect | Discrete archetype character | Continuous weight shifting |

### Algorithm Specification

```
ALGORITHM: ApplyAxisBias(baseWeights, axisX, axisY, patternLength)

  INPUT:
    baseWeights[patternLength]: float[] - From SHAPE blending
    axisX: float [0.0, 1.0] - X axis (0=downbeats, 1=offbeats)
    axisY: float [0.0, 1.0] - Y axis (0=simple, 1=complex)
    patternLength: int

  OUTPUT:
    weights[patternLength]: float[] - Biased weights

  // --- Step 1: Compute position biases ---
  // Center at 0.5 means no bias

  xBias = (axisX - 0.5) * 2.0  // Range: [-1.0, 1.0]
  yBias = (axisY - 0.5) * 2.0  // Range: [-1.0, 1.0]

  // --- Step 2: Apply X bias (beat position) ---
  // Positive xBias: boost offbeats, attenuate downbeats
  // Negative xBias: boost downbeats, attenuate offbeats

  FOR step = 0 TO patternLength - 1:
    positionStrength = GetPositionStrength(step, patternLength)
    // positionStrength: -1.0 for downbeats, +1.0 for offbeats

    xBoost = xBias * positionStrength * 0.4  // Max +/- 40% shift
    weights[step] = Clamp(baseWeights[step] + xBoost, 0.0, 1.0)

  // --- Step 3: Apply Y bias (intricacy) ---
  // Positive yBias: boost weak positions, add complexity
  // Negative yBias: suppress weak positions, simplify

  FOR step = 0 TO patternLength - 1:
    isWeakPosition = GetMetricStrength(step, patternLength) < 0.5

    IF isWeakPosition:
      yBoost = yBias * 0.3  // Max +/- 30% shift for weak positions
      weights[step] = Clamp(weights[step] + yBoost, 0.0, 1.0)
    ELSE IF yBias < 0:
      // Negative Y also strengthens strong positions
      yBoost = -yBias * 0.2  // Max +20% boost for strong positions
      weights[step] = Clamp(weights[step] + yBoost, 0.0, 1.0)

  RETURN weights
```

```
ALGORITHM: GetPositionStrength(step, patternLength)

  // Returns value from -1.0 (strong downbeat) to +1.0 (weak offbeat)
  // Used for X-axis biasing

  // Map step to 32-step normalized grid
  normalizedStep = step * 32 / patternLength

  IF normalizedStep == 0:
    RETURN -1.0   // Beat 1 - strongest downbeat
  ELSE IF normalizedStep == 16:
    RETURN -0.8   // Half bar
  ELSE IF normalizedStep % 8 == 0:
    RETURN -0.6   // Quarter notes
  ELSE IF normalizedStep % 4 == 0:
    RETURN -0.3   // 8th notes
  ELSE IF normalizedStep % 2 == 0:
    RETURN 0.3    // 16th notes (on-beat)
  ELSE:
    RETURN 1.0    // Off-beat 16ths
```

### Real-Time Constraints

- **No grid lookup**: Direct computation, O(patternLength)
- **Continuous response**: No quantization or snapping
- **CV-friendly**: Direct 0-1 input, linear response

---

## 3. Voice Relationship: COMPLEMENT + DRIFT Variation

### V5 Design Intent

V5 simplifies voice coupling to COMPLEMENT-only (shimmer fills anchor gaps) with DRIFT controlling per-phrase variation. This eliminates the INTERLOCK and SHADOW modes from V4.

### V4 Current State

V4 `VoiceRelation.cpp` provides three modes:
- INDEPENDENT: Voices fire freely, can overlap
- INTERLOCK: Remove shimmer hits that coincide with anchor (broken, removed in Task 22)
- SHADOW: Shimmer echoes anchor with 1-step delay

**Key existing code** (`VoiceRelation.cpp:131-172`):
```cpp
void ApplyVoiceRelationship(uint32_t anchorMask,
                            uint32_t& shimmerMask,
                            VoiceCoupling coupling,
                            int patternLength)
{
    uint32_t originalShimmer = shimmerMask;
    switch (coupling) {
        case VoiceCoupling::INDEPENDENT: break;
        case VoiceCoupling::INTERLOCK:
            ApplyInterlock(anchorMask, shimmerMask, patternLength);
            ApplyGapFill(anchorMask, shimmerMask, originalShimmer, 8, patternLength);
            break;
        case VoiceCoupling::SHADOW:
            ApplyShadow(anchorMask, shimmerMask, patternLength);
            break;
    }
}
```

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| Modes | 3 (INDEPENDENT, INTERLOCK, SHADOW) | 1 (COMPLEMENT) |
| Control | Config knob | Fixed behavior |
| Variation | DRIFT affects seed | DRIFT affects gap-seeking |
| Gap handling | Optional gap-fill | Core mechanism |

### Algorithm Specification

```
ALGORITHM: ApplyComplementRelationship(anchorMask, shimmerMask, drift, seed, patternLength)

  INPUT:
    anchorMask: uint32 - Anchor hit bitmask
    shimmerMask: uint32 - Initial shimmer hit bitmask (from weights)
    drift: float [0.0, 1.0] - DRIFT parameter
    seed: uint32 - Pattern seed for determinism
    patternLength: int

  OUTPUT:
    shimmerMask: uint32 - Modified shimmer mask (complement of anchor)

  // --- Step 1: Find gaps in anchor pattern ---

  gaps[] = FindGaps(anchorMask, patternLength)
  // gaps: array of {start, length} for each gap

  // --- Step 2: Determine shimmer hit count ---

  targetShimmerHits = CountBits(shimmerMask)  // Preserve density intent

  // --- Step 3: Distribute shimmer into gaps ---

  newShimmerMask = 0
  hitsPlaced = 0

  // Sort gaps by length (larger gaps get more hits)
  SortGapsByLength(gaps)

  FOR EACH gap IN gaps:
    // Hits per gap proportional to gap length
    gapShare = gap.length * targetShimmerHits / TotalGapLength(gaps)
    gapShare = Max(1, Round(gapShare))  // At least 1 hit per significant gap

    // Place hits within gap
    FOR i = 0 TO gapShare - 1:
      IF hitsPlaced >= targetShimmerHits:
        BREAK

      // Determine position within gap based on DRIFT
      IF drift < 0.3:
        // Low drift: deterministic center placement
        position = gap.start + (gap.length * (i + 1)) / (gapShare + 1)
      ELSE:
        // High drift: seed-varied placement
        noise = HashToFloat(seed, gap.start + i)
        position = gap.start + Floor(noise * gap.length)

      newShimmerMask |= (1 << position)
      hitsPlaced++

  // --- Step 4: Apply DRIFT variation ---
  // At high DRIFT, some hits may shift by 1 step

  IF drift > 0.5:
    shiftProbability = (drift - 0.5) * 0.6  // Max 30% chance at DRIFT=1
    FOR step = 0 TO patternLength - 1:
      IF newShimmerMask & (1 << step):
        IF HashToFloat(seed ^ 0xDEAD, step) < shiftProbability:
          direction = (HashToFloat(seed ^ 0xBEEF, step) > 0.5) ? 1 : -1
          newStep = (step + direction + patternLength) % patternLength
          // Only shift if destination is empty
          IF NOT (anchorMask & (1 << newStep)) AND NOT (newShimmerMask & (1 << newStep)):
            newShimmerMask &= ~(1 << step)
            newShimmerMask |= (1 << newStep)

  RETURN newShimmerMask
```

### Real-Time Constraints

- **Max 32 gaps**: Fixed-size gap array (stack allocated)
- **O(n) complexity**: Single pass for gap finding, single pass for placement
- **No recursion**: Iterative gap-fill algorithm

---

## 4. Hat Burst: Pattern-Aware, Pre-Allocated Arrays

### V5 Design Intent

HAT mode (AUX output secret "2.5 pulse") produces pattern-aware hat bursts during fill:
- Burst density follows ENERGY
- Burst regularity follows SHAPE
- Pre-allocated (max 12 triggers)
- Velocity reduced 70% near main pattern hits

### V4 Current State

V4 `AuxOutput.cpp` provides HAT mode as a third trigger voice following the aux hit mask. There is no special "burst" behavior during fills - it simply fires on the aux mask.

**Key insight**: V5 HAT burst is a new feature that triggers specially during fills.

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| HAT output | Follows aux mask always | Burst during fill, normal otherwise |
| Fill behavior | No special handling | Burst density/regularity from ENERGY/SHAPE |
| Velocity | Fixed | Reduced near main hits |
| Memory | Dynamic mask | Pre-allocated 12-trigger array |

### Algorithm Specification

```
ALGORITHM: GenerateHatBurst(energy, shape, anchorMask, shimmerMask, fillDuration, seed)

  INPUT:
    energy: float [0.0, 1.0] - Controls burst density
    shape: float [0.0, 1.0] - Controls burst regularity
    anchorMask: uint32 - Anchor pattern (for velocity ducking)
    shimmerMask: uint32 - Shimmer pattern (for velocity ducking)
    fillDuration: int - Fill length in steps (typically 4-8)
    seed: uint32 - Deterministic seed

  OUTPUT:
    burst: HatBurst struct {
      triggers[12]: {step: int, velocity: float}
      count: int
    }

  // --- Step 1: Determine trigger count from ENERGY ---

  minTriggers = 2
  maxTriggers = 12
  triggerCount = Round(minTriggers + energy * (maxTriggers - minTriggers))

  // --- Step 2: Generate timing based on SHAPE ---

  burst.count = 0
  mainHits = anchorMask | shimmerMask

  IF shape < 0.3:
    // Low SHAPE: Even euclidean distribution
    spacing = fillDuration / triggerCount
    FOR i = 0 TO triggerCount - 1:
      step = Round(i * spacing)
      IF burst.count < 12:
        burst.triggers[burst.count].step = step
        burst.count++

  ELSE IF shape < 0.7:
    // Mid SHAPE: Euclidean with jitter
    spacing = fillDuration / triggerCount
    FOR i = 0 TO triggerCount - 1:
      step = Round(i * spacing)
      jitter = (HashToFloat(seed, i) - 0.5) * 2  // [-1, 1]
      step = Clamp(step + Round(jitter * shape), 0, fillDuration - 1)
      IF burst.count < 12:
        burst.triggers[burst.count].step = step
        burst.count++

  ELSE:
    // High SHAPE: Weighted random placement
    FOR i = 0 TO triggerCount - 1:
      step = Floor(HashToFloat(seed, i) * fillDuration)
      IF burst.count < 12:
        burst.triggers[burst.count].step = step
        burst.count++

  // --- Step 3: Compute velocities with ducking ---

  FOR i = 0 TO burst.count - 1:
    step = burst.triggers[i].step
    baseVelocity = 0.7 + 0.3 * energy  // 70-100% base

    // Check proximity to main pattern hits
    nearMainHit = FALSE
    FOR offset = -1 TO 1:
      checkStep = (step + offset + 32) % 32
      IF mainHits & (1 << checkStep):
        nearMainHit = TRUE
        BREAK

    IF nearMainHit:
      burst.triggers[i].velocity = baseVelocity * 0.3  // 70% reduction
    ELSE:
      burst.triggers[i].velocity = baseVelocity

  RETURN burst
```

### Data Structure

```cpp
// Pre-allocated, stack-safe structure
struct HatBurst {
    struct Trigger {
        uint8_t step;      // Step index within fill
        float velocity;    // 0.0-1.0
    };

    Trigger triggers[12];  // Max 12 triggers, always allocated
    uint8_t count;         // Actual trigger count (0-12)
    uint8_t fillStart;     // Fill start step in pattern
};
```

### Real-Time Constraints

- **Fixed 12-trigger limit**: 12 * 5 bytes = 60 bytes stack
- **No dynamic allocation**: Array always full size
- **O(n) complexity**: Single pass generation, single pass velocity calculation
- **No floating-point division in hot path**: Pre-computed spacing

---

## 5. Fill: Multi-Attribute Boost + AUX Output

### V5 Design Intent

Fill (button tap) applies multi-attribute boost during the fill zone:
- Density boost across all voices
- Velocity boost (louder hits)
- Opens eligibility (more positions can fire)
- AUX output goes HIGH (FILL GATE mode)

### V4 Current State

V4 `HitBudget.cpp` has `ApplyFillBoost()` that modifies budgets:
- Multiplies hit counts by `fillMultiplier` (archetype-specific, 1.15x-2.1x)
- Opens eligibility masks during intense fills

V4 `VelocityCompute.cpp` has BUILD phase modifiers:
- FILL phase (87.5-100% of phrase) gets +12-20% velocity boost
- Force accents when BUILD > 60%

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| Trigger | Phrase position (automatic) | Button tap (manual) |
| Duration | Fixed 12.5% of phrase | Configurable (default 4 steps) |
| Density | Archetype multiplier | Fixed 1.5x all voices |
| Velocity | BUILD-dependent | Fixed +20% |
| AUX | No special handling | FILL GATE output |

### Algorithm Specification

```
ALGORITHM: ComputeFillModifiers(fillActive, fillProgress, energy, voiceBudgets)

  INPUT:
    fillActive: bool - Whether fill is currently active
    fillProgress: float [0.0, 1.0] - Progress through fill (0=start, 1=end)
    energy: float [0.0, 1.0] - ENERGY parameter
    voiceBudgets: {anchor, shimmer, aux} - Base hit budgets

  OUTPUT:
    modifiers: FillModifiers struct

  IF NOT fillActive:
    RETURN DefaultModifiers()  // No boost

  // --- Density boost ---
  // Linear ramp from 1.0 to 1.5 over fill duration
  densityMultiplier = 1.0 + 0.5 * fillProgress

  modifiers.anchorBudget = Round(voiceBudgets.anchor * densityMultiplier)
  modifiers.shimmerBudget = Round(voiceBudgets.shimmer * densityMultiplier)
  modifiers.auxBudget = Round(voiceBudgets.aux * densityMultiplier)

  // --- Velocity boost ---
  // Immediate +15%, ramping to +25% by fill end
  modifiers.velocityBoost = 0.15 + 0.10 * fillProgress

  // --- Eligibility expansion ---
  // Open up 16th note positions for all voices
  modifiers.expandEligibility = TRUE
  modifiers.eligibilityMask = 0xFFFFFFFF  // All positions

  // --- Force accents ---
  // All hits accented during fill peak (last 25%)
  modifiers.forceAccents = (fillProgress > 0.75)

  // --- AUX gate ---
  modifiers.auxGateHigh = TRUE

  RETURN modifiers
```

### Real-Time Constraints

- **No allocation**: Modifier struct on stack
- **O(1) computation**: Direct formulas, no iteration
- **Button debouncing**: Handled in control layer, not here

---

## 6. ACCENT: Musical Weight to Velocity Mapping

### V5 Design Intent

ACCENT (K4 in Config mode) controls velocity depth from flat (all hits equal) to dynamic (ghost notes to accents). This replaces V4's PUNCH parameter with a simpler mental model.

### V4 Current State

V4 `VelocityCompute.cpp` uses PUNCH to control:
- `accentProbability`: 20% to 50%
- `velocityFloor`: 65% down to 30%
- `accentBoost`: +15% to +45%
- `velocityVariation`: +/-3% to +/-15%

**Key existing code** (`VelocityCompute.cpp:25-46`):
```cpp
void ComputePunch(float punch, PunchParams& params)
{
    punch = Clamp(punch, 0.0f, 1.0f);
    params.accentProbability = 0.20f + punch * 0.30f;
    params.velocityFloor = 0.65f - punch * 0.35f;
    params.accentBoost = 0.15f + punch * 0.30f;
    params.velocityVariation = 0.03f + punch * 0.12f;
}
```

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| Name | PUNCH | ACCENT |
| Location | Performance shift | Config |
| Floor range | 65%-30% | 80%-30% |
| Boost range | +15% to +45% | +10% to +50% |
| Probability | 20%-50% | Based on metric position |
| CV modulation | Yes (shift parameter) | No (config) |

**Key Change**: V5 ACCENT uses musical weight (metric position) to determine accent strength, rather than probability.

### Algorithm Specification

```
ALGORITHM: ComputeAccentVelocity(accent, step, isHit, patternLength, seed)

  INPUT:
    accent: float [0.0, 1.0] - ACCENT parameter (0=flat, 1=dynamic)
    step: int - Step index
    isHit: bool - Whether this step fires
    patternLength: int
    seed: uint32 - For deterministic variation

  OUTPUT:
    velocity: float [0.3, 1.0] - Output velocity

  IF NOT isHit:
    RETURN 0.0

  // --- Step 1: Get metric weight for this step ---
  // Strong positions (downbeats) = high weight
  // Weak positions (offbeats) = low weight

  metricWeight = GetMetricWeight(step, patternLength)
  // Returns 0.0-1.0, where 1.0 = beat 1, 0.0 = weak 16th

  // --- Step 2: Compute velocity range from ACCENT ---

  // Low ACCENT = narrow range (flat dynamics)
  // High ACCENT = wide range (ghost notes to accents)

  velocityFloor = 0.80 - accent * 0.50   // 80% to 30%
  velocityCeiling = 0.90 + accent * 0.10 // 90% to 100%

  // --- Step 3: Map metric weight to velocity ---

  velocityRange = velocityCeiling - velocityFloor
  baseVelocity = velocityFloor + metricWeight * velocityRange

  // --- Step 4: Add micro-variation ---
  // Small random variation for human feel

  variationAmount = 0.02 + accent * 0.06  // 2% to 8%
  noise = (HashToFloat(seed, step) - 0.5) * 2.0  // [-1, 1]
  velocity = baseVelocity + noise * variationAmount

  // --- Step 5: Clamp to valid range ---

  RETURN Clamp(velocity, 0.30, 1.0)
```

```
ALGORITHM: GetMetricWeight(step, patternLength)

  // Returns metric strength for velocity mapping
  // 1.0 = strongest (beat 1)
  // 0.0 = weakest (off-beat 16th)

  normalizedStep = step * 32 / patternLength

  IF normalizedStep == 0:
    RETURN 1.0    // Beat 1 of bar 1
  ELSE IF normalizedStep == 16:
    RETURN 0.95   // Beat 1 of bar 2
  ELSE IF normalizedStep % 8 == 0:
    RETURN 0.85   // Quarter notes
  ELSE IF normalizedStep % 4 == 0:
    RETURN 0.70   // 8th notes
  ELSE IF normalizedStep % 2 == 0:
    RETURN 0.45   // 16th notes (on-beat)
  ELSE:
    RETURN 0.20   // Off-beat 16ths (ghost notes)
```

### Real-Time Constraints

- **O(1) per step**: Direct lookup/computation
- **No conditionals in hot path**: Table-based metric weight
- **Stack-only**: No allocation

---

## Summary: V4 to V5 Migration Path

| V5 Algorithm | Existing V4 Code | Reuse Strategy |
|--------------|------------------|----------------|
| SHAPE blending | `PatternField.cpp` | Simplify to 1D interpolation |
| AXIS biasing | None | New implementation |
| Voice COMPLEMENT | `VoiceRelation.cpp` | Refactor INTERLOCK to gap-based |
| Hat burst | `AuxOutput.cpp` | New burst generation |
| Fill | `HitBudget.cpp`, `VelocityCompute.cpp` | Combine and simplify |
| ACCENT | `VelocityCompute.cpp` | Replace probability with metric weight |

### Implementation Order Recommendation

1. **ACCENT** (low risk, isolated, tests existing velocity infra)
2. **SHAPE blending** (refactor existing, well-tested)
3. **AXIS biasing** (new, but simple additive)
4. **Voice COMPLEMENT** (moderate refactor)
5. **Fill modifiers** (combine existing)
6. **Hat burst** (new feature, can be last)

---

## Real-Time Safety Checklist

All algorithms must pass the `daisysp-review` criteria:

- [ ] No `new`, `delete`, `malloc`, `free` in any algorithm
- [ ] No `std::vector`, `std::string`, or other heap-allocating containers
- [ ] All arrays are fixed-size with compile-time bounds
- [ ] No blocking operations (I/O, locks, sleep)
- [ ] No `exp()`, `log()`, `sin()`, `cos()` in per-step code (pre-compute or approximate)
- [ ] Worst-case execution time < 20us at 48kHz
- [ ] All inputs validated (Clamp before use)
- [ ] No division by zero possible

---

*Draft generated for design review. Pending feedback from design-critic and code-reviewer.*
