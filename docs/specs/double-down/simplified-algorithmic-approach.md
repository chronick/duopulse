# DuoPulse v3: Simplified Algorithmic Approach

**Specification Draft v0.2**  
**Date: 2025-12-17**

---

## Executive Summary

This document proposes a radical simplification of DuoPulse's rhythm generation system, replacing the 16 discrete pattern lookup tables with a continuous **algorithmic pulse field** controlled by four primary axes:

1. **DENSITY** — How many triggers fire (sparse → dense). **DENSITY=0 = absolute silence.**
2. **BROKEN** — Pattern structure/shape (4/4 techno → IDM chaos). Controls WHERE hits occur.
3. **DRIFT** — Pattern evolution over time (locked → generative). Controls WHETHER pattern repeats.
4. **RATCHET** — Fill intensity during phrase transitions. Controls fill energy.

This approach eliminates the Terrain/Grid mismatch problem, reduces cognitive load, and provides infinite variation along a musically coherent gradient from club-ready techno to experimental IDM.

---

## Design Philosophy

### The Problem with Discrete Patterns

The current v2 system has:
- 16 hand-crafted pattern skeletons
- TERRAIN control (genre: Techno/Tribal/Trip-Hop/IDM)
- GRID control (pattern selection within genre)

**Issues:**
1. Terrain and Grid can mismatch (Tribal pattern with Techno swing)
2. 16 patterns × density × flux × fuse = unpredictable combinations
3. Pattern transitions feel abrupt
4. Hard to "dial in" a specific feel

### The Algorithmic Solution

Replace discrete patterns with a **continuous algorithm** where:
- Every parameter change produces a **predictable, musical result**
- Genre character **emerges** from the BROKEN parameter
- Infinite variation within a coherent musical space
- No parameter mismatches possible

### Core Mental Model

```
DENSITY = "How much is happening"     (0 = silence, always)
BROKEN  = "What shape is the pattern" (4/4 → IDM)
DRIFT   = "How much it changes"       (locked → evolving)
RATCHET = "How intense are fills"     (subtle → ratcheted)
```

**The Reference Point:**
> If BROKEN=0, DRIFT=0, both DENSITIES at 50%: **classic 4/4 kick with snare on backbeat, repeated with zero variation.**

Together DENSITY and BROKEN span the rhythmic space:

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

**Critical Design Rule: DENSITY=0 is an absolute floor. Zero triggers, always, regardless of BROKEN, DRIFT, fills, or any other modulation.**

---

## Feature: Weighted Pulse Field Algorithm [pulse-field]

### Concept

Each of the 32 steps in a pattern has a **weight** that represents its "likelihood" of triggering. The algorithm uses these weights combined with DENSITY and BROKEN to determine what fires.

### Critical Design Rules

1. **DENSITY=0 is ABSOLUTE SILENCE.** No triggers fire, regardless of BROKEN, DRIFT, phrase position, or any other modulation. This must be enforced as a hard floor before any other calculations.

2. **BROKEN controls pattern STRUCTURE**, not density. At BROKEN=0, only high-weight positions fire. At BROKEN=100%, all positions have equal probability. But the TOTAL number of triggers is still controlled by DENSITY.

3. **Weights are normalized** such that at DENSITY=50%, BROKEN=0, you get a classic 4/4 pattern.

### Grid Position Weights

Steps are weighted by their musical importance in a standard 4/4 context:

| Step Positions | Weight | Musical Role |
|----------------|--------|--------------|
| 0, 16 | 1.0 | Bar downbeats ("THE ONE") |
| 8, 24 | 0.85 | Half-note positions |
| 4, 12, 20, 28 | 0.7 | Quarter notes |
| 2, 6, 10, 14, 18, 22, 26, 30 | 0.4 | 8th note off-beats |
| 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31 | 0.2 | 16th note off-beats |

### The Algorithm

```cpp
bool ShouldStepFire(int step, float density, float broken, const float* weights) {
    // ============================================
    // CRITICAL: DENSITY=0 IS ABSOLUTE SILENCE
    // ============================================
    // This check comes FIRST, before any other calculations.
    // No phrase boosts, no BROKEN noise, nothing can override this.
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
    //    More broken = more random variation in weights
    //    NOTE: This affects WHERE hits land, not how many (that's DENSITY)
    float noise = (Random() - 0.5f) * broken * 0.4f;
    effectiveWeight += noise;
    effectiveWeight = Clamp(effectiveWeight, 0.0f, 1.0f);
    
    // 4. DENSITY sets the threshold
    //    density=0 → threshold=1.0 (nothing fires) [handled above]
    //    density=1 → threshold=0.0 (everything fires)
    float threshold = 1.0f - density;
    
    // 5. Fire if weight exceeds threshold
    return effectiveWeight > threshold;
}
```

### How It Works

**At DENSITY = 0% (Silence):**
- **Zero triggers, always.** This is an absolute floor that cannot be overridden.

**At BROKEN = 0% (Straight):**
- Weights are sharply differentiated
- Only highest-weight positions fire at medium density
- Result: Regular, predictable patterns (classic 4/4)

**At BROKEN = 100% (Chaos):**
- All weights converge toward 0.5
- Any step is equally likely to fire
- Additional noise makes pattern structure irregular
- Result: Unpredictable, IDM-style patterns

**Density Sweeps:**
- At any BROKEN level, DENSITY controls how many steps fire
- Low density: Only the highest-weight steps
- High density: Nearly all steps

### Acceptance Criteria
- [x] Weight table for 32 steps implemented
- [x] Effective weight calculation with BROKEN flattening
- [x] Noise injection scaled by BROKEN
- [x] Threshold comparison with DENSITY
- [x] Deterministic mode (seeded RNG) for reproducibility
- [ ] **DENSITY=0 check is FIRST, as absolute floor**

---

## Feature: Voice Differentiation [voice-weights]

### Anchor and Shimmer Weight Profiles

Each voice has a different weight profile to create natural kick/snare differentiation:

