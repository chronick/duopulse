# DuoPulse v3: Simplified Algorithmic Approach

**Specification Draft v0.1**  
**Date: 2025-12-16**

---

## Executive Summary

This document proposes a radical simplification of DuoPulse's rhythm generation system, replacing the 16 discrete pattern lookup tables with a continuous **algorithmic pulse field** controlled by two primary axes:

1. **DENSITY** — How many triggers fire (sparse → dense)
2. **BROKEN** — How regular/irregular the pattern is (4/4 techno → IDM chaos)

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
- No pattern mismatches possible

### Core Mental Model

```
DENSITY = "How much is happening"
BROKEN  = "How weird is it"
```

Together they span the entire rhythmic space:

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
   T     0%   │ Silence          Silence        │
   Y          └────────────────────────────────┘
```

---

## Feature: Weighted Pulse Field Algorithm [pulse-field]

### Concept

Each of the 32 steps in a pattern has a **weight** that represents its "likelihood" of triggering. The algorithm uses these weights combined with DENSITY and BROKEN to determine what fires.

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
    // 1. Get base weight for this step position
    float baseWeight = weights[step];
    
    // 2. BROKEN flattens the weight distribution
    //    At broken=0: full differentiation (downbeats dominate)
    //    At broken=1: weights converge to 0.5 (equal probability)
    float effectiveWeight = Lerp(baseWeight, 0.5f, broken);
    
    // 3. Add randomness scaled by BROKEN
    //    More broken = more random variation in weights
    float noise = (Random() - 0.5f) * broken * 0.4f;
    effectiveWeight += noise;
    effectiveWeight = Clamp(effectiveWeight, 0.0f, 1.0f);
    
    // 4. DENSITY sets the threshold
    //    density=0 → threshold=1.0 (nothing fires)
    //    density=1 → threshold=0.0 (everything fires)
    float threshold = 1.0f - density;
    
    // 5. Fire if weight exceeds threshold
    return effectiveWeight > threshold;
}
```

### How It Works

**At BROKEN = 0% (Straight):**
- Weights are sharply differentiated
- Only highest-weight positions fire at medium density
- Result: Regular, predictable patterns

**At BROKEN = 100% (Chaos):**
- All weights converge toward 0.5
- Any step is equally likely to fire
- Additional noise makes each iteration different
- Result: Unpredictable, IDM-style patterns

**Density Sweeps:**
- At any BROKEN level, DENSITY controls how many steps fire
- Low density: Only the highest-weight steps
- High density: Nearly all steps

### Acceptance Criteria
- [ ] Weight table for 32 steps implemented
- [ ] Effective weight calculation with BROKEN flattening
- [ ] Noise injection scaled by BROKEN
- [ ] Threshold comparison with DENSITY
- [ ] Deterministic mode (seeded RNG) for reproducibility

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
| 50% | 0% | Kicks on 0, 8, 16, 24 | Snares on 8, 24 |
| 50% | 50% | Kicks shift around | Snares + off-beat hits |
| 50% | 100% | Random kick placement | Random snare placement |
| 100% | 0% | Full kick pattern, on-grid | Full snare pattern, on-grid |
| 100% | 100% | Maximum kick chaos | Maximum snare chaos |

### Acceptance Criteria
- [ ] Separate weight tables for Anchor and Shimmer
- [ ] Anchor emphasizes downbeats (0, 8, 16, 24)
- [ ] Shimmer emphasizes backbeats (8, 24)
- [ ] Both voices respond identically to density/broken controls
- [ ] Weight profiles can be tuned via constants

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

The algorithm should be aware of phrase position to create musical builds and fills.

### Phrase Position Tracking

Same as v2, track position within the loop:

```cpp
struct PhrasePosition {
    int   stepInPhrase;    // 0 to (loopLengthBars * 16 - 1)
    float phraseProgress;  // 0.0 to 1.0
    bool  isDownbeat;      // Step 0 of any bar
    bool  isFillZone;      // Last 25% of phrase
    bool  isBuildZone;     // Last 50% of phrase
};
```

### Phrase Modulation of Weights

Near phrase boundaries, weights are modulated to create natural fills:

```cpp
float GetPhraseWeightBoost(const PhrasePosition& pos, float broken) {
    if (!pos.isBuildZone) return 0.0f;
    
    // Base boost: increases toward phrase end
    float boost = 0.0f;
    
    if (pos.isFillZone) {
        // Last 25%: significant boost to off-beat weights
        boost = 0.15f + (pos.phraseProgress - 0.75f) * 0.4f;  // 0.15 to 0.25
    } else if (pos.isBuildZone) {
        // 50-75%: subtle boost
        boost = (pos.phraseProgress - 0.5f) * 0.3f;  // 0 to 0.075
    }
    
    // Techno (low broken) has subtle fills; IDM (high broken) has dramatic fills
    float genreScale = 0.5f + broken * 1.0f;  // 0.5 to 1.5
    
    return boost * genreScale;
}
```

