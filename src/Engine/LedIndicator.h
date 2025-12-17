#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>

namespace daisysp_idm_grids
{

// Helper for C++14 compatibility (std::clamp is C++17)
inline float Clamp(float value, float minVal, float maxVal)
{
    return std::max(minVal, std::min(value, maxVal));
}

/**
 * LED Feedback System for DuoPulse v3
 * 
 * The LED provides visual feedback that reflects the musical state:
 * 
 * Mode Indication:
 * - Performance Mode: pulse on Anchor triggers
 * - Config Mode: solid ON
 * - Shift held: slower breathing (500ms cycle)
 * 
 * Parameter Feedback (when knob moved):
 * - DENSITY: brightness = level
 * - BROKEN: flash rate increases with level
 * - DRIFT: pulse regularity decreases with level
 * 
 * BROKEN × DRIFT Behavior:
 * - Low BROKEN + Low DRIFT: regular, steady pulses
 * - Low BROKEN + High DRIFT: regular timing, varying intensity
 * - High BROKEN + Low DRIFT: irregular timing, consistent each loop
 * - High BROKEN + High DRIFT: maximum irregularity
 * 
 * Phrase Position Feedback:
 * - Downbeat: extra bright pulse
 * - Fill Zone: rapid triple-pulse pattern
 * - Build Zone: gradually increasing pulse rate
 */

enum class LedMode
{
    Performance,   // Normal performance mode
    Config,        // Config mode (solid ON)
    ShiftHeld,     // Shift button held (breathing)
    Interaction    // Knob being turned (show value)
};

struct LedState
{
    // Mode state
    LedMode mode = LedMode::Performance;
    
    // Parameter values (0-1)
    float broken           = 0.0f;
    float drift            = 0.0f;
    float anchorDensity    = 0.5f;
    float shimmerDensity   = 0.5f;
    
    // Phrase position
    float phraseProgress   = 0.0f;
    bool  isDownbeat       = false;
    bool  isFillZone       = false;
    bool  isBuildZone      = false;
    
    // Trigger events
    bool  anchorTriggered  = false;  // True on frame when anchor fires
    
    // Interaction state
    float interactionValue = 0.0f;   // Value to display during interaction
};

class LedIndicator
{
  public:
    static constexpr float kLedOnVoltage  = 5.0f;
    static constexpr float kLedOffVoltage = 0.0f;
    
    // Timing constants (in milliseconds)
    static constexpr float kBreathingCycleMs     = 500.0f;  // Shift breathing cycle
    static constexpr float kTriggerPulseMs       = 50.0f;   // Anchor trigger pulse
    static constexpr float kDownbeatPulseMs      = 80.0f;   // Extra long for downbeat
    static constexpr float kTriplePulseMs        = 40.0f;   // Fill zone rapid pulse
    static constexpr float kTriplePulseGapMs     = 30.0f;   // Gap between triple pulses
    static constexpr float kMinFlashPeriodMs     = 50.0f;   // Fastest flash (high BROKEN)
    static constexpr float kMaxFlashPeriodMs     = 300.0f;  // Slowest flash (low BROKEN)
    
    // Brightness levels
    static constexpr float kDownbeatBrightness   = 1.0f;    // Extra bright
    static constexpr float kNormalBrightness     = 0.8f;
    static constexpr float kDimBrightness        = 0.3f;
    static constexpr float kMinBrightness        = 0.1f;

    LedIndicator() = default;

    void Init(float sampleRateHz)
    {
        sampleRate_    = sampleRateHz;
        msPerSample_   = 1000.0f / sampleRateHz;
        timeMs_        = 0.0f;
        triggerTimeMs_ = -1000.0f; // Start with no recent trigger
        seed_          = 12345;
    }

    /**
     * Process one audio sample worth of time and update LED state.
     * Call this at control rate (typically 1kHz) or audio rate.
     * 
     * @param state Current LED state (mode, parameters, triggers)
     * @return Brightness value 0-1
     */
    float Process(const LedState& state)
    {
        timeMs_ += msPerSample_;
        
        // Handle anchor trigger (reset pulse timer)
        if(state.anchorTriggered)
        {
            triggerTimeMs_ = timeMs_;
        }
        
        float brightness = 0.0f;
        
        switch(state.mode)
        {
            case LedMode::Interaction:
                brightness = ProcessInteraction(state);
                break;
                
            case LedMode::Config:
                brightness = kNormalBrightness;
                break;
                
            case LedMode::ShiftHeld:
                brightness = ProcessBreathing();
                break;
                
            case LedMode::Performance:
            default:
                brightness = ProcessPerformance(state);
                break;
        }
        
        return Clamp(brightness, 0.0f, 1.0f);
    }

    /**
     * Convert brightness (0-1) to voltage (0-5V)
     */
    static float BrightnessToVoltage(float brightness)
    {
        return brightness * kLedOnVoltage;
    }

    /**
     * Legacy helper for simple on/off state
     */
    static constexpr float VoltageForState(bool isOn)
    {
        return isOn ? kLedOnVoltage : kLedOffVoltage;
    }

  private:
    float sampleRate_    = 48000.0f;
    float msPerSample_   = 1000.0f / 48000.0f;
    float timeMs_        = 0.0f;
    float triggerTimeMs_ = -1000.0f;
    uint32_t seed_       = 12345;

    /**
     * Interaction mode: show parameter value as brightness
     */
    float ProcessInteraction(const LedState& state)
    {
        return state.interactionValue;
    }

    /**
     * Breathing pattern for shift held (sine wave)
     */
    float ProcessBreathing()
    {
        float phase = fmodf(timeMs_, kBreathingCycleMs) / kBreathingCycleMs;
        // Sine wave from 0.2 to 1.0
        float sine = sinf(phase * 2.0f * 3.14159265f);
        return 0.6f + 0.4f * sine;
    }