#### Anchor (Kick Character)

Emphasizes downbeats and strong positions:

```cpp
constexpr float kAnchorWeights[32] = {
    // Bar 1: Steps 0-15
    1.00f, 0.15f, 0.30f, 0.15f,  // 0-3:  DOWNBEAT, ghost, 8th, ghost
    0.70f, 0.15f, 0.30f, 0.15f,  // 4-7:  quarter, ghost, 8th, ghost
    0.85f, 0.15f, 0.30f, 0.15f,  // 8-11: half, ghost, 8th, ghost
    0.70f, 0.15f, 0.30f, 0.20f,  // 12-15: quarter, ghost, 8th, ghost+
    
    // Bar 2: Steps 16-31
    1.00f, 0.15f, 0.30f, 0.15f,  // 16-19: DOWNBEAT
    0.70f, 0.15f, 0.30f, 0.15f,  // 20-23: quarter
    0.85f, 0.15f, 0.30f, 0.15f,  // 24-27: half
    0.70f, 0.15f, 0.35f, 0.25f   // 28-31: quarter (slight fill zone boost)
};
```

#### Shimmer (Snare Character)

Emphasizes backbeats (steps 8 and 24):

```cpp
constexpr float kShimmerWeights[32] = {
    // Bar 1: Steps 0-15
    0.25f, 0.15f, 0.35f, 0.15f,  // 0-3:  low downbeat, ghost, 8th, ghost
    0.60f, 0.15f, 0.35f, 0.20f,  // 4-7:  quarter (pre-snare), ghost, 8th, ghost
    1.00f, 0.15f, 0.35f, 0.15f,  // 8-11: BACKBEAT (snare!), ghost, 8th, ghost
    0.60f, 0.15f, 0.35f, 0.20f,  // 12-15: quarter, ghost, 8th, ghost
    
    // Bar 2: Steps 16-31
    0.25f, 0.15f, 0.35f, 0.15f,  // 16-19: low downbeat
    0.60f, 0.15f, 0.35f, 0.20f,  // 20-23: quarter (pre-snare)
    1.00f, 0.15f, 0.35f, 0.15f,  // 24-27: BACKBEAT (snare!)
    0.60f, 0.15f, 0.40f, 0.30f   // 28-31: quarter (fill zone boost)
};
```

### Natural Behavior

With these weight profiles:

| DENSITY | BROKEN | Anchor | Shimmer |
|---------|--------|--------|---------|
| **0%** | **any** | **SILENCE** | **SILENCE** |
| 50% | 0% | Kicks on 0, 8, 16, 24 | Snares on 8, 24 |
| 50% | 50% | Kicks shift around | Snares + off-beat hits |
| 50% | 100% | Random kick placement | Random snare placement |
| 100% | 0% | Full kick pattern, on-grid | Full snare pattern, on-grid |
| 100% | 100% | Maximum kick chaos | Maximum snare chaos |

**Critical:** DENSITY=0 for either voice produces **absolute silence** for that voice, regardless of BROKEN, DRIFT, fills, or any other modulation.

### Acceptance Criteria
- [x] Separate weight tables for Anchor and Shimmer
- [x] Anchor emphasizes downbeats (0, 8, 16, 24)
- [x] Shimmer emphasizes backbeats (8, 24)
- [x] Both voices respond identically to density/broken controls
- [x] Weight profiles can be tuned via constants
- [ ] **DENSITY=0 produces absolute silence (hard floor)**

---

## Feature: BROKEN Effects Stack [broken-effects]

The BROKEN parameter doesn't just flatten weights—it also progressively adds timing effects that contribute to genre character.

### Effect 1: Swing (Tied to BROKEN)

Swing is no longer a separate genre setting. It scales with BROKEN:

| BROKEN Range | Genre Feel | Swing % | Character |
|--------------|------------|---------|-----------|
| 0-25% | Techno | 50-54% | Nearly straight, driving |
| 25-50% | Tribal | 54-60% | Mild shuffle, groove |
| 50-75% | Trip-Hop | 60-66% | Lazy, behind-beat |
| 75-100% | IDM | 66-58% + jitter | Continuous from Trip-Hop, heavy jitter provides chaos |

```cpp
float GetSwingFromBroken(float broken) {
    if (broken < 0.25f) {
        // Techno: 50-54% (nearly straight)
        return Lerp(0.50f, 0.54f, broken * 4.0f);
    }
    else if (broken < 0.50f) {
        // Tribal: 54-60% (shuffled)
        return Lerp(0.54f, 0.60f, (broken - 0.25f) * 4.0f);
    }
    else if (broken < 0.75f) {
        // Trip-Hop: 60-66% (lazy)
        return Lerp(0.60f, 0.66f, (broken - 0.50f) * 4.0f);
    }
    else {
        // IDM: Variable swing combined with heavy jitter for "broken" feel
        // Continuity: start at 0.66 (where Trip-Hop ends), move toward 0.58
        // The micro-timing jitter (up to ±12ms) provides the chaos, not swing reduction
        float idmProgress = (broken - 0.75f) * 4.0f;  // 0.0 to 1.0
        float baseSwing = Lerp(0.66f, 0.58f, idmProgress);
        return baseSwing;
    }
}
```

### Effect 2: Micro-Timing Jitter

Humanize/jitter increases with BROKEN:

| BROKEN Range | Max Jitter | Feel |
|--------------|------------|------|
| 0-40% | ±0ms | Machine-tight |
| 40-70% | ±3ms | Subtle human feel |
| 70-90% | ±6ms | Loose, organic |
| 90-100% | ±12ms | Broken, glitchy |

```cpp
float GetJitterMsFromBroken(float broken) {
    if (broken < 0.4f) return 0.0f;
    if (broken < 0.7f) return Lerp(0.0f, 3.0f, (broken - 0.4f) / 0.3f);
    if (broken < 0.9f) return Lerp(3.0f, 6.0f, (broken - 0.7f) / 0.2f);
    return Lerp(6.0f, 12.0f, (broken - 0.9f) / 0.1f);
}
```

