# DuoPulse v4: Technical Architecture Reference

**Based on**: Current implementation in `src/` and `src/Engine/`
**Target Platform**: Daisy Patch.init() (Electro-Smith)
**Version**: 4.0 Implementation

This document maps the DuoPulse v4 specification to actual code locations, providing file:line references for key components.

---

## Table of Contents

1. [Directory Structure](#1-directory-structure)
2. [Entry Point & Audio Callback](#2-entry-point--audio-callback)
3. [Type System](#3-type-system)
4. [State Management](#4-state-management)
5. [Generation Pipeline](#5-generation-pipeline)
6. [Archetype System](#6-archetype-system)
7. [Pattern Field Morphing](#7-pattern-field-morphing)
8. [Hit Selection (Gumbel Sampler)](#8-hit-selection-gumbel-sampler)
9. [Voice Relationships](#9-voice-relationships)
10. [Guard Rails](#10-guard-rails)
11. [Timing System (BROKEN Stack)](#11-timing-system-broken-stack)
12. [Velocity System](#12-velocity-system)
13. [Pattern Evolution (DRIFT)](#13-pattern-evolution-drift)
14. [Output System](#14-output-system)
15. [Control Flow Diagram](#15-control-flow-diagram)
16. [Key Constants Reference](#16-key-constants-reference)

---

## 1. Directory Structure

```
src/
├── main.cpp                      # Entry point, audio callback, controls
└── Engine/                       # Core sequencer logic (~48 files)
    ├── Core Types
    │   ├── DuoPulseTypes.h       # Enums: Genre, Voice, EnergyZone, etc.
    │   ├── ArchetypeDNA.h        # Archetype definition struct
    │   └── PhrasePosition.h      # Phrase tracking
    │
    ├── State Management
    │   ├── DuoPulseState.h       # Master state container
    │   ├── ControlState.h        # All 16 control parameters
    │   ├── SequencerState.h      # Position, masks, timing
    │   └── OutputState.h         # Trigger/velocity/LED states
    │
    ├── Generation Pipeline
    │   ├── HitBudget.h/cpp       # Hit count computation
    │   ├── GumbelSampler.h/cpp   # Weighted hit selection
    │   ├── VoiceRelation.h/cpp   # Voice interaction logic
    │   ├── GuardRails.h/cpp      # Constraint enforcement
    │   └── Sequencer.h/cpp       # Pipeline orchestration
    │
    ├── Pattern System
    │   ├── ArchetypeData.h/cpp   # Pre-defined archetype tables
    │   ├── PatternField.h/cpp    # 2D morphing system
    │   └── DriftControl.h        # Pattern evolution seeds
    │
    ├── Timing & Velocity
    │   ├── BrokenEffects.h/cpp   # Swing, jitter, displacement
    │   └── VelocityCompute.h/cpp # PUNCH-driven dynamics
    │
    ├── Outputs
    │   ├── GateScaler.h/cpp      # Gate voltage processing
    │   ├── VelocityOutput.h/cpp  # Sample & hold outputs
    │   └── LedIndicator.h/cpp    # LED feedback
    │
    └── Persistence
        ├── Persistence.h/cpp     # Flash save/load
        └── SoftKnob.h/cpp        # Knob smoothing

inc/
└── config.h                      # Build flags, debug modes
```

---

## 2. Entry Point & Audio Callback

### Main Entry: `src/main.cpp`

**Key Global State** (lines 45-90):
```cpp
DaisyPatchSM patch;                    // Hardware abstraction
Sequencer sequencer;                   // Main engine
Switch tapButton, modeSwitch;          // User input
GateScaler anchorGate, shimmerGate;    // Output processors
SoftKnob softKnobs[16];                // 4 knobs × 4 modes
```

**Control State Structure** (`src/main.cpp:89-150`):
```cpp
struct MainControlState {
    // Performance Primary (CV-able)
    float energy, build, fieldX, fieldY;

    // Performance Shift
    float punch, genre, drift, balance;

    // Config Primary
    float patternLength, swing, auxMode, resetMode;

    // Config Shift
    float phraseLength, clockDiv, auxDensity, voiceCoupling;

    ControlMode mode;  // Which mode/shift combination is active
};
```

**Audio Callback** (`src/main.cpp:194-248`):
```cpp
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out, size_t size) {
    // 1. External clock detection (Gate In 1)
    // 2. Call sequencer.ProcessAudio() → returns [anchor_vel, shimmer_vel]
    // 3. Write gate triggers (Gate Out 1/2)
    // 4. Output velocity as 0-5V audio signals
    // 5. Auto-save processing
}
```

**Control Processing** (`src/main.cpp:300-522`):
- Called in main loop at ~1kHz
- Handles mode switch, shift button, soft knobs
- CV modulation application
- Reset and fill trigger handling

---

## 3. Type System

### Core Enums: `src/Engine/DuoPulseTypes.h`

**Genre** (line ~45):
```cpp
enum class Genre : uint8_t {
    TECHNO = 0,  // Four-on-floor, driving
    TRIBAL = 1,  // Syncopated, polyrhythmic
    IDM    = 2,  // Displaced, fragmented
    COUNT  = 3
};
```

**Voice** (line ~55):
```cpp
enum class Voice : uint8_t {
    ANCHOR  = 0,  // Gate Out 1 (kick-like)
    SHIMMER = 1,  // Gate Out 2 (snare-like)
    AUX     = 2,  // CV Out 1 (hi-hat)
    COUNT   = 3
};
```

**EnergyZone** (line ~65):
```cpp
enum class EnergyZone : uint8_t {
    MINIMAL = 0,  // 0-20%:   Sparse, tight timing
    GROOVE  = 1,  // 20-50%:  Stable, locked
    BUILD   = 2,  // 50-75%:  Density increasing
    PEAK    = 3,  // 75-100%: Maximum activity
    COUNT   = 4
};
```

**Other Enums**:
- `AuxMode`: HAT, FILL_GATE, PHRASE_CV, EVENT (line ~80)
- `AuxDensity`: SPARSE, NORMAL, DENSE, BUSY (line ~90)
- `VoiceCoupling`: INDEPENDENT, INTERLOCK, SHADOW (line ~100)
- `ResetMode`: PHRASE, BAR, STEP (line ~110)

**Constants** (line ~20):
```cpp
constexpr int kMaxSteps = 32;              // 2 bars at 16th notes
constexpr int kMaxPhraseSteps = 256;       // 8 bars
constexpr int kArchetypesPerGenre = 9;     // 3×3 grid
constexpr int kNumGenres = 3;
```

---

## 4. State Management

### Master State Container: `src/Engine/DuoPulseState.h`

**DuoPulseState** (lines 1-203):
```cpp
struct DuoPulseState {
    ControlState controls;           // All knob/CV parameters
    SequencerState sequencer;        // Position and patterns
    OutputState outputs;             // Gates/velocities/LED

    GenreField currentField;         // 3×3 archetype grid
    ArchetypeDNA blendedArchetype;   // Result of FIELD X/Y morphing

    float sampleRate;
    float samplesPerStep;
    float currentBpm;
    bool running;

    void Init(float sampleRateHz);
    void UpdateSamplesPerStep();
    void SetBpm(float bpm);
    float GetPhraseProgress() const;
    void AdvanceStep();
};
```

### Control State: `src/Engine/ControlState.h` (lines 181-384)

```cpp
struct ControlState {
    // Performance Primary (CV-able)
    float energy;      // K1: Hit density (0-1)
    float build;       // K2: Phrase arc (0-1)
    float fieldX;      // K3: Syncopation (0-1)
    float fieldY;      // K4: Complexity (0-1)

    // Performance Shift
    float punch;       // Shift+K1: Velocity dynamics
    Genre genre;       // Shift+K2: Style bank
    float drift;       // Shift+K3: Evolution rate
    float balance;     // Shift+K4: Voice ratio

    // Config Primary
    int patternLength; // 16/24/32/64 steps
    float swing;       // 0-1
    AuxMode auxMode;
    ResetMode resetMode;

    // Config Shift
    int phraseLength;  // 1/2/4/8 bars
    int clockDivision;
    AuxDensity auxDensity;
    VoiceCoupling voiceCoupling;

    // Derived
    EnergyZone energyZone;
    PunchParams punchParams;
    BuildModifiers buildModifiers;

    // CV modulation (raw values)
    float energyCV, buildCV, fieldXCV, fieldYCV, flavorCV;

    float GetEffectiveEnergy() const;  // knob + CV
};
```

### Sequencer State: `src/Engine/SequencerState.h` (lines 102-413)

```cpp
struct SequencerState {
    // Position
    int currentBar;
    int currentStep;      // 0-31
    long totalSteps;

    // Hit masks (bitmask per voice)
    uint32_t anchorMask;
    uint32_t shimmerMask;
    uint32_t auxMask;
    uint32_t anchorAccentMask;
    uint32_t shimmerAccentMask;

    // Timing offsets (samples per step)
    int timingOffsets[32];

    // Boundaries
    bool isBarBoundary;
    bool isPhraseBoundary;

    // Seeds
    DriftState driftState;
    GuardRailState guardRailState;
};
```

### Output State: `src/Engine/OutputState.h` (lines 1-490)

```cpp
struct TriggerState {
    bool high;
    int samplesRemaining;
    void Fire();
    void Process();
};

struct VelocityOutputState {
    float heldVoltage;      // 0-1 (maps to 0-5V)
    float targetVoltage;
    bool triggered;
};

struct OutputState {
    TriggerState anchorTrigger;      // Gate Out 1
    TriggerState shimmerTrigger;     // Gate Out 2
    VelocityOutputState anchorVelocity;   // Audio Out L
    VelocityOutputState shimmerVelocity;  // Audio Out R

    void FireAnchor(float velocity, bool accented);
    void FireShimmer(float velocity, bool accented);
};
```

---

## 5. Generation Pipeline

### Sequencer: `src/Engine/Sequencer.h` (lines 35-312)

**Main Interface**:
```cpp
class Sequencer {
public:
    void Init(float sampleRate);
    std::array<float, 2> ProcessAudio();  // Returns [anchor_vel, shimmer_vel]

    // Per-bar generation
    void GenerateBar();
    void ProcessStep();
    void AdvanceStep();

    // Parameter setters (Performance Primary)
    void SetEnergy(float value);
    void SetBuild(float value);
    void SetFieldX(float value);
    void SetFieldY(float value);

    // ... (16 total parameter setters)
};
```

**ProcessAudio Implementation** (`src/Engine/Sequencer.cpp:77-180`):
```
1. Clock Processing (internal metro or external)
2. On step tick:
   a. AdvanceStep()
   b. On bar boundary: GenerateBar()
   c. ProcessStep() to check hits
3. Delayed trigger processing (swing/jitter)
4. Return velocity values
```

**GenerateBar Pipeline** (`src/Engine/Sequencer.cpp:182+`):
```
1. BlendArchetype()           → FIELD X/Y morphing
2. ComputeBarBudget()         → Hit counts from ENERGY/BALANCE
3. SelectHitsGumbelTopK()     → Weighted selection
4. ApplyVoiceRelationship()   → INTERLOCK/SHADOW/INDEPENDENT
5. ApplyHardGuardRails()      → Constraint enforcement
6. ComputeVelocities()        → PUNCH/BUILD dynamics
7. ComputeTimingOffsets()     → FLAVOR/BROKEN timing
```

---

## 6. Archetype System

### Archetype DNA: `src/Engine/ArchetypeDNA.h` (lines 20-158)

```cpp
struct ArchetypeDNA {
    // Step weights (32 steps)
    float anchorWeights[32];
    float shimmerWeights[32];
    float auxWeights[32];

    // Accent eligibility
    uint32_t anchorAccentMask;
    uint32_t shimmerAccentMask;

    // Timing characteristics
    float swingAmount;        // 0-1
    float swingPattern;       // 0=8ths, 1=16ths, 2=mixed

    // Voice relationship default
    float defaultCouple;      // 0-1

    // Fill behavior
    float fillDensityMultiplier;
    uint32_t ratchetEligibleMask;

    // Grid position
    uint8_t gridX, gridY;     // 0-2 each
};

struct GenreField {
    ArchetypeDNA archetypes[3][3];  // [y][x]
};
```

### Archetype Data: `src/Engine/ArchetypeData.h`

Contains 27 pre-defined archetypes (9 per genre × 3 genres):

**Grid Layout**:
```
Y (complexity)
^
2  [0,2] Busy      [1,2] Polyrhythm  [2,2] Chaos
1  [0,1] Driving   [1,1] Groovy      [2,1] Broken
0  [0,0] Minimal   [1,0] Steady      [2,0] Displaced
   0              1                 2      → X (syncopation)
```

**Example** (Techno Minimal):
```cpp
namespace techno {
    constexpr float kMinimal_Anchor[32] = { 1.0f, 0.0f, 0.0f, 0.0f, ... };
    // Four-on-floor kick pattern
}
```

---

## 7. Pattern Field Morphing

### PatternField: `src/Engine/PatternField.h` (159 lines)

**Main Function**:
```cpp
void GetBlendedArchetype(
    const GenreField& field,     // 3×3 grid of archetypes
    float fieldX, float fieldY,  // 0-1 position in grid
    float temperature,           // Softmax sharpness (default 0.5)
    ArchetypeDNA& outArchetype   // Result
);
```

**Implementation** (`src/Engine/PatternField.cpp:10+`):
```cpp
// 1. Map 0-1 to grid coordinates
float gx = fieldX * 2.0f;  // 0-2
float gy = fieldY * 2.0f;  // 0-2

// 2. Find 4 surrounding archetypes
int x0 = (int)gx, y0 = (int)gy;
int x1 = min(x0+1, 2), y1 = min(y0+1, 2);

// 3. Compute bilinear weights
float fx = gx - x0, fy = gy - y0;
float w00 = (1-fx)*(1-fy), w10 = fx*(1-fy), etc.

// 4. Apply softmax with temperature (winner-take-more)
SoftmaxWithTemperature(weights, 4, temperature);

// 5. Blend continuous properties, select discrete from dominant
```

---

## 8. Hit Selection (Gumbel Sampler)

### GumbelSampler: `src/Engine/GumbelSampler.h` (173 lines)

**Main Function**:
```cpp
uint32_t SelectHitsGumbelTopK(
    const float* weights,        // Step weights from archetype
    uint32_t eligibilityMask,    // Which steps can fire
    int targetCount,             // Budget (how many hits)
    uint32_t seed,               // Deterministic randomness
    int patternLength,           // Usually 32
    int minSpacing               // Prevent clumping
);
```

**Algorithm** (`src/Engine/GumbelSampler.cpp:8+`):
```cpp
// For each step:
score[i] = log(weight[i]) + GumbelNoise(seed, i)

// Select top K scores respecting:
// - Eligibility mask
// - Minimum spacing between hits
// - Already selected steps
```

**Key Helpers**:
```cpp
float HashToFloat(uint32_t seed, int step);  // Deterministic [0,1)
float UniformToGumbel(float uniform);        // Transform to Gumbel distribution
```

---

## 9. Voice Relationships

### VoiceRelation: `src/Engine/VoiceRelation.h` (165 lines)

**Main Function**:
```cpp
void ApplyVoiceRelationship(
    uint32_t anchorMask,
    uint32_t& shimmerMask,       // Modified in place
    VoiceCoupling coupling,
    int patternLength
);
```

**Coupling Modes** (`src/Engine/VoiceRelation.cpp`):

| Mode | Effect | Implementation |
|------|--------|----------------|
| INDEPENDENT | No interaction | Return unchanged |
| INTERLOCK | Suppress simultaneous | Remove shimmer where anchor fires |
| SHADOW | Echo anchor +1 step | Shift and OR into shimmer |

```cpp
void ApplyInterlock(uint32_t anchorMask, uint32_t& shimmerMask, int len) {
    shimmerMask &= ~anchorMask;  // Remove overlapping hits
}

void ApplyShadow(uint32_t anchorMask, uint32_t& shimmerMask, int len) {
    uint32_t echo = ShiftMaskLeft(anchorMask, 1, len);
    shimmerMask |= echo;  // Add echo hits
}
```

---

## 10. Guard Rails

### GuardRails: `src/Engine/GuardRails.h` (220 lines)

**Two-Phase System**:

**Phase 1: Soft Repair** (`src/Engine/GuardRails.cpp`):
```cpp
int SoftRepairPass(
    uint32_t& anchorMask,
    uint32_t& shimmerMask,
    const float* anchorWeights,
    const float* shimmerWeights,
    EnergyZone zone,
    int patternLength
);
// Proactively swap weakest hit for rescue candidate
```

**Phase 2: Hard Guard Rails**:
```cpp
int ApplyHardGuardRails(
    uint32_t& anchorMask,
    uint32_t& shimmerMask,
    EnergyZone zone,
    Genre genre,
    int patternLength
);
```

**Individual Rules**:

| Rule | Function | Behavior |
|------|----------|----------|
| Downbeat Protection | `EnforceDownbeat()` | Force anchor on step 0 (GROOVE+) |
| Max Gap | `EnforceMaxGap()` | No gap > zone max (8 GROOVE, 6 BUILD, 4 PEAK) |
| Consecutive Shimmer | `EnforceConsecutiveShimmer()` | Limit shimmer bursts (4 normal, 6 PEAK) |
| Genre Rules | `EnforceGenreRules()` | Techno backbeat, etc. |

---

## 11. Timing System (BROKEN Stack)

### BrokenEffects: `src/Engine/BrokenEffects.h` (lines 1-232)

**Four Timing Layers** (controlled by FLAVOR parameter):

| Layer | Source | Effect Range |
|-------|--------|--------------|
| Swing | `GetSwingFromBroken()` | 50% → 66% |
| Jitter | `GetJitterMsFromBroken()` | ±0ms → ±12ms |
| Displacement | During fills | ±1-2 steps |
| Velocity Chaos | `VelocityCompute` | ±0% → ±25% |

**Zone Clamping** (`src/Engine/BrokenEffects.cpp`):
```cpp
// GROOVE zone enforces tight timing:
if (zone == EnergyZone::GROOVE) {
    swing = min(swing, 0.58f);
    jitter = min(jitter, 3.0f);  // ms
    // Displacement disabled
}
```

**Swing Calculation**:
```cpp
float GetSwingFromBroken(float broken) {
    return 0.50f + broken * 0.16f;  // 50% to 66%
}
```

**Jitter Application**:
```cpp
float ApplyJitter(float maxJitterMs, uint32_t seed, int step) {
    float jitterNorm = HashToFloat(seed, step + 5000) * 2.0f - 1.0f;
    return jitterNorm * maxJitterMs;  // ±ms
}
```

---

## 12. Velocity System

### VelocityCompute: `src/Engine/VelocityCompute.h` (152 lines)

**PUNCH Parameters** (`src/Engine/VelocityCompute.cpp`):
```cpp
struct PunchParams {
    float accentProbability;    // 15-50%
    float velocityFloor;        // 70-30%
    float accentBoost;          // +10-+35%
    float velocityVariation;    // ±5-±20%
};

void ComputePunch(float punch, PunchParams& params) {
    params.accentProbability = 0.15f + punch * 0.35f;
    params.velocityFloor = 0.7f - punch * 0.4f;
    params.accentBoost = 0.1f + punch * 0.25f;
    params.velocityVariation = 0.05f + punch * 0.15f;
}
```

**BUILD Modifiers**:
```cpp
struct BuildModifiers {
    float densityMultiplier;    // 1.0-1.5
    float fillIntensity;        // 0-1
    bool inFillZone;            // Last 12.5% of phrase
    float phraseProgress;       // 0-1
};

void ComputeBuildModifiers(float build, float phraseProgress, BuildModifiers& mods) {
    float curve = phraseProgress * phraseProgress;  // Exponential
    mods.densityMultiplier = 1.0f + build * curve * 0.5f;
    mods.fillIntensity = build;
    mods.inFillZone = (phraseProgress > 0.875f);  // Last 12.5%
}
```

**Velocity Calculation**:
```cpp
float ComputeVelocity(const PunchParams& punch, const BuildModifiers& build,
                      bool isAccent, uint32_t seed, int step) {
    float velocity;
    if (isAccent) {
        velocity = 0.8f + punch.accentBoost + build.accentBoost;
    } else {
        velocity = punch.velocityFloor;
    }

    // Add variation
    float variation = (HashToFloat(seed, step) * 2.0f - 1.0f) * punch.velocityVariation;
    return Clamp(velocity + variation, 0.2f, 1.0f);
}
```

---

## 13. Pattern Evolution (DRIFT)

### DriftControl: `src/Engine/DriftControl.h` (207 lines)

**Seed System**:
```cpp
struct DriftState {
    uint32_t patternSeed;     // Fixed per "song"
    uint32_t phraseSeed;      // Changes each phrase
    uint32_t phraseCounter;
    bool reseedRequested;

    uint32_t GetSeedForStep(float drift, float stepStability) const;
};
```

**Step Stability Hierarchy**:
```cpp
constexpr float kStabilityDownbeat = 1.0f;      // Step 0
constexpr float kStabilityHalfBar = 0.9f;       // Step 16
constexpr float kStabilityQuarter = 0.7f;       // 8, 24
constexpr float kStabilityEighth = 0.5f;        // 4, 12, 20, 28
constexpr float kStabilitySixteenth = 0.3f;     // Other even
constexpr float kStabilityOffbeat = 0.1f;       // Odd steps
```

**DRIFT Algorithm**:
```cpp
uint32_t SelectSeed(const DriftState& state, float drift, int step) {
    float stability = GetStepStability(step);
    if (stability > drift) {
        return state.patternSeed;   // Locked
    } else {
        return state.phraseSeed;    // Evolving
    }
}
```

**Effect of DRIFT**:
- DRIFT = 0%: All steps use patternSeed (fully locked)
- DRIFT = 50%: Only steps with stability > 0.5 locked (downbeats, halves, quarters)
- DRIFT = 100%: Only downbeat locked, everything else evolves

---

## 14. Output System

### Gate Output: `src/Engine/GateScaler.h`

```cpp
class GateScaler {
    void SetTargetVoltage(float voltage);  // 0 or 5V
    float Process();  // Per-sample smoothing

    static float VoltageToCodecSample(float voltage);
};
```

### Velocity Output: `src/Engine/VelocityOutput.h`

**Sample & Hold behavior**:
```cpp
class VelocityOutput {
    void SetVelocity(float velocity);  // Called on trigger
    float Process();  // Returns held voltage until next trigger
};
```

### LED Feedback: `src/Engine/LedIndicator.h`

| Brightness | Meaning |
|------------|---------|
| 0% | Idle |
| 30% | Shimmer trigger |
| 80% | Anchor trigger |
| 100% flash | Reset/reseed/mode change |
| Pulse | Live fill mode |

### AUX Output Modes: `src/main.cpp:400+`

| Mode | Output |
|------|--------|
| HAT | Third trigger voice |
| FILL_GATE | High during fills |
| PHRASE_CV | 0-5V ramp over phrase |
| EVENT | Trigger on accents/boundaries |

---

## 15. Control Flow Diagram

```
main()
├── Init hardware & sequencer
├── Load config from flash
└── while(1):
    │
    ├─→ ProcessControls() [~1kHz]
    │   ├── Read knobs/switches/CV
    │   ├── Soft knob smoothing
    │   ├── Update MainControlState
    │   └── Call sequencer.Set*()
    │
    └─→ AudioCallback() [48kHz ISR]
        │
        ├─→ sequencer.ProcessAudio()
        │   │
        │   ├─→ On step tick:
        │   │   ├── AdvanceStep()
        │   │   │
        │   │   └─→ On bar boundary:
        │   │       │
        │   │       ├── GetBlendedArchetype()
        │   │       │   └── PatternField.cpp
        │   │       │
        │   │       ├── ComputeBarBudget()
        │   │       │   └── HitBudget.cpp
        │   │       │
        │   │       ├── SelectHitsGumbelTopK()
        │   │       │   └── GumbelSampler.cpp
        │   │       │
        │   │       ├── ApplyVoiceRelationship()
        │   │       │   └── VoiceRelation.cpp
        │   │       │
        │   │       ├── ApplyHardGuardRails()
        │   │       │   └── GuardRails.cpp
        │   │       │
        │   │       ├── ComputeVelocities()
        │   │       │   └── VelocityCompute.cpp
        │   │       │
        │   │       └── ComputeTimingOffsets()
        │   │           └── BrokenEffects.cpp
        │   │
        │   └─→ Return [anchor_vel, shimmer_vel]
        │
        ├── Write Gate Out 1/2
        ├── Write Audio Out L/R (velocity)
        ├── Write CV Out 1 (AUX)
        └── Write CV Out 2 (LED)
```

---

## 16. Key Constants Reference

### Mask Constants (`src/Engine/HitBudget.h`)

```cpp
constexpr uint32_t kDownbeatMask     = 0x00010001;  // Steps 0, 16
constexpr uint32_t kHalfNoteMask     = 0x01010101;  // Steps 0, 8, 16, 24
constexpr uint32_t kQuarterNoteMask  = 0x11111111;  // Every 4 steps
constexpr uint32_t kEighthNoteMask   = 0x55555555;  // Every 2 steps
constexpr uint32_t kSixteenthMask    = 0xFFFFFFFF;  // All steps
constexpr uint32_t kBackbeatMask     = 0x01000100;  // Steps 8, 24
```

### Energy Zone Thresholds

```
0.00 ≤ MINIMAL < 0.20    → Sparse, tight timing
0.20 ≤ GROOVE  < 0.50    → Stable, locked
0.50 ≤ BUILD   < 0.75    → Density increasing
0.75 ≤ PEAK    ≤ 1.00    → Maximum activity
```

### Guard Rail Limits

| Zone | Max Gap | Max Consecutive Shimmer |
|------|---------|-------------------------|
| MINIMAL | No limit | 4 |
| GROOVE | 8 steps | 4 |
| BUILD | 6 steps | 4 |
| PEAK | 4 steps | 6 |

### Tempo Calculation

```cpp
samplesPerStep = (sampleRate * 60.0f) / (BPM * 4.0f)

// Example: 120 BPM @ 48kHz
samplesPerStep = (48000 * 60) / (120 * 4) = 6000 samples
               = 125 ms per step
```

### Debug Flags (`inc/config.h`)

| Flag | Effect |
|------|--------|
| `DEBUG_BASELINE_MODE` | Force ENERGY=0.75, FIELD=0.5 |
| `DEBUG_SIMPLE_TRIGGERS` | Bypass generation, fixed 4-on-floor |
| `DEBUG_FIXED_SEED` | Reproducible randomness |
| `DEBUG_LOG_MASKS` | Output masks as stepped voltages |
| `DEBUG_FEATURE_LEVEL` | Progressive enablement (0-5) |

---

## Quick Reference: File Locations

| Component | Header | Implementation |
|-----------|--------|----------------|
| Entry Point | - | `src/main.cpp` |
| Sequencer | `src/Engine/Sequencer.h` | `src/Engine/Sequencer.cpp` |
| Types | `src/Engine/DuoPulseTypes.h` | - |
| State | `src/Engine/DuoPulseState.h` | - |
| Controls | `src/Engine/ControlState.h` | - |
| Archetypes | `src/Engine/ArchetypeDNA.h` | `src/Engine/ArchetypeData.cpp` |
| Pattern Field | `src/Engine/PatternField.h` | `src/Engine/PatternField.cpp` |
| Hit Budget | `src/Engine/HitBudget.h` | `src/Engine/HitBudget.cpp` |
| Gumbel Sampler | `src/Engine/GumbelSampler.h` | `src/Engine/GumbelSampler.cpp` |
| Voice Relation | `src/Engine/VoiceRelation.h` | `src/Engine/VoiceRelation.cpp` |
| Guard Rails | `src/Engine/GuardRails.h` | `src/Engine/GuardRails.cpp` |
| BROKEN Effects | `src/Engine/BrokenEffects.h` | `src/Engine/BrokenEffects.cpp` |
| Velocity | `src/Engine/VelocityCompute.h` | `src/Engine/VelocityCompute.cpp` |
| DRIFT | `src/Engine/DriftControl.h` | - |
| Persistence | `src/Engine/Persistence.h` | `src/Engine/Persistence.cpp` |
| Config | `inc/config.h` | - |

---

## 17. Differences from Specification Documents

This section details the differences between the current implementation and the sibling specification documents:
- `duopulse-v4-spec_1.md` - Original specification
- `duopulse-v4-detailed.md` - Detailed implementation spec
- `duopulse-v4-visual-guide.md` - User-facing visual guide

### 17.1 Implemented as Specified

The following features are implemented as documented:

| Feature | Status | Notes |
|---------|--------|-------|
| 3×3 Archetype Grid | ✅ Complete | All 27 archetypes (9 per genre × 3 genres) |
| Gumbel Top-K Selection | ✅ Complete | Deterministic weighted sampling |
| Guard Rails | ✅ Complete | Downbeat protection, max gap, consecutive limits |
| DRIFT Evolution | ✅ Complete | Dual-seed system with step stability |
| Energy Zones | ✅ Complete | MINIMAL/GROOVE/BUILD/PEAK thresholds |
| Voice Coupling | ✅ Complete | INDEPENDENT/INTERLOCK/SHADOW modes |
| AUX Output Modes | ✅ Complete | HAT/FILL_GATE/PHRASE_CV/EVENT |
| CV Modulation | ✅ Complete | CV 1-4 modulate primary performance controls |
| Double-tap Reseed | ✅ Complete | 400ms window, queued for phrase boundary |

### 17.2 Implementation Differences

#### 17.2.1 Fill Zone Threshold

| Spec | Implementation |
|------|----------------|
| 12.5% of phrase (last bar of 8-bar) | Dynamic: 4-32 steps based on pattern length |

**Implementation** (`src/Engine/PhrasePosition.h:43-58`):
```cpp
// Fill zone: last 4 steps per bar of loop length (min 4, max 32)
int fillZoneSteps = loopLengthBars * 4;
if(fillZoneSteps < 4) fillZoneSteps = 4;
if(fillZoneSteps > 32) fillZoneSteps = 32;
pos.isFillZone = (stepsFromEnd <= fillZoneSteps);
```

The spec defines a fixed 12.5% threshold (`phraseProgress > 0.875f`), but the implementation uses a step-count approach that scales with pattern/phrase length.

#### 17.2.2 Control Mapping - Switch Position

| Spec | Implementation |
|------|----------------|
| Switch A (up) = Performance | Switch DOWN = Performance |
| Switch B (down) = Config | Switch UP = Config |

**Implementation** (`src/main.cpp:311-320`): The switch position is inverted compared to the spec labeling. The code checks `modeSwitch.Pressed()` where pressed (high) = Config mode.

#### 17.2.3 Button Timing Thresholds

| Gesture | Spec | Implementation |
|---------|------|----------------|
| Tap | <200ms | `kTapMaxMs` (configurable, typically 150-200ms) |
| Hold/Shift | >200ms | `kHoldThresholdMs` (configurable) |
| Double-tap window | <400ms | `kDoubleTapWindowMs = 400` ✅ |

**Implementation** (`src/Engine/ControlProcessor.h:22-25`):
```cpp
constexpr uint32_t kTapMaxMs = 150;
constexpr uint32_t kDoubleTapWindowMs = 400;
constexpr uint32_t kHoldThresholdMs = 300;
```

#### 17.2.4 Soft Knob System

| Spec | Implementation |
|------|----------------|
| Not mentioned | Full soft knob implementation with 16 virtual knobs |

**Implementation** (`src/main.cpp`, `src/Engine/SoftKnob.h`): The implementation includes a `SoftKnob` class that provides:
- Smooth ADC reading with configurable slew
- Dead zone handling
- Mode-switch awareness (4 modes × 4 knobs = 16 virtual knobs)
- "Takeover" behavior to avoid parameter jumps

#### 17.2.5 LED Feedback - Fill Zone Behavior

| Spec | Implementation |
|------|----------------|
| "Pulsing" during fill | Rapid triple-pulse pattern |

**Implementation** (`src/Engine/LedIndicator.h:110`, `LedIndicator.cpp:122-160`):
```cpp
static constexpr float kTriplePulseMs = 40.0f;  // Fill zone rapid pulse
float LedIndicator::ProcessFillZone(const LedState& state) { ... }
```

The implementation uses a specific "triple-pulse" pattern during fill zones rather than simple pulsing.

### 17.3 Extended Features (Not in Original Spec)

| Feature | Location | Description |
|---------|----------|-------------|
| **ControlProcessor class** | `src/Engine/ControlProcessor.h/cpp` | Dedicated class for all control processing logic |
| **PhrasePosition struct** | `src/Engine/PhrasePosition.h` | Tracks phrase position with computed fill/build zones |
| **AuxOutput class** | `src/Engine/AuxOutput.h/cpp` | Dedicated class for AUX output mode handling |
| **LedIndicator class** | `src/Engine/LedIndicator.h/cpp` | State machine for LED feedback |
| **Debug feature levels** | `inc/config.h` | Progressive feature enablement (0-5) for testing |

### 17.4 Spec Features Not Yet Implemented

| Feature | Status | Notes |
|---------|--------|-------|
| **Live Fill Mode** (hold >500ms) | ⚠️ Partial | Button hold works but timing/boost differs |
| **Fill CV pressure sensitivity** | ⚠️ Partial | Gate detection works, intensity scaling may differ |
| **Flavor CV from Audio In R** | ✅ Complete | Implemented as specified |
| **Ratchet system** | ⚠️ Partial | Mask exists but ratcheting logic simplified |

### 17.5 Pseudocode vs Implementation Mapping

The spec provides pseudocode that maps to actual implementation as follows:

| Spec Pseudocode | Implementation File | Key Differences |
|-----------------|---------------------|-----------------|
| `GenerateBar()` | `Sequencer.cpp:200-350` | Split into multiple helper methods |
| `ProcessStep()` | `Sequencer.cpp:400-500` | Includes timing delay queue |
| `ProcessControls()` | `ControlProcessor.cpp` + `main.cpp` | Split between classes |
| `ComputeBarBudget()` | `HitBudget.cpp` | Additional zone-aware scaling |
| `SelectHitsGumbelTopK()` | `GumbelSampler.cpp` | Includes spacing penalty system |
| `ApplyVoiceRelationship()` | `VoiceRelation.cpp` | Match spec closely |
| `ApplyHardGuardRails()` | `GuardRails.cpp` | Additional genre-specific rules |
| `ComputeSwing()` | `BrokenEffects.cpp` | Zone clamping more nuanced |
| `ComputeVelocity()` | `VelocityCompute.cpp` | BUILD modifier integration |
| `ProcessLED()` | `LedIndicator.cpp` | Full state machine vs simple logic |

### 17.6 Data Structure Differences

#### ControlState

| Spec Field | Implementation | Difference |
|------------|----------------|------------|
| `flavor` | `flavor` + `flavorCV` | Separate raw CV vs processed |
| `zone` | `energyZone` | Renamed for clarity |
| `fillQueued` | In `ButtonState` | Moved to dedicated struct |
| `liveFillActive` | In `ButtonState` | Moved to dedicated struct |

#### SequencerState

| Spec Field | Implementation | Difference |
|------------|----------------|------------|
| `blendedDNA` | In `DuoPulseState` | Lifted to parent struct |
| `guardRailState` | Inline in `SequencerState` | Kept as specified |
| `timingOffsets[32]` | Per-step in `ProcessStep()` | Computed on-demand, not stored |

### 17.7 Constants and Thresholds

| Constant | Spec Value | Implementation | Notes |
|----------|------------|----------------|-------|
| Sample rate | 48000 Hz | 48000 Hz | ✅ Match |
| Trigger length | 5ms | Configurable | `SetAccentHoldMs()`, `SetHihatHoldMs()` |
| Max pattern length | 64 steps | 32 steps | `kMaxSteps = 32` (2 bars at 16ths) |
| Max phrase steps | Not specified | 256 steps | `kMaxPhraseSteps = 256` |
| Softmax temperature | 0.4 | 0.5 | Slightly less "winner-take-all" |
| Max gap (GROOVE) | 8 steps | 8 steps | ✅ Match |
| Max gap (BUILD) | Not specified | 6 steps | Implementation detail |
| Max gap (PEAK) | Not specified | 4 steps | Implementation detail |

### 17.8 Visual Guide Accuracy

The visual guide (`duopulse-v4-visual-guide.md`) is generally accurate with these notes:

| Section | Accuracy | Notes |
|---------|----------|-------|
| I/O mapping | ✅ Accurate | Hardware pins match |
| Control layout | ✅ Accurate | Knob domains correct |
| Pattern Field grid | ✅ Accurate | 3×3 with same naming |
| Energy Zones | ✅ Accurate | Thresholds match |
| PUNCH effect | ✅ Accurate | Velocity floor/boost ranges |
| BUILD effect | ✅ Accurate | Phrase dynamics |
| Guard Rails | ✅ Accurate | Rules match |
| LED brightness | ⚠️ Simplified | Fill zone uses triple-pulse, not simple "pulsing" |

---

## 18. Analysis: Bugs, Complexity, Usability & Stability

### 18.1 Potential Bugs

#### 18.1.1 Static State in Persistence Module

**Location**: `src/Engine/Persistence.cpp:20`, `src/Engine/Persistence.cpp:253`
```cpp
static bool s_crc32Initialized = false;
static bool s_flashHasData = false;
```

**Risk**: Module-level static state can cause issues if the module is re-initialized or if there are multiple instances. The CRC32 table initialization is guarded but `s_flashHasData` could get out of sync.

**Severity**: Low - unlikely in single-module firmware

#### 18.1.2 Static Initialization in PatternField

**Location**: `src/Engine/PatternField.cpp:17`
```cpp
static bool s_initialized = false;
```

**Risk**: If `GetBlendedArchetype()` is called before initialization, behavior is undefined. The guard exists but relies on caller discipline.

**Severity**: Low - initialization happens early in boot sequence

#### 18.1.3 First-Step Edge Case

**Location**: `src/Engine/Sequencer.cpp:105-116`
```cpp
if (!isFirstStep) {
    AdvanceStep();
} else {
    state_.sequencer.totalSteps = 1;
}
```

**Risk**: The first step handling sets `totalSteps = 1` without calling `AdvanceStep()`, which could skip phrase/bar boundary checks on the very first bar.

**Severity**: Medium - could cause first-bar pattern to differ from expected

#### 18.1.4 Trigger Delay Underflow

**Location**: `src/Engine/Sequencer.cpp:142-143`
```cpp
triggerDelayRemaining_[v]--;
if (triggerDelayRemaining_[v] <= 0)
```

**Risk**: If timing offset is negative (which can happen with certain swing calculations), the trigger fires immediately but `triggerDelayRemaining_` continues decrementing into negative territory.

**Severity**: Low - triggers still fire, just with slightly different timing

#### 18.1.5 Division Without Zero Check

**Location**: Multiple locations in `src/Engine/BrokenEffects.cpp`
```cpp
float t = (broken - 0.25f) * 4.0f;    // line 27
float t = (broken - 0.4f) / 0.3f;     // line 66
float fillProgress = (phraseProgress - 0.75f) * 4.0f;  // line 418
```

**Risk**: While these divisions use literal constants (safe), the pattern of computing normalized progress without bounds checking could cause issues if inputs are out of expected range.

**Severity**: Low - inputs are clamped upstream

### 18.2 Complexity Points

#### 18.2.1 16 Virtual Soft Knobs

**Location**: `src/main.cpp`, `src/Engine/SoftKnob.h`

**Complexity**: 4 physical knobs × 4 modes (Perf/Config × Primary/Shift) = 16 virtual knobs with independent state, lock/unlock logic, and pickup thresholds.

**Issues**:
- Hard to debug which virtual knob is active
- Mode switching requires careful state management
- User can get "stuck" if physical position diverges significantly from virtual

**Recommendation**: Consider adding debug output to show knob state, or simplifying to immediate-takeover for config mode.

#### 18.2.2 Dual-Seed DRIFT System

**Location**: `src/Engine/DriftControl.h`, `src/Engine/SequencerState.h`

**Complexity**: Two seed sources (`patternSeed`, `phraseSeed`) with per-step stability thresholds determine which seed is used.

**Issues**:
- Non-obvious behavior: downbeats always locked, off-beats drift first
- Hard to predict which elements will change on reseed
- Phrase boundary logic duplicated in multiple places

**Recommendation**: Add visualization mode to show which steps are "locked" vs "drifting"

#### 18.2.3 Guard Rail State Machine

**Location**: `src/Engine/GuardRails.cpp`

**Complexity**: Two-phase repair (soft then hard) with multiple zone-dependent rules that can interact.

**Issues**:
- Order of rule application matters
- Soft repair can undo Gumbel selection decisions
- Genre-specific rules add another dimension

**Recommendation**: Add guard rail hit counters for debugging

#### 18.2.4 Trigger Delay Queue

**Location**: `src/Engine/Sequencer.cpp:137-164`

**Complexity**: Per-voice trigger delay with sample-accurate timing for swing/jitter effects.

**Issues**:
- Array indices via `static_cast<int>(Voice::XXX)` - fragile if enum changes
- Pending triggers survive bar boundaries (intentional but subtle)
- No cancel mechanism for pending triggers on reset

**Recommendation**: Consider dedicated `TriggerQueue` class with clear semantics

### 18.3 Usability Improvements

#### 18.3.1 No Visual Feedback for Mode/Shift State

**Current**: LED shows triggers and fills, but not which mode/shift combination is active.

**Improvement**: Brief LED flash pattern when switching modes (already partially implemented with `LedEventType::MODE_CHANGE`)

#### 18.3.2 Soft Knob "Hunting" Behavior

**Current**: When switching modes, knobs must "pick up" the stored value, which can feel unresponsive.

**Improvement**:
- Add visual feedback when knob is locked vs tracking
- Consider "immediate" mode for config parameters that don't need smooth transitions
- Show target value somehow during pickup phase

#### 18.3.3 No Tempo Display

**Current**: Internal tempo is set by knob but not displayed.

**Improvement**: LED blink rate could indicate approximate tempo, or add tempo to AUX EVENT mode

#### 18.3.4 Fill Activation Feedback

**Current**: Fill is queued for "next phrase" but no indication of when it will fire.

**Improvement**: LED could indicate "fill queued" state distinctly from "fill active"

#### 18.3.5 Reseed Confirmation

**Current**: Double-tap reseeds with a brief LED flash.

**Improvement**: Consider distinct flash pattern for reseed vs other mode changes

### 18.4 Stability Risks

#### 18.4.1 No Thread Safety Between Audio Callback and Main Loop

**Locations**:
- Audio callback: `src/main.cpp:194-248`
- Control processing: `src/main.cpp:300-522`

**Risk**: The Sequencer state is modified from both the audio ISR and main loop. While Daisy typically runs audio at highest priority (non-preemptible), the pattern is:
```cpp
// Audio callback reads state, main loop writes parameters
sequencer.SetEnergy(value);  // Called from main loop
sequencer.ProcessAudio();     // Called from ISR
```

**Mitigation**: Daisy's audio callback completes before control processing runs (blocking model), but this is implicit, not explicit.

**Recommendation**: Add comments clarifying the threading model, or use explicit double-buffering for control state.

#### 18.4.2 Flash Write During Playback

**Location**: `src/Engine/Persistence.cpp` called from `src/main.cpp`

**Risk**: Auto-save triggers QSPI flash erase/write which can cause brief delays. The 2-second debounce helps but flash operations could theoretically interfere with audio if they take too long.

**Mitigation**: Flash operations are in main loop, not ISR, so audio should continue.

**Recommendation**: Measure actual flash operation timing, consider async writes.

#### 18.4.3 Gumbel Sampler Numerical Stability

**Location**: `src/Engine/GumbelSampler.cpp:41-47`
```cpp
float UniformToGumbel(float uniform) {
    uniform = std::max(kEpsilon, std::min(1.0f - kEpsilon, uniform));
    return -std::log(-std::log(uniform));
}
```

**Risk**: `log(-log(x))` can produce extreme values near x=0 or x=1. While clamping is applied, edge cases with very small weights could still produce problematic scores.

**Mitigation**: Weights are clamped to `kEpsilon` minimum, scores are bounded by `kMinScore`.

**Recommendation**: Add unit tests for edge cases (all-zero weights, single-step eligible, etc.)

#### 18.4.4 External Clock Timeout

**Location**: `src/Engine/Sequencer.cpp:493-496`
```cpp
void Sequencer::TriggerExternalClock() {
    externalClockTimeout_ = static_cast<int>(sampleRate_ * 2.0f);  // 2 second timeout
    usingExternalClock_ = true;
}
```

**Risk**: 2-second timeout is long; if external clock stops mid-phrase, there's a noticeable delay before internal clock takes over.

**Recommendation**: Reduce timeout to ~500ms, or make configurable

#### 18.4.5 Pattern Length Mismatch

**Location**: Multiple files assume `kMaxSteps = 32`

**Risk**: If pattern length is set to 64 (allowed by spec), array accesses could overflow:
- `timingOffsets[32]` in `SequencerState`
- Bitmasks are `uint32_t` (32 bits max)
- Archetype weights are `[32]` arrays

**Mitigation**: Implementation actually limits to 32 steps despite spec allowing 64.

**Recommendation**: Either enforce 32-step limit in config or extend all arrays/masks to 64.

### 18.5 Performance Concerns

#### 18.5.1 Per-Sample Processing in ProcessAudio

**Location**: `src/Engine/Sequencer.cpp:60-180`

**Observations**:
- No `expf`, `logf`, `sqrtf` in per-sample path (good)
- Gumbel/log operations only at bar boundaries (good)
- `std::array` return value may involve copy (minor)

**Recommendation**: Profile on hardware; consider returning by reference for velocity pair

#### 18.5.2 Archetype Blending on Every Bar

**Location**: `src/Engine/Sequencer.cpp:122`
```cpp
if (state_.sequencer.isBarBoundary || isFirstStep) {
    BlendArchetype();  // Called every bar
    ...
}
```

**Observation**: Archetype blending involves 4-way softmax and 32×3 weight interpolations. At fast tempos (300 BPM), this is ~20 times/second.

**Recommendation**: Cache blend results when FIELD X/Y haven't changed

### 18.6 Test Coverage Gaps

| Area | Current | Needed |
|------|---------|--------|
| Gumbel edge cases | Unknown | All-zero weights, single eligible step |
| Guard rail interactions | Unknown | Downbeat rescue + max gap conflict |
| DRIFT boundary | Unknown | Exactly DRIFT=0.5 with step at stability=0.5 |
| Fill zone transition | Unknown | Phrase boundary during fill |
| External clock recovery | Unknown | Clock stop/resume mid-phrase |
| Flash persistence | Unknown | Corrupt data, version mismatch |

---

*Generated from DuoPulse v4 implementation source code.*
