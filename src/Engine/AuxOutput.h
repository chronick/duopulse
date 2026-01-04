#pragma once

#include "DuoPulseTypes.h"
#include "OutputState.h"
#include "SequencerState.h"
#include "GateScaler.h"

namespace daisysp_idm_grids
{

/**
 * AuxOutput: Processor for the AUX output (CV Out 1)
 *
 * The AUX output can serve different purposes based on AuxMode:
 * - HAT: Third trigger voice (ghost/hi-hat pattern)
 * - FILL_GATE: Gate high during fill zones
 * - PHRASE_CV: 0-5V ramp over phrase, resets at loop boundary
 * - EVENT: Trigger on "interesting" moments (accents, fills, changes)
 *
 * Reference: docs/specs/main.md section 8.3
 *
 * Output: CV Out 1 (0-5V)
 */
class AuxOutput
{
  public:
    /// Maximum output voltage
    static constexpr float kMaxVoltage = 5.0f;

    /// Trigger duration for HAT and EVENT modes (ms)
    static constexpr float kTriggerDurationMs = 1.0f;

    AuxOutput() = default;

    /**
     * Initialize the AUX output processor
     *
     * @param sampleRate Audio sample rate in Hz
     */
    void Init(float sampleRate);

    /**
     * Set the AUX output mode
     *
     * @param mode New AUX mode
     */
    void SetMode(AuxMode mode);

    /**
     * Get current AUX mode
     */
    AuxMode GetMode() const { return mode_; }

    /**
     * Compute the AUX output value based on mode and state
     *
     * This is the main processing function that determines what the AUX
     * output should be based on the current mode and sequencer state.
     *
     * @param auxState AUX output state (updated with new values)
     * @param seqState Current sequencer state
     * @param inFillZone Whether currently in a fill zone
     * @param auxFires Whether AUX should fire (from hit mask)
     * @param isEvent Whether an "interesting" event occurred
     */
    void ComputeAuxOutput(AuxOutputState& auxState,
                          const SequencerState& seqState,
                          bool inFillZone,
                          bool auxFires,
                          bool isEvent);

    /**
     * Compute mode-specific output value
     *
     * Internal helper that computes the output for a specific mode.
     *
     * @param mode AUX mode to compute for
     * @param auxState AUX output state
     * @param seqState Sequencer state
     * @param patternLength Steps per bar
     * @param phraseLength Bars per phrase
     * @param inFillZone Whether in fill zone
     * @param auxFires Whether AUX trigger fires
     * @param isEvent Whether event trigger fires
     * @return Output voltage (0-5V)
     */
    float ComputeAuxModeOutput(AuxMode mode,
                               const AuxOutputState& auxState,
                               const SequencerState& seqState,
                               int patternLength,
                               int phraseLength,
                               bool inFillZone,
                               bool auxFires,
                               bool isEvent) const;

    /**
     * Get codec sample value for AUX output
     *
     * @param auxState Current AUX output state
     * @return Codec sample value
     */
    float ProcessAuxOutput(const AuxOutputState& auxState) const;

    /**
     * Fire the AUX trigger (for HAT and EVENT modes)
     *
     * @param auxState AUX output state to update
     */
    static void FireTrigger(AuxOutputState& auxState);

    /**
     * Set fill gate state
     *
     * @param auxState AUX output state to update
     * @param gateHigh Whether gate should be high
     */
    static void SetFillGate(AuxOutputState& auxState, bool gateHigh);

    /**
     * Update phrase ramp value
     *
     * @param auxState AUX output state to update
     * @param progress Phrase progress (0.0-1.0)
     */
    static void UpdatePhraseRamp(AuxOutputState& auxState, float progress);

  private:
    float sampleRate_ = 48000.0f;
    AuxMode mode_     = AuxMode::HAT;

    // Cached pattern/phrase lengths for PHRASE_CV mode
    int patternLength_ = 16;
    int phraseLength_  = 4;
};

} // namespace daisysp_idm_grids
