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

## Part 3: v4 DuoPulse — 2D Pattern Field Approach

### Design Goals

v4 addresses limitations of both v2 and v3 while preserving their strengths:

| Problem | v2 Issue | v3 Issue | v4 Solution |
|---------|----------|----------|-------------|
| **Pattern variety** | Discrete, abrupt transitions | Too much variety, hard to control | 2D field with smooth morphing |
| **Musicality** | Good patterns but hard to navigate | Probability can produce bad patterns | Eligibility masks + guard rails |
| **Density control** | Unpredictable | Clumping/silence from coin flips | Hit budgets with weighted sampling |
| **Genre coherence** | Terrain/grid mismatch | Genre emerges unpredictably | GENRE selects timing profile; patterns use archetype grid |
| **Repeatability** | No determinism | DRIFT=0 works but fragile | Seed-controlled everything |
| **Playability** | Too many parameters | Controls don't map to intent | Ergonomic pairings (ENERGY/PUNCH, BUILD/GENRE) |

### Core Philosophy

1. **Musicality over flexibility**: Every output should be danceable/usable. No "probability soup."
2. **Playability**: Controls map directly to musical intent. ENERGY = tension. GENRE = feel.
3. **Deterministic variation**: Same settings + same seed = identical output. Variation is controlled, not random.
4. **Constraint-first generation**: Define what's *possible* (eligibility), then what's *probable* (weights).
5. **Hit budgets over coin flips**: Guarantee density matches intent; vary placement, not count.

---

### Architecture

v4 uses a **3-layer pipeline**: Control → Generation → Timing

```
┌─────────────────────────────────────────────────────────────┐
│                    CONTROL LAYER                             │
│                                                              │
│   ENERGY (K1)    BUILD (K2)    FIELD X (K3)    FIELD Y (K4) │
│   ↓ +Shift       ↓ +Shift      ↓ +Shift        ↓ +Shift     │
│   PUNCH          GENRE          DRIFT           BALANCE      │
│                                                              │
│   Energy → Zone → Hit Budget                                │
│   Field X/Y → Archetype Selection (3×3 grid per genre)      │
└──────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                  GENERATION LAYER                            │
│                                                              │
│   1. Compute hit budgets (from ENERGY + BALANCE + zone)     │
│   2. Compute eligibility mask (which steps CAN fire)        │
│   3. Blend archetype weights (from FIELD X/Y)               │
│   4. Select hits via Gumbel Top-K (deterministic, seeded)   │
│   5. Apply voice relationship (archetype-driven)            │
│   6. Soft repair pass (bias rescue steps)                   │
│   7. Hard guard rails (force corrections)                   │
└──────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    TIMING LAYER (BROKEN Stack)               │
│                                                              │
│   Swing → Microtiming Jitter → Step Displacement            │
│   (all bounded by GENRE, all deterministic per seed)        │
└──────────────────────────────────────────────────────────────┘
```

---

### 3×3 Pattern Field System

Each genre has a **3×3 grid of archetype patterns** (27 total across 3 genres):

```
              Y: COMPLEXITY (pattern density/intricacy)
                    ↑
                    │
           complex  │  [0,2]         [1,2]         [2,2]
              2     │  busy          polyrhythm    chaos
                    │  driving 16ths  3-vs-4       glitchy fills
                    │
                    │  [0,1]         [1,1]         [2,1]
              1     │  driving       groovy        broken
                    │  steady 8ths   swing pocket  displaced hits
                    │
           sparse   │  [0,0]         [1,0]         [2,0]
              0     │  minimal       steady        displaced
                    │  just kicks    basic groove  off-grid sparse
                    │
                    └──────────────────────────────────────→
                         0              1              2
                      straight     syncopated      broken
                                X: SYNCOPATION
```

**Archetype DNA Structure:**

Each archetype stores:
- **32-step weight tables** for Anchor, Shimmer, Aux (0.0-1.0 per step)
- **Accent eligibility masks** (which steps CAN accent, not which WILL)
- **Timing characteristics** (base swing, swing pattern type)
- **Voice relationship defaults** (suggested coupling value)
- **Fill behavior** (density multiplier, ratchet-eligible steps)

