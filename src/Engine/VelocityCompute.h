#pragma once

#include <cstdint>
#include "ControlState.h"
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * VelocityCompute: Velocity computation for DuoPulse v5
 *
 * Velocity is controlled by the ACCENT parameter (was: PUNCH), which sets
 * the dynamic contrast between accented and ghost hits. SHAPE (was: BUILD)
 * adds phrase-arc modulation, and accent masks determine which steps get emphasized.
 *
 * V5 Changes (Task 27):
 * - PunchParams renamed to AccentParams
 * - BuildModifiers renamed to ShapeModifiers
 * - ComputePunch renamed to ComputeAccent
 * - ComputeBuildModifiers renamed to ComputeShapeModifiers
 *
 * Reference: docs/specs/main.md section 7.2
 *
 * Note: ShapePhase enum is defined in ControlState.h to avoid circular dependency.
 */

// =============================================================================
// ACCENT Parameter Computation (V5: was PUNCH)
// =============================================================================

/**
 * Compute AccentParams from ACCENT knob value.
 *
 * V5 (Task 35): ACCENT controls metric weight-based velocity dynamics:
 * - ACCENT = 0%: Flat dynamics (all hits 80-88%)
 * - ACCENT = 100%: Wide dynamics (30-100%, downbeats loud, offbeats soft)
 *
 * @param accent ACCENT parameter value (0.0-1.0)
 * @param[out] params Output AccentParams struct
 */
void ComputeAccent(float accent, AccentParams& params);

/**
 * Compute velocity from ACCENT parameter and metric weight.
 *
 * V5 (Task 35): Position-aware velocity mapping:
 * 1. Get metric weight for step position
 * 2. Map weight to velocity range (floor to ceiling)
 * 3. Add micro-variation for human feel
 * 4. Clamp to valid range (0.30-1.0)
 *
 * @param accent ACCENT parameter (0.0-1.0)
 * @param step Step index (0-patternLength-1)
 * @param patternLength Pattern length (typically 16)
 * @param seed Seed for deterministic micro-variation
 * @return Computed velocity (0.30-1.0)
 */
float ComputeAccentVelocity(float accent, int step, int patternLength, uint32_t seed);

// =============================================================================
// SHAPE Parameter Computation (V5: was BUILD)
// =============================================================================

/**
 * Compute ShapeModifiers from SHAPE value and phrase position.
 *
 * SHAPE controls the narrative arc of each phrase:
 * - SHAPE = 0%: Flat throughout (no builds, no fills)
 * - SHAPE = 50%: Subtle build (slight density increase, fills at end)
 * - SHAPE = 100%: Dramatic arc (big builds, intense fills)
 *
 * V5 Change: Renamed from ComputeBuildModifiers to ComputeShapeModifiers (Task 27)
 *
 * @param shape SHAPE parameter value (0.0-1.0)
 * @param phraseProgress Current phrase progress (0.0-1.0)
 * @param[out] modifiers Output ShapeModifiers struct
 */
void ComputeShapeModifiers(float shape, float phraseProgress, ShapeModifiers& modifiers);

// =============================================================================
// Velocity Computation
// =============================================================================

/**
 * Compute velocity for a step based on ACCENT, SHAPE, and accent status.
 *
 * The velocity computation pipeline:
 * 1. Determine base velocity from velocityFloor
 * 2. If accented, add accentBoost
 * 3. Apply SHAPE modifiers (fill intensity, phrase position)
 * 4. Add random variation (from velocityVariation)
 * 5. Clamp to valid range
 *
 * @param accentParams Computed ACCENT parameters
 * @param shapeMods Computed SHAPE modifiers
 * @param isAccent Whether this step should be accented
 * @param seed Seed for deterministic randomness
 * @param step Step index for per-step variation
 * @return Computed velocity (0.0-1.0)
 */
float ComputeVelocity(const AccentParams& accentParams,
                      const ShapeModifiers& shapeMods,
                      bool isAccent,
                      uint32_t seed,
                      int step,
                      int patternLength = 16);

/**
 * Determine if a step should be accented based on accent mask and probability.
 *
 * Accents are controlled by:
 * 1. Accent eligibility mask (which steps CAN accent)
 * 2. Accent probability from ACCENT (how often eligible steps DO accent)
 * 3. SHAPE forceAccents flag (FILL phase at high SHAPE forces all accents)
 *
 * @param step Step index (0-31)
 * @param accentMask Bitmask of accent-eligible steps
 * @param accentProbability Probability of accenting eligible steps (0.0-1.0)
 * @param shapeMods SHAPE modifiers (for forceAccents flag)
 * @param seed Seed for deterministic randomness
 * @return true if step should be accented
 */
bool ShouldAccent(int step,
                  uint64_t accentMask,
                  float accentProbability,
                  const ShapeModifiers& shapeMods,
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
uint64_t GetDefaultAccentMask(Voice voice);

/**
 * Compute velocity for anchor voice with all parameters.
 *
 * Convenience function that combines ACCENT, SHAPE, and accent computation.
 *
 * V5 Change: Parameters renamed from punch/build to accent/shape (Task 27)
 *
 * @param accent ACCENT parameter (0.0-1.0)
 * @param shape SHAPE parameter (0.0-1.0)
 * @param phraseProgress Current phrase progress (0.0-1.0)
 * @param step Step index (0-31)
 * @param seed Seed for deterministic randomness
 * @param accentMask Accent eligibility mask (0 = use default)
 * @return Computed velocity (0.0-1.0)
 */
float ComputeAnchorVelocity(float accent,
                            float shape,
                            float phraseProgress,
                            int step,
                            int patternLength = 16,
                            uint32_t seed = 0,
                            uint64_t accentMask = 0);

/**
 * Compute velocity for shimmer voice with all parameters.
 *
 * Convenience function that combines ACCENT, SHAPE, and accent computation.
 *
 * V5 Change: Parameters renamed from punch/build to accent/shape (Task 27)
 *
 * @param accent ACCENT parameter (0.0-1.0)
 * @param shape SHAPE parameter (0.0-1.0)
 * @param phraseProgress Current phrase progress (0.0-1.0)
 * @param step Step index (0-31)
 * @param seed Seed for deterministic randomness
 * @param accentMask Accent eligibility mask (0 = use default)
 * @return Computed velocity (0.0-1.0)
 */
float ComputeShimmerVelocity(float accent,
                             float shape,
                             float phraseProgress,
                             int step,
                             int patternLength = 16,
                             uint32_t seed = 0,
                             uint64_t accentMask = 0);

// =============================================================================
// Legacy Function Aliases (for backward compatibility)
// =============================================================================

/// Legacy alias for ComputeAccent (V5: renamed from ComputePunch)
inline void ComputePunch(float punch, AccentParams& params)
{
    ComputeAccent(punch, params);
}

/// Legacy alias for ComputeShapeModifiers (V5: renamed from ComputeBuildModifiers)
inline void ComputeBuildModifiers(float build, float phraseProgress, ShapeModifiers& modifiers)
{
    ComputeShapeModifiers(build, phraseProgress, modifiers);
}

} // namespace daisysp_idm_grids
