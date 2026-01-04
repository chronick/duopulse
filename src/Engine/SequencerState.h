#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * DriftState: Seed management for pattern evolution
 *
 * The DRIFT system controls pattern evolution using dual seeds:
 * - patternSeed: Fixed per "song", changes only on reseed
 * - phraseSeed: Changes each phrase, derived from patternSeed + counter
 *
 * Reference: docs/specs/main.md section 6.7
 */
struct DriftState
{
    /// Fixed seed for "locked" elements (persists across phrases)
    uint32_t patternSeed;

    /// Varying seed for "drifting" elements (changes each phrase)
    uint32_t phraseSeed;

    /// Counter for generating new phrase seeds
    uint32_t phraseCounter;

    /// Whether a reseed was requested (takes effect at phrase boundary)
    bool reseedRequested;

    /**
     * Initialize with a starting seed
     *
     * @param initialSeed Initial pattern seed
     */
    void Init(uint32_t initialSeed = 0x12345678)
    {
        patternSeed     = initialSeed;
        phraseSeed      = initialSeed ^ 0xDEADBEEF;
        phraseCounter   = 0;
        reseedRequested = false;
    }

    /**
     * Request a reseed (will take effect at next phrase boundary)
     */
    void RequestReseed()
    {
        reseedRequested = true;
    }

    /**
     * Called at phrase boundary to regenerate phrase seed
     * and handle any pending reseed request
     */
    void OnPhraseBoundary()
    {
        if (reseedRequested)
        {
            // Generate entirely new pattern seed
            // Use current time-ish value for unpredictability
            patternSeed ^= phraseCounter * 0x9e3779b9;
            patternSeed ^= (patternSeed >> 16);
            patternSeed *= 0x85ebca6b;
            reseedRequested = false;
        }

        // Always generate new phrase seed
        phraseCounter++;
        phraseSeed = HashCombine(patternSeed, phraseCounter);
    }

    /**
     * Get the seed to use for a step based on DRIFT and step stability
     *
     * @param drift DRIFT parameter (0.0-1.0)
     * @param stepStability Stability of the step (0.0-1.0, downbeats = 1.0)
     * @return Seed to use for this step
     */
    uint32_t GetSeedForStep(float drift, float stepStability) const
    {
        // If step stability > drift threshold, use locked pattern seed
        // Otherwise use varying phrase seed
        if (stepStability > drift)
        {
            return patternSeed;
        }
        return phraseSeed;
    }

private:
    /**
     * Simple hash combine function
     */
    static uint32_t HashCombine(uint32_t seed, uint32_t value)
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
};

/**
 * GuardRailState: Tracking for constraint enforcement
 *
 * Guard rails ensure musical output regardless of parameter settings.
 * This state tracks what corrections have been made and whether
 * constraints are being violated.
 *
 * Reference: docs/specs/main.md section 6.6
 */
struct GuardRailState
{
    /// Steps since last anchor hit (for max gap rule)
    int stepsSinceLastAnchor;

    /// Consecutive shimmer hits without anchor (for burst limiting)
    int consecutiveShimmerHits;

    /// Whether downbeat was forced this bar
    bool downbeatForced;

    /// Number of soft repairs made this bar
    int softRepairsThisBar;

    /// Number of hard corrections made this bar
    int hardCorrectionsThisBar;

    /**
     * Initialize state at bar start
     */
    void Init()
    {
        stepsSinceLastAnchor    = 0;
        consecutiveShimmerHits  = 0;
        downbeatForced          = false;
        softRepairsThisBar      = 0;
        hardCorrectionsThisBar  = 0;
    }

    /**
     * Reset at bar boundary
     */
    void OnBarBoundary()
    {
        downbeatForced         = false;
        softRepairsThisBar     = 0;
        hardCorrectionsThisBar = 0;
        // Note: stepsSinceLastAnchor and consecutiveShimmerHits persist
    }

    /**
     * Update after an anchor hit
     */
    void OnAnchorHit()
    {
        stepsSinceLastAnchor   = 0;
        consecutiveShimmerHits = 0;
    }

    /**
     * Update after a shimmer hit (without anchor)
     */
    void OnShimmerOnlyHit()
    {
        consecutiveShimmerHits++;
    }

    /**
     * Update after a step with no hits
     */
    void OnNoHit()
    {
        stepsSinceLastAnchor++;
    }
};

/**
 * SequencerState: Core sequencer position and pattern state
 *
 * This struct maintains the current position within the sequence,
 * the generated hit masks for the current bar, and event flags.
 *
 * Reference: docs/specs/main.md section 10
 */
struct SequencerState
{
    // =========================================================================
    // Position Tracking
    // =========================================================================

    /// Current step within bar (0 to patternLength-1)
    int currentStep;

    /// Current bar within phrase (0 to phraseLength-1)
    int currentBar;

    /// Current phrase number (monotonically increasing)
    int currentPhrase;

    /// Total steps processed (for timing)
    uint64_t totalSteps;

