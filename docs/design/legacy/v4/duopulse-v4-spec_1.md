# DuoPulse v4: Algorithmic Drum Sequencer Specification

**Target Platform**: Daisy Patch.init() (Electro-Smith)  
**Version**: 4.0  
**Status**: Implementation Spec  
**Last Updated**: 2025-01-XX

---

## Table of Contents

1. [Core Principles](#1-core-principles)
2. [Architecture Overview](#2-architecture-overview)
3. [Hardware Interface](#3-hardware-interface)
4. [Control System](#4-control-system)
5. [Pattern Field System](#5-pattern-field-system)
6. [Generation Pipeline](#6-generation-pipeline)
7. [Timing System (BROKEN Stack)](#7-timing-system-broken-stack)
8. [Output System](#8-output-system)
9. [LED Feedback System](#9-led-feedback-system)
10. [Data Structures](#10-data-structures)
11. [Implementation Pseudocode](#11-implementation-pseudocode)
12. [Configuration & Persistence](#12-configuration--persistence)
13. [Testing Requirements](#13-testing-requirements)

---

## Quick Reference: I/O Summary

### Outputs
| Output | Hardware | Signal |
|--------|----------|--------|
| **Anchor Trig** | Gate Out 1 | 5V trigger on anchor hits |
| **Shimmer Trig** | Gate Out 2 | 5V trigger on shimmer hits |
| **Anchor Velocity** | Audio Out L | 0-5V sample & hold (persists until next trig) |
| **Shimmer Velocity** | Audio Out R | 0-5V sample & hold (persists until next trig) |
| **AUX** | CV Out 1 | Clock (if no ext clock) OR Hat/FillGate/PhraseCV/Event |
| **LED** | CV Out 2 | Visual feedback (brightness = trigger activity) |

### Inputs
| Input | Hardware | Function |
|-------|----------|----------|
| **Clock** | Gate In 1 | External clock (if patched, enables AUX modes) |
| **Reset** | Gate In 2 | Reset to step 0 |
| **Fill CV** | Audio In L | Pressure-sensitive fill trigger (0-5V = intensity) |
| **Flavor CV** | Audio In R | Timing feel override (0-5V = straight→broken) |

### Performance Mode (Switch A) — Ergonomic Pairings
| Knob | Domain | Primary (CV-able) | +Shift |
|------|--------|-------------------|--------|
| K1 | **Intensity** | ENERGY (hit density) | PUNCH (velocity dynamics) |
| K2 | **Drama** | BUILD (phrase arc) | GENRE (style bank) |
| K3 | **Pattern** | FIELD X (syncopation) | DRIFT (evolution rate) |
| K4 | **Texture** | FIELD Y (complexity) | BALANCE (voice ratio) |

### Config Mode (Switch B) — Domain-Based
| Knob | Domain | Primary | +Shift |
|------|--------|---------|--------|
| K1 | **Grid** | PATTERN LENGTH (16/24/32/64) | PHRASE LENGTH (1/2/4/8 bars) |
| K2 | **Timing** | SWING (0-100%) | CLOCK DIV (1/2/4/8) |
| K3 | **Output** | AUX MODE (Hat/Fill/Phrase/Event) | AUX DENSITY (50-200%) |
| K4 | **Behavior** | RESET MODE (Phrase/Bar/Step) | VOICE COUPLING (Ind/Lock/Shadow) |

### CV Inputs (modulate primary performance controls)
| CV In | Modulates | Use Case |
|-------|-----------|----------|
| CV 1 | ENERGY | Sidechain from kick, intensity envelope |
| CV 2 | BUILD | LFO for automatic phrase dynamics! |
| CV 3 | FIELD X | Navigate syncopation axis |
| CV 4 | FIELD Y | Navigate complexity axis |

---

## 1. Core Principles

### 1.1 Design Philosophy

DuoPulse v4 is an **opinionated drum sequencer** that prioritizes:

1. **Musicality over flexibility**: Every output should be danceable/usable. No "probability soup."
2. **Playability**: Controls map directly to musical intent. ENERGY = tension. FLAVOR = feel.
3. **Deterministic variation**: Same settings + same seed = identical output. Variation is controlled, not random.
4. **Constraint-first generation**: Define what's *possible* (eligibility), then what's *probable* (weights).
5. **Hit budgets over coin flips**: Guarantee density matches intent; vary placement, not count.

### 1.2 Genre Targets

The sequencer is optimized for:
- **Techno**: Four-on-floor, driving, minimal-to-industrial
- **Tribal/Broken Techno**: Syncopated, polyrhythmic, off-beat emphasis
- **IDM/Glitch**: Displaced, fragmented, controlled chaos

### 1.3 Key Innovations (vs v2/v3)

| Problem | v2 Issue | v3 Issue | v4 Solution |
|---------|----------|----------|-------------|
| Pattern variety | Discrete, abrupt transitions | Too much variety, hard to control | 2D field with smooth morphing |
| Musicality | Good patterns but hard to navigate | Probability can produce bad patterns | Eligibility masks + guard rails |
| Density control | Unpredictable | Clumping/silence from coin flips | Hit budgets with weighted sampling |
| Genre coherence | Terrain/grid mismatch | Genre emerges unpredictably | FLAVOR selects constraints; BROKEN is timing-only |
| Repeatability | No determinism | DRIFT=0 works but fragile | Seed-controlled everything |

---

## 2. Architecture Overview

### 2.1 High-Level Pipeline

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           CONTROL LAYER                                  │
│                                                                          │
│   ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐                    │
│   │ ENERGY  │  │ FLAVOR  │  │ FIELD X │  │ FIELD Y │                    │
│   │  (K1)   │  │  (K2)   │  │  (K3)   │  │  (K4)   │                    │
│   └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘                    │
│        │            │            │            │                          │
│        ▼            │            └──────┬─────┘                          │
│   ┌─────────┐       │                   │                                │
│   │  ZONE   │       │                   ▼                                │
│   │ COMPUTE │       │        ┌────────────────────┐                      │
│   └────┬────┘       │        │  ARCHETYPE MORPH   │                      │
│        │            │        │ (winner-take-more) │                      │
│        │            │        └──────────┬─────────┘                      │
└────────┼────────────┼───────────────────┼────────────────────────────────┘
         │            │                   │
         ▼            ▼                   ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         GENERATION LAYER                                 │
│                                                                          │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐             │
│  │   HIT BUDGET   │  │  ELIGIBILITY   │  │  STEP WEIGHTS  │             │
│  │ (from ENERGY   │  │     MASK       │  │ (from morph +  │             │
│  │  + BALANCE)    │  │ (from ENERGY   │  │  FLAVOR)       │             │
│  │                │  │  + FLAVOR)     │  │                │             │
│  └───────┬────────┘  └───────┬────────┘  └───────┬────────┘             │
│          │                   │                   │                       │
│          └───────────────────┼───────────────────┘                       │
│                              ▼                                           │
│                   ┌─────────────────────┐                                │
│                   │  WEIGHTED SAMPLING  │◄──── DRIFT seed control        │
│                   │  (Gumbel Top-K)     │                                │
│                   │  + spacing rules    │                                │
│                   └──────────┬──────────┘                                │
│                              │                                           │
│                              ▼                                           │
│                   ┌─────────────────────┐                                │
│                   │  VOICE RELATION   │ (archetype-driven)             │
│                   └──────────┬──────────┘                                │
│                              │                                           │
│                              ▼                                           │
│                   ┌─────────────────────┐                                │
│                   │  SOFT REPAIR PASS   │ (bias rescue steps)            │
│                   └──────────┬──────────┘                                │
│                              │                                           │
│                              ▼                                           │
│                   ┌─────────────────────┐                                │
│                   │  HARD GUARD RAILS   │ (final constraints)            │
│                   └──────────┬──────────┘                                │
└──────────────────────────────┼───────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          TIMING LAYER                                    │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                       BROKEN STACK                                  │ │
│  │  swing → microtiming jitter → step displacement → velocity chaos   │ │
│  │  (all bounded by zone, all deterministic per seed)                 │ │
│  └────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          OUTPUT LAYER                                    │
│                                                                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │ GATE 1  │ │ GATE 2  │ │ OUT L   │ │ OUT R   │ │  CV 1   │           │
│  │ Anchor  │ │ Shimmer │ │ Anc Vel │ │ Shm Vel │ │   AUX   │           │
│  │  Trig   │ │  Trig   │ │  (S&H)  │ │  (S&H)  │ │         │           │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘           │
│                                                                          │
│  CV_OUT_2 = LED (visual feedback, not shown in signal flow)              │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Processing Flow Per Bar

1. **Compute hit budgets** for Anchor, Shimmer, Aux based on ENERGY + BALANCE + zone
2. **Compute eligibility mask** based on ENERGY + FLAVOR (which steps *can* fire)
3. **Get blended weights** from pattern field based on FIELD X/Y position
4. **Select hits via Gumbel Top-K** sampling (deterministic, seeded by DRIFT)
5. **Apply voice relationship** (archetype-driven, suppress/boost based on voice relationship)
6. **Soft repair pass** (bias rescue steps if constraints nearly violated)
7. **Hard guard rails** (force corrections only if still violating)
8. **Store hit masks** for the bar
9. **On each step**: apply BROKEN timing stack, output triggers + CVs

---

## 3. Hardware Interface

### 3.1 Patch.init() Hardware Map

| Hardware | Label | Function |
|----------|-------|----------|
| Knob 1 | CTRL_1 | K1: ENERGY / PUNCH (shifted) |
| Knob 2 | CTRL_2 | K2: BUILD / GENRE (shifted) |
| Knob 3 | CTRL_3 | K3: FIELD X / DRIFT (shifted) |
| Knob 4 | CTRL_4 | K4: FIELD Y / BALANCE (shifted) |
| CV In 1 | CV_1 | ENERGY CV modulation |
| CV In 2 | CV_2 | BUILD CV modulation |
| CV In 3 | CV_3 | FIELD X CV modulation |
| CV In 4 | CV_4 | FIELD Y CV modulation |
| Audio In L | IN_L | FILL CV input (0-5V = fill intensity, pressure-sensitive) |
| Audio In R | IN_R | FLAVOR CV input (0-5V = straight→broken) |
| Gate In 1 | GATE_IN_1 | Clock input |
| Gate In 2 | GATE_IN_2 | Reset input |
| Gate Out 1 | GATE_OUT_1 | Anchor trigger |
| Gate Out 2 | GATE_OUT_2 | Shimmer trigger |
| CV Out 1 | CV_OUT_1 | AUX output (clock if internal, else AUX mode) |
| CV Out 2 | CV_OUT_2 | **LED output** (directly controlled) |
| Audio Out L | OUT_L | Anchor velocity (0-5V, sample & hold) |
| Audio Out R | OUT_R | Shimmer velocity (0-5V, sample & hold) |
| Button | SW_1 | Shift (hold) / Fill (tap) / Reseed (double-tap) |
| Switch | SW_2 | Performance (A) / Config (B) mode |

### 3.2 CV Input Processing

CV inputs 1-4 directly modulate the four main performance parameters:

| CV Input | Modulates | Behavior |
|----------|-----------|----------|
| CV_1 | ENERGY | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_2 | BUILD | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_3 | FIELD X | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_4 | FIELD Y | Bipolar: 0V = no mod, ±5V = ±50% |

```cpp
float ProcessCVModulation(float knobValue, float cvVoltage) {
    // CV is bipolar: -5V to +5V maps to -0.5 to +0.5 modulation
    float cvNormalized = cvVoltage / 10.0f;  // -0.5 to +0.5
    return Clamp(knobValue + cvNormalized, 0.0f, 1.0f);
}
```

### 3.3 Audio Input Processing

Audio inputs are repurposed as CV inputs for additional control:

| Audio Input | Function | Behavior |
|-------------|----------|----------|
| IN_L | **Fill CV** | Gate (>1V) triggers fill; CV level (0-5V) = fill intensity |
| IN_R | **Flavor CV** | 0-5V maps to FLAVOR/BROKEN parameter (straight → broken) |

#### Fill Trigger Input (IN_L)

Allows external control of fill behavior via gate or pressure-sensitive CV:

```cpp
struct FillInputState {
    bool gateActive;
    float intensity;  // 0-1, from CV level
    float threshold;  // Gate detection threshold (default 0.2 = 1V)
};

void ProcessFillInput(FillInputState& state, float inputVoltage) {
    float normalized = inputVoltage / 5.0f;  // Assume 0-5V range
    
    // Gate detection
    state.gateActive = normalized > state.threshold;
    
    // Intensity from CV level (for pressure-sensitive pads)
    if (state.gateActive) {
        state.intensity = Clamp((normalized - state.threshold) / (1.0f - state.threshold), 0.0f, 1.0f);
    } else {
        state.intensity = 0.0f;
    }
}
```

When fill input is active:
- Overrides button-triggered fill behavior
- `intensity` scales fill density multiplier (0% = normal, 100% = max fill)
- Respects guard rails (fills are still musical)

#### Flavor CV Input (IN_R)

Allows external control of the FLAVOR/BROKEN parameter via CV:

```cpp
float ProcessFlavorCV(float inputVoltage, float knobGenre, bool cvPatched) {
    if (!cvPatched) {
        // No external CV, flavor derived from genre position
        return GetDefaultFlavorForGenre(knobGenre);
    }
    
    // CV directly controls flavor (0-5V → 0-1)
    return Clamp(inputVoltage / 5.0f, 0.0f, 1.0f);
}

float GetDefaultFlavorForGenre(float genreNorm) {
    // Default flavor increases with genre: Techno=low, IDM=high
    if (genreNorm < 0.33f) return 0.2f;       // Techno: mostly straight
    if (genreNorm < 0.67f) return 0.45f;      // Tribal: moderately broken
    return 0.7f;                               // IDM: quite broken
}
```

When Flavor CV is patched:
- Overrides the default genre-derived flavor
- Allows real-time modulation of timing feel
- 0V = completely straight, 5V = maximum broken
- Works well with LFOs for evolving grooves

---

## 4. Control System

### 4.1 Mode Overview

| Mode | Switch Position | Knob Behavior |
|------|-----------------|---------------|
| **Performance** | A (up) | ENERGY / BUILD / FIELD X / FIELD Y |
| **Config** | B (down) | PATTERN LENGTH / PHRASE LENGTH / AUX MODE / SWING BASE |

### 4.2 Performance Mode Controls — Ergonomic Pairings

Each knob controls a conceptual "domain" with primary/shift being two aspects of that domain:

#### Primary Controls (no shift) — CV-able

| Knob | Domain | Parameter | Range | Function |
|------|--------|-----------|-------|----------|
| K1 | Intensity | **ENERGY** | 0-100% | Hit density: how many hits per bar |
| K2 | Drama | **BUILD** | 0-100% | Phrase arc: flat (0%) → climactic (100%) |
| K3 | Pattern | **FIELD X** | 0-100% | Syncopation: straight → broken |
| K4 | Texture | **FIELD Y** | 0-100% | Complexity: sparse → dense |

#### Shift Controls (button held + knob)

| Combo | Domain | Parameter | Range | Function |
|-------|--------|-----------|-------|----------|
| Shift+K1 | Intensity | **PUNCH** | 0-100% | Velocity dynamics: flat (0%) → punchy (100%) |
| Shift+K2 | Drama | **GENRE** | 3 zones | Style bank: Techno / Tribal / IDM |
| Shift+K3 | Pattern | **DRIFT** | 0-100% | Evolution: locked (0%) → evolving (100%) |
| Shift+K4 | Texture | **BALANCE** | 0-100% | Voice ratio: anchor-heavy (0%) → shimmer-heavy (100%) |

#### Why These Pairings Work

**K1 (Intensity)**: ENERGY = "how many hits", PUNCH = "how hard those hits are"  
**K2 (Drama)**: BUILD = "how dramatic the phrase", GENRE = "what style of drama"  
**K3 (Pattern)**: FIELD X = "where in grid" (spatial), DRIFT = "how it changes" (temporal)  
**K4 (Texture)**: FIELD Y = "how complex", BALANCE = "which voice dominates"

### 4.3 PUNCH Parameter (Velocity Dynamics)

PUNCH controls how dynamic the groove feels—the contrast between loud and soft hits:

```
PUNCH = 0%:   Flat dynamics. All hits similar velocity. Hypnotic, machine-like.
              ●●●●●●●● (all same intensity)

PUNCH = 50%:  Normal dynamics. Standard groove with natural accents.
              ●○●○●○●○ (accented and normal hits)

PUNCH = 100%: Maximum dynamics. Accents POP, ghosts nearly silent.
              ●  ● ●  ● (huge velocity differences)
```

```cpp
struct PunchParams {
    float accentProbability;  // How often hits are accented
    float velocityFloor;      // Minimum velocity for non-accented hits
    float accentBoost;        // How much louder accents are
    float velocityVariation;  // Random variation range
};

PunchParams ComputePunch(float punch) {
    return {
        .accentProbability = 0.15f + punch * 0.35f,   // 15% to 50%
        .velocityFloor = 0.7f - punch * 0.4f,         // 70% down to 30%
        .accentBoost = 0.1f + punch * 0.25f,          // +10% to +35%
        .velocityVariation = 0.05f + punch * 0.15f   // ±5% to ±20%
    };
}
```

### 4.4 BUILD Parameter (Phrase Dynamics)

BUILD controls the narrative arc of each phrase—how much tension builds toward the end:

```
BUILD = 0%:   Flat throughout. No builds, no fills. Pure repetition.
              ████████████████ (constant energy)

BUILD = 50%:  Subtle build. Slight density increase, fills at phrase end.
              ████████████▲▲▲▲ (gentle rise)

BUILD = 100%: Dramatic arc. Big builds, intense fills, energy peaks.
              ████▲▲▲▲▲▲▲▲████ (tension → release)
```

**What BUILD affects over the course of a phrase:**
- Density multiplier (more hits toward end)
- Fill probability and intensity
- Accent probability increase
- Timing looseness (more humanization at phrase end)
- Aux lane activity boost

```cpp
struct BuildModifiers {
    float densityMultiplier;    // 1.0 at phrase start, up to 1.5 at end
    float fillProbability;      // Chance of fills at phrase boundaries
    float fillIntensity;        // How dramatic fills are (affects budget)
    float accentBoost;          // Extra accent probability toward phrase end
    float timingLooseness;      // How much BROKEN effect increases
};

BuildModifiers ComputeBuildModifiers(float build, float phraseProgress) {
    // phraseProgress = 0.0 at phrase start, 1.0 at phrase end
    // Exponential curve for dramatic effect
    float curve = phraseProgress * phraseProgress;
    
    return {
        .densityMultiplier = 1.0f + build * curve * 0.5f,        // Up to 1.5x
        .fillProbability = build * curve,                         // 0% to 100%
        .fillIntensity = build,                                   // Direct
        .accentBoost = build * curve * 0.3f,                     // Up to +30%
        .timingLooseness = build * curve * 0.25f                 // Up to +25%
    };
}
```

**BUILD + Fill CV Interaction:**
- BUILD sets the **automatic** fill intensity (phrase-driven fills)
- Fill CV (Audio In L) **adds on top** (manual fills)

| BUILD | Fill CV | Result |
|-------|---------|--------|
| 0% | 0V | No fills ever |
| 0% | 5V | Fills only when you press |
| 100% | 0V | Automatic climactic fills |
| 50% | 3V | Moderate auto-fills + boosted by pressure |

### 4.4.1 Control Interaction Matrix

The four shift controls create an expressive performance space:

```
                    Low BUILD              High BUILD
                 (static phrase)        (dramatic arc)
                ┌─────────────────────┬─────────────────────┐
    Low ENERGY  │  Minimal, hypnotic  │  Subtle swells      │
    (sparse)    │  Locked groove      │  Gentle fills       │
                ├─────────────────────┼─────────────────────┤
    High ENERGY │  Dense, driving     │  Climactic, intense │
    (busy)      │  Relentless         │  Big builds & drops │
                └─────────────────────┴─────────────────────┘

                    Low PUNCH              High PUNCH
                 (flat dynamics)        (punchy dynamics)
                ┌─────────────────────┬─────────────────────┐
    Low DRIFT   │  Robotic, precise   │  Punchy but locked  │
    (locked)    │  Machine loop       │  Consistent groove  │
                ├─────────────────────┼─────────────────────┤
    High DRIFT  │  Evolving, subtle   │  Alive, expressive  │
    (evolving)  │  Shifting textures  │  Human drummer feel │
                └─────────────────────┴─────────────────────┘
```

### 4.4.2 Performance Scenarios

**Scenario 1: Building to a Drop**
1. Start: ENERGY=40%, BUILD=20%, PUNCH=50%
2. Building: Sweep BUILD up to 80% over 16 bars (or let CV 2 do it automatically!)
3. Pre-drop: ENERGY jumps to 70%, BUILD at max
4. Drop: ENERGY=100%, BUILD=30% (dense but not building), PUNCH=80%

**Scenario 2: Breakdown**
1. Main groove: ENERGY=60%, BUILD=50%
2. Breakdown: ENERGY drops to 20%, BUILD drops to 10%
3. During breakdown: Sweep FIELD X/Y for pattern variations
4. Re-entry: ENERGY back up, BUILD spikes for dramatic return

**Scenario 3: Evolving Texture (Patched Modulation)**
- CV 2 = Slow LFO → BUILD breathes automatically (phrase drama ebbs and flows)
- Audio In R = Faster LFO → FLAVOR shifts between straight and broken
- Result: Living, breathing drum pattern that evolves without touching anything

**Scenario 4: Genre-Morphing Set**
1. Start with GENRE=Techno, FIELD at [0,1] (driving)
2. Gradually shift GENRE toward Tribal as energy builds
3. For breakdown, sweep FIELD X toward broken while reducing ENERGY
4. Peak: GENRE=IDM, high ENERGY, high BUILD, high PUNCH

### 4.5 Config Mode Controls — Domain-Based

Each knob controls a conceptual domain, with primary/shift being related aspects:

#### K1: GRID Domain (Loop Architecture)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **PATTERN LENGTH** | 16 / 24 / 32 / 64 | Steps per bar |
| Shift | **PHRASE LENGTH** | 1 / 2 / 4 / 8 | Bars per phrase |

These define the fundamental temporal structure. Together they determine total loop length (e.g., 16 steps × 4 bars = 64-step phrase).

#### K2: TIMING Domain (Feel)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **SWING** | 0-100% | Base swing amount (combined with FLAVOR) |
| Shift | **CLOCK DIV** | 1 / 2 / 4 / 8 | Internal clock division (ignored with ext clock) |

Both affect temporal feel. CLOCK DIV is appropriately hidden on shift since it only matters without external clock.

#### K3: OUTPUT Domain (Signal Configuration)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **AUX MODE** | Hat / Fill Gate / Phrase CV / Event | What AUX output does |
| Shift | **AUX DENSITY** | 50% / 100% / 150% / 200% | Aux hit budget multiplier |

Both configure the AUX output. AUX DENSITY is especially useful for IDM — crank it up for busy hat/ratchet patterns.

```cpp
enum class AuxDensity {
    SPARSE = 0,   // 50% of normal budget
    NORMAL,       // 100% (default)
    DENSE,        // 150%
    BUSY          // 200% — ratchet/trill territory
};

float GetAuxDensityMultiplier(AuxDensity density) {
    switch (density) {
        case AuxDensity::SPARSE: return 0.5f;
        case AuxDensity::NORMAL: return 1.0f;
        case AuxDensity::DENSE:  return 1.5f;
        case AuxDensity::BUSY:   return 2.0f;
    }
    return 1.0f;
}
```

#### K4: BEHAVIOR Domain (Module Response)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **RESET MODE** | Phrase / Bar / Step | What reset input does |
| Shift | **VOICE COUPLING** | Independent / Interlock / Shadow | Voice relationship override |

**RESET MODE** determines granularity of reset:
- **Phrase**: Reset to start of phrase (most musical)
- **Bar**: Reset to start of current bar
- **Step**: Immediate reset to step 0

**VOICE COUPLING** overrides archetype's default voice relationship:
- **Independent**: Voices fire freely, can overlap (0-33%)
- **Interlock**: Suppress simultaneous hits, call-response feel (33-67%)
- **Shadow**: Shimmer echoes anchor with 1-step delay (67-100%)

```cpp
enum class VoiceCoupling {
    INDEPENDENT,  // Voices can overlap freely
    INTERLOCK,    // Suppress simultaneous, call-response
    SHADOW        // Shimmer follows anchor by 1 step
};
```

#### Hardware Constants (Not User-Configurable)

Some parameters are hardcoded but adjustable at compile time for firmware iteration:

```cpp
// Trigger output configuration
constexpr int kTriggerLengthMs = 5;        // 5ms works with most modules
constexpr float kVelocityRangeV = 5.0f;    // 0-5V output range

// Convert to samples at runtime
int triggerLengthSamples = (kTriggerLengthMs * sampleRate) / 1000;
```

### 4.6 Button Behavior

| Gesture | Action | Timing |
|---------|--------|--------|
| **Tap** (<200ms) | Queue fill for next phrase | Immediate feedback via LED |
| **Hold** (>200ms) | Shift modifier active | LED dims while held |
| **Double-tap** (<400ms between) | Reseed pattern at next phrase boundary | LED flashes 100% |

### 4.7 Energy Zones

ENERGY doesn't just scale density—it changes behavioral rules:

| Zone | ENERGY Range | Characteristics |
|------|--------------|-----------------|
| **MINIMAL** | 0-20% | Sparse, skeleton only, large gaps allowed, tight timing |
| **GROOVE** | 20-50% | Stable, danceable, locked pattern, moderate fills, tight timing |
| **BUILD** | 50-75% | Increasing ghosts, phrase-end fills, aux active, timing loosens |
| **PEAK** | 75-100% | Maximum activity, ratchets allowed, all voices busy, expressive timing |

```cpp
enum class EnergyZone {
    MINIMAL,  // 0-20%
    GROOVE,   // 20-50%
    BUILD,    // 50-75%
    PEAK      // 75-100%
};

EnergyZone GetZone(float energy) {
    if (energy < 0.20f) return EnergyZone::MINIMAL;
    if (energy < 0.50f) return EnergyZone::GROOVE;
    if (energy < 0.75f) return EnergyZone::BUILD;
    return EnergyZone::PEAK;
}
```

---

## 5. Pattern Field System

### 5.1 3×3 Archetype Grid

Each genre has a 3×3 grid of archetypes (9 per genre, 27 total):

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

### 5.2 Archetype DNA Structure

Each archetype stores more than just step weights:

```cpp
struct ArchetypeDNA {
    // Step weights for each voice (32 steps max)
    float anchorWeights[32];     // 0.0-1.0 probability weight per step
    float shimmerWeights[32];    // 0.0-1.0 probability weight per step
    float auxWeights[32];        // 0.0-1.0 probability weight per step (hat lane)
    
    // Accent eligibility (which steps CAN accent, not which WILL)
    uint32_t anchorAccentMask;   // Bitmask: 1 = accent-eligible
    uint32_t shimmerAccentMask;
    
    // Timing characteristics
    float swingAmount;           // 0.0-1.0, base swing for this archetype
    float swingPattern;          // Which beats swing (0=8ths, 1=16ths, 2=mixed)
    
    // Voice relationship defaults
    float defaultCouple;         // 0.0-1.0, suggested COUPLE value
    
    // Fill behavior
    float fillDensityMultiplier; // How much denser during fills
    uint32_t ratchetEligibleMask;// Which steps can ratchet
    
    // Metadata
    uint8_t gridX;               // Position in grid (0-2)
    uint8_t gridY;               // Position in grid (0-2)
};
```

### 5.3 Winner-Take-More Blending

When FIELD X/Y is between grid points, blend using softmax with temperature:

```cpp
ArchetypeDNA BlendArchetypes(float fieldX, float fieldY, const ArchetypeDNA grid[3][3]) {
    // Convert 0-1 range to grid coordinates
    float gx = fieldX * 2.0f;  // 0-2
    float gy = fieldY * 2.0f;  // 0-2
    
    // Find the four surrounding grid points
    int x0 = (int)gx;
    int y0 = (int)gy;
    int x1 = min(x0 + 1, 2);
    int y1 = min(y0 + 1, 2);
    
    // Compute distances to each corner
    float fx = gx - x0;
    float fy = gy - y0;
    
    // Bilinear weights
    float w00 = (1 - fx) * (1 - fy);
    float w10 = fx * (1 - fy);
    float w01 = (1 - fx) * fy;
    float w11 = fx * fy;
    
    // Apply softmax with temperature for "winner-take-more"
    float temperature = 0.4f;  // Lower = more winner-take-all
    float weights[4] = {w00, w10, w01, w11};
    SoftmaxWithTemperature(weights, 4, temperature);
    
    // Blend DNA properties
    ArchetypeDNA result = {};
    
    const ArchetypeDNA* sources[4] = {
        &grid[y0][x0], &grid[y0][x1], 
        &grid[y1][x0], &grid[y1][x1]
    };
    
    for (int step = 0; step < 32; step++) {
        // Blend step weights (these interpolate well)
        result.anchorWeights[step] = 
            weights[0] * sources[0]->anchorWeights[step] +
            weights[1] * sources[1]->anchorWeights[step] +
            weights[2] * sources[2]->anchorWeights[step] +
            weights[3] * sources[3]->anchorWeights[step];
        
        result.shimmerWeights[step] = 
            weights[0] * sources[0]->shimmerWeights[step] +
            weights[1] * sources[1]->shimmerWeights[step] +
            weights[2] * sources[2]->shimmerWeights[step] +
            weights[3] * sources[3]->shimmerWeights[step];
            
        result.auxWeights[step] = 
            weights[0] * sources[0]->auxWeights[step] +
            weights[1] * sources[1]->auxWeights[step] +
            weights[2] * sources[2]->auxWeights[step] +
            weights[3] * sources[3]->auxWeights[step];
    }
    
    // Blend continuous properties
    result.swingAmount = 
        weights[0] * sources[0]->swingAmount +
        weights[1] * sources[1]->swingAmount +
        weights[2] * sources[2]->swingAmount +
        weights[3] * sources[3]->swingAmount;
    
    result.defaultCouple = 
        weights[0] * sources[0]->defaultCouple +
        weights[1] * sources[1]->defaultCouple +
        weights[2] * sources[2]->defaultCouple +
        weights[3] * sources[3]->defaultCouple;
    
    // Select discrete properties from DOMINANT archetype (don't blend)
    int dominant = 0;
    for (int i = 1; i < 4; i++) {
        if (weights[i] > weights[dominant]) dominant = i;
    }
    result.anchorAccentMask = sources[dominant]->anchorAccentMask;
    result.shimmerAccentMask = sources[dominant]->shimmerAccentMask;
    result.ratchetEligibleMask = sources[dominant]->ratchetEligibleMask;
    
    return result;
}

void SoftmaxWithTemperature(float* weights, int n, float temperature) {
    float maxW = weights[0];
    for (int i = 1; i < n; i++) maxW = max(maxW, weights[i]);
    
    float sum = 0;
    for (int i = 0; i < n; i++) {
        weights[i] = expf((weights[i] - maxW) / temperature);
        sum += weights[i];
    }
    for (int i = 0; i < n; i++) {
        weights[i] /= sum;
    }
}
```

### 5.4 Genre-Specific Grids

Each genre has its own 3×3 grid tuned to that style:

#### Techno Grid Characteristics

| Position | Name | Anchor Character | Shimmer Character |
|----------|------|------------------|-------------------|
| [0,0] | Minimal | Quarter notes only | Sparse, 2&4 |
| [1,0] | Steady | Quarter + some 8ths | Backbeat + off-8ths |
| [2,0] | Displaced | Skipped beat 3 | Anticipated snares |
| [0,1] | Driving | Straight 8ths | Steady 8th hats |
| [1,1] | Groovy | Swung 8ths | Shuffled backbeat |
| [2,1] | Broken | Missing downbeats | Syncopated claps |
| [0,2] | Busy | 16th kicks | Driving 16ths |
| [1,2] | Polyrhythm | 3-feel over 4 | Counter-rhythm |
| [2,2] | Chaos | Irregular clusters | Fragmented |

#### Tribal Grid Characteristics

| Position | Name | Anchor Character | Shimmer Character |
|----------|------|------------------|-------------------|
| [0,0] | Minimal | Heartbeat pulse | Call only |
| [1,0] | Clave | 3-2 son clave | Response pattern |
| [2,0] | Displaced | Anticipated | Delayed response |
| [0,1] | Driving | Tresillo base | Interlocking |
| [1,1] | Groovy | Clave + variations | Call-response |
| [2,1] | Broken | Fragmented clave | Displaced response |
| [0,2] | Busy | Dense tresillo | Busy interlocks |
| [1,2] | Polyrhythm | 6/8 over 4/4 | Cross-rhythm |
| [2,2] | Chaos | Afro-Cuban chaos | Maximum interlock |

#### IDM Grid Characteristics

| Position | Name | Anchor Character | Shimmer Character |
|----------|------|------------------|-------------------|
| [0,0] | Minimal | Sparse, irregular | Glitchy accents |
| [1,0] | Steady | Broken 4/4 | Stuttered |
| [2,0] | Displaced | Extreme displacement | Reverse feel |
| [0,1] | Driving | Drill-style | Rapid-fire |
| [1,1] | Groovy | Warped swing | Phased |
| [2,1] | Broken | Fragmented | Scattered |
| [0,2] | Busy | Granular density | Noise-like |
| [1,2] | Polyrhythm | Irrational meters | Cross-phase |
| [2,2] | Chaos | Maximum entropy | Controlled chaos |

---

## 6. Generation Pipeline

### 6.1 Hit Budget Calculation

```cpp
struct BarBudget {
    int anchorHits;
    int shimmerHits;
    int auxHits;
    // Note: Accent counts are computed separately using PunchParams
};

BarBudget ComputeBarBudget(float energy, float balance, EnergyZone zone, 
                           int barInPhrase, int phraseLength,
                           const BuildModifiers& buildMods, float auxDensityMult) {
    BarBudget budget;
    
    // Base hit counts from energy (per 16 steps, scale for other lengths)
    int baseHits;
    switch (zone) {
        case EnergyZone::MINIMAL: baseHits = 2 + (int)(energy * 5);  break;  // 2-3
        case EnergyZone::GROOVE:  baseHits = 4 + (int)(energy * 8);  break;  // 4-6
        case EnergyZone::BUILD:   baseHits = 6 + (int)(energy * 12); break;  // 6-9
        case EnergyZone::PEAK:    baseHits = 8 + (int)(energy * 16); break;  // 8-12
    }
    
    // Balance splits between anchor and shimmer
    float anchorRatio = 0.3f + balance * 0.4f;  // 0.3 to 0.7
    
    budget.anchorHits = (int)(baseHits * anchorRatio);
    budget.shimmerHits = (int)(baseHits * (1.0f - anchorRatio));
    
    // Aux is denser, especially at higher energy, scaled by AUX DENSITY config
    float auxBase = baseHits * (0.5f + energy * 0.5f);
    budget.auxHits = (int)(auxBase * auxDensityMult);
    
    // Accents: subset of hits, scaled by PUNCH
    // (PunchParams.accentProbability is passed separately to accent selection)
    
    // BUILD modifier: increase density toward phrase end
    float densityMult = buildMods.densityMultiplier;
    budget.anchorHits = (int)(budget.anchorHits * densityMult);
    budget.shimmerHits = (int)(budget.shimmerHits * densityMult);
    budget.auxHits = (int)(budget.auxHits * densityMult);
    
    // Fill bias: last bar of phrase gets more hits (scaled by BUILD)
    bool isLastBar = (barInPhrase == phraseLength - 1);
    if (isLastBar && zone >= EnergyZone::BUILD) {
        int fillBoost = (int)(2 + buildMods.fillIntensity * 3);  // 2-5 extra hits
        budget.anchorHits += fillBoost;
        budget.shimmerHits += fillBoost;
        budget.auxHits += (int)((fillBoost + 1) * auxDensityMult);  // Scale fill boost too
    }
    
    // Ensure minimums
    budget.anchorHits = max(1, budget.anchorHits);
    budget.shimmerHits = max(zone >= EnergyZone::GROOVE ? 1 : 0, budget.shimmerHits);
    
    return budget;
}
```

### 6.2 Eligibility Mask Computation

```cpp
// Standard mask constants (for 32-step patterns)
constexpr uint32_t kDownbeatMask     = 0x00010001;  // Steps 0, 16
constexpr uint32_t kHalfNoteMask     = 0x01010101;  // Steps 0, 8, 16, 24
constexpr uint32_t kQuarterNoteMask  = 0x11111111;  // Steps 0, 4, 8, 12, 16, 20, 24, 28
constexpr uint32_t kEighthNoteMask   = 0x55555555;  // Every other step
constexpr uint32_t kSixteenthMask    = 0xFFFFFFFF;  // All steps
constexpr uint32_t kOffbeatMask      = 0xAAAAAAAA;  // Off-beat 16ths
constexpr uint32_t kBackbeatMask     = 0x00100010;  // Steps 4, 20 (snare positions)

uint32_t ComputeEligibilityMask(float energy, float flavor, EnergyZone zone, 
                                 Voice voice, Genre genre) {
    uint32_t mask = 0;
    
    // Energy determines metric depth of eligible positions
    if (energy > 0.05f) mask |= kDownbeatMask;
    if (energy > 0.15f) mask |= kHalfNoteMask;
    if (energy > 0.25f) mask |= kQuarterNoteMask;
    if (energy > 0.40f) mask |= kEighthNoteMask;
    if (energy > 0.60f) mask |= kSixteenthMask;
    
    // Flavor modifies eligible positions
    if (flavor > 0.3f) {
        // Broken: allow off-beat emphasis
        mask |= kOffbeatMask;
    }
    if (flavor > 0.6f) {
        // IDM-ish: allow any position
        mask = kSixteenthMask;
    }
    
    // Voice-specific rules
    if (voice == Voice::ANCHOR) {
        // Anchor always eligible on downbeats (for guard rails)
        mask |= kDownbeatMask;
        
        // Techno anchor: quarter notes always eligible
        if (genre == Genre::TECHNO && zone >= EnergyZone::GROOVE) {
            mask |= kQuarterNoteMask;
        }
    }
    
    if (voice == Voice::SHIMMER) {
        // Backbeat always eligible for shimmer
        if (genre == Genre::TECHNO) {
            mask |= kBackbeatMask;
        }
    }
    
    // Zone-specific restrictions
    if (zone == EnergyZone::MINIMAL) {
        // Only strong beats in minimal zone
        mask &= kQuarterNoteMask;
    }
    
    return mask;
}
```

### 6.3 Gumbel Top-K Selection

Weighted sampling without replacement using Gumbel noise:

```cpp
void SelectHitsGumbelTopK(const float* weights, uint32_t eligibilityMask,
                          int budget, uint32_t& hitMask, uint32_t seed,
                          int patternLength) {
    hitMask = 0;
    
    if (budget <= 0) return;
    
    // Compute score for each step: log(weight) + Gumbel noise
    float scores[32];
    for (int i = 0; i < patternLength; i++) {
        if (!(eligibilityMask & (1 << i))) {
            scores[i] = -1000.0f;  // Ineligible
            continue;
        }
        
        float w = max(weights[i], 0.001f);  // Avoid log(0)
        float gumbel = -logf(-logf(HashToFloat(seed, i)));  // Gumbel noise
        scores[i] = logf(w) + gumbel;
    }
    
    // Apply spacing penalty: reduce score of steps adjacent to high scorers
    // This prevents clumping
    float spacingPenalty = 2.0f;
    float penalizedScores[32];
    memcpy(penalizedScores, scores, sizeof(scores));
    
    // Select top K
    for (int k = 0; k < budget; k++) {
        // Find highest remaining score
        int best = -1;
        float bestScore = -999.0f;
        for (int i = 0; i < patternLength; i++) {
            if ((hitMask & (1 << i)) == 0 && penalizedScores[i] > bestScore) {
                bestScore = penalizedScores[i];
                best = i;
            }
        }
        
        if (best < 0) break;
        
        hitMask |= (1 << best);
        
        // Penalize adjacent steps for next selection
        if (best > 0) penalizedScores[best - 1] -= spacingPenalty;
        if (best < patternLength - 1) penalizedScores[best + 1] -= spacingPenalty;
    }
}

float HashToFloat(uint32_t seed, int step) {
    // Simple hash to [0, 1)
    uint32_t h = seed ^ (step * 2654435761u);
    h = ((h >> 16) ^ h) * 0x45d9f3b;
    h = ((h >> 16) ^ h) * 0x45d9f3b;
    h = (h >> 16) ^ h;
    return (float)(h & 0xFFFFFF) / (float)0x1000000;
}
```

### 6.4 Voice Relationship (Config-Driven)

Voice interaction is controlled by the VOICE COUPLING config parameter, which can override the archetype's default:

```cpp
// Convert config VoiceCoupling enum to float value
float GetCoupleValueFromConfig(VoiceCoupling coupling, float archetypeDefault) {
    switch (coupling) {
        case VoiceCoupling::INDEPENDENT:
            return 0.0f;   // No coupling, voices overlap freely
        case VoiceCoupling::INTERLOCK:
            return 0.35f;  // Suppress simultaneous hits, call-response
        case VoiceCoupling::SHADOW:
            return 0.75f;  // Shimmer echoes anchor with delay
    }
    return archetypeDefault;  // Fallback (shouldn't happen)
}

void ApplyVoiceRelationship(float couple, uint32_t& anchorMask, uint32_t& shimmerMask,
                            const float* anchorWeights, const float* shimmerWeights,
                            int patternLength, uint32_t seed) {
    // couple comes from config VoiceCoupling (or archetype default if not set)
    if (couple < 0.1f) return;  // Fully independent
    
    for (int step = 0; step < patternLength; step++) {
        bool anchorFires = (anchorMask & (1 << step)) != 0;
        bool shimmerFires = (shimmerMask & (1 << step)) != 0;
        
        if (couple < 0.5f) {
            // INTERLOCK mode: suppress simultaneous hits
            float suppressChance = couple * 1.6f;  // 0-80%
            if (anchorFires && shimmerFires) {
                if (HashToFloat(seed, step + 1000) < suppressChance) {
                    // Remove the weaker hit
                    if (anchorWeights[step] > shimmerWeights[step]) {
                        shimmerMask &= ~(1 << step);
                    } else {
                        anchorMask &= ~(1 << step);
                    }
                }
            }
        } else {
            // SHADOW mode: shimmer echoes anchor
            float shadowChance = (couple - 0.5f) * 2.0f;  // 0-100%
            if (anchorFires && step < patternLength - 1) {
                if (HashToFloat(seed, step + 2000) < shadowChance) {
                    shimmerMask |= (1 << (step + 1));  // Echo on next step
                }
            }
            
            // Also boost shimmer in anchor gaps at high couple
            if (!anchorFires && !shimmerFires && couple > 0.7f) {
                float gapFillChance = (couple - 0.7f) * 1.5f;  // 0-45%
                if (HashToFloat(seed, step + 3000) < gapFillChance) {
                    shimmerMask |= (1 << step);
                }
            }
        }
    }
}
```

### 6.5 Soft Repair Pass

```cpp
void SoftRepairPass(uint32_t& anchorMask, uint32_t eligibilityMask,
                    const float* weights, int patternLength, EnergyZone zone) {
    // Check if we need rescue (no hits on strong beats)
    bool hasDownbeat = (anchorMask & kDownbeatMask) != 0;
    
    if (!hasDownbeat && zone >= EnergyZone::GROOVE) {
        // Find the strongest eligible rescue step
        int bestRescue = -1;
        float bestWeight = 0;
        
        uint32_t rescueCandidates = kDownbeatMask & eligibilityMask & ~anchorMask;
        for (int i = 0; i < patternLength; i++) {
            if ((rescueCandidates & (1 << i)) && weights[i] > bestWeight) {
                bestWeight = weights[i];
                bestRescue = i;
            }
        }
        
        if (bestRescue >= 0) {
            // Swap: remove weakest current hit, add rescue
            int weakest = -1;
            float weakestWeight = 1000.0f;
            for (int i = 0; i < patternLength; i++) {
                if ((anchorMask & (1 << i)) && weights[i] < weakestWeight) {
                    weakestWeight = weights[i];
                    weakest = i;
                }
            }
            
            if (weakest >= 0 && !(kDownbeatMask & (1 << weakest))) {
                anchorMask &= ~(1 << weakest);  // Remove weak hit
                anchorMask |= (1 << bestRescue);  // Add rescue
            }
        }
    }
}
```

### 6.6 Hard Guard Rails

```cpp
struct GuardRailState {
    int stepsSinceAnchor;
    bool hadDownbeatThisBar;
    int consecutiveShimmerHits;
};

void ApplyHardGuardRails(uint32_t& anchorMask, uint32_t& shimmerMask,
                         EnergyZone zone, Genre genre, int patternLength,
                         GuardRailState& state) {
    
    // Process step by step for stateful rules
    for (int step = 0; step < patternLength; step++) {
        bool anchorFires = (anchorMask & (1 << step)) != 0;
        bool shimmerFires = (shimmerMask & (1 << step)) != 0;
        
        // RULE 1: Downbeat protection
        bool isDownbeat = (step == 0 || step == 16);
        if (isDownbeat) {
            state.hadDownbeatThisBar = false;
        }
        
        if (isDownbeat && !state.hadDownbeatThisBar && zone != EnergyZone::MINIMAL) {
            if (!anchorFires) {
                anchorMask |= (1 << step);  // Force downbeat
                anchorFires = true;
            }
        }
        if (anchorFires && isDownbeat) {
            state.hadDownbeatThisBar = true;
        }
        
        // RULE 2: Max gap (8 steps without anchor unless MINIMAL)
        if (anchorFires) {
            state.stepsSinceAnchor = 0;
        } else {
            state.stepsSinceAnchor++;
            if (state.stepsSinceAnchor > 8 && zone >= EnergyZone::GROOVE) {
                anchorMask |= (1 << step);
                state.stepsSinceAnchor = 0;
            }
        }
        
        // RULE 3: Max consecutive shimmer (4 unless PEAK)
        if (shimmerFires) {
            state.consecutiveShimmerHits++;
            if (state.consecutiveShimmerHits > 4 && zone < EnergyZone::PEAK) {
                shimmerMask &= ~(1 << step);
                state.consecutiveShimmerHits = 0;
            }
        } else {
            state.consecutiveShimmerHits = 0;
        }
        
        // RULE 4: Techno backbeat floor
        if (genre == Genre::TECHNO && zone >= EnergyZone::GROOVE) {
            if ((step == 4 || step == 12 || step == 20 || step == 28) && !shimmerFires) {
                shimmerMask |= (1 << step);
            }
        }
    }
}
```

### 6.7 DRIFT Seed System

```cpp
struct DriftState {
    uint32_t patternSeed;   // Fixed per "song", changes on reseed
    uint32_t phraseSeed;    // Changes each phrase
    int phraseCounter;
};

uint32_t SelectSeed(int step, float drift, const DriftState& state) {
    // Step stability: how "important" is this step position
    float stability = GetStepStability(step);
    
    // If stability > drift, use locked seed; else use evolving seed
    if (stability > drift) {
        return state.patternSeed;
    } else {
        return state.phraseSeed;
    }
}

float GetStepStability(int step) {
    // Downbeats are most stable, ghost positions least
    if (step == 0 || step == 16) return 1.0f;      // Bar downbeats
    if (step == 8 || step == 24) return 0.85f;     // Half notes
    if (step % 4 == 0) return 0.70f;               // Quarter notes
    if (step % 2 == 0) return 0.40f;               // 8th notes
    return 0.20f;                                   // 16th ghost positions
}

void OnPhraseEnd(DriftState& state) {
    state.phraseCounter++;
    // Generate new phrase seed from pattern seed + counter
    state.phraseSeed = state.patternSeed ^ (state.phraseCounter * 2654435761u);
}

void Reseed(DriftState& state) {
    // Called on double-tap or explicit reseed
    state.patternSeed = GetRandomSeed();  // From hardware RNG or timer
    state.phraseCounter = 0;
    state.phraseSeed = state.patternSeed;
}
```

---

## 7. Timing System (BROKEN Stack)

### 7.1 BROKEN Stack Overview

BROKEN controls four timing layers, each bounded by zone:

| Layer | BROKEN 0% | BROKEN 100% | Zone Limit |
|-------|-----------|-------------|------------|
| **Swing** | 50% (straight) | 66% (heavy triplet) | GROOVE: max 58% |
| **Microtiming Jitter** | ±0ms | ±12ms | GROOVE: max ±3ms |
| **Step Displacement** | Never | 40% chance, ±2 steps | GROOVE: never |
| **Velocity Chaos** | ±0% | ±25% | Always allowed |

### 7.2 Swing Application

```cpp
float ComputeSwing(float broken, float baseSwing, EnergyZone zone) {
    // Swing ratio: 0.5 = straight, 0.66 = triplet feel
    float targetSwing = 0.50f + broken * 0.16f;  // 50% to 66%
    
    // Blend with archetype's base swing
    float swing = baseSwing * 0.3f + targetSwing * 0.7f;
    
    // Zone clamping
    switch (zone) {
        case EnergyZone::MINIMAL:
        case EnergyZone::GROOVE:
            swing = min(swing, 0.58f);  // Keep it tight
            break;
        case EnergyZone::BUILD:
            swing = min(swing, 0.62f);
            break;
        case EnergyZone::PEAK:
            // No limit
            break;
    }
    
    return swing;
}

int ApplySwingToStep(int step, float swing, int samplesPerStep) {
    // Swing affects off-beat 8ths (steps 2, 6, 10, 14, 18, 22, 26, 30 in 32-step)
    bool isSwungStep = (step % 4 == 2);
    
    if (!isSwungStep) return 0;
    
    // Delay = (swing - 0.5) * 2 * samplesPerStep
    float delayRatio = (swing - 0.5f) * 2.0f;
    return (int)(delayRatio * samplesPerStep);
}
```

### 7.3 Microtiming Jitter (Humanization)

```cpp
int ComputeMicrotimingOffset(int step, float broken, EnergyZone zone, 
                              uint32_t seed, int sampleRate) {
    // Max jitter in milliseconds based on broken
    float maxJitterMs;
    if (broken < 0.3f) {
        maxJitterMs = 0.0f;
    } else if (broken < 0.6f) {
        maxJitterMs = (broken - 0.3f) * 20.0f;  // 0-6ms
    } else {
        maxJitterMs = 6.0f + (broken - 0.6f) * 15.0f;  // 6-12ms
    }
    
    // Zone clamping
    switch (zone) {
        case EnergyZone::MINIMAL:
            maxJitterMs = 0.0f;
            break;
        case EnergyZone::GROOVE:
            maxJitterMs = min(maxJitterMs, 3.0f);
            break;
        case EnergyZone::BUILD:
            maxJitterMs = min(maxJitterMs, 6.0f);
            break;
        case EnergyZone::PEAK:
            // No limit
            break;
    }
    
    if (maxJitterMs <= 0.0f) return 0;
    
    // Deterministic jitter from seed
    float jitterNorm = HashToFloat(seed, step + 5000) * 2.0f - 1.0f;  // -1 to +1
    float jitterMs = jitterNorm * maxJitterMs;
    
    // Convert to samples
    return (int)(jitterMs * sampleRate / 1000.0f);
}
```

### 7.4 Step Displacement

```cpp
int ComputeStepDisplacement(int step, float broken, EnergyZone zone,
                            uint32_t seed, uint32_t eligibilityMask) {
    // Displacement only at high broken and not in GROOVE or lower
    if (broken < 0.5f || zone <= EnergyZone::GROOVE) return 0;
    
    // Probability of displacement
    float displaceChance;
    if (broken < 0.75f) {
        displaceChance = (broken - 0.5f) * 0.6f;  // 0-15%
    } else {
        displaceChance = 0.15f + (broken - 0.75f) * 1.0f;  // 15-40%
    }
    
    // Check if this step should displace
    if (HashToFloat(seed, step + 6000) > displaceChance) return 0;
    
    // Displacement amount: ±1 at moderate broken, ±2 at high
    int maxDisplace = (broken > 0.85f) ? 2 : 1;
    
    // Direction from hash
    int direction = (HashToFloat(seed, step + 7000) > 0.5f) ? 1 : -1;
    int amount = 1 + (int)(HashToFloat(seed, step + 8000) * maxDisplace);
    int newStep = step + direction * amount;
    
    // Validate: new position must be eligible
    if (newStep < 0 || newStep >= 32) return 0;
    if (!(eligibilityMask & (1 << newStep))) return 0;
    
    return direction * amount;
}
```

### 7.5 Velocity Computation (PUNCH-driven)

Velocity is controlled by the PUNCH parameter, which determines the dynamic range between accented and non-accented hits:

```cpp
float ComputeVelocity(int step, bool isAccent, const PunchParams& punch,
                      const BuildModifiers& build, uint32_t seed) {
    float velocity;
    
    if (isAccent) {
        // Accented hits: high base + boost
        velocity = 0.8f + punch.accentBoost;
    } else {
        // Non-accented hits: floor (lower at high punch = more contrast)
        velocity = punch.velocityFloor;
    }
    
    // Add random variation (more at high punch)
    float variation = (HashToFloat(seed, step + 9000) * 2.0f - 1.0f) * punch.velocityVariation;
    velocity += variation;
    
    // BUILD modifier: boost accents toward phrase end
    if (isAccent) {
        velocity += build.accentBoost;
    }
    
    // Soft clamp to valid range
    return Clamp(velocity, 0.2f, 1.0f);
}
```

**PUNCH Effect on Velocity Distribution:**

```
PUNCH = 0%:   ████████████████  All hits ~70% velocity (minimal contrast)
              Hypnotic, machine-like, even dynamics

PUNCH = 50%:  ████  ████  ████  Accents ~85%, normal ~55% (moderate contrast)
              Natural groove feel

PUNCH = 100%: ██        ██      Accents ~95%, ghosts ~30% (maximum contrast)
              Punchy, aggressive, big dynamics
```

---

## 8. Output System

### 8.1 Output Mapping

| Output | Signal | Voltage Range | Update Rate |
|--------|--------|---------------|-------------|
| GATE_OUT_1 | Anchor trigger | 0V / 5V | Per step |
| GATE_OUT_2 | Shimmer trigger | 0V / 5V | Per step |
| CV_OUT_1 | AUX output | 0-5V | Mode-dependent (clock if unpatched) |
| CV_OUT_2 | LED | 0-5V (brightness) | Continuous |
| OUT_L | Anchor velocity | 0-5V (sample & hold) | On anchor trigger |
| OUT_R | Shimmer velocity | 0-5V (sample & hold) | On shimmer trigger |

### 8.2 Trigger Output

```cpp
struct TriggerState {
    bool isHigh;
    int samplesRemaining;
};

void ProcessTriggerOutput(TriggerState& state, bool shouldFire, 
                          int triggerLengthSamples, float* output) {
    if (shouldFire && !state.isHigh) {
        state.isHigh = true;
        state.samplesRemaining = triggerLengthSamples;
    }
    
    if (state.isHigh) {
        *output = 5.0f;
        state.samplesRemaining--;
        if (state.samplesRemaining <= 0) {
            state.isHigh = false;
        }
    } else {
        *output = 0.0f;
    }
}
```

### 8.3 Velocity Output (Sample & Hold)

Velocity outputs use **sample & hold** behavior—the voltage is set on trigger and held until the next trigger on that channel. This works well with external VCAs since the triggered voices already have their own amplitude envelopes.

```cpp
struct VelocityOutputState {
    float currentVoltage;     // Held value (persists between triggers)
};

void ProcessVelocityOutput(VelocityOutputState& state, bool triggerFired, 
                           float newVelocity, float* output) {
    if (triggerFired) {
        // Latch new velocity on trigger
        state.currentVoltage = newVelocity * 5.0f;  // Scale to 0-5V
    }
    
    // Output held value (persists until next trigger)
    *output = state.currentVoltage;
}
```

**Optional: Fast Attack Envelope**

For smoother transitions without audible clicks, apply a very fast attack (~1ms) when velocity changes:

```cpp
void ProcessVelocityOutputSmooth(VelocityOutputState& state, bool triggerFired,
                                  float newVelocity, float* output, 
                                  float slewRate) {
    if (triggerFired) {
        state.targetVoltage = newVelocity * 5.0f;
    }
    
    // Fast slew toward target
    if (state.currentVoltage < state.targetVoltage) {
        state.currentVoltage = min(state.currentVoltage + slewRate, 
                                   state.targetVoltage);
    } else if (state.currentVoltage > state.targetVoltage) {
        state.currentVoltage = max(state.currentVoltage - slewRate,
                                   state.targetVoltage);
    }
    
    *output = state.currentVoltage;
}
```

### 8.4 AUX Output Modes

```cpp
enum class AuxMode {
    HAT,        // 0-25%: Third trigger voice (ghost/hi-hat pattern)
    FILL_GATE,  // 25-50%: High during fill zones
    PHRASE_CV,  // 50-75%: 0-5V ramp over phrase
    EVENT       // 75-100%: Trigger on "interesting" moments
};

// Main AUX output function with clock fallback
float ComputeAuxOutput(AuxMode mode, const SequencerState& state, 
                       bool externalClockPatched, bool clockHigh) {
    // If no external clock, AUX outputs clock signal (mode ignored)
    if (!externalClockPatched) {
        return clockHigh ? 5.0f : 0.0f;
    }
    
    // External clock patched: use selected AUX mode
    return ComputeAuxModeOutput(mode, state);
}

float ComputeAuxModeOutput(AuxMode mode, const SequencerState& state) {
    switch (mode) {
        case AuxMode::HAT: {
            // Third voice: uses aux hit mask + velocity
            if (state.auxFires) {
                return state.auxVelocity * 5.0f;
            }
            return 0.0f;
        }
        
        case AuxMode::FILL_GATE: {
            // High during fill/build zones
            bool inFill = state.inFillZone || 
                         (state.zone >= EnergyZone::BUILD && state.isLastBarOfPhrase);
            return inFill ? 5.0f : 0.0f;
        }
        
        case AuxMode::PHRASE_CV: {
            // Smooth ramp over phrase (0-5V)
            return state.phraseProgress * 5.0f;
        }
        
        case AuxMode::EVENT: {
            // Trigger on interesting moments
            bool isEvent = state.isFillStart ||
                          state.isAccentHit ||
                          state.isSectionBoundary ||
                          state.isRatchet;
            return isEvent ? 5.0f : 0.0f;
        }
    }
    return 0.0f;
}
```

---

## 9. LED Feedback System

### 9.1 LED Output (CV_OUT_2)

The single dimmable LED provides visual feedback through brightness levels:

| Brightness | Meaning |
|------------|---------|
| 0% | Off (no activity) |
| 30% | Shimmer trigger |
| 80% | Anchor trigger |
| 100% | Flash (mode change, reset, reseed) |
| Pulse | Live fill mode active |
| Gradient | Continuous parameter being adjusted |

### 9.2 LED State Machine

```cpp
enum class LEDMode {
    IDLE,           // No output, LED off
    TRIGGER,        // Showing trigger activity
    MODE_CHANGE,    // Flash for discrete mode change
    CONTINUOUS,     // Gradient for continuous parameter
    FILL_ACTIVE,    // Pulsing during live fill
    RESET_FLASH     // Bright flash for reset/reseed
};

struct LEDState {
    LEDMode mode;
    float brightness;
    float targetBrightness;
    int flashSamplesRemaining;
    int pulsePhase;
};

float ProcessLED(LEDState& state, bool anchorFired, bool shimmerFired,
                 bool parameterChanged, bool isDiscreteParam, float paramValue,
                 bool fillActive, bool resetTriggered, int sampleRate) {
    
    // Priority: Reset > Fill > Mode Change > Continuous > Trigger > Idle
    
    if (resetTriggered) {
        state.mode = LEDMode::RESET_FLASH;
        state.brightness = 1.0f;
        state.flashSamplesRemaining = sampleRate / 10;  // 100ms flash
        return state.brightness * 5.0f;
    }
    
    if (state.mode == LEDMode::RESET_FLASH) {
        state.flashSamplesRemaining--;
        if (state.flashSamplesRemaining <= 0) {
            state.mode = LEDMode::IDLE;
        }
        return state.brightness * 5.0f;
    }
    
    if (fillActive) {
        state.mode = LEDMode::FILL_ACTIVE;
        // Pulse between 40% and 90%
        state.pulsePhase = (state.pulsePhase + 1) % (sampleRate / 4);
        float pulsePos = (float)state.pulsePhase / (sampleRate / 4);
        state.brightness = 0.4f + 0.5f * sinf(pulsePos * 2.0f * M_PI);
        return state.brightness * 5.0f;
    }
    
    if (parameterChanged) {
        if (isDiscreteParam) {
            // Mode change: brief bright flash + "click"
            state.mode = LEDMode::MODE_CHANGE;
            state.brightness = 1.0f;
            state.flashSamplesRemaining = sampleRate / 20;  // 50ms flash
        } else {
            // Continuous: show value as gradient
            state.mode = LEDMode::CONTINUOUS;
            state.targetBrightness = 0.2f + paramValue * 0.7f;  // 20-90%
        }
    }
    
    if (state.mode == LEDMode::MODE_CHANGE) {
        state.flashSamplesRemaining--;
        if (state.flashSamplesRemaining <= 0) {
            state.mode = LEDMode::TRIGGER;
        }
        return state.brightness * 5.0f;
    }
    
    if (state.mode == LEDMode::CONTINUOUS) {
        // Smooth fade to target
        state.brightness += (state.targetBrightness - state.brightness) * 0.01f;
        // Exit continuous mode after settling
        if (fabsf(state.brightness - state.targetBrightness) < 0.01f) {
            state.mode = LEDMode::TRIGGER;
        }
        return state.brightness * 5.0f;
    }
    
    // Default: trigger mode
    state.mode = LEDMode::TRIGGER;
    if (anchorFired) {
        state.brightness = 0.8f;
    } else if (shimmerFired) {
        state.brightness = 0.3f;
    } else {
        // Decay
        state.brightness *= 0.95f;
    }
    
    return state.brightness * 5.0f;
}
```

### 9.3 Parameter Change Detection

```cpp
struct ParameterState {
    float lastValue;
    bool isDiscrete;
    int discreteZones;  // Number of zones for discrete params
    int lastZone;
};

bool DetectParameterChange(ParameterState& param, float currentValue, 
                           bool& isDiscrete, float& normalizedValue) {
    float delta = fabsf(currentValue - param.lastValue);
    bool changed = delta > 0.01f;  // 1% threshold
    
    if (changed) {
        param.lastValue = currentValue;
        
        if (param.isDiscrete) {
            int currentZone = (int)(currentValue * param.discreteZones);
            currentZone = min(currentZone, param.discreteZones - 1);
            
            if (currentZone != param.lastZone) {
                param.lastZone = currentZone;
                isDiscrete = true;
                normalizedValue = (float)currentZone / (param.discreteZones - 1);
                return true;
            }
            return false;  // Same zone, no visual change
        } else {
            isDiscrete = false;
            normalizedValue = currentValue;
            return true;
        }
    }
    
    return false;
}
```

---

## 10. Data Structures

### 10.1 Complete State Structure

```cpp
// Forward declarations
enum class Genre { TECHNO, TRIBAL, IDM };
enum class Voice { ANCHOR, SHIMMER, AUX };
enum class EnergyZone { MINIMAL, GROOVE, BUILD, PEAK };
enum class AuxMode { HAT, FILL_GATE, PHRASE_CV, EVENT };
enum class AuxDensity { SPARSE, NORMAL, DENSE, BUSY };
enum class VoiceCoupling { INDEPENDENT, INTERLOCK, SHADOW };
enum class ResetMode { PHRASE, BAR, STEP };

// Archetype DNA (9 per genre = 27 total)
struct ArchetypeDNA {
    float anchorWeights[32];
    float shimmerWeights[32];
    float auxWeights[32];
    uint32_t anchorAccentMask;
    uint32_t shimmerAccentMask;
    float swingAmount;
    float defaultCouple;
    float fillDensityMultiplier;
    uint32_t ratchetEligibleMask;
    uint8_t gridX;
    uint8_t gridY;
};

// Genre pattern field
struct GenreField {
    ArchetypeDNA archetypes[3][3];  // [y][x]
    float baseSwing;
    float baseBrokenBias;
};

// Runtime control state
struct ControlState {
    // Primary performance parameters (CV-able)
    float energy;         // K1: Hit density (0-1)
    float build;          // K2: Phrase arc / drama (0-1)
    float fieldX;         // K3: Pattern morph X - syncopation (0-1)
    float fieldY;         // K4: Pattern morph Y - complexity (0-1)
    
    // Shift performance parameters
    float punch;          // Shift+K1: Velocity dynamics (0-1)
    Genre genre;          // Shift+K2: Archetype bank (TECHNO/TRIBAL/IDM)
    float drift;          // Shift+K3: Pattern evolution rate (0-1)
    float balance;        // Shift+K4: Anchor/shimmer ratio (0-1)
    
    // External CV input (Audio In R)
    float flavor;         // Timing feel / broken amount (0-1)
    
    // Config primary parameters (domain-based)
    int patternLength;    // Config K1 (Grid): 16, 24, 32, 64 steps
    float swing;          // Config K2 (Timing): Base swing amount (0-1)
    AuxMode auxMode;      // Config K3 (Output): Hat/FillGate/PhraseCV/Event
    ResetMode resetMode;  // Config K4 (Behavior): Phrase/Bar/Step
    
    // Config shift parameters
    int phraseLength;     // Config Shift+K1 (Grid): 1, 2, 4, 8 bars
    int clockDiv;         // Config Shift+K2 (Timing): 1, 2, 4, 8
    AuxDensity auxDensity;     // Config Shift+K3 (Output): Sparse/Normal/Dense/Busy
    VoiceCoupling voiceCoupling; // Config Shift+K4 (Behavior): Ind/Interlock/Shadow
    
    // Derived state
    EnergyZone zone;
    PunchParams punchParams;      // Computed from punch
    BuildModifiers buildModifiers; // Computed from build + phraseProgress
    
    // Button state
    bool shiftHeld;
    bool fillQueued;
    bool liveFillActive;
    float fillIntensity;  // 0-1, from fill CV input
    bool reseedQueued;
};

// Punch parameters (computed from punch knob)
struct PunchParams {
    float accentProbability;  // How often hits are accented (0.15-0.5)
    float velocityFloor;      // Minimum velocity for non-accented hits (0.3-0.7)
    float accentBoost;        // How much louder accents are (0.1-0.35)
    float velocityVariation;  // Random variation range (0.05-0.2)
};

// Build modifiers (computed from build + phraseProgress)
struct BuildModifiers {
    float densityMultiplier;    // 1.0 at phrase start, up to 1.5 at end
    float fillProbability;      // Chance of fills at phrase boundaries
    float fillIntensity;        // How dramatic fills are (affects budget)
    float accentBoost;          // Extra accent probability toward phrase end
    float timingLooseness;      // How much BROKEN effect increases
};

// Fill input state (from Audio In L)
struct FillInputState {
    bool gateActive;
    float intensity;     // 0-1, from CV level
    float threshold;     // Gate detection threshold (default 0.2 = 1V)
};

// Sequencer runtime state
struct SequencerState {
    // Position
    int currentStep;
    int currentBar;
    int currentBarInPhrase;
    float phraseProgress;  // 0-1
    
    // Hit masks for current bar
    uint32_t anchorHitMask;
    uint32_t shimmerHitMask;
    uint32_t auxHitMask;
    uint32_t anchorAccentMask;
    uint32_t shimmerAccentMask;
    
    // Current step outputs
    bool anchorFires;
    bool shimmerFires;
    bool auxFires;
    float anchorVelocity;
    float shimmerVelocity;
    float auxVelocity;
    
    // Timing offsets (in samples)
    int anchorTimingOffset;
    int shimmerTimingOffset;
    int auxTimingOffset;
    
    // Event flags (for AUX EVENT mode)
    bool isFillStart;
    bool isAccentHit;
    bool isSectionBoundary;
    bool isRatchet;
    
    // Fill state
    bool inFillZone;
    bool isLastBarOfPhrase;
    
    // Seeds
    DriftState driftState;
    
    // Guard rail state
    GuardRailState guardRailState;
    
    // Blended archetype (cached per control change)
    ArchetypeDNA blendedDNA;
};

// Velocity output state (sample & hold)
struct VelocityOutputState {
    float currentVoltage;   // Held value (persists between triggers)
    float targetVoltage;    // For optional smooth slew
};

// Trigger output state
struct OutputState {
    TriggerState anchorTrigger;
    TriggerState shimmerTrigger;
    TriggerState auxTrigger;
    VelocityOutputState anchorVelocity;
    VelocityOutputState shimmerVelocity;
    LEDState led;
};

// Complete firmware state
struct DuoPulseState {
    ControlState controls;
    SequencerState sequencer;
    OutputState outputs;
    FillInputState fillInputState;
    AutoSaveState autoSave;
    
    // Timing
    int sampleRate;
    int samplesPerStep;
    int triggerLengthSamples;
    
    // Clock
    bool externalClockPatched;
    bool clockHigh;
    int samplesSinceLastClock;
};
```

---

## 11. Implementation Pseudocode

### 11.1 Main Audio Callback

```cpp
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
    DuoPulseState& state = GetState();
    
    for (size_t i = 0; i < size; i++) {
        // Process clock
        bool clockTick = ProcessClock(state, i);
        
        if (clockTick) {
            // Advance step
            AdvanceStep(state);
            
            // Check for bar boundary
            if (state.sequencer.currentStep == 0) {
                GenerateBar(state);
            }
            
            // Get current step outputs
            ProcessStep(state);
        }
        
        // Process controls (less frequently, not in audio-rate path)
        if (i % 32 == 0) {
            ProcessControls(state, state.autoSave);
        }
        
        // Generate trigger outputs
        float anchorGate = ProcessTrigger(state.outputs.anchorTrigger, 
                                          state.sequencer.anchorFires,
                                          state.triggerLengthSamples);
        float shimmerGate = ProcessTrigger(state.outputs.shimmerTrigger,
                                           state.sequencer.shimmerFires,
                                           state.triggerLengthSamples);
        
        // Generate velocity outputs (sample & hold - only update on trigger)
        ProcessVelocityOutput(state.outputs.anchorVelocity,
                              state.sequencer.anchorFires,
                              state.sequencer.anchorVelocity,
                              &anchorVelCV);
        ProcessVelocityOutput(state.outputs.shimmerVelocity,
                              state.sequencer.shimmerFires,
                              state.sequencer.shimmerVelocity,
                              &shimmerVelCV);
        
        // Generate AUX output (clock if internal, else AUX mode)
        float auxCV = ComputeAuxOutput(state.controls.auxMode, 
                                       state.sequencer,
                                       state.externalClockPatched,
                                       state.clockHigh);
        
        // Generate LED output
        float ledCV = ProcessLED(state.outputs.led, 
                                 state.sequencer.anchorFires,
                                 state.sequencer.shimmerFires,
                                 parameterChanged, isDiscreteParam, paramValue,
                                 state.controls.liveFillActive,
                                 state.controls.reseedQueued,
                                 state.sampleRate);
        
        // Write outputs
        // GATE_OUT_1: Anchor trigger
        dsy_gpio_write(&gate1_pin, anchorGate > 2.5f);
        // GATE_OUT_2: Shimmer trigger  
        dsy_gpio_write(&gate2_pin, shimmerGate > 2.5f);
        // CV_OUT_1: AUX output
        dsy_dac_write(DAC_CHN_1, auxCV / 5.0f * 4095);
        // CV_OUT_2: LED
        dsy_dac_write(DAC_CHN_2, ledCV / 5.0f * 4095);
        // OUT_L: Anchor velocity (sample & hold)
        out[0][i] = anchorVelCV / 5.0f;
        // OUT_R: Shimmer velocity (sample & hold)
        out[1][i] = shimmerVelCV / 5.0f;
    }
}

// Call in main loop (not audio callback) for auto-save
void MainLoop() {
    while (1) {
        ProcessAutoSave(state.autoSave);
        // ... other main loop tasks
    }
}
```

### 11.2 Bar Generation

```cpp
void GenerateBar(DuoPulseState& state) {
    ControlState& ctrl = state.controls;
    SequencerState& seq = state.sequencer;
    
    // 1. Compute hit budgets (BUILD affects density, AUX DENSITY scales aux)
    float auxDensityMult = GetAuxDensityMultiplier(ctrl.auxDensity);
    BarBudget budget = ComputeBarBudget(
        ctrl.energy, ctrl.balance, ctrl.zone,
        seq.currentBarInPhrase, ctrl.phraseLength,
        ctrl.buildModifiers, auxDensityMult
    );
    
    // 2. Compute eligibility masks
    uint32_t anchorEligibility = ComputeEligibilityMask(
        ctrl.energy, ctrl.flavor, ctrl.zone, Voice::ANCHOR, ctrl.genre
    );
    uint32_t shimmerEligibility = ComputeEligibilityMask(
        ctrl.energy, ctrl.flavor, ctrl.zone, Voice::SHIMMER, ctrl.genre
    );
    uint32_t auxEligibility = ComputeEligibilityMask(
        ctrl.energy, ctrl.flavor, ctrl.zone, Voice::AUX, ctrl.genre
    );
    
    // 3. Get blended weights from pattern field (cached on control change)
    const ArchetypeDNA& dna = seq.blendedDNA;
    
    // 4. Select seed based on DRIFT
    uint32_t barSeed = SelectBarSeed(seq.currentBar, ctrl.drift, seq.driftState);
    
    // 5. Select hits via Gumbel Top-K
    SelectHitsGumbelTopK(dna.anchorWeights, anchorEligibility,
                         budget.anchorHits, seq.anchorHitMask, 
                         barSeed, ctrl.patternLength);
    SelectHitsGumbelTopK(dna.shimmerWeights, shimmerEligibility,
                         budget.shimmerHits, seq.shimmerHitMask,
                         barSeed + 1000, ctrl.patternLength);
    SelectHitsGumbelTopK(dna.auxWeights, auxEligibility,
                         budget.auxHits, seq.auxHitMask,
                         barSeed + 2000, ctrl.patternLength);
    
    // 6. Select accents (PUNCH controls accent probability + BUILD boosts toward phrase end)
    float effectiveAccentProb = ctrl.punchParams.accentProbability + ctrl.buildModifiers.accentBoost;
    int anchorAccentCount = max(1, (int)(budget.anchorHits * effectiveAccentProb));
    int shimmerAccentCount = max(1, (int)(budget.shimmerHits * effectiveAccentProb));
    
    seq.anchorAccentMask = SelectAccents(seq.anchorHitMask, dna.anchorAccentMask,
                                         anchorAccentCount, barSeed + 3000);
    seq.shimmerAccentMask = SelectAccents(seq.shimmerHitMask, dna.shimmerAccentMask,
                                          shimmerAccentCount, barSeed + 4000);
    
    // 7. Apply voice relationship (from config, overrides archetype default)
    float coupleValue = GetCoupleValueFromConfig(ctrl.voiceCoupling, dna.defaultCouple);
    ApplyVoiceRelationship(coupleValue, seq.anchorHitMask, seq.shimmerHitMask,
                dna.anchorWeights, dna.shimmerWeights,
                ctrl.patternLength, barSeed + 5000);
    
    // 8. Soft repair pass
    SoftRepairPass(seq.anchorHitMask, anchorEligibility,
                   dna.anchorWeights, ctrl.patternLength, ctrl.zone);
    
    // 9. Hard guard rails
    ApplyHardGuardRails(seq.anchorHitMask, seq.shimmerHitMask,
                        ctrl.zone, ctrl.genre, ctrl.patternLength,
                        seq.guardRailState);
    
    // 10. Check for phrase boundary
    if (seq.currentBarInPhrase == ctrl.phraseLength - 1) {
        seq.isLastBarOfPhrase = true;
        seq.inFillZone = (ctrl.zone >= EnergyZone::BUILD);
    }
}
```

### 11.3 Step Processing

```cpp
void ProcessStep(DuoPulseState& state) {
    ControlState& ctrl = state.controls;
    SequencerState& seq = state.sequencer;
    int step = seq.currentStep;
    
    // Check hit masks
    seq.anchorFires = (seq.anchorHitMask & (1 << step)) != 0;
    seq.shimmerFires = (seq.shimmerHitMask & (1 << step)) != 0;
    seq.auxFires = (seq.auxHitMask & (1 << step)) != 0;
    
    // Check accents
    bool anchorAccent = (seq.anchorAccentMask & (1 << step)) != 0;
    bool shimmerAccent = (seq.shimmerAccentMask & (1 << step)) != 0;
    
    // Compute velocities
    uint32_t seed = SelectSeed(step, ctrl.drift, seq.driftState);
    seq.anchorVelocity = seq.anchorFires ? 
        ComputeVelocity(step, anchorAccent, ctrl.punchParams, ctrl.buildModifiers, seed) : 0.0f;
    seq.shimmerVelocity = seq.shimmerFires ?
        ComputeVelocity(step, shimmerAccent, ctrl.punchParams, ctrl.buildModifiers, seed) : 0.0f;
    seq.auxVelocity = seq.auxFires ? 0.7f : 0.0f;
    
    // Compute timing offsets (BROKEN stack)
    float swing = ComputeSwing(ctrl.flavor, seq.blendedDNA.swingAmount, ctrl.zone);
    int swingOffset = ApplySwingToStep(step, swing, state.samplesPerStep);
    int jitterOffset = ComputeMicrotimingOffset(step, ctrl.flavor, ctrl.zone, 
                                                 seed, state.sampleRate);
    
    seq.anchorTimingOffset = swingOffset + jitterOffset;
    seq.shimmerTimingOffset = swingOffset + jitterOffset;
    seq.auxTimingOffset = swingOffset;  // Aux follows swing but less jitter
    
    // Update event flags for AUX EVENT mode
    seq.isAccentHit = anchorAccent || shimmerAccent;
    seq.isSectionBoundary = (step == 0 && seq.currentBarInPhrase == 0);
    seq.isFillStart = (step == 0 && seq.inFillZone && !seq.wasInFillZone);
    
    // Update phrase progress
    int totalSteps = ctrl.patternLength * ctrl.phraseLength;
    int currentStepInPhrase = seq.currentBarInPhrase * ctrl.patternLength + step;
    seq.phraseProgress = (float)currentStepInPhrase / totalSteps;
}
```

### 11.4 Control Processing

```cpp
void ProcessControls(DuoPulseState& state, AutoSaveState& autoSave) {
    ControlState& ctrl = state.controls;
    bool configChanged = false;
    
    // Read hardware - knobs
    float k1 = hw.GetKnobValue(KNOB_1);
    float k2 = hw.GetKnobValue(KNOB_2);
    float k3 = hw.GetKnobValue(KNOB_3);
    float k4 = hw.GetKnobValue(KNOB_4);
    
    // Read hardware - CV inputs (bipolar, -5V to +5V)
    float cv1 = hw.GetCvValue(CV_1);
    float cv2 = hw.GetCvValue(CV_2);
    float cv3 = hw.GetCvValue(CV_3);
    float cv4 = hw.GetCvValue(CV_4);
    
    // Read hardware - audio inputs (0-5V)
    float audioInL = hw.GetAudioInValue(IN_L);  // Fill CV
    float audioInR = hw.GetAudioInValue(IN_R);  // Flavor CV
    
    bool button = hw.GetButton();
    bool switchPos = hw.GetSwitch();  // true = Config mode
    
    // Check if audio inputs are patched (detect cable presence)
    bool fillInputPatched = hw.IsPatched(IN_L);
    bool flavorCVPatched = hw.IsPatched(IN_R);
    
    // Process button gestures
    ProcessButtonGestures(state, button);
    
    // Process fill input (overrides button fill when patched)
    if (fillInputPatched) {
        ProcessFillInput(state.fillInputState, audioInL);
        if (state.fillInputState.gateActive) {
            ctrl.liveFillActive = true;
            ctrl.fillIntensity = state.fillInputState.intensity;
        }
    }
    
    // Apply CV modulation to primary performance controls
    float e1 = ProcessCVModulation(k1, cv1);
    float e2 = ProcessCVModulation(k2, cv2);
    float e3 = ProcessCVModulation(k3, cv3);
    float e4 = ProcessCVModulation(k4, cv4);
    
    if (!switchPos) {
        // ========== PERFORMANCE MODE ==========
        if (!ctrl.shiftHeld) {
            // Primary controls: ENERGY / BUILD / FIELD X / FIELD Y (all CV-able)
            ctrl.energy = e1;
            ctrl.build = e2;
            ctrl.fieldX = e3;
            ctrl.fieldY = e4;
        } else {
            // Shift controls: PUNCH / GENRE / DRIFT / BALANCE
            ctrl.punch = e1;
            
            // Genre (discrete, 3 zones)
            Genre newGenre = GenreFromNormalized(e2);
            if (newGenre != ctrl.genre) {
                ctrl.genre = newGenre;
                configChanged = true;
            }
            
            ctrl.drift = e3;
            ctrl.balance = e4;
        }
        
        // Flavor CV input (Audio In R) - directly controls timing feel
        if (flavorCVPatched) {
            ctrl.flavor = Clamp(audioInR / 5.0f, 0.0f, 1.0f);
        } else {
            // Default flavor derived from genre when no CV
            ctrl.flavor = GetDefaultFlavorForGenre(ctrl.genre);
        }
        
        // Compute derived parameters
        ctrl.punchParams = ComputePunch(ctrl.punch);
        ctrl.buildModifiers = ComputeBuildModifiers(ctrl.build, 
                                                     state.sequencer.phraseProgress);
        
    } else {
        // ========== CONFIG MODE (Domain-Based) ==========
        if (!ctrl.shiftHeld) {
            // Primary: PATTERN LENGTH / SWING / AUX MODE / RESET MODE
            
            // K1: Pattern length (Grid domain - discrete, 4 values: 16/24/32/64)
            int lengthIndex = (int)(e1 * 4);
            lengthIndex = min(lengthIndex, 3);
            int lengths[] = {16, 24, 32, 64};
            if (ctrl.patternLength != lengths[lengthIndex]) {
                ctrl.patternLength = lengths[lengthIndex];
                configChanged = true;
            }
            
            // K2: Swing (Timing domain - continuous 0-100%)
            ctrl.swing = e2;
            
            // K3: AUX mode (Output domain - discrete, 4 zones)
            int auxIndex = (int)(e3 * 4);
            auxIndex = min(auxIndex, 3);
            AuxMode newAuxMode = (AuxMode)auxIndex;
            if (ctrl.auxMode != newAuxMode) {
                ctrl.auxMode = newAuxMode;
                configChanged = true;
            }
            
            // K4: Reset mode (Behavior domain - discrete, 3 values)
            int resetIndex = (int)(e4 * 3);
            resetIndex = min(resetIndex, 2);
            ResetMode modes[] = {ResetMode::PHRASE, ResetMode::BAR, ResetMode::STEP};
            if (ctrl.resetMode != modes[resetIndex]) {
                ctrl.resetMode = modes[resetIndex];
                configChanged = true;
            }
            
        } else {
            // Shift: PHRASE LENGTH / CLOCK DIV / AUX DENSITY / VOICE COUPLING
            
            // K1: Phrase length (Grid domain - discrete, 4 values: 1/2/4/8 bars)
            int phraseIndex = (int)(e1 * 4);
            phraseIndex = min(phraseIndex, 3);
            int phraseLengths[] = {1, 2, 4, 8};
            if (ctrl.phraseLength != phraseLengths[phraseIndex]) {
                ctrl.phraseLength = phraseLengths[phraseIndex];
                configChanged = true;
            }
            
            // K2: Clock division (Timing domain - discrete, 4 values)
            int divIndex = (int)(e2 * 4);
            divIndex = min(divIndex, 3);
            int divs[] = {1, 2, 4, 8};
            if (ctrl.clockDiv != divs[divIndex]) {
                ctrl.clockDiv = divs[divIndex];
                configChanged = true;
            }
            
            // K3: Aux density (Output domain - discrete, 4 values)
            int densityIndex = (int)(e3 * 4);
            densityIndex = min(densityIndex, 3);
            AuxDensity densities[] = {AuxDensity::SPARSE, AuxDensity::NORMAL,
                                      AuxDensity::DENSE, AuxDensity::BUSY};
            if (ctrl.auxDensity != densities[densityIndex]) {
                ctrl.auxDensity = densities[densityIndex];
                configChanged = true;
            }
            
            // K4: Voice coupling (Behavior domain - discrete, 3 values)
            int couplingIndex = (int)(e4 * 3);
            couplingIndex = min(couplingIndex, 2);
            VoiceCoupling couplings[] = {VoiceCoupling::INDEPENDENT,
                                         VoiceCoupling::INTERLOCK,
                                         VoiceCoupling::SHADOW};
            if (ctrl.voiceCoupling != couplings[couplingIndex]) {
                ctrl.voiceCoupling = couplings[couplingIndex];
                configChanged = true;
            }
        }
    }
    
    // Update derived state
    ctrl.zone = GetZone(ctrl.energy);
    
    // Recompute blended archetype if field position or genre changed
    static float lastFieldX = -1, lastFieldY = -1;
    static Genre lastGenre = Genre::TECHNO;
    if (ctrl.fieldX != lastFieldX || ctrl.fieldY != lastFieldY || 
        ctrl.genre != lastGenre) {
        state.sequencer.blendedDNA = BlendArchetypes(
            ctrl.fieldX, ctrl.fieldY, 
            GetGenreField(ctrl.genre).archetypes
        );
        lastFieldX = ctrl.fieldX;
        lastFieldY = ctrl.fieldY;
        lastGenre = ctrl.genre;
    }
    
    // Mark config dirty for auto-save if anything changed
    if (configChanged) {
        MarkConfigDirty(autoSave, state);
    }
}

Genre GenreFromNormalized(float value) {
    if (value < 0.33f) return Genre::TECHNO;
    if (value < 0.67f) return Genre::TRIBAL;
    return Genre::IDM;
}
```

---

## 12. Configuration & Persistence

### 12.1 Auto-Save System

Config changes are automatically persisted to flash with debouncing to minimize flash wear and avoid interrupting audio.

```cpp
struct AutoSaveState {
    bool dirty;                    // Config has changed since last save
    uint32_t lastChangeTime;       // Timestamp of last change
    uint32_t debounceMs;           // Wait time before saving (default: 2000ms)
    PersistentConfig pendingConfig; // Buffered config to save
};

void MarkConfigDirty(AutoSaveState& autoSave, const DuoPulseState& state) {
    autoSave.dirty = true;
    autoSave.lastChangeTime = System::GetNow();
    
    // Buffer the config immediately
    autoSave.pendingConfig.magic = kConfigMagic;
    autoSave.pendingConfig.version = 1;
    
    // Performance shift
    autoSave.pendingConfig.genre = state.controls.genre;
    
    // Config primary (domain-based)
    autoSave.pendingConfig.patternLength = state.controls.patternLength;
    autoSave.pendingConfig.swing = state.controls.swing;
    autoSave.pendingConfig.auxMode = state.controls.auxMode;
    autoSave.pendingConfig.resetMode = state.controls.resetMode;
    
    // Config shift (domain-based)
    autoSave.pendingConfig.phraseLength = state.controls.phraseLength;
    autoSave.pendingConfig.clockDiv = state.controls.clockDiv;
    autoSave.pendingConfig.auxDensity = state.controls.auxDensity;
    autoSave.pendingConfig.voiceCoupling = state.controls.voiceCoupling;
    
    // Pattern seed
    autoSave.pendingConfig.savedPatternSeed = state.sequencer.driftState.patternSeed;
}

// Call this in main loop (not audio callback)
void ProcessAutoSave(AutoSaveState& autoSave) {
    if (!autoSave.dirty) return;
    
    uint32_t elapsed = System::GetNow() - autoSave.lastChangeTime;
    if (elapsed < autoSave.debounceMs) return;
    
    // Debounce period elapsed, commit to flash
    autoSave.pendingConfig.checksum = ComputeChecksum(
        &autoSave.pendingConfig, sizeof(PersistentConfig) - 4);
    
    hw.qspi.Erase(kConfigAddress, sizeof(PersistentConfig));
    hw.qspi.Write(kConfigAddress, sizeof(PersistentConfig), 
                  (uint8_t*)&autoSave.pendingConfig);
    
    autoSave.dirty = false;
}
```

### 12.2 Flash Storage

```cpp
// Store config in Daisy's QSPI flash
constexpr uint32_t kConfigMagic = 0xDUO4;
constexpr uint32_t kConfigAddress = 0x90000000;  // QSPI base

struct PersistentConfig {
    uint32_t magic;
    uint8_t version;
    
    // Performance mode settings (persisted)
    Genre genre;
    
    // Config primary settings (domain-based)
    int patternLength;
    float swing;
    AuxMode auxMode;
    ResetMode resetMode;
    
    // Config shift settings (domain-based)
    int phraseLength;
    int clockDiv;
    AuxDensity auxDensity;
    VoiceCoupling voiceCoupling;
    
    // Pattern seed (survives power cycles)
    uint32_t savedPatternSeed;
    
    uint32_t checksum;
};

bool LoadConfig(DuoPulseState& state) {
    PersistentConfig config;
    hw.qspi.Read(kConfigAddress, sizeof(PersistentConfig), (uint8_t*)&config);
    
    if (config.magic != kConfigMagic) return false;
    if (config.checksum != ComputeChecksum(&config, sizeof(config) - 4)) return false;
    
    state.controls.genre = config.genre;
    state.controls.auxMode = config.auxMode;
    state.controls.phraseLength = config.phraseLength;
    state.controls.patternLength = config.patternLength;
    state.controls.swing = config.swing;
    state.controls.clockDiv = config.clockDiv;
    state.controls.auxDensity = config.auxDensity;
    state.controls.resetMode = config.resetMode;
    state.controls.voiceCoupling = config.voiceCoupling;
    
    state.sequencer.driftState.patternSeed = config.savedPatternSeed;
    
    return true;
}
```

### 12.3 What Gets Saved

| Category | Parameters | Trigger |
|----------|------------|---------|
| **Config Primary** | Pattern length, swing, AUX mode, reset mode | On change (debounced) |
| **Config Shift** | Phrase length, clock div, aux density, voice coupling | On change (debounced) |
| **Performance Shift** | Genre | On change (debounced) |
| **Pattern Seed** | Current pattern seed | On reseed |

**Not Saved** (read from knobs on boot):
- ENERGY, BUILD, FIELD X/Y (primary performance controls)
- PUNCH, DRIFT, BALANCE (shift performance controls)
- FLAVOR (from Audio In R)

### 12.4 Default Values

```cpp
void ResetToDefaults(DuoPulseState& state) {
    // Primary performance controls
    state.controls.energy = 0.5f;
    state.controls.build = 0.4f;
    state.controls.fieldX = 0.5f;
    state.controls.fieldY = 0.5f;
    
    // Shift performance controls
    state.controls.punch = 0.5f;
    state.controls.genre = Genre::TECHNO;
    state.controls.drift = 0.2f;
    state.controls.balance = 0.5f;
    
    // External CV (default when unpatched)
    state.controls.flavor = 0.3f;
    
    // Config primary (domain-based)
    state.controls.patternLength = 16;
    state.controls.swing = 0.54f;
    state.controls.auxMode = AuxMode::HAT;
    state.controls.resetMode = ResetMode::PHRASE;
    
    // Config shift (domain-based)
    state.controls.phraseLength = 4;
    state.controls.clockDiv = 1;
    state.controls.auxDensity = AuxDensity::NORMAL;
    state.controls.voiceCoupling = VoiceCoupling::INDEPENDENT;
    
    state.controls.zone = EnergyZone::GROOVE;
    state.controls.shiftHeld = false;
    state.controls.fillQueued = false;
    state.controls.liveFillActive = false;
    state.controls.reseedQueued = false;
    
    // Sequencer
    state.sequencer.currentStep = 0;
    state.sequencer.currentBar = 0;
    state.sequencer.currentBarInPhrase = 0;
    state.sequencer.phraseProgress = 0.0f;
    
    state.sequencer.driftState.patternSeed = 12345;
    state.sequencer.driftState.phraseSeed = 12345;
    state.sequencer.driftState.phraseCounter = 0;
    
    // Timing
    state.sampleRate = 48000;
    state.samplesPerStep = state.sampleRate / 8;  // ~120 BPM at 16th notes
    state.triggerLengthSamples = state.sampleRate / 200;  // 5ms triggers
}
```

---

## 13. Testing Requirements

### 13.1 Unit Tests

| Test Area | Tests Required |
|-----------|----------------|
| **Hit Budget** | Budget scales with energy; balance splits correctly; fill bias on last bar |
| **Eligibility Mask** | Energy unlocks metric levels; flavor adds offbeats; zone restrictions |
| **Gumbel Top-K** | Correct count selected; spacing rules enforced; deterministic with seed |
| **Voice Relationship** | Interlock suppresses simultaneous; shadow creates echoes; gap-fill works |
| **Guard Rails** | Downbeat forced when missing; max gap enforced; consecutive shimmer limited |
| **BROKEN Stack** | Swing applied to correct steps; jitter bounded by zone; displacement zone-gated |
| **Archetype Blend** | Softmax sharpens weights; continuous properties interpolate; discrete from dominant |
| **DRIFT** | Seed selection by stability; phrase seed changes; reseed works |

### 13.2 Integration Tests

| Test | Description |
|------|-------------|
| **Full Bar Generation** | Generate 100 bars, verify all have valid hit counts and pass guard rails |
| **Energy Sweep** | Sweep energy 0-100%, verify zone transitions and density changes |
| **Field Navigation** | Navigate all 9 grid positions, verify distinct character at each |
| **Genre Switch** | Switch genres mid-phrase, verify clean transition |
| **Clock Stability** | External clock at various tempos (60-200 BPM), verify timing accuracy |
| **Persistence** | Save config, simulate power cycle, verify restore |

### 13.3 Musical Validation

| Test | Expected Outcome |
|------|------------------|
| **ENERGY=30%, FLAVOR=20%** | Steady techno groove, tight timing, predictable |
| **ENERGY=80%, FLAVOR=50%** | Busy broken beat, fills at phrase end, loose timing |
| **DRIFT=0%** | Identical pattern every phrase |
| **DRIFT=100%** | Varied pattern but same density feel |
| **Field [0,0] → [2,2]** | Clear progression from minimal to chaos |

---

## Appendix A: Archetype Weight Tables

*To be populated during implementation. Each genre needs 9 archetypes with 32-step weight arrays per voice.*

Example structure for Techno [1,1] "Groovy":

```cpp
const ArchetypeDNA kTechnoGroovy = {
    // Anchor: quarter notes strong, some 8th ghost potential
    .anchorWeights = {
        1.0, 0.1, 0.3, 0.1,  // Beat 1
        0.9, 0.1, 0.2, 0.1,  // Beat 2
        0.9, 0.1, 0.3, 0.1,  // Beat 3
        0.9, 0.1, 0.2, 0.1,  // Beat 4
        // ... repeat for 32 steps
    },
    // Shimmer: backbeat focus, off-8th potential
    .shimmerWeights = {
        0.1, 0.2, 0.3, 0.2,  // Beat 1
        0.9, 0.3, 0.4, 0.2,  // Beat 2 (snare)
        0.1, 0.2, 0.3, 0.2,  // Beat 3
        0.9, 0.3, 0.4, 0.2,  // Beat 4 (snare)
        // ...
    },
    // Aux: 8th note hats, open on off-beats
    .auxWeights = {
        0.7, 0.3, 0.9, 0.3,
        0.7, 0.3, 0.9, 0.3,
        // ...
    },
    .anchorAccentMask = 0x00010001,  // Downbeats
    .shimmerAccentMask = 0x00100010, // Backbeats
    .swingAmount = 0.54f,
    .defaultCouple = 0.3f,
    .fillDensityMultiplier = 1.5f,
    .ratchetEligibleMask = 0x11111111,
    .gridX = 1,
    .gridY = 1,
};
```

---

## Appendix B: Glossary

| Term | Definition |
|------|------------|
| **Anchor** | Primary voice, typically kick-like role |
| **Shimmer** | Secondary voice, typically snare/clap-like role |
| **AUX** | Third voice, typically hi-hat/percussion role |
| **Hit Budget** | Target number of hits per bar, guarantees density |
| **Eligibility Mask** | Bitmask of steps that *can* fire (possible vs probable) |
| **Gumbel Top-K** | Weighted sampling method using Gumbel noise for deterministic selection |
| **COUPLE** | Voice relationship parameter stored in archetypes (independent → interlock → shadow) |
| **PUNCH** | Velocity dynamics parameter (flat dynamics → punchy with accent contrast) |
| **BUILD** | Phrase arc parameter (flat phrase → climactic with fills and builds) |
| **DRIFT** | Pattern evolution rate (0% = locked, 100% = varies each phrase) |
| **BROKEN** | Timing stack: swing + jitter + displacement + velocity chaos |
| **Guard Rails** | Hard rules that guarantee musicality (downbeat protection, max gap, etc.) |
| **Energy Zone** | Behavioral mode derived from ENERGY: MINIMAL/GROOVE/BUILD/PEAK |
| **Archetype** | One of 9 curated pattern templates per genre |
| **Pattern Field** | 3×3 grid of archetypes navigated by FIELD X/Y |
| **Winner-Take-More** | Softmax blending that preserves dominant archetype character |

---

*End of Specification*
