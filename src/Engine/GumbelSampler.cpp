#include "GumbelSampler.h"
#include <cmath>
#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Constants
// =============================================================================

/// Small epsilon to avoid log(0)
static constexpr float kEpsilon = 1e-6f;

/// Maximum score value (for initialization)
static constexpr float kMinScore = -1e9f;

// =============================================================================
// Hash Functions
// =============================================================================

float HashToFloat(uint32_t seed, int step)
{
    // Combine seed and step using a fast hash
    uint32_t hash = seed;
    hash ^= static_cast<uint32_t>(step) * 0x9e3779b9;
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;

    // Convert to float in (0, 1) - avoid exactly 0 for Gumbel
    // Use top 24 bits for precision
    float result = static_cast<float>(hash >> 8) / 16777216.0f;

    // Clamp to (epsilon, 1-epsilon) for numerical stability
    return std::max(kEpsilon, std::min(1.0f - kEpsilon, result));
}

float UniformToGumbel(float uniform)
{
    // Gumbel distribution: -log(-log(u))
    // Clamp to avoid numerical issues
    uniform = std::max(kEpsilon, std::min(1.0f - kEpsilon, uniform));
    return -std::log(-std::log(uniform));
}

// =============================================================================
// Score Computation
// =============================================================================

void ComputeGumbelScores(const float* weights,
                         uint32_t seed,
                         int patternLength,
                         float* outScores)
{
    for (int step = 0; step < patternLength; ++step)
    {
        float weight = weights[step];

        // Ensure positive weight for log
        if (weight < kEpsilon)
        {
            outScores[step] = kMinScore;
            continue;
        }

        // Compute log(weight) + Gumbel(seed, step)
        float uniform = HashToFloat(seed, step);
        float gumbel = UniformToGumbel(uniform);
        outScores[step] = std::log(weight) + gumbel;
    }
}

// =============================================================================
// Spacing Helpers
// =============================================================================

uint32_t GetSpacingExclusionMask(int step, int minSpacing, int patternLength)
{
    if (minSpacing <= 0)
    {
        return 0;
    }

    uint32_t mask = 0;

    // Exclude steps within minSpacing of the reference step
    for (int offset = -minSpacing; offset <= minSpacing; ++offset)
    {
        if (offset == 0) continue;  // Don't exclude the step itself

        int excludeStep = step + offset;

        // Handle wrap-around for circular patterns
        if (excludeStep < 0)
        {
            excludeStep += patternLength;
        }
        else if (excludeStep >= patternLength)
        {
            excludeStep -= patternLength;
        }

        if (excludeStep >= 0 && excludeStep < 32)
        {
            mask |= (1U << excludeStep);
        }
    }

    return mask;
}

bool CheckSpacingValid(uint32_t selectedMask, int candidateStep, int minSpacing, int patternLength)
{
    if (minSpacing <= 0 || selectedMask == 0)
    {
        return true;
    }

    // Check distance to each selected step
    for (int step = 0; step < patternLength; ++step)
    {
        if ((selectedMask & (1U << step)) == 0)
        {
            continue;
        }

        // Compute minimum distance (circular)
        int dist = std::abs(candidateStep - step);
        int wrapDist = patternLength - dist;
        int minDist = std::min(dist, wrapDist);

        if (minDist < minSpacing)
        {
            return false;
        }
    }

    return true;
}

int GetMinSpacingForZone(EnergyZone zone)
{
    switch (zone)
    {
        case EnergyZone::MINIMAL:
            return 4;  // Sparse patterns need more spacing

        case EnergyZone::GROOVE:
            return 2;  // Moderate spacing

        case EnergyZone::BUILD:
            return 1;  // Tight patterns allowed

        case EnergyZone::PEAK:
            return 0;  // No spacing constraint at peak

        default:
            return 2;
    }
}

// =============================================================================
// Selection Functions
// =============================================================================

int FindBestStep(const float* scores,
                 uint32_t eligibilityMask,
                 uint32_t selectedMask,
                 int patternLength,
                 int minSpacing)
{
    float bestScore = kMinScore;
    int bestStep = -1;

    for (int step = 0; step < patternLength && step < 32; ++step)
    {
        // Check eligibility
        if ((eligibilityMask & (1U << step)) == 0)
        {
            continue;
        }

        // Check already selected
        if ((selectedMask & (1U << step)) != 0)
        {
            continue;
        }

        // Check spacing constraint
        if (!CheckSpacingValid(selectedMask, step, minSpacing, patternLength))
        {
            continue;
        }

        // Check if this is the best so far
        if (scores[step] > bestScore)
        {
            bestScore = scores[step];
            bestStep = step;
        }
    }

    return bestStep;
}

uint32_t SelectHitsGumbelSimple(const float* weights,
                                 uint32_t eligibilityMask,
                                 int targetCount,
                                 uint32_t seed,
                                 int patternLength)
{
    return SelectHitsGumbelTopK(weights, eligibilityMask, targetCount,
                                 seed, patternLength, 0);
}

uint32_t SelectHitsGumbelTopK(const float* weights,
                               uint32_t eligibilityMask,
                               int targetCount,
                               uint32_t seed,
                               int patternLength,
                               int minSpacing)
{
    // Clamp pattern length for bitmask operations
    patternLength = std::min(patternLength, 32);

    // Handle edge cases
    if (targetCount <= 0 || eligibilityMask == 0)
    {
        return 0;
    }

    // Limit target count to reasonable maximum
    targetCount = std::min(targetCount, kMaxSelectableHits);

    // Compute Gumbel scores for all steps
    float scores[kMaxSteps];
    ComputeGumbelScores(weights, seed, patternLength, scores);

    // Greedily select top-K steps respecting spacing
    uint32_t selectedMask = 0;
    int selectedCount = 0;

    // First pass: try to hit target with spacing constraints
    while (selectedCount < targetCount)
    {
        int bestStep = FindBestStep(scores, eligibilityMask, selectedMask,
                                    patternLength, minSpacing);

        if (bestStep < 0)
        {
            // No more valid steps with current spacing
            break;
        }

        selectedMask |= (1U << bestStep);
        selectedCount++;
    }

    // Second pass: if we didn't hit target and spacing was limiting,
    // try again with relaxed spacing
    if (selectedCount < targetCount && minSpacing > 0)
    {
        int relaxedSpacing = minSpacing / 2;

        while (selectedCount < targetCount)
        {
            int bestStep = FindBestStep(scores, eligibilityMask, selectedMask,
                                        patternLength, relaxedSpacing);

            if (bestStep < 0)
            {
                break;
            }

            selectedMask |= (1U << bestStep);
            selectedCount++;
        }
    }

    // Final pass: no spacing constraint if still short
    if (selectedCount < targetCount)
    {
        while (selectedCount < targetCount)
        {
            int bestStep = FindBestStep(scores, eligibilityMask, selectedMask,
                                        patternLength, 0);

            if (bestStep < 0)
            {
                break;  // No more eligible steps at all
            }

            selectedMask |= (1U << bestStep);
            selectedCount++;
        }
    }

    return selectedMask;
}

} // namespace daisysp_idm_grids
