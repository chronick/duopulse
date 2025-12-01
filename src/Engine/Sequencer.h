#pragma once

#include <array>

#include "daisysp.h"
#include "PatternGenerator.h"
#include "ChaosModulator.h"
#include "GenreConfig.h"

namespace daisysp_idm_grids
{

class Sequencer
{
public:
    Sequencer() = default;
    ~Sequencer() = default;

    void Init(float sampleRate);

    // === DuoPulse v2 Parameter Setters ===
    // All setters apply immediately (no queuing)

    // Performance Mode Primary
    void SetAnchorDensity(float value);  // K1: Anchor hit frequency (0-1)
    void SetShimmerDensity(float value); // K2: Shimmer hit frequency (0-1)
    void SetFlux(float value);           // K3: Global variation/fills/ghosts (0-1)
    void SetFuse(float value);           // K4: Cross-lane energy tilt (0-1, center=balanced)

    // Performance Mode Shift
    void SetAnchorAccent(float value);   // K1+Shift: Anchor accent intensity (0-1)
    void SetShimmerAccent(float value);  // K2+Shift: Shimmer accent intensity (0-1)
    void SetOrbit(float value);          // K3+Shift: Voice relationship (0-1: Interlock/Free/Shadow)
    void SetContour(float value);        // K4+Shift: CV output shape (0-1: Vel/Decay/Pitch/Random)

    // Config Mode Primary
    void SetTerrain(float value);        // K1: Genre character (0-1: Techno/Tribal/TripHop/IDM)
    void SetLength(int bars);            // K2: Loop length (1,2,4,8,16 bars)
    void SetGrid(float value);           // K3: Pattern family selection (0-1 maps to 1-16)
    void SetTempoControl(float value);   // K4: BPM control (0-1 maps to 90-160)

    // Config Mode Shift
    void SetSwingTaste(float value);     // K1+Shift: Fine-tune swing within genre (0-1)
    void SetGateTime(float value);       // K2+Shift: Trigger duration (0-1 maps to 5-50ms)
    void SetHumanize(float value);       // K3+Shift: Micro-timing jitter (0-1)
    void SetClockDiv(float value);       // K4+Shift: Clock output div/mult (0-1)

    // Legacy interface (for backward compatibility)
    void SetLowDensity(float value)  { SetAnchorDensity(value); }
    void SetHighDensity(float value) { SetShimmerDensity(value); }
    void SetLowVariation(float value);   // Maps to flux
    void SetHighVariation(float value);  // Maps to flux
    void SetStyle(float value) { SetTerrain(value); }
    void SetEmphasis(float value) { SetGrid(value); }

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
    Genre GetCurrentGenre() const { return GetGenreFromTerrain(terrain_); }
    void  SetBpm(float bpm);
    void  SetAccentHoldMs(float milliseconds);
    void  SetHihatHoldMs(float milliseconds);

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

    // === DuoPulse v2 Parameters ===
    
    // Performance Primary
    float anchorDensity_  = 0.5f; // Hit frequency for anchor
    float shimmerDensity_ = 0.5f; // Hit frequency for shimmer
    float flux_           = 0.0f; // Variation/fills/ghosts
    float fuse_           = 0.5f; // Cross-lane energy tilt

    // Performance Shift
    float anchorAccent_  = 0.5f; // Accent intensity for anchor
    float shimmerAccent_ = 0.5f; // Accent intensity for shimmer
    float orbit_         = 0.5f; // Voice relationship mode
    float contour_       = 0.0f; // CV output shape

    // Config Primary
    float terrain_     = 0.0f; // Genre character
    int   loopLengthBars_ = 4; // Loop length
    float grid_        = 0.0f; // Pattern family

    // Config Shift
    float swingTaste_ = 0.5f; // Swing fine-tune
    float gateTime_   = 0.2f; // Gate duration (normalized 0-1)
    float humanize_   = 0.0f; // Micro-timing jitter
    float clockDiv_   = 0.5f; // Clock division

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

    // Legacy aliases (for internal use during transition)
    float& lowDensity_    = anchorDensity_;
    float& highDensity_   = shimmerDensity_;
    float& style_         = terrain_;
    float& emphasis_      = grid_;

    int gateTimers_[2] = {0, 0}; // Kick, Snare
    int gateDurationSamples_ = 0;
    int clockTimer_ = 0;
    int clockDurationSamples_ = 0;

    // External Clock Logic
    bool usingExternalClock_ = false;
    int  externalClockTimeout_ = 0;
    bool mustTick_ = false;

    daisysp::Metro   metro_;
    PatternGenerator patternGen_;
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

    void TriggerGate(int channel);
    void TriggerClock();
    void ProcessGates();
    void ProcessSwingDelay();
    void UpdateSwingParameters();
    int  HoldMsToSamples(float milliseconds) const;
};

} // namespace daisysp_idm_grids
