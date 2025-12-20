#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include "config.h"
// PulseField.h defines Clamp
#include "PulseField.h"

namespace daisysp_idm_grids
{

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

    void Init(float sampleRateHz);

    /**
     * Process one audio sample worth of time and update LED state.
     * Call this at control rate (typically 1kHz) or audio rate.
     * 
     * @param state Current LED state (mode, parameters, triggers)
     * @return Brightness value 0-1
     */
    float Process(const LedState& state);

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
    float ProcessInteraction(const LedState& state);

    /**
     * Breathing pattern for shift held (sine wave)
     */
    float ProcessBreathing();

    /**
     * Performance mode: BROKEN × DRIFT behavior with phrase awareness
     */
    float ProcessPerformance(const LedState& state);

    /**
     * Fill Zone: rapid triple-pulse pattern
     * Three quick pulses in succession
     */
    float ProcessFillZone(const LedState& state);

    /**
     * Build Zone: gradually increasing pulse rate
     * Pulse rate increases as we approach fill zone
     */
    float ProcessBuildZone(const LedState& state);

    /**
     * BROKEN × DRIFT behavior matrix:
     * - Low BROKEN + Low DRIFT: regular, steady pulses
     * - Low BROKEN + High DRIFT: regular timing, varying intensity
     * - High BROKEN + Low DRIFT: irregular timing, consistent pattern each loop
     * - High BROKEN + High DRIFT: maximum irregularity
     */
    float ProcessBrokenDrift(const LedState& state);

    /**
     * Check if we're within trigger pulse window
     */
    bool IsInTriggerPulse(float pulseDurationMs) const;

    /**
     * Simple pseudo-random for LED variation (deterministic per seed)
     */
    float GetPseudoRandom();
};

} // namespace daisysp_idm_grids