### Phrase Modulation of BROKEN

Temporarily increase BROKEN in fill zones for extra chaos:

```cpp
float GetEffectiveBroken(float broken, const PhrasePosition& pos) {
    if (!pos.isFillZone) return broken;
    
    // Boost BROKEN by up to 20% in fill zone
    float fillBoost = 0.2f * (pos.phraseProgress - 0.75f) * 4.0f;  // 0 to 0.2
    
    return Clamp(broken + fillBoost, 0.0f, 1.0f);
}
```

### Downbeat Accent

First beat of phrase gets velocity boost:

```cpp
float GetPhraseAccent(const PhrasePosition& pos) {
    if (pos.stepInPhrase == 0) return 1.2f;  // Phrase downbeat
    if (pos.isDownbeat) return 1.1f;          // Bar downbeat
    return 1.0f;
}
```

### Acceptance Criteria
- [ ] Phrase position tracking (same as v2)
- [ ] Weight boost in build/fill zones
- [ ] BROKEN boost in fill zones
- [ ] Downbeat accent multiplier
- [ ] Fill intensity scales with BROKEN level

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

**DRIFT** is the fourth primary performance control. It determines **how much the pattern evolves over time** — the axis from fully locked/predictable to constantly varying/evolving.

### The Core Concept

Instead of separate "deterministic mode" and "random fills" controls, DRIFT provides a unified axis:

| DRIFT Level | Behavior |
|-------------|----------|
| **0%** | Fully locked — exact same pattern every loop |
| **25%** | Core groove locked, ghost notes vary |
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
        float weight = GetEffectiveWeight(step, broken, isAnchor);
        float threshold = 1.0f - density;
        
        // Apply per-voice drift multiplier
        float driftMult = isAnchor ? kAnchorDriftMult : kShimmerDriftMult;
        float effectiveDrift = Clamp(drift * driftMult, 0.0f, 1.0f);
        
        // Get step's stability tier
        float stability = GetStepStability(step);
        
        // Is this step locked or can it drift?
        bool isLocked = stability > effectiveDrift;
        
        // Pick the appropriate random seed
        uint32_t rng = isLocked 
            ? HashStep(patternSeed_, step)   // Same every loop
            : HashStep(loopSeed_, step);     // Different each loop
        
        // Calculate trigger with appropriate randomness
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

These two axes are independent and combine powerfully:

| BROKEN | DRIFT | Result |
|--------|-------|--------|
| Low | Low | Classic 4/4, same every loop (DJ tool) |
| Low | High | Classic 4/4 base, subtle variations each loop |
| High | Low | IDM pattern, but same chaos every loop (signature beat) |
| High | High | Maximum unpredictability (generative) |

### CV Modulation of DRIFT

CV 8 → DRIFT is extremely expressive:

| Patch | Effect |
|-------|--------|
| **Envelope** | Pattern "crystallizes" as envelope rises, drifts as it falls |
| **Slow LFO** | Pattern oscillates between locked and evolving |
| **Gate from phrase** | Lock during verse, drift during chorus |
| **Manual CV** | Performance control: twist to freeze/unfreeze |

### Acceptance Criteria
- [ ] Step stability values implemented (1.0 for downbeats → 0.2 for 16ths)
- [ ] DRIFT threshold determines which steps use locked vs. varying seed
- [ ] `patternSeed_` persists across loops (locked elements)
- [ ] `loopSeed_` regenerates on phrase reset (drifting elements)
- [ ] DRIFT = 0% produces identical pattern every loop
- [ ] DRIFT = 100% produces unique pattern each loop
- [ ] **Per-voice DRIFT**: Anchor uses 0.7× drift multiplier, Shimmer uses 1.3×
- [ ] At DRIFT = 100%, Anchor still has some stability (effective 70%)
- [ ] CV modulation of DRIFT works correctly

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
| **K1** | ANCHOR DENSITY | 0%=silent → 100%=full | "How much kick" |
| **K2** | SHIMMER DENSITY | 0%=silent → 100%=full | "How much snare" |
| **K3** | BROKEN | 0%=4/4 → 100%=IDM | "How weird" |
| **K4** | DRIFT | 0%=locked → 100%=evolving | "How much it changes" |

