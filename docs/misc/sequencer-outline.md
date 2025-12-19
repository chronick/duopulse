# DuoPulse Sequencer Algorithm Comparison

This document outlines the two alternative drum sequencing algorithms used in the DuoPulse firmware: the **v2 2-Channel Grid Approach** and the **v3 3-Channel Algorithmic Approach**.

---

## Overview

| Aspect | v2 (Grid/Pattern Lookup) | v3 (Algorithmic Pulse Field) |
|--------|--------------------------|------------------------------|
| Pattern Source | 16 hand-crafted skeleton patterns | Continuous weighted probability algorithm |
| Genre Control | TERRAIN + GRID (can mismatch) | Single BROKEN parameter (genre emerges) |
| Variation | FLUX (adds chaos) | DRIFT (locked ↔ evolving) |
| Voice Relationship | ORBIT (3 discrete modes) | COUPLE (0-100% continuous) |
| Determinism | Not built-in | DRIFT=0 = identical every loop |
| Transitions | Abrupt pattern changes | Continuous morphing |

---

## Part 1: v2 2-Channel Grid Approach

### Architecture

The v2 system uses **lookup tables** containing 16 pre-designed pattern "skeletons" combined with **algorithmic layers** for ghost notes, bursts, and variations.

```
┌─────────────────────────────────────────────────────────────┐
│                    Pattern Selection                         │
│  ┌─────────┐    ┌─────────┐    ┌─────────────────────────┐  │
│  │ TERRAIN │───►│  Genre  │───►│  Pattern Index (0-15)   │  │
│  │  (0-1)  │    │  Zone   │    │  via GRID parameter     │  │
│  └─────────┘    └─────────┘    └─────────────────────────┘  │
│                      │                      │                │
│                      ▼                      ▼                │
│              ┌───────────────────────────────────┐          │
│              │     PatternSkeleton Lookup        │          │
│              │   (16 patterns × 32 steps × 2     │          │
│              │    voices × 4-bit intensity)      │          │
│              └───────────────────────────────────┘          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  Per-Step Processing                         │
│                                                              │
│  For each step:                                              │
│    1. Get intensity from skeleton (0-15)                     │
│    2. Compare to density threshold                           │
│    3. Apply FLUX probabilistic variation                     │
│    4. Apply FUSE energy balance                              │
│    5. Apply ORBIT voice relationship                         │
│    6. Output: anchorFires, shimmerFires, velocities          │
└─────────────────────────────────────────────────────────────┘
```

### Data Structure

Each pattern skeleton stores 32 steps with 4-bit intensity values per voice:

```cpp
struct PatternSkeleton {
    // 32-step skeleton for anchor voice
    // Packed: 2 steps per byte (high nibble = even, low nibble = odd)
    uint8_t anchorIntensity[16];   // 16 bytes = 32 × 4-bit values
    
    // 32-step skeleton for shimmer voice
    uint8_t shimmerIntensity[16];
    
    // Accent eligibility mask (bit N = step N can accent)
    uint32_t accentMask;
    
    // Default voice relationship (Interlock/Free/Shadow)
    uint8_t relationship;
    
    // Genre suitability bitfield
    uint8_t genreAffinity;
};
```

### Intensity Levels

| Value Range | Level | Behavior |
|-------------|-------|----------|
| 0 | Off | Never fires |
| 1-4 | Ghost | Fires only at high density |
| 5-10 | Normal | Fires at medium density |
| 11-15 | Strong | Fires at low density, accent candidate |

### The Density Threshold Algorithm

```cpp
bool ShouldStepFire(const uint8_t* skeleton, int step, float density) {
    uint8_t intensity = GetStepIntensity(skeleton, step);
    if (intensity == 0) return false;
    
    // Convert density (0-1) to threshold (15-0)
    // High density = low threshold = more steps fire
    int threshold = (int)((1.0f - density) * 15.0f);
    
    return intensity > threshold;
}
```

**Visual representation of density sweep:**

```
Intensity:   15 ████████████████
             14 ███████████████
             13 ██████████████
             12 █████████████
             11 ████████████
             10 ███████████         ← Normal range
              9 ██████████
              8 █████████
              7 ████████
              6 ███████
              5 ██████              ← Ghost range
              4 █████
              3 ████
              2 ███
              1 ██
              0 (never fires)
                ──────────────────────────────►
                Low ← DENSITY → High
```

