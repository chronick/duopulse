#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * TriggerState: State for a single trigger output
 *
 * Tracks trigger timing for gate outputs. Triggers are short pulses
 * (typically 1-5ms) that fire on hits.
 *
 * Reference: docs/specs/main.md section 8.1
 */
struct TriggerState
{
    /// Whether the trigger is currently high
    bool high;

    /// Samples remaining in current trigger pulse
    int samplesRemaining;

    /// Configured trigger duration in samples
    int triggerDurationSamples;

    /// Event latch: set on Fire(), cleared by main loop via AcknowledgeEvent()
    /// This prevents race conditions where pulse completes before main loop checks
    bool eventPending;

    /**
     * Initialize trigger state
     *
     * @param durationSamples Trigger pulse duration in samples
     */
    void Init(int durationSamples = 48)
    {
        high                    = false;
        samplesRemaining        = 0;
        triggerDurationSamples  = durationSamples;
        eventPending            = false;
    }

    /**
     * Fire a trigger
     */
    void Fire()
    {
        high             = true;
        samplesRemaining = triggerDurationSamples;
        eventPending     = true;  // Latch for main loop detection
    }

    /**
     * Process one sample, decrementing trigger timer
     */
    void Process()
    {
        if (samplesRemaining > 0)
        {
            samplesRemaining--;
            if (samplesRemaining == 0)
            {
                high = false;
            }
        }
    }

    /**
     * Check if an event is pending (for main loop edge detection)
     * @return true if a trigger has fired since last acknowledgment
     */
    bool HasPendingEvent() const
    {
        return eventPending;
    }

    /**
     * Acknowledge the pending event (call from main loop after detecting)
     */
    void AcknowledgeEvent()
    {
        eventPending = false;
    }

    /**
     * Set trigger duration from milliseconds
     *
     * @param ms Duration in milliseconds
     * @param sampleRate Sample rate in Hz
     */
    void SetDurationMs(float ms, float sampleRate)
    {
        triggerDurationSamples = static_cast<int>(ms * sampleRate / 1000.0f);
        if (triggerDurationSamples < 1)
        {
            triggerDurationSamples = 1;
        }
    }
};

/**
 * VelocityOutputState: State for velocity CV output (sample & hold)
 *
 * Velocity outputs use sample & hold behavior—the voltage is set
 * on trigger and held until the next trigger on that channel.
 *
 * Reference: docs/specs/main.md section 8.2
 */
struct VelocityOutputState
{
    /// Current held voltage (0.0-1.0, maps to 0-5V)
    float heldVoltage;

    /// Target voltage for next trigger
    float targetVoltage;

    /// Whether a new value was triggered this sample
    bool triggered;

    /**
     * Initialize velocity output state
     */
    void Init()
    {
        heldVoltage   = 0.0f;
        targetVoltage = 0.0f;
        triggered     = false;
    }

    /**
     * Trigger a new velocity value (sample & hold)
     *
     * @param velocity Velocity value (0.0-1.0)
     */
    void Trigger(float velocity)
    {
        // Clamp velocity
        if (velocity < 0.0f) velocity = 0.0f;
        if (velocity > 1.0f) velocity = 1.0f;

        targetVoltage = velocity;
        heldVoltage   = velocity;
        triggered     = true;
    }

    /**
     * Process one sample (clears triggered flag)
     */
    void Process()
    {
        triggered = false;
    }

    /**
     * Get output voltage (0-5V scaled)
     */
    float GetVoltage() const
    {
        return heldVoltage * 5.0f;
    }
};

/**
 * LEDState: State for LED feedback output
 *
 * The LED output (CV_OUT_2) provides visual feedback for triggers,
 * mode changes, and parameter adjustments.
 *
 * Reference: docs/specs/main.md section 9.1
 */
struct LEDState
{
    /// Current brightness (0.0-1.0)
    float brightness;

    /// Target brightness (for smooth transitions)
    float targetBrightness;

    /// Brightness decay rate per sample
    float decayRate;

