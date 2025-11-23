#pragma once

#include <random>

namespace daisysp_idm_grids
{

class ChaosModulator
{
public:
    struct Sample
    {
        float jitterX = 0.0f;
        float jitterY = 0.0f;
        float densityBias = 0.0f;
        bool  ghostTrigger = false;
    };

    void Init(uint32_t seed = 0x4b1d2f3c);
    void SetAmount(float amount);
    Sample NextSample();

private:
    float             amount_ = 0.0f;
    std::minstd_rand  rng_;

    float NextUniform();
    float NextSigned();
};

} // namespace daisysp_idm_grids


