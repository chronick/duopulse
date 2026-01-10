# DuoPulse Implementation Roadmap

## Overview

This document outlines the implementation plan for DuoPulse, building on the existing codebase. The current implementation provides a solid foundation with `Sequencer`, `PatternGenerator`, `SoftKnob`, and `GateScaler` classes. We'll evolve this architecture rather than rewrite from scratch.

---

## Current Codebase Summary

### Existing Components

| File | Purpose | Reuse Strategy |
|------|---------|----------------|
| `main.cpp` | Control routing, audio callback | **Modify** - Add shift layer, fill trigger |
| `Sequencer.h/cpp` | Core timing engine | **Extend** - Add swing, orbit, fill |
| `PatternGenerator.h/cpp` | Pattern lookup | **Replace** - New hybrid algorithm |
| `SoftKnob.h/cpp` | Gradual pickup | **Enhance** - Improve interpolation |
| `GateScaler.h/cpp` | Gate voltage scaling | **Keep** - Works well |
| `ChaosModulator.h/cpp` | Variation injection | **Enhance** - Integrate with FLUX |
| `GridsData.h/cpp` | Pattern lookup tables | **Adapt** - Partial reuse |
| `ControlUtils.h` | CV mixing | **Keep** - Works well |
| `LedIndicator.h` | LED voltage levels | **Extend** - Add fill flash |

### Current Control State

```cpp
struct ControlState {
    // Base Mode Parameters
    float lowDensity    = 0.5f;
    float highDensity   = 0.5f;
    float lowVariation  = 0.0f;
    float highVariation = 0.0f;

    // Config Mode Parameters
    float style    = 0.0f;
    float length   = 0.5f;
    float emphasis = 0.5f;
    float tempo    = 0.5f;

    bool configMode = false;
};
```

---

## Phase 1: Control System Overhaul

**Goal**: Implement shift layer, rename parameters, add new controls.

### 1.1 Update ControlState

**File**: `src/main.cpp`

**New Structure**:

```cpp
struct ControlState {
    // Performance Mode - Primary
    float anchorDensity  = 0.5f;
    float shimmerDensity = 0.5f;
    float flux           = 0.0f;
    float fuse           = 0.5f;

    // Performance Mode - Shift
    float anchorAccent   = 0.5f;
    float shimmerAccent  = 0.5f;
    float orbit          = 0.5f;  // 0-33% interlock, 33-67% free, 67-100% shadow
    float contour        = 0.0f;  // CV mode

    // Config Mode - Primary
    float terrain        = 0.0f;  // Genre: techno/tribal/triphop/idm
    float length         = 0.5f;  // Loop bars
    float grid           = 0.0f;  // Pattern select
    float tempo          = 0.5f;

    // Config Mode - Shift
    float swingTaste     = 0.5f;  // Fine-tune within genre range
    float gateTime       = 0.5f;  // 5-50ms
    float humanize       = 0.0f;  // Micro-timing jitter (0-10ms)
    float clockDiv       = 0.5f;  // ÷4 to ×4

    // Mode State
    bool configMode      = false;
    bool shiftActive     = false;
    bool fillActive      = false;
};
```

**Effort**: 1-2 hours

### 1.2 Implement Shift Layer Detection

**File**: `src/main.cpp`

**Changes**:
- Track button hold duration to distinguish tap (fill) from hold (shift)
- Add shift state tracking
- Manage 4 separate SoftKnob sets (perf primary, perf shift, config primary, config shift)

```cpp
// Button handling
constexpr uint32_t kShiftThresholdMs = 150;  // Hold > 150ms = shift
uint32_t buttonDownTime = 0;
bool buttonWasPressed = false;

void ProcessButton() {
    bool pressed = tapButton.Pressed();
    
    if (pressed && !buttonWasPressed) {
        buttonDownTime = System::GetNow();
    }
    
    if (!pressed && buttonWasPressed) {
        uint32_t holdDuration = System::GetNow() - buttonDownTime;
        if (holdDuration < kShiftThresholdMs) {
            // Short tap = fill trigger
            TriggerFill();
        }
        controlState.shiftActive = false;
    }
    
    if (pressed && (System::GetNow() - buttonDownTime > kShiftThresholdMs)) {
        controlState.shiftActive = true;
    }
    
    buttonWasPressed = pressed;
}
```

**Effort**: 2-3 hours

### 1.3 Update CV Routing

**Critical**: CV always modulates performance parameters.

