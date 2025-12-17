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

} // namespace daisysp_idm_grids
