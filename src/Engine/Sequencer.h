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
    void ProcessControl(float tempoControl,
                        float mapX,
                        float mapY,
                        float chaosAmount,
                        bool tapButtonTrigger,
                        uint32_t nowMs);
    std::array<float, 2> ProcessAudio();
    void ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent = false);

    bool IsGateHigh(int channel) const;
    bool IsClockHigh() const { return clockTimer_ > 0; }

    float GetBpm() const { return currentBpm_; }
    void  SetBpm(float bpm);

private:
    static constexpr float kMinTempo = 30.0f;
    static constexpr float kMaxTempo = 200.0f;

    float    sampleRate_ = 48000.0f;
    float    currentBpm_ = 120.0f;
    float    lastTempoControl_ = -1.0f;
    uint32_t lastTapTime_ = 0;

    int   stepIndex_ = 0;
    float mapX_ = 0.0f;
    float mapY_ = 0.0f;
    float chaosAmount_ = 0.0f;

    int gateTimers_[2] = {0, 0}; // Kick, Snare
    int gateDurationSamples_ = 0;
    int clockTimer_ = 0;
    int clockDurationSamples_ = 0;

    daisysp::Metro   metro_;
    PatternGenerator patternGen_;
    ChaosModulator   chaos_;
    bool             forceNextTriggers_ = false;
    bool             forcedTriggers_[3] = {false, false, false};
    bool             forcedKickAccent_ = false;
    int              accentTimer_ = 0;
    int              hihatTimer_ = 0;

    void TriggerGate(int channel);
    void TriggerClock();
    void ProcessGates();
    int  ComputeAccentThreshold(float density) const;
};

} // namespace daisysp_idm_grids
