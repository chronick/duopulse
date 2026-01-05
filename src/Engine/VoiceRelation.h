#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * VoiceRelation: V5 Voice COMPLEMENT Relationship
 *
 * V5 uses a single COMPLEMENT mode where shimmer fills gaps in anchor pattern.
 * The DRIFT parameter controls placement variation within gaps:
 * - Low DRIFT (0-30%): Evenly spaced within gaps
 * - Mid DRIFT (30-70%): Weighted by shimmer step weights
 * - High DRIFT (70-100%): Seed-varied random within gaps
 *
 * Reference: docs/specs/main.md section 6.4, Task 30
 */

// =============================================================================
// Gap Detection Types
// =============================================================================

/// Maximum number of gaps that can be detected (RT-safe fixed allocation)
constexpr int kMaxGaps = 16;

/**
 * Gap: Represents a contiguous gap in the anchor pattern
 */
struct Gap
{
    int start;   ///< First step of gap (0-based)
    int length;  ///< Number of steps in gap
};

// =============================================================================
// V5 COMPLEMENT Relationship (Task 30)
// =============================================================================

/**
 * Find gaps in anchor mask
 *
 * Detects contiguous runs of empty steps in the anchor pattern.
 * Handles wrap-around: if pattern ends and starts with gaps, they're combined.
 *
 * @param anchorMask Anchor hit mask
 * @param patternLength Pattern length in steps
 * @param gaps Output array of gaps (must have space for kMaxGaps)
 * @return Number of gaps found
 */
int FindGaps(uint32_t anchorMask, int patternLength, Gap* gaps);

/**
 * Apply V5 COMPLEMENT voice relationship
 *
 * Places shimmer hits in gaps of the anchor pattern.
 * Hit distribution is proportional to gap length.
 * Placement strategy varies with DRIFT parameter.
 *
 * @param anchorMask Anchor hit mask (read-only)
 * @param shimmerWeights Per-step shimmer weights from archetype (0.0-1.0)
 * @param drift DRIFT parameter (0.0-1.0) controls placement strategy
 * @param seed Random seed for high-DRIFT variation
 * @param patternLength Pattern length in steps
 * @param targetHits Desired number of shimmer hits
 * @return Shimmer hit mask with hits placed in gaps
 */
uint32_t ApplyComplementRelationship(uint32_t anchorMask,
                                     const float* shimmerWeights,
                                     float drift, uint32_t seed,
                                     int patternLength, int targetHits);

// =============================================================================
// Legacy V4 Functions (deprecated, kept for compatibility)
// =============================================================================

/**
 * Apply voice relationship to shimmer mask based on anchor mask
 *
 * V5: This function now only supports INDEPENDENT mode.
 * Use ApplyComplementRelationship() for V5 COMPLEMENT behavior.
 *
 * @deprecated Use ApplyComplementRelationship() for V5
 */
void ApplyVoiceRelationship(uint32_t anchorMask,
                            uint32_t& shimmerMask,
                            VoiceCoupling coupling,
                            int patternLength);

/**
 * Apply voice relationship to aux mask
 *
 * Aux voice (hi-hat) relationship rules.
 * V5: Simplified to INDEPENDENT behavior only.
 *
 * @param anchorMask Anchor hit mask
 * @param shimmerMask Shimmer hit mask
 * @param auxMask Aux mask to modify
 * @param coupling Voice coupling mode (V5: ignored, always INDEPENDENT)
 * @param patternLength Pattern length
 */
void ApplyAuxRelationship(uint32_t anchorMask,
                          uint32_t shimmerMask,
                          uint32_t& auxMask,
                          VoiceCoupling coupling,
                          int patternLength);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Shift a mask left (delay) with wrap-around
 *
 * @param mask Original mask
 * @param shift Number of steps to shift
 * @param patternLength Pattern length for wrap
 * @return Shifted mask
 */
uint32_t ShiftMaskLeft(uint32_t mask, int shift, int patternLength);

/**
 * Shift a mask right (advance) with wrap-around
 *
 * @param mask Original mask
 * @param shift Number of steps to shift
 * @param patternLength Pattern length for wrap
 * @return Shifted mask
 */
uint32_t ShiftMaskRight(uint32_t mask, int shift, int patternLength);

/**
 * Find largest gap in a combined mask
 *
 * @param mask Combined anchor + shimmer mask
 * @param patternLength Pattern length
 * @return Size of largest gap (in steps)
 */
int FindLargestGap(uint32_t mask, int patternLength);

/**
 * Find the first step in a gap of a given size
 *
 * @param mask Combined mask
 * @param minGapSize Minimum gap size to find
 * @param patternLength Pattern length
 * @return First step of the gap, or -1 if none found
 */
int FindGapStart(uint32_t mask, int minGapSize, int patternLength);

} // namespace daisysp_idm_grids
