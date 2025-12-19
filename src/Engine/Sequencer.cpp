#include "Sequencer.h"
#include "config.h"

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

    metro_.Init(2.0f, sampleRate_);
    SetBpm(currentBpm_);

    gateDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    gateTimers_[0] = 0;
    gateTimers_[1] = 0;
    clockDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    clockTimer_ = 0;
    accentTimer_ = 0;
    hihatTimer_ = 0;
    accentHoldSamples_ = gateDurationSamples_;
    hihatHoldSamples_  = gateDurationSamples_;
    
    usingExternalClock_ = false;
    externalClockTimeout_ = 0;
    mustTick_ = false;
    
    // Initialize parameters
    anchorDensity_  = 0.5f;
    shimmerDensity_ = 0.5f;
    broken_         = 0.0f;
    drift_          = 0.0f;
    fuse_           = 0.5f;
    loopLengthBars_ = 4;
    couple_         = 0.5f;
    ratchet_        = 0.0f;
    anchorAccent_   = 0.5f;
    shimmerAccent_  = 0.5f;
    contour_        = 0.0f;
    swingTaste_     = 0.5f;
    gateTime_       = 0.2f;
    humanize_       = 0.0f;
    clockDiv_       = 0.5f;

    currentSwing_ = 0.5f;

    // Initialize phrase position
    phrasePos_ = CalculatePhrasePosition(0, loopLengthBars_);

    // Initialize pattern system
    currentPatternIndex_ = 0;
}

// === Parameter Setters ===

void Sequencer::SetAnchorDensity(float value)
{
    anchorDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetShimmerDensity(float value)
{
    shimmerDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetBroken(float value)
{
    broken_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetDrift(float value)
{
    drift_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFuse(float value)
{
    fuse_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetLength(int bars)
{
    if(bars < 1)
        bars = 1;
    if(bars > 16)
        bars = 16;
    loopLengthBars_ = bars;
}

void Sequencer::SetCouple(float value)
{
    couple_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetRatchet(float value)
{
    ratchet_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetAnchorAccent(float value)
{
    anchorAccent_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetShimmerAccent(float value)
{
    shimmerAccent_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetContour(float value)
{
    contour_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetSwingTaste(float value)
{
    swingTaste_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFlux(float value)
{
    SetBroken(value);
}

void Sequencer::SetOrbit(float value)
{
    SetCouple(value);
}

void Sequencer::SetTerrain(float value)
{
    // Deprecated: no-op
    (void)value;
}

void Sequencer::SetGrid(float value)
{
    // Deprecated: no-op
    (void)value;
}

void Sequencer::SetGateTime(float value)
{
    gateTime_ = Clamp(value, 0.0f, 1.0f);
    float gateMs         = kMinGateMs + (gateTime_ * (kMaxGateMs - kMinGateMs));
    gateDurationSamples_ = static_cast<int>(sampleRate_ * gateMs / 1000.0f);
    if(gateDurationSamples_ < 1)
        gateDurationSamples_ = 1;
}

void Sequencer::SetHumanize(float value)
{
    humanize_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetClockDiv(float value)
{
    clockDiv_ = Clamp(value, 0.0f, 1.0f);
}

int Sequencer::GetClockDivisionFactor() const
{
    if(clockDiv_ < 0.2f)
        return 4;
    if(clockDiv_ < 0.4f)
        return 2;
    if(clockDiv_ < 0.6f)
        return 1;
    if(clockDiv_ < 0.8f)
        return -2;
    return -4;
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
    stepIndex_ = -1;
    metro_.Reset();
}

void Sequencer::TriggerExternalClock()
{
    externalClockTimeout_ = static_cast<int>(sampleRate_ * 2.0f);
    usingExternalClock_ = true;
    mustTick_ = true;
}

std::array<float, 2> Sequencer::ProcessAudio()
{
    uint8_t tick = 0;

    if(usingExternalClock_)
    {
        if(mustTick_)
        {
            tick = 1;
            mustTick_ = false;
        }
        
        externalClockTimeout_--;
        if(externalClockTimeout_ <= 0)
        {
            usingExternalClock_ = false;
            metro_.Reset();
        }
    }
    else
    {
        tick = metro_.Process();
    }

    if(tick)
    {
        // Handle Loop Length
        int effectiveLoopSteps = loopLengthBars_ * 16;
        if(effectiveLoopSteps > 32)
            effectiveLoopSteps = 32;

        stepIndex_ = (stepIndex_ + 1) % effectiveLoopSteps;

        // Update phrase position tracking
        phrasePos_ = CalculatePhrasePosition(stepIndex_, loopLengthBars_);

        // Stub: Fire anchor on downbeats, shimmer on backbeats
        bool gate0 = (stepIndex_ % 4 == 0);  // Simple 4-on-floor
        bool gate1 = (stepIndex_ % 8 == 4);  // Simple backbeat

        if(forceNextTriggers_)
        {
            gate0 = forcedTriggers_[0];
            gate1 = forcedTriggers_[1];
            forceNextTriggers_ = false;
            forcedKickAccent_ = false;
        }

        TriggerClock();

        if(gate0)
        {
            TriggerGate(0);
            accentTimer_     = accentHoldSamples_;
            outputLevels_[0] = 0.8f;
        }

        if(gate1)
        {
            TriggerGate(1);
            hihatTimer_      = hihatHoldSamples_;
            outputLevels_[1] = 0.8f;
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
    int divFactor = GetClockDivisionFactor();

    if(divFactor >= 1)
    {
        clockTimer_ = clockDurationSamples_;
    }
    else
    {
        clockTimer_ = clockDurationSamples_;
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