```cpp
// In ProcessControls(), always apply CV to performance params:
const float cv1 = patch.GetAdcValue(CV_5);
const float cv2 = patch.GetAdcValue(CV_6);
const float cv3 = patch.GetAdcValue(CV_7);
const float cv4 = patch.GetAdcValue(CV_8);

// These are ALWAYS applied, regardless of mode
float finalAnchorDens  = MixControl(controlState.anchorDensity, cv1);
float finalShimmerDens = MixControl(controlState.shimmerDensity, cv2);
float finalFlux        = MixControl(controlState.flux, cv3);
float finalFuse        = MixControl(controlState.fuse, cv4);

sequencer.SetAnchorDensity(finalAnchorDens);
sequencer.SetShimmerDensity(finalShimmerDens);
sequencer.SetFlux(finalFlux);
sequencer.SetFuse(finalFuse);

// Then process mode-specific knob assignments...
```

**Effort**: 1 hour

---

## Phase 2: Sequencer Engine Updates

**Goal**: Add swing, orbit, fill, and genre-aware behavior.

### 2.1 New Sequencer Interface

**File**: `src/Engine/Sequencer.h`

**New Methods**:

```cpp
class Sequencer {
public:
    // All setters apply IMMEDIATELY - no waiting for pattern boundary
    
    // Renamed/New Performance Parameters
    void SetAnchorDensity(float value);
    void SetShimmerDensity(float value);
    void SetFlux(float value);
    void SetFuse(float value);
    
    void SetAnchorAccent(float value);
    void SetShimmerAccent(float value);
    void SetOrbit(float value);       // Voice relationship
    void SetContour(float value);     // CV mode
    
    // Renamed/New Config Parameters
    void SetTerrain(float value);     // Genre character
    void SetSwingTaste(float value);  // Fine-tune swing
    void SetGateTime(float value);    // Gate duration
    void SetHumanize(float value);    // Micro-timing jitter
    void SetClockDiv(float value);    // Clock output division
    void SetLength(int bars);         // Loop length (wraps immediately if needed)
    
    // Fill System
    void TriggerFill();
    bool IsFillActive() const;
    
    // Output Queries
    float GetAnchorCV() const;        // Velocity/expression CV
    float GetShimmerCV() const;
    bool  IsSwungClockHigh() const;   // Clock with swing applied

private:
    // New members
    float anchorDensity_  = 0.5f;
    float shimmerDensity_ = 0.5f;
    float flux_           = 0.0f;
    float fuse_           = 0.5f;
    
    float anchorAccent_   = 0.5f;
    float shimmerAccent_  = 0.5f;
    float orbit_          = 0.5f;
    float contour_        = 0.0f;
    
    float terrain_        = 0.0f;
    float swingTaste_     = 0.5f;
    float gateTime_       = 0.5f;
    float humanize_       = 0.0f;  // 0 = tight, 1 = ±10ms jitter
    float clockDiv_       = 0.5f;
    
    // Derived values
    float effectiveSwing_ = 0.5f;     // Genre + taste
    int   swungClockTimer_ = 0;
    
    // Fill state
    bool  fillActive_     = false;
    int   fillEndStep_    = 0;
    
    // CV output values
    float anchorCV_       = 0.0f;
    float shimmerCV_      = 0.0f;
};
```

**Effort**: 2-3 hours

### 2.2 Tempo Range Update

**File**: `src/Engine/Sequencer.h`

Update the tempo constants:

```cpp
// Current (to change):
static constexpr float kMinTempo = 30.0f;
static constexpr float kMaxTempo = 200.0f;

// New values:
static constexpr float kMinTempo = 90.0f;
static constexpr float kMaxTempo = 160.0f;
```

**Rationale**: 90-160 BPM covers the sweet spot for techno (125-135), tribal (120-130), trip-hop (80-95 half-time = ~160-190 double-time internally, but we handle this with pattern feel), and IDM (90-150+). This focused range provides finer control resolution.

**Effort**: 5 minutes

### 2.3 Immediate Parameter Application

**Critical Design Rule**: All parameters apply immediately, never waiting for pattern/bar boundaries.

