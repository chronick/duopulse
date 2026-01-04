#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * HitBudget: Target hit counts and eligibility masks for pattern generation
 *
 * Hit budgets guarantee density matches intent. Budget is calculated from
 * ENERGY + BALANCE + zone, then BUILD modifiers adjust for phrase position.
 *
 * Reference: docs/specs/main.md section 6.1 and 6.2
 */

// =============================================================================
// Eligibility Mask Constants (for 32-step patterns)
// =============================================================================

/// Downbeats: steps 0, 16 (bar starts)
constexpr uint32_t kDownbeatMask = 0x00010001;

/// Quarter notes: steps 0, 4, 8, 12, 16, 20, 24, 28
constexpr uint32_t kQuarterNoteMask = 0x11111111;

/// 8th notes: all even steps
constexpr uint32_t kEighthNoteMask = 0x55555555;

/// 16th notes: all steps
constexpr uint32_t kSixteenthNoteMask = 0xFFFFFFFF;

/// Backbeats: steps 8, 24 (snare positions in 4/4)
constexpr uint32_t kBackbeatMask = 0x01000100;

/// Off-beats: odd 8th notes (steps 2, 6, 10, 14, 18, 22, 26, 30)
constexpr uint32_t kOffbeatMask = 0x44444444;

/// Syncopated positions: "e" and "a" of beat (steps 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31)
constexpr uint32_t kSyncopationMask = 0xAAAAAAAA;

// =============================================================================
// BarBudget Structure
// =============================================================================

/**
 * BarBudget: Target hit counts for a single bar
 *
 * Contains the target number of hits for each voice, as well as
 * the eligibility mask defining which steps can potentially fire.
 */
struct BarBudget
{
    /// Target hit count for anchor voice (1-16 typical)
    int anchorHits;

    /// Target hit count for shimmer voice (1-16 typical)
    int shimmerHits;

    /// Target hit count for aux voice (0-16 typical)
    int auxHits;

    /// Which steps are eligible for anchor hits (bitmask)
    uint32_t anchorEligibility;

    /// Which steps are eligible for shimmer hits (bitmask)
    uint32_t shimmerEligibility;

    /// Which steps are eligible for aux hits (bitmask)
    uint32_t auxEligibility;

    /**
     * Initialize with default values (minimal pattern)
     */
    void Init()
    {
        anchorHits  = 4;
        shimmerHits = 2;
        auxHits     = 4;

        anchorEligibility  = kQuarterNoteMask;
        shimmerEligibility = kBackbeatMask;
        auxEligibility     = kEighthNoteMask;
    }
};

// =============================================================================
// Budget Computation Functions
// =============================================================================

/**
 * Compute hit budget for anchor voice based on energy and zone
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param patternLength Steps per bar (16, 24, 32, or 64)
 * @return Target number of anchor hits
 */
int ComputeAnchorBudget(float energy, EnergyZone zone, int patternLength);

/**
 * Compute hit budget for shimmer voice based on energy, zone, and balance
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param balance BALANCE parameter (0.0-1.0, 0=anchor-heavy, 1=shimmer-heavy)
 * @param zone Current energy zone
 * @param patternLength Steps per bar
 * @return Target number of shimmer hits
 */
int ComputeShimmerBudget(float energy, float balance, EnergyZone zone, int patternLength);

/**
 * Compute hit budget for aux voice
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param auxDensity AUX density multiplier setting
 * @param patternLength Steps per bar
 * @return Target number of aux hits
 */
int ComputeAuxBudget(float energy, EnergyZone zone, AuxDensity auxDensity, int patternLength);

/**
 * Compute complete bar budget from control state
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param balance BALANCE parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param auxDensity AUX density setting
 * @param patternLength Steps per bar
 * @param buildMultiplier Density multiplier from BUILD (1.0 = no change)
 * @param outBudget Output budget struct
 */
void ComputeBarBudget(float energy,
                      float balance,
                      EnergyZone zone,
                      AuxDensity auxDensity,
                      int patternLength,
                      float buildMultiplier,
                      BarBudget& outBudget);

// =============================================================================
// Eligibility Mask Functions
// =============================================================================

/**
 * Compute eligibility mask for anchor voice
 *
 * Higher energy unlocks more metric positions. FLAVOR adds syncopation.
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param flavor FLAVOR parameter (0.0-1.0, affects syncopation)
 * @param zone Current energy zone
 * @param patternLength Steps per bar
 * @return Bitmask of eligible steps
 */
uint32_t ComputeAnchorEligibility(float energy, float flavor, EnergyZone zone, int patternLength);

/**
 * Compute eligibility mask for shimmer voice
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param flavor FLAVOR parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param patternLength Steps per bar
 * @return Bitmask of eligible steps
 */
uint32_t ComputeShimmerEligibility(float energy, float flavor, EnergyZone zone, int patternLength);

/**
 * Compute eligibility mask for aux voice
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param flavor FLAVOR parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param patternLength Steps per bar
 * @return Bitmask of eligible steps
 */
uint32_t ComputeAuxEligibility(float energy, float flavor, EnergyZone zone, int patternLength);

/**
 * Apply fill boost to a budget (increases hits during fill zones)
 *
 * @param budget Budget to modify
 * @param fillIntensity Fill intensity (0.0-1.0)
 * @param fillMultiplier Archetype's fill density multiplier
 * @param patternLength Steps per bar
 */
void ApplyFillBoost(BarBudget& budget,
                    float fillIntensity,
                    float fillMultiplier,
                    int patternLength);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Count set bits in a 32-bit mask
 *
 * @param mask Bitmask
 * @return Number of set bits
 */
int CountBits(uint32_t mask);

/**
 * Limit pattern length to 32 bits for mask operations
 *
 * @param patternLength Original pattern length
 * @return Clamped pattern length (max 32)
 */
int ClampPatternLength(int patternLength);

} // namespace daisysp_idm_grids
