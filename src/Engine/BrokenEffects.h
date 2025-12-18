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
 * Get the weight boost for the current phrase position with DRIFT/RATCHET control.
 *
 * This is the v3 version that implements the DRIFT × RATCHET interaction:
 * - DRIFT gates fill probability (at DRIFT=0, no fills occur)
 * - RATCHET controls fill intensity (0-30% density boost)
 *
 * Fill zones:
 * - Mid-phrase (40-60%): Potential mid-phrase fill
 * - Build zone (50-75%): Increasing energy toward phrase end  
 * - Fill zone (75-100%): Maximum fill activity
 *
 * Reference: docs/specs/main.md [ratchet-control]
 *
 * @param pos Current phrase position
 * @param broken BROKEN parameter (0.0-1.0) - affects genre scaling
 * @param drift DRIFT parameter (0.0-1.0) - gates fill probability
 * @param ratchet RATCHET parameter (0.0-1.0) - controls fill intensity
 * @return Weight boost to add to step weights (0.0 to ~0.45)
 */
inline float GetPhraseWeightBoostWithRatchet(const PhrasePosition& pos,
                                             float broken,
                                             float drift,
                                             float ratchet)
{
    // CRITICAL: DRIFT=0 means no fills occur, regardless of RATCHET
    if(drift <= 0.0f)
        return 0.0f;

    // No boost outside fill-relevant zones
    if(!pos.isBuildZone && !pos.isMidPhrase)
        return 0.0f;

    // Clamp parameters to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;
    if(drift < 0.0f)
        drift = 0.0f;
    if(drift > 1.0f)
        drift = 1.0f;
    if(ratchet < 0.0f)
        ratchet = 0.0f;
    if(ratchet > 1.0f)
        ratchet = 1.0f;

    // Base boost depends on zone
    float boost = 0.0f;

    if(pos.isFillZone)
    {
        // Fill zone (75-100%): Maximum fill activity
        // phraseProgress goes from 0.75 to 1.0 in fill zone
        // Base boost: 0.15 to 0.25
        float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f;
        boost              = 0.15f + fillProgress * 0.10f;
    }
    else if(pos.isBuildZone)
    {
        // Build zone (50-75%): Increasing energy
        // phraseProgress goes from 0.5 to 0.75 in build zone
        // boost goes from 0 to 0.075
        float buildProgress = (pos.phraseProgress - 0.5f) * 4.0f;
        boost               = buildProgress * 0.075f;
    }
    else if(pos.isMidPhrase)
    {
        // Mid-phrase (40-60%): Potential mid-phrase fill
        // Subtle boost, only with higher RATCHET
        // boost goes from 0 to 0.05 based on RATCHET
        boost = 0.05f * ratchet;
    }

    // RATCHET scales fill intensity (0-30% additional boost at max)
    // At RATCHET=0: only base boost
    // At RATCHET=1: base boost + 30% extra
    float ratchetBoost = boost * ratchet * 0.6f; // Up to 60% extra on base boost
    boost += ratchetBoost;

    // DRIFT gates how much fill activity occurs
    // At DRIFT=0: no fills (handled above)
    // At DRIFT=1: full fill probability
    boost *= drift;

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

/**
 * Get the velocity accent multiplier with RATCHET-enhanced resolution accent.
 *
 * RATCHET boosts the resolution accent on phrase downbeats:
 * - Phrase downbeat: 1.2× to 1.5× based on RATCHET
 * - Bar downbeat: 1.1× (unchanged)
 * - Fill zone: velocity ramp 1.0-1.3× toward phrase end
 *
 * Reference: docs/specs/main.md [ratchet-control]
 *
 * @param pos Current phrase position
 * @param ratchet RATCHET parameter (0.0-1.0) - fill intensity
 * @return Velocity multiplier (1.0-1.5)
 */
inline float GetPhraseAccentWithRatchet(const PhrasePosition& pos, float ratchet)
{
    // Clamp ratchet to valid range
    if(ratchet < 0.0f)
        ratchet = 0.0f;
    if(ratchet > 1.0f)
        ratchet = 1.0f;

    // Phrase downbeat gets resolution accent boosted by RATCHET
    // Base: 1.2×, max with RATCHET=1: 1.5×
    if(pos.stepInPhrase == 0)
        return 1.2f + (ratchet * 0.3f);

    // Bar downbeat gets moderate accent (unchanged by RATCHET)
    if(pos.isDownbeat)
        return 1.1f;

    // Fill zone: velocity ramp toward phrase end (fills get louder)
    // Ramp scales with RATCHET: 0 at RATCHET=0, up to 1.3× at RATCHET=1
    if(pos.isFillZone && ratchet > 0.0f)
    {
        // fillProgress: 0 at start of fill zone (75%), 1 at end (100%)
        float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f;
        if(fillProgress < 0.0f)
            fillProgress = 0.0f;
        if(fillProgress > 1.0f)
            fillProgress = 1.0f;
        // Velocity ramp: 1.0 to 1.3× toward end, scaled by RATCHET
        return 1.0f + (fillProgress * 0.3f * ratchet);
    }

    // No accent for other steps
    return 1.0f;
}

// =============================================================================
// Voice Interaction: FUSE Energy Balance [fuse-balance]
// =============================================================================

/**
 * Apply FUSE energy balance between Anchor and Shimmer voices.
 *
 * FUSE tilts the energy between voices:
 * - fuse = 0.0: anchor-heavy (kick emphasized)
 * - fuse = 0.5: balanced (no change)
 * - fuse = 1.0: shimmer-heavy (snare/hat emphasized)
 *
 * At extremes, density shifts by ±15%.
 *
 * v3 Critical Rule: If a voice's base density was 0, FUSE must NOT boost it above 0.
 * DENSITY=0 must always mean absolute silence.
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [fuse-balance]
 *
 * @param fuse FUSE parameter (0.0-1.0)
 * @param anchorDensity Reference to anchor density (modified in place)
 * @param shimmerDensity Reference to shimmer density (modified in place)
 */
inline void ApplyFuse(float fuse, float& anchorDensity, float& shimmerDensity)
{
    // v3 Critical Rule: DENSITY=0 = absolute silence
    // Store whether each voice was at zero before any modification
    bool anchorWasZero  = (anchorDensity <= 0.0f);
    bool shimmerWasZero = (shimmerDensity <= 0.0f);

    // Clamp fuse to valid range
    if(fuse < 0.0f)
        fuse = 0.0f;
    if(fuse > 1.0f)
        fuse = 1.0f;

    // Calculate bias: (fuse - 0.5) * 0.3 gives ±0.15 at extremes
    // fuse = 0.0 → bias = -0.15 (anchor boost, shimmer reduce)
    // fuse = 0.5 → bias = 0.0 (balanced)
    // fuse = 1.0 → bias = +0.15 (shimmer boost, anchor reduce)
    float bias = (fuse - 0.5f) * 0.3f;

    // Apply bias: subtract from anchor, add to shimmer
    anchorDensity  = Clamp(anchorDensity - bias, 0.0f, 1.0f);
    shimmerDensity = Clamp(shimmerDensity + bias, 0.0f, 1.0f);

    // Enforce DENSITY=0 invariant: if voice started at 0, keep it at 0
    if(anchorWasZero)
        anchorDensity = 0.0f;
    if(shimmerWasZero)
        shimmerDensity = 0.0f;
}

// =============================================================================
// Voice Interaction: COUPLE Interlock [couple-interlock]
// =============================================================================

// Magic numbers for COUPLE hash mixing
constexpr uint32_t kCoupleSuppressHashMagic = 0x53555050; // "SUPP"
constexpr uint32_t kCoupleBoostHashMagic    = 0x424F5354; // "BOST"
constexpr uint32_t kCoupleVelHashMagic      = 0x56454C43; // "VELC"

/**
 * Apply COUPLE interlock between Anchor and Shimmer voices.
 *
 * COUPLE controls voice relationship strength:
 * - 0%: fully independent (voices can collide or gap freely)
 * - 50%: soft interlock (slight collision avoidance)
 * - 100%: hard interlock (shimmer strongly fills anchor gaps)
 *
 * When anchor fires, shimmer may be suppressed to avoid collision.
 * When anchor is silent, shimmer may be boosted to fill the gap.
 *
 * v3 Critical Rule: If shimmer's density is 0, COUPLE must NOT inject triggers.
 * DENSITY=0 must always mean absolute silence.
 *
 * Reference: docs/specs/double-down/simplified-algorithmic-approach.md [couple-interlock]
 *
 * @param couple COUPLE parameter (0.0-1.0)
 * @param anchorFires Whether anchor is triggering this step
 * @param shimmerFires Reference to shimmer trigger (may be modified)
 * @param shimmerVel Reference to shimmer velocity (may be modified for fills)
 * @param seed Seed for deterministic randomness
 * @param step Step index for per-step randomness
 * @param shimmerDensity Shimmer density (for enforcing DENSITY=0 invariant)
 */
inline void ApplyCouple(float couple,
                        bool anchorFires,
                        bool& shimmerFires,
                        float& shimmerVel,
                        uint32_t seed,
                        int step,
                        float shimmerDensity = -1.0f)
{
    // Clamp couple to valid range
    if(couple < 0.0f)
        couple = 0.0f;
    if(couple > 1.0f)
        couple = 1.0f;

    // Below 10% couple: fully independent, no interaction
    if(couple < 0.1f)
        return;

    if(anchorFires)
    {
        // Anchor is firing — reduce shimmer probability (collision avoidance)
        // Suppression chance scales from 0% at couple=0.1 to 80% at couple=1.0
        float suppressChance = couple * 0.8f;

        uint32_t hash   = HashStep(seed ^ kCoupleSuppressHashMagic, step);
        float randomVal = HashToFloat(hash);

        if(randomVal < suppressChance)
        {
            shimmerFires = false;
        }
    }
    else
    {
        // Anchor is silent — boost shimmer probability (gap filling)
        // Only applies when shimmer wasn't already firing and couple > 50%
        //
        // v3 Critical Rule: NEVER gap-fill if shimmerDensity is 0
        // DENSITY=0 = absolute silence
        bool densityAllowsFill = (shimmerDensity < 0.0f) || (shimmerDensity > 0.0f);
        
        if(!shimmerFires && couple > 0.5f && densityAllowsFill)
        {
            // Boost chance scales from 0% at couple=0.5 to 30% at couple=1.0
            float boostChance = (couple - 0.5f) * 0.6f;

            uint32_t hash   = HashStep(seed ^ kCoupleBoostHashMagic, step);
            float randomVal = HashToFloat(hash);

            if(randomVal < boostChance)
            {
                shimmerFires = true;

                // Generate medium velocity for fill (0.5 to 0.8)
                uint32_t velHash = HashStep(seed ^ kCoupleVelHashMagic, step);
                shimmerVel       = 0.5f + HashToFloat(velHash) * 0.3f;
            }
        }
    }
}

} // namespace daisysp_idm_grids
