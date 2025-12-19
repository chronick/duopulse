#pragma once

#include <array>

#include "daisysp.h"
#include "PhrasePosition.h"
#include "config.h"

namespace daisysp_idm_grids
{

/**
 * DuoPulse Sequencer - Stub for v4 Migration
 * 
 * This is a minimal stub that maintains the public interface expected by main.cpp
 * while the v4 architecture is being implemented.
 * 
 * v4 Implementation will replace this entirely in Phase 9.
 */
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
    void SetRatchet(float value);        // K4+Shift: Fill intensity (0-1)

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
    float GetRatchet() const { return ratchet_; }
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

    // === DuoPulse Parameters ===
    
    // Performance Primary
    float anchorDensity_  = 0.5f;
    float shimmerDensity_ = 0.5f;
    float broken_         = 0.0f;
    float drift_          = 0.0f;

    // Performance Shift
    float fuse_   = 0.5f;
    int   loopLengthBars_ = 4;
    float couple_ = 0.5f;
    float ratchet_ = 0.0f;

    // Config Primary
    float anchorAccent_  = 0.5f;
    float shimmerAccent_ = 0.5f;
    float contour_       = 0.0f;

    // Config Shift
    float swingTaste_ = 0.5f;
    float gateTime_   = 0.2f;
    float humanize_   = 0.0f;
    float clockDiv_   = 0.5f;

    // Swing state
    float currentSwing_ = 0.5f;

    // Phrase position tracking
    PhrasePosition phrasePos_;

    int gateTimers_[2] = {0, 0};
    int gateDurationSamples_ = 0;
    int clockTimer_ = 0;
    int clockDurationSamples_ = 0;
    int accentHoldSamples_ = 0;
    int hihatHoldSamples_ = 0;
    int accentTimer_ = 0;
    int hihatTimer_ = 0;
    
    // Output Levels (Velocity)
    float outputLevels_[2] = {0.0f, 0.0f};

    // Current pattern index
    int currentPatternIndex_ = 0;

    // External Clock Logic
    bool usingExternalClock_ = false;
    int  externalClockTimeout_ = 0;
    bool mustTick_ = false;

    daisysp::Metro metro_;
    bool forceNextTriggers_ = false;
    bool forcedTriggers_[3] = {false, false, false};
    bool forcedKickAccent_ = false;

    void TriggerGate(int channel);
    void TriggerClock();
    void ProcessGates();
    int  HoldMsToSamples(float milliseconds) const;
    int  GetClockDivisionFactor() const;
};

} // namespace daisysp_idm_grids
