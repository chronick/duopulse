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

uint64_t RotateWithPreserve(uint64_t mask, int rotation, int length, int preserveStep)
{
    if (rotation == 0 || length <= 1) return mask;

    // Check if preserve step is set
    bool preserveWasSet = (mask & (1ULL << preserveStep)) != 0;

    // Clear the preserve step before rotation
    mask &= ~(1ULL << preserveStep);

    // Rotate the remaining bits
    rotation = rotation % length;
    uint64_t lengthMask = (length >= 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << length) - 1);
    mask = ((mask << rotation) | (mask >> (length - rotation))) & lengthMask;

    // Restore preserve step to its original position
    if (preserveWasSet) {
        mask |= (1ULL << preserveStep);
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

    // Iteration 2026-01-20-008 fix: Dynamic hit count variance for wild zone
    // The fixed hit budget (K) forces gaps toward uniformity. Adding variance
    // to K creates irregular gap distributions for lower regularity.
    if (params.shape > 0.7f)
    {
        // Hash key includes SHAPE (quantized) so same seed + different SHAPE = different variance
        int shapeKey = static_cast<int>(params.shape * 100);  // Quantize to 0.01 precision
        float variance = (HashToFloat(params.seed, 999 + shapeKey) - 0.5f) * 4.0f;  // Range: -2 to +2
        int variedHits = budget.anchorHits + static_cast<int>(variance);

        // Clamp to valid range (at least 2 hits, at most 2/3 of pattern)
        int maxHits = (params.patternLength * 2) / 3;
        budget.anchorHits = std::max(2, std::min(variedHits, maxHits));
    }

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
            budget.anchorEligibility,
            zone, params.patternLength
        );
    }

    // Apply guard rails (eligibility-aware)
    ApplyHardGuardRails(result.anchorMask, result.shimmerMask,
                        budget.anchorEligibility, zone,
                        params.genre, params.patternLength);

    // V5 Task 44: Apply seed-based rotation for anchor variation (AFTER guard rails)
    // Iteration 2026-01-22-004: Extended rotation to syncopated zone
    // Weight-based selection alone couldn't overcome Gumbel seed determinism.
    // Rotation physically shifts hits off strong beats to create syncopation.
    bool inSyncopatedZone = params.shape >= 0.30f && params.shape < 0.70f;
    bool inWildZone = params.shape >= 0.70f;
    bool shouldRotate = (inSyncopatedZone || inWildZone) &&
                        zone != EnergyZone::MINIMAL;  // not MINIMAL by ENERGY

    if (shouldRotate) {
        int maxRotation;
        int minRotation;
        int hashKey;
        if (inWildZone) {
            // Wild zone: stronger rotation for chaotic displacement
            maxRotation = 4;
            minRotation = 0;  // Can be 0-3
            hashKey = 2000;
        } else {
            // Syncopated zone: guaranteed rotation for syncopation feel
            // Iteration 2026-01-22-004: minRotation=1 ensures hits shift off strong beats
            // Iteration 2026-01-23-001: increased to min=2 for stronger displacement
            maxRotation = 4;
            minRotation = 2;  // Guaranteed 2-3 step rotation
            hashKey = 3000;
        }
        int rotationRange = maxRotation - minRotation;
        int rotation = minRotation + static_cast<int>(HashToFloat(params.seed, hashKey) * rotationRange);
        if (rotation > 0) {
            result.anchorMask = RotateWithPreserve(result.anchorMask, rotation,
                                                    params.patternLength, 0);
        }
    }

    // Generate aux hits
    float auxWeights[kMaxSteps];
    uint64_t combinedMask = result.anchorMask | result.shimmerMask;
    for (int i = 0; i < params.patternLength; ++i)
    {
        float metricW = GetMetricWeight(i, params.patternLength);
        auxWeights[i] = 1.0f - metricW * 0.5f;
        if ((combinedMask & (1ULL << i)) != 0)
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
        if ((result.anchorMask & (1ULL << step)) != 0)
            result.anchorVelocity[step] = ComputeAccentVelocity(
                params.accent, step, params.patternLength, params.seed);
        if ((result.shimmerMask & (1ULL << step)) != 0)
            result.shimmerVelocity[step] = ComputeAccentVelocity(
                params.accent * 0.7f, step, params.patternLength, params.seed + 1);
        if ((result.auxMask & (1ULL << step)) != 0)
        {
            float baseVel = 0.5f + params.energy * 0.3f;
            float variation = (HashToFloat(params.seed + 4, step) - 0.5f) * 0.15f;
            result.auxVelocity[step] = std::max(0.3f, std::min(1.0f, baseVel + variation));
        }
    }
}

// =============================================================================
// Fill Pattern Generation
// =============================================================================

void GenerateFillPattern(const PatternParams& params, PatternResult& result)
{
    // Create a modified copy of params with fill boosts applied
    PatternParams fillParams = params;

    // Clamp fillProgress to valid range
    float fillProgress = std::max(0.0f, std::min(1.0f, params.fillProgress));

    // Enable fill zone
    fillParams.inFillZone = true;
    fillParams.fillIntensity = fillProgress;

    // Compute fill density multiplier (spec 9.2):
    // maxBoost = 0.6 + energy * 0.4
    // densityMultiplier = 1.0 + maxBoost * (fillProgress^2)
    float maxBoost = 0.6f + params.energy * 0.4f;
    float fillProgressSquared = fillProgress * fillProgress;
    fillParams.fillDensityMultiplier = 1.0f + maxBoost * fillProgressSquared;

    // Generate base pattern with fill modifiers
    GeneratePattern(fillParams, result);

    // Post-process velocities with fill-specific velocity boost (spec 9.2):
    // velocityBoost = 0.10 + 0.15 * fillProgress
    float velocityBoost = 0.10f + 0.15f * fillProgress;

    // Apply velocity boost to all active hits
    for (int step = 0; step < result.patternLength; ++step)
    {
        if ((result.anchorMask & (1ULL << step)) != 0)
        {
            result.anchorVelocity[step] = std::min(1.0f, result.anchorVelocity[step] + velocityBoost);
        }
        if ((result.shimmerMask & (1ULL << step)) != 0)
        {
            result.shimmerVelocity[step] = std::min(1.0f, result.shimmerVelocity[step] + velocityBoost);
        }
        if ((result.auxMask & (1ULL << step)) != 0)
        {
            result.auxVelocity[step] = std::min(1.0f, result.auxVelocity[step] + velocityBoost);
        }
    }

    // Force accents when fillProgress > 0.85 (spec 9.2)
    // This is done by boosting velocities to near-maximum for strong positions
    if (fillProgress > 0.85f)
    {
        const float forceAccentVelocity = 0.95f;
        for (int step = 0; step < result.patternLength; ++step)
        {
            // Force anchor accents on downbeats (steps 0, 4, 8, 12, etc.)
            if ((result.anchorMask & (1ULL << step)) != 0 && (step % 4 == 0))
            {
                result.anchorVelocity[step] = std::max(result.anchorVelocity[step], forceAccentVelocity);
            }
            // Force shimmer accents on all hits when fillProgress > 0.85
            if ((result.shimmerMask & (1ULL << step)) != 0)
            {
                result.shimmerVelocity[step] = std::max(result.shimmerVelocity[step], forceAccentVelocity * 0.9f);
            }
        }
    }
}

} // namespace daisysp_idm_grids
