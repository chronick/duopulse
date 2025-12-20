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
// Voice Relationship Core
// =============================================================================

void ApplyInterlock(uint32_t anchorMask, uint32_t& shimmerMask, int patternLength)
{
    // Remove shimmer hits that coincide with anchor
    shimmerMask &= ~anchorMask;
}

void ApplyShadow(uint32_t anchorMask, uint32_t& shimmerMask, int patternLength)
{
    // Shadow: shimmer echoes anchor with 1-step delay
    // This replaces the shimmer mask entirely
    shimmerMask = ShiftMaskLeft(anchorMask, 1, patternLength);
}

void ApplyVoiceRelationship(uint32_t anchorMask,
                            uint32_t& shimmerMask,
                            VoiceCoupling coupling,
                            int patternLength)
{
    // Store original for potential gap-fill
    uint32_t originalShimmer = shimmerMask;

    switch (coupling)
    {
        case VoiceCoupling::INDEPENDENT:
            // No modification - voices can overlap
            break;

        case VoiceCoupling::INTERLOCK:
            ApplyInterlock(anchorMask, shimmerMask, patternLength);

            // Apply gap-fill if interlock created too large a gap
            ApplyGapFill(anchorMask, shimmerMask, originalShimmer, 8, patternLength);
            break;

        case VoiceCoupling::SHADOW:
            ApplyShadow(anchorMask, shimmerMask, patternLength);
            break;

        default:
            break;
    }
}

VoiceCoupling GetEffectiveCoupling(VoiceCoupling configCoupling,
                                   float archetypeDefault,
                                   bool useConfig)
{
    if (useConfig)
    {
        return configCoupling;
    }

    // Derive from archetype's default couple value
    return GetVoiceCouplingFromValue(archetypeDefault);
}

void ApplyGapFill(uint32_t anchorMask,
                  uint32_t& shimmerMask,
                  uint32_t originalShimmer,
                  int maxGap,
                  int patternLength)
{
    // Check if there's a large gap in the combined pattern
    uint32_t combined = anchorMask | shimmerMask;
    int largestGap = FindLargestGap(combined, patternLength);

    if (largestGap <= maxGap)
    {
        return;  // Gap is acceptable
    }

    // Find the gap and try to fill it with original shimmer hits
    int gapStart = FindGapStart(combined, largestGap, patternLength);

    if (gapStart < 0)
    {
        return;
    }

    // Look for original shimmer hits within the gap
    for (int offset = 0; offset < largestGap; ++offset)
    {
        int step = (gapStart + offset) % patternLength;

        if ((originalShimmer & (1U << step)) != 0)
        {
            // Restore this shimmer hit to fill the gap
            shimmerMask |= (1U << step);
            break;  // Only fill one hit to break up the gap
        }
    }
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
    // Aux voice (hi-hat) has simpler rules
    switch (coupling)
    {
        case VoiceCoupling::INDEPENDENT:
            // Aux fires independently
            break;

        case VoiceCoupling::INTERLOCK:
            // In interlock mode, aux avoids anchor but can coincide with shimmer
            // This creates rhythmic interplay: kick → hat → snare patterns
            auxMask &= ~anchorMask;
            break;

        case VoiceCoupling::SHADOW:
            // In shadow mode, aux adds density around both voices
            // Keep aux hits that are adjacent to anchor or shimmer
            {
                uint32_t neighbors = ShiftMaskLeft(anchorMask | shimmerMask, 1, patternLength) |
                                     ShiftMaskRight(anchorMask | shimmerMask, 1, patternLength);
                // Keep original aux that's near other voices, or on standalone positions
                uint32_t nearVoices = auxMask & neighbors;
                uint32_t awayFromVoices = auxMask & ~(anchorMask | shimmerMask);
                auxMask = nearVoices | awayFromVoices;
            }
            break;

        default:
            break;
    }
}

} // namespace daisysp_idm_grids