```cpp
// Length change with immediate wrap
void Sequencer::SetLength(int bars) {
    loopLengthBars_ = bars;
    int maxStep = bars * 16;
    
    // Immediately wrap if we're past the new boundary
    if (stepIndex_ >= maxStep) {
        stepIndex_ = stepIndex_ % maxStep;
    }
}

// All other setters are simple immediate assignments:
void Sequencer::SetAnchorDensity(float value) {
    anchorDensity_ = value;  // Used on next step
}

void Sequencer::SetTerrain(float value) {
    terrain_ = value;
    UpdateEffectiveSwing();  // Immediately recalculate swing range
}
```

**Rationale**: Immediate changes provide responsive feel for both knob turns and CV modulation. The modular context expects sample-accurate parameter response.

**Effort**: Built into each setter (no additional effort)

### 2.4 Swing Implementation

**File**: `src/Engine/Sequencer.cpp`

**Genre-Aware Swing Calculation**:

```cpp
struct SwingRange {
    float min;
    float max;
};

// Swing ranges per genre (as percentages, 50 = straight)
constexpr SwingRange kSwingRanges[] = {
    {52.0f, 57.0f},  // Techno: minimal swing
    {56.0f, 62.0f},  // Tribal: moderate swing
    {60.0f, 68.0f},  // Trip-Hop: heavy swing
    {54.0f, 65.0f},  // IDM: varied + jitter
};

void Sequencer::UpdateEffectiveSwing() {
    // Determine genre from terrain (0-1 maps to 4 genres)
    int genreIndex = static_cast<int>(terrain_ * 3.99f);
    genreIndex = std::clamp(genreIndex, 0, 3);
    
    const SwingRange& range = kSwingRanges[genreIndex];
    
    // swingTaste (0-1) interpolates within genre range
    effectiveSwing_ = range.min + swingTaste_ * (range.max - range.min);
    
    // Convert to ratio (50% = 0.5 ratio = straight)
    swingRatio_ = effectiveSwing_ / 100.0f;
}

// Apply swing to step timing
int Sequencer::GetSwungStepSamples(int step) const {
    bool isOffBeat = (step % 2) == 1;  // Odd steps are off-beats
    
    if (!isOffBeat) {
        return step * stepDurationSamples_;
    }
    
    // Calculate swing delay
    float swingOffset = (swingRatio_ - 0.5f) * 2.0f;  // -1 to +1
    int delaySamples = static_cast<int>(swingOffset * stepDurationSamples_ * 0.5f);
    
    return step * stepDurationSamples_ + delaySamples;
}
```

**Effort**: 3-4 hours

### 2.5 Swung Clock Output

**File**: `src/Engine/Sequencer.cpp`

The clock output must respect swing timing so downstream modules stay in sync:

```cpp
void Sequencer::TriggerSwungClock() {
    // Clock fires on each step, but with swing-adjusted timing
    swungClockTimer_ = clockDurationSamples_;
}

void Sequencer::ProcessSwungClock() {
    if (swungClockTimer_ > 0) {
        swungClockTimer_--;
    }
}

bool Sequencer::IsSwungClockHigh() const {
    return swungClockTimer_ > 0;
}
```

**Main Loop Update**:

```cpp
// In ProcessControls()
patch.WriteCvOut(patch_sm::CV_OUT_1,
                 LedIndicator::VoltageForState(sequencer.IsSwungClockHigh()));
```

**Effort**: 1-2 hours

### 2.6 Orbit (Voice Relationship)

**File**: `src/Engine/Sequencer.cpp`

```cpp
enum class OrbitMode { INTERLOCK, FREE, SHADOW };

OrbitMode Sequencer::GetOrbitMode() const {
    if (orbit_ < 0.33f) return OrbitMode::INTERLOCK;
    if (orbit_ < 0.67f) return OrbitMode::FREE;
    return OrbitMode::SHADOW;
}

void Sequencer::ApplyOrbitLogic(bool& anchorTrig, bool& shimmerTrig, 
                                 float& anchorVel, float& shimmerVel) {
    switch (GetOrbitMode()) {
        case OrbitMode::INTERLOCK:
            // Shimmer fills gaps where Anchor is silent
            if (anchorTrig) {
                // Reduce shimmer probability when anchor fires
                shimmerTrig = shimmerTrig && (RandomFloat() > 0.7f);
            } else {
                // Boost shimmer probability when anchor silent
                if (!shimmerTrig && RandomFloat() < shimmerDensity_ * 0.3f) {
                    shimmerTrig = true;
                    shimmerVel = 0.4f + RandomFloat() * 0.3f;
                }
            }
            break;
            
        case OrbitMode::FREE:
            // No modification, independent patterns
            break;
            
        case OrbitMode::SHADOW:
            // Shimmer echoes Anchor with 1-step delay
            shimmerTrig = lastAnchorTrig_;
            shimmerVel = lastAnchorVel_ * 0.7f;  // Slightly quieter echo
            break;
    }
}
```

