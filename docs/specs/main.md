# DuoPulse v4: Algorithmic Drum Sequencer Specification

**Target Platform**: Daisy Patch.init() (Electro-Smith)  
**Version**: 4.0  
**Status**: Implementation Spec  
**Last Updated**: 2025-12-19

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
| ~~**Flavor CV**~~ | ~~Audio In R~~ | ~~Removed in v4~~ (timing now controlled by GENRE + SWING config) |

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
2. **Playability**: Controls map directly to musical intent. ENERGY = tension. GENRE = feel.
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
| Genre coherence | Terrain/grid mismatch | Genre emerges unpredictably | GENRE selects timing profile; patterns use archetype grid |
| Repeatability | No determinism | DRIFT=0 works but fragile | Seed-controlled everything |

---

## 2. Architecture Overview

### 2.1 High-Level Pipeline

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           CONTROL LAYER                                  │
│                                                                          │
│   ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐                    │
│   │ ENERGY  │  │  BUILD  │  │ FIELD X │  │ FIELD Y │                    │
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
│  │ (from ENERGY   │  │     MASK       │  │ (from archetype│             │
│  │  + BALANCE)    │  │ (from ENERGY)  │  │   morph)       │             │
│  │                │  │                │  │                │             │
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
2. **Compute eligibility mask** based on ENERGY (which steps *can* fire)
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
| ~~Audio In R~~ | ~~IN_R~~ | ~~Removed~~ (timing controlled by GENRE + SWING config) |
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

### 3.3 Audio Input Processing

Audio inputs are repurposed as CV inputs for additional control:

| Audio Input | Function | Behavior |
|-------------|----------|----------|
| IN_L | **Fill CV** | Gate (>1V) triggers fill; CV level (0-5V) = fill intensity |
| ~~IN_R~~ | ~~**Flavor CV**~~ | ~~Removed~~ (timing controlled by GENRE + SWING config) |

### 3.4 Clock and Reset Behavior [exclusive-external-clock]

The sequencer supports both internal and external clock sources with **exclusive** operation:

#### Clock Source Selection

| Condition | Clock Source | Step Advancement |
|-----------|--------------|------------------|
| **Gate In 1 unpatched** | Internal Metro at configured BPM | Steps advance on internal clock ticks |
| **Gate In 1 patched** | External clock only | Steps advance ONLY on rising edges, internal clock disabled |

**Acceptance Criteria**:
- ✅ When external clock is patched, internal Metro is completely disabled (not just suppressed)
- ✅ Steps advance only on rising edges detected at Gate In 1
- ✅ No timeout-based fallback to internal clock while external clock is patched
- ✅ Unplugging external clock restores internal clock operation immediately
- ✅ Clock source detection is deterministic and predictable

#### Reset Behavior

Reset (Gate In 2) operates identically regardless of clock source:

| Reset Mode | Behavior | Clock Source Independence |
|------------|----------|---------------------------|
| **PHRASE** | Reset to step 0, bar 0, regenerate pattern | ✅ Works with internal or external clock |
| **BAR** | Reset to step 0, keep current bar | ✅ Works with internal or external clock |
| **STEP** | Reset to step 0 only | ✅ Works with internal or external clock |

**Acceptance Criteria**:
- ✅ Reset detects rising edges reliably
- ✅ Reset behavior is identical whether using internal or external clock
- ✅ Reset respects configured reset mode in all cases

#### Implementation Simplifications

To ensure predictable behavior:

1. **No parallel clock operation**: Internal Metro stops completely when external clock detected
2. **No timeout logic**: External clock remains active until physically unplugged
3. **Simple rising edge detection**: Both clock and reset use consistent edge detection
4. **Immediate switchover**: Clock source changes take effect within one audio callback cycle

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

**Velocity ranges** (v4.1):
- velocityFloor: 65% down to 30% (widened for more contrast)
- accentBoost: +15% to +45% (increased for punch)
- Minimum output velocity: 30% (for VCA audibility)

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

