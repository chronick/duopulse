# V5 Algorithm Appendix - Final Draft

**Date**: 2026-01-04
**Status**: Final Draft for Review
**Incorporates**: UX review feedback, syncopation adjustments, preset map

---

## Purpose

This appendix expands the Algorithm Summary from the V5 design document into detailed pseudocode specifications suitable for implementation. Each section includes:

1. **V5 Design Intent** - What the algorithm should accomplish
2. **V4 Current State** - What exists in the codebase today
3. **Delta Analysis** - Differences between V4 and V5
4. **Algorithm Specification** - Detailed pseudocode
5. **Real-Time Constraints** - DaisySP/STM32 safety considerations

---

## 1. SHAPE Blending: Three-Way Interpolation with Syncopation Peak

### V5 Design Intent

SHAPE (K2) controls pattern regularity through THREE character zones:
- **0-30%**: Stable humanized euclidean (techno, four-on-floor)
- **30-70%**: Syncopation peak (funk, displaced, tension)
- **70-100%**: Wild weighted (IDM, chaos)

This addresses the "syncopation gap" where linear blending between stable and wild skips over musically interesting syncopated territory.

### V4 Current State

V4 uses a 2D Pattern Field (FIELD X/Y) with 3x3 archetypes per genre:
- `PatternField.cpp`: `GetBlendedArchetype()` uses bilinear interpolation + softmax
- Syncopation was explicit on FIELD X axis
- Archetypes included "Displaced" and "Broken" patterns

**Key existing code** (`PatternField.cpp`):
```cpp
void GetBlendedArchetype(const GenreField& field,
                         float fieldX, float fieldY,
                         float temperature,
                         ArchetypeDNA& outArchetype)
```

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| Dimensions | 2D (X syncopation, Y complexity) | 1D SHAPE + 2D AXIS modifiers |
| Syncopation | Explicit axis | Peak zone at SHAPE 50% |
| Blending | Bilinear + softmax | Three-way linear interpolation |
| Archetypes | 9 per genre | 3 conceptual: stable, syncopated, wild |

### Algorithm Specification

```
ALGORITHM: ComputeShapeBlendedWeights(shape, energy, seed, patternLength)

INPUT:
  shape: float [0.0, 1.0] - SHAPE parameter
  energy: float [0.0, 1.0] - ENERGY parameter (affects density)
  seed: uint32 - Deterministic seed
  patternLength: int [16, 24, 32] - Steps per pattern

OUTPUT:
  weights[patternLength]: float[] - Per-step hit weights [0.0, 1.0]

// --- Step 1: Generate three base archetypes ---

stableWeights[patternLength] = GenerateStablePattern(energy, patternLength)
syncopatedWeights[patternLength] = GenerateSyncopationPattern(energy, seed, patternLength)
wildWeights[patternLength] = GenerateWildPattern(energy, seed, patternLength)

// --- Step 2: Three-way interpolation based on SHAPE zones ---

FOR step = 0 TO patternLength - 1:
  IF shape < 0.30:
    // Zone 1: Pure stable with humanization fade-in
    t = shape / 0.30  // 0 to 1 within zone
    weights[step] = stableWeights[step]
    // Humanization decreases as we approach syncopation
    humanize = 0.05 * (1.0 - t)
    noise = (HashToFloat(seed, step) - 0.5) * 2.0 * humanize
    weights[step] = Clamp(weights[step] + noise, 0.0, 1.0)

  ELSE IF shape < 0.50:
    // Zone 2a: Blend stable -> syncopated
    t = (shape - 0.30) / 0.20  // 0 to 1
    weights[step] = Lerp(stableWeights[step], syncopatedWeights[step], t)

  ELSE IF shape < 0.70:
    // Zone 2b: Blend syncopated -> wild
    t = (shape - 0.50) / 0.20  // 0 to 1
    weights[step] = Lerp(syncopatedWeights[step], wildWeights[step], t)

  ELSE:
    // Zone 3: Pure wild with increasing chaos
    t = (shape - 0.70) / 0.30  // 0 to 1 within zone
    weights[step] = wildWeights[step]
    // Add extra chaos at extreme values
    chaos = t * 0.15
    noise = (HashToFloat(seed ^ 0xCAFE, step) - 0.5) * 2.0 * chaos
    weights[step] = Clamp(weights[step] + noise, 0.0, 1.0)

RETURN weights
```