**Effort**: 2-3 hours

### 2.7 Humanize (Micro-Timing Jitter)

**File**: `src/Engine/Sequencer.cpp`

Humanize adds random micro-timing offsets to trigger timing, creating a more organic feel.

```cpp
void Sequencer::SetHumanize(float value) {
    humanize_ = value;
    // Max jitter = ±10ms at 48kHz = ±480 samples
    maxJitterSamples_ = static_cast<int>(value * 480.0f);
}

int Sequencer::GetHumanizedTriggerOffset() const {
    if (humanize_ < 0.01f) return 0;  // Skip if minimal
    
    // Random offset between -maxJitter and +maxJitter
    int jitter = (RandomInt() % (maxJitterSamples_ * 2 + 1)) - maxJitterSamples_;
    return jitter;
}

// In ProcessAudio, apply jitter to trigger timing:
void Sequencer::ProcessStep() {
    int triggerOffset = GetHumanizedTriggerOffset();
    // Offset is applied to when the gate fires within the step
    // Positive = late, Negative = early
    stepTriggerOffset_ = triggerOffset;
}
```

**Genre Interaction**: IDM terrain (75-100%) automatically adds extra humanize on top of the knob setting, creating the characteristic "broken" timing.

```cpp
float Sequencer::GetEffectiveHumanize() const {
    float base = humanize_;
    
    // IDM adds automatic jitter
    if (terrain_ > 0.75f) {
        float idmBoost = (terrain_ - 0.75f) * 4.0f;  // 0-1 in IDM range
        base += idmBoost * 0.3f;  // Up to 30% extra
    }
    
    return std::min(base, 1.0f);
}
```

**Effort**: 1-2 hours

---

## Phase 3: Pattern Generator Rewrite

**Goal**: Hybrid system with skeleton tables + algorithmic variation.

### 3.1 New Pattern Structure

**File**: `src/Engine/PatternGenerator.h`

```cpp
#pragma once
#include <cstdint>

namespace daisysp_idm_grids {

struct PatternSkeleton {
    uint32_t anchor[1];    // 32 steps, 1 bit per step (skeleton only)
    uint32_t shimmer[1];
    uint8_t  accentMask;   // Which steps can accent
    uint8_t  genreAffinity;// 0=all, 1=techno, 2=tribal, 3=triphop, 4=idm
};

class PatternGenerator {
public:
    void Init();
    
    // Main generation method
    void Generate(
        int step,
        float anchorDensity,
        float shimmerDensity,
        float flux,
        float fuse,
        float terrain,
        float anchorAccent,
        float shimmerAccent,
        int patternIndex,
        bool& anchorTrig,
        bool& shimmerTrig,
        float& anchorVelocity,
        float& shimmerVelocity
    );
    
    static constexpr int kNumPatterns = 16;
    static constexpr int kPatternLength = 32;
    
private:
    float RandomFloat();
    uint32_t GetSkeletonBit(int pattern, int lane, int step) const;
    float GetIntensity(int pattern, int lane, int step, float density) const;
    
    uint32_t randomState_ = 12345;
};

} // namespace daisysp_idm_grids
```

### 3.2 Pattern Data

**File**: `src/Engine/PatternGenerator.cpp`

