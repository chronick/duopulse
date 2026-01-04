# V5 Design Final Specification

**Date**: 2026-01-04
**Status**: Ready for Implementation
**Iterations**: 8 rounds of design review

---

## Executive Summary

V5 is a complete redesign of the control interface, eliminating all shift layers while maintaining expressiveness. The result is a clean 8-parameter system (4 performance + 4 config) with intuitive knob pairing and a single "secret mode" gesture for AUX output selection.

### Key Design Principles

1. **Zero shift layers** - No hidden functions behind button holds in either mode
2. **CV law** - CV1-4 always modulate performance parameters
3. **Knob pairing** - Each knob position has related functions across modes
4. **Secret mode** - HAT output as discoverable easter egg ("2.5 pulse")

---

## Control Layout

### Performance Mode

```
K1: ENERGY    K2: SHAPE     K3: AXIS X    K4: AXIS Y
CV1           CV2           CV3           CV4
```

| Parameter | Domain | 0% | 100% |
|-----------|--------|-----|------|
| ENERGY | Density | Sparse | Busy |
| SHAPE | Regularity | Stable (humanized euclidean) | Wild (weighted) |
| AXIS X | Beat Position | Grounded (downbeats) | Floating (offbeats) |
| AXIS Y | Intricacy | Simple | Complex |

### Config Mode

```
K1: CLOCK DIV   K2: SWING   K3: DRIFT   K4: ACCENT
```

| Parameter | Domain | 0% | 100% |
|-----------|--------|-----|------|
| CLOCK DIV | Tempo | ÷4 (slow) | ×4 (fast) |
| SWING | Groove | Straight | Heavy swing |
| DRIFT | Evolution | Locked (same each phrase) | Evolving |
| ACCENT | Velocity Depth | Flat (all hits equal) | Dynamic (ghost notes to accents) |

### Knob Pairing

| Knob | Performance | Config | Conceptual Link |
|------|-------------|--------|-----------------|
| K1 | ENERGY | CLOCK DIV | Rate/Density |
| K2 | SHAPE | SWING | Timing Feel |
| K3 | AXIS X | DRIFT | Variation/Movement |
| K4 | AXIS Y | ACCENT | Intricacy/Depth |

---

## Button & Switch Behavior

### Button Gestures

| Gesture | Effect |
|---------|--------|
| Tap (20-500ms) | Trigger Fill |
| Hold 3s | Reseed pattern on release |
| Hold + Switch UP | HAT mode ("2.5 pulse" secret) |
| Hold + Switch DOWN | FILL GATE mode (default) |

### Switch

| Position | Normal | With Button Held |
|----------|--------|------------------|
| UP | Performance mode | Set AUX to HAT |
| DOWN | Config mode | Set AUX to FILL GATE |

### AUX Mode Gesture Priority

