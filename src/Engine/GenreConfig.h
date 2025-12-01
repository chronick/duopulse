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

// === CV-Driven Fills (FLUX) ===

/**
 * FLUX controls fill probability and pattern variation.
 * Higher FLUX = more fills, ghost notes, and chaos.
 * 
 * FLUX Level | Behavior
 * -----------|----------
 * 0-20%      | Clean, minimal pattern
 * 20-50%     | Some ghost notes, subtle variation
 * 50-70%     | Active fills, velocity swells
 * 70-90%     | Busy, lots of ghost notes and fills
 * 90-100%    | Maximum chaos, fill on every opportunity
 */

// Fill probability thresholds
constexpr float kFluxFillThreshold    = 0.5f;  // FLUX level where fills start appearing
constexpr float kFluxMaxFillProb      = 0.3f;  // Max 30% fill probability at FLUX=1
constexpr float kFluxGhostMultiplier  = 0.5f;  // Ghost note probability scales with FLUX

/**
 * Calculate fill trigger probability based on FLUX level.
 * Fills start appearing at 50% FLUX and scale up to 30% probability at 100%.
 * 
 * @param flux FLUX parameter (0-1)
 * @return Fill probability (0-0.3)
 */
inline float CalculateFillProbability(float flux)
{
    if(flux < kFluxFillThreshold)
        return 0.0f;

    // Linear scale from threshold to max
    float fillAmount = (flux - kFluxFillThreshold) / (1.0f - kFluxFillThreshold);
    return fillAmount * kFluxMaxFillProb;
}

/**
 * Check if a fill should trigger this step.
 * 
 * @param flux FLUX parameter (0-1)
 * @param randomValue Random value 0-1 for probability check
 * @return True if fill should trigger
 */
inline bool ShouldTriggerFill(float flux, float randomValue)
{
    float prob = CalculateFillProbability(flux);
    return randomValue < prob;
}

/**
 * Calculate velocity for a fill trigger.
 * Fills have varied velocity based on FLUX intensity.
 * 
 * @param flux FLUX parameter (0-1)
 * @param randomValue Random value 0-1 for velocity variation
 * @return Velocity (0.4-0.9)
 */
inline float CalculateFillVelocity(float flux, float randomValue)
{
    // Base velocity 0.4-0.7, higher FLUX = higher possible velocity
    float baseVel = 0.4f + (flux * 0.3f);
    float variation = (randomValue - 0.5f) * 0.2f; // ±0.1 variation
    float vel = baseVel + variation;
    if(vel < 0.3f)
        vel = 0.3f;
    if(vel > 0.9f)
        vel = 0.9f;
    return vel;
}

// === Humanize Timing Jitter ===

// Max jitter range in milliseconds
constexpr float kMaxHumanizeJitterMs = 10.0f;

// IDM terrain adds extra humanize on top of the knob setting
constexpr float kIdmExtraHumanize = 0.30f; // 30% extra

/**
 * Calculate effective humanize amount including IDM bonus.
 * 
 * @param humanize Base humanize parameter (0-1)
 * @param terrain Genre selector (0-1)
 * @return Effective humanize (0-1.3 for IDM)
 */
inline float CalculateEffectiveHumanize(float humanize, float terrain)
{
    float effective = humanize;

    // IDM (terrain >= 0.75) adds extra humanize
    if(terrain >= 0.75f)
    {
        // Scale extra humanize based on how deep into IDM territory we are
        float idmDepth   = (terrain - 0.75f) / 0.25f; // 0 at 0.75, 1 at 1.0
        float extraBonus = kIdmExtraHumanize * idmDepth;
        effective += extraBonus;
    }

    return effective;
}

/**
 * Calculate jitter delay in samples.
 * Returns a random value in range [-maxJitter, +maxJitter].
 * 
 * @param humanize Effective humanize amount (0-1.3)
 * @param sampleRate Audio sample rate
 * @param randomSeed Random value (0-1) for jitter calculation
 * @return Jitter in samples (can be negative for early triggers)
 */
inline int CalculateHumanizeJitterSamples(float humanize, float sampleRate, float randomValue)
{
    if(humanize <= 0.0f)
        return 0;

    // Max jitter at humanize = 1.0 is ±10ms
    // randomValue is 0-1, map to -1 to +1
    float normalizedRandom = (randomValue * 2.0f) - 1.0f;

    // Jitter range scales with humanize
    float jitterMs = normalizedRandom * kMaxHumanizeJitterMs * humanize;

    // Clamp to ±13ms (allowing for IDM's extra 30%)
    if(jitterMs > 13.0f)
        jitterMs = 13.0f;
    if(jitterMs < -13.0f)
        jitterMs = -13.0f;

    return static_cast<int>((jitterMs / 1000.0f) * sampleRate);
}

// === Orbit Voice Relationship Modes ===

enum class OrbitMode : uint8_t
{
    Interlock = 0, // Shimmer fills gaps in Anchor (call-response)
    Free      = 1, // Independent patterns, no collision logic
    Shadow    = 2  // Shimmer echoes Anchor with 1-step delay
};

/**
 * Get Orbit mode from orbit parameter (0-1).
 * Interlock: 0-33%, Free: 33-67%, Shadow: 67-100%
 */
inline OrbitMode GetOrbitMode(float orbit)
{
    if(orbit < 0.33f)
        return OrbitMode::Interlock;
    if(orbit < 0.67f)
        return OrbitMode::Free;
    return OrbitMode::Shadow;
}

/**
 * Calculate shimmer probability modifier for Interlock mode.
 * When anchor fires, shimmer probability is reduced.
 * When anchor is silent, shimmer probability is boosted.
 * 
 * @param anchorFired True if anchor triggered this step
 * @param orbit Orbit parameter (0-0.33 for Interlock)
 * @return Probability modifier (-0.3 to +0.3)
 */
inline float GetInterlockModifier(bool anchorFired, float orbit)
{
    // Interlock strength scales with how deep into the Interlock zone we are
    // At orbit=0, max interlock effect. At orbit=0.33, minimal effect
    float strength = 1.0f - (orbit / 0.33f);
    if(strength < 0.0f)
        strength = 0.0f;

    // ±30% probability modifier at max strength
    return anchorFired ? (-0.3f * strength) : (0.3f * strength);
}

} // namespace daisysp_idm_grids

