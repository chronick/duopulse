#include "PatternGenerator.h"
#include "PatternField.h"      // ComputeShapeBlendedWeights, ApplyAxisBias, GetMetricWeight, ClampWeight
#include "VoiceRelation.h"     // ApplyComplementRelationship
#include "VelocityCompute.h"   // ComputeAccentVelocity
#include "HashUtils.h"         // HashToFloat
#include "HitBudget.h"         // ComputeBarBudget, GetEnergyZone, GetMinSpacingForZone
#include "GumbelSampler.h"     // SelectHitsGumbelTopK
#include "GuardRails.h"        // ApplyHardGuardRails, SoftRepairPass
#include "EuclideanGen.h"      // BlendEuclideanWithWeights

#include <algorithm>  // for std::max, std::min

namespace daisysp_idm_grids
{

// =============================================================================
// Hit Count Computation
// =============================================================================

int ComputeTargetHits(float energy, int patternLength, Voice voice, float shape)
{
    EnergyZone zone = GetEnergyZone(energy);
    BarBudget budget;
    ComputeBarBudget(energy, 0.5f, zone, AuxDensity::NORMAL, patternLength, 1.0f, shape, budget);

    switch (voice)
    {
        case Voice::ANCHOR:  return budget.anchorHits;
        case Voice::SHIMMER: return budget.shimmerHits;
        case Voice::AUX:     return budget.auxHits;
        default:             return budget.anchorHits;
    }
}

// =============================================================================
// Rotation Utilities
// =============================================================================

uint32_t RotateWithPreserve(uint32_t mask, int rotation, int length, int preserveStep)
{
    if (rotation == 0 || length <= 1) return mask;

    // Check if preserve step is set
    bool preserveWasSet = (mask & (1U << preserveStep)) != 0;

    // Clear the preserve step before rotation
    mask &= ~(1U << preserveStep);

    // Rotate the remaining bits
    rotation = rotation % length;
    uint32_t lengthMask = (length >= 32) ? 0xFFFFFFFF : ((1U << length) - 1);
    mask = ((mask << rotation) | (mask >> (length - rotation))) & lengthMask;

    // Restore preserve step to its original position
    if (preserveWasSet) {
        mask |= (1U << preserveStep);
    }

    return mask;
}

// =============================================================================
// Pattern Generation
// =============================================================================

void GeneratePattern(const PatternParams& params, PatternResult& result)
{
    result.patternLength = params.patternLength;
    result.anchorMask = 0;
    result.shimmerMask = 0;
    result.auxMask = 0;

    EnergyZone zone = GetEnergyZone(params.energy);
    int minSpacing = GetMinSpacingForZone(zone);

    // Generate anchor weights
    float anchorWeights[kMaxSteps];
    ComputeShapeBlendedWeights(params.shape, params.energy, params.seed,
                               params.patternLength, anchorWeights,
                               params.patternFieldConfig);
    ApplyAxisBias(anchorWeights, params.axisX, params.axisY,
                  params.shape, params.seed, params.patternLength);

    // V5 Task 44: Add seed-based weight perturbation for anchor variation
    {
        const float noiseScale = 0.4f * (1.0f - params.shape);
        for (int step = 0; step < params.patternLength; ++step) {
            if (step == 0 && params.shape < 0.3f) continue;
            float noise = (HashToFloat(params.seed, step + 1000) - 0.5f) * noiseScale;
            anchorWeights[step] = ClampWeight(anchorWeights[step] + noise);
        }
    }

    // Generate shimmer weights (different seed)
    float shimmerWeights[kMaxSteps];
    ComputeShapeBlendedWeights(params.shape, params.energy, params.seed + 1,
                               params.patternLength, shimmerWeights,
                               params.patternFieldConfig);

    // Compute hit budget using all parameters
    BarBudget budget;
    ComputeBarBudget(
        params.energy, params.balance, zone, params.auxDensity,
        params.patternLength, params.densityMultiplier, params.shape, budget
    );

    // Apply fill boost if in fill zone
    if (params.inFillZone)
    {
        ApplyFillBoost(budget, params.fillIntensity,
                       params.fillDensityMultiplier, params.patternLength);
    }

    // Generate anchor hits with optional Euclidean blend
    if (params.euclideanRatio > 0.01f)
    {
        result.anchorMask = BlendEuclideanWithWeights(
            budget.anchorHits,
            params.patternLength,
            anchorWeights,
            budget.anchorEligibility,
            params.euclideanRatio,
            params.seed
        );
    }
    else
    {
        result.anchorMask = SelectHitsGumbelTopK(
            anchorWeights,
            budget.anchorEligibility,
            budget.anchorHits,
            params.seed,
            params.patternLength,
            minSpacing
        );
    }

    // Apply COMPLEMENT relationship for shimmer
    // Note: Shimmer uses COMPLEMENT (gap-filling) rather than independent Euclidean/Gumbel
    // This is the V5 design - shimmer fills gaps in anchor pattern
    result.shimmerMask = ApplyComplementRelationship(
        result.anchorMask, shimmerWeights, params.drift, params.seed ^ 0x12345678,
        params.patternLength, budget.shimmerHits
    );

    // Apply soft repair pass if enabled
    if (params.applySoftRepair)
    {
        SoftRepairPass(
            result.anchorMask, result.shimmerMask,
            anchorWeights, shimmerWeights,
            zone, params.patternLength
        );
    }

    // Apply guard rails
    ApplyHardGuardRails(result.anchorMask, result.shimmerMask, zone,
                        params.genre, params.patternLength);

    // V5 Task 44: Apply seed-based rotation for anchor variation (AFTER guard rails)
    if (params.shape < 0.7f) {
        int maxRotation = std::max(1, params.patternLength / 4);
        int rotation = static_cast<int>(HashToFloat(params.seed, 2000) * maxRotation);
        result.anchorMask = RotateWithPreserve(result.anchorMask, rotation,
                                                params.patternLength, 0);
    }

    // Generate aux hits
    float auxWeights[kMaxSteps];
    uint32_t combinedMask = result.anchorMask | result.shimmerMask;
    for (int i = 0; i < params.patternLength; ++i)
    {
        float metricW = GetMetricWeight(i, params.patternLength);
        auxWeights[i] = 1.0f - metricW * 0.5f;
        if ((combinedMask & (1U << i)) != 0)
            auxWeights[i] *= 0.3f;
    }
    result.auxMask = SelectHitsGumbelTopK(
        auxWeights,
        budget.auxEligibility,
        budget.auxHits,
        params.seed ^ 0x87654321,
        params.patternLength,
        0
    );

    // Apply aux voice relationship
    ApplyAuxRelationship(result.anchorMask, result.shimmerMask, result.auxMask,
                         params.voiceCoupling, params.patternLength);

    // Compute velocities
    for (int step = 0; step < params.patternLength; ++step)
    {
        if ((result.anchorMask & (1U << step)) != 0)
            result.anchorVelocity[step] = ComputeAccentVelocity(
                params.accent, step, params.patternLength, params.seed);
        if ((result.shimmerMask & (1U << step)) != 0)
            result.shimmerVelocity[step] = ComputeAccentVelocity(
                params.accent * 0.7f, step, params.patternLength, params.seed + 1);
        if ((result.auxMask & (1U << step)) != 0)
        {
            float baseVel = 0.5f + params.energy * 0.3f;
            float variation = (HashToFloat(params.seed + 4, step) - 0.5f) * 0.15f;
            result.auxVelocity[step] = std::max(0.3f, std::min(1.0f, baseVel + variation));
        }
    }
}

} // namespace daisysp_idm_grids