### Effect 3: Step Displacement

At higher BROKEN, triggers can shift to adjacent steps:

| BROKEN Range | Displacement Chance | Max Shift |
|--------------|---------------------|-----------|
| 0-50% | 0% | 0 steps |
| 50-75% | 0-15% | ±1 step |
| 75-100% | 15-40% | ±2 steps |

```cpp
int GetDisplacedStep(int step, float broken, uint32_t& rng) {
    if (broken < 0.5f) return step;
    
    float displaceChance = (broken - 0.5f) * 0.8f;  // 0-40% at max
    
    if (RandomFloat(rng) < displaceChance) {
        int maxShift = (broken > 0.75f) ? 2 : 1;
        int shift = RandomInt(rng, -maxShift, maxShift);
        return (step + shift + 32) % 32;  // Wrap around
    }
    
    return step;
}
```

### Effect 4: Velocity Variation

Velocity consistency decreases with BROKEN:

| BROKEN Range | Velocity Variation | Character |
|--------------|-------------------|-----------|
| 0-30% | ±5% | Consistent, mechanical |
| 30-60% | ±10% | Subtle dynamics |
| 60-100% | ±20% | Expressive, uneven |

```cpp
float GetVelocityWithVariation(float baseVel, float broken, uint32_t& rng) {
    float variationRange;
    if (broken < 0.3f) variationRange = 0.05f;
    else if (broken < 0.6f) variationRange = Lerp(0.05f, 0.10f, (broken - 0.3f) / 0.3f);
    else variationRange = Lerp(0.10f, 0.20f, (broken - 0.6f) / 0.4f);
    
    float variation = (RandomFloat(rng) - 0.5f) * 2.0f * variationRange;
    return Clamp(baseVel + variation, 0.2f, 1.0f);
}
```

### Acceptance Criteria
- [ ] Swing calculated from BROKEN parameter
- [ ] Jitter scales with BROKEN (0 below 40%)
- [ ] Step displacement at BROKEN > 50%
- [ ] Velocity variation scales with BROKEN
- [ ] All effects combine coherently

---

## Feature: Phrase-Aware Modulation [phrase-modulation]

The algorithm is aware of phrase position to create musical builds and fills. Even at high DRIFT, fills target musically appropriate positions to maintain danceability.

### Design Philosophy: Structured Fills

Even with maximum DRIFT, patterns should feel like they have **musical intention**:

1. **Fills happen at phrase boundaries** — middle (50%) and end (75-100%) of phrases
2. **Fills resolve to downbeats** — energy builds toward the next phrase start
3. **Higher DRIFT = more fill probability** — but fills still target the right positions
4. **RATCHET controls fill intensity** — DRIFT says "when", RATCHET says "how hard"

### Phrase Position Tracking

Same as v2, track position within the loop:

```cpp
struct PhrasePosition {
    int   stepInPhrase;    // 0 to (loopLengthBars * 16 - 1)
    float phraseProgress;  // 0.0 to 1.0
    bool  isDownbeat;      // Step 0 of any bar
    bool  isFillZone;      // Last 25% of phrase
    bool  isBuildZone;     // Last 50% of phrase
    bool  isMidPhrase;     // 40-60% of phrase (potential mid-phrase fill)
};
```

### Fill Zones

Fills are structured to create musical tension and release:

| Zone | Phrase Progress | Purpose |
|------|-----------------|---------|
| **Groove** | 0-40% | Stable pattern, minimal fills |
| **Mid-Point** | 40-60% | Potential mid-phrase fill (builds anticipation) |
| **Build** | 50-75% | Increasing energy toward phrase end |
| **Fill** | 75-100% | Maximum fill activity, resolving to downbeat |

### Phrase Modulation of Weights

Near phrase boundaries, weights are modulated to create natural fills:

```cpp
float GetPhraseWeightBoost(const PhrasePosition& pos, float drift, float ratchet) {
    // No boost outside fill-eligible zones
    if (!pos.isBuildZone && !pos.isMidPhrase) return 0.0f;
    
    // DRIFT controls fill PROBABILITY (when fills happen)
    // RATCHET controls fill INTENSITY (how hard fills hit)
    
    float boost = 0.0f;
    
    if (pos.isFillZone) {
        // Last 25%: significant boost, scales with RATCHET
        float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f;  // 0 to 1
        float baseBoost = 0.15f + fillProgress * 0.10f;  // 0.15 to 0.25
        boost = baseBoost * (0.5f + ratchet * 0.5f);  // Half boost at RATCHET=0, full at RATCHET=1
    } 
    else if (pos.isBuildZone) {
        // 50-75%: subtle boost
        float buildProgress = (pos.phraseProgress - 0.5f) * 4.0f;
        boost = buildProgress * 0.075f * (0.5f + ratchet * 0.5f);
    }
    else if (pos.isMidPhrase) {
        // 40-60%: optional mid-phrase fill (probability scales with DRIFT)
        // Only trigger if DRIFT > 50%
        if (drift > 0.5f) {
            float midProgress = 1.0f - fabsf(pos.phraseProgress - 0.5f) * 5.0f;  // Peak at 50%
            boost = midProgress * 0.1f * (drift - 0.5f) * 2.0f * ratchet;
        }
    }
    
    return boost;
}
```

### RATCHET Parameter [ratchet-control]

**RATCHET** (K4+Shift) controls fill intensity and timing. While DRIFT controls WHEN fills occur, RATCHET controls HOW INTENSE they are.

| RATCHET Level | Behavior |
|---------------|----------|
| **0%** | Subtle fills — slight density boost only |
| **25%** | Moderate fills — noticeable energy increase |
| **50%** | Standard fills — clear rhythmic intensification |
| **75%** | Aggressive fills — ratcheted 16ths, strong build |
| **100%** | Maximum intensity — rapid-fire hits, peak energy |

#### RATCHET Effects

