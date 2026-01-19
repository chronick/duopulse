#include "GuardRails.h"
#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Utility Functions
// =============================================================================

int GetMaxGapForZone(EnergyZone zone)
{
    switch (zone)
    {
        case EnergyZone::MINIMAL:
            return 32;  // No gap limit in minimal zone

        case EnergyZone::GROOVE:
            return kMaxGapGroove;

        case EnergyZone::BUILD:
            return kMaxGapBuild;

        case EnergyZone::PEAK:
            return kMaxGapPeak;

        default:
            return kMaxGapGroove;
    }
}

int GetMaxConsecutiveShimmerForZone(EnergyZone zone)
{
    switch (zone)
    {
        case EnergyZone::MINIMAL:
            return 2;  // Very limited in minimal

        case EnergyZone::GROOVE:
        case EnergyZone::BUILD:
            return kMaxConsecutiveShimmer;

        case EnergyZone::PEAK:
            return kMaxConsecutiveShimmerPeak;

        default:
            return kMaxConsecutiveShimmer;
    }
}

uint64_t FindGapMidpoints(uint64_t mask, int minGapSize, int patternLength)
{
    if (mask == 0 || minGapSize <= 1)
    {
        return 0;
    }

    int clampedLength = std::min(patternLength, 64);
    uint64_t midpoints = 0;

    // Find each gap and its midpoint
    int gapStart = -1;
    int gapLength = 0;

    for (int i = 0; i < clampedLength * 2; ++i)
    {
        int step = i % clampedLength;
        bool isHit = (mask & (1ULL << step)) != 0;

        if (!isHit)
        {
            if (gapStart < 0)
            {
                gapStart = step;
                gapLength = 1;
            }
            else
            {
                gapLength++;
            }
        }
        else
        {
            // Hit found, check if we just ended a gap
            if (gapStart >= 0 && gapLength >= minGapSize)
            {
                // Add midpoint of this gap
                int midpoint = (gapStart + gapLength / 2) % clampedLength;
                midpoints |= (1ULL << midpoint);
            }
            gapStart = -1;
            gapLength = 0;
        }

        // Prevent infinite loop in wrap-around case
        if (i >= clampedLength && gapLength >= clampedLength)
        {
            break;
        }
    }

    return midpoints;
}

int CountMaxConsecutiveShimmer(uint64_t anchorMask,
                               uint64_t shimmerMask,
                               int patternLength)
{
    int clampedLength = std::min(patternLength, 64);
    int maxRun = 0;
    int currentRun = 0;

    // Check for wrap-around by iterating twice
    for (int i = 0; i < clampedLength * 2; ++i)
    {
        int step = i % clampedLength;
        bool hasAnchor = (anchorMask & (1ULL << step)) != 0;
        bool hasShimmer = (shimmerMask & (1ULL << step)) != 0;

        if (hasShimmer && !hasAnchor)
        {
            currentRun++;
            if (currentRun > maxRun)
            {
                maxRun = currentRun;
            }
        }
        else
        {
            // Any non-shimmer step (anchor or empty) breaks the run
            currentRun = 0;
        }

        // Limit to pattern length
        if (currentRun >= clampedLength)
        {
            break;
        }
    }

    return std::min(maxRun, clampedLength);
}

// =============================================================================
// Soft Repair Functions
// =============================================================================

int FindWeakestHit(uint64_t mask, const float* weights, int patternLength)
{
    if (mask == 0)
    {
        return -1;
    }

    int clampedLength = std::min(patternLength, 64);
    int weakestStep = -1;
    float weakestWeight = 2.0f;  // Higher than any valid weight

    for (int step = 0; step < clampedLength; ++step)
    {
        if ((mask & (1ULL << step)) == 0)
        {
            continue;
        }

        if (weights[step] < weakestWeight)
        {
            weakestWeight = weights[step];
            weakestStep = step;
        }
    }

    return weakestStep;
}

int FindRescueCandidate(uint64_t mask,
                        uint64_t rescueMask,
                        const float* weights,
                        int patternLength)
{
    // Find the step in rescueMask that isn't in mask and has highest weight
    uint64_t candidates = rescueMask & ~mask;

    if (candidates == 0)
    {
        return -1;
    }

    int clampedLength = std::min(patternLength, 64);
    int bestStep = -1;
    float bestWeight = -1.0f;

    for (int step = 0; step < clampedLength; ++step)
    {
        if ((candidates & (1ULL << step)) == 0)
        {
            continue;
        }

        if (weights[step] > bestWeight)
        {
            bestWeight = weights[step];
            bestStep = step;
        }
    }

    return bestStep;
}