### Pattern Library (16 Patterns)

| Index | Name | Genre | Character |
|-------|------|-------|-----------|
| 0 | Techno Four-on-Floor | Techno | Classic kick on quarters, 8th hats |
| 1 | Techno Minimal | Techno | Sparse, hypnotic |
| 2 | Techno Driving | Techno | Relentless 16th hats |
| 3 | Techno Pounding | Techno | Industrial, double kicks |
| 4 | Tribal Clave | Tribal | 3-2 clave rhythm |
| 5 | Tribal Interlocking | Tribal | Perfect call-response |
| 6 | Tribal Polyrhythmic | Tribal | 3-against-4 feel |
| 7 | Tribal Circular | Tribal | Hypnotic rotation |
| 8 | Trip-Hop Sparse | Trip-Hop | Heavy, minimal |
| 9 | Trip-Hop Lazy | Trip-Hop | Behind-the-beat ghosts |
| 10 | Trip-Hop Heavy | Trip-Hop | Crushing weight |
| 11 | Trip-Hop Groove | Trip-Hop | Hip-hop influenced |
| 12 | IDM Broken | IDM | Fragmented, glitchy |
| 13 | IDM Glitch | IDM | Micro-edits, stutters |
| 14 | IDM Irregular | IDM | Unpredictable placement |
| 15 | IDM Chaos | IDM | Maximum complexity |

### FLUX Variation Layer

FLUX adds probabilistic variation on top of the skeleton:

```cpp
void ApplyFlux(float flux, StepResult& result) {
    // Ghost note probability (up to 50% at max flux)
    float ghostChance = flux * 0.5f;
    
    // Fill chance at phrase boundaries (up to 30% at max flux)
    float fillChance = flux * 0.3f;
    
    // Velocity jitter (up to ±20%)
    float velocityJitter = flux * 0.2f;
    
    // Apply probabilistic variations...
}
```

### ORBIT Voice Relationship Modes

```
┌─────────────────────────────────────────────────────────┐
│  INTERLOCK (0-33%)                                      │
│  ┌─────────────────────────────────────────────────┐    │
│  │ Anchor:  ●───────●───────●───────●──────────    │    │
│  │ Shimmer: ────●───────●───────●───────●────      │    │
│  │          (fills gaps, call-response)            │    │
│  └─────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────┤
│  FREE (33-67%)                                          │
│  ┌─────────────────────────────────────────────────┐    │
│  │ Anchor:  ●───●─────●───────●────────●──────     │    │
│  │ Shimmer: ──●─────●───●───────●────●───────      │    │
│  │          (independent, can overlap)              │    │
│  └─────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────┤
│  SHADOW (67-100%)                                       │
│  ┌─────────────────────────────────────────────────┐    │
│  │ Anchor:  ●───────●───────●───────●──────────    │    │
│  │ Shimmer: ─●───────●───────●───────●─────────    │    │
│  │          (1-step delayed echo at 70% velocity)   │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

### Genre-Aware Swing

Swing is configured per-genre via TERRAIN:

| Terrain Range | Genre | Swing Range |
|---------------|-------|-------------|
| 0-25% | Techno | 52-57% (nearly straight) |
| 25-50% | Tribal | 56-62% (shuffled) |
| 50-75% | Trip-Hop | 60-68% (lazy) |
| 75-100% | IDM | 54-65% + jitter |

### v2 Limitations

1. **Terrain/Grid Mismatch**: A Tribal pattern can play with Techno swing
2. **Discrete Transitions**: Pattern changes are abrupt
3. **Unpredictable Combinations**: 16 patterns × density × flux × fuse = complex interactions
4. **No Determinism**: Cannot guarantee identical patterns across loops

---

## Part 2: v3 3-Channel Algorithmic Approach

### Architecture

The v3 system replaces discrete patterns with a **continuous weighted probability algorithm** controlled by four primary axes:

```
┌─────────────────────────────────────────────────────────────┐
│                     Control Inputs                           │
│                                                              │
│    DENSITY           BROKEN           DRIFT                  │
│    "How much"        "What shape"     "Does it change"       │
│       │                  │                │                  │
│       ▼                  ▼                ▼                  │
│    ┌──────────────────────────────────────────────────────┐ │
│    │              Weighted Pulse Field                     │ │
│    │                                                       │ │
│    │  ┌──────────┐    ┌──────────┐    ┌──────────┐        │ │
│    │  │ Anchor   │    │ BROKEN   │    │ DRIFT    │        │ │
│    │  │ Weights  │───►│ Flatten  │───►│ Seed     │        │ │
│    │  │ (32)     │    │ + Noise  │    │ Select   │        │ │
│    │  └──────────┘    └──────────┘    └──────────┘        │ │
│    │                                                       │ │
│    │  ┌──────────┐    ┌──────────┐    ┌──────────┐        │ │
│    │  │ Shimmer  │    │ BROKEN   │    │ DRIFT    │        │ │
│    │  │ Weights  │───►│ Flatten  │───►│ Seed     │        │ │
│    │  │ (32)     │    │ + Noise  │    │ Select   │        │ │
│    │  └──────────┘    └──────────┘    └──────────┘        │ │
│    └──────────────────────────────────────────────────────┘ │
│                            │                                 │
│                            ▼                                 │
│    ┌──────────────────────────────────────────────────────┐ │
│    │              BROKEN Effects Stack                     │ │
│    │  • Swing (tied to BROKEN)                            │ │
│    │  • Micro-timing jitter                               │ │
│    │  • Step displacement                                  │ │
│    │  • Velocity variation                                 │ │
│    └──────────────────────────────────────────────────────┘ │
│                            │                                 │
│                            ▼                                 │
│    ┌──────────────────────────────────────────────────────┐ │
│    │              Voice Interaction                        │ │
│    │  • FUSE (energy balance)                             │ │
│    │  • COUPLE (interlock strength)                       │ │
│    └──────────────────────────────────────────────────────┘ │
│                            │                                 │
│                            ▼                                 │
│              Output: anchorFires, shimmerFires               │
└─────────────────────────────────────────────────────────────┘
```

### Core Mental Model

```
                        BROKEN
              0%                      100%
              (Straight)              (Chaos)
              ┌────────────────────────────┐
        100%  │ Full 16ths,      Full 16ths,   │
              │ metronomic       scattered      │
              │                                 │
   D          │                                 │
   E     50%  │ Classic 4/4      Syncopated,   │
   N          │ kick-snare       broken         │
   S          │                                 │
   I          │                                 │
   T     0%   │ SILENCE          SILENCE        │
   Y          └────────────────────────────────┘