1. **Fill Density Boost**: Higher RATCHET = more triggers during fill zones
2. **Ratcheting**: At RATCHET > 50%, fills can include rapid repeated hits (32nd notes)
3. **Velocity Ramp**: Fill velocities increase toward phrase end
4. **Resolution**: Final fill beat resolves strongly to phrase downbeat

```cpp
struct RatchetConfig {
    float densityBoost;      // Extra density during fills (0.0-0.3)
    bool  enableRatcheting;  // Allow 32nd note subdivisions
    float velocityRamp;      // Velocity increase toward phrase end (1.0-1.3)
    float resolutionAccent;  // Velocity boost on phrase downbeat (1.0-1.5)
};

RatchetConfig GetRatchetConfig(float ratchet) {
    RatchetConfig cfg;
    
    // Density boost scales linearly with RATCHET
    cfg.densityBoost = ratchet * 0.3f;  // 0 to 30% extra density
    
    // Ratcheting (32nd subdivisions) only above 50%
    cfg.enableRatcheting = ratchet > 0.5f;
    
    // Velocity ramp: fills get louder toward phrase end
    cfg.velocityRamp = 1.0f + ratchet * 0.3f;  // 1.0 to 1.3
    
    // Strong resolution to phrase downbeat
    cfg.resolutionAccent = 1.0f + ratchet * 0.5f;  // 1.0 to 1.5
    
    return cfg;
}
```

### DRIFT × RATCHET Interaction

These parameters work together to control fill behavior:

| DRIFT | RATCHET | Result |
|-------|---------|--------|
| 0 | 0 | No fills, pattern repeats exactly |
| 0 | 100 | No fills (DRIFT gates fill probability) |
| 100 | 0 | Fills occur, but subtle/minimal |
| 100 | 100 | Maximum fill intensity and frequency |
| 50 | 50 | Moderate fills at phrase boundaries |

**Key distinction:**
- **DRIFT** = "Do fills happen?" (probability gate)
- **RATCHET** = "How intense are fills?" (intensity control)

### Phrase Modulation of BROKEN

Temporarily increase BROKEN in fill zones for extra chaos:

```cpp
float GetEffectiveBroken(float broken, const PhrasePosition& pos, float drift) {
    if (!pos.isFillZone) return broken;
    
    // Boost BROKEN by up to 20% in fill zone, scaled by DRIFT
    // At DRIFT=0, no chaos boost (pattern stays stable)
    float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f;  // 0 to 1
    float fillBoost = fillProgress * 0.2f * drift;  // 0 to 0.2 at max DRIFT
    
    return Clamp(broken + fillBoost, 0.0f, 1.0f);
}
```

### Downbeat Accent

First beat of phrase gets velocity boost (scaled by RATCHET):

```cpp
float GetPhraseAccent(const PhrasePosition& pos, float ratchet) {
    if (pos.stepInPhrase == 0) {
        // Phrase downbeat: stronger accent with higher RATCHET
        return 1.2f + ratchet * 0.3f;  // 1.2 to 1.5
    }
    if (pos.isDownbeat) return 1.1f;  // Bar downbeat
    return 1.0f;
}
```

### Acceptance Criteria
- [x] Phrase position tracking (same as v2)
- [x] Weight boost in build/fill zones
- [x] BROKEN boost in fill zones
- [x] Downbeat accent multiplier
- [ ] **RATCHET parameter controls fill intensity**
- [ ] **Fills target phrase boundaries (mid-point, end)**
- [ ] **High DRIFT + high RATCHET = intense fills**
- [ ] **DRIFT=0 = no fills, regardless of RATCHET**
- [ ] **Ratcheting (32nd subdivisions) at RATCHET > 50%**
- [ ] **Fill resolution accent on phrase downbeat**

---

## Feature: FUSE Energy Balance [fuse-balance]

FUSE tilts the energy between Anchor and Shimmer voices.

### Implementation

```cpp
void ApplyFuse(float fuse, float& anchorDensity, float& shimmerDensity) {
    // fuse = 0.0: anchor-heavy (kick emphasized)
    // fuse = 0.5: balanced
    // fuse = 1.0: shimmer-heavy (snare/hat emphasized)
    
    float bias = (fuse - 0.5f) * 0.3f;  // ±15% density shift
    
    anchorDensity = Clamp(anchorDensity - bias, 0.0f, 1.0f);
    shimmerDensity = Clamp(shimmerDensity + bias, 0.0f, 1.0f);
}
```

### Acceptance Criteria
- [ ] FUSE at 0.5 = no change
- [ ] FUSE CCW boosts Anchor, reduces Shimmer
- [ ] FUSE CW boosts Shimmer, reduces Anchor
- [ ] ±15% density shift at extremes

---

## Feature: DRIFT Variation Control [drift-control]

**DRIFT** is the third primary performance control. It determines **how much the pattern evolves over time** — the axis from fully locked/predictable to constantly varying/evolving.

### Critical Design Rules

1. **DRIFT=0 means ZERO variation.** The pattern is 100% identical every loop. No beat variation whatsoever. This is an absolute guarantee.

2. **DRIFT controls evolution, not structure.** BROKEN controls WHERE hits occur (pattern shape). DRIFT controls WHETHER those decisions change over time.

3. **The Reference Point:** At BROKEN=0, DRIFT=0, DENSITY=50%: classic 4/4 kick with snare on backbeat, repeated identically forever.

### The Core Concept

Instead of separate "deterministic mode" and "random fills" controls, DRIFT provides a unified axis:

| DRIFT Level | Behavior |
|-------------|----------|
| **0%** | **Fully locked — exact same pattern every single loop. ZERO variation.** |
| **25%** | Core groove locked, ghost notes may vary |
| **50%** | Downbeats stable, off-beats can shift |
| **75%** | Only bar downbeats locked, rest evolves |
| **100%** | Everything varies — maximum evolution |

### Stratified Stability Model

The key insight: **not all steps are equally "lockable"**. Important beats (downbeats) should stay stable longer than ghost notes as DRIFT increases.

