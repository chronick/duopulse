#pragma once

#include <array>

#include "daisysp.h"
#include "PatternSkeleton.h"
#include "PatternData.h"
#include "ChaosModulator.h"
#include "GenreConfig.h"
#include "config.h"

#ifdef USE_PULSE_FIELD_V3
#include "PulseField.h"
#include "BrokenEffects.h"
#endif

namespace daisysp_idm_grids
{

class Sequencer
{
public:
    Sequencer() = default;
    ~Sequencer() = default;

    void Init(float sampleRate);

    // === DuoPulse v3 Parameter Setters ===
    // All setters apply immediately (no queuing)

    // Performance Mode Primary
    void SetAnchorDensity(float value);  // K1: Anchor hit frequency (0-1)
    void SetShimmerDensity(float value); // K2: Shimmer hit frequency (0-1)
    void SetBroken(float value);         // K3: Pattern regularity (0=straight, 1=IDM chaos)
    void SetDrift(float value);          // K4: Pattern evolution (0=locked, 1=generative)

    // Performance Mode Shift
    void SetFuse(float value);           // K1+Shift: Cross-lane energy tilt (0-1, center=balanced)
    void SetLength(int bars);            // K2+Shift: Loop length (1,2,4,8,16 bars)
    void SetCouple(float value);         // K3+Shift: Voice interlock strength (0-1)
    // K4+Shift: Reserved

    // Config Mode Primary
    void SetAnchorAccent(float value);   // K1: Anchor accent intensity (0-1)
    void SetShimmerAccent(float value);  // K2: Shimmer accent intensity (0-1)
    void SetContour(float value);        // K3: CV output shape (0-1: Vel/Decay/Pitch/Random)
    void SetTempoControl(float value);   // K4: BPM control (0-1 maps to 90-160)

    // Config Mode Shift
    void SetSwingTaste(float value);     // K1+Shift: Fine-tune swing within BROKEN's range (0-1)
    void SetGateTime(float value);       // K2+Shift: Trigger duration (0-1 maps to 5-50ms)
    void SetHumanize(float value);       // K3+Shift: Extra jitter on top of BROKEN's (0-1)
    void SetClockDiv(float value);       // K4+Shift: Clock output div/mult (0-1)

    // === Deprecated v2 Setters (for backward compatibility) ===
    void SetFlux(float value);           // Deprecated: use SetBroken
    void SetOrbit(float value);          // Deprecated: use SetCouple
    void SetTerrain(float value);        // Deprecated: genre emerges from BROKEN
    void SetGrid(float value);           // Deprecated: no pattern selection in v3

    // System Triggers
    void TriggerTapTempo(uint32_t nowMs);
    void TriggerReset();
    void TriggerExternalClock(); // Call on rising edge of external clock

    std::array<float, 2> ProcessAudio();
    void ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent = false);

    bool IsGateHigh(int channel) const;
    bool IsClockHigh() const { return clockTimer_ > 0; }

    float GetBpm() const { return currentBpm_; }
    float GetSwingPercent() const { return currentSwing_; }
    float GetBroken() const { return broken_; }
    float GetDrift() const { return drift_; }
    const PhrasePosition& GetPhrasePosition() const { return phrasePos_; }
    void  SetBpm(float bpm);
    void  SetAccentHoldMs(float milliseconds);
    void  SetHihatHoldMs(float milliseconds);

    int  GetCurrentPatternIndex() const { return currentPatternIndex_; }

private:
    // Tempo range per spec: 90-160 BPM
    static constexpr float kMinTempo = 90.0f;
    static constexpr float kMaxTempo = 160.0f;

    // Gate time range: 5-50ms
    static constexpr float kMinGateMs = 5.0f;
    static constexpr float kMaxGateMs = 50.0f;

    float    sampleRate_ = 48000.0f;
    float    currentBpm_ = 120.0f;
    float    lastTempoControl_ = -1.0f;
    uint32_t lastTapTime_ = 0;

    int stepIndex_ = 0;

    // === DuoPulse v3 Parameters ===
    
    // Performance Primary
    float anchorDensity_  = 0.5f; // K1: Hit frequency for anchor
    float shimmerDensity_ = 0.5f; // K2: Hit frequency for shimmer
    float broken_         = 0.0f; // K3: Pattern regularity (0=straight, 1=chaos)
    float drift_          = 0.0f; // K4: Pattern evolution (0=locked, 1=generative)

    // Performance Shift
    float fuse_   = 0.5f; // K1+Shift: Cross-lane energy tilt
    int   loopLengthBars_ = 4;    // K2+Shift: Loop length
    float couple_ = 0.5f; // K3+Shift: Voice interlock strength
    // K4+Shift: Reserved