    /**
     * Performance mode: BROKEN × DRIFT behavior with phrase awareness
     */
    float ProcessPerformance(const LedState& state)
    {
        float brightness = 0.0f;
        
        // Phrase position modulation
        if(state.isFillZone)
        {
            brightness = ProcessFillZone(state);
        }
        else if(state.isBuildZone)
        {
            brightness = ProcessBuildZone(state);
        }
        else
        {
            brightness = ProcessBrokenDrift(state);
        }
        
        // Downbeat: extra bright pulse overlay
        if(state.isDownbeat && IsInTriggerPulse(kDownbeatPulseMs))
        {
            brightness = std::max(brightness, kDownbeatBrightness);
        }
        // Normal anchor trigger pulse
        else if(IsInTriggerPulse(kTriggerPulseMs))
        {
            brightness = std::max(brightness, kNormalBrightness);
        }
        
        return brightness;
    }

    /**
     * Fill Zone: rapid triple-pulse pattern
     * Three quick pulses in succession
     */
    float ProcessFillZone(const LedState& state)
    {
        // Triple pulse cycle
        float cyclePeriod = (kTriplePulseMs * 3) + (kTriplePulseGapMs * 3);
        float cyclePos = fmodf(timeMs_, cyclePeriod);
        
        // Three pulses with gaps
        float pulsePhase = cyclePos / cyclePeriod;
        bool inPulse = false;
        
        // Pulse 1: 0-33% of pulse portion
        // Pulse 2: 33-66% of pulse portion  
        // Pulse 3: 66-100% of pulse portion
        float pulseRatio = kTriplePulseMs / (kTriplePulseMs + kTriplePulseGapMs);
        float segmentSize = 1.0f / 3.0f;
        
        for(int i = 0; i < 3; i++)
        {
            float segmentStart = i * segmentSize;
            float pulseEnd = segmentStart + (segmentSize * pulseRatio);
            if(pulsePhase >= segmentStart && pulsePhase < pulseEnd)
            {
                inPulse = true;
                break;
            }
        }
        
        return inPulse ? kNormalBrightness : kDimBrightness;
    }

    /**
     * Build Zone: gradually increasing pulse rate
     * Pulse rate increases as we approach fill zone
     */
    float ProcessBuildZone(const LedState& state)
    {
        // Build zone typically spans 50-75% of phrase
        // Assume buildProgress goes 0-1 within build zone
        // (We use phraseProgress as proxy: 0.5-0.75 mapped to 0-1)
        float buildProgress = 0.0f;
        if(state.phraseProgress >= 0.5f && state.phraseProgress < 0.75f)
        {
            buildProgress = (state.phraseProgress - 0.5f) / 0.25f;
        }
        
        // Pulse rate: 400ms → 100ms as build progresses
        float pulsePeriod = 400.0f - (300.0f * buildProgress);
        float phase = fmodf(timeMs_, pulsePeriod) / pulsePeriod;
        
        // Square wave with increasing duty cycle
        float dutyCycle = 0.3f + (0.3f * buildProgress);
        return (phase < dutyCycle) ? kNormalBrightness : kDimBrightness;
    }

    /**
     * BROKEN × DRIFT behavior matrix:
     * - Low BROKEN + Low DRIFT: regular, steady pulses
     * - Low BROKEN + High DRIFT: regular timing, varying intensity
     * - High BROKEN + Low DRIFT: irregular timing, consistent pattern each loop
     * - High BROKEN + High DRIFT: maximum irregularity
     */
    float ProcessBrokenDrift(const LedState& state)
    {
        // Flash period decreases with BROKEN (faster = more chaotic)
        float flashPeriod = kMaxFlashPeriodMs - 
                           (state.broken * (kMaxFlashPeriodMs - kMinFlashPeriodMs));
        
        // Low DRIFT: use consistent timing based on absolute time
        // High DRIFT: add randomness to timing
        float effectiveTime = timeMs_;
        if(state.drift > 0.3f)
        {
            // Add time jitter proportional to drift
            float jitter = GetPseudoRandom() * state.drift * flashPeriod * 0.3f;
            effectiveTime += jitter;
        }
        
        float phase = fmodf(effectiveTime, flashPeriod) / flashPeriod;
        
        // Duty cycle: lower BROKEN = longer on-time (more steady)
        float dutyCycle = 0.5f - (state.broken * 0.3f);
        bool inPulse = (phase < dutyCycle);
        
        // Intensity variation with DRIFT
        float intensity = kNormalBrightness;
        if(state.drift > 0.2f)
        {
            // Vary intensity with drift level
            float variation = GetPseudoRandom() * state.drift * 0.4f;
            intensity = kNormalBrightness - variation;
        }
        
        // High BROKEN: add some irregularity to pulse shape
        if(state.broken > 0.5f && inPulse)
        {
            float chaosFactor = (state.broken - 0.5f) * 2.0f;
            if(GetPseudoRandom() < chaosFactor * 0.3f)
            {
                // Random dropout
                inPulse = false;
            }
        }
        
        return inPulse ? intensity : kMinBrightness;
    }

    /**
     * Check if we're within trigger pulse window
     */
    bool IsInTriggerPulse(float pulseDurationMs) const
    {
        return (timeMs_ - triggerTimeMs_) < pulseDurationMs;
    }

    /**
     * Simple pseudo-random for LED variation (deterministic per seed)
     */
    float GetPseudoRandom()
    {
        seed_ = seed_ * 1103515245 + 12345;
        return static_cast<float>((seed_ >> 16) & 0x7FFF) / 32767.0f;
    }
};

} // namespace daisysp_idm_grids
