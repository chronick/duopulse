#include "VoiceRelation.h"
#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Utility Functions
// =============================================================================

uint32_t ShiftMaskLeft(uint32_t mask, int shift, int patternLength)
{
    if (shift <= 0 || patternLength <= 0)
    {
        return mask;
    }

    int clampedLength = std::min(patternLength, 32);

    // Avoid undefined behavior: (1U << 32) is UB
    uint32_t lengthMask = (clampedLength >= 32) ? 0xFFFFFFFF : ((1U << clampedLength) - 1);

    // Shift reduces to modulo pattern length
    shift = shift % clampedLength;

    // Left shift (delay in time = higher step numbers)
    uint32_t shifted = (mask << shift) & lengthMask;
    uint32_t wrapped = (mask >> (clampedLength - shift)) & lengthMask;

    return (shifted | wrapped) & lengthMask;
}

uint32_t ShiftMaskRight(uint32_t mask, int shift, int patternLength)
{
    if (shift <= 0 || patternLength <= 0)
    {
        return mask;
    }

    int clampedLength = std::min(patternLength, 32);

    // Avoid undefined behavior: (1U << 32) is UB
    uint32_t lengthMask = (clampedLength >= 32) ? 0xFFFFFFFF : ((1U << clampedLength) - 1);

    shift = shift % clampedLength;

    // Right shift (advance in time = lower step numbers)
    uint32_t shifted = (mask >> shift) & lengthMask;
    uint32_t wrapped = (mask << (clampedLength - shift)) & lengthMask;

    return (shifted | wrapped) & lengthMask;
}

int FindLargestGap(uint32_t mask, int patternLength)
{
    if (mask == 0)
    {
        return patternLength;  // All gap
    }

    int clampedLength = std::min(patternLength, 32);
    int maxGap = 0;
    int currentGap = 0;

    // Need to handle wrap-around, so we iterate twice through the pattern
    for (int i = 0; i < clampedLength * 2; ++i)
    {
        int step = i % clampedLength;

        if ((mask & (1U << step)) != 0)
        {
            // Hit found, check if current gap is largest
            if (currentGap > maxGap)
            {
                maxGap = currentGap;
            }
            currentGap = 0;
        }
        else
        {
            currentGap++;
        }

        // Limit gap to pattern length (for wrap-around)
        if (currentGap >= clampedLength)
        {
            maxGap = clampedLength;
            break;
        }
    }

    return std::min(maxGap, clampedLength);
}

int FindGapStart(uint32_t mask, int minGapSize, int patternLength)
{
    if (mask == 0)
    {
        return 0;  // Entire pattern is a gap
    }

    int clampedLength = std::min(patternLength, 32);

    for (int start = 0; start < clampedLength; ++start)
    {
        bool gapFound = true;

        for (int offset = 0; offset < minGapSize; ++offset)
        {
            int step = (start + offset) % clampedLength;
            if ((mask & (1U << step)) != 0)
            {
                gapFound = false;
                break;
            }
        }

        if (gapFound)
        {
            return start;
        }
    }

    return -1;  // No gap of sufficient size
}

// =============================================================================
// V5 COMPLEMENT Relationship (Task 30)
// =============================================================================

