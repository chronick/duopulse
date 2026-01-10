# DuoPulse v5: Algorithmic Drum Sequencer Specification

**Target Platform**: Daisy Patch.init() (Electro-Smith)
**Version**: 5.0
**Status**: Complete
**Last Updated**: 2026-01-06

### Pending Changes (Active Tasks)

| Task | Spec Section | Change |
|------|--------------|--------|
| **Task 46** | SHAPE Algorithm | V5.5 noise formula fix |
| **Task 47** | ACCENT Velocity | V5.5 velocity variation (ghost notes) |
| **Task 48** | Generation Pipeline | V5.5 micro-displacement |
| **Task 49** | AUX Output | V5.5 AUX style zones, beat 1 enforcement |
| **Task 50** | Generation Pipeline | V5.5 two-pass generation (conditional) |

### Recent Changes

| Task | Date | Changes |
|------|------|---------|
| **Task 45** | 2026-01-08 | Pattern generator extraction (shared firmware/viz module) |
| **Task 44** | 2026-01-07 | Anchor seed variation fix |
| **Task 43** | 2026-01-07 | Shimmer seed variation fix |
| **Task 42** | 2026-01-07 | V4 dead code cleanup (~2,630 lines removed) |
| **Task 41** | 2026-01-06 | SHAPE budget zone boundaries fix |
| **Task 40** | 2026-01-06 | PatternGenerator class extraction |
| **Task 39** | 2026-01-06 | SHAPE-modulated hit budget for pattern variation |
| **Task 38** | 2026-01-06 | C++ pattern visualization test tool |
| **Task 37** | 2026-01-04 | Code review critical fixes (bounds, null checks) |
| **Task 35** | 2026-01-04 | ACCENT parameter with metric weight velocity |
| **Task 34** | 2026-01-04 | 5-layer priority LED feedback system |
| **Task 33** | 2026-01-04 | Boot-time AUX mode selection gesture |
| **Task 32** | 2026-01-04 | Hold+Switch gesture for runtime AUX mode |
| **Task 31** | 2026-01-04 | Hat burst pattern-aware fill triggers |
| **Task 30** | 2026-01-04 | Voice COMPLEMENT relationship with DRIFT |
| **Task 29** | 2026-01-04 | AXIS X/Y bidirectional biasing |
| **Task 28** | 2026-01-04 | SHAPE 3-way blending algorithm |
| **Task 27** | 2026-01-04 | Control renaming, zero shift layers |

---

## Table of Contents