    // =========================================================================
    // Hit Masks for Current Bar
    // =========================================================================

    /// Anchor hit mask (bit N = step N fires)
    uint64_t anchorMask;

    /// Shimmer hit mask (bit N = step N fires)
    uint64_t shimmerMask;

    /// Aux hit mask (bit N = step N fires)
    uint64_t auxMask;

    /// Anchor accent mask (bit N = step N is accented)
    uint64_t anchorAccentMask;

    /// Shimmer accent mask (bit N = step N is accented)
    uint64_t shimmerAccentMask;

    // =========================================================================
    // Timing Offsets for Current Bar (in samples)
    // =========================================================================

    /// Swing offset for each step (positive = late)
    int16_t swingOffsets[kMaxSteps];

    /// Microtiming jitter for each step
    int16_t jitterOffsets[kMaxSteps];

    // =========================================================================
    // Event Flags (reset each step)
    // =========================================================================

    /// Whether this step is a bar boundary
    bool isBarBoundary;

    /// Whether this step is a phrase boundary
    bool isPhraseBoundary;

    /// Whether this step is in a fill zone
    bool inFillZone;

    /// Whether a reset was triggered this step
    bool resetTriggered;

    // =========================================================================
    // Drift and Guard Rail State
    // =========================================================================

    DriftState driftState;
    GuardRailState guardRailState;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize to start of sequence
     *
     * @param seed Initial pattern seed
     */
    void Init(uint32_t seed = 0x12345678)
    {
        currentStep   = 0;
        currentBar    = 0;
        currentPhrase = 0;
        totalSteps    = 0;

        anchorMask        = 0;
        shimmerMask       = 0;
        auxMask           = 0;
        anchorAccentMask  = 0;
        shimmerAccentMask = 0;

        for (int i = 0; i < kMaxSteps; ++i)
        {
            swingOffsets[i]  = 0;
            jitterOffsets[i] = 0;
        }

        isBarBoundary    = true;
        isPhraseBoundary = true;
        inFillZone       = false;
        resetTriggered   = false;

        driftState.Init(seed);
        guardRailState.Init();
    }

    /**
     * Advance to next step
     *
     * @param patternLength Steps per bar
     * @param phraseLength Bars per phrase
     */
    void AdvanceStep(int patternLength, int phraseLength)
    {
        totalSteps++;
        currentStep++;

        isBarBoundary    = false;
        isPhraseBoundary = false;
        resetTriggered   = false;

        if (currentStep >= patternLength)
        {
            currentStep = 0;
            currentBar++;
            isBarBoundary = true;
            guardRailState.OnBarBoundary();

            if (currentBar >= phraseLength)
            {
                currentBar = 0;
                currentPhrase++;
                isPhraseBoundary = true;
                driftState.OnPhraseBoundary();
            }
        }
    }

    /**
     * Reset to start based on reset mode
     *
     * @param mode Reset behavior
     * @param patternLength Steps per bar (for BAR mode)
     */
    void Reset(ResetMode mode, int patternLength)
    {
        resetTriggered = true;

        switch (mode)
        {
            case ResetMode::PHRASE:
                currentStep   = 0;
                currentBar    = 0;
                isBarBoundary    = true;
                isPhraseBoundary = true;
                guardRailState.Init();
                break;

            case ResetMode::BAR:
                currentStep = 0;
                isBarBoundary = true;
                guardRailState.OnBarBoundary();
                break;

            case ResetMode::STEP:
                currentStep = 0;
                break;

            default:
                break;
        }
    }

    /**
     * Check if anchor fires on current step
     */
    bool AnchorFires() const
    {
        return (anchorMask & (1ULL << currentStep)) != 0;
    }

    /**
     * Check if shimmer fires on current step
     */
    bool ShimmerFires() const
    {
        return (shimmerMask & (1ULL << currentStep)) != 0;
    }

    /**
     * Check if aux fires on current step
     */
    bool AuxFires() const
    {
        return (auxMask & (1ULL << currentStep)) != 0;
    }

    /**
     * Check if anchor is accented on current step
     */
    bool AnchorAccented() const
    {
        return AnchorFires() && (anchorAccentMask & (1ULL << currentStep)) != 0;
    }

    /**
     * Check if shimmer is accented on current step
     */
    bool ShimmerAccented() const
    {
        return ShimmerFires() && (shimmerAccentMask & (1ULL << currentStep)) != 0;
    }

    /**
     * Get current phrase progress (0.0-1.0)
     *
     * @param patternLength Steps per bar
     * @param phraseLength Bars per phrase
     */
    float GetPhraseProgress(int patternLength, int phraseLength) const
    {
        int totalStepsInPhrase = patternLength * phraseLength;
        int currentStepInPhrase = currentBar * patternLength + currentStep;
        return static_cast<float>(currentStepInPhrase) / static_cast<float>(totalStepsInPhrase);
    }
};

} // namespace daisysp_idm_grids
