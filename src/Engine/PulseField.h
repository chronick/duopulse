#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * DuoPulse v3: Weighted Pulse Field Algorithm
 *
 * Replaces discrete pattern lookup with a continuous algorithm where each
 * of the 32 steps has a weight representing its "likelihood" of triggering.
 * The algorithm uses these weights combined with DENSITY and BROKEN to
 * determine what fires.
 *
 * Core concept:
 *   DENSITY = "How much is happening"
 *   BROKEN  = "How weird is it"
 *
 * Weight tiers (based on musical importance):
 *   1.0  = Bar downbeats (steps 0, 16) - "THE ONE"
 *   0.85 = Half-note positions (steps 8, 24)
 *   0.7  = Quarter notes (steps 4, 12, 20, 28)
 *   0.4  = 8th note off-beats (steps 2, 6, 10, 14, 18, 22, 26, 30)
 *   0.2  = 16th note off-beats (odd steps)
 *
 * Reference: docs/specs/main.md section "Weighted Pulse Field Algorithm [pulse-field]"
 */

// Number of steps in the pattern
constexpr int kPulseFieldSteps = 32;

// Per-voice drift multipliers (Anchor more stable, Shimmer more drifty)
constexpr float kAnchorDriftMultiplier  = 0.7f;
constexpr float kShimmerDriftMultiplier = 1.3f;

/**
 * Base weight table for generic grid positions.
 * Weighted by musical importance in a standard 4/4 context.
 */
constexpr float kBaseWeights[kPulseFieldSteps] = {
    // Bar 1: Steps 0-15
    1.00f, 0.20f, 0.40f, 0.20f,  // 0-3:  DOWNBEAT, 16th, 8th, 16th
    0.70f, 0.20f, 0.40f, 0.20f,  // 4-7:  quarter, 16th, 8th, 16th
    0.85f, 0.20f, 0.40f, 0.20f,  // 8-11: half, 16th, 8th, 16th
    0.70f, 0.20f, 0.40f, 0.20f,  // 12-15: quarter, 16th, 8th, 16th

    // Bar 2: Steps 16-31
    1.00f, 0.20f, 0.40f, 0.20f,  // 16-19: DOWNBEAT, 16th, 8th, 16th
    0.70f, 0.20f, 0.40f, 0.20f,  // 20-23: quarter, 16th, 8th, 16th
    0.85f, 0.20f, 0.40f, 0.20f,  // 24-27: half, 16th, 8th, 16th
    0.70f, 0.20f, 0.40f, 0.20f,  // 28-31: quarter, 16th, 8th, 16th
};

/**
 * Anchor (Kick Character) weight profile.
 * Emphasizes downbeats and strong positions (0, 8, 16, 24).
 * Creates a solid foundation with occasional ghost hits.
 */
constexpr float kAnchorWeights[kPulseFieldSteps] = {
    // Bar 1: Steps 0-15
    1.00f, 0.15f, 0.30f, 0.15f,  // 0-3:  DOWNBEAT, ghost, 8th, ghost
    0.70f, 0.15f, 0.30f, 0.15f,  // 4-7:  quarter, ghost, 8th, ghost
    0.85f, 0.15f, 0.30f, 0.15f,  // 8-11: half, ghost, 8th, ghost
    0.70f, 0.15f, 0.30f, 0.20f,  // 12-15: quarter, ghost, 8th, ghost+

    // Bar 2: Steps 16-31
    1.00f, 0.15f, 0.30f, 0.15f,  // 16-19: DOWNBEAT
    0.70f, 0.15f, 0.30f, 0.15f,  // 20-23: quarter
    0.85f, 0.15f, 0.30f, 0.15f,  // 24-27: half
    0.70f, 0.15f, 0.35f, 0.25f,  // 28-31: quarter (slight fill zone boost)
};

/**
 * Shimmer (Snare Character) weight profile.
 * Emphasizes backbeats (steps 8 and 24) with more activity on off-beats.
 * Provides contrast and syncopation to Anchor.
 */
