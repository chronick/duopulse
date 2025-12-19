#pragma once

#include "PulseField.h"       // For Lerp, Clamp, HashStep, HashToFloat
#include "PhrasePosition.h"   // For PhrasePosition struct

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
float GetSwingFromBroken(float broken);

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
float GetJitterMsFromBroken(float broken);

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
float ApplyJitter(float maxJitterMs, uint32_t seed, int step);

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
int GetDisplacedStep(int step, float broken, uint32_t seed);

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
float GetVelocityWithVariation(float baseVel, float broken, uint32_t seed, int step);

/**
 * Get the velocity variation range for a given BROKEN level.
 * Useful for testing or displaying the current variation amount.
 *
 * @param broken BROKEN parameter (0.0-1.0)
 * @return Variation range (e.g., 0.05 means ±5%)
 */
float GetVelocityVariationRange(float broken);

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
float GetPhraseWeightBoost(const PhrasePosition& pos, float broken);

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
float GetPhraseWeightBoostWithRatchet(const PhrasePosition& pos,
                                      float broken,
                                      float drift,
                                      float ratchet);

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
float GetEffectiveBroken(float broken, const PhrasePosition& pos);

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
float GetPhraseAccent(const PhrasePosition& pos);

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
float GetPhraseAccentWithRatchet(const PhrasePosition& pos, float ratchet);

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
void ApplyFuse(float fuse, float& anchorDensity, float& shimmerDensity);

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
 * @param shimmerDensity Shimmer density (for enforcing DENSITY=0 invariant, default -1.0 means not checked)
 */
void ApplyCouple(float couple,
                 bool anchorFires,
                 bool& shimmerFires,
                 float& shimmerVel,
                 uint32_t seed,
                 int step,
                 float shimmerDensity = -1.0f);

} // namespace daisysp_idm_grids