```cpp
// 16 skeleton patterns optimized for 2-voice output
// Each pattern has anchor and shimmer skeletons
static const PatternSkeleton kPatterns[16] = {
    // Pattern 0: Four-on-floor techno
    {{0x11111111}, {0x00000000}, 0x11, 1},  // Anchor: every 4th, Shimmer: sparse
    
    // Pattern 1: Minimal techno
    {{0x10001000}, {0x00100010}, 0x11, 1},  // Sparse kick, offbeat shimmer
    
    // Pattern 2: Driving techno
    {{0x11111111}, {0x22222222}, 0x55, 1},  // Regular pulse both
    
    // Pattern 3: Tribal 1
    {{0x10100100}, {0x01010010}, 0x55, 2},  // Interlocking
    
    // Pattern 4: Tribal 2  
    {{0x10010010}, {0x00101001}, 0x33, 2},  // Call-response
    
    // Pattern 5: Breakbeat
    {{0x10001001}, {0x00100100}, 0x99, 3},  // Syncopated
    
    // Pattern 6: Trip-hop 1
    {{0x10000010}, {0x00010100}, 0x11, 3},  // Very sparse
    
    // Pattern 7: Trip-hop 2
    {{0x10001000}, {0x00000010}, 0x11, 3},  // Minimal
    
    // Pattern 8: IDM irregular
    {{0x10100001}, {0x01001010}, 0x99, 4},  // Broken
    
    // Pattern 9: IDM scatter
    {{0x10010100}, {0x00101001}, 0xAA, 4},  // Chaotic
    
    // Pattern 10-15: Additional variations...
    // (Full pattern set to be tuned by ear)
};

void PatternGenerator::Generate(
    int step,
    float anchorDensity,
    float shimmerDensity,
    float flux,
    float fuse,
    float terrain,
    float anchorAccent,
    float shimmerAccent,
    int patternIndex,
    bool& anchorTrig,
    bool& shimmerTrig,
    float& anchorVelocity,
    float& shimmerVelocity
) {
    patternIndex = std::clamp(patternIndex, 0, kNumPatterns - 1);
    
    // Get skeleton bits
    bool anchorSkel = GetSkeletonBit(patternIndex, 0, step);
    bool shimmerSkel = GetSkeletonBit(patternIndex, 1, step);
    
    // Apply fuse (energy tilt)
    float anchorFuseBoost = (1.0f - fuse) * 0.3f;    // CCW boosts anchor
    float shimmerFuseBoost = fuse * 0.3f;             // CW boosts shimmer
    
    float effectiveAnchorDens = anchorDensity + anchorFuseBoost;
    float effectiveShimmerDens = shimmerDensity + shimmerFuseBoost;
    
    // Calculate trigger probability
    float anchorProb = anchorSkel ? effectiveAnchorDens : effectiveAnchorDens * 0.2f * flux;
    float shimmerProb = shimmerSkel ? effectiveShimmerDens : effectiveShimmerDens * 0.2f * flux;
    
    // Random triggers based on probability
    anchorTrig = RandomFloat() < anchorProb;
    shimmerTrig = RandomFloat() < shimmerProb;
    
    // Velocity calculation
    if (anchorTrig) {
        bool isAccent = (kPatterns[patternIndex].accentMask & (1 << (step % 8))) != 0;
        float baseVel = 0.5f + RandomFloat() * 0.3f;
        anchorVelocity = isAccent ? baseVel + anchorAccent * 0.4f : baseVel - (1.0f - anchorAccent) * 0.2f;
        anchorVelocity = std::clamp(anchorVelocity, 0.1f, 1.0f);
    } else {
        anchorVelocity = 0.0f;
    }
    
    if (shimmerTrig) {
        bool isAccent = (kPatterns[patternIndex].accentMask & (1 << ((step + 4) % 8))) != 0;
        float baseVel = 0.4f + RandomFloat() * 0.4f;
        shimmerVelocity = isAccent ? baseVel + shimmerAccent * 0.4f : baseVel - (1.0f - shimmerAccent) * 0.2f;
        shimmerVelocity = std::clamp(shimmerVelocity, 0.1f, 1.0f);
    } else {
        shimmerVelocity = 0.0f;
    }
    
    // IDM terrain adds jitter to velocity
    if (terrain > 0.75f) {
        float jitter = (RandomFloat() - 0.5f) * flux * 0.3f;
        anchorVelocity = std::clamp(anchorVelocity + jitter, 0.0f, 1.0f);
        shimmerVelocity = std::clamp(shimmerVelocity + jitter, 0.0f, 1.0f);
    }
}
```

**Effort**: 6-8 hours

### 3.3 Phrase Structure Implementation

**File**: `src/Engine/Sequencer.h`, `src/Engine/PatternGenerator.cpp`

**New Struct**:

```cpp
struct PhrasePosition {
    int currentBar;        // 0 to (loopLengthBars - 1)
    int stepInBar;         // 0 to 15
    int stepInPhrase;      // 0 to (totalSteps - 1)
    float phraseProgress;  // 0.0 to 1.0
    bool isLastBar;
    bool isFillZone;
    bool isBuildZone;
    bool isDownbeat;
};

enum class PhraseModParam {
    FILL_PROBABILITY,
    GHOST_PROBABILITY,
    SYNCOPATION,
    ACCENT_INTENSITY,
    VELOCITY_VARIANCE
};
```