constexpr float kShimmerWeights[kPulseFieldSteps] = {
    // Bar 1: Steps 0-15
    0.25f, 0.15f, 0.35f, 0.15f,  // 0-3:  low downbeat, ghost, 8th, ghost
    0.60f, 0.15f, 0.35f, 0.20f,  // 4-7:  quarter (pre-snare), ghost, 8th, ghost
    1.00f, 0.15f, 0.35f, 0.15f,  // 8-11: BACKBEAT (snare!), ghost, 8th, ghost
    0.60f, 0.15f, 0.35f, 0.20f,  // 12-15: quarter, ghost, 8th, ghost

    // Bar 2: Steps 16-31
    0.25f, 0.15f, 0.35f, 0.15f,  // 16-19: low downbeat
    0.60f, 0.15f, 0.35f, 0.20f,  // 20-23: quarter (pre-snare)
    1.00f, 0.15f, 0.35f, 0.15f,  // 24-27: BACKBEAT (snare!)
    0.60f, 0.15f, 0.40f, 0.30f,  // 28-31: quarter (fill zone boost)
};

/**
 * Step stability values for DRIFT system.
 * Determines which steps lock first as DRIFT decreases.
 * Higher stability = stays locked at higher DRIFT values.
 */
constexpr float kStepStability[kPulseFieldSteps] = {
    // Bar 1: Steps 0-15
    1.00f, 0.20f, 0.40f, 0.20f,  // 0-3:  bar downbeat (most stable)
    0.70f, 0.20f, 0.40f, 0.20f,  // 4-7:  quarter note
    0.85f, 0.20f, 0.40f, 0.20f,  // 8-11: half note
    0.70f, 0.20f, 0.40f, 0.20f,  // 12-15: quarter note

    // Bar 2: Steps 16-31
    1.00f, 0.20f, 0.40f, 0.20f,  // 16-19: bar downbeat (most stable)
    0.70f, 0.20f, 0.40f, 0.20f,  // 20-23: quarter note
    0.85f, 0.20f, 0.40f, 0.20f,  // 24-27: half note
    0.70f, 0.20f, 0.40f, 0.20f,  // 28-31: quarter note
};

/**
 * Get the weight for a specific step position.
 *
 * @param step Step index (0-31)
 * @param isAnchor True for Anchor voice, false for Shimmer
 * @return Weight value (0.0-1.0)
 */
inline float GetStepWeight(int step, bool isAnchor)
{
    if(step < 0 || step >= kPulseFieldSteps)
        step = 0;

    return isAnchor ? kAnchorWeights[step] : kShimmerWeights[step];
}

/**
 * Get the stability value for a specific step.
 * Used by DRIFT system to determine lock threshold.
 *
 * @param step Step index (0-31)
 * @return Stability value (0.0-1.0)
 */
inline float GetStepStability(int step)
{
    if(step < 0 || step >= kPulseFieldSteps)
        step = 0;

    return kStepStability[step];
}

/**
 * Get the effective DRIFT for a voice.
 * Anchor is more stable (0.7× multiplier), Shimmer is more drifty (1.3×).
 *
 * @param drift Base DRIFT parameter (0-1)
 * @param isAnchor True for Anchor voice, false for Shimmer
 * @return Effective DRIFT (0-1, clamped)
 */
inline float GetEffectiveDrift(float drift, bool isAnchor)
{
    float multiplier = isAnchor ? kAnchorDriftMultiplier : kShimmerDriftMultiplier;
    float effective  = drift * multiplier;

    // Clamp to 0-1
    if(effective < 0.0f)
        effective = 0.0f;
    if(effective > 1.0f)
        effective = 1.0f;

    return effective;
}

// =============================================================================
// Deterministic Random Number Generation
// =============================================================================

/**
 * Hash a seed and step index to produce a deterministic pseudo-random value.
 * Uses a simple but effective hash combining (similar to boost::hash_combine).
 *
 * @param seed Base seed for randomness
 * @param step Step index to hash
 * @return Hashed value
 */