Each step has a **stability value** based on its musical importance:

```cpp
float GetStepStability(int step) {
    // Bar downbeats: most stable (lock until DRIFT > 100%)
    if (step % 16 == 0) return 1.0f;
    // Half notes: very stable (lock until DRIFT > 85%)
    if (step % 8 == 0) return 0.85f;
    // Quarter notes: stable (lock until DRIFT > 70%)
    if (step % 4 == 0) return 0.7f;
    // 8th off-beats: moderate (lock until DRIFT > 40%)
    if (step % 2 == 0) return 0.4f;
    // 16th ghosts: least stable (vary unless DRIFT < 20%)
    return 0.2f;
}
```

**DRIFT sets the threshold**: Steps with stability *above* DRIFT use a fixed seed (same every loop). Steps *below* DRIFT use a varying seed (different each loop).

### Implementation

```cpp
struct PulseField {
    uint32_t patternSeed_;    // Fixed seed for "locked" elements
    uint32_t loopSeed_;       // Changes each phrase for "drifting" elements
    
    // Per-voice drift multipliers: Anchor more stable, Shimmer more drifty
    static constexpr float kAnchorDriftMult = 0.7f;
    static constexpr float kShimmerDriftMult = 1.3f;
    
    bool ShouldStepFire(int step, float density, float broken, float drift, bool isAnchor) {
        // ============================================
        // CRITICAL: DRIFT=0 MEANS ZERO VARIATION
        // ============================================
        // At DRIFT=0, ALL steps use patternSeed_ (fully deterministic)
        // BROKEN still affects WHERE hits land (pattern structure)
        // but DRIFT=0 guarantees identical output every loop
        
        float weight = GetEffectiveWeight(step, broken, isAnchor);
        float threshold = 1.0f - density;
        
        // Apply per-voice drift multiplier
        float driftMult = isAnchor ? kAnchorDriftMult : kShimmerDriftMult;
        float effectiveDrift = Clamp(drift * driftMult, 0.0f, 1.0f);
        
        // Get step's stability tier
        float stability = GetStepStability(step);
        
        // Is this step locked or can it drift?
        // At DRIFT=0: effectiveDrift=0, all stability values > 0, so ALL steps locked
        bool isLocked = stability > effectiveDrift;
        
        // Pick the appropriate random seed
        uint32_t rng = isLocked 
            ? HashStep(patternSeed_, step)   // Same every loop
            : HashStep(loopSeed_, step);     // Different each loop
        
        // Calculate trigger with appropriate randomness
        // NOTE: This noise is deterministic given the seed.
        // At DRIFT=0, seed is always patternSeed_, so noise is identical every loop.
        float noise = (RandomFloat(rng) - 0.5f) * broken * 0.4f;
        float effectiveWeight = Lerp(weight, 0.5f, broken) + noise;
        
        return effectiveWeight > threshold;
    }
    
    void OnPhraseReset() {
        // New seed each loop for drifting elements
        loopSeed_ = GenerateNewSeed();
    }
};
```

### DRIFT × BROKEN Interaction

These two axes are **independent** and combine powerfully:

| BROKEN | DRIFT | Result |
|--------|-------|--------|
| **0 (straight)** | **0 (locked)** | **Classic 4/4, identical every loop (DJ tool)** |
| 0 (straight) | 100 (evolving) | Classic 4/4 base, subtle variations each loop |
| 100 (chaos) | 0 (locked) | IDM pattern, but same chaos every loop (signature beat) |
| 100 (chaos) | 100 (evolving) | Maximum unpredictability (generative) |

**Key distinction:**
- **BROKEN** controls the SHAPE of the pattern (simulating different base patterns)
- **DRIFT** controls the STABILITY of the pattern (whether it repeats or evolves)

### CV Modulation of DRIFT

CV 8 → DRIFT is extremely expressive:

| Patch | Effect |
|-------|--------|
| **Envelope** | Pattern "crystallizes" as envelope rises, drifts as it falls |
| **Slow LFO** | Pattern oscillates between locked and evolving |
| **Gate from phrase** | Lock during verse, drift during chorus |
| **Manual CV** | Performance control: twist to freeze/unfreeze |

### Acceptance Criteria
- [x] Step stability values implemented (1.0 for downbeats → 0.2 for 16ths)
- [x] DRIFT threshold determines which steps use locked vs. varying seed
- [x] `patternSeed_` persists across loops (locked elements)
- [x] `loopSeed_` regenerates on phrase reset (drifting elements)
- [ ] **DRIFT = 0% produces IDENTICAL pattern every loop (ZERO variation)**
- [x] DRIFT = 100% produces unique pattern each loop
- [x] **Per-voice DRIFT**: Anchor uses 0.7× drift multiplier, Shimmer uses 1.3×
- [x] At DRIFT = 100%, Anchor still has some stability (effective 70%)
- [x] CV modulation of DRIFT works correctly

---

## Feature: COUPLE Voice Interlock [couple-interlock]

**COUPLE** is a simplified replacement for the three-mode ORBIT system. It provides a single 0-100% axis for voice relationship strength.

### Behavior

| COUPLE | Behavior |
|--------|----------|
| **0%** | Fully independent — voices can collide or gap freely |
| **50%** | Soft interlock — slight collision avoidance |
| **100%** | Hard interlock — Shimmer always fills Anchor gaps |

### Implementation

```cpp
void ApplyCouple(float couple, bool anchorFires, bool& shimmerFires, float& shimmerVel) {
    if (couple < 0.1f) return;  // Fully independent
    
    if (anchorFires) {
        // Anchor is firing — reduce shimmer probability
        float suppressChance = couple * 0.8f;  // Up to 80% suppression at max
        if (RandomFloat() < suppressChance) {
            shimmerFires = false;
        }
    } else {
        // Anchor is silent — boost shimmer probability
        if (!shimmerFires && couple > 0.5f) {
            float boostChance = (couple - 0.5f) * 0.6f;  // Up to 30% boost
            if (RandomFloat() < boostChance) {
                shimmerFires = true;
                shimmerVel = 0.5f + RandomFloat() * 0.3f;  // Medium velocity fill
            }
        }
    }
}
```