**BUILD operates in three phases** (v4.1):

| Phrase % | Phase | Density | Velocity | Notes |
|----------|-------|---------|----------|-------|
| 0-60% | GROOVE | 1.0× | normal | Stable pattern |
| 60-87.5% | BUILD | +0-35% | +0-8% floor | Ramping energy |
| 87.5-100% | FILL | +35-50% | +8-12% floor | All accents (BUILD>60%) |

Phase boundaries are computed from phrase progress, respecting configured phrase length.

### 4.5 Config Mode Controls — Domain-Based

#### K1: GRID Domain (Loop Architecture)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **PATTERN LENGTH** | 16 / 24 / 32 / 64 | Steps per bar |
| Shift | **PHRASE LENGTH** | 1 / 2 / 4 / 8 | Bars per phrase |

#### K2: TIMING Domain (Feel)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **SWING** | 0-100% | Base swing amount (combined with FLAVOR) |
| Shift | **CLOCK DIV** | ÷8 / ÷4 / ÷2 / ×1 / ×2 / ×4 / ×8 | Clock division/multiplication (applies to both internal and external clock) |

#### K3: OUTPUT Domain (Signal Configuration)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **AUX MODE** | Hat / Fill Gate / Phrase CV / Event | What AUX output does |
| Shift | **AUX DENSITY** | 50% / 100% / 150% / 200% | Aux hit budget multiplier |

#### K4: BEHAVIOR Domain (Module Response)

| Mode | Parameter | Values | Function |
|------|-----------|--------|----------|
| Primary | **RESET MODE** | Phrase / Bar / Step | What reset input does |
| Shift | **VOICE COUPLING** | Independent / Interlock / Shadow | Voice relationship override |

### 4.6 Button Behavior

| Gesture | Action | Timing |
|---------|--------|--------|
| **Tap** (<200ms) | Queue fill for next phrase | Immediate feedback via LED |
| **Hold** (>200ms) | Shift modifier active | LED dims while held |
| **Hold** (>500ms, no knob moved) | Live fill mode (temporary boost) | LED pulses |
| **Double-tap** (<400ms between) | Reseed pattern at next phrase boundary | LED flashes 100% |

### 4.7 Energy Zones

ENERGY doesn't just scale density—it changes behavioral rules:

| Zone | ENERGY Range | Characteristics |
|------|--------------|-----------------|
| **MINIMAL** | 0-20% | Sparse, skeleton only, large gaps allowed, tight timing |
| **GROOVE** | 20-50% | Stable, danceable, locked pattern, moderate fills, tight timing |
| **BUILD** | 50-75% | Increasing ghosts, phrase-end fills, aux active, timing loosens |
| **PEAK** | 75-100% | Maximum activity, ratchets allowed, all voices busy, expressive timing |

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

When FIELD X/Y is between grid points, blend using softmax with temperature. Lower temperature = more winner-take-all behavior, preserving archetype character during transitions.

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

---

## 6. Generation Pipeline

### 6.1 Hit Budget Calculation

Hit budgets guarantee density matches intent. Budget is calculated from ENERGY + BALANCE + zone, then BUILD modifiers adjust for phrase position.

### 6.2 Eligibility Mask Computation

Standard masks for 32-step patterns define which metric positions can fire based on ENERGY level.

### 6.3 Gumbel Top-K Selection

Weighted sampling without replacement using Gumbel noise provides deterministic, seeded hit selection with spacing rules to prevent clumping.

### 6.3.1 Euclidean Foundation (Genre-Dependent, v4.1)

Before Gumbel Top-K selection, an optional Euclidean foundation guarantees even hit distribution:

| Genre | Base Euclidean Ratio | Notes |
|-------|---------------------|-------|
| Techno | 70% at Field X=0 | Ensures four-on-floor at low complexity |
| Tribal | 40% at Field X=0 | Balances structure with polyrhythm |
| IDM | 0% (disabled) | Allows maximum irregularity |