    /// Flash override brightness (for events)
    float flashBrightness;

    /// Flash samples remaining
    int flashSamplesRemaining;

    /// Whether in pulse mode (for live fill)
    bool pulseMode;

    /// Pulse phase (0.0-1.0)
    float pulsePhase;

    /// Pulse rate in Hz
    float pulseRate;

    /**
     * Initialize LED state
     *
     * @param sampleRate Sample rate in Hz
     */
    void Init(float sampleRate = 48000.0f)
    {
        brightness           = 0.0f;
        targetBrightness     = 0.0f;
        decayRate            = 1.0f / (sampleRate * 0.1f);  // 100ms decay
        flashBrightness      = 0.0f;
        flashSamplesRemaining = 0;
        pulseMode            = false;
        pulsePhase           = 0.0f;
        pulseRate            = 4.0f;  // 4 Hz pulse
    }

    /**
     * Trigger LED for an event
     *
     * @param intensity Brightness level (0.0-1.0)
     */
    void Trigger(float intensity)
    {
        if (intensity > targetBrightness)
        {
            targetBrightness = intensity;
            brightness       = intensity;
        }
    }

    /**
     * Flash LED (overrides normal brightness temporarily)
     *
     * @param durationSamples Flash duration in samples
     */
    void Flash(int durationSamples)
    {
        flashBrightness       = 1.0f;
        flashSamplesRemaining = durationSamples;
    }

    /**
     * Set pulse mode (for live fill indication)
     *
     * @param enabled Whether pulse mode is active
     */
    void SetPulseMode(bool enabled)
    {
        pulseMode = enabled;
        if (!enabled)
        {
            pulsePhase = 0.0f;
        }
    }

    /**
     * Process one sample
     *
     * @param sampleRate Sample rate in Hz
     */
    void Process(float sampleRate)
    {
        // Handle flash
        if (flashSamplesRemaining > 0)
        {
            flashSamplesRemaining--;
        }

        // Handle pulse mode
        if (pulseMode)
        {
            pulsePhase += pulseRate / sampleRate;
            if (pulsePhase >= 1.0f)
            {
                pulsePhase -= 1.0f;
            }
        }

        // Decay toward zero
        if (brightness > targetBrightness)
        {
            brightness -= decayRate;
            if (brightness < targetBrightness)
            {
                brightness = targetBrightness;
            }
        }

        // Reset target for next trigger
        targetBrightness = 0.0f;
    }

    /**
     * Get current output brightness (0.0-1.0)
     */
    float GetBrightness() const
    {
        // Flash overrides everything
        if (flashSamplesRemaining > 0)
        {
            return flashBrightness;
        }

        // Pulse mode modulates brightness
        if (pulseMode)
        {
            // Sine wave pulse (0.3 to 0.8 range)
            float pulse = 0.55f + 0.25f * sinf(pulsePhase * 6.2831853f);
            return pulse;
        }

        return brightness;
    }

    /**
     * Get output voltage (0-5V)
     */
    float GetVoltage() const
    {
        return GetBrightness() * 5.0f;
    }

private:
    // Simple sin approximation (avoid math.h in tight loops)
    static float sinf(float x)
    {
        // Normalize to -PI to PI range
        while (x > 3.14159265f) x -= 6.2831853f;
        while (x < -3.14159265f) x += 6.2831853f;

        // Parabolic approximation
        float y = x * (1.27323954f - 0.405284735f * (x < 0 ? -x : x));
        return y;
    }
};

/**
 * AuxOutputState: State for AUX output (CV_OUT_1)
 *
 * The AUX output can serve different purposes based on AuxMode.
 *
 * Reference: docs/specs/main.md section 8.3
 */
struct AuxOutputState
{
    /// Current mode
    AuxMode mode;

    /// Trigger state (for HAT and EVENT modes)
    TriggerState trigger;

    /// Gate state (for FILL_GATE mode)
    bool gateHigh;

    /// Phrase ramp value (for PHRASE_CV mode, 0.0-1.0)
    float phraseRamp;