**Winner-Take-More Blending:**

When FIELD X/Y is between grid points, blend using softmax with temperature:
- Lower temperature = more winner-take-all behavior
- Preserves archetype character during transitions
- Continuous properties (weights, timing) interpolate smoothly
- Discrete properties (masks) use dominant archetype

---

### Generation Pipeline

#### 1. Hit Budget Calculation

**Purpose**: Guarantee density matches user intent, no randomness in hit count.

```cpp
// Energy → Zone → Budget multipliers
Zone GetEnergyZone(float energy) {
    if (energy < 0.20f) return MINIMAL;  // Sparse, skeleton only
    if (energy < 0.50f) return GROOVE;   // Stable, danceable
    if (energy < 0.75f) return BUILD;    // Increasing activity
    return PEAK;                          // Maximum activity
}

// Budget calculation (simplified)
int anchorBudget = baseHits * energyMultiplier(zone);
int shimmerBudget = anchorBudget * balanceRatio;  // 0-150% of anchor
```

**Balance range** (extended in Task 21):
- 0% = shimmer silent
- 50% = shimmer 75% of anchor
- 100% = shimmer 150% of anchor (shimmer-heavy patterns)

#### 2. Eligibility Mask

**Purpose**: Define which steps *can* fire based on energy level.

```
Energy 0-20% (MINIMAL):   Only quarter notes eligible
Energy 20-50% (GROOVE):   Quarters + 8th offbeats
Energy 50-75% (BUILD):    Add some 16th subdivisions
Energy 75-100% (PEAK):    All 32 steps eligible
```

#### 3. Gumbel Top-K Selection

**Purpose**: Weighted sampling without replacement, deterministic and seeded.

```cpp
// For each eligible step:
//   1. Get weight from blended archetype
//   2. Add Gumbel noise: -log(-log(uniform_random))
//   3. Sort by (weight + noise)
//   4. Select top K steps (where K = hit budget)
//   5. Apply spacing rules to prevent clumping
```

**Euclidean Foundation** (Task 21, genre-dependent):
- Techno: 70% Euclidean ratio at Field X=0 (ensures four-on-floor)
- Tribal: 40% Euclidean ratio (balances structure with polyrhythm)
- IDM: 0% (disabled for maximum irregularity)
- Ratio reduced by Field X (at X=1.0, ratio ≈ 0.3× base)
- Only active in MINIMAL and GROOVE zones

#### 4. Voice Relationship

**Voice Coupling Config** (simplified in Task 22):
- **INDEPENDENT (0-50%)**: Voices fire freely, can overlap
- **SHADOW (50-100%)**: Shimmer echoes anchor with 1-step delay

**Note**: INTERLOCK mode removed in Task 22 due to broken 1:1 trigger behavior.

#### 5. Guard Rails

**Soft Repair Pass**:
- If constraints nearly violated, bias rescue steps
- Swap weakest hit for strongest rescue candidate
- No change to total hit count

**Hard Guard Rails** (force corrections only if still violating):
- Downbeat protection (force anchor on beat 1 if missing)
- Max gap rule (no more than 8 steps without anchor in GROOVE+)
- Max consecutive shimmer (4 unless PEAK zone)
- Genre-specific floors (e.g., Techno backbeat)

#### 6. DRIFT Seed System

**Purpose**: Control pattern evolution while preserving downbeat stability.

```
Two-seed system:
- patternSeed: Fixed per "song", changes only on reseed
- phraseSeed: Changes each phrase, derived from patternSeed + counter

Step stability determines which seed is used:
- Bar downbeats (0, 16): stability = 1.0 (locks until DRIFT > 100%)
- Half notes (8, 24): stability = 0.85 (locks until DRIFT > 85%)
- Quarter notes: stability = 0.7 (locks until DRIFT > 70%)
- 8th off-beats: stability = 0.4
- 16th ghosts: stability = 0.2

Per-voice multipliers:
- Anchor: 0.7× (kicks stay grounded)
- Shimmer: 1.3× (hats/snares vary more)
```