- Field X reduces Euclidean ratio by up to 70% (at X=1.0, ratio ≈ 0.3× base)
- Only active in MINIMAL and GROOVE zones
- Rotation derived from seed for determinism

### 6.4 Voice Relationship

VOICE COUPLING config parameter controls voice interaction:
- **Independent (0-33%)**: Voices fire freely, can overlap
- **Interlock (33-67%)**: Suppress simultaneous hits, call-response feel
- **Shadow (67-100%)**: Shimmer echoes anchor with 1-step delay

### 6.5 Soft Repair Pass

Bias rescue steps if constraints nearly violated—swap weakest hit for strongest rescue candidate without changing total hit count.

### 6.6 Hard Guard Rails

Force corrections only if still violating:
- Downbeat protection (force anchor on beat 1 if missing)
- Max gap rule (no more than 8 steps without anchor in GROOVE+)
- Max consecutive shimmer (4 unless PEAK zone)
- Genre-specific floors (e.g., Techno backbeat)

### 6.7 DRIFT Seed System

DRIFT controls pattern evolution:
- **patternSeed**: Fixed per "song", changes only on reseed
- **phraseSeed**: Changes each phrase, derived from patternSeed + counter
- Step stability determines which seed is used (downbeats use locked seed longer)

---

## 7. Timing System (BROKEN Stack)

### 7.1 BROKEN Stack Overview

Timing is controlled by **GENRE** (Performance+Shift K2) and **SWING** (Config K2):

| Layer | Control | Range | Zone Limit |
|-------|---------|-------|------------|
| **Swing** | Config K2 (SWING) | 0-100% (straight → heavy triplet) | GENRE-dependent max |
| **Microtiming Jitter** | GENRE-based | ±0ms (Techno) to ±12ms (IDM) | Genre profiles |
| **Step Displacement** | GENRE-based | Never (Techno) to 40% chance (IDM) | Genre profiles |
| **Velocity Chaos** | PUNCH parameter | ±0% to ±25% variation | Always allowed |

**Note**: Audio In R (FLAVOR CV) removed in v4. Timing feel is now determined by GENRE selection and SWING config only.

### 7.2 Velocity Computation (PUNCH-driven)

Velocity is controlled by the PUNCH parameter:
- **accentProbability**: How often hits are accented (20% to 50%)
- **velocityFloor**: Minimum velocity for non-accented hits (65% down to 30%)
- **accentBoost**: How much louder accents are (+15% to +45%)
- **velocityVariation**: Random variation range (±3% to ±15%)
- **Minimum output**: 30% (ensures audibility through VCA)

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

### 8.2 Velocity Output (Sample & Hold)

Velocity outputs use **sample & hold** behavior—the voltage is set on trigger and held until the next trigger on that channel.

### 8.3 AUX Output Modes

| Mode | Signal | Description |
|------|--------|-------------|
| HAT | Trigger | Third trigger voice (ghost/hi-hat pattern) |
| FILL_GATE | Gate | High during fill zones |
| PHRASE_CV | 0-5V | Ramp over phrase, resets at loop boundary |
| EVENT | Trigger | Fires on "interesting" moments (accents, fills, section changes) |

---

## 9. LED Feedback System

### 9.1 LED Output (CV_OUT_2)

| Brightness | Meaning |
|------------|---------|
| 0% | Off (no activity) |
| 30% | Shimmer trigger |
| 80% | Anchor trigger |
| 100% | Flash (mode change, reset, reseed) |
| Pulse | Live fill mode active |
| Gradient | Continuous parameter being adjusted |

---

## 10. Data Structures

