#pragma once

#include <array>

#include "daisysp.h"
#include "PatternGenerator.h"
#include "ChaosModulator.h"

namespace daisysp_idm_grids
{

class Sequencer
{
public:
    Sequencer() = default;
    ~Sequencer() = default;

    void Init(float sampleRate);
    
    // Parameter Setters
    void SetLowDensity(float value);
    void SetHighDensity(float value);
    void SetLowVariation(float value);
    void SetHighVariation(float value);
    void SetStyle(float value);
    void SetLength(int bars);
    void SetEmphasis(float value);
    void SetTempoControl(float value); // 0.0 - 1.0

    // System Triggers
    void TriggerTapTempo(uint32_t nowMs);
    void TriggerReset();

    std::array<float, 2> ProcessAudio();
    void ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent = false);

    bool IsGateHigh(int channel) const;
    bool IsClockHigh() const { return clockTimer_ > 0; }

    float GetBpm() const { return currentBpm_; }
    void  SetBpm(float bpm);
    void  SetAccentHoldMs(float milliseconds);
    void  SetHihatHoldMs(float milliseconds);

private:
    static constexpr float kMinTempo = 30.0f;
    static constexpr float kMaxTempo = 200.0f;

    float    sampleRate_ = 48000.0f;
    float    currentBpm_ = 120.0f;
    float    lastTempoControl_ = -1.0f;
    uint32_t lastTapTime_ = 0;

    int   stepIndex_ = 0;
    
    // Parameters
    float lowDensity_ = 0.5f;
    float highDensity_ = 0.5f;
    float lowVariation_ = 0.0f;
    float highVariation_ = 0.0f;
    float style_ = 0.0f;
    int   loopLengthBars_ = 4;
    float emphasis_ = 0.5f;

    // Internal
    float mapX_ = 0.0f; // Derived from Style
    float mapY_ = 0.0f; // Derived from Style/Fixed
    // chaosAmount_ removed, using independent modulators

    int gateTimers_[2] = {0, 0}; // Kick, Snare
    int gateDurationSamples_ = 0;
    int clockTimer_ = 0;
    int clockDurationSamples_ = 0;

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

    void TriggerGate(int channel);
    void TriggerClock();
    void ProcessGates();
    int  ComputeAccentThreshold(float density) const;
    int  HoldMsToSamples(float milliseconds) const;
};

} // namespace daisysp_idm_grids
