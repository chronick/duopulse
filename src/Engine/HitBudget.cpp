#include "HitBudget.h"
#include "../../inc/algorithm_config.h"
#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Euclidean K / HitBudget Fade System (Task 73)
// =============================================================================

/**
 * Compute euclidean K for anchor voice from ENERGY
 * Uses kAnchorKMin/kAnchorKMax from algorithm_config.h
 */
int ComputeAnchorEuclideanK(float energy, int patternLength)
{
    using namespace AlgorithmConfig;

    energy = std::max(0.0f, std::min(1.0f, energy));
    int k = kAnchorKMin + static_cast<int>(energy * (kAnchorKMax - kAnchorKMin));
    return std::min(k, patternLength);
}

/**
 * Compute euclidean K for shimmer voice from ENERGY
 */
int ComputeShimmerEuclideanK(float energy, int patternLength)
{
    using namespace AlgorithmConfig;

    energy = std::max(0.0f, std::min(1.0f, energy));
    int k = kShimmerKMin + static_cast<int>(energy * (kShimmerKMax - kShimmerKMin));
    return std::min(k, patternLength);
}

/**
 * Compute effective hit count by fading between euclidean K and budget
 * based on SHAPE parameter.
 *
 * At SHAPE <= 0.05 AND ENERGY <= 0.05: pure four-on-floor mode with quarter-note floor
 * At SHAPE 0.05-0.15: use minimum of euclideanK and budgetK (preserve baseline sparsity)
 * At SHAPE = 1.0: pure budget-based (density-driven)
 *
 * Using min() at low SHAPE ensures we don't ADD hits compared to baseline,
 * while still preferring euclidean-style placement when hit counts match.
 *
 * The ENERGY check prevents the quarter-note floor from activating for
 * sparse presets like Minimal Techno (ENERGY=0.20, SHAPE=0) that should
 * follow the normal euclideanK-based hit count.
 */
int ComputeEffectiveHitCount(int euclideanK, int budgetK, float shape, float energy, int patternLength)
{
    using namespace AlgorithmConfig;

    // At very low SHAPE AND very low ENERGY (Four on Floor mode), use quarter-note count
    // euclidean(64,16) or euclidean(32,8) produces perfect quarter-note grid
    // Only activate for true Four on Floor patterns (ENERGY=0, SHAPE=0), not for
    // sparse techno patterns like Minimal Techno (ENERGY=0.20, SHAPE=0)
    if (shape <= 0.05f && energy <= 0.05f) {
        int quarterNoteCount = patternLength / 4;  // Quarter notes for this pattern length
        return std::max(euclideanK, quarterNoteCount);
    }

    // At low SHAPE, use minimum to preserve baseline sparsity
    // This prevents euclideanK from inflating hit count beyond budget
    if (shape <= 0.15f) {
        return std::min(euclideanK, budgetK);
    }

    // Linear fade from 0.15 to 1.0
    float fadeProgress = (shape - 0.15f) / 0.85f;
    fadeProgress = std::min(1.0f, fadeProgress);

    // Blend toward budgetK
    int baseK = std::min(euclideanK, budgetK);
    return static_cast<int>(
        baseK + fadeProgress * (budgetK - baseK) + 0.5f
    );
}

// =============================================================================
// Utility Functions
// =============================================================================

int CountBits(uint64_t mask)
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
    return std::min(patternLength, 64);
}

float GetAnchorBudgetMultiplier(float shape)
{
    // Clamp shape to valid range
    shape = std::max(0.0f, std::min(1.0f, shape));

    // V5 Spec 5.4 zone boundaries: 0.30 and 0.70
    if (shape < 0.30f)
    {
        // Stable zone: 100% of base
        return 1.0f;
    }
    else if (shape < 0.70f)
    {
        // Syncopated zone: 100% -> 90% (lerp over 0.30-0.70)
        float progress = (shape - 0.30f) / 0.40f;
        return 1.0f - progress * 0.10f;
    }
    else
    {
        // Wild zone: 90% -> 80% (lerp over 0.70-1.0)
        float progress = (shape - 0.70f) / 0.30f;
        return 0.90f - progress * 0.10f;
    }
}

float GetShimmerBudgetMultiplier(float shape)
{
    // Clamp shape to valid range
    shape = std::max(0.0f, std::min(1.0f, shape));

    // V5 Spec 5.4 zone boundaries: 0.30 and 0.70
    if (shape < 0.30f)
    {
        // Stable zone: 100% of base
        return 1.0f;
    }
    else if (shape < 0.70f)
    {
        // Syncopated zone: 110% -> 130% (lerp over 0.30-0.70)
        float progress = (shape - 0.30f) / 0.40f;
        return 1.10f + progress * 0.20f;
    }
    else
    {
        // Wild zone: 130% -> 150% (lerp over 0.70-1.0)
        float progress = (shape - 0.70f) / 0.30f;
        return 1.30f + progress * 0.20f;
    }
}