```
ALGORITHM: GenerateStablePattern(energy, patternLength)

INPUT:
  energy: float [0.0, 1.0]
  patternLength: int

OUTPUT:
  weights[patternLength]: float[] - Euclidean-distributed weights

// Determine hit count from energy (2 to patternLength/2)
minHits = 2
maxHits = patternLength / 2
hitCount = Round(minHits + energy * (maxHits - minHits))

// Compute Euclidean pattern using Bjorklund's algorithm
euclideanMask = ComputeEuclideanPattern(hitCount, patternLength)

// Convert to weights with metric strength influence
FOR step = 0 TO patternLength - 1:
  metricStrength = GetMetricWeight(step, patternLength)

  IF euclideanMask & (1 << step):
    // Euclidean hit: high weight, boosted by metric strength
    weights[step] = 0.85 + 0.15 * metricStrength
  ELSE:
    // Non-hit: low weight, still influenced by metric
    weights[step] = 0.10 + 0.10 * metricStrength

RETURN weights
```

```
ALGORITHM: GenerateSyncopationPattern(energy, seed, patternLength)

INPUT:
  energy: float [0.0, 1.0]
  seed: uint32
  patternLength: int

OUTPUT:
  weights[patternLength]: float[] - Syncopation-optimized weights

// Syncopation creates tension through:
// 1. Suppressed downbeats (not eliminated)
// 2. Boosted anticipation positions (step before strong beat)
// 3. Strong offbeats
// 4. Occasional gaps for "breath"

FOR step = 0 TO patternLength - 1:
  metricStrength = GetMetricWeight(step, patternLength)
  normalizedStep = step * 32 / patternLength

  // Identify anticipation positions (step before downbeat)
  isAnticipation = ((normalizedStep + 1) % 8 == 0) OR
                   ((normalizedStep + 1) % 16 == 0)

  IF metricStrength > 0.9:
    // Beat 1 - SUPPRESS but don't eliminate
    weights[step] = 0.35 + energy * 0.15  // 35-50%

  ELSE IF metricStrength > 0.7:
    // Other strong beats (quarters) - moderate suppression
    weights[step] = 0.45 + energy * 0.15  // 45-60%

  ELSE IF isAnticipation:
    // Anticipation positions - BOOST strongly
    weights[step] = 0.80 + energy * 0.15  // 80-95%

  ELSE IF metricStrength < 0.3:
    // Weak offbeats - BOOST
    weights[step] = 0.70 + energy * 0.20  // 70-90%

  ELSE:
    // Middle positions - moderate
    weights[step] = 0.50 + energy * 0.20  // 50-70%

  // Add slight per-step variation for groove
  noise = (HashToFloat(seed, step) - 0.5) * 0.10
  weights[step] = Clamp(weights[step] + noise, 0.15, 0.95)

RETURN weights
```

```
ALGORITHM: GenerateWildPattern(energy, seed, patternLength)

INPUT:
  energy: float [0.0, 1.0]
  seed: uint32
  patternLength: int

OUTPUT:
  weights[patternLength]: float[] - Chaotic weighted pattern

// Wild patterns use seed-based pseudo-random weights
// with energy controlling the density bias

FOR step = 0 TO patternLength - 1:
  // Base random weight
  randomWeight = HashToFloat(seed, step)

  // Energy biases toward higher weights (more hits)
  energyBias = energy * 0.4  // 0 to 0.4 bias
  weights[step] = Clamp(randomWeight + energyBias, 0.1, 0.95)

  // Occasional strong accent (1 in 4 steps on average)
  IF HashToFloat(seed ^ 0xBEEF, step) < 0.25:
    weights[step] = Min(weights[step] + 0.3, 0.98)

RETURN weights
```

### Real-Time Constraints

- **Pre-allocated arrays**: 3 x 32 floats = 384 bytes max (stack safe)
- **No exp()/log()**: Linear interpolation only
- **O(n) complexity**: Single pass per archetype, single blend pass
- **Deterministic**: Same inputs = identical output

---

## 2. AXIS Biasing: Bidirectional Offsets with Downbeat Suppression

