#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * GumbelSampler: Weighted sampling without replacement using Gumbel noise
 *
 * Gumbel Top-K selection provides deterministic, seeded hit selection
 * with the ability to respect spacing rules to prevent clumping.
 *
 * The key insight: argmax(log(weight) + Gumbel_noise) gives weighted sampling.
 * Using consistent seeds makes this deterministic.
 *
 * Reference: docs/specs/main.md section 6.3
 */

// =============================================================================
// Constants
// =============================================================================

/// Minimum spacing between hits (in steps) for tight patterns
constexpr int kMinSpacingTight = 1;

/// Minimum spacing between hits for loose patterns
constexpr int kMinSpacingLoose = 2;

/// Maximum hits that can be selected in one call
constexpr int kMaxSelectableHits = 16;

// =============================================================================
// Hash Functions
// =============================================================================

/**
 * Convert a seed and step index to a deterministic float in [0, 1)
 *
 * Uses a fast hash function to generate consistent pseudo-random values.
 *
 * @param seed Base seed
 * @param step Step index
 * @return Float in range [0, 1)
 */
float HashToFloat(uint32_t seed, int step);

/**
 * Generate Gumbel noise from a uniform random value
 *
 * Gumbel distribution: -log(-log(u)) where u is uniform (0,1)
 *
 * @param uniform Uniform random value in (0, 1)
 * @return Gumbel-distributed value
 */
float UniformToGumbel(float uniform);

// =============================================================================
// Selection Functions
// =============================================================================

/**
 * Select hits using Gumbel Top-K sampling with spacing rules
 *
 * This is the main selection function. It:
 * 1. Computes log(weight) + Gumbel noise for each eligible step
 * 2. Selects the K steps with highest scores
 * 3. Enforces minimum spacing between selected steps
 *
 * @param weights Step weights (0.0-1.0 for each step)
 * @param eligibilityMask Bitmask of steps that can be selected
 * @param targetCount Number of hits to select
 * @param seed Random seed for deterministic selection
 * @param patternLength Number of steps in pattern
 * @param minSpacing Minimum steps between selected hits (0 = no constraint)
 * @return Bitmask of selected steps
 */
uint64_t SelectHitsGumbelTopK(const float* weights,
                               uint64_t eligibilityMask,
                               int targetCount,
                               uint32_t seed,
                               int patternLength,
                               int minSpacing);

/**
 * Simplified selection without spacing rules
 *
 * @param weights Step weights
 * @param eligibilityMask Eligible steps
 * @param targetCount Hits to select
 * @param seed Random seed
 * @param patternLength Pattern length
 * @return Selected step mask
 */
uint64_t SelectHitsGumbelSimple(const float* weights,
                                 uint64_t eligibilityMask,
                                 int targetCount,
                                 uint32_t seed,
                                 int patternLength);

// =============================================================================
// Score Computation
// =============================================================================

/**
 * Compute Gumbel scores for all steps
 *
 * Score = log(weight + epsilon) + Gumbel(seed, step)
 *
 * @param weights Step weights (0.0-1.0)
 * @param seed Random seed
 * @param patternLength Pattern length
 * @param outScores Output array (must hold patternLength elements)
 */
void ComputeGumbelScores(const float* weights,
                         uint32_t seed,
                         int patternLength,
                         float* outScores);

/**
 * Find the step with the highest score that respects spacing
 *
 * @param scores Step scores
 * @param eligibilityMask Eligible steps (modified to exclude selected)
 * @param selectedMask Already selected steps
 * @param patternLength Pattern length
 * @param minSpacing Minimum spacing from selected steps
 * @return Best step index, or -1 if none available
 */
int FindBestStep(const float* scores,
                 uint64_t eligibilityMask,
                 uint64_t selectedMask,
                 int patternLength,
                 int minSpacing);

// =============================================================================
// Spacing Helpers
// =============================================================================

/**
 * Compute a mask of steps that violate spacing from a given step
 *
 * @param step The reference step
 * @param minSpacing Minimum required spacing
 * @param patternLength Pattern length
 * @return Mask of steps within minSpacing of the reference
 */
uint64_t GetSpacingExclusionMask(int step, int minSpacing, int patternLength);

/**
 * Check if a step selection satisfies spacing constraints
 *
 * @param selectedMask Currently selected steps
 * @param candidateStep Step being considered
 * @param minSpacing Minimum spacing requirement
 * @param patternLength Pattern length
 * @return true if candidate respects spacing from all selected steps
 */
bool CheckSpacingValid(uint64_t selectedMask, int candidateStep, int minSpacing, int patternLength);

/**
 * Get minimum spacing based on energy zone
 *
 * Lower energy = more spacing required (sparser patterns)
 * Higher energy = less spacing required (denser patterns)
 *
 * @param zone Current energy zone
 * @return Minimum spacing in steps
 */
int GetMinSpacingForZone(EnergyZone zone);

} // namespace daisysp_idm_grids
