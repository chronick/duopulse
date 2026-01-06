#include "HitBudget.h"
#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Utility Functions
// =============================================================================

int CountBits(uint32_t mask)
{
    // Brian Kernighan's algorithm
    int count = 0;
    while (mask)
    {
        mask &= (mask - 1);
        count++;
    }
    return count;
}

int ClampPatternLength(int patternLength)
{
    // For bitmask operations, we work with at most 32 steps
    // Longer patterns use two bars
    return std::min(patternLength, 32);
}

float GetShapeBudgetMultiplier(float shape)
{
    // Clamp shape to valid range
    shape = std::max(0.0f, std::min(1.0f, shape));

    if (shape < 0.33f)
    {
        // Stable zone: sparse patterns (0.75-0.85x)
        return 0.75f + (shape / 0.33f) * 0.10f;
    }
    else if (shape < 0.66f)
    {
        // Syncopated zone: normal density (1.0x)
        return 1.0f;
    }
    else
    {
        // Wild zone: denser patterns (1.15-1.25x)
        float wildProgress = (shape - 0.66f) / 0.34f;
        return 1.15f + wildProgress * 0.10f;
    }
}

// =============================================================================
// Budget Computation
// =============================================================================

int ComputeAnchorBudget(float energy, EnergyZone zone, int patternLength, float shape)
{
    // Clamp energy to valid range
    energy = std::max(0.0f, std::min(1.0f, energy));

    // Get shape multiplier for density adjustment (Task 39)
    float shapeMult = GetShapeBudgetMultiplier(shape);

    // Base hits scale with pattern length
    // For 32 steps: MINIMAL=1-2, GROOVE=3-5, BUILD=5-8, PEAK=8-12
    int maxHits = patternLength / 3;  // Max = 8th note density (expanded from /4)

    int minHits;
    int typicalHits;

    switch (zone)
    {
        case EnergyZone::MINIMAL:
            minHits = 1;
            typicalHits = std::max(1, patternLength / 16);  // Very sparse
            break;

        case EnergyZone::GROOVE:
            minHits = 3;  // Raised from 2
            typicalHits = patternLength / 6;  // Raised from /8
            break;

        case EnergyZone::BUILD:
            minHits = 4;  // Raised from 3
            typicalHits = patternLength / 4;  // Raised from /6
            break;

        case EnergyZone::PEAK:
            minHits = 6;  // Raised from 4
            typicalHits = patternLength / 3;  // Raised from /4
            break;

        default:
            minHits = 2;
            typicalHits = patternLength / 8;
            break;
    }

    // Apply SHAPE multiplier to base hits (Task 39)
    typicalHits = static_cast<int>(typicalHits * shapeMult + 0.5f);
    minHits = std::max(1, static_cast<int>(minHits * shapeMult + 0.5f));

    // Scale within zone range based on energy
    // Energy position within zone affects density
    float zoneProgress = 0.0f;
    switch (zone)
    {
        case EnergyZone::MINIMAL:
            zoneProgress = energy / 0.20f;
            break;
        case EnergyZone::GROOVE:
            zoneProgress = (energy - 0.20f) / 0.30f;
            break;
        case EnergyZone::BUILD:
            zoneProgress = (energy - 0.50f) / 0.25f;
            break;
        case EnergyZone::PEAK:
            zoneProgress = (energy - 0.75f) / 0.25f;
            break;
        default:
            zoneProgress = 0.5f;
            break;
    }
    zoneProgress = std::max(0.0f, std::min(1.0f, zoneProgress));

    int hits = minHits + static_cast<int>((typicalHits - minHits) * zoneProgress + 0.5f);
    return std::max(1, std::min(hits, maxHits));
}

int ComputeShimmerBudget(float energy, float balance, EnergyZone zone, int patternLength, float shape)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    balance = std::max(0.0f, std::min(1.0f, balance));

    // Base shimmer budget is typically half of anchor
    int anchorBudget = ComputeAnchorBudget(energy, zone, patternLength, shape);

    // Balance shifts hits between voices (expanded range to 150%)
    // balance = 0.0: shimmer gets 0% of anchor
    // balance = 0.5: shimmer gets 75% of anchor
    // balance = 1.0: shimmer gets 150% of anchor
    float shimmerRatio = balance * 1.5f;

    // Zone-aware shimmer cap to prevent over-density in low-energy zones
    if (zone == EnergyZone::GROOVE || zone == EnergyZone::MINIMAL)
    {
        shimmerRatio = std::min(shimmerRatio, 1.0f);
    }

    int hits = static_cast<int>(anchorBudget * shimmerRatio + 0.5f);

    // Minimum of 1 hit except in MINIMAL zone
    if (zone == EnergyZone::MINIMAL)
    {
        return std::max(0, std::min(hits, patternLength / 8));
    }

    return std::max(1, std::min(hits, patternLength / 4));
}

