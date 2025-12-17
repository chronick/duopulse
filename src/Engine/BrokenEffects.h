#pragma once

#include "PulseField.h"    // For Lerp, Clamp, HashStep, HashToFloat
#include "GenreConfig.h"   // For PhrasePosition

namespace daisysp_idm_grids
{

/**
 * BROKEN Effects Stack
 *
 * The BROKEN parameter doesn't just flatten weights—it also progressively
 * adds timing effects that contribute to genre character. As BROKEN increases:
 *
 * 1. Swing increases (Techno → Tribal → Trip-Hop → IDM)
 * 2. Micro-timing jitter increases
 * 3. Step displacement becomes possible
 * 4. Velocity variation increases
 *
 * These effects combine to create a coherent transition from straight 4/4
 * to experimental IDM patterns.
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [broken-effects]
 */

// Magic numbers for hash mixing (to avoid correlation between effects)
constexpr uint32_t kJitterHashMagic       = 0x4A495454; // "JITT"
constexpr uint32_t kDisplaceChanceHashMagic = 0x44495331; // "DIS1"
constexpr uint32_t kDisplaceShiftHashMagic  = 0x44495332; // "DIS2"
constexpr uint32_t kVelocityHashMagic      = 0x56454C30; // "VEL0"

// =============================================================================
// Effect 1: Swing (Tied to BROKEN)
// =============================================================================

/**
 * Get the swing amount from the BROKEN parameter.
 *
 * Swing is no longer a separate genre setting. It scales with BROKEN:
 *
 * | BROKEN Range | Genre Feel | Swing %   | Character              |
 * |--------------|------------|-----------|------------------------|
 * | 0-25%        | Techno     | 50-54%    | Nearly straight        |
 * | 25-50%       | Tribal     | 54-60%    | Mild shuffle           |
 * | 50-75%       | Trip-Hop   | 60-66%    | Lazy, behind-beat      |
 * | 75-100%      | IDM        | 66-58%    | Variable + heavy jitter|
 *
 * @param broken BROKEN parameter (0.0-1.0)
 * @return Swing amount (0.5 = straight, 0.66 = max lazy)
 */
inline float GetSwingFromBroken(float broken)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    if(broken < 0.25f)
    {
        // Techno: 50-54% (nearly straight)
        float t = broken * 4.0f;  // 0.0 to 1.0 within range
        return 0.50f + t * 0.04f; // 0.50 to 0.54
    }
    else if(broken < 0.50f)
    {
        // Tribal: 54-60% (shuffled)
        float t = (broken - 0.25f) * 4.0f; // 0.0 to 1.0 within range
        return 0.54f + t * 0.06f;          // 0.54 to 0.60
    }
    else if(broken < 0.75f)
    {
        // Trip-Hop: 60-66% (lazy)
        float t = (broken - 0.50f) * 4.0f; // 0.0 to 1.0 within range
        return 0.60f + t * 0.06f;          // 0.60 to 0.66
    }
    else
    {
        // IDM: 66-58% (variable swing combined with heavy jitter)
        // Continuity: start at 0.66 (where Trip-Hop ends), move toward 0.58
        // The micro-timing jitter provides the chaos, not swing reduction
        float t = (broken - 0.75f) * 4.0f; // 0.0 to 1.0 within range
        return 0.66f - t * 0.08f;          // 0.66 to 0.58
    }
}

// Note: IsOffBeat() is defined in GenreConfig.h (included above)

// =============================================================================
// Effect 2: Micro-Timing Jitter
// =============================================================================

/**
 * Get the maximum jitter amount in milliseconds from the BROKEN parameter.
 *
 * Humanize/jitter increases with BROKEN:
 *
 * | BROKEN Range | Max Jitter | Feel           |
 * |--------------|------------|----------------|
 * | 0-40%        | ±0ms       | Machine-tight  |
 * | 40-70%       | ±3ms       | Subtle human   |
 * | 70-90%       | ±6ms       | Loose, organic |
 * | 90-100%      | ±12ms      | Broken, glitchy|
 *
 * @param broken BROKEN parameter (0.0-1.0)
 * @return Maximum jitter in milliseconds (to be applied as ±jitter)
 */
inline float GetJitterMsFromBroken(float broken)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    if(broken < 0.4f)
    {
        // Machine-tight: 0ms
        return 0.0f;
    }
    else if(broken < 0.7f)
    {
        // Subtle human feel: 0-3ms
        float t = (broken - 0.4f) / 0.3f; // 0.0 to 1.0 within range
        return t * 3.0f;                  // 0.0 to 3.0
    }
    else if(broken < 0.9f)
    {
        // Loose, organic: 3-6ms
        float t = (broken - 0.7f) / 0.2f; // 0.0 to 1.0 within range
        return 3.0f + t * 3.0f;           // 3.0 to 6.0
    }
    else
    {
        // Broken, glitchy: 6-12ms
        float t = (broken - 0.9f) / 0.1f; // 0.0 to 1.0 within range
        return 6.0f + t * 6.0f;           // 6.0 to 12.0
    }
}

