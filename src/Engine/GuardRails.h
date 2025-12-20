#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"
#include "HitBudget.h"

namespace daisysp_idm_grids
{

/**
 * GuardRails: Pattern constraint enforcement
 *
 * Guard rails ensure musical output regardless of parameter settings.
 * They operate in two phases:
 * 1. Soft repair: Bias rescue steps if constraints nearly violated
 *    (swap weakest hit for strongest rescue candidate)
 * 2. Hard guard rails: Force corrections only if still violating
 *
 * Reference: docs/specs/main.md section 6.5 and 6.6
 */

// =============================================================================
// Constants
// =============================================================================

/// Maximum gap (in steps) before forcing an anchor hit in GROOVE+ zones
constexpr int kMaxGapGroove = 8;

/// Maximum gap in BUILD zone
constexpr int kMaxGapBuild = 6;

/// Maximum gap in PEAK zone
constexpr int kMaxGapPeak = 4;

/// Maximum consecutive shimmer hits without anchor (non-PEAK)
constexpr int kMaxConsecutiveShimmer = 4;

/// Maximum consecutive shimmer in PEAK zone
constexpr int kMaxConsecutiveShimmerPeak = 6;

// =============================================================================
// Soft Repair Functions
// =============================================================================

/**
 * Soft repair pass: proactive constraint satisfaction
 *
 * If a constraint is nearly violated (e.g., gap is close to max),
 * this function swaps the weakest hit for a rescue candidate that
 * would prevent the violation. This preserves the hit count while
 * improving the pattern.
 *
 * @param anchorMask Anchor hit mask (will be modified)
 * @param shimmerMask Shimmer hit mask (will be modified)
 * @param anchorWeights Weight of each anchor hit (for finding weakest)
 * @param shimmerWeights Weight of each shimmer hit
 * @param zone Current energy zone (affects thresholds)
 * @param patternLength Pattern length in steps
 * @return Number of repairs made
 */
int SoftRepairPass(uint32_t& anchorMask,
                   uint32_t& shimmerMask,
                   const float* anchorWeights,
                   const float* shimmerWeights,
                   EnergyZone zone,
                   int patternLength);

/**
 * Find weakest hit in a mask based on weights
 *
 * @param mask Hit mask
 * @param weights Step weights
 * @param patternLength Pattern length
 * @return Step index of weakest hit, or -1 if no hits
 */
int FindWeakestHit(uint32_t mask, const float* weights, int patternLength);

/**
 * Find best rescue candidate for a constraint violation
 *
 * @param mask Current hit mask
 * @param rescueMask Mask of acceptable rescue positions
 * @param weights Step weights (higher = better candidate)
 * @param patternLength Pattern length
 * @return Step index of best rescue, or -1 if none found
 */
int FindRescueCandidate(uint32_t mask,
                        uint32_t rescueMask,
                        const float* weights,
                        int patternLength);

// =============================================================================
// Hard Guard Rails
// =============================================================================

/**
 * Apply hard guard rails (final constraint enforcement)
 *
 * These are last-resort corrections that ensure basic musicality:
 * - Downbeat protection: force anchor on beat 1 if missing
 * - Max gap rule: no more than N steps without anchor
 * - Max consecutive shimmer: limit shimmer bursts
 * - Genre-specific floors (e.g., techno backbeat)
 *
 * @param anchorMask Anchor hit mask (will be modified)
 * @param shimmerMask Shimmer hit mask (will be modified)
 * @param zone Current energy zone
 * @param genre Current genre (affects some rules)
 * @param patternLength Pattern length
 * @return Number of corrections made
 */
int ApplyHardGuardRails(uint32_t& anchorMask,
                        uint32_t& shimmerMask,
                        EnergyZone zone,
                        Genre genre,
                        int patternLength);

/**
 * Enforce downbeat protection
 *
 * In GROOVE+ zones, ensures anchor fires on step 0 (downbeat).
 *
 * @param anchorMask Anchor mask to modify
 * @param zone Energy zone
 * @param patternLength Pattern length
 * @return true if downbeat was forced
 */
bool EnforceDownbeat(uint32_t& anchorMask, EnergyZone zone, int patternLength);

/**
 * Enforce maximum gap rule
 *
 * Adds anchor hits to break up gaps that exceed the zone's maximum.
 *
 * @param anchorMask Anchor mask to modify
 * @param zone Energy zone (affects max gap)
 * @param patternLength Pattern length
 * @return Number of hits added
 */
int EnforceMaxGap(uint32_t& anchorMask, EnergyZone zone, int patternLength);

/**
 * Enforce consecutive shimmer limit
 *
 * Prevents too many shimmer hits without an anchor hit.
 *
 * @param anchorMask Anchor mask (read-only)
 * @param shimmerMask Shimmer mask to modify
 * @param zone Energy zone (affects limit)
 * @param patternLength Pattern length
 * @return Number of shimmer hits removed
 */
int EnforceConsecutiveShimmer(uint32_t anchorMask,
                               uint32_t& shimmerMask,
                               EnergyZone zone,
                               int patternLength);

/**
 * Enforce genre-specific rules
 *
 * - Techno: Encourage backbeat in GROOVE+ zones
 *
 * @param anchorMask Anchor mask
 * @param shimmerMask Shimmer mask to modify
 * @param genre Current genre
 * @param zone Energy zone
 * @param patternLength Pattern length
 * @return Number of modifications made
 */
int EnforceGenreRules(uint32_t anchorMask,
                       uint32_t& shimmerMask,
                       Genre genre,
                       EnergyZone zone,
                       int patternLength);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Get maximum allowed gap for a zone
 *
 * @param zone Energy zone
 * @return Maximum gap in steps
 */
int GetMaxGapForZone(EnergyZone zone);

/**
 * Get maximum consecutive shimmer for a zone
 *
 * @param zone Energy zone
 * @return Maximum consecutive shimmer hits
 */
int GetMaxConsecutiveShimmerForZone(EnergyZone zone);

/**
 * Find gaps in a mask and return a mask of gap midpoints
 *
 * Useful for determining where to add rescue hits.
 *
 * @param mask Hit mask
 * @param minGapSize Minimum gap size to consider
 * @param patternLength Pattern length
 * @return Mask of gap midpoints
 */
uint32_t FindGapMidpoints(uint32_t mask, int minGapSize, int patternLength);

/**
 * Count consecutive shimmer hits without anchor
 *
 * @param anchorMask Anchor hit mask
 * @param shimmerMask Shimmer hit mask
 * @param patternLength Pattern length
 * @return Maximum consecutive shimmer run length
 */
int CountMaxConsecutiveShimmer(uint32_t anchorMask,
                               uint32_t shimmerMask,
                               int patternLength);

} // namespace daisysp_idm_grids
