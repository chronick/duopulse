#pragma once

#include <algorithm>
#include <cmath>

namespace daisysp_idm_grids
{

// Forward declaration - full definition in ControlState.h
struct FillInputState;

// =============================================================================
// Basic Utility Functions
// =============================================================================

/**
 * Clamp a value to the 0-1 range
 */
inline float Clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

/**
 * Mix a knob value with CV modulation, clamped to 0-1
 */
inline float MixControl(float knobValue, float cvValue)
{
    return Clamp01(knobValue + cvValue);
}

// =============================================================================
// CV Modulation Processing
// =============================================================================

/**
 * Process CV modulation input to a ±0.5 range
 *
 * CV inputs are expected to be bipolar (-5V to +5V) normalized to -1.0 to +1.0.
 * This function converts to ±0.5 range for modulating 0-1 parameters.
 *
 * @param rawCV Raw CV input value (-1.0 to +1.0, or 0.0 to +1.0 for unipolar)
 * @return Modulation value in ±0.5 range
 *
 * Reference: docs/specs/main.md section 3.2
 */
inline float ProcessCVModulation(float rawCV)
{
    // Clamp input to valid range
    if (rawCV < -1.0f) rawCV = -1.0f;
    if (rawCV > 1.0f) rawCV = 1.0f;

    // Scale to ±0.5 range (±5V = ±50% modulation)
    return rawCV * 0.5f;
}

/// Gate threshold for fill input: 1V = 0.2 in normalized 0-5V range
constexpr float kFillGateThreshold = 0.2f;

/// Hysteresis for gate detection to prevent oscillation
constexpr float kFillGateHysteresis = 0.05f;

/**
 * Detect gate state from fill CV with hysteresis
 *
 * @param rawFillCV Raw fill CV input (0.0 to 1.0 normalized from 0-5V)
 * @param wasGateHigh Previous gate state (for hysteresis)
 * @return true if gate is now high
 */
inline bool DetectFillGate(float rawFillCV, bool wasGateHigh)
{
    // Clamp input
    if (rawFillCV < 0.0f) rawFillCV = 0.0f;
    if (rawFillCV > 1.0f) rawFillCV = 1.0f;

    // Apply hysteresis
    if (wasGateHigh)
    {
        return (rawFillCV > (kFillGateThreshold - kFillGateHysteresis));
    }
    else
    {
        return (rawFillCV > kFillGateThreshold);
    }
}

/**
 * Process Fill CV input for gate detection and intensity
 *
 * The fill input is "pressure-sensitive": gate detection (>1V) triggers fills,
 * and the CV level (0-5V) determines fill intensity.
 *
 * @param rawFillCV Raw fill CV input (0.0 to 1.0 normalized from 0-5V)
 * @param prevGateHigh Previous gate state (for rising edge detection)
 * @param outGateHigh Output: current gate state
 * @param outTriggered Output: true if rising edge detected
 * @param outIntensity Output: fill intensity (0.0 to 1.0)
 *
 * Reference: docs/specs/main.md section 3.3
 */
inline void ProcessFillInputRaw(float rawFillCV, bool prevGateHigh,
                                bool& outGateHigh, bool& outTriggered,
                                float& outIntensity)
{
    // Clamp input
    if (rawFillCV < 0.0f) rawFillCV = 0.0f;
    if (rawFillCV > 1.0f) rawFillCV = 1.0f;

    // Detect gate with hysteresis
    outGateHigh = DetectFillGate(rawFillCV, prevGateHigh);

    // Detect rising edge (trigger)
    outTriggered = (outGateHigh && !prevGateHigh);

    // Intensity is the full CV level (0-5V mapped to 0-1)
    outIntensity = rawFillCV;
}

/**
 * Process Flavor CV input for timing feel override
 *
 * Flavor CV controls the BROKEN timing stack (swing, jitter, displacement).
 * 0V = straight feel, 5V = maximum broken feel.
 *
 * @param rawFlavorCV Raw flavor CV input (0.0 to 1.0 normalized from 0-5V)
 * @return Flavor value (0.0 to 1.0)
 *
 * Reference: docs/specs/main.md section 3.3
 */
inline float ProcessFlavorCV(float rawFlavorCV)
{
    // Simple clamp - flavor is just a direct mapping
    if (rawFlavorCV < 0.0f) rawFlavorCV = 0.0f;
    if (rawFlavorCV > 1.0f) rawFlavorCV = 1.0f;
    return rawFlavorCV;
}

// =============================================================================
// Discrete Parameter Quantization
// =============================================================================

/**
 * Quantize a 0-1 knob value to discrete pattern length
 *
 * @param value Knob value (0.0 to 1.0)
 * @return Pattern length (16, 24, 32, or 64 steps)
 */
inline int QuantizePatternLength(float value)
{
    if (value < 0.25f) return 16;
    if (value < 0.50f) return 24;
    if (value < 0.75f) return 32;
    return 64;
}

/**
 * Quantize a 0-1 knob value to discrete phrase length
 *
 * @param value Knob value (0.0 to 1.0)
 * @return Phrase length (1, 2, 4, or 8 bars)
 */
inline int QuantizePhraseLength(float value)
{
    if (value < 0.25f) return 1;
    if (value < 0.50f) return 2;
    if (value < 0.75f) return 4;
    return 8;
}

/**
 * Quantize a 0-1 knob value to discrete clock division
 *
 * @param value Knob value (0.0 to 1.0)
 * @return Clock division (1, 2, 4, or 8)
 */
inline int QuantizeClockDivision(float value)
{
    if (value < 0.25f) return 1;
    if (value < 0.50f) return 2;
    if (value < 0.75f) return 4;
    return 8;
}

} // namespace daisysp_idm_grids

