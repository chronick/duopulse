#pragma once

namespace daisysp_idm_grids
{

struct LedIndicator
{
    static constexpr float kLedOnVoltage  = 5.0f;
    static constexpr float kLedOffVoltage = 0.0f;

    static constexpr float VoltageForState(bool isOn)
    {
        return isOn ? kLedOnVoltage : kLedOffVoltage;
    }
};

} // namespace daisysp_idm_grids


