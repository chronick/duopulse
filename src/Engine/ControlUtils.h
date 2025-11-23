#pragma once

#include <algorithm>

namespace daisysp_idm_grids
{

inline float Clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

inline float MixControl(float knobValue, float cvValue)
{
    return Clamp01(knobValue + cvValue);
}

} // namespace daisysp_idm_grids