```

**The Reference Point:** At BROKEN=0, DRIFT=0, DENSITIES at 50%:
> **Classic 4/4 kick with snare on backbeat, repeated identically forever.**

### Weight Tables

Each step has a weight (0.0-1.0) based on musical importance:

```cpp
// Anchor (Kick Character) - emphasizes downbeats
constexpr float kAnchorWeights[32] = {
    // Bar 1: Steps 0-15
    1.00f, 0.15f, 0.30f, 0.15f,  // 0-3:  DOWNBEAT, ghost, 8th, ghost
    0.70f, 0.15f, 0.30f, 0.15f,  // 4-7:  quarter, ghost, 8th, ghost
    0.85f, 0.15f, 0.30f, 0.15f,  // 8-11: half, ghost, 8th, ghost
    0.70f, 0.15f, 0.30f, 0.20f,  // 12-15: quarter
    
    // Bar 2: Steps 16-31
    1.00f, 0.15f, 0.30f, 0.15f,  // 16-19: DOWNBEAT
    0.70f, 0.15f, 0.30f, 0.15f,  // 20-23: quarter
    0.85f, 0.15f, 0.30f, 0.15f,  // 24-27: half
    0.70f, 0.15f, 0.35f, 0.25f   // 28-31: quarter (fill zone boost)
};

// Shimmer (Snare Character) - emphasizes backbeats
constexpr float kShimmerWeights[32] = {
    // Bar 1: Steps 0-15
    0.25f, 0.15f, 0.35f, 0.15f,  // 0-3:  low downbeat
    0.60f, 0.15f, 0.35f, 0.20f,  // 4-7:  pre-snare
    1.00f, 0.15f, 0.35f, 0.15f,  // 8-11: BACKBEAT (snare!)
    0.60f, 0.15f, 0.35f, 0.20f,  // 12-15: quarter
    
    // Bar 2: Steps 16-31
    0.25f, 0.15f, 0.35f, 0.15f,  // 16-19: low downbeat
    0.60f, 0.15f, 0.35f, 0.20f,  // 20-23: pre-snare
    1.00f, 0.15f, 0.35f, 0.15f,  // 24-27: BACKBEAT (snare!)
    0.60f, 0.15f, 0.40f, 0.30f   // 28-31: fill zone boost
};
```

**Visual representation of weight profiles:**

```
Anchor Weights (Kick):
Step:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
       █                    █           █
       █     █     █     █  █     █     █     █     █
       ▓  ░  ▓  ░  ▓  ░  ▓  ▓  ░  ▓  ░  ▓  ░  ▓  ░