/**
 * Apply jitter to a trigger timing.
 *
 * Given the max jitter from GetJitterMsFromBroken(), this function
 * returns a random jitter amount in the range [-maxJitter, +maxJitter].
 *
 * @param maxJitterMs Maximum jitter in milliseconds
 * @param seed Seed for deterministic randomness
 * @param step Step index (for unique per-step randomness)
 * @return Jitter amount in milliseconds
 */
inline float ApplyJitter(float maxJitterMs, uint32_t seed, int step)
{
    if(maxJitterMs <= 0.0f)
        return 0.0f;

    // Use a different hash offset to avoid correlation with other randomness
    uint32_t hash   = HashStep(seed ^ kJitterHashMagic, step);
    float randomVal = HashToFloat(hash); // 0.0 to 1.0
    // Map to [-1.0, +1.0] and scale
    return (randomVal - 0.5f) * 2.0f * maxJitterMs;
}

// =============================================================================
// Effect 3: Step Displacement
// =============================================================================

/**
 * Get a potentially displaced step position based on BROKEN level.
 *
 * At higher BROKEN, triggers can shift to adjacent steps:
 *
 * | BROKEN Range | Displacement Chance | Max Shift   |
 * |--------------|---------------------|-------------|
 * | 0-50%        | 0%                  | 0 steps     |
 * | 50-75%       | 0-15%               | ±1 step     |
 * | 75-100%      | 15-40%              | ±2 steps    |
 *
 * @param step Original step index (0-31)
 * @param broken BROKEN parameter (0.0-1.0)
 * @param seed Seed for deterministic randomness
 * @return Displaced step index (0-31, wrapped)
 */
inline int GetDisplacedStep(int step, float broken, uint32_t seed)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // No displacement below 50% BROKEN
    if(broken < 0.5f)
        return step;

    // Calculate displacement chance based on BROKEN level
    float displaceChance;
    int maxShift;

    if(broken < 0.75f)
    {
        // 50-75%: 0-15% chance, ±1 step
        float t       = (broken - 0.5f) * 4.0f; // 0.0 to 1.0 within range
        displaceChance = t * 0.15f;             // 0.0 to 0.15
        maxShift       = 1;
    }
    else
    {
        // 75-100%: 15-40% chance, ±2 steps
        float t       = (broken - 0.75f) * 4.0f; // 0.0 to 1.0 within range
        displaceChance = 0.15f + t * 0.25f;      // 0.15 to 0.40
        maxShift       = 2;
    }

    // Determine if displacement happens
    uint32_t chanceHash = HashStep(seed ^ kDisplaceChanceHashMagic, step);
    float chanceRoll    = HashToFloat(chanceHash);

    if(chanceRoll >= displaceChance)
        return step; // No displacement

    // Determine shift direction and amount
    uint32_t shiftHash = HashStep(seed ^ kDisplaceShiftHashMagic, step);
    float shiftRoll    = HashToFloat(shiftHash);

    // Map [0, 1) to [-maxShift, +maxShift]
    int shift = static_cast<int>((shiftRoll * (2 * maxShift + 1))) - maxShift;

    // Wrap around to valid step range
    int newStep = (step + shift + 32) % 32;
    return newStep;
}

// =============================================================================
// Effect 4: Velocity Variation
// =============================================================================

/**
 * Apply velocity variation based on BROKEN level.
 *
 * Velocity consistency decreases with BROKEN:
 *
 * | BROKEN Range | Velocity Variation | Character           |
 * |--------------|-------------------|---------------------|
 * | 0-30%        | ±5%               | Consistent          |
 * | 30-60%       | ±10%              | Subtle dynamics     |
 * | 60-100%      | ±20%              | Expressive, uneven  |
 *
 * @param baseVel Base velocity (0.0-1.0)
 * @param broken BROKEN parameter (0.0-1.0)
 * @param seed Seed for deterministic randomness
 * @param step Step index (for unique per-step randomness)
 * @return Varied velocity, clamped to [0.2, 1.0]
 */
inline float GetVelocityWithVariation(float baseVel, float broken, uint32_t seed, int step)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // Calculate variation range based on BROKEN level
    float variationRange;

    if(broken < 0.3f)
    {
        // Consistent: ±5%
        variationRange = 0.05f;
    }
    else if(broken < 0.6f)
    {
        // Subtle dynamics: 5-10%
        float t        = (broken - 0.3f) / 0.3f; // 0.0 to 1.0 within range
        variationRange = 0.05f + t * 0.05f;      // 0.05 to 0.10
    }
    else
    {
        // Expressive, uneven: 10-20%
        float t        = (broken - 0.6f) / 0.4f; // 0.0 to 1.0 within range
        variationRange = 0.10f + t * 0.10f;      // 0.10 to 0.20
    }

    // Apply random variation
    uint32_t hash   = HashStep(seed ^ kVelocityHashMagic, step);
    float randomVal = HashToFloat(hash);                   // 0.0 to 1.0
    float variation = (randomVal - 0.5f) * 2.0f * variationRange;

    float result = baseVel + variation;

    // Clamp to valid velocity range (minimum 0.2 to ensure audibility)
    if(result < 0.2f)
        result = 0.2f;
    if(result > 1.0f)
        result = 1.0f;

    return result;
}