    /**
     * Initialize AUX output state
     *
     * @param auxMode Initial mode
     */
    void Init(AuxMode auxMode = AuxMode::HAT)
    {
        mode = auxMode;
        trigger.Init();
        gateHigh   = false;
        phraseRamp = 0.0f;
    }

    /**
     * Get current output voltage (0-5V)
     */
    float GetVoltage() const
    {
        switch (mode)
        {
            case AuxMode::HAT:
            case AuxMode::EVENT:
                return trigger.high ? 5.0f : 0.0f;

            case AuxMode::FILL_GATE:
                return gateHigh ? 5.0f : 0.0f;

            case AuxMode::PHRASE_CV:
                return phraseRamp * 5.0f;

            default:
                return 0.0f;
        }
    }

    /**
     * Process one sample
     */
    void Process()
    {
        trigger.Process();
    }
};

/**
 * OutputState: Complete output state for all outputs
 *
 * This struct combines all output states for convenient access.
 *
 * Reference: docs/specs/main.md section 8
 */
struct OutputState
{
    /// Anchor trigger (Gate Out 1)
    TriggerState anchorTrigger;

    /// Shimmer trigger (Gate Out 2)
    TriggerState shimmerTrigger;

    /// Anchor velocity (Audio Out L, sample & hold)
    VelocityOutputState anchorVelocity;

    /// Shimmer velocity (Audio Out R, sample & hold)
    VelocityOutputState shimmerVelocity;

    /// AUX output (CV Out 1)
    AuxOutputState aux;

    /// LED output (CV Out 2)
    LEDState led;

    /// Clock output (used when no external clock patched)
    TriggerState clockOut;

    /// Whether using external clock (if true, AUX is free for other modes)
    bool usingExternalClock;

    /**
     * Initialize all outputs
     *
     * @param sampleRate Sample rate in Hz
     */
    void Init(float sampleRate = 48000.0f)
    {
        // Standard trigger duration: 10ms (matches main branch)
        // Many Eurorack drum modules need at least 2-5ms to reliably trigger
        int triggerSamples = static_cast<int>(sampleRate * 0.01f);  // 10ms

        anchorTrigger.Init(triggerSamples);
        shimmerTrigger.Init(triggerSamples);
        anchorVelocity.Init();
        shimmerVelocity.Init();
        aux.Init();
        led.Init(sampleRate);
        clockOut.Init(triggerSamples);

        usingExternalClock = false;
    }

    /**
     * Process one sample for all outputs
     *
     * @param sampleRate Sample rate in Hz
     */
    void Process(float sampleRate)
    {
        anchorTrigger.Process();
        shimmerTrigger.Process();
        anchorVelocity.Process();
        shimmerVelocity.Process();
        aux.Process();
        led.Process(sampleRate);
        clockOut.Process();
    }

    /**
     * Fire anchor with velocity
     *
     * @param velocity Velocity value (0.0-1.0)
     * @param accented Whether this hit is accented
     */
    void FireAnchor(float velocity, bool accented)
    {
        anchorTrigger.Fire();
        anchorVelocity.Trigger(velocity);

        // LED feedback: anchor = 80%, accented = 100%
        led.Trigger(accented ? 1.0f : 0.8f);
    }

    /**
     * Fire shimmer with velocity
     *
     * @param velocity Velocity value (0.0-1.0)
     * @param accented Whether this hit is accented
     */
    void FireShimmer(float velocity, bool accented)
    {
        shimmerTrigger.Fire();
        shimmerVelocity.Trigger(velocity);

        // LED feedback: shimmer = 30%, accented = 50%
        led.Trigger(accented ? 0.5f : 0.3f);
    }

    /**
     * Fire AUX (when in HAT or EVENT mode)
     */
    void FireAux()
    {
        if (aux.mode == AuxMode::HAT || aux.mode == AuxMode::EVENT)
        {
            aux.trigger.Fire();
        }
    }
};

} // namespace daisysp_idm_grids