---

### Control System

v4 introduces **ergonomic pairings**: each knob domain has primary (CV-able) and shift controls.

#### Performance Mode — Ergonomic Pairings

| Knob | Domain | Primary (CV-able) | +Shift |
|------|--------|-------------------|--------|
| K1 | **Intensity** | ENERGY (hit density) | PUNCH (velocity dynamics) |
| K2 | **Drama** | BUILD (phrase arc) | GENRE (style bank) |
| K3 | **Pattern** | FIELD X (syncopation) | DRIFT (evolution rate) |
| K4 | **Texture** | FIELD Y (complexity) | BALANCE (voice ratio) |

**PUNCH Parameter** (Velocity Dynamics):
```
PUNCH = 0%:   Flat dynamics. All hits similar velocity. Hypnotic.
              ●●●●●●●● (all same intensity)

PUNCH = 50%:  Normal dynamics. Standard groove with natural accents.
              ●○●○●○●○ (accented and normal hits)

PUNCH = 100%: Maximum dynamics. Accents POP, ghosts nearly silent.
              ●  ● ●  ● (huge velocity differences)
```

Velocity ranges (widened in Task 21):
- velocityFloor: 65% down to 30% (was 65%-55% in v4.0)
- accentBoost: +15% to +45% (was +15% to +35%)
- Minimum output velocity: 30% (ensures VCA audibility)

**BUILD Parameter** (Phrase Dynamics):
```
BUILD = 0%:   Flat throughout. No builds, no fills.
              ████████████████ (constant energy)

BUILD = 50%:  Subtle build. Slight density increase, fills at phrase end.
              ████████████▲▲▲▲ (gentle rise)

BUILD = 100%: Dramatic arc. Big builds, intense fills, energy peaks.
              ████▲▲▲▲▲▲▲▲████ (tension → release)
```

BUILD operates in three phases (Task 21):
- **0-60% (GROOVE)**: Stable pattern, 1.0× density, normal velocity
- **60-87.5% (BUILD)**: Ramping energy, +0-35% density, +0-8% velocity floor
- **87.5-100% (FILL)**: All accents (if BUILD>60%), +35-50% density, +8-12% floor

#### Config Mode — Domain-Based

| Knob | Domain | Primary | +Shift |
|------|--------|---------|--------|
| K1 | **Grid** | PATTERN LENGTH (16/24/32/64) | ~~PHRASE LENGTH~~ (auto-derived in Task 22) |
| K2 | **Timing** | SWING (0-100%) | CLOCK DIV (÷8 to ×8) |
| K3 | **Output** | AUX MODE (Hat/Fill/Phrase/Event) | AUX DENSITY (50-200%) |
| K4 | **Behavior** | ~~RESET MODE~~ (hardcoded STEP in Task 22) | VOICE COUPLING (Ind/Shadow) |

**Phrase Length Auto-Derivation** (Task 22):

Phrase length is automatically derived to maintain ~128 steps per phrase:
- 16 steps → 8 bars = 128 steps
- 24 steps → 5 bars = 120 steps
- 32 steps → 4 bars = 128 steps
- 64 steps → 2 bars = 128 steps

---

### BROKEN Timing Stack

Timing controlled by **GENRE** (Performance+Shift K2) and **SWING** (Config K2):

| Layer | Control | Range | Zone Limit |
|-------|---------|-------|------------|
| **Swing** | Config K2 | 0-100% (straight → heavy triplet) | GENRE-dependent max |
| **Microtiming Jitter** | GENRE-based | ±0ms (Techno) to ±12ms (IDM) | Genre profiles |
| **Step Displacement** | GENRE-based | Never (Techno) to 40% chance (IDM) | Genre profiles |
| **Velocity Chaos** | PUNCH parameter | ±0% to ±25% variation | Always allowed |