1. [Core Principles](#1-core-principles)
2. [Architecture Overview](#2-architecture-overview)
3. [Hardware Interface](#3-hardware-interface)
4. [Control System](#4-control-system)
5. [SHAPE Algorithm](#5-shape-algorithm)
6. [AXIS Biasing](#6-axis-biasing)
7. [Voice Relationship (COMPLEMENT)](#7-voice-relationship-complement)
8. [ACCENT Velocity](#8-accent-velocity)
9. [Fill System](#9-fill-system)
10. [AUX Output System](#10-aux-output-system)
11. [LED Feedback System](#11-led-feedback-system)
12. [Boot Behavior](#12-boot-behavior)
13. [Clock and Reset](#13-clock-and-reset)
14. [Testing Requirements](#14-testing-requirements)

---

## Quick Reference: I/O Summary

### Outputs
| Output | Hardware | Signal |
|--------|----------|--------|
| **Voice 1 Trig** | Gate Out 1 | 5V trigger on anchor hits |
| **Voice 2 Trig** | Gate Out 2 | 5V trigger on shimmer hits |
| **Voice 1 Velocity** | Audio Out L | 0-5V sample & hold |
| **Voice 2 Velocity** | Audio Out R | 0-5V sample & hold |
| **AUX** | CV Out 1 | Fill Gate (default) OR Hat Burst |
| **LED** | CV Out 2 | Visual feedback (brightness) |

### Inputs
| Input | Hardware | Function |
|-------|----------|----------|
| **Clock** | Gate In 1 | External clock (disables internal when patched) |
| **Reset** | Gate In 2 | Reset to step 0 |
| **Fill CV** | Audio In L | Fill trigger (>1V gate) |

### Performance Mode (Switch UP)
| Knob | Parameter | 0% | 100% |
|------|-----------|-----|------|
| K1 | **ENERGY** | Sparse | Busy |
| K2 | **SHAPE** | Stable (humanized euclidean) | Wild (weighted) |
| K3 | **AXIS X** | Grounded (downbeats) | Floating (offbeats) |
| K4 | **AXIS Y** | Simple | Complex |

### Config Mode (Switch DOWN)
| Knob | Parameter | 0% | 100% |
|------|-----------|-----|------|
| K1 | **CLOCK DIV** | ÷4 (slow) | ×4 (fast) |
| K2 | **SWING** | Straight | Heavy swing |
| K3 | **DRIFT** | Locked (same each phrase) | Evolving |
| K4 | **ACCENT** | Flat (all hits equal) | Dynamic (ghosts to accents) |

### CV Inputs (always modulate Performance params)
| CV In | Modulates | Range |
|-------|-----------|-------|
| CV 1 | ENERGY | ±50% |
| CV 2 | SHAPE | ±50% |
| CV 3 | AXIS X | ±50% |
| CV 4 | AXIS Y | ±50% |

---

## 1. Core Principles

### 1.1 Design Philosophy

DuoPulse v5 is an **opinionated drum sequencer** with:

1. **Zero shift layers**: Every parameter is directly accessible
2. **CV law**: CV1-4 always modulate performance parameters, regardless of mode
3. **Knob pairing**: Related functions across Performance/Config modes
4. **Secret mode**: Hat burst as discoverable easter egg ("2.5 pulse")
5. **Deterministic variation**: Same settings + seed = identical output

### 1.2 Key Changes from v4

| Aspect | v4 | v5 |
|--------|----|----|
| Shift layers | 4 shift parameters | None |
| Parameters | 12+ with enums | 8 direct knobs |
| GENRE control | 3 selectable genres | Algorithm-driven (removed) |
| BALANCE | Voice ratio knob | Removed (SHAPE handles) |
| BUILD | Phrase arc | Replaced by SHAPE |
| PUNCH | Velocity dynamics | Replaced by ACCENT |
| Voice coupling | Independent/Shadow/Interlock | COMPLEMENT only |
| AUX modes | 4 config options | 2 via gesture (HAT/FILL GATE) |

### 1.3 Knob Pairing Concept

Each knob position has related functions across modes:

| Knob | Performance | Config | Conceptual Link |
|------|-------------|--------|-----------------|
| K1 | ENERGY | CLOCK DIV | Rate/Density |
| K2 | SHAPE | SWING | Timing Feel |
| K3 | AXIS X | DRIFT | Variation/Movement |
| K4 | AXIS Y | ACCENT | Intricacy/Depth |

---

## 2. Architecture Overview

### 2.1 High-Level Pipeline

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           CONTROL LAYER                                  │
│                                                                          │
│   ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐                    │
│   │ ENERGY  │  │  SHAPE  │  │ AXIS X  │  │ AXIS Y  │                    │
│   │  (K1)   │  │  (K2)   │  │  (K3)   │  │  (K4)   │                    │
│   └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘                    │
│        │            │            │            │                          │
│        ▼            ▼            └──────┬─────┘                          │
│   ┌─────────┐  ┌─────────────┐         │                                │
│   │   HIT   │  │  3-WAY      │         ▼                                │
│   │  BUDGET │  │  BLENDING   │  ┌────────────────┐                      │
│   └────┬────┘  └──────┬──────┘  │ AXIS BIASING   │                      │
│        │              │         └───────┬────────┘                      │
└────────┼──────────────┼─────────────────┼───────────────────────────────┘
         │              │                 │
         ▼              ▼                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         GENERATION LAYER                                 │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                     WEIGHTED SAMPLING                               │ │
│  │  (Gumbel Top-K with SHAPE-blended weights + AXIS-biased scores)    │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                              │                                           │
│                              ▼                                           │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                   COMPLEMENT RELATIONSHIP                           │ │
│  │  (Voice 2 fills gaps in Voice 1, placement varies with DRIFT)       │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                              │                                           │
│                              ▼                                           │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                    ACCENT VELOCITY                                  │ │
│  │  (Metric weight → velocity, range scaled by ACCENT param)           │ │
│  └────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          OUTPUT LAYER                                    │
│                                                                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │ GATE 1  │ │ GATE 2  │ │ OUT L   │ │ OUT R   │ │  CV 1   │           │
│  │ Voice 1 │ │ Voice 2 │ │ V1 Vel  │ │ V2 Vel  │ │   AUX   │           │
│  │  Trig   │ │  Trig   │ │  (S&H)  │ │  (S&H)  │ │         │           │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘           │
│                                                                          │
│  CV_OUT_2 = LED (visual feedback)                                        │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Processing Flow Per Bar

1. **Compute hit budgets** from ENERGY (SHAPE modulates anchor/shimmer ratio)
2. **Generate SHAPE-blended weights** (stable ↔ syncopated ↔ wild)
3. **Apply AXIS biasing** (X = beat position, Y = intricacy)
4. **Select hits via Gumbel Top-K** sampling
5. **Apply COMPLEMENT** for Voice 2 (gap-filling with DRIFT variation)
6. **Compute velocities** from ACCENT + metric position
7. **Store hit masks** for the bar
8. **On each step**: apply SWING timing, output triggers + velocities

---

## 3. Hardware Interface

### 3.1 Patch.init() Hardware Map

| Hardware | Label | Function |
|----------|-------|----------|
| Knob 1 | CTRL_1 | K1: ENERGY (perf) / CLOCK DIV (config) |
| Knob 2 | CTRL_2 | K2: SHAPE (perf) / SWING (config) |
| Knob 3 | CTRL_3 | K3: AXIS X (perf) / DRIFT (config) |
| Knob 4 | CTRL_4 | K4: AXIS Y (perf) / ACCENT (config) |
| CV In 1 | CV_1 | ENERGY modulation |
| CV In 2 | CV_2 | SHAPE modulation |
| CV In 3 | CV_3 | AXIS X modulation |
| CV In 4 | CV_4 | AXIS Y modulation |
| Audio In L | IN_L | FILL CV input (>1V triggers fill) |
| Gate In 1 | GATE_IN_1 | Clock input |
| Gate In 2 | GATE_IN_2 | Reset input |
| Gate Out 1 | GATE_OUT_1 | Voice 1 trigger |
| Gate Out 2 | GATE_OUT_2 | Voice 2 trigger |
| CV Out 1 | CV_OUT_1 | AUX output (Fill Gate or Hat Burst) |
| CV Out 2 | CV_OUT_2 | LED output |
| Audio Out L | OUT_L | Voice 1 velocity (0-5V S&H) |
| Audio Out R | OUT_R | Voice 2 velocity (0-5V S&H) |
| Button | SW_1 | Fill (tap) / Reseed (hold 3s) / AUX gesture |
| Switch | SW_2 | Performance (UP) / Config (DOWN) |

### 3.2 CV Input Behavior

CV inputs **always modulate Performance mode parameters**, regardless of current mode:

| CV Input | Modulates | Behavior |
|----------|-----------|----------|
| CV_1 | ENERGY | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_2 | SHAPE | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_3 | AXIS X | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_4 | AXIS Y | Bipolar: 0V = no mod, ±5V = ±50% |

This allows CV sequences to keep running while adjusting config settings.

---

## 4. Control System

### 4.1 Mode Overview

| Mode | Switch Position | Knob Behavior |
|------|-----------------|---------------|
| **Performance** | UP | ENERGY / SHAPE / AXIS X / AXIS Y |
| **Config** | DOWN | CLOCK DIV / SWING / DRIFT / ACCENT |

**No shift layers exist.** Each mode has 4 direct parameters.

### 4.2 Performance Mode Parameters

| Knob | Parameter | Range | Function |
|------|-----------|-------|----------|
| K1 | **ENERGY** | 0-100% | Hit density: how many hits per bar |
| K2 | **SHAPE** | 0-100% | Pattern character: stable → syncopated → wild |
| K3 | **AXIS X** | 0-100% | Beat position: grounded (downbeats) → floating (offbeats) |
| K4 | **AXIS Y** | 0-100% | Intricacy: simple → complex |

### 4.3 Config Mode Parameters

| Knob | Parameter | Range | Function |
|------|-----------|-------|----------|
| K1 | **CLOCK DIV** | ÷4 to ×4 | Clock division/multiplication |
| K2 | **SWING** | 0-100% | Timing feel: straight → heavy swing |
| K3 | **DRIFT** | 0-100% | Evolution: locked → varies each phrase |
| K4 | **ACCENT** | 0-100% | Velocity range: flat → dynamic |

### 4.4 Button Gestures

| Gesture | Effect |
|---------|--------|
| **Tap** (20-500ms) | Trigger Fill |
| **Hold 3s** | Reseed pattern on release |
| **Hold + Switch UP** | Set AUX to HAT mode (secret "2.5 pulse") |
| **Hold + Switch DOWN** | Set AUX to FILL GATE mode (default) |

### 4.5 AUX Mode Gesture Priority

When button is held and switch moves:
1. Cancel any pending fill
2. Set AUX mode based on switch direction
3. Consume switch event (don't change Perf/Config mode)
4. Button release returns to normal (no fill triggered)

---

## 5. SHAPE Algorithm

SHAPE (K2) controls pattern regularity through THREE character zones with crossfade transitions.

### 5.1 Zone Overview

| Zone | SHAPE Range | Character |
|------|-------------|-----------|
| **Stable** | 0-30% | Humanized euclidean, techno, four-on-floor |
| **Syncopated** | 30-70% | Funk, displaced, tension |
| **Wild** | 70-100% | IDM, chaos, weighted random |

### 5.2 Three-Way Blending

```
SHAPE  0%          30%          50%          70%         100%
       |--- Stable --|-- Crossfade --|-- Syncopated --|-- Crossfade --|-- Wild --|
                   (4%)                              (4%)
```

Crossfade windows (4% overlap) prevent discontinuities at zone boundaries.

### 5.3 Algorithm

```
FOR each step:
  IF shape < 0.28:
    // Pure Zone 1: stable with humanization
    weight = stableWeight[step]
    humanize = 0.05 * (1.0 - shape/0.28)
    weight += (hash(seed, step) - 0.5) * humanize

  ELSE IF shape < 0.32:
    // Crossfade Zone 1 → Zone 2
    fade = (shape - 0.28) / 0.04
    weight = lerp(stableWeight, syncopatedWeight, fade)

  ELSE IF shape < 0.68:
    // Zone 2: stable ↔ syncopated ↔ wild
    t = (shape - 0.32) / 0.36
    IF t < 0.5:
      weight = lerp(stableWeight, syncopatedWeight, t * 2)
    ELSE:
      weight = lerp(syncopatedWeight, wildWeight, (t - 0.5) * 2)

  ELSE IF shape < 0.72:
    // Crossfade Zone 2 → Zone 3
    fade = (shape - 0.68) / 0.04
    weight = lerp(syncopatedWeight, wildWeight, fade)

  ELSE:
    // Pure Zone 3: wild with chaos
    weight = wildWeight[step]
    chaos = (shape - 0.72) / 0.28 * 0.15
    weight += (hash(seed ^ 0xCAFEBABE, step) - 0.5) * chaos
```

### 5.4 SHAPE-Modulated Hit Budget

SHAPE also affects anchor/shimmer hit ratio:

| Zone | Anchor Budget | Shimmer Budget |
|------|---------------|----------------|
| Stable (0-30%) | 100% of base | 100% of base |
| Syncopated (30-70%) | 90-100% | 110-130% |
| Wild (70-100%) | 80-90% | 130-150% |

---

## 6. AXIS Biasing

AXIS X/Y provide continuous control with bidirectional effect (no dead zones).

### 6.1 AXIS X: Beat Position

```
xBias = (axisX - 0.5) * 2.0  // [-1.0, 1.0]

FOR each step:
  positionStrength = GetPositionStrength(step)  // negative=downbeat, positive=offbeat

  IF xBias > 0:
    // Moving toward offbeats
    IF positionStrength < 0:
      xEffect = -xBias * |positionStrength| * 0.45  // Suppress downbeats
    ELSE:
      xEffect = xBias * positionStrength * 0.60     // Boost offbeats
  ELSE:
    // Moving toward downbeats
    IF positionStrength < 0:
      xEffect = -xBias * |positionStrength| * 0.60  // Boost downbeats
    ELSE:
      xEffect = xBias * positionStrength * 0.45     // Suppress offbeats

  weight[step] = clamp(baseWeight[step] + xEffect, 0.05, 1.0)
```

### 6.2 AXIS Y: Intricacy

```
yBias = (axisY - 0.5) * 2.0  // [-1.0, 1.0]

FOR each step:
  isWeakPosition = GetMetricWeight(step) < 0.5

  IF yBias > 0:
    yEffect = yBias * (isWeakPosition ? 0.50 : 0.15)   // Boost weak positions
  ELSE:
    yEffect = yBias * (isWeakPosition ? 0.50 : -0.25)  // Suppress weak positions

  weight[step] = clamp(weight[step] + yEffect, 0.05, 1.0)
```

### 6.3 Broken Mode (High SHAPE + High AXIS X)

When SHAPE > 60% AND AXIS X > 70%, enter "broken" mode:

```
brokenIntensity = (shape - 0.6) * 2.5 * (axisX - 0.7) * 3.33

FOR step IN downbeatPositions:
  IF hash(seed ^ 0xDEADBEEF, step) < brokenIntensity * 0.6:
    weight[step] *= 0.25  // Suppress some downbeats completely
```

---

## 7. Voice Relationship (COMPLEMENT)

Voice 2 (shimmer) fills gaps in Voice 1 (anchor) pattern. DRIFT controls placement variation.

### 7.1 Gap Finding

```
gaps = FindGaps(anchorMask, patternLength)

// Handle wrap-around: combine tail+head if both are gaps
IF gaps[0].start == 0 AND gaps[last].end == patternLength:
  CombineWrapAroundGaps(gaps)
```

### 7.2 Shimmer Placement by DRIFT

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

## 8. ACCENT Velocity

ACCENT (Config K4) controls velocity range based on metric position.

### 8.1 Velocity Computation

```
metricWeight = GetMetricWeight(step)  // 0.0-1.0

// Velocity range scales with ACCENT
velocityFloor = 0.80 - accent * 0.50    // 80% → 30%
velocityCeiling = 0.88 + accent * 0.12  // 88% → 100%

// Map metric weight to velocity
velocity = velocityFloor + metricWeight * (velocityCeiling - velocityFloor)

// Micro-variation for human feel
variation = 0.02 + accent * 0.05
velocity += (hash(seed, step) - 0.5) * variation

RETURN clamp(velocity, 0.30, 1.0)
```

### 8.2 ACCENT Effect

| ACCENT | Velocity Floor | Velocity Ceiling | Feel |
|--------|----------------|------------------|------|
| 0% | 80% | 88% | Flat, machine-like |
| 50% | 55% | 94% | Normal dynamics |
| 100% | 30% | 100% | Extreme ghost/accent contrast |

---

## 9. Fill System

### 9.1 Fill Triggering

| Trigger | Behavior |
|---------|----------|
| Button tap (20-500ms) | Queue fill for next phrase |
| Fill CV (>1V) | Immediate fill |

### 9.2 Fill Modifiers

```
IF fillActive:
  // Exponential density curve
  maxBoost = 0.6 + energy * 0.4
  densityMultiplier = 1.0 + maxBoost * (fillProgress^2)

  // Velocity boost
  velocityBoost = 0.10 + 0.15 * fillProgress

  // Accent probability ramp
  accentProbability = 0.50 + 0.50 * fillProgress
  forceAccents = (fillProgress > 0.85)

  // Eligibility expansion
  expandEligibility = (fillProgress > 0.5)
```

---

## 10. AUX Output System

### 10.1 AUX Modes

| Mode | Signal | Description |
|------|--------|-------------|
| **FILL GATE** (default) | Gate | HIGH during fill duration |
| **HAT** (secret) | Trigger | Pattern-aware hat burst during fill |

### 10.2 Hat Burst Generation

```
// Determine trigger count (2-12)
triggerCount = max(1, round(2 + energy * 10))

FOR i = 0 TO triggerCount - 1:
  IF burst.count >= 12: BREAK

  IF shape < 0.3:
    step = (i * fillDuration) / triggerCount  // Evenly spaced
  ELSE IF shape < 0.7:
    step = EuclideanWithJitter(i, fillDuration, shape, seed)
  ELSE:
    step = RandomStep(fillDuration, seed, i)

  // Collision detection: nudge to nearest empty
  IF step IN usedSteps:
    step = FindNearestEmpty(step, fillDuration, usedSteps)

  burst.triggers[burst.count++] = step

// Velocity ducking near main hits
FOR each trigger:
  nearMainHit = CheckProximity(trigger.step, mainPattern, 1)
  baseVelocity = 0.65 + 0.35 * energy
  trigger.velocity = nearMainHit ? baseVelocity * 0.30 : baseVelocity
```

### 10.3 Hat Burst Data Structure

```cpp
struct HatBurst {
    struct Trigger { uint8_t step; float velocity; };
    Trigger triggers[12];  // Pre-allocated, no heap
    uint8_t count, fillStart, fillDuration, _pad;
};
```

---

## 11. LED Feedback System

### 11.1 5-Layer Priority System

| Layer | Priority | State | Behavior |
|-------|----------|-------|----------|
| 1 | Base | Idle (perf) | Gentle breath synced to clock |
| 1 | Base | Idle (config) | Slower breath |
| 2 | Additive | Clock sync | Subtle pulse on beats |
| 3 | Maximum | Trigger activity | Pulse on hits, envelope decay |
| 4 | Maximum | Fill active | Accelerating strobe + trigger overlay |
| 5 | Replace | Reseed progress | Building pulse (1-5Hz over 3s) |
| 5 | Replace | Reseed confirm | POP POP POP (3 flashes) |
| 5 | Replace | Mode switch | Quick signature |
| 5 | Replace | AUX mode unlock | Triple rising flash |
| 5 | Replace | AUX mode reset | Single fade |

### 11.2 Layer Composition

```
finalBrightness = max(
  baseBrightness,
  clockPulse,      // Layer 2 additive
  triggerPulse,    // Layer 3 maximum
  fillStrobe       // Layer 4 maximum
)

IF layer5Active:
  finalBrightness = layer5Value  // Layer 5 replaces all
```

---

## 12. Boot Behavior

### 12.1 Boot Defaults

All parameters reset to musical defaults on power-on:

| Parameter | Default | Rationale |
|-----------|---------|-----------|
| ENERGY | 50% | Neutral density |
| SHAPE | 30% | Humanized euclidean zone |
| AXIS X | 50% | Neutral beat position |
| AXIS Y | 50% | Moderate intricacy |
| CLOCK DIV | ×1 | No division |
| SWING | 50% | Neutral |
| DRIFT | 0% | Locked pattern |
| ACCENT | 50% | Normal dynamics |
| AUX MODE | FILL GATE | Default (no HAT) |
| PATTERN LENGTH | 16 | Compile-time default |

### 12.2 Boot-Time AUX Mode Selection

Optional gesture during power-on:
- Hold button during power-on
- Switch UP → HAT mode (triple rising flash)
- Switch DOWN → FILL GATE mode (single fade)

AUX mode is **volatile** (not persisted across power cycles).

### 12.3 Performance Knob Reading

On boot, performance knobs (K1-K4) are read from hardware immediately:
- No soft takeover needed
- Values apply directly
- Provides immediate playability

---

## 13. Clock and Reset

### 13.1 Clock Source Selection

| Condition | Clock Source | Behavior |
|-----------|--------------|----------|
| Gate In 1 unpatched | Internal Metro | Steps advance on internal clock |
| Gate In 1 patched | External only | Internal clock disabled |

External clock is **exclusive**: no timeout fallback, no parallel operation.

### 13.2 Reset Behavior

Reset (Gate In 2) always resets to step 0. Pattern regeneration occurs at bar boundaries.

---

## 14. Testing Requirements

### 14.1 Unit Tests

| Test Area | Tests Required |
|-----------|----------------|
| **SHAPE Blending** | Zone transitions, crossfade smoothness, determinism |
| **AXIS Biasing** | Bidirectional effect, no dead zones, broken mode |
| **COMPLEMENT** | Gap finding, wrap-around handling, DRIFT placement |
| **ACCENT Velocity** | Range scaling, metric weight mapping, micro-variation |
| **Hat Burst** | Pre-allocation, collision detection, velocity ducking |
| **LED Layers** | Priority ordering, layer composition |

### 14.2 Integration Tests

| Test | Description |
|------|-------------|
| **Full Bar Generation** | Generate 100 bars, verify hit counts and patterns |
| **SHAPE Sweep** | Sweep 0-100%, verify smooth character transitions |
| **AXIS Navigation** | Full X/Y range, verify continuous effect |
| **Fill Behavior** | Button tap, CV trigger, AUX output |
| **Boot Defaults** | Power cycle, verify all parameters reset |

### 14.3 Musical Validation

| Settings | Expected Outcome |
|----------|------------------|
| ENERGY=45%, SHAPE=25% | Driving techno, stable groove |
| ENERGY=55%, SHAPE=50%, AXIS X=65% | Groovy funk, syncopated |
| ENERGY=65%, SHAPE=60%, AXIS X=75% | Broken beat mode |
| ENERGY=85%, SHAPE=90% | IDM chaos |
| DRIFT=0% | Identical pattern every phrase |
| DRIFT=100% | Varied placement, same density feel |

---

## Appendix: Style Preset Map

### Performance Mode Settings

| Style | ENERGY | SHAPE | AXIS X | AXIS Y |
|-------|--------|-------|--------|--------|
| Minimal Techno | 20% | 10% | 20% | 20% |
| Driving Techno | 45% | 25% | 30% | 40% |
| Groovy/Funk | 55% | 50% | 65% | 55% |
| Broken Beat | 65% | 60% | 75% | 70% |
| Tribal/Poly | 70% | 45% | 60% | 80% |
| IDM/Chaos | 85% | 90% | 85% | 90% |

### Config Mode Settings

| Style | CLOCK DIV | SWING | DRIFT | ACCENT |
|-------|-----------|-------|-------|--------|
| Minimal Techno | 50% | 20% | 10% | 70% |
| Driving Techno | 50% | 30% | 20% | 75% |
| Groovy/Funk | 50% | 60% | 40% | 65% |
| Broken Beat | 50% | 45% | 55% | 60% |
| IDM/Chaos | Varies | Varies | 80% | 85% |

---

## Appendix: Glossary

| Term | Definition |
|------|------------|
| **Voice 1 / Anchor** | Primary voice, typically kick-like role |
| **Voice 2 / Shimmer** | Secondary voice, fills gaps in Voice 1 |
| **COMPLEMENT** | Voice 2 gap-filling relationship |
| **SHAPE** | Pattern character (stable → syncopated → wild) |
| **AXIS X** | Beat position control (downbeats ↔ offbeats) |
| **AXIS Y** | Intricacy control (simple ↔ complex) |
| **ACCENT** | Velocity range (flat → dynamic) |
| **DRIFT** | Pattern evolution rate (locked → varies each phrase) |
| **Hat Burst** | Pattern-aware fill triggers on AUX output |
| **Broken Mode** | High SHAPE + high AXIS X interaction zone |

---

*End of Specification*
