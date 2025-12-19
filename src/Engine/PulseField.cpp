#include "PulseField.h"

namespace daisysp_idm_grids
{

// =============================================================================
// Core Pulse Field Algorithm
// =============================================================================

bool ShouldStepFire(int step,
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

bool ShouldStepFire(int step,
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

bool ShouldStepFireWithDrift(int step,
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

void GetPulseFieldTriggers(int step,
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
