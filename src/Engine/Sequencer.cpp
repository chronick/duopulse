#include "Sequencer.h"

#include <algorithm>
#include <cmath>

namespace daisysp_idm_grids
{

namespace
{
template <typename T>
inline T Clamp(T value, T minValue, T maxValue)
{
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}
} // namespace

void Sequencer::Init(float sampleRate)
{
    sampleRate_ = sampleRate;

    patternGen_.Init();

    metro_.Init(2.0f, sampleRate_);
    SetBpm(currentBpm_);

    chaos_.Init();
    chaos_.SetAmount(0.0f);

    gateDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    gateTimers_[0] = 0;
    gateTimers_[1] = 0;
    clockDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    clockTimer_ = 0;
    accentTimer_ = 0;
    hihatTimer_ = 0;
}

void Sequencer::ProcessControl(float    tempoControl,
                               float    knobX,
                               float    knobY,
                               float    chaosAmount,
                               bool     tapButtonTrigger,
                               uint32_t nowMs)
{
    mapX_ = Clamp(knobX, 0.0f, 1.0f);
    mapY_ = Clamp(knobY, 0.0f, 1.0f);
    chaosAmount_ = Clamp(chaosAmount, 0.0f, 1.0f);
    chaos_.SetAmount(chaosAmount_);

    tempoControl = Clamp(tempoControl, 0.0f, 1.0f);
    if(std::abs(tempoControl - lastTempoControl_) > 0.01f)
    {
        float newBpm = kMinTempo + (tempoControl * (kMaxTempo - kMinTempo));
        SetBpm(newBpm);
        lastTempoControl_ = tempoControl;
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

std::array<float, 2> Sequencer::ProcessAudio()
{
    uint8_t tick = metro_.Process();

    if(tick)
    {
        stepIndex_ = (stepIndex_ + 1) % PatternGenerator::kPatternLength;

        bool trigs[3] = {false, false, false};
        bool kickAccent = false;
        const auto chaosSample = chaos_.NextSample();

        if(forceNextTriggers_)
        {
            for(int i = 0; i < 3; ++i)
            {
                trigs[i] = forcedTriggers_[i];
            }
            forceNextTriggers_ = false;
            kickAccent = forcedKickAccent_ && trigs[0];
            forcedKickAccent_ = false;
        }
        else
        {
            float jitteredX = Clamp(mapX_ + chaosSample.jitterX, 0.0f, 1.0f);
            float jitteredY = Clamp(mapY_ + chaosSample.jitterY, 0.0f, 1.0f);
            float baseDensity = 0.6f + (chaosAmount_ * 0.2f);
            float density = Clamp(baseDensity + chaosSample.densityBias, 0.05f, 0.95f);
            patternGen_.GetTriggers(jitteredX, jitteredY, stepIndex_, density, trigs);
            if(trigs[0])
            {
                const uint8_t kickLevel = patternGen_.GetLevel(jitteredX, jitteredY, 0, stepIndex_);
                kickAccent = kickLevel >= ComputeAccentThreshold(density);
            }
        }

        TriggerClock();
        const bool kickTrig = trigs[0];
        const bool snareTrig = trigs[1];
        const bool hihatTrig = trigs[2];
        const bool percTrigBase = snareTrig || hihatTrig;
        bool       percTrig = percTrigBase;
        if(kickTrig)
        {
            TriggerGate(0);
            if(kickAccent)
            {
                accentTimer_ = gateDurationSamples_;
            }
            else
            {
                accentTimer_ = 0;
            }
        }
        if(!percTrig && chaosSample.ghostTrigger)
        {
            percTrig = true;
        }
        if(percTrig)
        {
            TriggerGate(1);
        }
        if(hihatTrig)
        {
            hihatTimer_ = gateDurationSamples_;
        }
        else if(snareTrig)
        {
            hihatTimer_ = 0;
        }
    }

    ProcessGates();

    float accentCv = accentTimer_ > 0 ? 1.0f : 0.0f;
    float hihatCv = hihatTimer_ > 0 ? 1.0f : 0.0f;
    return {accentCv, hihatCv};
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
    currentBpm_ = Clamp(bpm, kMinTempo, kMaxTempo);
    metro_.SetFreq(currentBpm_ / 60.0f * 4.0f);
}

void Sequencer::TriggerGate(int channel)
{
    if(channel < 2)
    {
        gateTimers_[channel] = gateDurationSamples_;
    }
}

void Sequencer::TriggerClock()
{
    clockTimer_ = clockDurationSamples_;
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
    if(clockTimer_ > 0)
    {
        --clockTimer_;
    }
    if(accentTimer_ > 0)
    {
        --accentTimer_;
    }
    if(hihatTimer_ > 0)
    {
        --hihatTimer_;
    }
}

int Sequencer::ComputeAccentThreshold(float density) const
{
    constexpr int kAccentOffset = 64;
    int base = static_cast<int>(std::round((1.0f - density) * 255.0f));
    base = Clamp(base, 0, 255);
    int accented = base + kAccentOffset;
    return accented > 255 ? 255 : accented;
}

void Sequencer::ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent)
{
    forcedTriggers_[0] = kick;
    forcedTriggers_[1] = snare;
    forcedTriggers_[2] = hh;
    forceNextTriggers_ = true;
    forcedKickAccent_ = kickAccent;
}

} // namespace daisysp_idm_grids


