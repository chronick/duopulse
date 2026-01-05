#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include "config.h"

// PulseField.h defines the Clamp helper function
#include "PulseField.h"

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

// =============================================================================
// V5 LED Layer System (Task 34)
// =============================================================================

/**
 * LED layer priority levels.
 * Higher values = higher priority and override lower layers.
 */
enum class LedLayer : uint8_t
{
    BASE    = 0,  ///< Base brightness (e.g., breath pattern during shift-held)
    TRIGGER = 1,  ///< Trigger-based brightness pulses
    FILL    = 2,  ///< Fill mode strobe pattern
    FLASH   = 3,  ///< Flash events (mode change, reset, reseed)
    REPLACE = 4   ///< Full replacement (boot patterns, config mode)
};

/// Number of LED layers in the system
static constexpr uint8_t kNumLedLayers = 5;

/**
 * State for a single LED layer.
 */
struct LedLayerState
{
    float    brightness{0.0f};    ///< Layer brightness (0-1)
    uint32_t expiresAtMs{0};      ///< Time when layer expires (0 = never)
    bool     active{false};       ///< Whether this layer is currently active
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

    // =========================================================================
    // V5 Layer System Private Members (Task 34)
    // =========================================================================
    LedLayerState layers_[kNumLedLayers]; ///< Layer states (indexed by LedLayer enum)
    float         breathPhase_{0.0f};     ///< Phase for breath animation (0-1)
    float         strobePhase_{0.0f};     ///< Phase for fill strobe animation (0-1)
    uint32_t      currentTimeMs_{0};      ///< Current time in milliseconds

  public:
    // =========================================================================
    // V5 Layer System Public Methods (Task 34)
    // =========================================================================

    /**
     * Set a layer's brightness and optional expiration time.
     * @param layer Which layer to set
     * @param brightness Brightness value (0-1)
     * @param durationMs Duration in ms (0 = permanent until cleared)
     */
    void SetLayer(LedLayer layer, float brightness, uint32_t durationMs = 0);

    /**
     * Clear a layer (deactivate it).
     * @param layer Which layer to clear
     */
    void ClearLayer(LedLayer layer);

    /**
     * Compute final brightness from all active layers.
     * Uses highest-priority active layer.
     * @return Combined brightness value (0-1)
     */
    float ComputeFinalBrightness();

    /**
     * Update the breathing animation (for shift-held mode).
     * Call at control rate. Updates BASE layer.
     * @param deltaMs Time since last call in milliseconds
     */
    void UpdateBreath(float deltaMs);

    /**
     * Update trigger decay animation.
     * Call at control rate. Updates TRIGGER layer.
     * @param deltaMs Time since last call in milliseconds
     * @param decayRatePerMs Decay rate per millisecond
     */
    void UpdateTriggerDecay(float deltaMs, float decayRatePerMs = 0.02f);

    /**
     * Update fill mode strobe animation.
     * Call at control rate. Updates FILL layer.
     * @param deltaMs Time since last call in milliseconds
     * @param periodMs Strobe period in milliseconds
     */
    void UpdateFillStrobe(float deltaMs, float periodMs = 100.0f);

    /**
     * Trigger a flash event (mode change, reset, reseed).
     * Sets FLASH layer for specified duration.
     * @param durationMs Duration of flash in milliseconds
     */
    void TriggerFlash(uint32_t durationMs = 100);

    // =========================================================================
    // Task 33: Boot-Time AUX Mode Flash Patterns
    // =========================================================================

    /**
     * Set direct brightness value (for boot flash patterns).
     * @param brightness Brightness value (0-1)
     */
    void SetBrightness(float brightness)
    {
        directBrightness_ = Clamp(brightness, 0.0f, 1.0f);
    }

    /**
     * Get current direct brightness value.
     * @return Current brightness (0-1)
     */
    float GetBrightness() const
    {
        return directBrightness_;
    }

    /**
     * Flash pattern for HAT mode unlock (rising: 33% -> 66% -> 100%).
     * Uses blocking delays on hardware, instant on host builds.
     *
     * IMPORTANT: Must only be called during boot initialization,
     * before StartAudio() is invoked. Blocking delays would cause
     * audio dropouts if called from audio callback path.
     */
    void FlashHatUnlock();

    /**
     * Flash pattern for FILL_GATE mode reset (fading: 100% -> 0%).
     * Uses blocking delays on hardware, instant on host builds.
     *
     * IMPORTANT: Must only be called during boot initialization,
     * before StartAudio() is invoked. Blocking delays would cause
     * audio dropouts if called from audio callback path.
     */
    void FlashFillGateReset();

  private:
    float directBrightness_{0.0f};  ///< Direct brightness for boot flash patterns
};

} // namespace daisysp_idm_grids