**New Methods**:

```cpp
void UpdatePhrasePosition();
float GetPhraseModulation(PhraseModParam param) const;
float GetGenrePhraseScale() const;
int GetFillZoneLength() const;
int GetBuildZoneStart() const;
```

**Implementation**:

```cpp
void Sequencer::UpdatePhrasePosition() {
    int totalSteps = loopLengthBars_ * 16;
    
    phrasePosition_.stepInPhrase = stepIndex_;
    phrasePosition_.currentBar = stepIndex_ / 16;
    phrasePosition_.stepInBar = stepIndex_ % 16;
    phrasePosition_.phraseProgress = static_cast<float>(stepIndex_) / totalSteps;
    phrasePosition_.isLastBar = (phrasePosition_.currentBar == loopLengthBars_ - 1);
    phrasePosition_.isFillZone = (totalSteps - stepIndex_) <= GetFillZoneLength();
    phrasePosition_.isBuildZone = (stepIndex_ >= GetBuildZoneStart());
    phrasePosition_.isDownbeat = (phrasePosition_.stepInBar == 0);
}

float Sequencer::GetPhraseModulation(PhraseModParam param) const {
    float progress = phrasePosition_.phraseProgress;
    float lastBarBoost = phrasePosition_.isLastBar ? 1.0f : 0.0f;
    float fillZoneBoost = phrasePosition_.isFillZone ? 1.0f : 0.0f;
    float buildZoneBoost = phrasePosition_.isBuildZone ? 1.0f : 0.0f;
    
    // Genre scales the phrase modulation intensity
    // Techno: subtle, Tribal: moderate, Trip-Hop: sparse, IDM: chaotic
    float genreScale = GetGenrePhraseScale();
    
    switch (param) {
        case PhraseModParam::FILL_PROBABILITY:
            return (lastBarBoost * 0.3f + fillZoneBoost * 0.5f) * genreScale;
            
        case PhraseModParam::GHOST_PROBABILITY:
            return progress * 0.3f * genreScale;
            
        case PhraseModParam::SYNCOPATION:
            return buildZoneBoost * (progress - 0.5f) * 0.4f * genreScale;
            
        case PhraseModParam::ACCENT_INTENSITY:
            if (phrasePosition_.stepInPhrase == 0) return 0.3f;
            if (phrasePosition_.isFillZone) return 0.2f * genreScale;
            return 0.0f;
            
        case PhraseModParam::VELOCITY_VARIANCE:
            return fillZoneBoost * 0.2f * genreScale;
    }
    return 0.0f;
}

float Sequencer::GetGenrePhraseScale() const {
    // terrain_: 0-25% techno, 25-50% tribal, 50-75% trip-hop, 75-100% IDM
    if (terrain_ < 0.25f) return 0.5f;       // Techno: subtle phrase modulation
    if (terrain_ < 0.50f) return 1.2f;       // Tribal: pronounced phrase structure
    if (terrain_ < 0.75f) return 0.7f;       // Trip-Hop: sparse, dropout-based
    return 1.5f;                              // IDM: extreme phrase modulation
}
```

**Integration with Pattern Generation**:

```cpp
void PatternGenerator::Generate(...) {
    // Get phrase modulation from sequencer
    float fillMod = sequencer.GetPhraseModulation(PhraseModParam::FILL_PROBABILITY);
    float ghostMod = sequencer.GetPhraseModulation(PhraseModParam::GHOST_PROBABILITY);
    float accentMod = sequencer.GetPhraseModulation(PhraseModParam::ACCENT_INTENSITY);
    
    // Apply to base probabilities
    float effectiveFlux = flux + ghostMod;
    float effectiveAccent = anchorAccent + accentMod;
    
    // Fill zone increases density
    if (sequencer.IsInFillZone()) {
        effectiveFlux = std::min(effectiveFlux + fillMod, 1.0f);
    }
    
    // Generate with modulated parameters...
}
```

**Effort**: 3-4 hours

---

## Phase 4: CV Expression (Contour)

**Goal**: Implement velocity, decay, pitch, and random CV modes.

### 4.1 Contour Implementation

**File**: `src/Engine/Sequencer.cpp`