### V5 Design Intent

AXIS X (K3) and AXIS Y (K4) provide continuous control with FULL RANGE effect:
- **AXIS X**: Beat position with bidirectional action
  - Low (0%): Boost downbeats, suppress offbeats
  - Mid (50%): Neutral
  - High (100%): Suppress downbeats, boost offbeats
- **AXIS Y**: Intricacy (simple to complex)
  - Increased range for audible effect

**Key improvement**: AXIS X now SUPPRESSES as well as boosts, enabling true "broken" patterns.

### V4 Current State

V4 FIELD X/Y were grid selectors, not modifiers:
- Position in 3x3 grid determined archetype blend
- No post-processing bias system

### Delta Analysis

| Aspect | V4 | V5 |
|--------|----|----|
| Mechanism | Grid selection | Additive/subtractive bias |
| X behavior | Select syncopation level | Bidirectional beat shift |
| Y behavior | Select complexity level | Intricacy modification |
| Max effect | Discrete archetype | +/-60% X, +/-50% Y |

### Algorithm Specification

```
ALGORITHM: ApplyAxisBias(baseWeights, axisX, axisY, shape, seed, patternLength)

INPUT:
  baseWeights[patternLength]: float[] - From SHAPE blending
  axisX: float [0.0, 1.0] - X axis (0=downbeats, 1=offbeats)
  axisY: float [0.0, 1.0] - Y axis (0=simple, 1=complex)
  shape: float [0.0, 1.0] - For "broken" mode detection
  seed: uint32 - For broken mode randomization
  patternLength: int

OUTPUT:
  weights[patternLength]: float[] - Biased weights

// --- Step 1: Compute bipolar biases from 0-1 range ---
// 0.5 = neutral (no bias)

xBias = (axisX - 0.5) * 2.0  // Range: [-1.0, 1.0]
yBias = (axisY - 0.5) * 2.0  // Range: [-1.0, 1.0]

// --- Step 2: Apply X bias (bidirectional beat position) ---

FOR step = 0 TO patternLength - 1:
  positionStrength = GetPositionStrength(step, patternLength)
  // positionStrength: -1.0 for downbeats, +1.0 for offbeats

  // Calculate bidirectional effect
  IF xBias > 0:
    // Moving toward offbeats: suppress downbeats, boost offbeats
    IF positionStrength < 0:
      // Downbeat: SUPPRESS
      xEffect = -xBias * Abs(positionStrength) * 0.45
    ELSE:
      // Offbeat: BOOST
      xEffect = xBias * positionStrength * 0.60

  ELSE:
    // Moving toward downbeats: boost downbeats, suppress offbeats
    IF positionStrength < 0:
      // Downbeat: BOOST
      xEffect = -xBias * Abs(positionStrength) * 0.60
    ELSE:
      // Offbeat: SUPPRESS
      xEffect = xBias * positionStrength * 0.45

  weights[step] = Clamp(baseWeights[step] + xEffect, 0.05, 1.0)

// --- Step 3: Apply Y bias (intricacy) ---

FOR step = 0 TO patternLength - 1:
  metricStrength = GetMetricWeight(step, patternLength)
  isWeakPosition = metricStrength < 0.5

  IF yBias > 0:
    // Positive Y: add complexity (boost weak, moderate strong)
    IF isWeakPosition:
      yEffect = yBias * 0.50  // Strong boost to weak positions
    ELSE:
      yEffect = yBias * 0.15  // Slight boost to strong (more hits overall)

  ELSE:
    // Negative Y: simplify (suppress weak, boost strong)
    IF isWeakPosition:
      yEffect = yBias * 0.50  // Suppress weak positions
    ELSE:
      yEffect = -yBias * 0.25  // Boost strong positions

  weights[step] = Clamp(weights[step] + yEffect, 0.05, 1.0)

// --- Step 4: Apply "Broken" mode at high SHAPE + high AXIS X ---

IF shape > 0.6 AND axisX > 0.7:
  brokenIntensity = (shape - 0.6) * 2.5 * (axisX - 0.7) * 3.33
  // brokenIntensity: 0 to ~1.0 at shape=1.0, axisX=1.0

  // Target downbeat positions for potential heavy suppression
  downbeatSteps = [0, 8, 16, 24]  // For 32-step pattern, scale for others

  FOR step IN downbeatSteps:
    scaledStep = step * patternLength / 32
    IF scaledStep < patternLength:
      IF HashToFloat(seed ^ 0xDEAD, scaledStep) < brokenIntensity * 0.6:
        // Heavy suppression (but not elimination)
        weights[scaledStep] *= 0.25

RETURN weights
```

