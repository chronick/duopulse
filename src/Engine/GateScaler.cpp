#include "GateScaler.h"

namespace daisysp_idm_grids
{

void GateScaler::Init(float sampleRate)
{
    sampleRate_     = sampleRate;
    triggerSamples_ = static_cast<int>(kDefaultTriggerMs * sampleRate / 1000.0f);
    if (triggerSamples_ < 1)
    {
        triggerSamples_ = 1;
    }
}

void GateScaler::SetTargetVoltage(float volts)
{
    targetVoltage_ = ClampVoltage(volts);
}

void GateScaler::SetTriggerDuration(float ms)
{
    triggerSamples_ = static_cast<int>(ms * sampleRate_ / 1000.0f);
    if (triggerSamples_ < 1)
    {
        triggerSamples_ = 1;
    }
}

float GateScaler::Render(float gateState) const
{
    const float gated = daisysp::fclamp(gateState, 0.0f, 1.0f) * targetVoltage_;
    return VoltageToCodecSample(gated);
}

float GateScaler::ProcessTriggerOutput(const TriggerState& trigger) const
{
    // Convert trigger high state to gate value
    float gateState = trigger.high ? 1.0f : 0.0f;
    return Render(gateState);
}

float GateScaler::ClampVoltage(float volts)
{
    if (volts > kGateVoltageLimit)
    {
        return kGateVoltageLimit;
    }
    if (volts < -kGateVoltageLimit)
    {
        return -kGateVoltageLimit;
    }
    return volts;
}

float GateScaler::VoltageToCodecSample(float volts)
{
    float clamped    = ClampVoltage(volts);
    float normalized = clamped / kCodecMaxVoltage;
    return -normalized; // Codec polarity is inverted (positive float -> negative voltage)
}

} // namespace daisysp_idm_grids


