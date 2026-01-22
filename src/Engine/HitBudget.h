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
// Eligibility Mask Constants (for 64-step patterns)
// =============================================================================

/// Downbeats: steps 0, 16 (bar starts)
constexpr uint64_t kDownbeatMask = 0x0001000100010001ULL;

/// Quarter notes: steps 0, 4, 8, 12, 16, 20, 24, 28
constexpr uint64_t kQuarterNoteMask = 0x1111111111111111ULL;

/// 8th notes: all even steps
constexpr uint64_t kEighthNoteMask = 0x5555555555555555ULL;

/// 16th notes: all steps
constexpr uint64_t kSixteenthNoteMask = 0xFFFFFFFFFFFFFFFFULL;

/// Backbeats: steps 8, 24 (snare positions in 4/4)
constexpr uint64_t kBackbeatMask = 0x0100010001000100ULL;

/// Off-beats: odd 8th notes (steps 2, 6, 10, 14, 18, 22, 26, 30)
constexpr uint64_t kOffbeatMask = 0x4444444444444444ULL;

/// Syncopated positions: "e" and "a" of beat (steps 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31)
constexpr uint64_t kSyncopationMask = 0xAAAAAAAAAAAAAAAAULL;

/// Anticipation positions only: just before each beat (steps 3,7,11,15,19,23,27,31,...)
/// These create forward motion without excessive syncopation
constexpr uint64_t kAnticipationMask = 0x8888888888888888ULL;

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
    uint64_t anchorEligibility;

    /// Which steps are eligible for shimmer hits (bitmask)
    uint64_t shimmerEligibility;

    /// Which steps are eligible for aux hits (bitmask)
    uint64_t auxEligibility;

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
// Euclidean K / HitBudget Fade System (Task 73)
// =============================================================================

/**
 * Compute euclidean K for anchor voice from ENERGY
 * Uses kAnchorKMin/kAnchorKMax from algorithm_config.h
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param patternLength Steps per bar
 * @return Euclidean K value (number of evenly-spaced hits)
 */
int ComputeAnchorEuclideanK(float energy, int patternLength);

/**
 * Compute euclidean K for shimmer voice from ENERGY
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param patternLength Steps per bar
 * @return Euclidean K value
 */
int ComputeShimmerEuclideanK(float energy, int patternLength);

/**
 * Compute effective hit count by fading between euclidean K and budget
 * based on SHAPE parameter.
 *
 * At SHAPE <= 0.05 AND ENERGY <= 0.05: quarter-note floor (Four on Floor mode)
 * At SHAPE <= 0.15: use minimum of euclideanK and budgetK (preserve sparsity)
 * At SHAPE = 1.0: pure budget-based (density-driven)
 *
 * This ensures SHAPE=0 + ENERGY=0 produces clean four-on-floor patterns,
 * while SHAPE=0 + ENERGY>0.05 produces sparse euclidean patterns.
 *
 * @param euclideanK Hit count from euclidean algorithm
 * @param budgetK Hit count from density-based budget
 * @param shape SHAPE parameter (0.0-1.0)
 * @param energy ENERGY parameter (0.0-1.0), used for four-on-floor detection
 * @param patternLength Pattern length in steps (needed for quarter-note floor calculation)
 * @return Effective hit count (blended based on SHAPE)
 */
int ComputeEffectiveHitCount(int euclideanK, int budgetK, float shape, float energy, int patternLength);

// =============================================================================
// Budget Computation Functions
// =============================================================================

/**
 * Compute hit budget for anchor voice based on energy and zone
 *
 * At SHAPE <= 0.15: Returns euclidean K (grid-locked four-on-floor)
 * At SHAPE > 0.15: Fades toward density-based budget (Task 73)
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param patternLength Steps per bar (16, 24, 32, or 64)
 * @param shape SHAPE parameter (0.0-1.0), affects density via zone
 * @return Target number of anchor hits
 */
int ComputeAnchorBudget(float energy, EnergyZone zone, int patternLength, float shape = 0.5f);

/**
 * Compute hit budget for shimmer voice based on energy, zone, and balance
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param balance BALANCE parameter (0.0-1.0, 0=anchor-heavy, 1=shimmer-heavy)
 * @param zone Current energy zone
 * @param patternLength Steps per bar
 * @param shape SHAPE parameter (0.0-1.0), affects density via zone
 * @return Target number of shimmer hits
 */
int ComputeShimmerBudget(float energy, float balance, EnergyZone zone, int patternLength, float shape = 0.5f);

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
 * @param shape SHAPE parameter (0.0-1.0), affects density via zone
 * @param outBudget Output budget struct
 */
void ComputeBarBudget(float energy,
                      float balance,
                      EnergyZone zone,
                      AuxDensity auxDensity,
                      int patternLength,
                      float buildMultiplier,
                      float shape,
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
uint64_t ComputeAnchorEligibility(float energy, float flavor, EnergyZone zone, int patternLength);

/**
 * Compute eligibility mask for shimmer voice
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param flavor FLAVOR parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param patternLength Steps per bar
 * @return Bitmask of eligible steps
 */
uint64_t ComputeShimmerEligibility(float energy, float flavor, EnergyZone zone, int patternLength);

/**
 * Compute eligibility mask for aux voice
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param flavor FLAVOR parameter (0.0-1.0)
 * @param zone Current energy zone
 * @param patternLength Steps per bar
 * @return Bitmask of eligible steps
 */
uint64_t ComputeAuxEligibility(float energy, float flavor, EnergyZone zone, int patternLength);

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
int CountBits(uint64_t mask);

/**
 * Limit pattern length to 32 bits for mask operations
 *
 * @param patternLength Original pattern length
 * @return Clamped pattern length (max 32)
 */
int ClampPatternLength(int patternLength);

/**
 * Get anchor budget multiplier based on SHAPE zone
 *
 * V5 Spec 5.4:
 * - Stable (0-30%): 100% (1.0)
 * - Syncopated (30-70%): 90-100% (lerp 1.0 -> 0.90)
 * - Wild (70-100%): 80-90% (lerp 0.90 -> 0.80)
 *
 * @param shape SHAPE parameter (0.0-1.0)
 * @return Multiplier for anchor hit budget (0.80-1.0)
 */
float GetAnchorBudgetMultiplier(float shape);

/**
 * Get shimmer budget multiplier based on SHAPE zone
 *
 * V5 Spec 5.4:
 * - Stable (0-30%): 100% (1.0)
 * - Syncopated (30-70%): 110-130% (lerp 1.10 -> 1.30)
 * - Wild (70-100%): 130-150% (lerp 1.30 -> 1.50)
 *
 * @param shape SHAPE parameter (0.0-1.0)
 * @return Multiplier for shimmer hit budget (1.0-1.5)
 */
float GetShimmerBudgetMultiplier(float shape);

} // namespace daisysp_idm_grids