```
ALGORITHM: GetPositionStrength(step, patternLength)

// Returns value from -1.0 (strong downbeat) to +1.0 (weak offbeat)
// Used for bidirectional X-axis biasing

// Map step to 32-step normalized grid
normalizedStep = step * 32 / patternLength

IF normalizedStep == 0:
  RETURN -1.0   // Beat 1 bar 1 - strongest downbeat
ELSE IF normalizedStep == 16:
  RETURN -0.85  // Beat 1 bar 2
ELSE IF normalizedStep % 8 == 0:
  RETURN -0.65  // Quarter notes
ELSE IF normalizedStep % 4 == 0:
  RETURN -0.30  // 8th notes (still "on" beat)
ELSE IF normalizedStep % 2 == 0:
  RETURN 0.40   // 16th notes (on-beat but weak)
ELSE:
  RETURN 1.0    // Off-beat 16ths - weakest
```

```
ALGORITHM: GetMetricWeight(step, patternLength)

// Returns metric strength for velocity and complexity calculations
// 1.0 = strongest (beat 1), 0.0 = weakest (off-beat 16th)

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

- **O(n) complexity**: Two passes over pattern
- **No branching in inner loop**: Position strength is table lookup
- **Floor of 0.05**: Prevents complete note elimination
- **Deterministic broken mode**: Uses seeded hash

---

## 3. Voice Relationship: COMPLEMENT with DRIFT Variation

### V5 Design Intent

V5 simplifies to COMPLEMENT-only behavior:
- Shimmer fills gaps in anchor pattern
- DRIFT controls placement variation within gaps
- No user-selectable modes (INDEPENDENT, SHADOW removed)

### V4 Current State

V4 `VoiceRelation.cpp` provides three modes with gap-fill logic for INTERLOCK.

### Algorithm Specification

```
ALGORITHM: ApplyComplementRelationship(anchorMask, shimmerWeights, drift, seed, patternLength)

INPUT:
  anchorMask: uint32 - Anchor hit bitmask (already determined)
  shimmerWeights[patternLength]: float[] - Shimmer weights from SHAPE+AXIS
  drift: float [0.0, 1.0] - DRIFT parameter
  seed: uint32 - Pattern seed
  patternLength: int

OUTPUT:
  shimmerMask: uint32 - Shimmer hits placed in anchor gaps

// --- Step 1: Determine target shimmer hit count ---

targetHits = 0
FOR step = 0 TO patternLength - 1:
  IF shimmerWeights[step] > 0.5:
    targetHits++
targetHits = Max(1, Min(targetHits, patternLength / 2))

// --- Step 2: Find gaps in anchor pattern ---

gaps[MAX_GAPS]: struct { start: int, length: int }
gapCount = 0
inGap = FALSE
gapStart = 0

FOR step = 0 TO patternLength - 1:
  isAnchorHit = (anchorMask & (1 << step)) != 0

  IF NOT isAnchorHit AND NOT inGap:
    // Start new gap
    inGap = TRUE
    gapStart = step
  ELSE IF isAnchorHit AND inGap:
    // End gap
    IF gapCount < MAX_GAPS:
      gaps[gapCount].start = gapStart
      gaps[gapCount].length = step - gapStart
      gapCount++
    inGap = FALSE

// Handle wrap-around gap
IF inGap:
  // Gap extends to end of pattern
  IF gapCount < MAX_GAPS:
    gaps[gapCount].start = gapStart
    gaps[gapCount].length = patternLength - gapStart
    gapCount++

// --- Step 3: Calculate total gap length for proportional distribution ---

totalGapLength = 0
FOR i = 0 TO gapCount - 1:
  totalGapLength += gaps[i].length

IF totalGapLength == 0:
  RETURN 0  // No gaps, no shimmer

// --- Step 4: Distribute shimmer hits into gaps ---