// =============================================================================
// Budget Computation
// =============================================================================

int ComputeAnchorBudget(float energy, EnergyZone zone, int patternLength, float shape)
{
    // Clamp energy to valid range
    energy = std::max(0.0f, std::min(1.0f, energy));
    shape = std::max(0.0f, std::min(1.0f, shape));

    // ==========================================================================
    // Task 73: Euclidean K / HitBudget Fade
    // At SHAPE <= 0.15: use euclidean K directly (grid-locked four-on-floor)
    // At SHAPE > 0.15: fade toward density-based budget
    // ==========================================================================

    // Compute euclidean K from ENERGY (what SHAPE=0 should produce)
    int euclideanK = ComputeAnchorEuclideanK(energy, patternLength);

    // Get shape multiplier for density adjustment (V5 Spec 5.4)
    float shapeMult = GetAnchorBudgetMultiplier(shape);

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

    int budgetK = minHits + static_cast<int>((typicalHits - minHits) * zoneProgress + 0.5f);
    budgetK = std::max(1, std::min(budgetK, maxHits));

    // Fade between euclidean K and budget K based on SHAPE (and ENERGY for four-on-floor)
    int effectiveK = ComputeEffectiveHitCount(euclideanK, budgetK, shape, energy, patternLength);
    return std::max(1, std::min(effectiveK, maxHits));
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

    // V5 Spec 5.4: Apply shape-based divergence correction
    // anchorBudget already has anchor multiplier baked in, so we swap it for shimmer's
    float anchorMult = GetAnchorBudgetMultiplier(shape);
    float shimmerMult = GetShimmerBudgetMultiplier(shape);
    float shapeCorrection = (anchorMult > 0.0f) ? (shimmerMult / anchorMult) : 1.0f;
    float adjustedShimmerRatio = shimmerRatio * shapeCorrection;

    int hits = static_cast<int>(anchorBudget * adjustedShimmerRatio + 0.5f);

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

    // Compute eligibility masks based on zone
    // Derive flavor from SHAPE: high SHAPE allows syncopation positions
    // This ensures evaluation patterns have position eligibility matching algorithm blending
    const float flavor = shape;
    outBudget.anchorEligibility  = ComputeAnchorEligibility(energy, flavor, zone, clampedLength);
    outBudget.shimmerEligibility = ComputeShimmerEligibility(energy, flavor, zone, clampedLength);
    outBudget.auxEligibility     = ComputeAuxEligibility(energy, flavor, zone, clampedLength);
}

// =============================================================================
// Eligibility Mask Computation
// =============================================================================

uint64_t ComputeAnchorEligibility(float energy, float flavor, EnergyZone zone, int patternLength)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    flavor = std::max(0.0f, std::min(1.0f, flavor));
    int clampedLength = ClampPatternLength(patternLength);

    // Avoid UB: (1U << 32) is undefined behavior
    uint64_t lengthMask = (clampedLength >= 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << clampedLength) - 1);
    uint64_t eligibility = 0;

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
    // For syncopated zone (shape > 0.28), add offbeats (even positions)
    if (flavor > 0.28f)
    {
        eligibility |= kOffbeatMask;
    }
    // For mid-syncopated zone (shape > 0.40), add odd positions
    // This allows Gumbel sampling to select a mix based on weights
    // Targeting ~30-40% odd positions for moderate syncopation (0.22-0.48)
    if (flavor > 0.40f)
    {
        eligibility |= kSyncopationMask;
    }

    return eligibility & lengthMask;
}

uint64_t ComputeShimmerEligibility(float energy, float flavor, EnergyZone zone, int patternLength)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    flavor = std::max(0.0f, std::min(1.0f, flavor));
    int clampedLength = ClampPatternLength(patternLength);

    // Avoid UB: (1U << 32) is undefined behavior
    uint64_t lengthMask = (clampedLength >= 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << clampedLength) - 1);
    uint64_t eligibility = 0;

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

    // FLAVOR allows more syncopation (threshold raised to reduce syncopation)
    if (flavor > 0.60f)
    {
        eligibility |= kSyncopationMask;
    }

    return eligibility & lengthMask;
}

uint64_t ComputeAuxEligibility(float energy, float flavor, EnergyZone zone, int patternLength)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    flavor = std::max(0.0f, std::min(1.0f, flavor));
    int clampedLength = ClampPatternLength(patternLength);

    // Avoid UB: (1U << 32) is undefined behavior
    uint64_t lengthMask = (clampedLength >= 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << clampedLength) - 1);
    uint64_t eligibility = 0;

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