```cpp
enum class ContourMode { VELOCITY, DECAY, PITCH, RANDOM };

ContourMode Sequencer::GetContourMode() const {
    if (contour_ < 0.25f) return ContourMode::VELOCITY;
    if (contour_ < 0.50f) return ContourMode::DECAY;
    if (contour_ < 0.75f) return ContourMode::PITCH;
    return ContourMode::RANDOM;
}

void Sequencer::UpdateCVOutputs(float anchorVel, float shimmerVel, 
                                  bool anchorTrig, bool shimmerTrig) {
    switch (GetContourMode()) {
        case ContourMode::VELOCITY:
            anchorCV_ = anchorTrig ? anchorVel : anchorCV_ * 0.95f;  // Slight hold
            shimmerCV_ = shimmerTrig ? shimmerVel : shimmerCV_ * 0.95f;
            break;
            
        case ContourMode::DECAY:
            // Higher velocity = longer decay hint (0-5V)
            if (anchorTrig) {
                anchorCV_ = anchorVel;  // Will decay in ProcessAudio
            }
            if (shimmerTrig) {
                shimmerCV_ = shimmerVel;
            }
            // Apply decay envelope
            anchorCV_ *= 0.998f;
            shimmerCV_ *= 0.996f;  // Shimmer decays faster
            break;
            
        case ContourMode::PITCH:
            // Random pitch offset scaled by velocity
            if (anchorTrig) {
                anchorCV_ = anchorVel * (0.3f + RandomFloat() * 0.7f);
            }
            if (shimmerTrig) {
                shimmerCV_ = shimmerVel * (0.2f + RandomFloat() * 0.8f);
            }
            break;
            
        case ContourMode::RANDOM:
            // Full S&H random
            if (anchorTrig) {
                anchorCV_ = RandomFloat();
            }
            if (shimmerTrig) {
                shimmerCV_ = RandomFloat();
            }
            break;
    }
}
```

**Effort**: 2-3 hours

---

## Phase 5: Soft Pickup Enhancement

**Goal**: Improve SoftKnob for gradual interpolation.

### 5.1 Enhanced SoftKnob

**File**: `src/Engine/SoftKnob.h`

```cpp
class SoftKnob {
public:
    void Init(float initialValue);
    float Process(float knobPosition);
    bool HasMoved() const;
    void SetValue(float value);  // For mode switches
    
private:
    static constexpr float kCatchupRate = 0.05f;  // 5% per control cycle
    static constexpr float kThreshold = 0.02f;    // Movement threshold
    
    float currentValue_ = 0.5f;
    float targetValue_ = 0.5f;
    float lastKnobPos_ = 0.5f;
    bool hasCaughtUp_ = true;
    bool hasMoved_ = false;
};
```

**File**: `src/Engine/SoftKnob.cpp`

```cpp
float SoftKnob::Process(float knobPosition) {
    hasMoved_ = false;
    
    // Check for significant knob movement
    float knobDelta = std::abs(knobPosition - lastKnobPos_);
    if (knobDelta > kThreshold) {
        hasMoved_ = true;
        lastKnobPos_ = knobPosition;
        targetValue_ = knobPosition;
        
        // Check if we've crossed the current value (catchup complete)
        if (!hasCaughtUp_) {
            float prevDelta = targetValue_ - currentValue_;
            float newDelta = knobPosition - currentValue_;
            if (prevDelta * newDelta <= 0) {
                hasCaughtUp_ = true;
            }
        }
    }
    
    if (hasCaughtUp_) {
        // Gradual interpolation toward knob position
        float delta = targetValue_ - currentValue_;
        currentValue_ += delta * kCatchupRate;
        
        // Snap when very close
        if (std::abs(delta) < 0.001f) {
            currentValue_ = targetValue_;
        }
    }
    // If not caught up, currentValue_ stays frozen
    
    return currentValue_;
}
```

**Effort**: 1-2 hours

---

## Phase 6: LED Feedback

**Goal**: Visual feedback for mode, triggers, parameters, and fills.

### 6.1 LED State Machine

**File**: `src/main.cpp`

