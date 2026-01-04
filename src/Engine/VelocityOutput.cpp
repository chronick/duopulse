#include "VelocityOutput.h"
#include <cmath>

namespace daisysp_idm_grids
{

void VelocityOutput::Init(float sampleRate)
{
    sampleRate_ = sampleRate;
}

void VelocityOutput::TriggerVelocity(VelocityOutputState& state, float velocity)
{
    // Clamp velocity to valid range
    if (velocity < 0.0f)
    {
        velocity = 0.0f;
    }
    if (velocity > 1.0f)
    {
        velocity = 1.0f;
    }

    // Sample and hold: latch the new velocity value
    state.targetVoltage = velocity;
    state.heldVoltage   = velocity;
    state.triggered     = true;
}

float VelocityOutput::ProcessVelocityOutput(const VelocityOutputState& state) const
{
    // Convert held voltage (0-1) to output voltage (0-5V)
    float voltage = VelocityToVoltage(state.heldVoltage);

    // Convert to codec sample (with polarity inversion)
    return GateScaler::VoltageToCodecSample(voltage);
}

void VelocityOutput::ProcessVelocityOutputs(const OutputState& output,
                                            float& anchorSample,
                                            float& shimmerSample) const
{
    anchorSample  = ProcessVelocityOutput(output.anchorVelocity);
    shimmerSample = ProcessVelocityOutput(output.shimmerVelocity);
}

float VelocityOutput::ApplyVelocityCurve(float linearVelocity, float curveAmount)
{
    // Clamp inputs
    if (linearVelocity < 0.0f)
    {
        linearVelocity = 0.0f;
    }
    if (linearVelocity > 1.0f)
    {
        linearVelocity = 1.0f;
    }
    if (curveAmount < 0.0f)
    {
        curveAmount = 0.0f;
    }
    if (curveAmount > 1.0f)
    {
        curveAmount = 1.0f;
    }

    // Linear when curveAmount = 0
    // Exponential when curveAmount = 1
    // Blend between the two
    float exponential = linearVelocity * linearVelocity;
    return linearVelocity + curveAmount * (exponential - linearVelocity);
}

float VelocityOutput::VelocityToVoltage(float velocity)
{
    // Clamp velocity
    if (velocity < 0.0f)
    {
        velocity = 0.0f;
    }
    if (velocity > 1.0f)
    {
        velocity = 1.0f;
    }

    // Scale to voltage range
    return kMinVelocityVoltage + velocity * (kMaxVelocityVoltage - kMinVelocityVoltage);
}

} // namespace daisysp_idm_grids
