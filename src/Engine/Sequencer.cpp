#include "Sequencer.h"

#include <algorithm>
#include <cmath>

namespace daisysp_idm_grids
{

void Sequencer::Init(float sampleRate)
{
    sampleRate_ = sampleRate;

    patternGen_.Init();

    metro_.Init(2.0f, sampleRate_);
    SetBpm(currentBpm_);

    hhNoise_.Init();
    hhEnv_.Init(sampleRate_);
    hhEnv_.SetTime(daisysp::ADSR_SEG_ATTACK, 0.001f);
    hhEnv_.SetTime(daisysp::ADSR_SEG_DECAY, 0.05f);
    hhEnv_.SetTime(daisysp::ADSR_SEG_RELEASE, 0.01f);
    hhEnv_.SetSustainLevel(0.0f);

    gateDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    gateTimers_[0] = 0;
    gateTimers_[1] = 0;
}

void Sequencer::ProcessControl(float    knobTempo,
                               float    knobX,
                               float    knobY,
                               bool     tapButtonTrigger,
                               uint32_t nowMs)
{
    mapX_ = knobX;
    mapY_ = knobY;

    if(std::abs(knobTempo - lastKnobVal_) > 0.01f)
    {
        float newBpm = kMinTempo + (knobTempo * (kMaxTempo - kMinTempo));
        SetBpm(newBpm);
        lastKnobVal_ = knobTempo;
    }

    if(tapButtonTrigger)
    {
        if(lastTapTime_ != 0)
        {
            uint32_t interval = nowMs - lastTapTime_;
            if(interval > 100 && interval < 2000)
            {
                float newBpm = 60000.0f / static_cast<float>(interval);
                SetBpm(newBpm);
            }
        }
        lastTapTime_ = nowMs;
    }
}

float Sequencer::ProcessAudio()
{
    uint8_t tick = metro_.Process();

    if(tick)
    {
        stepIndex_ = (stepIndex_ + 1) % PatternGenerator::kPatternLength;

        bool trigs[3];
        patternGen_.GetTriggers(mapX_, mapY_, stepIndex_, 0.6f, trigs);

        if(trigs[0])
        {
            TriggerGate(0);
        }
        if(trigs[1])
        {
            TriggerGate(1);
        }
        if(trigs[2])
        {
            hhEnv_.Retrigger(false);
        }
    }

    ProcessGates();

    float env = hhEnv_.Process(false);
    float noise = hhNoise_.Process();
    return noise * env * 0.5f;
}

bool Sequencer::IsGateHigh(int channel) const
{
    if(channel >= 0 && channel < 2)
    {
        return gateTimers_[channel] > 0;
    }
    return false;
}

void Sequencer::SetBpm(float bpm)
{
    currentBpm_ = std::clamp(bpm, kMinTempo, kMaxTempo);
    metro_.SetFreq(currentBpm_ / 60.0f * 4.0f);
}

void Sequencer::TriggerGate(int channel)
{
    if(channel < 2)
    {
        gateTimers_[channel] = gateDurationSamples_;
    }
}

void Sequencer::ProcessGates()
{
    for(int& timer : gateTimers_)
    {
        if(timer > 0)
        {
            --timer;
        }
    }
}

} // namespace daisysp_idm_grids


