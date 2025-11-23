#pragma once

#include "daisysp.h"
#include "PatternGenerator.h"

namespace daisysp_idm_grids
{

class Sequencer
{
public:
    Sequencer() = default;
    ~Sequencer() = default;

    void Init(float sampleRate);
    void ProcessControl(float knobTempo,
                        float knobX,
                        float knobY,
                        bool tapButtonTrigger,
                        uint32_t nowMs);
    float ProcessAudio();

    bool IsGateHigh(int channel) const;

    float GetBpm() const { return currentBpm_; }
    void  SetBpm(float bpm);

private:
    static constexpr float kMinTempo = 30.0f;
    static constexpr float kMaxTempo = 200.0f;

    float    sampleRate_ = 48000.0f;
    float    currentBpm_ = 120.0f;
    float    lastKnobVal_ = -1.0f;
    uint32_t lastTapTime_ = 0;

    int   stepIndex_ = 0;
    float mapX_ = 0.0f;
    float mapY_ = 0.0f;
    [[maybe_unused]] float density_ = 0.5f; // Placeholder for future density control

    int gateTimers_[2] = {0, 0}; // Kick, Snare
    int gateDurationSamples_ = 0;

    daisysp::Metro        metro_;
    PatternGenerator      patternGen_;
    daisysp::WhiteNoise   hhNoise_;
    daisysp::Adsr         hhEnv_;

    void TriggerGate(int channel);
    void ProcessGates();
};

} // namespace daisysp_idm_grids