shimmerMask = 0
hitsPlaced = 0

FOR i = 0 TO gapCount - 1:
  // Proportional hits for this gap
  gapShare = Round(gaps[i].length * targetHits / totalGapLength)
  gapShare = Max(1, gapShare)  // At least 1 per gap

  FOR j = 0 TO gapShare - 1:
    IF hitsPlaced >= targetHits:
      BREAK

    // Determine position within gap based on DRIFT
    IF drift < 0.3:
      // Low drift: evenly spaced within gap
      position = gaps[i].start + ((gaps[i].length * (j + 1)) / (gapShare + 1))
    ELSE IF drift < 0.7:
      // Mid drift: weighted by shimmer weights
      bestPos = gaps[i].start
      bestWeight = 0
      FOR k = 0 TO gaps[i].length - 1:
        pos = gaps[i].start + k
        IF shimmerWeights[pos] > bestWeight:
          IF (shimmerMask & (1 << pos)) == 0:  // Not already placed
            bestWeight = shimmerWeights[pos]
            bestPos = pos
      position = bestPos
    ELSE:
      // High drift: seed-varied placement
      noise = HashToFloat(seed ^ (i * 17), j)
      position = gaps[i].start + Floor(noise * gaps[i].length)

    // Ensure position is valid and not already used
    position = position % patternLength
    IF (shimmerMask & (1 << position)) == 0:
      shimmerMask |= (1 << position)
      hitsPlaced++

// --- Step 5: Apply DRIFT micro-shifts ---

IF drift > 0.5:
  shiftProbability = (drift - 0.5) * 0.5  // Max 25% at DRIFT=1

  FOR step = 0 TO patternLength - 1:
    IF shimmerMask & (1 << step):
      IF HashToFloat(seed ^ 0xFACE, step) < shiftProbability:
        // Determine shift direction
        direction = (HashToFloat(seed ^ 0xCAFE, step) > 0.5) ? 1 : -1
        newStep = (step + direction + patternLength) % patternLength

        // Only shift if destination is empty (no anchor, no shimmer)
        destEmpty = ((anchorMask & (1 << newStep)) == 0) AND
                    ((shimmerMask & (1 << newStep)) == 0)
        IF destEmpty:
          shimmerMask &= ~(1 << step)
          shimmerMask |= (1 << newStep)

RETURN shimmerMask
```

### Real-Time Constraints

- **MAX_GAPS = 16**: Fixed array, stack allocated
- **O(n) gap finding**: Single pass
- **O(n * g) distribution**: n=patternLength, g=gaps (bounded)
- **No recursion**: Iterative placement

---

## 4. Hat Burst: Pattern-Aware Fill Triggers

### V5 Design Intent

HAT mode produces intelligent bursts during fills:
- Density follows ENERGY
- Regularity follows SHAPE
- Velocity ducked near main pattern hits
- Pre-allocated (max 12 triggers)

### Algorithm Specification

```
ALGORITHM: GenerateHatBurst(energy, shape, anchorMask, shimmerMask, fillStart, fillDuration, seed)

INPUT:
  energy: float [0.0, 1.0] - Controls burst density
  shape: float [0.0, 1.0] - Controls burst regularity
  anchorMask: uint32 - Anchor pattern (for velocity ducking)
  shimmerMask: uint32 - Shimmer pattern (for velocity ducking)
  fillStart: int - Fill start step in pattern
  fillDuration: int - Fill length in steps (typically 4-8)
  seed: uint32 - Deterministic seed

OUTPUT:
  burst: HatBurst struct

// --- Step 1: Determine trigger count from ENERGY ---

minTriggers = 2
maxTriggers = 12
triggerCount = Round(minTriggers + energy * (maxTriggers - minTriggers))

// --- Step 2: Generate timing based on SHAPE ---

burst.count = 0
burst.fillStart = fillStart
mainHits = anchorMask | shimmerMask

IF shape < 0.3:
  // Low SHAPE: Even euclidean distribution
  FOR i = 0 TO triggerCount - 1:
    IF burst.count >= 12: BREAK
    step = (i * fillDuration) / triggerCount
    burst.triggers[burst.count].step = fillStart + step
    burst.count++

