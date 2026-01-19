#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"
#include "SequencerState.h"

namespace daisysp_idm_grids
{

/**
 * DriftControl: Pattern evolution control system
 *
 * DRIFT controls how patterns evolve over time using dual seed management:
 * - patternSeed: Fixed per "song", changes only on reseed (defines character)
 * - phraseSeed: Changes each phrase, derived from patternSeed + counter (adds variation)
 *
 * The DRIFT parameter (0.0-1.0) interpolates between these seeds:
 * - DRIFT=0: 100% locked (same pattern every phrase)
 * - DRIFT=1: 100% evolving (new pattern each phrase)
 *
 * Step stability determines which seed is used for each step:
 * - Downbeats (step 0) have high stability (1.0) - use locked seed longer
 * - Off-beats have low stability (0.1-0.3) - evolve first as DRIFT increases
 *
 * Reference: docs/specs/main.md section 6.7
 */

// =============================================================================
// Constants
// =============================================================================

/// Stability values for different metric positions
constexpr float kStabilityDownbeat = 1.0f;       ///< Beat 1 (step 0) - most stable
constexpr float kStabilityHalfBar = 0.9f;        ///< Half bar (step 16) - very stable
constexpr float kStabilityQuarter = 0.7f;        ///< Quarter notes (steps 8, 24)
constexpr float kStabilityEighth = 0.5f;         ///< Eighth notes (steps 4, 12, 20, 28)
constexpr float kStabilitySixteenth = 0.3f;      ///< Strong sixteenths (even steps)
constexpr float kStabilityOffbeat = 0.1f;        ///< Weak positions (odd steps)

/// Default initial seed for new patterns
constexpr uint32_t kDefaultPatternSeed = 0x12345678;

/// XOR constant for phrase seed derivation
constexpr uint32_t kPhraseSeedXor = 0xDEADBEEF;

// =============================================================================
// Step Stability Functions
// =============================================================================

/**
 * Get the stability value for a step based on its metric position
 *
 * Stability determines how resistant a step is to pattern evolution.
 * High stability = uses locked seed longer (stays consistent across phrases)
 * Low stability = uses evolving seed sooner (changes first as DRIFT increases)
 *
 * The hierarchy (for 32-step patterns):
 * - 1.0: Downbeat (step 0)
 * - 0.9: Half-bar (step 16)
 * - 0.7: Quarter notes (steps 8, 24)
 * - 0.5: Eighth notes (steps 4, 12, 20, 28)
 * - 0.3: Strong sixteenths (other even steps)
 * - 0.1: Off-beats (odd steps)
 *
 * @param step Step index (0 to patternLength-1)
 * @param patternLength Total steps in pattern
 * @return Stability value (0.0-1.0)
 */
float GetStepStability(int step, int patternLength);

/**
 * Get the stability mask for a pattern (high bits = high stability steps)
 *
 * This is useful for visualizing which steps will lock first as DRIFT
 * is reduced.
 *
 * @param patternLength Total steps in pattern
 * @param stabilityThreshold Only include steps with stability >= this
 * @return Bitmask of stable steps
 */
uint64_t GetStabilityMask(int patternLength, float stabilityThreshold);

// =============================================================================
// Seed Selection Functions
// =============================================================================

/**
 * Select the appropriate seed for a step based on DRIFT and step stability
 *
 * This is the core DRIFT algorithm:
 * - If step stability > DRIFT: use locked patternSeed (consistent)
 * - If step stability <= DRIFT: use evolving phraseSeed (varies)
 *
 * @param state Current drift state (contains both seeds)
 * @param drift DRIFT parameter (0.0-1.0)
 * @param step Step index
 * @param patternLength Pattern length
 * @return Seed to use for this step's generation
 */
uint32_t SelectSeed(const DriftState& state, float drift, int step, int patternLength);

/**
 * Select seed using pre-computed stability value
 *
 * Use this if you've already computed step stability.
 *
 * @param state Current drift state
 * @param drift DRIFT parameter
 * @param stepStability Pre-computed stability for the step
 * @return Seed to use
 */
uint32_t SelectSeedWithStability(const DriftState& state, float drift, float stepStability);

// =============================================================================
// Phrase and Reseed Functions
// =============================================================================

/**
 * Called at the end of each phrase to update phrase seed
 *
 * This should be called at phrase boundaries. It:
 * - Processes any pending reseed request
 * - Generates a new phrase seed from patternSeed + counter
 *
 * @param state Drift state to update (modified in place)
 */
void OnPhraseEnd(DriftState& state);

/**
 * Request a reseed that will take effect at the next phrase boundary
 *
 * This is a "soft" reseed - it queues the reseed for the next phrase
 * to avoid abrupt pattern changes mid-phrase.
 *
 * @param state Drift state to mark for reseed
 */
void RequestReseed(DriftState& state);

/**
 * Immediately reseed the pattern (hard reseed)
 *
 * This forces an immediate pattern change. Use sparingly.
 * Call OnPhraseEnd() after this if you want the phrase seed updated too.
 *
 * @param state Drift state to reseed
 * @param newSeed Optional new seed (0 = generate from current state)
 */
void Reseed(DriftState& state, uint32_t newSeed = 0);

/**
 * Generate a new random-ish seed based on current state
 *
 * Uses a mixing function to generate unpredictable new seeds.
 *
 * @param currentSeed Current seed to mix
 * @param counter Counter value to mix in
 * @return New seed value
 */
uint32_t GenerateNewSeed(uint32_t currentSeed, uint32_t counter);

// =============================================================================
// Initialization
// =============================================================================

/**
 * Initialize a DriftState with a given seed
 *
 * @param state Drift state to initialize
 * @param seed Initial pattern seed (0 = use default)
 */
void InitDriftState(DriftState& state, uint32_t seed = kDefaultPatternSeed);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Get the "locked ratio" for a given DRIFT value
 *
 * This tells you what fraction of steps will use the locked seed.
 *
 * @param drift DRIFT parameter (0.0-1.0)
 * @param patternLength Pattern length
 * @return Fraction of steps using locked seed (0.0-1.0)
 */
float GetLockedRatio(float drift, int patternLength);

/**
 * Check if a step will be locked (use pattern seed) at a given DRIFT value
 *
 * @param step Step index
 * @param patternLength Pattern length
 * @param drift DRIFT parameter
 * @return true if step uses locked seed
 */
bool IsStepLocked(int step, int patternLength, float drift);

/**
 * Hash combine helper for seed generation
 *
 * @param seed Base seed
 * @param value Value to combine
 * @return Combined seed
 */
uint32_t HashCombine(uint32_t seed, uint32_t value);

} // namespace daisysp_idm_grids
