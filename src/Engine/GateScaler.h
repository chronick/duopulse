#pragma once

#include "daisysp.h"
#include "OutputState.h"

namespace daisysp_idm_grids
{

/**
 * GateScaler: Converts trigger/gate states to codec samples
 *
 * Handles voltage scaling and polarity inversion for the Daisy codec.
 * The codec outputs ±9V, but we typically want 0-5V triggers.
 *
 * Reference: docs/specs/main.md section 8.1
 */
class GateScaler
{
  public:
    /// Maximum voltage the codec can output (±9V)
    static constexpr float kCodecMaxVoltage = 9.0f;

    /// Standard gate/trigger voltage for Eurorack (5V)
    static constexpr float kGateVoltageLimit = 5.0f;

    /// Default trigger duration in milliseconds
    static constexpr float kDefaultTriggerMs = 1.0f;

    GateScaler() = default;

    /**
     * Initialize the gate scaler
     *
     * @param sampleRate Audio sample rate in Hz
     */
    void Init(float sampleRate);

    /**
     * Set target output voltage
     *
     * @param volts Target voltage (will be clamped to ±5V)
     */
    void SetTargetVoltage(float volts);

    /**
     * Get current target voltage
     */
    float GetTargetVoltage() const { return targetVoltage_; }

    /**
     * Set trigger pulse duration
     *
     * @param ms Duration in milliseconds
     */
    void SetTriggerDuration(float ms);

    /**
     * Render a gate/trigger state to codec sample
     *
     * @param gateState Gate state (0.0 = off, 1.0 = on)
     * @return Codec sample value (with polarity inversion)
     */
    float Render(float gateState) const;

    /**
     * Process a TriggerState and return codec sample
     *
     * Convenience method that reads the trigger state and returns
     * the appropriate codec sample value.
     *
     * @param trigger Trigger state to process
     * @return Codec sample value
     */
    float ProcessTriggerOutput(const TriggerState& trigger) const;

    /**
     * Clamp voltage to safe range
     *
     * @param volts Input voltage
     * @return Clamped voltage (±5V)
     */
    static float ClampVoltage(float volts);

    /**
     * Convert voltage to codec sample (with polarity inversion)
     *
     * The Daisy codec has inverted polarity: positive float values
     * produce negative voltages. This function handles the conversion.
     *
     * @param volts Target voltage
     * @return Codec sample value
     */
    static float VoltageToCodecSample(float volts);

  private:
    float targetVoltage_ = kGateVoltageLimit;
    float sampleRate_    = 48000.0f;
    int   triggerSamples_ = 48;  // 1ms at 48kHz
};

} // namespace daisysp_idm_grids


