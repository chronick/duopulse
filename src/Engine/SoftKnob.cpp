#include "SoftKnob.h"
#include <cmath>

namespace daisysp_idm_grids {

SoftKnob::SoftKnob() 
    : value_(0.0f), locked_(false), first_process_(true), 
      last_raw_(0.0f), moved_(false), interpolation_rate_(kDefaultInterpRate) {}

void SoftKnob::Init(float value) {
    value_ = std::max(0.0f, std::min(1.0f, value));
    locked_ = true;
    first_process_ = true;
    moved_ = false;
}

float SoftKnob::Process(float raw_value) {
    moved_ = false;
    
    // Clamp raw input
    raw_value = std::max(0.0f, std::min(1.0f, raw_value));

    if (locked_) {
        // === Cross-Detection for Immediate Catchup ===
        // If physical position crosses the stored value, immediately unlock
        if (!first_process_) {
            bool was_above = last_raw_ > value_;
            bool is_above = raw_value > value_;
            bool crossed = (was_above != is_above);
            
            if (crossed) {
                // Physical knob crossed our stored value - immediate catchup
                locked_ = false;
                value_ = raw_value;
                moved_ = true;
                last_raw_ = raw_value;
                return value_;
            }
        }
        
        // === Immediate Unlock if Close Enough ===
        float diff = raw_value - value_;
        if (std::abs(diff) < kPickupThreshold) {
            locked_ = false;
            value_ = raw_value;
            last_raw_ = raw_value;
            return value_;
        }
        
        if (first_process_) {
            last_raw_ = raw_value;
            first_process_ = false;
            return value_;
        }

        // Detect knob movement for HasMoved() and for conditional interpolation
        float delta = raw_value - last_raw_;
        bool knobMoved = std::abs(delta) > kMovementThreshold;
        if (knobMoved) {
            moved_ = true;
        }

        // === Gradual Interpolation (10% per cycle toward physical position) ===
        // ONLY interpolate when the knob is being actively moved.
        // This prevents parameter drift when switching modes with the knob stationary.
        if (knobMoved) {
            float distance = raw_value - value_;
            value_ += distance * interpolation_rate_;
            
            // Clamp result
            value_ = std::max(0.0f, std::min(1.0f, value_));
            
            // Check for unlock (within threshold)
            if (std::abs(raw_value - value_) < kPickupThreshold) {
                locked_ = false;
                value_ = raw_value;
            }
        }
    } else {
        // Unlocked - direct tracking
        if (std::abs(raw_value - value_) > kMovementThreshold) {
            moved_ = true;
        }
        value_ = raw_value;
    }
    
    last_raw_ = raw_value;
    return value_;
}

float SoftKnob::GetValue() const {
    return value_;
}

void SoftKnob::SetValue(float value) {
    value_ = std::max(0.0f, std::min(1.0f, value));
    locked_ = true;
    first_process_ = true; // Reset raw tracking on external set
}

void SoftKnob::Lock() {
    locked_ = true;
    first_process_ = true;
}

bool SoftKnob::IsLocked() const {
    return locked_;
}

bool SoftKnob::HasMoved() {
    bool m = moved_;
    moved_ = false;
    return m;
}

void SoftKnob::SetInterpolationRate(float rate) {
    interpolation_rate_ = std::max(0.01f, std::min(1.0f, rate));
}

}
