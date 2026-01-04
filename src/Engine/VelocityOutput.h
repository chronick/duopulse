#pragma once

#include "OutputState.h"
#include "GateScaler.h"

namespace daisysp_idm_grids
{

/**
 * VelocityOutput: Processor for velocity CV outputs (sample & hold)
 *
 * Velocity outputs use sample & hold behavior—the voltage is set on trigger
 * and held until the next trigger on that channel. This class handles the
 * processing and codec conversion for velocity CV outputs.
 *
 * Reference: docs/specs/main.md section 8.2
 *
 * Output Mapping:
 * - Audio Out L: Anchor velocity (0-5V sample & hold)
 * - Audio Out R: Shimmer velocity (0-5V sample & hold)
 */
class VelocityOutput
{
  public:
    /// Maximum output voltage for velocity
    static constexpr float kMaxVelocityVoltage = 5.0f;

    /// Minimum output voltage for velocity (can be non-zero for minimum gate)
    static constexpr float kMinVelocityVoltage = 0.0f;

    VelocityOutput() = default;

    /**
     * Initialize the velocity output processor
     *
     * @param sampleRate Audio sample rate in Hz
     */
    void Init(float sampleRate);

    /**
     * Process a velocity trigger for a channel
     *
     * When a trigger fires, the velocity value is sampled and held.
     *
     * @param state Velocity output state to update
     * @param velocity Velocity value (0.0-1.0)
     */
    static void TriggerVelocity(VelocityOutputState& state, float velocity);

    /**
     * Get the codec sample value for a velocity output
     *
     * Converts the held voltage to a codec sample with proper scaling.
     *
     * @param state Velocity output state
     * @return Codec sample value for audio output
     */
    float ProcessVelocityOutput(const VelocityOutputState& state) const;

    /**
     * Process both anchor and shimmer velocity outputs
     *
     * Convenience method that processes both velocity channels and
     * updates the output state.
     *
     * @param output Output state containing both velocity states
     * @param anchorSample Output: codec sample for anchor velocity
     * @param shimmerSample Output: codec sample for shimmer velocity
     */
    void ProcessVelocityOutputs(const OutputState& output,
                                float& anchorSample,
                                float& shimmerSample) const;

    /**
     * Apply velocity curve (optional)
     *
     * Transforms linear velocity to a more musical response curve.
     *
     * @param linearVelocity Input velocity (0.0-1.0)
     * @param curveAmount Curve amount (0.0 = linear, 1.0 = exponential)
     * @return Curved velocity value (0.0-1.0)
     */
    static float ApplyVelocityCurve(float linearVelocity, float curveAmount);

    /**
     * Convert velocity (0-1) to voltage (0-5V)
     *
     * @param velocity Velocity value (0.0-1.0)
     * @return Voltage (0-5V)
     */
    static float VelocityToVoltage(float velocity);

  private:
    float sampleRate_ = 48000.0f;
};

} // namespace daisysp_idm_grids