ELSE IF shape < 0.7:
  // Mid SHAPE: Euclidean with jitter (syncopated bursts)
  spacing = fillDuration / triggerCount
  FOR i = 0 TO triggerCount - 1:
    IF burst.count >= 12: BREAK
    baseStep = Round(i * spacing)
    // Jitter scales with shape (more jitter as shape increases)
    jitterRange = (shape - 0.3) * 2.5  // 0 to 1
    jitter = (HashToFloat(seed, i) - 0.5) * 2.0 * jitterRange
    step = Clamp(baseStep + Round(jitter * 2), 0, fillDuration - 1)
    burst.triggers[burst.count].step = fillStart + step
    burst.count++

ELSE:
  // High SHAPE: Weighted random placement
  FOR i = 0 TO triggerCount - 1:
    IF burst.count >= 12: BREAK
    step = Floor(HashToFloat(seed, i) * fillDuration)
    burst.triggers[burst.count].step = fillStart + step
    burst.count++

// --- Step 3: Compute velocities with proximity ducking ---

FOR i = 0 TO burst.count - 1:
  absoluteStep = burst.triggers[i].step % 32  // Wrap to pattern length
  baseVelocity = 0.65 + 0.35 * energy  // 65-100% base

  // Check proximity to main pattern hits (within 1 step)
  nearMainHit = FALSE
  FOR offset = -1 TO 1:
    checkStep = (absoluteStep + offset + 32) % 32
    IF mainHits & (1 << checkStep):
      nearMainHit = TRUE
      BREAK

  IF nearMainHit:
    // 70% velocity reduction when near main hits
    burst.triggers[i].velocity = baseVelocity * 0.30
  ELSE:
    burst.triggers[i].velocity = baseVelocity

RETURN burst
```

### Data Structure

```cpp
// Pre-allocated, stack-safe structure (68 bytes)
struct HatBurst {
    struct Trigger {
        uint8_t step;      // Step index (absolute)
        float velocity;    // 0.0-1.0
    };

    Trigger triggers[12];  // Max 12 triggers, always allocated
    uint8_t count;         // Actual trigger count (0-12)
    uint8_t fillStart;     // Fill start step
    uint8_t fillDuration;  // Fill length
    uint8_t _padding;      // Alignment
};
```

### Real-Time Constraints

- **Fixed 68 bytes**: No dynamic allocation
- **O(n) generation**: n = trigger count (max 12)
- **O(n) velocity calc**: Single pass with 3-step proximity check
- **Integer division**: Pre-computed spacing avoids FP division in loop

---

## 5. Fill: Multi-Attribute Boost with Exponential Curve

### V5 Design Intent

Fill applies intensifying boost with musical (non-linear) progression:
- Exponential density ramp (slow start, fast end)
- Velocity boost with accent forcing at peak
- Eligibility expansion for more hit positions
- AUX gate output for downstream effects

### Algorithm Specification

```
ALGORITHM: ComputeFillModifiers(fillActive, fillProgress, energy, baseBudgets)

INPUT:
  fillActive: bool - Whether fill is currently active
  fillProgress: float [0.0, 1.0] - Progress through fill
  energy: float [0.0, 1.0] - ENERGY parameter
  baseBudgets: {anchor: int, shimmer: int, aux: int}

OUTPUT:
  modifiers: FillModifiers struct

IF NOT fillActive:
  modifiers.densityMultiplier = 1.0
  modifiers.velocityBoost = 0.0
  modifiers.expandEligibility = FALSE
  modifiers.forceAccents = FALSE
  modifiers.accentProbability = 0.0
  modifiers.auxGateHigh = FALSE
  RETURN modifiers

// --- Exponential density curve ---
// Slow ramp at start, accelerating toward end
// Formula: 1.0 + maxBoost * (progress^2)

maxDensityBoost = 0.6 + energy * 0.4  // 60-100% max boost
densityCurve = fillProgress * fillProgress  // Quadratic for exponential feel
modifiers.densityMultiplier = 1.0 + maxDensityBoost * densityCurve

// Apply to budgets
modifiers.anchorBudget = Round(baseBudgets.anchor * modifiers.densityMultiplier)
modifiers.shimmerBudget = Round(baseBudgets.shimmer * modifiers.densityMultiplier)
modifiers.auxBudget = Round(baseBudgets.aux * modifiers.densityMultiplier)