**Note**: Audio In R (FLAVOR CV) removed in v4. Timing feel is now determined by GENRE selection and SWING config only.

**Genre-Aware Swing**:

Each genre has different swing characteristics:
- **Techno**: 52-57% (nearly straight, machine-like)
- **Tribal**: 56-62% (shuffled, polyrhythmic)
- **Trip-Hop**: 60-68% (lazy, behind-the-beat)
- **IDM**: 54-65% + jitter (irregular)

---

### Output System

| Output | Signal | Voltage Range | Behavior |
|--------|--------|---------------|----------|
| GATE_OUT_1 | Anchor trigger | 0V / 5V | Per step |
| GATE_OUT_2 | Shimmer trigger | 0V / 5V | Per step |
| OUT_L | Anchor velocity | 0-5V | Sample & hold (persists until next trig) |
| OUT_R | Shimmer velocity | 0-5V | Sample & hold (persists until next trig) |
| CV_OUT_1 | AUX output | 0-5V | Clock (if internal) OR mode-dependent |
| CV_OUT_2 | LED feedback | 0-5V | Visual feedback (brightness = activity) |

**AUX Modes**:
- **HAT**: Third trigger voice (ghost/hi-hat pattern)
- **FILL_GATE**: Gate high during fill zones
- **PHRASE_CV**: 0-5V ramp over phrase, resets at loop boundary
- **EVENT**: Trigger on "interesting" moments (accents, fills, section changes)

---

### Key Innovations vs v2/v3

| Feature | v2 | v3 | v4 |
|---------|----|----|-----|
| **Pattern Source** | 16 hand-crafted skeletons | Continuous weighted probability | 3×3 archetype grid per genre (27 total) |
| **Pattern Navigation** | GRID knob (discrete 0-15) | BROKEN (continuous) | FIELD X/Y (2D morphing) |
| **Density Control** | Threshold comparison | Probability per step | Hit budgets with Gumbel Top-K |
| **Voice Control** | ORBIT (3 discrete modes) | COUPLE (0-100% continuous) | BALANCE (0-150% ratio) + COUPLING (2 modes) |
| **Timing Control** | TERRAIN (genre mismatch) | BROKEN (pattern + timing) | GENRE (timing only) + SWING config |
| **Variation** | FLUX (probabilistic chaos) | DRIFT (locked ↔ evolving) | DRIFT (stratified stability per step) |
| **Determinism** | Not built-in | DRIFT=0 = identical every loop | Seed-controlled everything |
| **Transitions** | Abrupt pattern changes | Continuous morphing | Winner-take-more blending |
| **Control Philosophy** | Many independent params | Continuous axes | Ergonomic pairings (domain-based) |

---

### Post-Release Refinements (Tasks 21-24)

v4 shipped in Task 15 and was refined through hardware validation:

#### Task 21: Musicality Improvements (2025-12-31)

**Problem**: Patterns felt repetitive, velocity contrast insufficient, ghost notes not audible enough.

**Solutions**:
- **Archetype weight retuning**: Widened weight ranges (0.0-1.0 instead of clustering 0.5-0.7)
- **Velocity contrast widening**: velocityFloor 65%→30%, accentBoost +15%→+45%
- **Euclidean foundation layer**: Genre-dependent Euclidean blend at low Field X
- **BUILD phase refinement**: 3-phase system (GROOVE/BUILD/FILL) with coordinated density+velocity
- **Hit budget expansion**: Extended shimmer range to 0-150% of anchor

**Musical Impact**:
- Ghost notes now audible (30-50% velocity range)
- Accents pop dramatically (+45% boost at PUNCH=100%)
- Minimal patterns sound clean (Euclidean ensures even spacing)
- BUILD creates clear tension/release arc

#### Task 22: Control Simplification (2026-01-01)

**Problem**: Config mode had confusing/unnecessary controls from hardware validation.

