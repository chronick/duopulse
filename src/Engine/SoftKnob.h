#pragma once

#include <cmath>
#include <algorithm>

namespace daisysp_idm_grids {

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
     * Uses Value Scaling (Ableton style) when locked to smoothly interpolate
     * towards limits.
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

private:
    float value_; 
    bool locked_;
    bool first_process_;
    float last_raw_;
    bool moved_;
    const float kPickupThreshold = 0.05f; // 5% tolerance for immediate unlock
};

}