// --- Velocity boost (linear ramp) ---
// Immediate +10%, ramping to +25% by fill end

modifiers.velocityBoost = 0.10 + 0.15 * fillProgress

// --- Accent probability ramp ---
// Gradual increase instead of hard threshold
// 50% at start, 100% at end

modifiers.accentProbability = 0.50 + 0.50 * fillProgress

// --- Force accents at peak ---
// Only force ALL accents in final 15% (not 25%)

modifiers.forceAccents = (fillProgress > 0.85)

// --- Eligibility expansion ---
// Opens 16th positions in second half of fill

modifiers.expandEligibility = (fillProgress > 0.5)
IF modifiers.expandEligibility:
  modifiers.eligibilityMask = 0xFFFFFFFF  // All positions
ELSE:
  modifiers.eligibilityMask = 0x55555555  // 8th notes only

// --- AUX gate ---
modifiers.auxGateHigh = TRUE

RETURN modifiers
```

### Real-Time Constraints

- **O(1) computation**: Direct formulas only
- **No allocation**: Modifier struct on stack
- **Quadratic curve**: Single multiply, no exp()

---

## 6. ACCENT: Musical Weight to Velocity Mapping

### V5 Design Intent

ACCENT (K4 Config) controls velocity depth:
- 0%: Flat dynamics (all hits similar, 80% floor)
- 100%: Full dynamics (ghost notes at 30%, accents at 100%)

Velocity is determined by metric position, not probability.

### Algorithm Specification

```
ALGORITHM: ComputeAccentVelocity(accent, step, patternLength, seed)

INPUT:
  accent: float [0.0, 1.0] - ACCENT parameter
  step: int - Step index
  patternLength: int
  seed: uint32 - For micro-variation

OUTPUT:
  velocity: float [0.30, 1.0]

// --- Step 1: Get metric weight for this step ---

metricWeight = GetMetricWeight(step, patternLength)
// Returns 0.0-1.0, where 1.0 = beat 1, 0.2 = weak 16th

// --- Step 2: Compute velocity range from ACCENT ---

// Low ACCENT = narrow range centered high (flat dynamics)
// High ACCENT = wide range (ghost notes to accents)

velocityFloor = 0.80 - accent * 0.50    // 80% to 30%
velocityCeiling = 0.88 + accent * 0.12  // 88% to 100%

// --- Step 3: Map metric weight to velocity ---

velocityRange = velocityCeiling - velocityFloor
baseVelocity = velocityFloor + metricWeight * velocityRange

// --- Step 4: Add micro-variation for human feel ---

variationAmount = 0.02 + accent * 0.05  // 2% to 7%
noise = (HashToFloat(seed, step) - 0.5) * 2.0  // [-1, 1]
velocity = baseVelocity + noise * variationAmount

// --- Step 5: Clamp to valid range ---

RETURN Clamp(velocity, 0.30, 1.0)
```

### Real-Time Constraints

- **O(1) per step**: Direct computation
- **Table-based metric weight**: No conditionals in hot path possible
- **Deterministic**: Same seed+step = identical velocity

---

## 7. Preset Map: Musical Style Reference

```
+==============================================================================+
|                        V5 STYLE PRESET MAP                                   |
+==============================================================================+

PERFORMANCE MODE PRESETS
------------------------

Style              ENERGY   SHAPE    AXIS X   AXIS Y   Notes
------------------ -------- -------- -------- -------- ------------------------
MINIMAL TECHNO     20%      10%      20%      20%      Sparse, locked, 4-floor
DRIVING TECHNO     45%      25%      30%      40%      Solid groove, some ghosts
GROOVY/FUNK        55%      50%      65%      55%      SHAPE at peak syncopation
BROKEN BEAT        65%      60%      75%      70%      Displaced, tension
TRIBAL/POLY        70%      45%      60%      80%      Complex interlocking
IDM/CHAOS          85%      90%      85%      90%      Maximum irregularity

CONFIG MODE SUGGESTIONS
-----------------------