**Changes**:
- **Removed Reset Mode** from Config K4 (hardcoded to STEP)
- **Auto-derived Phrase Length** from Pattern Length (~128 steps per phrase)
- **Extended Balance Range** to 0-150% (was 30-100%)
- **Removed INTERLOCK coupling** mode (broken 1:1 trigger behavior)
- **Freed controls**: Config K4 primary, Config+Shift K1 available for future features

#### Task 23: Immediate Field Updates (2026-01-01)

**Problem**: Field X/Y knob changes only affected pattern after reset; user expected immediate feedback.

**Solution**:
- Detect significant Field X/Y change (>10% threshold)
- Trigger regeneration at next **beat boundary** (not bar boundary)
- Maximum latency: 1 beat (4 steps) instead of 1 bar (16-64 steps)
- Debouncing prevents noise-triggered regeneration

**User Experience**: Knob adjustments now audible within 1 beat maximum.

#### Task 24: Power-On Behavior (2026-01-01)

**Problem**: Config persistence caused unpredictable boot state; user wanted fresh start.

**Solution**:
- **Nothing persists** across power cycles
- **Config settings** reset to musical defaults on boot
- **Performance knobs** (K1-K4) read from hardware immediately
- **Shift parameters** reset to defaults (PUNCH=50%, GENRE=Techno, DRIFT=0%, BALANCE=50%)

**Boot Defaults**:
- Pattern Length: 32 steps
- Swing: 50% (neutral)
- AUX Mode: HAT
- Voice Coupling: INDEPENDENT

**Benefit**: Module always boots in known-good, immediately playable state.

---

### Memory & Performance

| Resource | v2 | v3 | v4 |
|----------|----|----|-----|
| Pattern data | ~640 bytes (16 × 40-byte skeletons) | ~256 bytes (weight tables) | ~3.5 KB (27 archetypes × ~130 bytes) |
| RAM per pattern | 40 bytes | N/A (computed) | N/A (computed from grid blend) |
| Code complexity | Lower (lookup) | Higher (algorithm) | Highest (grid blending + pipeline) |
| Flash usage | ~85 KB | ~95 KB | ~118 KB (90.08% of 131 KB) |

---

### Voice Control Exploration (Task 21 Ideation, Unimplemented)

Task 21 explored deeper voice redesign ideas that were deferred:

**Hat-Centric Shimmer + Aux Redesign**:
- Repurpose Shimmer as **closed hi-hat** voice (velocity-driven)
- Repurpose Aux as **open/closed switch** (Gate high = open hat mode)
- Implement **choke logic** (open hat cuts short if closed follows)

**Three-Tier Velocity System**:
```cpp
enum class HitIntensity {
    ACCENT,      // 0.85-1.0 - downbeats, emphasized hits
    NORMAL,      // 0.55-0.80 - standard pattern hits
    GHOST        // 0.25-0.50 - groove/feel notes
};
```

**Rationale for Deferral**:
- Shimmer/Aux role change too breaking for existing users
- Requires rethinking all archetype data
- Task 21's balance range extension (0-150%) provides 80% of benefit
- Hat-centric design better suited for future hardware variant

**Future Consideration**: v5 could explore dedicated hat voices or user-configurable voice roles.

---

## Conclusion

The **v2 system** provides predictable, curated patterns suitable for users who want immediate musical results without deep understanding.

The **v3 system** provides a continuous, coherent parameter space where every knob turn produces a predictable, audible change—ideal for performance and deep exploration.

The **v4 system** combines the best of both: curated archetype patterns (like v2) navigable via a continuous 2D field (like v3), with hit budgets guaranteeing musicality and ergonomic controls mapping directly to musical intent. Post-release refinements (Tasks 21-24) improved velocity contrast, pattern variety, responsiveness, and user experience based on hardware validation feedback.

**Design Evolution Summary**:
- v2: Hand-crafted patterns, discrete selection → predictable but limited
- v3: Algorithmic generation, continuous parameters → flexible but chaotic
- v4: Archetype grids + hit budgets + guard rails → **musical, playable, and coherent**