int FindGaps(uint32_t anchorMask, int patternLength, Gap* gaps)
{
    if (patternLength <= 0 || gaps == nullptr)
    {
        return 0;
    }

    int clampedLength = std::min(patternLength, 32);
    int gapCount = 0;

    // Handle empty anchor mask (entire pattern is one gap)
    if (anchorMask == 0)
    {
        gaps[0].start = 0;
        gaps[0].length = clampedLength;
        return 1;
    }

    // Find all gaps by scanning for runs of zeros
    int gapStart = -1;

    for (int i = 0; i < clampedLength; ++i)
    {
        bool isHit = (anchorMask & (1U << i)) != 0;

        if (!isHit && gapStart < 0)
        {
            // Start of a new gap
            gapStart = i;
        }
        else if (isHit && gapStart >= 0)
        {
            // End of current gap
            if (gapCount < kMaxGaps)
            {
                gaps[gapCount].start = gapStart;
                gaps[gapCount].length = i - gapStart;
                gapCount++;
            }
            gapStart = -1;
        }
    }

    // Handle gap that extends to end of pattern
    if (gapStart >= 0 && gapCount < kMaxGaps)
    {
        gaps[gapCount].start = gapStart;
        gaps[gapCount].length = clampedLength - gapStart;
        gapCount++;
    }

    // Handle wrap-around: combine first and last gaps if both touch boundaries
    if (gapCount > 1)
    {
        bool firstTouchesStart = (gaps[0].start == 0);
        bool lastTouchesEnd = (gaps[gapCount - 1].start + gaps[gapCount - 1].length == clampedLength);

        if (firstTouchesStart && lastTouchesEnd)
        {
            // Combine: last gap wraps into first gap
            // New gap starts at last gap's start, length is sum of both
            int combinedLength = gaps[0].length + gaps[gapCount - 1].length;

            // Keep the first gap but update to wrap-around version
            gaps[0].start = gaps[gapCount - 1].start;
            gaps[0].length = combinedLength;

            // Remove the last gap
            gapCount--;
        }
    }

    return gapCount;
}

namespace
{

// Simple LCG-based pseudo-random for RT-safe seeded randomness
inline uint32_t NextRandom(uint32_t& state)
{
    state = state * 1103515245U + 12345U;
    return (state >> 16) & 0x7FFF;
}

// Place hit evenly spaced within gap
int PlaceEvenlySpaced(const Gap& gap, int hitIndex, int totalHits, int patternLength)
{
    if (totalHits <= 0)
    {
        return gap.start;
    }

    // Distribute evenly: hit 0 goes near start, last hit near end
    int offset = (gap.length * hitIndex) / std::max(1, totalHits);
    return (gap.start + offset) % patternLength;
}

// Place hit at position with highest weight within gap
int PlaceWeightedBest(const Gap& gap, const float* shimmerWeights, int patternLength,
                      uint32_t usedMask)
{
    // Validate patternLength doesn't exceed array bounds
    if (patternLength > static_cast<int>(kMaxSteps)) {
        patternLength = static_cast<int>(kMaxSteps);
    }

    // Null check for shimmerWeights - find first available position in gap
    if (!shimmerWeights) {
        for (int offset = 0; offset < gap.length; ++offset) {
            int step = (gap.start + offset) % patternLength;
            if ((usedMask & (1U << step)) == 0) {
                return step;
            }
        }
        return gap.start;  // Fallback if all used
    }

    float bestWeight = -1.0f;
    int bestPos = gap.start;

    for (int offset = 0; offset < gap.length; ++offset)
    {
        int step = (gap.start + offset) % patternLength;

        // Skip already-used positions
        if ((usedMask & (1U << step)) != 0)
        {
            continue;
        }

        float weight = shimmerWeights[step];
        if (weight > bestWeight)
        {
            bestWeight = weight;
            bestPos = step;
        }
    }

    return bestPos;
}

// Place hit with seed-varied randomness within gap
int PlaceSeedVaried(const Gap& gap, uint32_t& rngState, int patternLength, uint32_t usedMask)
{
    // Count available positions
    int available = 0;
    for (int offset = 0; offset < gap.length; ++offset)
    {
        int step = (gap.start + offset) % patternLength;
        if ((usedMask & (1U << step)) == 0)
        {
            available++;
        }
    }

    if (available == 0)
    {
        return gap.start;  // Fallback
    }

    // Pick random index from available
    int targetIdx = static_cast<int>(NextRandom(rngState) % static_cast<uint32_t>(available));

    // Find the target position
    int count = 0;
    for (int offset = 0; offset < gap.length; ++offset)
    {
        int step = (gap.start + offset) % patternLength;
        if ((usedMask & (1U << step)) == 0)
        {
            if (count == targetIdx)
            {
                return step;
            }
            count++;
        }
    }

    return gap.start;  // Fallback
}

}  // anonymous namespace