See implementation pseudocode section for complete struct definitions including:
- `ArchetypeDNA` - Pattern archetype with weights and metadata
- `GenreField` - 3×3 grid of archetypes per genre
- `ControlState` - All runtime control parameters
- `SequencerState` - Position, hit masks, event flags
- `DuoPulseState` - Complete firmware state

---

## 11. Implementation Pseudocode

### 11.1 Main Audio Callback

Core audio callback processes clock, advances steps, generates bar patterns, computes step outputs, and writes to hardware outputs.

### 11.2 Bar Generation

Per-bar generation computes hit budgets, eligibility masks, selects hits via Gumbel Top-K, applies voice relationship, runs repair passes, and applies guard rails.

### 11.3 Step Processing

Per-step processing checks hit masks, computes velocities from PUNCH, applies BROKEN timing stack (swing, jitter, displacement), and updates event flags.

### 11.4 Control Processing

Control processing reads hardware, applies CV modulation, handles mode/shift switching, and computes derived parameters.

---

## 12. Configuration & Persistence

### 12.1 Auto-Save System

Config changes are automatically persisted to flash with 2-second debouncing to minimize flash wear.

### 12.2 What Gets Saved

| Category | Parameters |
|----------|------------|
| **Config Primary** | Pattern length, swing, AUX mode, reset mode |
| **Config Shift** | Phrase length, clock div, aux density, voice coupling |
| **Performance Shift** | Genre |
| **Pattern Seed** | Current pattern seed |

**Not Saved** (read from knobs on boot):
- ENERGY, BUILD, FIELD X/Y (primary performance controls)
- PUNCH, DRIFT, BALANCE (shift performance controls)

---

## 13. Runtime Logging System [runtime-logging]

### 13.1 Overview

DuoPulse v4 includes a compile-time and runtime configurable logging system for debugging and development without needing to rebuild firmware. The system follows embedded best practices:

- **Compile-time level cap**: Strip debug logs from release builds (zero cost)
- **Runtime level control**: Adjust verbosity without rebuilding firmware
- **USB serial transport**: Uses libDaisy's logger (StartLog/PrintLine)
- **Real-time safe**: No logging from audio callback (main loop only)

### 13.2 Log Levels

The system provides five log levels:

| Level | Value | Purpose | Example Use Case |
|-------|-------|---------|------------------|
| **TRACE** | 0 | Verbose debugging | Per-step state dumps, loop internals |
| **DEBUG** | 1 | Development info | Bar generation, archetype selection, pattern changes |
| **INFO** | 2 | Normal operation | Boot messages, mode changes, config updates |
| **WARN** | 3 | Warnings | Constraint violations, soft repair triggers |
| **ERROR** | 4 | Critical issues | Hardware init failures, invalid state |
| **OFF** | 5 | Disable logging | Production builds |

### 13.3 Compile-Time Configuration

Build-time flags in `Makefile` control what gets compiled into the binary:

```makefile
# Development build (keep DEBUG+ logs, default to INFO at runtime)
CFLAGS += -DLOG_COMPILETIME_LEVEL=1  # 0=TRACE, 1=DEBUG, 2=INFO, etc.
CFLAGS += -DLOG_DEFAULT_LEVEL=2      # Default runtime level

# Release build (only WARN/ERROR, quiet by default)
CFLAGS += -DLOG_COMPILETIME_LEVEL=3
CFLAGS += -DLOG_DEFAULT_LEVEL=3
```

**Acceptance Criteria**:
- ✅ Logs below `LOG_COMPILETIME_LEVEL` are stripped at compile time (zero code size)
- ✅ `LOG_DEFAULT_LEVEL` sets initial runtime filter level

### 13.4 Runtime Control

Runtime log level can be adjusted programmatically:

```cpp
logging::SetLevel(logging::DEBUG);  // Show DEBUG+ logs
logging::SetLevel(logging::WARN);   // Only show WARN/ERROR
```

**Future enhancement**: Add control via button combo (e.g., Shift + double-tap cycles levels)

