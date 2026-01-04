#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * VoiceRelation: Voice coupling and relationship logic
 *
 * Controls how voices interact with each other:
 * - INDEPENDENT: Voices fire freely, can overlap
 * - INTERLOCK: Suppress simultaneous hits, call-response feel
 * - SHADOW: Shimmer echoes anchor with 1-step delay
 *
 * Reference: docs/specs/main.md section 6.4
 */

// =============================================================================
// Voice Relationship Functions
// =============================================================================

/**
 * Apply voice relationship to shimmer mask based on anchor mask
 *
 * Modifies the shimmer hit mask according to the coupling mode:
 * - INDEPENDENT: No change to shimmer
 * - INTERLOCK: Remove shimmer hits that coincide with anchor
 * - SHADOW: Replace shimmer with delayed copy of anchor
 *
 * @param anchorMask Anchor hit mask (read-only)
 * @param shimmerMask Shimmer hit mask (will be modified)
 * @param coupling Voice coupling mode
 * @param patternLength Pattern length in steps
 */
void ApplyVoiceRelationship(uint32_t anchorMask,
                            uint32_t& shimmerMask,
                            VoiceCoupling coupling,
                            int patternLength);

/**
 * Apply interlock to shimmer mask
 *
 * Removes shimmer hits that coincide with anchor hits,
 * creating a call-response feel.
 *
 * @param anchorMask Anchor hit mask
 * @param shimmerMask Shimmer mask to modify
 * @param patternLength Pattern length
 */
void ApplyInterlock(uint32_t anchorMask, uint32_t& shimmerMask, int patternLength);

/**
 * Apply shadow effect to shimmer mask
 *
 * Makes shimmer echo anchor with a 1-step delay.
 * The original shimmer mask is replaced with delayed anchor.
 *
 * @param anchorMask Anchor hit mask
 * @param shimmerMask Shimmer mask to replace
 * @param patternLength Pattern length
 */
void ApplyShadow(uint32_t anchorMask, uint32_t& shimmerMask, int patternLength);

/**
 * Get voice coupling from config or archetype
 *
 * Archetype provides a default coupling value, but the config setting
 * can override it.
 *
 * @param configCoupling Coupling mode from config (VoiceCoupling enum)
 * @param archetypeDefault Archetype's default couple value (0.0-1.0)
 * @param useConfig Whether to use config value (true) or derive from archetype
 * @return Effective VoiceCoupling mode
 */
VoiceCoupling GetEffectiveCoupling(VoiceCoupling configCoupling,
                                   float archetypeDefault,
                                   bool useConfig);

/**
 * Apply gap-fill logic for INTERLOCK mode
 *
 * In INTERLOCK mode, if there's a large gap without any hits,
 * allow shimmer to fill the gap even if it would normally be suppressed.
 *
 * @param anchorMask Anchor hit mask
 * @param shimmerMask Shimmer mask (will be modified to add gap fills)
 * @param originalShimmer Original shimmer mask before interlock
 * @param maxGap Maximum allowed gap before filling
 * @param patternLength Pattern length
 */
void ApplyGapFill(uint32_t anchorMask,
                  uint32_t& shimmerMask,
                  uint32_t originalShimmer,
                  int maxGap,
                  int patternLength);

// =============================================================================
// Aux Voice Relationship
// =============================================================================

/**
 * Apply voice relationship to aux mask
 *
 * Aux voice (hi-hat) has simpler relationship rules:
 * - Can optionally avoid anchor hits
 * - Can be densified around shimmer hits
 *
 * @param anchorMask Anchor hit mask
 * @param shimmerMask Shimmer hit mask
 * @param auxMask Aux mask to modify
 * @param coupling Voice coupling mode
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