Style              CLOCK    SWING    DRIFT    ACCENT   Notes
------------------ -------- -------- -------- -------- ------------------------
MINIMAL TECHNO     50%      20%      10%      70%      Tight, dynamic
DRIVING TECHNO     50%      30%      20%      75%      Punchy accents
GROOVY/FUNK        50%      60%      40%      65%      Swing is critical!
BROKEN BEAT        50%      45%      55%      60%      Moderate variation
TRIBAL/POLY        50%      50%      50%      55%      Balanced
IDM/CHAOS          Varies   Varies   80%      85%      Maximum expression

TRANSITION PATHS
----------------

Techno -> Syncopation:
  1. Increase SHAPE from 25% to 50% (enters syncopation zone)
  2. Increase AXIS X from 30% to 65% (shift to offbeats)
  3. Increase SWING from 30% to 60% (groove feel)
  4. Optionally increase DRIFT for variation

Syncopation -> Chaos:
  1. Increase SHAPE from 50% to 85%+ (enters wild zone)
  2. Increase AXIS X to 80%+ (triggers broken mode)
  3. Increase ENERGY for density
  4. Increase DRIFT for phrase variation

Recovery (Chaos -> Techno):
  1. Drop SHAPE to 20% (immediate stabilization)
  2. Drop AXIS X to 30% (restore downbeats)
  3. Reduce ENERGY if too busy
  4. DRIFT can stay high (variation on stable pattern)

+==============================================================================+
```

---

## 8. V4 to V5 Code Migration Summary

| V5 Algorithm | V4 Source Files | Migration Strategy |
|--------------|-----------------|-------------------|
| SHAPE blending | `PatternField.cpp`, `ArchetypeData.h` | Replace bilinear with 3-way; add syncopation archetype |
| AXIS biasing | None | New implementation |
| Voice COMPLEMENT | `VoiceRelation.cpp` | Simplify to gap-fill only; remove mode selection |
| Hat burst | `AuxOutput.cpp` | New feature; add burst generation |
| Fill modifiers | `HitBudget.cpp`, `VelocityCompute.cpp` | Combine; change to exponential curve |
| ACCENT | `VelocityCompute.cpp` | Replace probability with metric weight |

### Reusable V4 Components

- `HashToFloat()`, `HashStep()` from `PulseField.cpp` - keep as-is
- `GetMetricWeight()` concept from `DriftControl.cpp` - adapt naming
- `CountBits()` from `HitBudget.cpp` - keep as-is
- Gap-finding logic from `VoiceRelation.cpp` - simplify

### Components to Remove

- `SoftmaxWithTemperature()` - no longer needed
- `VoiceCoupling` enum modes (INTERLOCK, SHADOW) - COMPLEMENT only
- `GenreField` 3x3 grid structure - replaced by SHAPE zones
- Bilinear interpolation code - replaced by linear/3-way

---

## 9. Real-Time Safety Checklist

All algorithms MUST pass these criteria before implementation:

### Memory Safety
- [ ] No `new`, `delete`, `malloc`, `free`
- [ ] No `std::vector`, `std::string`, `std::map`
- [ ] All arrays fixed-size with compile-time bounds
- [ ] Maximum stack usage documented per function

### Execution Safety
- [ ] No blocking operations (I/O, locks, sleep)
- [ ] No unbounded loops (all loops have max iteration count)
- [ ] No recursion
- [ ] No floating-point exceptions possible (div by zero, sqrt negative)

### Determinism
- [ ] Same inputs = identical outputs
- [ ] No reliance on uninitialized memory
- [ ] No reliance on execution timing

### Performance
- [ ] Worst-case < 20us per audio callback at 48kHz
- [ ] No exp(), log(), sin(), cos() in per-step code
- [ ] Pre-computed tables where beneficial
- [ ] All inputs validated (Clamp before use)

### Complexity Bounds

| Algorithm | Time Complexity | Space (Stack) |
|-----------|-----------------|---------------|
| SHAPE blending | O(3n) | 384 bytes |
| AXIS biasing | O(2n) | 128 bytes |
| Voice COMPLEMENT | O(n + ng) | 272 bytes |
| Hat burst | O(12) | 68 bytes |
| Fill modifiers | O(1) | 32 bytes |
| ACCENT velocity | O(1) | 8 bytes |

Where n = patternLength (max 32), g = gap count (max 16)

---

*Final draft ready for design-critic and code-reviewer feedback.*