int ComputeAuxBudget(float energy, EnergyZone zone, AuxDensity auxDensity, int patternLength)
{
    // Clamp energy
    energy = std::max(0.0f, std::min(1.0f, energy));

    // Base aux budget (typically hi-hat-like patterns)
    int baseBudget;

    switch (zone)
    {
        case EnergyZone::MINIMAL:
            baseBudget = 0;  // No aux in minimal zone
            break;

        case EnergyZone::GROOVE:
            baseBudget = patternLength / 8;  // Light 8th notes
            break;

        case EnergyZone::BUILD:
            baseBudget = patternLength / 4;  // More active
            break;

        case EnergyZone::PEAK:
            baseBudget = patternLength / 2;  // Very active
            break;

        default:
            baseBudget = patternLength / 8;
            break;
    }

    // Apply density multiplier
    float multiplier = GetAuxDensityMultiplier(auxDensity);
    int hits = static_cast<int>(baseBudget * multiplier + 0.5f);

    return std::max(0, std::min(hits, patternLength));
}

void ComputeBarBudget(float energy,
                      float balance,
                      EnergyZone zone,
                      AuxDensity auxDensity,
                      int patternLength,
                      float buildMultiplier,
                      float shape,
                      BarBudget& outBudget)
{
    // Clamp pattern length for mask operations
    int clampedLength = ClampPatternLength(patternLength);

    // Compute base budgets (pass shape for density modulation - Task 39)
    outBudget.anchorHits  = ComputeAnchorBudget(energy, zone, clampedLength, shape);
    outBudget.shimmerHits = ComputeShimmerBudget(energy, balance, zone, clampedLength, shape);
    outBudget.auxHits     = ComputeAuxBudget(energy, zone, auxDensity, clampedLength);

    // Apply build multiplier (phrase arc)
    if (buildMultiplier > 1.0f)
    {
        outBudget.anchorHits = static_cast<int>(outBudget.anchorHits * buildMultiplier + 0.5f);
        outBudget.shimmerHits = static_cast<int>(outBudget.shimmerHits * buildMultiplier + 0.5f);
        outBudget.auxHits = static_cast<int>(outBudget.auxHits * buildMultiplier + 0.5f);
    }

    // Clamp to valid ranges (expanded from /2 to allow denser patterns)
    int maxHits = clampedLength * 2 / 3;  // Up to 2/3 of steps
    outBudget.anchorHits  = std::min(outBudget.anchorHits, maxHits);
    outBudget.shimmerHits = std::min(outBudget.shimmerHits, maxHits);
    outBudget.auxHits     = std::min(outBudget.auxHits, clampedLength);

    // Initialize eligibility to full pattern (will be refined by caller)
    // Avoid UB: (1U << 32) is undefined behavior
    uint32_t fullMask = (clampedLength >= 32) ? 0xFFFFFFFF : ((1U << clampedLength) - 1);
    outBudget.anchorEligibility  = fullMask;
    outBudget.shimmerEligibility = fullMask;
    outBudget.auxEligibility     = fullMask;
}

// =============================================================================
// Eligibility Mask Computation
// =============================================================================

uint32_t ComputeAnchorEligibility(float energy, float flavor, EnergyZone zone, int patternLength)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    flavor = std::max(0.0f, std::min(1.0f, flavor));
    int clampedLength = ClampPatternLength(patternLength);

    // Avoid UB: (1U << 32) is undefined behavior
    uint32_t lengthMask = (clampedLength >= 32) ? 0xFFFFFFFF : ((1U << clampedLength) - 1);
    uint32_t eligibility = 0;

    // Base eligibility based on zone
    switch (zone)
    {
        case EnergyZone::MINIMAL:
            // Only downbeats and quarter notes
            eligibility = kDownbeatMask | kQuarterNoteMask;
            break;

        case EnergyZone::GROOVE:
            // Quarter notes + some 8ths
            eligibility = kQuarterNoteMask;
            if (energy > 0.35f)
            {
                eligibility |= kEighthNoteMask;
            }
            break;

        case EnergyZone::BUILD:
            // 8th notes + some 16ths
            eligibility = kEighthNoteMask;
            if (energy > 0.60f)
            {
                eligibility |= kSixteenthNoteMask;
            }
            break;

        case EnergyZone::PEAK:
            // All positions available
            eligibility = kSixteenthNoteMask;
            break;

        default:
            eligibility = kQuarterNoteMask;
            break;
    }

    // FLAVOR adds syncopation/offbeat positions
    if (flavor > 0.3f)
    {
        eligibility |= kOffbeatMask;
    }
    if (flavor > 0.6f)
    {
        eligibility |= kSyncopationMask;
    }

    return eligibility & lengthMask;
}