### Why This Replaces ORBIT

| Old ORBIT Mode | Now Achieved With |
|----------------|-------------------|
| Interlock | COUPLE = 75-100% |
| Free | COUPLE = 0-25% |
| Shadow | *Removed* — can be approximated with high COUPLE + delay FX |

Shadow mode added complexity for limited musical value. The call-response behavior of Interlock is the most musically useful, and COUPLE provides a continuous strength control.

### Acceptance Criteria
- [ ] COUPLE parameter (0-1) controls interlock strength
- [ ] At 0%: voices are fully independent
- [ ] At 100%: shimmer strongly fills anchor gaps
- [ ] Collision suppression scales with COUPLE
- [ ] Gap-filling boost at COUPLE > 50%

---

## Control Layout

### Performance Mode Primary (Switch DOWN) — The Main 4

| Knob | Parameter | Range | Mental Model |
|------|-----------|-------|--------------|
| **K1** | ANCHOR DENSITY | 0%=silence → 100%=full | "How much kick" |
| **K2** | SHIMMER DENSITY | 0%=silence → 100%=full | "How much snare" |
| **K3** | BROKEN | 0%=4/4 → 100%=IDM | "Pattern shape" |
| **K4** | DRIFT | 0%=locked → 100%=evolving | "Does it change?" |

These four knobs define the complete rhythmic character:
- **DENSITY** (K1, K2): How much activity for each voice. **0 = silence, always.**
- **BROKEN** (K3): Pattern structure (WHERE hits occur). Genre character emerges from this.
- **DRIFT** (K4): Pattern evolution (WHETHER it repeats). **0 = identical every loop.**

### Performance Mode Shift (Switch DOWN + B7) — Secondary Performance

| Knob | Parameter | Range | Notes |
|------|-----------|-------|-------|
| K1+S | FUSE | CCW=kick, CW=snare | Energy balance between voices |
| K2+S | LENGTH | 1,2,4,8,16 bars | Loop length |
| K3+S | COUPLE | 0%=independent, 100%=interlock | Voice relationship strength |
| **K4+S** | **RATCHET** | 0%=subtle, 100%=intense | Fill intensity during phrase transitions |

### RATCHET × DRIFT Interaction

| DRIFT | RATCHET | Fill Behavior |
|-------|---------|---------------|
| 0% | any | No fills (pattern locked) |
| 50% | 0% | Occasional subtle fills |
| 50% | 100% | Occasional intense fills |
| 100% | 0% | Frequent subtle fills |
| 100% | 100% | Maximum fill frequency and intensity |

### Config Mode Primary (Switch UP)

| Knob | Parameter | Range |
|------|-----------|-------|
| K1 | ANCHOR ACCENT | Subtle → Hard |
| K2 | SHIMMER ACCENT | Subtle → Hard |
| K3 | CONTOUR | Velocity/Decay/Pitch/Random |
| K4 | TEMPO / CLOCK DIV | Context-aware |

### Config Mode Shift (Switch UP + B7) — Low-Priority Tweaks

| Knob | Parameter | Range | Priority |
|------|-----------|-------|----------|
| K1+S | SWING TASTE | Fine-tune swing within BROKEN's range | Low |
| K2+S | GATE TIME | 5-50ms | Normal |
| K3+S | HUMANIZE | Extra jitter on top of BROKEN's jitter | Low |
| K4+S | CLOCK DIV / AUX MODE | Context-aware | Normal |

**Note on Low-Priority Controls**: SWING TASTE and HUMANIZE are marked low-priority because BROKEN already provides genre-appropriate swing and jitter. These controls allow fine-tuning but aren't essential for most use cases.

### CV Inputs (Always Performance)

| CV | Modulates | Use Case |
|----|-----------|----------|
| CV 5 | ANCHOR DENSITY | Sidechain, envelope follower |
| CV 6 | SHIMMER DENSITY | Sidechain, envelope follower |
| CV 7 | BROKEN | LFO for evolving weirdness, envelope for builds |
| CV 8 | DRIFT | Envelope to lock/unlock pattern, LFO for breathing |

**Expressive Patching Ideas**:
- **CV 7 (BROKEN)**: Slow triangle LFO → pattern gradually gets weirder, then returns to straight
- **CV 8 (DRIFT)**: Envelope from phrase gate → pattern locks in during phrase, drifts at boundaries
- **CV 7 + CV 8 together**: Create complex evolving patterns that shift in both character and stability

---

## Feature: LED Feedback [led-feedback]

The LED should provide clear visual feedback that reflects the current state and responds to the key parameters (especially BROKEN and DRIFT).

### Mode Indication

| State | LED Behavior |
|-------|--------------|
| Performance Mode | Pulse on Anchor triggers |
| Performance + Shift | Slower breathing (500ms cycle) |
| Config Mode | Solid ON |
| Config + Shift | Slow blink (1Hz) |

### Parameter Feedback (1s timeout after knob movement)

| Parameter | LED Behavior |
|-----------|--------------|
| DENSITY (either) | Brightness = density level |
| BROKEN | Flash rate increases with broken level |
| DRIFT | Pulse regularity decreases with drift (stable → jittery) |
| FUSE | Brightness weighted toward active voice |
| COUPLE | Pulse on both voices when high, independent when low |

### BROKEN × DRIFT LED Behavior

The LED timing should reflect the current state of the algorithm:

| BROKEN | DRIFT | LED Character |
|--------|-------|---------------|
| Low | Low | Regular, steady pulses (metronomic) |
| Low | High | Regular timing, but pulse intensity varies |
| High | Low | Irregular timing, but consistent each loop |
| High | High | Maximum irregularity (chaotic) |

### Phrase Position