    // Config Primary
    float anchorAccent_  = 0.5f; // K1: Accent intensity for anchor
    float shimmerAccent_ = 0.5f; // K2: Accent intensity for shimmer
    float contour_       = 0.0f; // K3: CV output shape
    // K4: Tempo (handled by currentBpm_)

    // Config Shift
    float swingTaste_ = 0.5f; // K1+Shift: Swing fine-tune
    float gateTime_   = 0.2f; // K2+Shift: Gate duration (normalized 0-1)
    float humanize_   = 0.0f; // K3+Shift: Micro-timing jitter
    float clockDiv_   = 0.5f; // K4+Shift: Clock division

    // Deprecated v2 parameters (kept for compatibility, mapped to v3)
    float flux_    = 0.0f; // Deprecated: mapped to broken_
    float orbit_   = 0.5f; // Deprecated: mapped to couple_
    float terrain_ = 0.0f; // Deprecated: genre from BROKEN
    float grid_    = 0.0f; // Deprecated: no pattern selection

    // Swing state
    float currentSwing_       = 0.5f;  // Current effective swing (0.5 = straight)
    int   swingDelaySamples_  = 0;     // Delay to apply to off-beat steps
    int   swingDelayCounter_  = 0;     // Countdown for delayed triggers
    bool  pendingAnchorTrig_  = false; // Anchor trigger waiting for swing delay
    bool  pendingShimmerTrig_ = false; // Shimmer trigger waiting for swing delay
    float pendingAnchorVel_   = 0.0f;  // Velocity for pending anchor
    float pendingShimmerVel_  = 0.0f;  // Velocity for pending shimmer
    bool  pendingClockTrig_   = false; // Clock trigger waiting for swing delay
    int   stepDurationSamples_ = 0;    // Duration of one 16th note in samples

    // Orbit/Shadow state (for Shadow mode: shimmer echoes anchor with 1-step delay)
    bool  lastAnchorTrig_ = false;     // Whether anchor triggered on previous step
    float lastAnchorVel_  = 0.0f;      // Velocity of previous anchor trigger

    // Humanize state
    uint32_t humanizeRngState_ = 0x12345678; // Simple RNG for humanize jitter

    // Phrase position tracking
    PhrasePosition phrasePos_;

    // Contour CV state (for decay/hold between triggers)
    float anchorContourCV_  = 0.0f;
    float shimmerContourCV_ = 0.0f;

    int gateTimers_[2] = {0, 0}; // Anchor, Shimmer
    int gateDurationSamples_ = 0;
    int clockTimer_ = 0;
    int clockDurationSamples_ = 0;

    // Clock Division State
    int clockDivCounter_ = 0;  // Counter for clock division

    // External Clock Logic
    bool usingExternalClock_ = false;
    int  externalClockTimeout_ = 0;
    bool mustTick_ = false;

    daisysp::Metro   metro_;
    ChaosModulator   chaosLow_;
    ChaosModulator   chaosHigh_;
    bool             forceNextTriggers_ = false;
    bool             forcedTriggers_[3] = {false, false, false};
    bool             forcedKickAccent_ = false;
    int              accentTimer_ = 0;
    int              hihatTimer_ = 0;
    int              accentHoldSamples_ = 0;
    int              hihatHoldSamples_ = 0;
    
    // Output Levels (Velocity)
    float            outputLevels_[2] = {0.0f, 0.0f};

    // Current pattern index (0-15)
    int  currentPatternIndex_ = 0;

    void  TriggerGate(int channel);
    void  TriggerClock();
    void  ProcessGates();
    void  ProcessSwingDelay();
    void  UpdateSwingParameters();
    int   HoldMsToSamples(float milliseconds) const;
    float NextHumanizeRandom(); // Returns 0-1 for humanize jitter
    int   GetClockDivisionFactor() const; // Returns 1, 2, 4 for division or -2, -4 for multiplication

    // PatternSkeleton-based trigger generation (v2)
    // Returns true if triggered, sets velocity output
    void GetSkeletonTriggers(int step, float anchorDens, float shimmerDens,
                             bool& anchorTrig, bool& shimmerTrig,
                             float& anchorVel, float& shimmerVel);

#ifdef USE_PULSE_FIELD_V3
    // === DuoPulse v3: Pulse Field State ===
    PulseFieldState pulseFieldState_;
    
    // PulseField-based trigger generation (v3)
    // Uses weighted pulse field algorithm with BROKEN/DRIFT controls
    void GetPulseFieldTriggers(int step, float anchorDens, float shimmerDens,
                               bool& anchorTrig, bool& shimmerTrig,
                               float& anchorVel, float& shimmerVel);
    
    // Apply BROKEN effects stack (swing from broken is handled separately)
    void ApplyBrokenEffects(int& step, float& anchorVel, float& shimmerVel,
                            bool anchorTrig, bool shimmerTrig);
#endif
};

} // namespace daisysp_idm_grids