When button is held and switch moves:
1. Cancel any pending shift mode activation
2. Cancel any pending live fill
3. Set AUX mode based on switch direction
4. Consume switch event (don't change Perf/Config mode)
5. Button release returns to normal (no fill triggered)

---

## AUX Output Modes

### FILL GATE (Default)

- AUX output goes HIGH during fill duration
- Simple gate signal for triggering external effects
- Matches "duopulse" branding (2 voices)

### HAT Mode (Secret "2.5 Pulse")

- AUX output produces pattern-aware hat burst during fill
- Burst density follows ENERGY
- Burst regularity follows SHAPE
- Pre-allocated (max 12 triggers, no heap allocation)
- Velocity reduced 70% near main pattern hits

### LED Feedback

```
HAT mode unlock:    ∙∙∙ ● ● ● (triple rising flash)
FILL GATE reset:    ● ∙ (single fade)
```

---

## Boot-Time Configuration

### Pattern Length

Default: 16 steps (compile-time)

Optional boot-time override:
- Hold button during power-on
- Switch UP: 16 steps (1 LED flash)
- Switch DOWN: 24 steps (2 LED flashes)

Compile-time options for 32/64 steps available for custom builds.

### AUX Mode

Always defaults to FILL GATE on boot (volatile, not persisted).

---

## CV Behavior

CV inputs (CV1-4) **always modulate Performance mode parameters**, regardless of current mode:

- In Performance mode: CV modulates same parameters as knobs
- In Config mode: CV still modulates performance parameters in background

This allows CV sequences to keep running while adjusting config settings.

---

## Algorithm Summary

| Algorithm | Approach |
|-----------|----------|
| SHAPE blending | Deterministic score interpolation |
| AXIS biasing | Additive offsets (no dead zones) |
| Voice relationship | COMPLEMENT + DRIFT variation |
| Hat burst | Pattern-aware, pre-allocated arrays |
| Fill | Multi-attribute boost + AUX output |
| ACCENT | Musical weight → velocity mapping |

---

## LED Feedback Summary

| State | Behavior | Layer |
|-------|----------|-------|
| Idle (perf) | Gentle breath synced to clock | 1 (base) |
| Idle (config) | Slower breath | 1 (base) |
| Clock sync | Subtle pulse on beats | 2 (additive) |
| Trigger activity | Pulse on hits, envelope decay | 3 (maximum) |
| Fill active | Accelerating strobe + trigger overlay | 4 (maximum) |
| Reseed progress | Building pulse (1-5Hz over 3s) | 5 (replace) |
| Reseed confirm | POP POP POP (3 flashes) | 5 (replace) |
| Mode switch | Quick signature | 5 (replace) |
| AUX mode unlock | Triple rising flash | 5 (replace) |
| AUX mode reset | Single fade | 5 (replace) |

---

## Hardware I/O

| I/O | Function |
|-----|----------|
| CV1-4 | Modulate ENERGY, SHAPE, AXIS X, AXIS Y |
| Gate Out 1 | Voice 1 triggers |
| Gate Out 2 | Voice 2 triggers |
| AUX Out | Fill gate OR hat burst |
| Clock In | External clock (normalled to internal) |
| Button | Fill / Reseed / AUX mode gesture |
| Switch | Mode select (up=Perf, down=Config) |
| LED | Status feedback (brightness only) |

---

## Implementation Checklist

### Critical (Must Do)

- [ ] Pre-allocate HatBurst arrays (no heap in audio path)
- [ ] Use deterministic score interpolation for SHAPE
- [ ] Use additive biasing for AXIS X/Y
- [ ] Implement LED layering as specified
- [ ] Layer trigger feedback on top of fills
- [ ] Implement Hold+Switch gesture for AUX MODE with proper cancellation
- [ ] Implement ACCENT parameter using musical weight for velocity

### Important (Should Do)

- [ ] COMPLEMENT relationship with DRIFT variation
- [ ] 70% velocity reduction for hat burst near main hits
- [ ] Boot-time LENGTH selection (button + switch at power-on)
- [ ] PATTERN_LENGTH as compile-time constant with boot override

### Deferred (Future)

- [ ] Extensibility framework (when bassline/ambient variations designed)
- [ ] Additional voice relationships (if user feedback indicates need)

---

## Open Items for Implementation

1. **CV slew/response** - Suggest: instant with per-phrase regeneration
2. **Power-on seed** - Suggest: fixed default seed, reseed to randomize

---

## Design Session Reference

Full design history in `docs/design/v5-ideation/`:
- `feedback.md` - Complete feedback log (8 rounds)
- `iteration-1.md` through `iteration-8.md` - Design evolution
- `critic-response-*.md` - Critical reviews

Key decisions made through iteration:
- Eliminated genre selection (algorithm-focused instead)
- Removed BUILD control (CV modulation replaces it)
- Simplified voice relationship to COMPLEMENT-only
- Added ACCENT as replacement for LENGTH in Config
- Created Hold+Switch gesture for AUX MODE

---

## Appendix A: Algorithm Implementation Details

**Date**: 2026-01-04
**Status**: Reviewed and Approved
**Reviews**: Design-critic, Code-reviewer, Implementation feasibility

---

### A.1 Reviewer Feedback Summary

Three reviews were conducted on the algorithm specifications:

#### Design Critic Findings

| Priority | Issue | Resolution |
|----------|-------|------------|
| **P0** | SHAPE zone boundary discontinuity at 30%/50%/70% | Add 4% crossfade overlap windows |
| **P0** | Broken mode activation ambiguity (SHAPE+AXIS X) | Document interaction zone clearly |
| **P1** | Syncopation downbeat suppression too aggressive | Increase beat 1 base weight to 0.50 |
| **P2** | Hat burst timing overlap (no collision detection) | Add nearest-empty nudging |
| **P2** | Wrap-around gap handling in COMPLEMENT | Combine tail+head into single gap |
| **P3** | Fill velocity boost stacking compresses dynamics | Consider multiplicative boost |

**Verdict**: Ready to implement with P0/P1 fixes applied.

#### Code Reviewer Findings (Real-Time Safety)

| Category | Status | Notes |
|----------|--------|-------|
| Heap Allocation | PASS | All arrays fixed-size, stack allocated |
| Bounded Execution | PASS | All loops have max iteration counts |
| Division Safety | NEEDS FIX | Add `Max(1, ...)` guards to prevent div-by-zero |
| Determinism | PASS | Same seed = identical output |
| Stack Usage | PASS | ~876 bytes worst-case (safe for STM32H7) |

#### Implementation Feasibility Findings

| Algorithm | Existing Code | Work Required |
|-----------|---------------|---------------|
| SHAPE blending | ~80% exists in PatternField.cpp | Wire to Euclidean ratio |
| AXIS biasing | ~95% exists | Rename FIELD X/Y |
| Voice COMPLEMENT | ~70% exists | Define semantics, simplify modes |
| Hat burst | ~20% exists | **Main new feature** |
| Fill modifiers | ~90% exists | Tuning only |
| ACCENT velocity | ~95% exists | Rename PUNCH parameter |

**Estimated total effort**: 5-7 SDD tasks (~27 hours)

---

### A.2 SHAPE Blending: Three-Way Interpolation

SHAPE (K2) controls pattern regularity through THREE character zones:
- **0-30%**: Stable humanized euclidean (techno, four-on-floor)
- **30-70%**: Syncopation peak (funk, displaced, tension)
- **70-100%**: Wild weighted (IDM, chaos)

#### Algorithm

```
ALGORITHM: ComputeShapeBlendedWeights(shape, energy, seed, patternLength)

INPUT:
  shape: float [0.0, 1.0]
  energy: float [0.0, 1.0]
  seed: uint32
  patternLength: int [16, 24, 32]

OUTPUT:
  weights[patternLength]: float[]

// Generate three base archetypes
stableWeights = GenerateStablePattern(energy, patternLength)
syncopatedWeights = GenerateSyncopationPattern(energy, seed, patternLength)
wildWeights = GenerateWildPattern(energy, seed, patternLength)

// Three-way interpolation with CROSSFADE OVERLAP
FOR step = 0 TO patternLength - 1:
  IF shape < 0.28:
    // Pure Zone 1: stable with humanization
    t = shape / 0.28
    weights[step] = stableWeights[step]
    humanize = 0.05 * (1.0 - t)
    weights[step] += (HashToFloat(seed, step) - 0.5) * humanize

  ELSE IF shape < 0.32:
    // Crossfade Zone 1 -> Zone 2a (4% overlap)
    fadeT = (shape - 0.28) / 0.04
    zone1 = stableWeights[step] + (HashToFloat(seed, step) - 0.5) * 0.05 * (1 - fadeT)
    zone2a = Lerp(stableWeights[step], syncopatedWeights[step], 0.0)
    weights[step] = Lerp(zone1, zone2a, fadeT)

  ELSE IF shape < 0.48:
    // Pure Zone 2a: stable -> syncopated
    t = (shape - 0.32) / 0.16
    weights[step] = Lerp(stableWeights[step], syncopatedWeights[step], t)

  ELSE IF shape < 0.52:
    // Crossfade Zone 2a -> Zone 2b (4% overlap at peak syncopation)
    fadeT = (shape - 0.48) / 0.04
    zone2a = Lerp(stableWeights[step], syncopatedWeights[step], 1.0)
    zone2b = Lerp(syncopatedWeights[step], wildWeights[step], 0.0)
    weights[step] = Lerp(zone2a, zone2b, fadeT)

  ELSE IF shape < 0.68:
    // Pure Zone 2b: syncopated -> wild
    t = (shape - 0.52) / 0.16
    weights[step] = Lerp(syncopatedWeights[step], wildWeights[step], t)

  ELSE IF shape < 0.72:
    // Crossfade Zone 2b -> Zone 3 (4% overlap)
    fadeT = (shape - 0.68) / 0.04
    zone2b = Lerp(syncopatedWeights[step], wildWeights[step], 1.0)
    zone3 = wildWeights[step]
    weights[step] = Lerp(zone2b, zone3, fadeT)

  ELSE:
    // Pure Zone 3: wild with chaos
    t = (shape - 0.72) / 0.28
    weights[step] = wildWeights[step]
    chaos = t * 0.15
    weights[step] += (HashToFloat(seed ^ 0xCAFEBABE, step) - 0.5) * chaos

  weights[step] = Clamp(weights[step], 0.0, 1.0)

RETURN weights
```

#### Syncopation Pattern (with downbeat protection)

```
ALGORITHM: GenerateSyncopationPattern(energy, seed, patternLength)

// Syncopation creates tension through:
// 1. Suppressed (but not eliminated) downbeats
// 2. Boosted anticipation positions
// 3. Strong offbeats

FOR step = 0 TO patternLength - 1:
  metricStrength = GetMetricWeight(step, patternLength)

  IF metricStrength > 0.9:
    // Beat 1 - SUPPRESS but protect (increased from 0.35)
    weights[step] = 0.50 + energy * 0.20  // 50-70% (was 35-50%)

  ELSE IF metricStrength > 0.7:
    // Other strong beats - moderate suppression
    weights[step] = 0.45 + energy * 0.15

  ELSE IF IsAnticipationPosition(step, patternLength):
    // Anticipation (step before downbeat) - BOOST
    weights[step] = 0.80 + energy * 0.15

  ELSE IF metricStrength < 0.3:
    // Weak offbeats - BOOST
    weights[step] = 0.70 + energy * 0.20

  ELSE:
    weights[step] = 0.50 + energy * 0.20

RETURN weights
```

---

### A.3 AXIS Biasing: Bidirectional Offsets

AXIS X/Y provide continuous control with full-range effect:
- **AXIS X**: Bidirectional beat position (suppresses AND boosts)
- **AXIS Y**: Intricacy with increased range (+/-50%)

#### Algorithm

```
ALGORITHM: ApplyAxisBias(baseWeights, axisX, axisY, shape, seed, patternLength)

// Compute bipolar biases (0.5 = neutral)
xBias = (axisX - 0.5) * 2.0  // [-1.0, 1.0]
yBias = (axisY - 0.5) * 2.0  // [-1.0, 1.0]

// Apply X bias (bidirectional)
FOR step = 0 TO patternLength - 1:
  positionStrength = GetPositionStrength(step, patternLength)

  IF xBias > 0:
    // Moving toward offbeats: suppress downbeats, boost offbeats
    IF positionStrength < 0:
      xEffect = -xBias * Abs(positionStrength) * 0.45  // Suppress
    ELSE:
      xEffect = xBias * positionStrength * 0.60  // Boost
  ELSE:
    // Moving toward downbeats: boost downbeats, suppress offbeats
    IF positionStrength < 0:
      xEffect = -xBias * Abs(positionStrength) * 0.60  // Boost
    ELSE:
      xEffect = xBias * positionStrength * 0.45  // Suppress

  weights[step] = Clamp(baseWeights[step] + xEffect, 0.05, 1.0)

// Apply Y bias (intricacy) - increased to +/-50%
FOR step = 0 TO patternLength - 1:
  isWeakPosition = GetMetricWeight(step, patternLength) < 0.5

  IF yBias > 0:
    yEffect = yBias * (isWeakPosition ? 0.50 : 0.15)
  ELSE:
    yEffect = yBias * (isWeakPosition ? 0.50 : -0.25)

  weights[step] = Clamp(weights[step] + yEffect, 0.05, 1.0)

// "Broken" mode at high SHAPE + high AXIS X
// NOTE: This interaction zone should be documented for users
IF shape > 0.6 AND axisX > 0.7:
  brokenIntensity = (shape - 0.6) * 2.5 * (axisX - 0.7) * 3.33
  FOR step IN downbeatPositions:
    IF HashToFloat(seed ^ 0xDEADBEEF, step) < brokenIntensity * 0.6:
      weights[step] *= 0.25

RETURN weights
```

---

### A.4 Voice COMPLEMENT with DRIFT

Shimmer voice fills gaps in anchor pattern. DRIFT controls placement variation.

```
ALGORITHM: ApplyComplementRelationship(anchorMask, shimmerWeights, drift, seed, patternLength)

// Find gaps in anchor pattern (including wrap-around)
gaps = FindGaps(anchorMask, patternLength)

// Handle wrap-around: combine tail+head if both are gaps
IF gaps[0].start == 0 AND gaps[last].end == patternLength:
  CombineWrapAroundGaps(gaps)

// Guard against div-by-zero
totalGapLength = SumGapLengths(gaps)
IF totalGapLength == 0:
  RETURN 0

// Distribute shimmer hits proportionally
FOR each gap:
  gapShare = Max(1, Round(gap.length * targetHits / totalGapLength))

  FOR j = 0 TO gapShare - 1:
    IF drift < 0.3:
      position = EvenlySpaced(gap, j, gapShare)
    ELSE IF drift < 0.7:
      position = WeightedBest(gap, shimmerWeights)
    ELSE:
      position = SeedVaried(gap, seed, j)

    PlaceShimmerHit(position)

RETURN shimmerMask
```

---

### A.5 Hat Burst: Pattern-Aware Fill Triggers

```
ALGORITHM: GenerateHatBurst(energy, shape, mainPattern, fillStart, fillDuration, seed)

// Determine trigger count (with div-by-zero guard)
triggerCount = Max(1, Round(2 + energy * 10))  // 2-12 triggers

// Generate timing with collision detection
usedSteps = {}
FOR i = 0 TO triggerCount - 1:
  IF burst.count >= 12: BREAK

  IF shape < 0.3:
    step = (i * fillDuration) / Max(1, triggerCount)
  ELSE IF shape < 0.7:
    step = EuclideanWithJitter(i, fillDuration, shape, seed)
  ELSE:
    step = RandomStep(fillDuration, seed, i)

  // Collision detection: nudge to nearest empty
  IF step IN usedSteps:
    step = FindNearestEmpty(step, fillDuration, usedSteps)

  usedSteps.add(step)
  burst.triggers[burst.count].step = fillStart + step
  burst.count++

// Velocity ducking near main hits
FOR each trigger:
  nearMainHit = CheckProximity(trigger.step, mainPattern, 1)
  baseVelocity = 0.65 + 0.35 * energy
  trigger.velocity = nearMainHit ? baseVelocity * 0.30 : baseVelocity

RETURN burst
```

**Pre-allocated structure** (68-96 bytes depending on alignment):
```cpp
struct HatBurst {
    struct Trigger { uint8_t step; float velocity; };
    Trigger triggers[12];
    uint8_t count, fillStart, fillDuration, _pad;
};
```

---

### A.6 Fill: Exponential Multi-Attribute Boost

```
ALGORITHM: ComputeFillModifiers(fillProgress, energy)

IF NOT fillActive:
  RETURN defaults

// Exponential density curve (progress^2)
maxBoost = 0.6 + energy * 0.4
densityMultiplier = 1.0 + maxBoost * (fillProgress * fillProgress)

// Velocity boost (linear ramp)
velocityBoost = 0.10 + 0.15 * fillProgress

// Accent probability ramp (gradual, not hard threshold)
accentProbability = 0.50 + 0.50 * fillProgress
forceAccents = (fillProgress > 0.85)

// Eligibility expansion at midpoint
expandEligibility = (fillProgress > 0.5)

RETURN modifiers
```

---

### A.7 ACCENT: Musical Weight to Velocity

```
ALGORITHM: ComputeAccentVelocity(accent, step, patternLength, seed)

metricWeight = GetMetricWeight(step, patternLength)

// Velocity range scales with ACCENT
velocityFloor = 0.80 - accent * 0.50    // 80% -> 30%
velocityCeiling = 0.88 + accent * 0.12  // 88% -> 100%

// Map metric weight to velocity
velocity = velocityFloor + metricWeight * (velocityCeiling - velocityFloor)

// Micro-variation for human feel
variation = 0.02 + accent * 0.05
velocity += (HashToFloat(seed, step) - 0.5) * variation

RETURN Clamp(velocity, 0.30, 1.0)
```

---

### A.8 Style Preset Map

```
PERFORMANCE MODE
----------------
Style              ENERGY   SHAPE    AXIS X   AXIS Y
MINIMAL TECHNO     20%      10%      20%      20%
DRIVING TECHNO     45%      25%      30%      40%
GROOVY/FUNK        55%      50%      65%      55%   <- syncopation peak
BROKEN BEAT        65%      60%      75%      70%   <- enters broken mode
TRIBAL/POLY        70%      45%      60%      80%
IDM/CHAOS          85%      90%      85%      90%

CONFIG MODE
-----------
Style              CLOCK    SWING    DRIFT    ACCENT
MINIMAL TECHNO     50%      20%      10%      70%
DRIVING TECHNO     50%      30%      20%      75%
GROOVY/FUNK        50%      60%      40%      65%   <- swing critical!
BROKEN BEAT        50%      45%      55%      60%
IDM/CHAOS          Varies   Varies   80%      85%

TRANSITION PATHS
----------------
Techno -> Syncopation:
  1. SHAPE 25% -> 50% (enters syncopation zone)
  2. AXIS X 30% -> 65% (shift to offbeats)
  3. SWING 30% -> 60% (groove feel)

Syncopation -> Chaos:
  1. SHAPE 50% -> 85%+ (enters wild zone)
  2. AXIS X 65% -> 80%+ (triggers broken mode)
  3. ENERGY up for density
```

---

### A.9 Real-Time Safety Checklist

| Criterion | Status | Notes |
|-----------|--------|-------|
| No heap allocation | PASS | All arrays fixed-size |
| Bounded loops | PASS | Max 32 steps, 16 gaps, 12 triggers |
| No blocking ops | PASS | Pure computation |
| Division guards | NEEDS IMPL | Add Max(1, ...) to prevent /0 |
| Stack usage | PASS | ~876 bytes worst case |
| Deterministic | PASS | Same seed = identical output |

---

### A.10 Implementation Task Sequence

| Order | Task | Risk | Effort |
|-------|------|------|--------|
| 1 | Control parameter renaming | Low | 2h |
| 2 | SHAPE parameter integration | Medium | 4h |
| 3 | ACCENT range verification | Low | 2h |
| 4 | Voice COMPLEMENT definition | Medium | 3h |
| 5 | Hat burst pre-allocation | Low | 2h |
| 6 | Hat burst pattern generation | High | 4h |
| 7 | Hat burst velocity ducking | Medium | 3h |
| 8 | AUX mode gesture (Hold+Switch) | Medium | 4h |
| 9 | LED feedback updates | Low | 3h |

**Total**: ~27 hours across 5-7 SDD tasks

---

*Algorithm appendix reviewed by design-critic, code-reviewer, and implementation feasibility agents. Ready for task creation.*
