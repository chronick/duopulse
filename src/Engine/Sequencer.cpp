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

    chaosLow_.Init(0x4b1d2f3c);
    chaosHigh_.Init(0xd3f2a1b9); // Different seed
    chaosLow_.SetAmount(0.0f);
    chaosHigh_.SetAmount(0.0f);

    gateDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    gateTimers_[0] = 0;
    gateTimers_[1] = 0;
    clockDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    clockTimer_ = 0;
    accentTimer_ = 0;
    hihatTimer_ = 0;
    accentHoldSamples_ = gateDurationSamples_;
    hihatHoldSamples_  = gateDurationSamples_;
    
    // Initialize parameters
    lowDensity_ = 0.5f;
    highDensity_ = 0.5f;
    lowVariation_ = 0.0f;
    highVariation_ = 0.0f;
    style_ = 0.0f;
    loopLengthBars_ = 4;
    emphasis_ = 0.5f;
}

void Sequencer::SetLowDensity(float value)
{
    lowDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetHighDensity(float value)
{
    highDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetLowVariation(float value)
{
    lowVariation_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetHighVariation(float value)
{
    highVariation_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetStyle(float value)
{
    style_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetLength(int bars)
{
    // Restrict bars to typical powers of 2 or simple integers
    if (bars < 1) bars = 1;
    if (bars > 16) bars = 16;
    loopLengthBars_ = bars;
}

void Sequencer::SetEmphasis(float value)
{
    emphasis_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetTempoControl(float value)
{
    float tempoControl = Clamp(value, 0.0f, 1.0f);
    if(std::abs(tempoControl - lastTempoControl_) > 0.01f)
    {
        float newBpm = kMinTempo + (tempoControl * (kMaxTempo - kMinTempo));
        SetBpm(newBpm);
        lastTempoControl_ = tempoControl;
    }
}

void Sequencer::TriggerTapTempo(uint32_t nowMs)
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

void Sequencer::TriggerReset()
{
    stepIndex_ = -1; // Next tick will be 0
    metro_.Reset();
}

std::array<float, 2> Sequencer::ProcessAudio()
{
    uint8_t tick = metro_.Process();

    if(tick)
    {
        // Handle Loop Length
        int effectiveLoopSteps = loopLengthBars_ * 16;
        if (effectiveLoopSteps > PatternGenerator::kPatternLength) 
            effectiveLoopSteps = PatternGenerator::kPatternLength;
            
        stepIndex_ = (stepIndex_ + 1) % effectiveLoopSteps;

        // Apply variation to independent modulators
        chaosLow_.SetAmount(lowVariation_);
        chaosHigh_.SetAmount(highVariation_);
        
        bool trigs[3] = {false, false, false};
        
        const auto chaosSampleLow = chaosLow_.NextSample();
        const auto chaosSampleHigh = chaosHigh_.NextSample();

        bool kickTrig = false;
        bool snareTrig = false;
        bool hhTrig = false;
        
        float kickVel = 0.0f;
        float snareVel = 0.0f;
        float hhVel = 0.0f;

        if(forceNextTriggers_)
        {
            kickTrig = forcedTriggers_[0];
            snareTrig = forcedTriggers_[1];
            hhTrig = forcedTriggers_[2];
            
            // For forced triggers, assume standard velocity or accent
            kickVel = forcedKickAccent_ ? 1.0f : 0.8f;
            snareVel = 0.8f;
            hhVel = 0.8f;

            forceNextTriggers_ = false;
            forcedKickAccent_ = false;
        }
        else
        {
            // Apply Chaos to Style (Map X).
            float avgJitterX = (chaosSampleLow.jitterX + chaosSampleHigh.jitterX) * 0.5f;
            float jitteredStyle = Clamp(style_ + avgJitterX, 0.0f, 1.0f);
            
            // Apply independent density bias
            float lowDensMod = Clamp(lowDensity_ + chaosSampleLow.densityBias, 0.05f, 0.95f);
            float highDensMod = Clamp(highDensity_ + chaosSampleHigh.densityBias, 0.05f, 0.95f);
            
            patternGen_.GetTriggers(jitteredStyle, stepIndex_, lowDensMod, highDensMod, trigs);
            
            kickTrig = trigs[0];
            snareTrig = trigs[1];
            hhTrig = trigs[2];

            // Get Levels for Velocity (Map Y fixed at 0.5)
            float mapY = 0.5f;
            kickVel = static_cast<float>(patternGen_.GetLevel(jitteredStyle, mapY, 0, stepIndex_)) / 255.0f;
            snareVel = static_cast<float>(patternGen_.GetLevel(jitteredStyle, mapY, 1, stepIndex_)) / 255.0f;
            hhVel = static_cast<float>(patternGen_.GetLevel(jitteredStyle, mapY, 2, stepIndex_)) / 255.0f;
        }

        // Apply Ghost Triggers to HH/Perc stream (High Variation)
        if(!hhTrig && chaosSampleHigh.ghostTrigger)
        {
            hhTrig = true;
            // Ghost triggers are usually quieter
            hhVel = 0.3f + (static_cast<float>(rand() % 100) / 200.0f); 
        }

        TriggerClock();
        
        // --- Routing Logic ---
        
        // Gate 0 (Low/Kick)
        bool gate0 = kickTrig;
        float vel0 = kickTrig ? kickVel : 0.0f;
        
        // Gate 1 (High/Snare)
        bool gate1 = snareTrig;
        float vel1 = snareTrig ? snareVel : 0.0f;
        
        // Route HH/Perc based on Emphasis
        if(hhTrig)
        {
            if(emphasis_ < 0.5f)
            {
                // Route to Low (add tom/perc flavor to kick channel)
                gate0 = true;
                vel0 = std::max(vel0, hhVel);
            }
            else
            {
                // Route to High (add hh/perc flavor to snare channel)
                gate1 = true;
                vel1 = std::max(vel1, hhVel);
            }
        }

        if(gate0)
        {
            TriggerGate(0);
            accentTimer_ = accentHoldSamples_;
            outputLevels_[0] = vel0;
        }
        
        if(gate1)
        {
            TriggerGate(1);
            hihatTimer_ = hihatHoldSamples_;
            outputLevels_[1] = vel1;
        }
    }

    ProcessGates();

    float out1 = accentTimer_ > 0 ? outputLevels_[0] : 0.0f;
    float out2 = hihatTimer_ > 0 ? outputLevels_[1] : 0.0f;
    return {out1, out2};
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

void Sequencer::ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent)
{
    forcedTriggers_[0] = kick;
    forcedTriggers_[1] = snare;
    forcedTriggers_[2] = hh;
    forceNextTriggers_ = true;
    forcedKickAccent_ = kickAccent;
}

void Sequencer::SetAccentHoldMs(float milliseconds)
{
    accentHoldSamples_ = HoldMsToSamples(milliseconds);
}

void Sequencer::SetHihatHoldMs(float milliseconds)
{
    hihatHoldSamples_ = HoldMsToSamples(milliseconds);
}

int Sequencer::HoldMsToSamples(float milliseconds) const
{
    const float clampedMs = milliseconds < 0.5f ? 0.5f : (milliseconds > 2000.0f ? 2000.0f : milliseconds);
    const float samples   = (clampedMs / 1000.0f) * sampleRate_;
    int         asInt     = static_cast<int>(samples);
    return asInt < 1 ? 1 : asInt;
}

} // namespace daisysp_idm_grids