inline uint32_t HashStep(uint32_t seed, int step)
{
    // Simple hash combining using golden ratio prime
    uint32_t h = seed;
    h ^= static_cast<uint32_t>(step) + 0x9e3779b9 + (h << 6) + (h >> 2);
    // Additional mixing
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

/**
 * Convert a hash value to a float in range [0, 1).
 *
 * @param hash Hash value from HashStep
 * @return Float in range [0, 1)
 */
inline float HashToFloat(uint32_t hash)
{
    // Use upper bits for better distribution
    return static_cast<float>(hash >> 8) / static_cast<float>(0x00FFFFFF);
}

/**
 * Linear interpolation between two values.
 *
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor (0-1)
 * @return Interpolated value
 */
inline float Lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

/**
 * Clamp a value to a range.
 *
 * @param value Value to clamp
 * @param minVal Minimum value
 * @param maxVal Maximum value
 * @return Clamped value
 */
inline float Clamp(float value, float minVal, float maxVal)
{
    if(value < minVal)
        return minVal;
    if(value > maxVal)
        return maxVal;
    return value;
}

// =============================================================================
// Core Pulse Field Algorithm
// =============================================================================

/**
 * Determine whether a step should fire based on the weighted pulse field algorithm.
 *
 * The algorithm:
 * 1. Get base weight for the step position from the weight table
 * 2. BROKEN flattens weight distribution (Lerp toward 0.5)
 * 3. Add noise scaled by BROKEN (±0.2 at max)
 * 4. DENSITY sets threshold: density=0 → threshold=1.0, density=1 → threshold=0.0
 * 5. Fire if effectiveWeight > threshold
 *
 * At BROKEN=0: Pattern follows weight table strictly (regular/predictable)
 * At BROKEN=1: All weights converge to ~0.5 with noise (chaotic/random)
 *
 * @param step Step index (0-31)
 * @param density How much is happening (0=silent, 1=full)
 * @param broken How irregular the pattern is (0=straight, 1=chaos)
 * @param weights Weight table to use (kAnchorWeights or kShimmerWeights)
 * @param seed Seed for deterministic randomness
 * @return true if step should fire
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [pulse-field]
 */
inline bool ShouldStepFire(int step,
                           float density,
                           float broken,
                           const float* weights,
                           uint32_t seed)
{
    // 1. Get base weight for this step position
    if(step < 0 || step >= kPulseFieldSteps)
        step = 0;
    float baseWeight = weights[step];

    // 2. BROKEN flattens the weight distribution
    //    At broken=0: full differentiation (downbeats dominate)
    //    At broken=1: weights converge to 0.5 (equal probability)
    float effectiveWeight = Lerp(baseWeight, 0.5f, broken);

    // 3. Add randomness scaled by BROKEN
    //    More broken = more random variation in weights
    //    Noise range: ±(broken * 0.2) = ±0.2 at max broken
    if(broken > 0.0f)
    {
        uint32_t hash = HashStep(seed, step);
        float noise   = (HashToFloat(hash) - 0.5f) * broken * 0.4f;
        effectiveWeight += noise;
        effectiveWeight = Clamp(effectiveWeight, 0.0f, 1.0f);
    }

    // 4. DENSITY sets the threshold
    //    density=0 → threshold=1.0 (nothing fires)
    //    density=1 → threshold=0.0 (everything fires)
    float threshold = 1.0f - density;

    // 5. Fire if weight exceeds threshold
    return effectiveWeight > threshold;
}

/**
 * Convenience wrapper for ShouldStepFire that selects the weight table
 * based on voice type.
 *
 * @param step Step index (0-31)
 * @param density How much is happening (0=silent, 1=full)
 * @param broken How irregular the pattern is (0=straight, 1=chaos)
 * @param isAnchor true for Anchor voice, false for Shimmer
 * @param seed Seed for deterministic randomness
 * @return true if step should fire
 */
inline bool ShouldStepFire(int step,
                           float density,
                           float broken,
                           bool isAnchor,
                           uint32_t seed)
{
    const float* weights = isAnchor ? kAnchorWeights : kShimmerWeights;
    return ShouldStepFire(step, density, broken, weights, seed);
}

// =============================================================================
// DRIFT System: Dual-Seed Locked/Drifting Pattern Generation
// =============================================================================

/**
 * State for the Pulse Field algorithm.
 * Maintains dual seeds for locked vs. drifting pattern elements.
 *
 * The DRIFT system determines how much the pattern evolves over time:
 * - patternSeed_: Fixed seed for "locked" elements (same every loop)
 * - loopSeed_: Changes each phrase for "drifting" elements (varies each loop)
 *
 * DRIFT sets the threshold: Steps with stability > DRIFT use patternSeed_,
 * steps with stability <= DRIFT use loopSeed_.
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [drift-control]
 */
struct PulseFieldState
{
    uint32_t patternSeed_;  // Fixed seed for locked elements (persists across loops)
    uint32_t loopSeed_;     // Varying seed for drifting elements (changes each phrase)
    uint32_t seedCounter_;  // Counter for generating new loopSeeds

    /**
     * Initialize the pulse field state.
     *
     * @param initialSeed Initial seed value for pattern generation
     */
    void Init(uint32_t initialSeed = 0x12345678)
    {
        patternSeed_ = initialSeed;
        loopSeed_    = initialSeed ^ 0xDEADBEEF;
        seedCounter_ = 0;
    }

    /**
     * Called on phrase reset to regenerate loopSeed_.
     * This causes drifting elements to produce different patterns each loop.
     */
    void OnPhraseReset()
    {
        // Generate a new loopSeed using seedCounter for variation
        seedCounter_++;
        loopSeed_ = HashStep(patternSeed_, static_cast<int>(seedCounter_));
    }

    /**
     * Lock the current pattern by copying loopSeed_ to patternSeed_.
     * This "freezes" the current drifting pattern.
     */
    void LockPattern()
    {
        patternSeed_ = loopSeed_;
    }

    /**
     * Set a new pattern seed (for loading saved patterns or user selection).
     *
     * @param seed New pattern seed
     */
    void SetPatternSeed(uint32_t seed)
    {
        patternSeed_ = seed;
    }
};

/**
 * Determine whether a step should fire with DRIFT-aware seed selection.
 *
 * This is the full DRIFT-aware algorithm:
 * 1. Calculate effective DRIFT based on voice (Anchor is more stable)
 * 2. Get step stability (downbeats are most stable)
 * 3. If stability > effectiveDrift, use patternSeed_ (locked)
 * 4. If stability <= effectiveDrift, use loopSeed_ (drifting)
 * 5. Apply the core pulse field algorithm with the selected seed
 *
 * At DRIFT=0%: All steps use patternSeed_ → identical pattern every loop
 * At DRIFT=100%: Most steps use loopSeed_ → unique pattern each loop
 * Per-voice: Anchor uses 0.7× drift multiplier, Shimmer uses 1.3×
 *
 * @param step Step index (0-31)
 * @param density How much is happening (0=silent, 1=full)
 * @param broken How irregular the pattern is (0=straight, 1=chaos)
 * @param drift How much the pattern evolves (0=locked, 1=evolving)
 * @param isAnchor true for Anchor voice (more stable), false for Shimmer (more drifty)
 * @param state Pulse field state with dual seeds
 * @return true if step should fire
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [drift-control]
 */
inline bool ShouldStepFireWithDrift(int step,
                                    float density,
                                    float broken,
                                    float drift,
                                    bool isAnchor,
                                    const PulseFieldState& state)
{
    // Get effective DRIFT with per-voice multiplier
    // Anchor (kick) is more stable (0.7× multiplier)
    // Shimmer (snare/hat) is more drifty (1.3× multiplier)
    float effectiveDrift = GetEffectiveDrift(drift, isAnchor);

    // Get step's stability tier
    float stability = GetStepStability(step);

    // Is this step locked or can it drift?
    // Steps with stability ABOVE effectiveDrift use the locked seed
    // Steps with stability AT OR BELOW effectiveDrift use the varying seed
    bool isLocked = stability > effectiveDrift;

    // Pick the appropriate random seed
    uint32_t seed = isLocked ? state.patternSeed_ : state.loopSeed_;

    // Get the weight table for this voice
    const float* weights = isAnchor ? kAnchorWeights : kShimmerWeights;

    // Apply the core pulse field algorithm with the selected seed
    return ShouldStepFire(step, density, broken, weights, seed);
}

/**
 * Get trigger results for both voices with full DRIFT awareness.
 *
 * @param step Step index (0-31)
 * @param anchorDensity Density for Anchor voice
 * @param shimmerDensity Density for Shimmer voice
 * @param broken BROKEN parameter (pattern regularity)
 * @param drift DRIFT parameter (pattern evolution)
 * @param state Pulse field state
 * @param anchorFires [out] Whether Anchor should trigger
 * @param shimmerFires [out] Whether Shimmer should trigger
 */
inline void GetPulseFieldTriggers(int step,
                                  float anchorDensity,
                                  float shimmerDensity,
                                  float broken,
                                  float drift,
                                  const PulseFieldState& state,
                                  bool& anchorFires,
                                  bool& shimmerFires)
{
    anchorFires  = ShouldStepFireWithDrift(step, anchorDensity, broken, drift, true, state);
    shimmerFires = ShouldStepFireWithDrift(step, shimmerDensity, broken, drift, false, state);
}

} // namespace daisysp_idm_grids
