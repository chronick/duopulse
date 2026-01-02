#pragma once

#include <array>
#include <cstdint>

#include "daisysp.h"
#include "DuoPulseState.h"
#include "PatternField.h"
#include "HitBudget.h"
#include "GumbelSampler.h"
#include "VoiceRelation.h"
#include "GuardRails.h"
#include "DriftControl.h"
#include "BrokenEffects.h"
#include "VelocityCompute.h"
#include "PhrasePosition.h"
#include "config.h"

namespace daisysp_idm_grids
{

/**
 * DuoPulse v4 Core Sequencer
 *
 * The sequencer orchestrates the entire generation pipeline:
 * 1. Per-bar: Generate hit patterns using archetype blending and Gumbel selection
 * 2. Per-step: Apply timing effects and fire triggers
 * 3. Handle clock (internal/external), reset, and phrase boundaries
 *
 * The sequencer uses DuoPulseState for all state management and communicates
 * with the outside world through its public interface.
 *
 * Reference: docs/specs/main.md sections 11.2, 11.3
 */
class Sequencer
{
public:
    Sequencer() = default;
    ~Sequencer() = default;

    /**
     * Initialize the sequencer with a sample rate
     *
     * @param sampleRate Audio sample rate in Hz (typically 48000)
     */
    void Init(float sampleRate);

    // =========================================================================
    // Core Processing
    // =========================================================================

    /**
     * Process one audio sample
     *
     * This is the main entry point called from the audio callback.
     * It handles clock advancement, step processing, and output updates.
     *
     * @return Array of two floats: [anchor velocity, shimmer velocity]
     */
    std::array<float, 2> ProcessAudio();

    /**
     * Generate patterns for the current bar
     *
     * Called automatically at bar boundaries. Performs the full generation
     * pipeline: budget computation, eligibility masks, Gumbel selection,
     * voice relationship, and guard rails.
     */
    void GenerateBar();

    /**
     * Process the current step
     *
     * Checks hit masks, computes velocities, applies timing effects,
     * and fires triggers.
     */
    void ProcessStep();

    /**
     * Advance to the next step
     *
     * Updates position tracking and handles bar/phrase boundaries.
     */
    void AdvanceStep();

    // =========================================================================
    // External Triggers
    // =========================================================================

    /**
     * Trigger a reset based on current reset mode
     */
    void TriggerReset();

    /**
     * Process external clock pulse (rising edge)
     *
     * When called, enables exclusive external clock mode:
     * - Internal Metro is completely disabled
     * - Steps advance only on external clock rising edges
     * - No timeout-based fallback
     */
    void TriggerExternalClock();

    /**
     * Disable external clock and restore internal Metro
     *
     * Called when external clock is unplugged. Immediately restores
     * internal clock operation with no delay.
     */
    void DisableExternalClock();

    /**
     * Process tap tempo
     *
     * @param nowMs Current system time in milliseconds
     */
    void TriggerTapTempo(uint32_t nowMs);

    /**
     * Request pattern reseed (takes effect at phrase boundary)
     */
    void TriggerReseed();

    /**
     * Check if Field X/Y has changed significantly
     *
     * Compares current effective Field X/Y values (including CV modulation)
     * with previous values. If change exceeds threshold (0.1 or 10%), sets
     * fieldChangeRegenPending_ flag and updates previous values.
     *
     * @return true if regeneration should be triggered
     */
    bool CheckFieldChange();

    // =========================================================================
    // Parameter Setters (Performance Mode Primary)
    // =========================================================================

    void SetEnergy(float value);
    void SetBuild(float value);
    void SetFieldX(float value);
    void SetFieldY(float value);

    // =========================================================================
    // Parameter Setters (Performance Mode Shift)
    // =========================================================================

    void SetPunch(float value);
    void SetGenre(float value);
    void SetDrift(float value);
    void SetBalance(float value);