| State | LED Behavior |
|-------|--------------|
| Downbeat | Extra bright pulse |
| Fill Zone | Rapid triple-pulse pattern |
| Build Zone | Gradually increasing pulse rate |
| Pattern locked (DRIFT=0) | Slightly brighter baseline |

### System States

| State | LED Behavior |
|-------|--------------|
| External Clock | Double-pulse pattern |
| High BROKEN (>75%) | Irregular timing in pulses |
| High DRIFT (>75%) | Varying pulse intensity |
| Pattern Lock gesture | Quick flash confirmation |

### Acceptance Criteria
- [ ] Mode indication (Performance/Config/Shift)
- [ ] Parameter value display on knob movement
- [ ] Phrase position feedback
- [ ] BROKEN level affects LED timing regularity
- [ ] DRIFT level affects LED pulse consistency
- [ ] 1-second timeout for parameter display
- [ ] LED character reflects current BROKEN × DRIFT state

---

## Implementation Strategy

### Phase 1: Core Algorithm ✓
1. Implement weight tables for Anchor and Shimmer
2. Implement `ShouldStepFire()` with BROKEN flattening
3. Implement DENSITY threshold **with absolute floor check FIRST**
4. Basic trigger generation loop (deterministic with fixed seed)

### Phase 2: DRIFT System ✓
1. Implement step stability values
2. Dual-seed system (patternSeed_ + loopSeed_)
3. DRIFT threshold for locked vs. drifting steps
4. **DRIFT=0 = ALL steps use patternSeed_ (zero variation)**
5. Phrase reset callback to regenerate loopSeed_

### Phase 3: BROKEN Effects Stack ✓
1. Swing tied to BROKEN
2. Jitter tied to BROKEN  
3. Step displacement at high BROKEN
4. Velocity variation scaling

### Phase 4: Phrase Awareness (Update Required)
1. Phrase position tracking (reuse from v2)
2. **Add isMidPhrase zone (40-60%)**
3. Weight boost in fill zones **scaled by RATCHET**
4. BROKEN boost in fill zones **scaled by DRIFT**
5. Downbeat accents **scaled by RATCHET**

### Phase 5: RATCHET System (NEW)
1. **Implement RATCHET parameter (K4+Shift)**
2. **RATCHET controls fill intensity (density boost, velocity ramp)**
3. **Ratcheting (32nd subdivisions) at RATCHET > 50%**
4. **Fill resolution accent on phrase downbeat**
5. **DRIFT gates fill probability (DRIFT=0 = no fills)**

### Phase 6: Voice Interaction ✓
1. FUSE energy balance
2. COUPLE interlock system
3. Collision avoidance at high COUPLE

### Phase 7: Bug Fixes (Required)
1. **Fix DENSITY=0 leakage** — absolute silence check must come FIRST
2. **Fix DRIFT=0 variation** — all steps must use patternSeed_

### Phase 8: Polish
1. LED feedback reflecting BROKEN × DRIFT × RATCHET
2. CV modulation of all primary params
3. Soft takeover for mode switching (reuse from v2)
4. Pattern lock gesture (optional)

### Migration from v2

The new algorithm can coexist with the old pattern system during development:

```cpp
#ifdef USE_PULSE_FIELD_V3
    pulseField_.GetTriggers(step, anchorDensity, shimmerDensity, broken, drift, ...);
#else
    GetSkeletonTriggers(step, anchorDensity, shimmerDensity, ...);
#endif
```

### Suggested File Structure

```
src/Engine/
├── PulseField.h          // Weight tables, core algorithm
├── PulseField.cpp        // ShouldStepFire, DRIFT logic
├── BrokenEffects.h       // Swing, jitter, displacement tied to BROKEN
├── Sequencer.h           // Main sequencer (updated for v3)
├── Sequencer.cpp         // Integration of PulseField
└── (existing files...)
```

---

## Comparison: v2 vs v3

| Aspect | v2 (Pattern Lookup) | v3 (Algorithmic) |
|--------|---------------------|------------------|
| Pattern Source | 16 hand-crafted skeletons | Weighted probability algorithm |
| Genre Control | Terrain + Grid (can mismatch) | Single BROKEN control |
| Variation Control | FLUX (adds chaos) | DRIFT (locked ↔ evolving) |
| Fill Control | Built into patterns | Separate RATCHET control |
| Voice Relationship | ORBIT (3 discrete modes) | COUPLE (0-100% strength) |
| Per-Voice Behavior | Same rules for both | Anchor stable, Shimmer drifty |
| Swing | Separate per-genre config | Tied to BROKEN |
| Predictability | Hard to predict combinations | Clear axes: structure × stability × fills |
| Determinism | Not supported | DRIFT=0 = identical every loop |
| DENSITY=0 Behavior | May have leakage | **Absolute silence guaranteed** |
| Code Size | ~500 bytes pattern data | ~200 bytes weight tables |
| Transitions | Abrupt pattern changes | Continuous morphing |
| Primary Knobs | 4 (Density×2, Flux, Fuse) | 4 (Density×2, Broken, Drift) |
| Shift Knobs | 4 | 4 (including RATCHET) |

---

## Design Decisions (Resolved)

### 1. FLUX → Removed, Replaced by DRIFT
FLUX was providing "extra randomness" but this is now elegantly covered by DRIFT. The combination of BROKEN (pattern regularity) and DRIFT (pattern stability) provides a complete 2D space of variation without needing a third chaos control.

### 2. SWING TASTE → Kept as Low-Priority Config+Shift
BROKEN already provides genre-appropriate swing (Techno=straight, Trip-Hop=lazy, etc.). SWING TASTE allows fine-tuning within that range for users who want precise control, but it's not essential.

### 3. ORBIT → Simplified to COUPLE
The three-mode system (Interlock/Free/Shadow) was complex. COUPLE provides a single 0-100% axis for voice relationship strength. Shadow mode (echo effect) was removed as it can be achieved with delay FX if desired.

