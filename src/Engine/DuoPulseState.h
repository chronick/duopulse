#pragma once

#include "DuoPulseTypes.h"
#include "ArchetypeDNA.h"
#include "ControlState.h"
#include "SequencerState.h"
#include "OutputState.h"

namespace daisysp_idm_grids
{

/**
 * DuoPulseState: Complete firmware state for DuoPulse v4
 *
 * This struct combines all state required for the sequencer to operate.
 * It serves as the central data structure passed between processing stages.
 *
 * Reference: docs/specs/main.md section 10
 */
struct DuoPulseState
{
    // =========================================================================
    // Core State Structures
    // =========================================================================

    /// All control parameters (from knobs, CVs, buttons)
    ControlState controls;

    /// Sequencer position and pattern state
    SequencerState sequencer;

    /// All output states (triggers, velocity, LED, AUX)
    OutputState outputs;

    // =========================================================================
    // Pattern Data (loaded per genre)
    // =========================================================================

    /// Current genre's 3x3 archetype field
    GenreField currentField;

    /// Currently blended archetype (result of FIELD X/Y morphing)
    ArchetypeDNA blendedArchetype;

    // =========================================================================
    // System State
    // =========================================================================

    /// Sample rate in Hz
    float sampleRate;

    /// Samples per step (derived from tempo)
    float samplesPerStep;

    /// Current tempo in BPM
    float currentBpm;

    /// Sample counter within current step
    int stepSampleCounter;

    /// Whether the system is running (responding to clock)
    bool running;

    /// Whether in config mode (false = performance mode)
    bool configMode;

    /// Whether shift button is held
    bool shiftHeld;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize all state to defaults
     *
     * @param sampleRateHz Sample rate in Hz (default 48000)
     */
    void Init(float sampleRateHz = 48000.0f)
    {
        sampleRate = sampleRateHz;

        controls.Init();
        sequencer.Init();
        outputs.Init(sampleRate);

        currentField.Init();
        blendedArchetype.Init();

        // Default tempo: 120 BPM
        currentBpm = 120.0f;
        UpdateSamplesPerStep();

        stepSampleCounter = 0;
        running           = true;
        configMode        = false;
        shiftHeld         = false;
    }

    /**
     * Update samples per step based on current BPM
     *
     * Steps are 16th notes, so:
     * samplesPerStep = (sampleRate * 60) / (BPM * 4)
     */
    void UpdateSamplesPerStep()
    {
        samplesPerStep = (sampleRate * 60.0f) / (currentBpm * 4.0f);
    }

    /**
     * Set tempo in BPM
     *
     * @param bpm Tempo in beats per minute (clamped to 30-300)
     */
    void SetBpm(float bpm)
    {
        // Clamp to reasonable range
        if (bpm < 30.0f) bpm = 30.0f;
        if (bpm > 300.0f) bpm = 300.0f;

        currentBpm = bpm;
        UpdateSamplesPerStep();
    }

    /**
     * Get current phrase progress (0.0-1.0)
     */
    float GetPhraseProgress() const
    {
        return sequencer.GetPhraseProgress(
            controls.patternLength,
            controls.GetDerivedPhraseLength()
        );
    }

    /**
     * Check if we should advance to the next step
     *
     * @return true if it's time to advance
     */
    bool ShouldAdvanceStep() const
    {
        return stepSampleCounter >= static_cast<int>(samplesPerStep);
    }

    /**
     * Advance to the next step
     */
    void AdvanceStep()
    {
        stepSampleCounter = 0;
        sequencer.AdvanceStep(controls.patternLength, controls.GetDerivedPhraseLength());

        // Update derived parameters based on new position
        controls.UpdateDerived(GetPhraseProgress());
    }

    /**
     * Process one audio sample
     *
     * Increments step counter and processes all outputs.
     */
    void ProcessSample()
    {
        stepSampleCounter++;
        outputs.Process(sampleRate);
    }

    /**
     * Trigger a reset based on current reset mode
     */
    void TriggerReset()
    {
        sequencer.Reset(controls.resetMode, controls.patternLength);
        stepSampleCounter = 0;
    }

    /**
     * Request pattern reseed (takes effect at phrase boundary)
     */
    void RequestReseed()
    {
        sequencer.driftState.RequestReseed();
        outputs.led.Flash(static_cast<int>(sampleRate * 0.1f));  // 100ms flash
    }

    /**
     * Check if current step is a bar boundary (time to regenerate patterns)
     */
    bool IsBarBoundary() const
    {
        return sequencer.isBarBoundary;
    }

    /**
     * Check if current step is a phrase boundary
     */
    bool IsPhraseBoundary() const
    {
        return sequencer.isPhraseBoundary;
    }
};

} // namespace daisysp_idm_grids
