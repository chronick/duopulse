#pragma once

#include "daisysp.h"

namespace daisysp_idm_grids
{

class GateScaler
{
  public:
    static constexpr float kCodecMaxVoltage = 9.0f;
    static constexpr float kGateVoltageLimit = 5.0f;

    GateScaler() = default;

    void  SetTargetVoltage(float volts);
    float GetTargetVoltage() const { return targetVoltage_; }

    float Render(float gateState) const;

    static float ClampVoltage(float volts);
    static float VoltageToCodecSample(float volts);

  private:
    float targetVoltage_ = kGateVoltageLimit;
};

} // namespace daisysp_idm_grids