    // =========================================================================
    // Parameter Setters (Config Mode Primary)
    // =========================================================================

    void SetPatternLength(int steps);
    void SetSwing(float value);
    void SetAuxMode(float value);
    void SetResetMode(float value);

    // =========================================================================
    // Parameter Setters (Config Mode Shift)
    // =========================================================================

    void SetPhraseLength(int bars);
    void SetClockDivision(int div);
    void SetAuxDensity(float value);
    void SetVoiceCoupling(float value);

    // =========================================================================
    // CV Modulation Inputs
    // =========================================================================

    void SetEnergyCV(float value);
    void SetBuildCV(float value);
    void SetFieldXCV(float value);
    void SetFieldYCV(float value);
    void SetFlavorCV(float value);

    // =========================================================================
    // Legacy v3 Compatibility (forward to v4 equivalents)
    // =========================================================================

    void SetAnchorDensity(float value) { SetEnergy(value); }
    void SetShimmerDensity(float value) { SetBalance(1.0f - value); }
    void SetBroken(float value) { SetFlavorCV(value); }
    void SetFuse(float value) { SetBalance(value); }
    void SetLength(int bars) { SetPhraseLength(bars); }
    void SetCouple(float value) { SetVoiceCoupling(value); }
    void SetRatchet(float value) { SetBuild(value); }
    void SetAnchorAccent(float value) { SetPunch(value); }
    void SetShimmerAccent(float value) { (void)value; }
    void SetContour(float value) { (void)value; }
    void SetTempoControl(float value);
    void SetSwingTaste(float value) { SetSwing(value); }
    void SetGateTime(float value);
    void SetHumanize(float value) { (void)value; }
    void SetClockDiv(float value);
    void SetFlux(float value) { SetFlavorCV(value); }
    void SetOrbit(float value) { SetVoiceCoupling(value); }
    void SetTerrain(float value) { (void)value; }
    void SetGrid(float value) { (void)value; }

    // =========================================================================
    // State Queries
    // =========================================================================

    bool IsGateHigh(int channel) const;
    bool IsClockHigh() const;

    /**
     * Check if a trigger event is pending (latched, survives after pulse ends)
     * Use this for reliable edge detection from main loop.
     * @param channel 0=anchor, 1=shimmer
     */
    bool HasPendingTrigger(int channel) const;

    /**
     * Acknowledge a pending trigger event (clears the latch)
     * Call this after detecting and logging the event.
     * @param channel 0=anchor, 1=shimmer
     */
    void AcknowledgeTrigger(int channel);

    float GetBpm() const { return state_.currentBpm; }
    float GetSwingPercent() const;
    float GetBroken() const { return state_.controls.flavorCV; }
    float GetDrift() const { return state_.controls.drift; }
    float GetRatchet() const { return state_.controls.build; }

    int GetCurrentPatternIndex() const { return 0; }

    const PhrasePosition& GetPhrasePosition() const { return phrasePos_; }

    void SetBpm(float bpm);
    void SetAccentHoldMs(float milliseconds);
    void SetHihatHoldMs(float milliseconds);

    void ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent = false);

    // =========================================================================
    // Debug Getters (safe to call from main loop)
    // =========================================================================

    /** Get current anchor mask (for debug logging, 64-bit for long patterns) */
    uint64_t GetAnchorMask() const { return state_.sequencer.anchorMask; }

    /** Get current shimmer mask (for debug logging, 64-bit for long patterns) */
    uint64_t GetShimmerMask() const { return state_.sequencer.shimmerMask; }

    /** Get current aux mask (for debug logging, 64-bit for long patterns) */
    uint64_t GetAuxMask() const { return state_.sequencer.auxMask; }

    /** Get blended archetype weight at step (for debug logging) */
    float GetBlendedAnchorWeight(int step) const {
        return (step >= 0 && step < 32) ? state_.blendedArchetype.anchorWeights[step] : 0.0f;
    }