uint32_t ApplyComplementRelationship(uint32_t anchorMask,
                                     const float* shimmerWeights,
                                     float drift, uint32_t seed,
                                     int patternLength, int targetHits)
{
    // Edge cases: no shimmer hits needed
    if (targetHits <= 0 || patternLength <= 0)
    {
        return 0;
    }

    int clampedLength = std::min(patternLength, 32);

    // Find gaps in anchor pattern
    Gap gaps[kMaxGaps];
    int gapCount = FindGaps(anchorMask, clampedLength, gaps);

    // If no gaps, no room for shimmer
    if (gapCount == 0)
    {
        return 0;
    }

    // Calculate total gap length
    int totalGapLength = 0;
    for (int g = 0; g < gapCount; ++g)
    {
        totalGapLength += gaps[g].length;
    }

    // If no gap space, return empty
    if (totalGapLength == 0)
    {
        return 0;
    }

    // Better seed mixing to avoid correlation (was: seed ^ 0xDEADBEEF)
    uint32_t rngState = (seed * 2654435761U) ^ (seed >> 16);
    if (rngState == 0) rngState = 1;  // Avoid zero state

    // Build shimmer mask by distributing hits proportionally to gaps
    uint32_t shimmerMask = 0;
    int remainingHits = targetHits;

    for (int g = 0; g < gapCount && remainingHits > 0; ++g)
    {
        // Proportional hit share for this gap
        int gapShare = (gaps[g].length * remainingHits) / std::max(1, totalGapLength);

        // At least 1 if there are remaining hits and gap has length
        if (gapShare == 0 && remainingHits > 0 && gaps[g].length > 0)
        {
            gapShare = 1;
        }

        // Don't exceed remaining budget or gap length
        gapShare = std::min(gapShare, remainingHits);
        gapShare = std::min(gapShare, gaps[g].length);

        // Update remaining for next iteration
        totalGapLength -= gaps[g].length;

        // Place hits in this gap using strategy based on drift
        for (int j = 0; j < gapShare; ++j)
        {
            int position;

            if (drift < 0.3f)
            {
                // Low drift: evenly spaced within gap
                position = PlaceEvenlySpaced(gaps[g], j, gapShare, clampedLength);
            }
            else if (drift < 0.7f)
            {
                // Mid drift: weighted by shimmer weights
                position = PlaceWeightedBest(gaps[g], shimmerWeights, clampedLength, shimmerMask);
            }
            else
            {
                // High drift: seed-varied random
                position = PlaceSeedVaried(gaps[g], rngState, clampedLength, shimmerMask);
            }

            shimmerMask |= (1U << position);
            remainingHits--;
        }
    }

    // If we still need more hits (due to rounding), fill remaining gaps
    for (int g = 0; g < gapCount && remainingHits > 0; ++g)
    {
        for (int offset = 0; offset < gaps[g].length && remainingHits > 0; ++offset)
        {
            int step = (gaps[g].start + offset) % clampedLength;
            if ((shimmerMask & (1U << step)) == 0)
            {
                shimmerMask |= (1U << step);
                remainingHits--;
            }
        }
    }

    return shimmerMask;
}

// =============================================================================
// Legacy V4 Functions (simplified for V5 compatibility)
// =============================================================================

void ApplyVoiceRelationship(uint32_t anchorMask,
                            uint32_t& shimmerMask,
                            VoiceCoupling coupling,
                            int patternLength)
{
    // V5: Only INDEPENDENT mode is supported
    // INTERLOCK and SHADOW are deprecated - use ApplyComplementRelationship instead
    (void)anchorMask;
    (void)coupling;
    (void)patternLength;
    // shimmerMask is not modified in INDEPENDENT mode
}

// =============================================================================
// Aux Voice Relationship
// =============================================================================

void ApplyAuxRelationship(uint32_t anchorMask,
                          uint32_t shimmerMask,
                          uint32_t& auxMask,
                          VoiceCoupling coupling,
                          int patternLength)
{
    // V5: Aux always operates independently
    // Legacy coupling modes are ignored
    (void)anchorMask;
    (void)shimmerMask;
    (void)coupling;
    (void)patternLength;
    // auxMask is not modified in INDEPENDENT mode
}

} // namespace daisysp_idm_grids
