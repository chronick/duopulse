/**
 * Algorithm Weights: Weight-based blending of pattern generation methods
 *
 * This module provides explicit weight calculation for blending between
 * euclidean, syncopation, and random pattern generation algorithms.
 * The weights are computed from SHAPE parameter using configurable curves.
 *
 * Key features:
 * - Explicit, normalized weights for each algorithm contribution
 * - Smooth transitions via smoothstep and bell curve functions
 * - Per-channel euclidean k parameter calculation
 * - All weights sum to 1.0 (normalized)
 *
 * Reference: docs/tasks/active/56-weight-based-blending.md
 */

#pragma once

#include <cstdint>

namespace daisysp_idm_grids {

// =============================================================================
// Algorithm Weights Structure
// =============================================================================

/**
 * Computed weights for algorithm blending
 *
 * All weights are normalized to sum to 1.0 for consistent blending.
 * These weights determine the contribution of each generation method
 * to the final pattern output.
 */
struct AlgorithmWeights {
    float euclidean;     ///< Euclidean pattern contribution (0.0-1.0)
    float syncopation;   ///< Syncopation overlay contribution (0.0-1.0)
    float random;        ///< Random perturbation contribution (0.0-1.0)

    /// Total should always equal 1.0 after normalization
    float total() const { return euclidean + syncopation + random; }
};

/**
 * Per-channel euclidean parameters
 *
 * Each voice has different k (hit count) ranges for euclidean patterns.
 * k scales with ENERGY parameter within the specified range.
 */
struct ChannelEuclideanParams {
    int anchorK;    ///< Anchor voice euclidean k value
    int shimmerK;   ///< Shimmer voice euclidean k value
    int auxK;       ///< Aux voice euclidean k value
    int rotation;   ///< Seed-derived rotation offset
};

// =============================================================================
// Math Utilities
// =============================================================================

/**
 * Hermite smoothstep interpolation
 *
 * Returns smooth transition from 0 to 1 as x moves from edge0 to edge1.
 * Provides C1-continuous transitions (smooth derivative).
 *
 * Formula: 3t^2 - 2t^3 where t = (x - edge0) / (edge1 - edge0)
 *
 * @param edge0 Lower bound (returns 0 below this)
 * @param edge1 Upper bound (returns 1 above this)
 * @param x Input value
 * @return Smoothly interpolated value in [0, 1]
 *
 * Example:
 *   smoothstep(0.3, 0.7, 0.3) = 0.0  // at edge0
 *   smoothstep(0.3, 0.7, 0.5) = 0.5  // midpoint
 *   smoothstep(0.3, 0.7, 0.7) = 1.0  // at edge1
 */
float Smoothstep(float edge0, float edge1, float x);

/**
 * Gaussian bell curve
 *
 * Returns value on a bell curve centered at 'center' with given width.
 * Peak value is 1.0 at center, falling off symmetrically.
 *
 * Formula: exp(-0.5 * ((x - center) / width)^2)
 *
 * @param x Input value
 * @param center Peak position of bell curve
 * @param width Standard deviation (controls curve spread)
 * @return Value in (0, 1] with peak at center
 *
 * Example:
 *   BellCurve(0.5, 0.5, 0.3) = 1.0    // at center
 *   BellCurve(0.2, 0.5, 0.3) ≈ 0.32   // 1σ away from center
 *   BellCurve(0.0, 0.5, 0.3) ≈ 0.05   // far from center
 */
float BellCurve(float x, float center, float width);

// =============================================================================
// Weight Computation
// =============================================================================

/**
 * Compute algorithm blend weights from SHAPE parameter
 *
 * Uses configurable curves from AlgorithmConfig to determine how much
 * each algorithm contributes at the given SHAPE value:
 *
 * - Euclidean: Full strength at low SHAPE, fades via smoothstep
 * - Syncopation: Bell curve centered in middle SHAPE range
 * - Random: Fades in via smoothstep at high SHAPE
 *
 * Weights are automatically normalized to sum to 1.0.
 *
 * @param shape SHAPE parameter (0.0-1.0)
 * @return Normalized algorithm weights
 *
 * Example (default config):
 *   SHAPE=0.0: {euclidean=1.0, syncopation=0.0, random=0.0}
 *   SHAPE=0.5: {euclidean≈0.35, syncopation≈0.55, random≈0.10}
 *   SHAPE=1.0: {euclidean=0.0, syncopation≈0.10, random≈0.90}
 */
AlgorithmWeights ComputeAlgorithmWeights(float shape);

/**
 * Compute per-channel euclidean k parameters from ENERGY
 *
 * Each voice has a different k range (from AlgorithmConfig):
 * - Anchor: sparse, foundational (kAnchorKMin to kAnchorKMax)
 * - Shimmer: more active (kShimmerKMin to kShimmerKMax)
 * - Aux: variable (kAuxKMin to kAuxKMax)
 *
 * k scales linearly with ENERGY within each voice's range.
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param seed Pattern seed for rotation offset
 * @param patternLength Pattern length (16/32)
 * @return Per-channel euclidean parameters
 *
 * Example:
 *   ENERGY=0.0: {anchorK=4, shimmerK=6, auxK=2}
 *   ENERGY=0.5: {anchorK=8, shimmerK=11, auxK=5}
 *   ENERGY=1.0: {anchorK=12, shimmerK=16, auxK=8}
 */
ChannelEuclideanParams ComputeChannelEuclidean(float energy, uint32_t seed, int patternLength);

/**
 * Debug output structure for visualization
 *
 * Contains all intermediate computation results for debugging
 * and the --debug-weights flag in pattern_viz.
 */
struct AlgorithmWeightsDebug {
    // Input
    float shape;
    float energy;

    // Raw (unnormalized) weights
    float rawEuclidean;
    float rawSyncopation;
    float rawRandom;

    // Normalized weights
    AlgorithmWeights weights;

    // Per-channel euclidean
    ChannelEuclideanParams channelParams;

    // Config values used (for verification)
    float euclideanFadeStart;
    float euclideanFadeEnd;
    float syncopationCenter;
    float syncopationWidth;
    float randomFadeStart;
    float randomFadeEnd;
};

/**
 * Compute algorithm weights with full debug information
 *
 * Same as ComputeAlgorithmWeights but returns all intermediate values
 * for debugging and visualization purposes.
 *
 * @param shape SHAPE parameter (0.0-1.0)
 * @param energy ENERGY parameter (0.0-1.0)
 * @param seed Pattern seed
 * @param patternLength Pattern length (16/32)
 * @return Debug structure with all computation details
 */
AlgorithmWeightsDebug ComputeAlgorithmWeightsDebug(float shape, float energy,
                                                   uint32_t seed, int patternLength);

} // namespace daisysp_idm_grids
