#pragma once

#include <cmath>
#include <algorithm>

namespace daisysp_idm_grids {

/**
 * DuoPulse v2 Soft Takeover Knob
 * 
 * Prevents parameter jumps when switching between modes/shift states.
 * Uses gradual interpolation: 10% per cycle toward physical position.
 * Cross-detection enables immediate catchup when physical crosses stored value.
 * 
 * Reference: docs/specs/main.md section "Soft Takeover [duopulse-soft-pickup]"
 */
class SoftKnob {
public:
    SoftKnob();

    /**
     * @brief Initialize or reset the knob with a value.
     * This sets the internal value and locks the knob until pickup.
     * @param value The initial value to start at.
     */
    void Init(float value);

    /**
     * @brief Process the raw hardware reading.
     * Uses gradual interpolation (10% per cycle) when locked.
     * Cross-detection enables immediate catchup.
     * @param raw_value The current normalized value from the hardware knob (0.0 - 1.0).
     * @return The effective value.
     */
    float Process(float raw_value);

    /**
     * @brief Get the current effective value.
     */
    float GetValue() const;

    /**
     * @brief Force the value to a specific point (e.g. preset load).
     * This will re-engage the lock if the hardware doesn't match.
     */
    void SetValue(float value);
    
    /**
     * @brief Explicitly lock the knob (e.g. when switching to this parameter).
     */
    void Lock();
    
    /**
     * @brief Check if the knob is currently locked (scaling mode).
     */
    bool IsLocked() const;

    /**
     * @brief Check if the knob was moved in the last Process call.
     * Resets after call.
     */
    bool HasMoved();

    /**
     * @brief Set interpolation rate (0.0-1.0).
     * Default is 0.1 (10% per cycle).
     */
    void SetInterpolationRate(float rate);

private:
    float value_; 
    bool locked_;
    bool first_process_;
    float last_raw_;
    bool moved_;
    float interpolation_rate_;
    
    static constexpr float kPickupThreshold = 0.02f;   // 2% tolerance for immediate unlock
    static constexpr float kDefaultInterpRate = 0.1f;  // 10% per cycle
    static constexpr float kMovementThreshold = 0.002f; // Noise filter threshold
};

}