**Acceptance Criteria**:
- ✅ Runtime level filter works even if compile-time level allows logs
- ✅ Changing runtime level takes effect immediately
- ✅ Runtime level defaults to `LOG_DEFAULT_LEVEL` on boot

### 13.5 Usage Pattern

Logging macros include file/line info for easy debugging:

```cpp
#include "logging.h"

// In main.cpp Init():
logging::Init(true);  // Wait for host to open serial (optional)

// In application code:
LOGT("Trace: per-step state dump");           // TRACE
LOGD("Debug: selected archetype [%d,%d]", x, y);  // DEBUG
LOGI("Info: mode changed to %s", modeName);   // INFO
LOGW("Warning: guard rail triggered");        // WARN
LOGE("Error: hardware init failed");          // ERROR
```

**Output format**:
```
[INFO] main.cpp:142 boot
[DEBUG] Engine/GenerationPipeline.cpp:87 selected archetype [1,2]
[WARN] Engine/GuardRails.cpp:45 downbeat missing, forcing anchor
```

### 13.6 Real-Time Safety

**Critical Rule**: Never log from audio callback. USB serial I/O is blocking and will cause audio dropouts.

**Accepted patterns**:
- ✅ Log from `main()` loop during initialization
- ✅ Log from control processing (button/knob handlers)
- ✅ Log before/after bar generation (outside audio callback)

**Forbidden**:
- ❌ Logging inside `AudioCallback()`
- ❌ `PrintLine()` directly from sample processing

**Future enhancement**: Ring buffer event system for audio-safe logging (push events from callback, flush from main loop)

**Acceptance Criteria**:
- ✅ No logging calls in audio callback path
- ✅ Audio timing unaffected by logging activity

### 13.7 Implementation Requirements

**Core Files**:
- `src/System/logging.h` - Level enum, macros, API declarations
- `src/System/logging.cpp` - Implementation using DaisyPatchSM::StartLog/PrintLine

**API**:
```cpp
namespace logging {
    void Init(bool wait_for_pc = true);  // Call after hw.Init()
    void SetLevel(Level lvl);
    Level GetLevel();
    void Print(Level lvl, const char* file, int line, const char* fmt, ...);
}
```

**Macros** (compile-time + runtime gating):
```cpp
LOGT(...) // TRACE
LOGD(...) // DEBUG
LOGI(...) // INFO
LOGW(...) // WARN
LOGE(...) // ERROR
```

**Acceptance Criteria**:
- ✅ `logging::Init()` initializes USB serial logger
- ✅ Macros include file:line info automatically
- ✅ Printf-style formatting supported (`%d`, `%s`, `%f`, etc.)
- ✅ Message buffer sized to avoid truncation (192 chars minimum)
- ✅ Stack-only allocation (no heap, safe for embedded)

### 13.8 Testing Requirements

**Unit Tests**:
- Verify compile-time gating strips logs below threshold
- Verify runtime filter prevents logs below current level
- Test all five log levels produce correct output format

**Integration Tests**:
- Boot with wait_for_pc=true, verify no messages missed
- Change runtime level, verify immediate effect
- Stress test: 100+ logs in main loop, verify no buffer overruns

**Hardware Validation**:
- Connect USB serial (screen/minicom), verify boot messages appear
- Adjust runtime level, verify log verbosity changes
- Confirm audio performance unaffected by logging

---

## 14. Testing Requirements

### 14.1 Unit Tests

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
| **ENERGY=30%, GENRE=Techno** | Steady techno groove, tight timing, predictable |
| **ENERGY=80%, GENRE=IDM, SWING=70%** | Busy broken beat, fills at phrase end, loose timing |
| **DRIFT=0%** | Identical pattern every phrase |
| **DRIFT=100%** | Varied pattern but same density feel |
| **Field [0,0] → [2,2]** | Clear progression from minimal to chaos |

---

## Appendix: Glossary

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
