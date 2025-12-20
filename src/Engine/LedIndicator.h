#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include "config.h"

#ifdef USE_PULSE_FIELD_V3
// When v3 is enabled, PulseField.h defines Clamp
#include "PulseField.h"
#else
namespace daisysp_idm_grids
{
// Helper for C++14 compatibility (std::clamp is C++17)
inline float Clamp(float value, float minVal, float maxVal)
{
    return std::max(minVal, std::min(value, maxVal));
}
} // namespace daisysp_idm_grids
#endif

namespace daisysp_idm_grids
{

/**
 * LED Feedback System for DuoPulse v4
 * 
 * The LED (CV_OUT_2) provides visual feedback that reflects the musical state.
 * This is a state machine that tracks triggers, mode changes, and parameter adjustments.
 * 
 * Brightness Levels (from spec 9.1):
 * - 0%   : Off (no activity)
 * - 30%  : Shimmer trigger
 * - 80%  : Anchor trigger
 * - 100% : Flash (mode change, reset, reseed)
 * - Pulse: Live fill mode active
 * - Gradient: Continuous parameter being adjusted
 * 
 * State Machine:
 * - Normal: Respond to triggers with appropriate brightness
 * - Flash: 100ms full brightness on mode change/reset/reseed
 * - FillPulse: Rapid pulsing during live fill mode
 * - ParameterAdjust: Show parameter value as brightness gradient
 * 
 * Reference: docs/specs/main.md section 9
 */

/**
 * LED operating mode (internal state machine)
 */
enum class LedMode
{
    Performance,   ///< Normal performance mode - respond to triggers
    Config,        ///< Config mode (solid ON at normal brightness)
    ShiftHeld,     ///< Shift button held (breathing pattern)
    Interaction    ///< Knob being turned (show value as gradient)
};

/**
 * LED event types that trigger special behavior
 */
enum class LedEvent : uint8_t
{
    NONE         = 0,  ///< No event
    MODE_CHANGE  = 1,  ///< Mode changed (perf <-> config)
    RESET        = 2,  ///< Reset triggered
    RESEED       = 3   ///< Pattern reseeded
};

/**
 * State passed to LedIndicator each process cycle
 */
struct LedState
{
    // Mode state
    LedMode mode = LedMode::Performance;
    
    // Parameter values (0-1) - v3 compatibility, may be removed
    float broken           = 0.0f;
    float drift            = 0.0f;
    float anchorDensity    = 0.5f;
    float shimmerDensity   = 0.5f;
    
    // Phrase position
    float phraseProgress   = 0.0f;
    bool  isDownbeat       = false;
    bool  isFillZone       = false;
    bool  isBuildZone      = false;
    
    // Trigger events (set true on frame when voice fires)
    bool  anchorTriggered  = false;
    bool  shimmerTriggered = false;
    
    // Special events (set on frame when event occurs)
    LedEvent event = LedEvent::NONE;
    
    // Live fill mode active (button held > 500ms, no knob moved)
    bool  liveFillActive   = false;
    
    // Interaction state (for parameter adjustment gradient)
    float interactionValue = 0.0f;   ///< Value to display during interaction (0-1)
};

/**
 * LED Indicator State Machine
 * 
 * Processes LedState and outputs brightness (0-1) for CV output.
 */
class LedIndicator
{
  public:
    static constexpr float kLedOnVoltage  = 5.0f;
    static constexpr float kLedOffVoltage = 0.0f;
    
    // Timing constants (in milliseconds)
    static constexpr float kBreathingCycleMs     = 500.0f;  ///< Shift breathing cycle
    static constexpr float kFlashDurationMs      = 100.0f;  ///< Mode change/reset/reseed flash
    static constexpr float kTriggerPulseMs       = 50.0f;   ///< Trigger pulse duration
    static constexpr float kDownbeatPulseMs      = 80.0f;   ///< Extra long for downbeat
    static constexpr float kFillPulsePeriodMs    = 150.0f;  ///< Live fill pulse period
    static constexpr float kTriplePulseMs        = 40.0f;   ///< Fill zone rapid pulse
    static constexpr float kTriplePulseGapMs     = 30.0f;   ///< Gap between triple pulses
    static constexpr float kMinFlashPeriodMs     = 50.0f;   ///< v3 compat: fastest flash
    static constexpr float kMaxFlashPeriodMs     = 300.0f;  ///< v3 compat: slowest flash
    
    // Brightness levels (from spec 9.1)
    static constexpr float kFlashBrightness      = 1.0f;    ///< 100% - mode change/reset/reseed
    static constexpr float kAnchorBrightness     = 0.8f;    ///< 80% - anchor trigger
    static constexpr float kShimmerBrightness    = 0.3f;    ///< 30% - shimmer trigger
    static constexpr float kOffBrightness        = 0.0f;    ///< 0% - no activity
    
    // Legacy aliases for v3 test compatibility
    static constexpr float kDownbeatBrightness   = kFlashBrightness;
    static constexpr float kNormalBrightness     = kAnchorBrightness;
    static constexpr float kDimBrightness        = kShimmerBrightness;
    static constexpr float kMinBrightness        = 0.1f;

    LedIndicator() = default;

    void Init(float sampleRateHz);

    /**
     * Process one sample worth of time and update LED state.
     * Call this at control rate (typically 1kHz) or audio rate.
     * 
     * @param state Current LED state (mode, triggers, events)
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
    float sampleRate_       = 48000.0f;
    float msPerSample_      = 1000.0f / 48000.0f;
    float timeMs_           = 0.0f;
    
    // Trigger tracking
    float anchorTriggerTimeMs_  = -1000.0f;
    float shimmerTriggerTimeMs_ = -1000.0f;
    
    // Event flash tracking
    float flashStartTimeMs_ = -1000.0f;
    
    // PRNG for v3 compatibility
    uint32_t seed_          = 12345;

    /**
     * Check if we're in a flash event (mode change, reset, reseed)
     */
    bool IsInFlashEvent() const;

    /**
     * Process flash event brightness
     */
    float ProcessFlashEvent();

    /**
     * Process live fill mode pulsing
     */
    float ProcessFillPulse();

    /**
     * Interaction mode: show parameter value as brightness gradient
     */
    float ProcessInteraction(const LedState& state);

    /**
     * Breathing pattern for shift held (sine wave)
     */
    float ProcessBreathing();

    /**
     * Performance mode: trigger-based brightness
     */
    float ProcessPerformance(const LedState& state);

    /**
     * Fill Zone: rapid triple-pulse pattern (v3 compatibility)
     */
    float ProcessFillZone(const LedState& state);

    /**
     * Build Zone: gradually increasing pulse rate (v3 compatibility)
     */
    float ProcessBuildZone(const LedState& state);

    /**
     * BROKEN × DRIFT behavior (v3 compatibility)
     */
    float ProcessBrokenDrift(const LedState& state);

    /**
     * Check if anchor trigger pulse is active
     */
    bool IsInAnchorPulse(float pulseDurationMs) const;

    /**
     * Check if shimmer trigger pulse is active
     */
    bool IsInShimmerPulse(float pulseDurationMs) const;

    /**
     * Simple pseudo-random for LED variation (deterministic per seed)
     */
    float GetPseudoRandom();
};

} // namespace daisysp_idm_grids
