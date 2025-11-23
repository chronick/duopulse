#include "ChaosModulator.h"

#include <algorithm>

#include "ControlUtils.h"

namespace daisysp_idm_grids
{

namespace
{
constexpr float kMaxJitter = 0.2f;
constexpr float kMaxDensityBias = 0.35f;
constexpr float kGhostProbabilityScale = 0.3f;
} // namespace

void ChaosModulator::Init(uint32_t seed)
{
    rng_.seed(seed);
}

void ChaosModulator::SetAmount(float amount)
{
    amount_ = Clamp01(amount);
}

ChaosModulator::Sample ChaosModulator::NextSample()
{
    Sample sample;
    if(amount_ <= 0.0f)
    {
        return sample;
    }

    const float jitterRange = kMaxJitter * amount_;
    sample.jitterX = jitterRange * NextSigned();
    sample.jitterY = jitterRange * NextSigned();

    const float densityRange = kMaxDensityBias * amount_;
    sample.densityBias = densityRange * NextSigned();

    const float ghostProbability = kGhostProbabilityScale * amount_;
    sample.ghostTrigger = NextUniform() < ghostProbability;

    return sample;
}

float ChaosModulator::NextUniform()
{
    const float maxValue = static_cast<float>(rng_.max());
    return static_cast<float>(rng_()) / (maxValue + 1.0f);
}

float ChaosModulator::NextSigned()
{
    return (NextUniform() * 2.0f) - 1.0f;
}

} // namespace daisysp_idm_grids