### 4. Deterministic Mode → Built into DRIFT
Rather than a separate "lock seed" mode, DRIFT = 0% provides fully deterministic patterns. DRIFT = 100% provides fully generative patterns. The stratified stability model means you can have "locked downbeats with varying ghosts" at intermediate DRIFT values.

---

## Open Questions (Remaining)

*(None — all questions resolved.)*

### Resolved: K4+S is RATCHET

**Decision**: K4+Shift is now **RATCHET** — fill intensity control.

RATCHET works in tandem with DRIFT:
- **DRIFT** controls fill probability (when fills occur)
- **RATCHET** controls fill intensity (how intense fills are)

At DRIFT=0, RATCHET has no effect (no fills occur). At high DRIFT, RATCHET scales from subtle fills (0%) to aggressive ratcheted fills (100%).

### Resolved: Per-Voice DRIFT

**Decision**: Shimmer drifts more than Anchor at the same DRIFT setting.

This makes musical sense: kicks should stay more stable/predictable while hats/snares can vary more freely. This creates the feel of a "solid foundation with living details on top."

**Implementation**: Apply a drift multiplier per voice:

```cpp
// Anchor is more stable (drift multiplier 0.7)
// Shimmer is more drifty (drift multiplier 1.3)
constexpr float kAnchorDriftMultiplier = 0.7f;
constexpr float kShimmerDriftMultiplier = 1.3f;

float GetEffectiveDrift(float drift, bool isAnchor) {
    float multiplier = isAnchor ? kAnchorDriftMultiplier : kShimmerDriftMultiplier;
    return Clamp(drift * multiplier, 0.0f, 1.0f);
}
```

**Effect at various DRIFT levels**:

| DRIFT Knob | Anchor Effective | Shimmer Effective | Result |
|------------|------------------|-------------------|--------|
| 0% | 0% | 0% | Both fully locked |
| 30% | 21% | 39% | Anchor mostly locked, Shimmer ghosts vary |
| 50% | 35% | 65% | Anchor quarters locked, Shimmer quite loose |
| 70% | 49% | 91% | Anchor half-notes locked, Shimmer nearly full drift |
| 100% | 70% | 100% | Anchor still somewhat stable, Shimmer fully evolving |

This means even at DRIFT = 100%, the kick still has some stability (downbeats locked), while the snare/hat is fully generative.

### Deferred: Pattern Lock Gesture

For future consideration: a button gesture to "freeze" a pattern you like (copy loopSeed_ to patternSeed_). Not implementing now.

---

## Summary

The v3 algorithmic approach replaces discrete pattern lookups with a **four-axis performance model**:

### The Core Controls

| Control | Function | Mental Model | Critical Behavior |
|---------|----------|--------------|-------------------|
| **ANCHOR DENSITY** | Kick activity level | "How much kick" | **0 = silence, always** |
| **SHIMMER DENSITY** | Snare/hat activity level | "How much snare" | **0 = silence, always** |
| **BROKEN** | Pattern structure | "Pattern shape" | Controls WHERE hits occur |
| **DRIFT** | Pattern evolution | "Does it repeat?" | **0 = identical every loop** |
| **RATCHET** (K4+S) | Fill intensity | "How hard fills hit" | Gates by DRIFT |

### The Reference Point

> **BROKEN=0, DRIFT=0, DENSITIES at 50%: Classic 4/4 kick with snare on backbeat, repeated identically forever.**

This is the "known good" state that every user can return to. From here, each axis adds complexity in a predictable way.

### Key Design Principles

1. **DENSITY=0 is absolute silence.** No triggers fire, regardless of any other parameter. This is a hard floor that cannot be overridden by fills, phrase modulation, or any other system.

2. **DRIFT=0 is absolute repetition.** The pattern is 100% identical every loop. No variation whatsoever. BROKEN affects pattern structure but not stability.

3. **BROKEN controls STRUCTURE, DRIFT controls EVOLUTION.** These are orthogonal axes:
   - BROKEN = WHERE hits occur (4/4 → scattered IDM)
   - DRIFT = WHETHER the pattern changes over time (locked → generative)

4. **Fills are structured, not random.** Even at high DRIFT, fills target musically appropriate positions (mid-phrase, phrase-end) and resolve to downbeats. The pattern always feels **danceable**.

5. **RATCHET controls fill intensity.** DRIFT says "when fills happen", RATCHET says "how intense they are". At DRIFT=0, RATCHET has no effect (no fills occur).

### Key Innovations

1. **BROKEN unifies genre**: Swing, jitter, displacement all scale with one knob. Genre character (Techno/Tribal/Trip-Hop/IDM) emerges naturally.

2. **DRIFT unifies predictability**: No more separate "deterministic mode" and "random fills". DRIFT = 0% locks the pattern; DRIFT = 100% makes it evolve.

3. **Stratified stability**: At intermediate DRIFT, downbeats stay locked while ghost notes vary. This creates "stable groove with living details".

4. **Per-voice DRIFT**: Anchor (kick) is more stable (0.7× drift), Shimmer (snare/hat) is more drifty (1.3× drift). Even at max DRIFT, kicks have some stability while hats are fully generative.

5. **RATCHET for fills**: Separate control for fill intensity allows precise control over phrase transitions without affecting base pattern stability.

6. **COUPLE simplifies voice relationship**: Single 0-100% interlock strength replaces three discrete modes.

7. **CV modulation of all axes**: BROKEN, DRIFT, and densities are all CV-controllable for expressive patching.

### The Result

A **controllable, playable, and professional** rhythm generator where:
- **DENSITY=0 is silence** — absolute floor, no surprises
- **DRIFT=0 is locked** — identical pattern every loop, reliable DJ tool
- **BROKEN controls genre** — techno→IDM gradient in one knob
- **RATCHET controls fills** — separate from pattern stability
- **Fills are musical** — they target phrase boundaries, not random positions
- Every knob turn produces a **predictable, audible change**
- No parameter combinations produce **incoherent** results

The module becomes an instrument you can **learn** and **master**, not a black box of unpredictable combinations.
