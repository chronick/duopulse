#include "SoftKnob.h"
#include <cmath>

namespace daisysp_idm_grids {

SoftKnob::SoftKnob() 
    : value_(0.0f), locked_(false), first_process_(true), 
      last_raw_(0.0f), moved_(false) {}

void SoftKnob::Init(float value) {
    value_ = value;
    locked_ = true;
    first_process_ = true;
    moved_ = false;
}

float SoftKnob::Process(float raw_value) {
    moved_ = false;
    
    // Clamp raw input
    raw_value = std::max(0.0f, std::min(1.0f, raw_value));

    if (locked_) {
        // Immediate unlock if close enough to start with
        if (std::abs(raw_value - value_) < kPickupThreshold) {
            locked_ = false;
            value_ = raw_value;
            // If this was a large jump from previous, it might be a move, 
            // but usually this happens on first approach.
        } else {
            if (first_process_) {
                last_raw_ = raw_value;
                first_process_ = false;
                return value_;
            }

            float delta = raw_value - last_raw_;
            
            // Movement threshold to filter noise
            if (std::abs(delta) > 0.002f) { // Slightly higher threshold for "intentional move"
                moved_ = true;
                
                // Value Scaling
                if (delta > 0) {
                    // Moving UP towards 1.0
                    float dist_k = 1.0f - last_raw_;
                    float dist_v = 1.0f - value_;
                    
                    if (dist_k > 0.0001f) {
                        float scale = dist_v / dist_k;
                        // Protect against overshooting if scale is huge (shouldn't be if bounds checked)
                        value_ += delta * scale;
                    } else {
                        value_ = 1.0f;
                    }
                } else {
                    // Moving DOWN towards 0.0
                    float dist_k = last_raw_;
                    float dist_v = value_;
                    
                    if (dist_k > 0.0001f) {
                        float scale = dist_v / dist_k;
                        value_ += delta * scale;
                    } else {
                        value_ = 0.0f;
                    }
                }
                
                // Clamp
                value_ = std::max(0.0f, std::min(1.0f, value_));

                // Check for unlock (Crossover or Threshold match)
                // Crossover: (raw > value) != (last_raw > prev_value) implies we crossed?
                // Not necessarily, if we scale, we might stay on one side.
                // But if we catch up:
                if (std::abs(raw_value - value_) < kPickupThreshold) {
                    locked_ = false;
                    value_ = raw_value;
                }
            }
        }
    } else {
        // Unlocked - direct tracking
        if (std::abs(raw_value - value_) > 0.002f) {
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
    value_ = value;
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

}
