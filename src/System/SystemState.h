#pragma once

#include <cstdint>

/**
 * Encapsulates the state logic for Phase 1 demo features:
 * - LED Blinking (1 Hz)
 * - Gate Toggling (1 Hz alternating)
 * - CV Ramping (0-5V over 4s)
 * 
 * This class is designed to be hardware-agnostic for easier testing.
 */
class SystemState
{
public:
    SystemState()
        : ledState_(false)
        , gateOneIsHigh_(false)
        , cvOutVoltage_(0.0f)
        , lastLedToggleMs_(0)
        , lastGateToggleMs_(0)
        , lastCvUpdateMs_(0)
    {}

    struct State
    {
        bool  ledOn;
        bool  gate1High;
        bool  gate2High;
        float cvOutputVolts;
    };

    void Init(uint32_t nowMs)
    {
        lastLedToggleMs_  = nowMs;
        lastGateToggleMs_ = nowMs;
        lastCvUpdateMs_   = nowMs;
        
        // Initial state
        ledState_      = false;
        gateOneIsHigh_ = false;
        cvOutVoltage_  = 0.0f;
    }

    State Process(uint32_t nowMs)
    {
        UpdateLed(nowMs);
        UpdateGates(nowMs);
        UpdateCvOutput(nowMs);

        return State{
            ledState_,
            gateOneIsHigh_,
            !gateOneIsHigh_,
            cvOutVoltage_
        };
    }

private:
    static constexpr uint32_t kLedToggleIntervalMs  = 500;
    static constexpr uint32_t kGateToggleIntervalMs = 1000;
    static constexpr uint32_t kCvRampPeriodMs       = 4000;
    static constexpr float    kCvRampMaxVoltage     = 5.0f;

    bool     ledState_;
    bool     gateOneIsHigh_;
    float    cvOutVoltage_;
    uint32_t lastLedToggleMs_;
    uint32_t lastGateToggleMs_;
    uint32_t lastCvUpdateMs_;

    float CvSlopePerMs() const
    {
        return kCvRampMaxVoltage / static_cast<float>(kCvRampPeriodMs);
    }

    void UpdateLed(uint32_t nowMs)
    {
        if(nowMs < lastLedToggleMs_) return; // Handle wrap-around or bad input gracefully
        if(nowMs - lastLedToggleMs_ >= kLedToggleIntervalMs)
        {
            ledState_ = !ledState_;
            lastLedToggleMs_ = nowMs;
        }
    }

    void UpdateGates(uint32_t nowMs)
    {
        if(nowMs < lastGateToggleMs_) return;
        if(nowMs - lastGateToggleMs_ >= kGateToggleIntervalMs)
        {
            gateOneIsHigh_ = !gateOneIsHigh_;
            lastGateToggleMs_ = nowMs;
        }
    }

    void UpdateCvOutput(uint32_t nowMs)
    {
        if(nowMs < lastCvUpdateMs_) return;
        const uint32_t elapsedMs = nowMs - lastCvUpdateMs_;
        if(elapsedMs == 0)
        {
            return;
        }

        cvOutVoltage_ += static_cast<float>(elapsedMs) * CvSlopePerMs();
        while(cvOutVoltage_ >= kCvRampMaxVoltage)
        {
            cvOutVoltage_ -= kCvRampMaxVoltage;
        }

        lastCvUpdateMs_ = nowMs;
    }
};

