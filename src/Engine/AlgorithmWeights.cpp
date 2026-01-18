/**
 * Algorithm Weights Implementation
 *
 * Computes weight-based blending of pattern generation algorithms
 * using configurable curves from AlgorithmConfig.
 */

#include "AlgorithmWeights.h"
#include "../../inc/algorithm_config.h"
#include "HashUtils.h"

#include <cmath>
#include <algorithm>

namespace daisysp_idm_grids {

// =============================================================================
// Math Utilities
// =============================================================================

float Smoothstep(float edge0, float edge1, float x)
{
    // Clamp to [0, 1]
    float t = (x - edge0) / (edge1 - edge0);
    t = std::max(0.0f, std::min(1.0f, t));

    // Hermite interpolation: 3t^2 - 2t^3
    return t * t * (3.0f - 2.0f * t);
}

float BellCurve(float x, float center, float width)
{
    // Gaussian: exp(-0.5 * ((x - center) / width)^2)
    float d = (x - center) / width;
    return std::exp(-0.5f * d * d);
}

// =============================================================================
// Weight Computation
// =============================================================================

AlgorithmWeights ComputeAlgorithmWeights(float shape)
{
    using namespace AlgorithmConfig;

    AlgorithmWeights weights;

    // Euclidean: high at low SHAPE, fades via smoothstep
    // 1.0 - smoothstep means: 1.0 below fadeStart, 0.0 above fadeEnd
    weights.euclidean = 1.0f - Smoothstep(kEuclideanFadeStart, kEuclideanFadeEnd, shape);

    // Syncopation: bell curve centered in middle
    weights.syncopation = BellCurve(shape, kSyncopationCenter, kSyncopationWidth);

    // Random: grows at high SHAPE via smoothstep
    weights.random = Smoothstep(kRandomFadeStart, kRandomFadeEnd, shape);

    // Normalize to sum to 1.0
    float total = weights.euclidean + weights.syncopation + weights.random;
    if (total > 0.001f) {
        weights.euclidean /= total;
        weights.syncopation /= total;
        weights.random /= total;
    } else {
        // Fallback: pure syncopation if all weights near zero
        weights.euclidean = 0.0f;
        weights.syncopation = 1.0f;
        weights.random = 0.0f;
    }

    return weights;
}

ChannelEuclideanParams ComputeChannelEuclidean(float energy, uint32_t seed, int patternLength)
{
    using namespace AlgorithmConfig;

    ChannelEuclideanParams params;

    // Linear interpolation within each voice's k range
    // k = kMin + energy * (kMax - kMin)
    params.anchorK = kAnchorKMin + static_cast<int>(energy * (kAnchorKMax - kAnchorKMin));
    params.shimmerK = kShimmerKMin + static_cast<int>(energy * (kShimmerKMax - kShimmerKMin));
    params.auxK = kAuxKMin + static_cast<int>(energy * (kAuxKMax - kAuxKMin));

    // Clamp to valid ranges (k must be <= pattern length)
    params.anchorK = std::min(params.anchorK, patternLength);
    params.shimmerK = std::min(params.shimmerK, patternLength);
    params.auxK = std::min(params.auxK, patternLength);

    // Seed-derived rotation for euclidean pattern variation
    // Use hash to get deterministic but varied rotation
    params.rotation = static_cast<int>(HashToFloat(seed, 3000) * (patternLength / 4));

    return params;
}

AlgorithmWeightsDebug ComputeAlgorithmWeightsDebug(float shape, float energy,
                                                   uint32_t seed, int patternLength)
{
    using namespace AlgorithmConfig;

    AlgorithmWeightsDebug debug;

    // Store inputs
    debug.shape = shape;
    debug.energy = energy;

    // Store config values for verification
    debug.euclideanFadeStart = kEuclideanFadeStart;
    debug.euclideanFadeEnd = kEuclideanFadeEnd;
    debug.syncopationCenter = kSyncopationCenter;
    debug.syncopationWidth = kSyncopationWidth;
    debug.randomFadeStart = kRandomFadeStart;
    debug.randomFadeEnd = kRandomFadeEnd;

    // Compute raw (unnormalized) weights
    debug.rawEuclidean = 1.0f - Smoothstep(kEuclideanFadeStart, kEuclideanFadeEnd, shape);
    debug.rawSyncopation = BellCurve(shape, kSyncopationCenter, kSyncopationWidth);
    debug.rawRandom = Smoothstep(kRandomFadeStart, kRandomFadeEnd, shape);

    // Compute normalized weights
    debug.weights = ComputeAlgorithmWeights(shape);

    // Compute per-channel euclidean params
    debug.channelParams = ComputeChannelEuclidean(energy, seed, patternLength);

    return debug;
}

} // namespace daisysp_idm_grids