```cpp
enum class LedMode { 
    PERFORMANCE_IDLE, 
    PERFORMANCE_TRIG,
    CONFIG,
    PARAMETER_SHOW,
    FILL_FLASH 
};

void UpdateLed() {
    float ledVoltage = 0.0f;
    uint32_t now = System::GetNow();
    
    // Priority 1: Fill active (rapid flash)
    if (sequencer.IsFillActive()) {
        bool flash = ((now / 50) % 2) == 0;  // 50ms flash rate
        ledVoltage = flash ? 5.0f : 0.0f;
    }
    // Priority 2: Recent parameter interaction
    else if (now - lastInteractionTime < 1000) {
        ledVoltage = activeParameterValue * 5.0f;
    }
    // Priority 3: Config mode (solid)
    else if (controlState.configMode) {
        ledVoltage = controlState.shiftActive ? 5.0f : 3.5f;
    }
    // Priority 4: Performance mode (trig pulse)
    else {
        bool trig = sequencer.IsGateHigh(0);  // Anchor trig
        ledVoltage = trig ? 5.0f : 0.5f;  // Dim baseline
        
        if (controlState.shiftActive) {
            ledVoltage = std::min(ledVoltage + 1.5f, 5.0f);  // Brighter when shift
        }
    }
    
    patch.WriteCvOut(patch_sm::CV_OUT_2, ledVoltage);
}
```

**Effort**: 1-2 hours

---

## Phase 7: Testing & Tuning

### 7.1 Unit Tests

**File**: `tests/test_sequencer.cpp`

New test cases:
- Swing calculation per genre
- Orbit mode behavior
- Fill trigger and duration
- Contour CV modes
- Soft pickup interpolation

**Effort**: 4-6 hours

### 7.2 Pattern Tuning

Listen and adjust:
- 16 skeleton patterns for musical feel
- Density curves for natural response
- Swing ranges per genre
- Accent intensity scaling

**Effort**: 4-6 hours (iterative)

### 7.3 Hardware Integration Testing

- Verify all control mappings
- Test CV input modulation
- Verify gate outputs and timing
- Test fill trigger via Gate In 2
- Verify swung clock output

**Effort**: 2-3 hours

---

## Implementation Order

### Recommended Sequence

1. **Phase 1.1-1.3**: Control system (foundation for everything)
2. **Phase 5**: Soft pickup enhancement (needed for mode switching)
3. **Phase 2.1**: Sequencer interface updates
4. **Phase 3**: Pattern generator rewrite (core functionality)
5. **Phase 2.2-2.3**: Swing + swung clock
6. **Phase 2.4**: Orbit modes
7. **Phase 2.5**: Fill system
8. **Phase 4**: CV expression modes
9. **Phase 6**: LED feedback
10. **Phase 7**: Testing and tuning

---

## Effort Summary

| Phase | Description | Hours |
|-------|-------------|-------|
| 1 | Control System | 4-6 |
| 2 | Sequencer Engine | 10-14 |
| 3 | Pattern Generator | 6-8 |
| 3.3 | Phrase Structure | 3-4 |
| 4 | CV Expression | 2-3 |
| 5 | Soft Pickup | 1-2 |
| 6 | LED Feedback | 1-2 |
| 7 | Testing/Tuning | 10-15 |
| **Total** | | **37-54 hours** |

---

## Success Criteria

1. ✅ Abstract terminology throughout (Anchor/Shimmer, Terrain, Orbit, etc.)
2. ✅ CV inputs always affect performance parameters
3. ✅ Genre-aware swing with taste fine-tuning
4. ✅ Fill trigger via button tap AND Gate In 2
5. ✅ Swung clock output
6. ✅ Gradual soft pickup on all knobs
7. ✅ Visual LED feedback for all states
8. ✅ Patterns optimized for 2-voice output
9. ✅ Musically appropriate for techno/tribal/trip-hop/IDM
10. ✅ All 16 controls accessible (4 modes × 4 knobs)

---

## Files Modified/Created

### Modified
- `src/main.cpp` - Control routing, shift layer, fill trigger
- `src/Engine/Sequencer.h` - New interface
- `src/Engine/Sequencer.cpp` - Swing, orbit, fill, CV expression
- `src/Engine/PatternGenerator.h` - New hybrid structure
- `src/Engine/PatternGenerator.cpp` - New generation algorithm
- `src/Engine/SoftKnob.h` - Enhanced interface
- `src/Engine/SoftKnob.cpp` - Gradual interpolation
- `tests/test_sequencer.cpp` - New test cases

### Potentially Created
- `src/Engine/PatternData.h` - Skeleton pattern tables (or inline)
- `src/Engine/GenreConfig.h` - Swing ranges and genre parameters

---

## Next Steps

1. Review and approve specification
2. Create feature branch: `feature/duopulse-v2`
3. Begin Phase 1 implementation
4. Regular testing and iteration
5. Documentation updates as features complete