These four knobs define the complete rhythmic character:
- **DENSITY** (K1, K2): How much activity for each voice
- **BROKEN** (K3): Pattern regularity (genre character emerges from this)
- **DRIFT** (K4): Pattern stability (locked groove vs. evolving chaos)

### Performance Mode Shift (Switch DOWN + B7) — Secondary Performance

| Knob | Parameter | Range | Notes |
|------|-----------|-------|-------|
| K1+S | FUSE | CCW=kick, CW=snare | Energy balance between voices |
| K2+S | LENGTH | 1,2,4,8,16 bars | Loop length |
| K3+S | COUPLE | 0%=independent, 100%=interlock | Voice relationship strength |
| K4+S | *(Reserved)* | — | Could add feature later or leave empty |

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

### Phase 1: Core Algorithm
1. Implement weight tables for Anchor and Shimmer
2. Implement `ShouldStepFire()` with BROKEN flattening
3. Implement DENSITY threshold
4. Basic trigger generation loop (deterministic with fixed seed)

### Phase 2: DRIFT System
1. Implement step stability values
2. Dual-seed system (patternSeed_ + loopSeed_)
3. DRIFT threshold for locked vs. drifting steps
4. Phrase reset callback to regenerate loopSeed_

### Phase 3: BROKEN Effects Stack
1. Swing tied to BROKEN
2. Jitter tied to BROKEN  
3. Step displacement at high BROKEN
4. Velocity variation scaling

### Phase 4: Phrase Awareness
1. Phrase position tracking (reuse from v2)
2. Weight boost in fill zones
3. BROKEN boost in fill zones
4. Downbeat accents

### Phase 5: Voice Interaction
1. FUSE energy balance
2. COUPLE interlock system
3. Collision avoidance at high COUPLE

### Phase 6: Polish
1. LED feedback reflecting BROKEN × DRIFT
2. CV modulation of all four primary params
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
| Voice Relationship | ORBIT (3 discrete modes) | COUPLE (0-100% strength) |
| Per-Voice Behavior | Same rules for both | Anchor stable, Shimmer drifty |
| Swing | Separate per-genre config | Tied to BROKEN |
| Predictability | Hard to predict combinations | Two clear axes: weird × stable |
| Determinism | Not supported | DRIFT = 0% for locked patterns |
| Code Size | ~500 bytes pattern data | ~200 bytes weight tables |
| Transitions | Abrupt pattern changes | Continuous morphing |
| Primary Knobs | 4 (Density×2, Flux, Fuse) | 4 (Density×2, Broken, Drift) |

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

1. **K4+S in Performance Shift**: Currently marked as "Reserved". Could be used for future features. Keep open for now.

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

The v3 algorithmic approach replaces discrete pattern lookups with a **two-axis performance model**:

### The Four Primary Controls

| Control | Function | Mental Model |
|---------|----------|--------------|
| **ANCHOR DENSITY** | Kick activity level | "How much kick" |
| **SHIMMER DENSITY** | Snare/hat activity level | "How much snare" |
| **BROKEN** | Pattern regularity | "How weird" (4/4 → IDM) |
| **DRIFT** | Pattern stability | "How much it changes" (locked → evolving) |

### Key Innovations

1. **BROKEN unifies genre**: Swing, jitter, displacement all scale with one knob. Genre character (Techno/Tribal/Trip-Hop/IDM) emerges naturally.

2. **DRIFT unifies predictability**: No more separate "deterministic mode" and "random fills". DRIFT = 0% locks the pattern; DRIFT = 100% makes it evolve.

3. **Stratified stability**: At intermediate DRIFT, downbeats stay locked while ghost notes vary. This creates "stable groove with living details".

4. **Per-voice DRIFT**: Anchor (kick) is more stable (0.7× drift), Shimmer (snare/hat) is more drifty (1.3× drift). Even at max DRIFT, kicks have some stability while hats are fully generative.

5. **COUPLE simplifies voice relationship**: Single 0-100% interlock strength replaces three discrete modes.

6. **CV modulation of BROKEN and DRIFT**: Extremely expressive patching possibilities for evolving patterns.

### The Result

A **controllable, playable, and professional** rhythm generator where:
- Every knob turn produces a **predictable, audible change**
- The full **techno→IDM gradient** is accessible via BROKEN
- Patterns can be **locked** (DJ tool) or **generative** (experimental)
- No parameter combinations produce **incoherent** results

The module becomes an instrument you can **learn** and **master**, not a black box of unpredictable combinations.