/**
 * Get the velocity variation range for a given BROKEN level.
 * Useful for testing or displaying the current variation amount.
 *
 * @param broken BROKEN parameter (0.0-1.0)
 * @return Variation range (e.g., 0.05 means ±5%)
 */
inline float GetVelocityVariationRange(float broken)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    if(broken < 0.3f)
    {
        return 0.05f;
    }
    else if(broken < 0.6f)
    {
        float t = (broken - 0.3f) / 0.3f;
        return 0.05f + t * 0.05f;
    }
    else
    {
        float t = (broken - 0.6f) / 0.4f;
        return 0.10f + t * 0.10f;
    }
}

// =============================================================================
// Phrase-Aware Modulation [phrase-modulation]
// =============================================================================

/**
 * Get the weight boost for the current phrase position.
 *
 * Near phrase boundaries, weights are modulated to create natural fills:
 * - Build zone (50-75%): subtle boost (0 to 0.075)
 * - Fill zone (75-100%): significant boost (0.15 to 0.25)
 *
 * The boost is scaled by BROKEN level:
 * - Techno (low broken): subtle fills (0.5× scale)
 * - IDM (high broken): dramatic fills (1.5× scale)
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [phrase-modulation]
 *
 * @param pos Current phrase position
 * @param broken BROKEN parameter (0.0-1.0)
 * @return Weight boost to add to step weights (0.0 to ~0.375)
 */
inline float GetPhraseWeightBoost(const PhrasePosition& pos, float broken)
{
    // No boost outside build zone
    if(!pos.isBuildZone)
        return 0.0f;

    // Clamp broken to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // Base boost: increases toward phrase end
    float boost = 0.0f;

    if(pos.isFillZone)
    {
        // Last 25%: significant boost to off-beat weights
        // phraseProgress goes from 0.75 to 1.0 in fill zone
        // boost goes from 0.15 to 0.25
        float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f; // 0.0 to 1.0
        boost              = 0.15f + fillProgress * 0.10f;        // 0.15 to 0.25
    }
    else
    {
        // Build zone but not fill zone (50-75%): subtle boost
        // phraseProgress goes from 0.5 to 0.75 in build zone
        // boost goes from 0 to 0.075
        float buildProgress = (pos.phraseProgress - 0.5f) * 4.0f; // 0.0 to 1.0
        boost               = buildProgress * 0.075f;             // 0.0 to 0.075
    }

    // Genre scale: Techno has subtle fills, IDM has dramatic fills
    // Scale ranges from 0.5 (at broken=0) to 1.5 (at broken=1)
    float genreScale = 0.5f + broken * 1.0f;

    return boost * genreScale;
}

/**
 * Get the effective BROKEN level, boosted in fill zones.
 *
 * Temporarily increase BROKEN in fill zones for extra chaos:
 * - Outside fill zone: no change
 * - In fill zone: boost by up to 20% toward phrase end
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [phrase-modulation]
 *
 * @param broken Base BROKEN parameter (0.0-1.0)
 * @param pos Current phrase position
 * @return Effective BROKEN level (0.0-1.0)
 */
inline float GetEffectiveBroken(float broken, const PhrasePosition& pos)
{
    // No boost outside fill zone
    if(!pos.isFillZone)
        return Clamp(broken, 0.0f, 1.0f);

    // Clamp input
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // Boost BROKEN by up to 20% in fill zone
    // phraseProgress goes from 0.75 to 1.0 in fill zone
    // fillBoost goes from 0 to 0.2
    float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f; // 0.0 to 1.0
    float fillBoost    = fillProgress * 0.2f;                 // 0.0 to 0.2

    return Clamp(broken + fillBoost, 0.0f, 1.0f);
}

/**
 * Get the velocity accent multiplier for the current phrase position.
 *
 * Downbeats get velocity boosts to emphasize phrase structure:
 * - Phrase downbeat (step 0 of phrase): 1.2× velocity
 * - Bar downbeat (step 0 of any bar): 1.1× velocity
 * - Other steps: 1.0× (no accent)
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [phrase-modulation]
 *
 * @param pos Current phrase position
 * @return Velocity multiplier (1.0, 1.1, or 1.2)
 */
inline float GetPhraseAccent(const PhrasePosition& pos)
{
    // Phrase downbeat gets maximum accent
    if(pos.stepInPhrase == 0)
        return 1.2f;

    // Bar downbeat gets moderate accent
    if(pos.isDownbeat)
        return 1.1f;

    // No accent for other steps
    return 1.0f;
}

} // namespace daisysp_idm_grids
