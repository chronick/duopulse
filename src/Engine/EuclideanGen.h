#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * Euclidean Rhythm Generator (Task 21 Phase F)
 *
 * Provides genre-aware Euclidean rhythm blending to guarantee musical
 * foundations at low Field X (syncopation axis) positions.
 *
 * The Euclidean algorithm (Bjorklund) distributes k hits as evenly as
 * possible across n steps, producing patterns like:
 * - E(4,16) = [x...x...x...x...] (4-on-floor kick)
 * - E(3,8)  = [x..x..x.]         (son clave)
 * - E(5,12) = [x..x.x..x.x.]     (jazz ride)
 *
 * Reference: docs/specs/main.md section 6.3.1
 */

// =============================================================================
// Euclidean Pattern Generation
// =============================================================================

/**
 * Generate Euclidean rhythm pattern using Bjorklund algorithm.
 *
 * Distributes 'hits' as evenly as possible across 'steps'.
 * Returns a bitmask where bit i = 1 means hit at step i.
 *
 * @param hits Number of hits to distribute (0-32)
 * @param steps Pattern length in steps (1-32)
 * @return Bitmask representing the Euclidean pattern
 *
 * Examples:
 * - GenerateEuclidean(4, 16) = 0x11111111 (every 4th step)
 * - GenerateEuclidean(3, 8)  = 0x25       (son clave: 100100100)
 * - GenerateEuclidean(5, 8)  = 0xAA       (10101010)
 */
uint64_t GenerateEuclidean(int hits, int steps);

/**
 * Rotate a pattern by offset steps (deterministic rotation).
 *
 * @param pattern Bitmask pattern to rotate
 * @param offset Number of steps to rotate right (can be negative for left)
 * @param steps Pattern length (needed for wrap-around)
 * @return Rotated pattern bitmask
 *
 * Example:
 * - RotatePattern(0x11, 1, 8) = 0x88  (00010001 -> 10001000)
 */
uint64_t RotatePattern(uint64_t pattern, int offset, int steps);

// =============================================================================
// Euclidean + Probabilistic Weight Blending
// =============================================================================

/**
 * Blend Euclidean foundation with probabilistic weight selection.
 *
 * At euclideanRatio = 0.0: Pure Gumbel Top-K selection from weights
 * At euclideanRatio = 1.0: Pure Euclidean pattern (weights ignored)
 * At 0.0 < ratio < 1.0: Hybrid blending:
 *   1. Generate Euclidean pattern E(budget, steps)
 *   2. Rotate by seed-derived offset
 *   3. Blend with Gumbel-sampled weights using ratio
 *
 * Blending strategy:
 * - For each step in the Euclidean pattern, "reserve" that step
 * - Fill remaining hits from Gumbel Top-K selection on eligible steps
 *
 * @param budget Number of hits to place (from hit budget calculation)
 * @param steps Pattern length (16/24/32, or 32 for 64-step half)
 * @param weights Per-step weights (from blended archetype)
 * @param eligibility Bitmask of eligible steps (1 = can have hit)
 * @param euclideanRatio Blend ratio (0.0 = pure weights, 1.0 = pure Euclidean)
 * @param seed RNG seed for Gumbel noise and rotation
 * @return Bitmask of selected hit steps
 */
uint64_t BlendEuclideanWithWeights(
    int budget,
    int steps,
    const float* weights,
    uint64_t eligibility,
    float euclideanRatio,
    uint32_t seed);

// =============================================================================
// Genre-Specific Euclidean Ratios
// =============================================================================

/**
 * Get genre-specific Euclidean blend ratio.
 *
 * Ratios taper with Field X (syncopation axis):
 * - Field X = 0.0 (straight): High Euclidean ratio (structured)
 * - Field X = 1.0 (syncopated): Low Euclidean ratio (probabilistic)
 *
 * Genre-specific base ratios at Field X = 0:
 * - Techno: 70% (ensures 4-on-floor kick at low complexity)
 * - Tribal: 40% (balances structure with polyrhythm)
 * - IDM: 0% (always probabilistic, maximum irregularity)
 *
 * Ratios reduce by ~70% as Field X increases from 0 to 1.
 *
 * Only active in MINIMAL and GROOVE zones. Returns 0 for BUILD/PEAK.
 *
 * At very low SHAPE (<=0.05), returns 1.0 for pure euclidean mode.
 * This enables four-on-floor patterns where euclidean(64,16) = quarter notes.
 *
 * @param genre Current genre setting
 * @param fieldX Field X position (0.0-1.0, syncopation axis)
 * @param zone Current energy zone
 * @param shape SHAPE parameter (0.0-1.0), optional, default 0.5
 * @return Euclidean blend ratio (0.0-1.0)
 */
float GetGenreEuclideanRatio(Genre genre, float fieldX, EnergyZone zone, float shape = 0.5f);

} // namespace daisysp_idm_grids