Shimmer Weights (Snare):
Step:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
                             █
       ░     ▓     █     █   █     ▓     █     ▓     █
       ░  ░  ▓  ░  ▓  ░  ▓  ▓  ░  ▓  ░  ▓  ░  ▓  ░

Legend: █ = 1.0 (strong), ▓ = 0.3-0.7 (medium), ░ = 0.15-0.25 (ghost)
```

### The Core Algorithm

```cpp
bool ShouldStepFire(int step, float density, float broken, 
                    const float* weights, uint32_t seed) {
    // ============================================
    // CRITICAL: DENSITY=0 IS ABSOLUTE SILENCE
    // ============================================
    if (density <= 0.0f) {
        return false;  // Absolute floor
    }
    
    // 1. Get base weight for this step position
    float baseWeight = weights[step];
    
    // 2. BROKEN flattens the weight distribution
    //    At broken=0: full differentiation (downbeats dominate)
    //    At broken=1: weights converge to 0.5 (equal probability)
    float effectiveWeight = Lerp(baseWeight, 0.5f, broken);
    
    // 3. Add randomness scaled by BROKEN
    float noise = (HashToFloat(HashStep(seed, step)) - 0.5f) * broken * 0.4f;
    effectiveWeight += noise;
    effectiveWeight = Clamp(effectiveWeight, 0.0f, 1.0f);
    
    // 4. DENSITY sets the threshold
    float threshold = 1.0f - density;
    
    // 5. Fire if weight exceeds threshold
    return effectiveWeight > threshold;
}
```

### BROKEN Effects Stack

BROKEN doesn't just flatten weights—it adds a coordinated stack of timing effects:

```
BROKEN Parameter
       │
       ├──► Swing (genre-appropriate)
       │    0-25%: Techno (50-54%)
       │    25-50%: Tribal (54-60%)
       │    50-75%: Trip-Hop (60-66%)
       │    75-100%: IDM (66-58% + jitter)
       │
       ├──► Micro-Timing Jitter
       │    0-40%: ±0ms (machine-tight)
       │    40-70%: ±3ms (subtle human)
       │    70-90%: ±6ms (loose)
       │    90-100%: ±12ms (broken)
       │
       ├──► Step Displacement
       │    0-50%: 0% chance, 0 steps
       │    50-75%: 0-15% chance, ±1 step
       │    75-100%: 15-40% chance, ±2 steps
       │
       └──► Velocity Variation
            0-30%: ±5% (mechanical)
            30-60%: ±10% (subtle)
            60-100%: ±20% (expressive)
```

### DRIFT: The Pattern Evolution System

DRIFT determines how much the pattern changes over time using a **dual-seed system**:

```
┌─────────────────────────────────────────────────────────────┐
│                    DRIFT System                              │
│                                                              │
│  ┌──────────────────┐    ┌──────────────────┐               │
│  │  patternSeed_    │    │   loopSeed_      │               │
│  │  (fixed)         │    │   (changes each  │               │
│  │                  │    │    phrase)       │               │
│  └────────┬─────────┘    └────────┬─────────┘               │
│           │                       │                          │
│           ▼                       ▼                          │
│  ┌────────────────────────────────────────────┐             │
│  │         Step Stability Check               │             │
│  │                                            │             │
│  │  if (stepStability > effectiveDrift) {    │             │
│  │      use patternSeed_ (LOCKED)            │             │
│  │  } else {                                 │             │
│  │      use loopSeed_ (DRIFTING)             │             │
│  │  }                                        │             │
│  └────────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────────┘
```

**Step Stability Values:**

| Position | Stability | Locks Until |
|----------|-----------|-------------|
| Bar downbeats (0, 16) | 1.0 | DRIFT > 100% |
| Half notes (8, 24) | 0.85 | DRIFT > 85% |
| Quarter notes (4, 12, 20, 28) | 0.7 | DRIFT > 70% |
| 8th off-beats | 0.4 | DRIFT > 40% |
| 16th ghosts | 0.2 | DRIFT > 20% |

**Per-Voice DRIFT Multipliers:**

```cpp
// Anchor is more stable (kicks stay grounded)
constexpr float kAnchorDriftMultiplier = 0.7f;

