#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * Genre-aware swing configuration for DuoPulse v2.
 * 
 * Swing is opinionated by genre but adjustable within a curated range.
 * This prevents musically inappropriate swing while allowing personal taste.
 * 
 * Swing percentage meaning:
 *   50% = straight (no swing)
 *   66% = triplet feel
 *   >66% = "drunk" feel
 */

// Genre identifiers
enum class Genre : uint8_t
{
    Techno  = 0, // 0-25% terrain
    Tribal  = 1, // 25-50% terrain
    TripHop = 2, // 50-75% terrain
    IDM     = 3  // 75-100% terrain
};

// Swing range for a genre
struct SwingRange
{
    float minSwing; // Swing at swingTaste = 0
    float maxSwing; // Swing at swingTaste = 1
    float jitter;   // Additional timing jitter (IDM only)
};

// Genre swing configurations per spec
// Swing values are percentages (50% = straight, 66% = triplet)
constexpr SwingRange kGenreSwingRanges[] = {
    // Techno: 52-57% (nearly straight to subtle groove)
    {0.52f, 0.57f, 0.0f},
    // Tribal: 56-62% (mild shuffle to pronounced swing)
    {0.56f, 0.62f, 0.0f},
    // Trip-Hop: 60-68% (lazy to very drunk)
    {0.60f, 0.68f, 0.0f},
    // IDM: 54-65% + timing jitter (tight to broken)
    {0.54f, 0.65f, 0.03f} // 3% extra jitter for IDM
};

/**
 * Get genre from terrain parameter (0-1).
 */
inline Genre GetGenreFromTerrain(float terrain)
{
    if(terrain < 0.25f)
        return Genre::Techno;
    if(terrain < 0.50f)
        return Genre::Tribal;
    if(terrain < 0.75f)
        return Genre::TripHop;
    return Genre::IDM;
}

/**
 * Get swing range for a genre.
 */
inline SwingRange GetSwingRange(Genre genre)
{
    uint8_t idx = static_cast<uint8_t>(genre);
    if(idx > 3)
        idx = 0;
    return kGenreSwingRanges[idx];
}

/**
 * Calculate effective swing percentage from terrain and swingTaste.
 * 
 * @param terrain   Genre selector (0-1)
 * @param swingTaste  Fine-tune within genre range (0-1)
 * @return Swing percentage (0.5 = straight, 0.66 = triplet)
 */
inline float CalculateSwing(float terrain, float swingTaste)
{
    Genre             genre = GetGenreFromTerrain(terrain);
    const SwingRange& range = GetSwingRange(genre);
    return range.minSwing + swingTaste * (range.maxSwing - range.minSwing);
}

/**
 * Get genre-specific jitter amount.
 * IDM adds extra micro-timing chaos.
 */
inline float GetGenreJitter(float terrain)
{
    Genre             genre = GetGenreFromTerrain(terrain);
    const SwingRange& range = GetSwingRange(genre);
    return range.jitter;
}

/**
 * Check if a step is an off-beat (should receive swing).
 * In 16th note patterns, off-beats are odd-numbered steps (1, 3, 5, 7...).
 */
inline bool IsOffBeat(int step)
{
    return (step & 1) != 0;
}

/**
 * Calculate swing delay in samples for an off-beat step.
 * 
 * Swing works by delaying off-beat notes. At 50% swing, no delay.
 * At 66% swing, the off-beat is delayed to create a triplet feel.
 * 
 * @param swingPercent Swing amount (0.5 = straight, 0.66 = triplet)
 * @param stepDurationSamples Duration of one 16th note in samples
 * @return Delay in samples to apply to off-beat
 */
inline int CalculateSwingDelaySamples(float swingPercent, int stepDurationSamples)
{
    // Swing delay formula:
    // At 50% swing, off-beat is exactly halfway (no delay)
    // At 66% swing, off-beat is 2/3 of the way (triplet)
    // Delay = (swingPercent - 0.5) * stepDuration
    float delayFraction = swingPercent - 0.5f;
    if(delayFraction < 0.0f)
        delayFraction = 0.0f;
    return static_cast<int>(delayFraction * static_cast<float>(stepDurationSamples));
}

} // namespace daisysp_idm_grids

