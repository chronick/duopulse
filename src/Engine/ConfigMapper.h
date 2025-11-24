#pragma once

#include <algorithm>

#include "GateScaler.h"

namespace daisysp_idm_grids
{

struct ConfigMapper
{
    static constexpr float kMinGateHoldMs = 5.0f;
    static constexpr float kMaxGateHoldMs = 500.0f;

    static float ClampNormalized(float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    static float NormalizedToVoltage(float normalized)
    {
        const float clamped = ClampNormalized(normalized);
        const float span    = GateScaler::kGateVoltageLimit * 2.0f;
        return (clamped * span) - GateScaler::kGateVoltageLimit;
    }

    static float NormalizedToHoldMs(float normalized)
    {
        const float clamped = ClampNormalized(normalized);
        return kMinGateHoldMs
               + (kMaxGateHoldMs - kMinGateHoldMs) * clamped;
    }
};

} // namespace daisysp_idm_grids