// Shimmer is more drifty (hats/snares vary more)
constexpr float kShimmerDriftMultiplier = 1.3f;
```

| DRIFT Knob | Anchor Effective | Shimmer Effective |
|------------|------------------|-------------------|
| 0% | 0% | 0% |
| 30% | 21% | 39% |
| 50% | 35% | 65% |
| 70% | 49% | 91% |
| 100% | 70% | 100% |

### COUPLE: Continuous Voice Interlock

```cpp
void ApplyCouple(float couple, bool anchorFires, 
                 bool& shimmerFires, float& shimmerVel) {
    if (couple < 0.1f) return;  // Fully independent
    
    if (anchorFires) {
        // Suppress shimmer when anchor fires (collision avoidance)
        float suppressChance = couple * 0.8f;  // Up to 80%
        if (Random() < suppressChance) {
            shimmerFires = false;
        }
    } else {
        // Boost shimmer when anchor silent (gap-filling)
        if (!shimmerFires && couple > 0.5f) {
            float boostChance = (couple - 0.5f) * 0.6f;  // Up to 30%
            if (Random() < boostChance) {
                shimmerFires = true;
                shimmerVel = 0.5f + Random() * 0.3f;
            }
        }
    }
}
```

### Phrase-Aware Modulation

The algorithm is aware of phrase position for musical builds and fills:

```
Phrase Progress:
 0%                     40%      50%      75%                100%
 │                       │        │        │                   │
 │      GROOVE          │MID-FILL│ BUILD  │       FILL        │
 │  (stable pattern)    │        │        │   (max activity)  │
 │                       │        │        │                   │
 └───────────────────────┴────────┴────────┴───────────────────┘
```

---

## Side-by-Side Comparison

### Trigger Decision Flow

**v2 (Pattern Lookup):**
```
step → GetPatternSkeleton(grid) → GetIntensity(step) → 
    if intensity > densityThreshold:
        → ApplyFlux(probabilistic) → ApplyOrbit(3 modes) → trigger
```

**v3 (Algorithmic):**
```
step → GetWeight(step, voice) → FlattenByBroken → AddNoise → 
    → SelectSeed(drift, stability) → 
    if effectiveWeight > densityThreshold:
        → ApplyBrokenEffects → ApplyCouple → trigger
```

### Control Parameter Mapping

| Function | v2 | v3 |
|----------|----|----|
| Voice density | K1, K2: Anchor/Shimmer Density | K1, K2: Anchor/Shimmer Density |
| Pattern shape | K3+S: GRID (selects pattern 0-15) | K3: BROKEN (continuous shape) |
| Variation | K3: FLUX (probabilistic chaos) | K4: DRIFT (deterministic evolution) |
| Energy balance | K4: FUSE | K1+S: FUSE |
| Voice relationship | K4+S: ORBIT (3 modes) | K3+S: COUPLE (continuous) |
| Genre/swing | K1+S: TERRAIN | Emerges from BROKEN |

### Memory Usage

| Resource | v2 | v3 |
|----------|----|----|
| Pattern data | ~640 bytes (16 × 40-byte skeletons) | ~256 bytes (weight tables) |
| RAM per pattern | 40 bytes | N/A (computed) |
| Code complexity | Lower (lookup) | Higher (algorithm) |

### Musical Characteristics

| Aspect | v2 | v3 |
|--------|----|----|
| Pattern variety | 16 fixed skeletons | Infinite continuous space |
| Transitions | Discrete (pattern switch) | Continuous (parameter morph) |
| Repeatability | Probabilistic (FLUX) | Guaranteed (DRIFT=0) |
| Genre coherence | Can mismatch | Always coherent |
| Learning curve | Lower (preset patterns) | Higher (understand axes) |

---

## Conclusion

The v2 system provides **predictable, curated patterns** suitable for users who want immediate musical results without deep understanding. The v3 system provides a **continuous, coherent parameter space** where every knob turn produces a predictable, audible change—ideal for performance and deep exploration.

Both systems share the same hardware interface and voice architecture (Anchor/Shimmer), allowing the firmware to support both via compile-time selection (`#ifdef USE_PULSE_FIELD_V3`).