int SoftRepairPass(uint64_t& anchorMask,
                   uint64_t& shimmerMask,
                   const float* anchorWeights,
                   const float* shimmerWeights,
                   EnergyZone zone,
                   int patternLength)
{
    int repairs = 0;
    int clampedLength = std::min(patternLength, 64);
    int maxGap = GetMaxGapForZone(zone);

    // Check for near-violation of gap rule
    // A "near violation" is when gap is within 2 of max
    uint64_t gapMidpoints = FindGapMidpoints(anchorMask, maxGap - 2, clampedLength);

    if (gapMidpoints != 0)
    {
        // There's a large gap - try to rescue
        int weakest = FindWeakestHit(anchorMask, anchorWeights, clampedLength);
        int rescue = FindRescueCandidate(anchorMask, gapMidpoints, anchorWeights, clampedLength);

        if (weakest >= 0 && rescue >= 0 && rescue != weakest)
        {
            // Swap weakest hit for rescue position
            anchorMask &= ~(1ULL << weakest);
            anchorMask |= (1ULL << rescue);
            repairs++;
        }
    }

    // Check for shimmer burst near limit
    int maxConsec = GetMaxConsecutiveShimmerForZone(zone);
    int currentConsec = CountMaxConsecutiveShimmer(anchorMask, shimmerMask, clampedLength);

    if (currentConsec >= maxConsec - 1)
    {
        // Near shimmer burst limit - try to break it up
        // Find the shimmer burst and remove the weakest hit in it
        int burstStart = -1;
        int burstLength = 0;

        for (int step = 0; step < clampedLength; ++step)
        {
            bool hasAnchor = (anchorMask & (1ULL << step)) != 0;
            bool hasShimmer = (shimmerMask & (1ULL << step)) != 0;

            if (hasShimmer && !hasAnchor)
            {
                if (burstStart < 0)
                {
                    burstStart = step;
                    burstLength = 1;
                }
                else
                {
                    burstLength++;
                }
            }
            else if (hasAnchor)
            {
                if (burstLength >= maxConsec - 1)
                {
                    break;  // Found the problematic burst
                }
                burstStart = -1;
                burstLength = 0;
            }
        }

        if (burstStart >= 0 && burstLength >= maxConsec - 1)
        {
            // Create mask of burst shimmer hits
            uint64_t burstMask = 0;
            for (int i = 0; i < burstLength; ++i)
            {
                int step = (burstStart + i) % clampedLength;
                if ((shimmerMask & (1ULL << step)) != 0)
                {
                    burstMask |= (1ULL << step);
                }
            }

            // Remove weakest shimmer in burst
            int weakest = FindWeakestHit(burstMask, shimmerWeights, clampedLength);
            if (weakest >= 0)
            {
                shimmerMask &= ~(1ULL << weakest);
                repairs++;
            }
        }
    }

    return repairs;
}

// =============================================================================
// Hard Guard Rails
// =============================================================================

bool EnforceDownbeat(uint64_t& anchorMask, EnergyZone zone, int patternLength)
{
    (void)patternLength;  // Not needed for step 0

    // Only enforce in GROOVE+ zones
    if (zone == EnergyZone::MINIMAL)
    {
        return false;
    }

    // Check if step 0 has anchor
    if ((anchorMask & 0x1) != 0)
    {
        return false;  // Already has downbeat
    }

    // Force anchor on step 0
    anchorMask |= 0x1;
    return true;
}

int EnforceMaxGap(uint64_t& anchorMask, EnergyZone zone, int patternLength)
{
    int maxGap = GetMaxGapForZone(zone);

    if (maxGap >= patternLength)
    {
        return 0;  // No gap limit for this zone
    }

    int clampedLength = std::min(patternLength, 64);
    int corrections = 0;

    // Find gaps and add hits to break them
    while (true)
    {
        uint64_t gapMidpoints = FindGapMidpoints(anchorMask, maxGap + 1, clampedLength);

        if (gapMidpoints == 0)
        {
            break;  // No more gaps exceeding limit
        }

        // Add hit at first midpoint
        for (int step = 0; step < clampedLength; ++step)
        {
            if ((gapMidpoints & (1ULL << step)) != 0)
            {
                anchorMask |= (1ULL << step);
                corrections++;
                break;
            }
        }

        // Safety limit to prevent infinite loop
        if (corrections >= 8)
        {
            break;
        }
    }

    return corrections;
}