    /** Get current bar number (for detecting bar boundaries in main loop) */
    int GetCurrentBar() const { return state_.sequencer.currentBar; }

    /** Get current step within bar */
    int GetCurrentStep() const { return state_.sequencer.currentStep; }

    /** Get AUX output voltage (0-5V, mode-dependent) */
    float GetAuxVoltage() const { return state_.outputs.aux.GetVoltage(); }

    /** Check if AUX trigger is high (for HAT/EVENT modes) */
    bool IsAuxHigh() const { return state_.outputs.aux.trigger.high; }

private:
    // =========================================================================
    // Internal Methods
    // =========================================================================

    /**
     * Process internal clock tick
     *
     * @return true if a step tick occurred
     */
    bool ProcessInternalClock();

    /**
     * Update phrase position tracking
     */
    void UpdatePhrasePosition();

    /**
     * Compute blended archetype from current field position
     */
    void BlendArchetype();

    /**
     * Compute timing offsets for the current bar
     */
    void ComputeTimingOffsets();

    /**
     * Update derived control parameters
     */
    void UpdateDerivedControls();

    /**
     * Get timing offset for current step in samples
     */
    int GetStepTimingOffset() const;

    /**
     * Check if it's time to fire trigger for a voice
     */
    bool ShouldFireTrigger(Voice voice) const;

    /**
     * Convert samples to hold time
     */
    int HoldMsToSamples(float milliseconds) const;

    // =========================================================================
    // State
    // =========================================================================

    /// Complete DuoPulse state
    DuoPulseState state_;

    /// Phrase position for v3 compatibility
    PhrasePosition phrasePos_;

    /// Internal clock metro
    daisysp::Metro metro_;

    /// Sample rate
    float sampleRate_ = 48000.0f;

    /// Samples per 16th note step
    float samplesPerStep_ = 0.0f;

    /// Sample counter for step timing
    int stepSampleCounter_ = 0;

    /// Timing offset remaining for delayed trigger
    int triggerDelayRemaining_[3] = {0, 0, 0};

    /// Whether triggers are pending (delayed by swing/jitter)
    bool triggerPending_[3] = {false, false, false};

    /// Pending velocities for delayed triggers
    float pendingVelocity_[3] = {0.0f, 0.0f, 0.0f};

    /// Pending accent flags for delayed triggers
    bool pendingAccent_[3] = {false, false, false};

    // External clock state (exclusive mode - see spec section 3.4)
    bool externalClockActive_ = false;  // true = external clock controls steps, internal Metro disabled
    bool externalClockTick_ = false;    // true = external clock rising edge detected, consume on next ProcessAudio()

    // Clock division/multiplication state
    int clockPulseCounter_ = 0;         // Counts pulses for division (÷2, ÷4, ÷8)
    uint32_t lastExternalClockTime_ = 0; // For measuring external clock interval (multiplication)
    uint32_t externalClockInterval_ = 0; // Measured interval between pulses (in samples)
    int multiplicationSubdivCounter_ = 0; // Counts subdivisions for multiplication

    // Tap tempo state
    uint32_t lastTapTime_ = 0;

    // Force trigger state (for testing)
    bool forceNextTriggers_ = false;
    bool forcedTriggers_[3] = {false, false, false};
    bool forcedKickAccent_ = false;

    // Clock output state
    int clockTimer_ = 0;
    int clockDurationSamples_ = 0;

    // Hold times
    int accentHoldSamples_ = 0;
    int hihatHoldSamples_ = 0;

    // Field change tracking (Task 23: Immediate Field Updates)
    float previousFieldX_ = 0.0f;
    float previousFieldY_ = 0.0f;
    bool fieldChangeRegenPending_ = false;

    // Tempo range
    static constexpr float kMinTempo = 30.0f;
    static constexpr float kMaxTempo = 300.0f;
};

} // namespace daisysp_idm_grids
