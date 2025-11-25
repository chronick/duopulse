#include "SoftKnob.h"
#include <cmath>

namespace daisysp_idm_grids {

SoftKnob::SoftKnob() : value_(0.0f), locked_(false) {}

void SoftKnob::Init(float value) {
    value_ = value;
    locked_ = true;
}

float SoftKnob::Process(float raw_value) {
    if (locked_) {
        // Simple window-based pickup
        if (std::abs(raw_value - value_) < kPickupThreshold) {
            locked_ = false;
            value_ = raw_value;
        }
    } else {
        value_ = raw_value;
    }
    return value_;
}

float SoftKnob::GetValue() const {
    return value_;
}

void SoftKnob::SetValue(float value) {
    value_ = value;
    locked_ = true; // Always lock on external set to prevent immediate jump
}

void SoftKnob::Lock() {
    locked_ = true;
}

bool SoftKnob::IsLocked() const {
    return locked_;
}

}