int EnforceConsecutiveShimmer(uint64_t anchorMask,
                               uint64_t& shimmerMask,
                               EnergyZone zone,
                               int patternLength)
{
    int maxConsec = GetMaxConsecutiveShimmerForZone(zone);
    int clampedLength = std::min(patternLength, 64);
    int removals = 0;

    // Remove shimmer hits to break up long runs
    // Strategy: find runs exceeding maxConsec and remove shimmer at interval positions
    while (CountMaxConsecutiveShimmer(anchorMask, shimmerMask, clampedLength) > maxConsec)
    {
        // Find a run that exceeds the limit and identify the break point
        int runLength = 0;
        int runStart = -1;
        int removeAt = -1;

        for (int step = 0; step < clampedLength; ++step)
        {
            bool hasAnchor = (anchorMask & (1ULL << step)) != 0;
            bool hasShimmer = (shimmerMask & (1ULL << step)) != 0;

            if (hasShimmer && !hasAnchor)
            {
                if (runStart < 0)
                {
                    runStart = step;
                }
                runLength++;

                // Remove at position (maxConsec) within the run to break it up
                if (runLength == maxConsec + 1)
                {
                    removeAt = step;
                    break;  // Found the position, stop searching
                }
            }
            else
            {
                // Reset run on any non-shimmer step (anchor or empty)
                runLength = 0;
                runStart = -1;
            }
        }

        if (removeAt >= 0)
        {
            shimmerMask &= ~(1ULL << removeAt);
            removals++;
        }
        else
        {
            break;  // Couldn't find a shimmer to remove
        }

        // Safety limit
        if (removals >= 16)
        {
            break;
        }
    }

    return removals;
}

int EnforceGenreRules(uint64_t anchorMask,
                       uint64_t& shimmerMask,
                       Genre genre,
                       EnergyZone zone,
                       int patternLength)
{
    int modifications = 0;

    // Only apply in GROOVE+ zones
    if (zone == EnergyZone::MINIMAL)
    {
        return 0;
    }

    switch (genre)
    {
        case Genre::TECHNO:
            // Techno: Encourage backbeat (steps 8, 24 in 32-step pattern)
            // Only if there's no shimmer at all on backbeats
            if (patternLength >= 16)
            {
                // Avoid UB: (1U << 32) is undefined behavior
                uint64_t lengthMask = (patternLength >= 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << patternLength) - 1);
                uint64_t backbeatMask = kBackbeatMask & lengthMask;

                // Check if shimmer has any backbeats
                if ((shimmerMask & backbeatMask) == 0 && CountBits(shimmerMask) > 0)
                {
                    // No backbeats - add step 8 (or nearest equivalent)
                    int backbeat = (patternLength >= 16) ? 8 : patternLength / 2;
                    shimmerMask |= (1ULL << backbeat);
                    modifications++;
                }
            }
            break;

        case Genre::TRIBAL:
            // Tribal: Less strict, but encourage some off-beat hits
            // No forced modifications - rely on archetype weights
            break;

        case Genre::IDM:
            // IDM: No forced patterns - embrace chaos
            break;

        default:
            break;
    }

    (void)anchorMask;  // Not used currently
    return modifications;
}

int ApplyHardGuardRails(uint64_t& anchorMask,
                        uint64_t& shimmerMask,
                        EnergyZone zone,
                        Genre genre,
                        int patternLength)
{
    int corrections = 0;

    // 1. Downbeat protection
    if (EnforceDownbeat(anchorMask, zone, patternLength))
    {
        corrections++;
    }

    // 2. Max gap enforcement
    corrections += EnforceMaxGap(anchorMask, zone, patternLength);

    // 3. Consecutive shimmer limit
    corrections += EnforceConsecutiveShimmer(anchorMask, shimmerMask, zone, patternLength);

    // 4. Genre-specific rules
    corrections += EnforceGenreRules(anchorMask, shimmerMask, genre, zone, patternLength);

    return corrections;
}

} // namespace daisysp_idm_grids