uint32_t ComputeShimmerEligibility(float energy, float flavor, EnergyZone zone, int patternLength)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    flavor = std::max(0.0f, std::min(1.0f, flavor));
    int clampedLength = ClampPatternLength(patternLength);

    // Avoid UB: (1U << 32) is undefined behavior
    uint32_t lengthMask = (clampedLength >= 32) ? 0xFFFFFFFF : ((1U << clampedLength) - 1);
    uint32_t eligibility = 0;

    // Base eligibility based on zone
    switch (zone)
    {
        case EnergyZone::MINIMAL:
            // Only backbeats
            eligibility = kBackbeatMask;
            break;

        case EnergyZone::GROOVE:
            // Backbeats + off-8ths
            eligibility = kBackbeatMask;
            if (energy > 0.30f)
            {
                eligibility |= kOffbeatMask;
            }
            break;

        case EnergyZone::BUILD:
            // 8th notes available
            eligibility = kEighthNoteMask;
            break;

        case EnergyZone::PEAK:
            // All positions
            eligibility = kSixteenthNoteMask;
            break;

        default:
            eligibility = kBackbeatMask;
            break;
    }

    // FLAVOR allows more syncopation
    if (flavor > 0.4f)
    {
        eligibility |= kSyncopationMask;
    }

    return eligibility & lengthMask;
}

uint32_t ComputeAuxEligibility(float energy, float flavor, EnergyZone zone, int patternLength)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    flavor = std::max(0.0f, std::min(1.0f, flavor));
    int clampedLength = ClampPatternLength(patternLength);

    // Avoid UB: (1U << 32) is undefined behavior
    uint32_t lengthMask = (clampedLength >= 32) ? 0xFFFFFFFF : ((1U << clampedLength) - 1);
    uint32_t eligibility = 0;

    // Aux (hi-hat) is more permissive
    switch (zone)
    {
        case EnergyZone::MINIMAL:
            // No aux in minimal
            eligibility = 0;
            break;

        case EnergyZone::GROOVE:
            // 8th notes
            eligibility = kEighthNoteMask;
            break;

        case EnergyZone::BUILD:
            // 8ths + some 16ths
            eligibility = kEighthNoteMask;
            if (energy > 0.60f)
            {
                eligibility |= kSixteenthNoteMask;
            }
            break;

        case EnergyZone::PEAK:
            // All positions
            eligibility = kSixteenthNoteMask;
            break;

        default:
            eligibility = kEighthNoteMask;
            break;
    }

    return eligibility & lengthMask;
}

void ApplyFillBoost(BarBudget& budget,
                    float fillIntensity,
                    float fillMultiplier,
                    int patternLength)
{
    if (fillIntensity <= 0.0f)
    {
        return;
    }

    // Clamp inputs
    fillIntensity = std::min(1.0f, fillIntensity);
    fillMultiplier = std::max(1.0f, fillMultiplier);

    int clampedLength = ClampPatternLength(patternLength);

    // Compute boost factor: ramps from 1.0 to fillMultiplier based on intensity
    float boostFactor = 1.0f + (fillMultiplier - 1.0f) * fillIntensity;

    // Apply to all voices
    budget.anchorHits = static_cast<int>(budget.anchorHits * boostFactor + 0.5f);
    budget.shimmerHits = static_cast<int>(budget.shimmerHits * boostFactor + 0.5f);
    budget.auxHits = static_cast<int>(budget.auxHits * boostFactor + 0.5f);

    // Clamp to valid ranges
    int maxHits = clampedLength / 2;
    budget.anchorHits  = std::min(budget.anchorHits, maxHits);
    budget.shimmerHits = std::min(budget.shimmerHits, maxHits);
    budget.auxHits     = std::min(budget.auxHits, clampedLength);

    // During fills, open up eligibility more
    if (fillIntensity > 0.5f)
    {
        budget.anchorEligibility |= kEighthNoteMask;
        budget.shimmerEligibility |= kEighthNoteMask;
        budget.auxEligibility |= kSixteenthNoteMask;
    }
}

} // namespace daisysp_idm_grids
