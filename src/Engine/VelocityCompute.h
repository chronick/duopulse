#pragma once

#include <cstdint>
#include "ControlState.h"
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * VelocityCompute: Velocity computation for DuoPulse v4
 *
 * Velocity is controlled by the PUNCH parameter, which sets the dynamic
 * contrast between accented and ghost hits. BUILD adds phrase-arc
 * modulation, and accent masks determine which steps get emphasized.
 *
 * Reference: docs/specs/main.md section 7.2
 */

// =============================================================================
// PUNCH Parameter Computation
// =============================================================================

/**
 * Compute PunchParams from PUNCH knob value.
 *
 * PUNCH controls velocity dynamics:
 * - PUNCH = 0%: Flat dynamics (all hits similar velocity)
 * - PUNCH = 50%: Normal dynamics (standard accented groove)
 * - PUNCH = 100%: Maximum dynamics (huge accent contrast)
 *
 * @param punch PUNCH parameter value (0.0-1.0)
 * @param[out] params Output PunchParams struct
 */
void ComputePunch(float punch, PunchParams& params);

// =============================================================================
// BUILD Parameter Computation
// =============================================================================

/**
 * Compute BuildModifiers from BUILD value and phrase position.
 *
 * BUILD controls the narrative arc of each phrase:
 * - BUILD = 0%: Flat throughout (no builds, no fills)
 * - BUILD = 50%: Subtle build (slight density increase, fills at end)
 * - BUILD = 100%: Dramatic arc (big builds, intense fills)
 *
 * @param build BUILD parameter value (0.0-1.0)
 * @param phraseProgress Current phrase progress (0.0-1.0)
 * @param[out] modifiers Output BuildModifiers struct
 */
void ComputeBuildModifiers(float build, float phraseProgress, BuildModifiers& modifiers);

// =============================================================================
// Velocity Computation
// =============================================================================

/**
 * Compute velocity for a step based on PUNCH, BUILD, and accent status.
 *
 * The velocity computation pipeline:
 * 1. Determine base velocity from velocityFloor
 * 2. If accented, add accentBoost
 * 3. Apply BUILD modifiers (fill intensity, phrase position)
 * 4. Add random variation (from velocityVariation)
 * 5. Clamp to valid range
 *
 * @param punchParams Computed PUNCH parameters
 * @param buildMods Computed BUILD modifiers
 * @param isAccent Whether this step should be accented
 * @param seed Seed for deterministic randomness
 * @param step Step index for per-step variation
 * @return Computed velocity (0.0-1.0)
 */
float ComputeVelocity(const PunchParams& punchParams,
                      const BuildModifiers& buildMods,
                      bool isAccent,
                      uint32_t seed,
                      int step);

/**
 * Determine if a step should be accented based on accent mask and probability.
 *
 * Accents are controlled by:
 * 1. Accent eligibility mask (which steps CAN accent)
 * 2. Accent probability from PUNCH (how often eligible steps DO accent)
 *
 * @param step Step index (0-31)
 * @param accentMask Bitmask of accent-eligible steps
 * @param accentProbability Probability of accenting eligible steps (0.0-1.0)
 * @param seed Seed for deterministic randomness
 * @return true if step should be accented
 */
bool ShouldAccent(int step,
                  uint32_t accentMask,
                  float accentProbability,
                  uint32_t seed);

/**
 * Get default accent mask for a voice.
 *
 * Default accent masks emphasize musically strong positions:
 * - Anchor: Downbeats and quarter notes (steps 0, 4, 8, 12, 16, 20, 24, 28)
 * - Shimmer: Backbeats (steps 8, 24) and some offbeats
 *
 * @param voice Voice type (ANCHOR, SHIMMER, AUX)
 * @return Accent eligibility bitmask
 */
uint32_t GetDefaultAccentMask(Voice voice);

/**
 * Compute velocity for anchor voice with all parameters.
 *
 * Convenience function that combines PUNCH, BUILD, and accent computation.
 *
 * @param punch PUNCH parameter (0.0-1.0)
 * @param build BUILD parameter (0.0-1.0)
 * @param phraseProgress Current phrase progress (0.0-1.0)
 * @param step Step index (0-31)
 * @param seed Seed for deterministic randomness
 * @param accentMask Accent eligibility mask (0 = use default)
 * @return Computed velocity (0.0-1.0)
 */
float ComputeAnchorVelocity(float punch,
                            float build,
                            float phraseProgress,
                            int step,
                            uint32_t seed,
                            uint32_t accentMask = 0);

/**
 * Compute velocity for shimmer voice with all parameters.
 *
 * Convenience function that combines PUNCH, BUILD, and accent computation.
 *
 * @param punch PUNCH parameter (0.0-1.0)
 * @param build BUILD parameter (0.0-1.0)
 * @param phraseProgress Current phrase progress (0.0-1.0)
 * @param step Step index (0-31)
 * @param seed Seed for deterministic randomness
 * @param accentMask Accent eligibility mask (0 = use default)
 * @return Computed velocity (0.0-1.0)
 */
float ComputeShimmerVelocity(float punch,
                             float build,
                             float phraseProgress,
                             int step,
                             uint32_t seed,
                             uint32_t accentMask = 0);

} // namespace daisysp_idm_grids
