#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"
#include "PatternField.h"  // For PatternFieldConfig

namespace daisysp_idm_grids
{

/**
 * PatternGenerator: Pure function API for pattern generation
 *
 * This module provides a stateless, deterministic interface for generating
 * drum patterns. Designed to be shared between firmware and visualization tools.
 *
 * Key properties:
 * - Pure functions with no side effects
 * - Same inputs always produce identical outputs (deterministic)
 * - No heap allocations (RT audio safe)
 * - No dependencies on external state
 *
 * Reference: docs/specs/main.md section 6
 */

// =============================================================================
// Input Parameters
// =============================================================================

/**
 * Input parameters for pattern generation (pure data, no state)
 *
 * All parameters are normalized to 0.0-1.0 range for consistency.
 * Core parameters are shared between firmware and visualization tools.
 * Firmware-specific options have defaults that match viz tool behavior.
 */
struct PatternParams
{
    // =========================================================================
    // Core parameters (required, shared with viz tool)
    // =========================================================================

    float energy = 0.50f;       ///< Hit density (0.0-1.0)
    float shape = 0.30f;        ///< Pattern character (0.0-1.0)
    float axisX = 0.50f;        ///< Beat position bias (0.0-1.0)
    float axisY = 0.50f;        ///< Intricacy bias (0.0-1.0)
    float drift = 0.00f;        ///< Shimmer placement variation (0.0-1.0)
    float accent = 0.50f;       ///< Velocity dynamics (0.0-1.0)
    uint32_t seed = 0xDEADBEEF; ///< Pattern seed for determinism
    int patternLength = 32;     ///< Steps per pattern (16, 32, or 64)

    // =========================================================================
    // Firmware-specific options (with viz-compatible defaults)
    // =========================================================================

    /// Balance parameter for anchor/shimmer ratio in hit budget
    /// Default 0.5 = equal balance (viz tool behavior)
    float balance = 0.50f;

    /// Density multiplier for hit budget (from SHAPE modifiers)
    /// Default 1.0 = no boost (viz tool behavior)
    float densityMultiplier = 1.0f;

    /// Fill zone state - when true, applies fill boost
    bool inFillZone = false;

    /// Fill intensity for density boost (0.0-1.0)
    float fillIntensity = 0.0f;

    /// Fill density multiplier (how much fill boosts density)
    float fillDensityMultiplier = 1.5f;

    /// Fill progress (0.0-1.0, position within fill for ramping effects)
    /// Used by GenerateFillPattern() to compute density/velocity curves
    float fillProgress = 0.0f;

    /// Euclidean blend ratio (0.0 = pure Gumbel, 1.0 = pure Euclidean)
    /// Default 0.0 = no Euclidean (viz tool behavior)
    float euclideanRatio = 0.0f;

    /// Genre for guard rails
    Genre genre = Genre::TECHNO;

    /// AuxDensity setting
    AuxDensity auxDensity = AuxDensity::NORMAL;

    /// Enable soft repair pass (firmware uses true)
    bool applySoftRepair = false;

    /// Voice coupling for aux relationship
    VoiceCoupling voiceCoupling = VoiceCoupling::INDEPENDENT;

    /// PatternField zone threshold configuration (for sensitivity analysis)
    /// Default uses original compiled-in values for backward compatibility
    PatternFieldConfig patternFieldConfig{};
};

// =============================================================================
// Output Results
// =============================================================================

/**
 * Output from pattern generation
 *
 * Contains hit masks (bitmasks indicating which steps fire) and
 * per-step velocities for each voice.
 */
struct PatternResult
{
    uint64_t anchorMask = 0;                   ///< Voice 1 hit mask
    uint64_t shimmerMask = 0;                  ///< Voice 2 hit mask
    uint64_t auxMask = 0;                      ///< Aux voice hit mask
    float anchorVelocity[kMaxSteps] = {0};     ///< V1 per-step velocities
    float shimmerVelocity[kMaxSteps] = {0};    ///< V2 per-step velocities
    float auxVelocity[kMaxSteps] = {0};        ///< Aux per-step velocities
    int patternLength = 32;                    ///< Copy of input length for convenience
};

// =============================================================================
// Pattern Generation
// =============================================================================

/**
 * Generate a complete drum pattern from parameters.
 *
 * This is a pure function with no side effects:
 * - Same inputs always produce identical outputs (deterministic)
 * - No heap allocations (RT audio safe)
 * - No dependencies on external state
 *
 * The generation pipeline:
 * 1. Compute SHAPE-blended weights for anchor
 * 2. Apply AXIS X/Y biasing
 * 3. Add seed-based noise perturbation
 * 4. Select anchor hits via Gumbel sampling
 * 5. Apply guard rails
 * 6. Apply seed-based rotation for variation
 * 7. Generate shimmer via COMPLEMENT relationship
 * 8. Generate aux avoiding main voices
 * 9. Compute velocities based on ACCENT and metric weight
 *
 * @param params Input parameters
 * @param result Output masks and velocities (modified in place)
 */
void GeneratePattern(const PatternParams& params, PatternResult& result);

/**
 * Generate a fill pattern with density and velocity ramping.
 *
 * Fill patterns use progressive modifiers based on fillProgress (0.0-1.0):
 * - Density multiplier: 1.0 + maxBoost * (fillProgress^2), where maxBoost = 0.6 + energy * 0.4
 * - Velocity boost: 0.10 + 0.15 * fillProgress added to all velocities
 * - Accent probability: 0.50 + 0.50 * fillProgress
 * - Force accents when fillProgress > 0.85
 *
 * Reference: docs/specs/main.md section 9.2
 *
 * @param params Input parameters (fillProgress field controls ramping)
 * @param result Output masks and velocities with fill modifiers applied
 */
void GenerateFillPattern(const PatternParams& params, PatternResult& result);

// =============================================================================
// Rotation Utilities
// =============================================================================

/**
 * Rotate a bitmask while preserving a specific step's state.
 *
 * Used for anchor variation without disrupting beat 1 (Techno kick stability).
 * The preserved step stays in its original position regardless of rotation.
 *
 * @param mask Input bitmask
 * @param rotation Number of steps to rotate left
 * @param length Pattern length (bits in use)
 * @param preserveStep Step index to keep in place (typically 0 for beat 1)
 * @return Rotated mask with preserved step unchanged
 */
uint64_t RotateWithPreserve(uint64_t mask, int rotation, int length, int preserveStep);

// =============================================================================
// Hit Count Computation
// =============================================================================

/**
 * Compute target hit count for a voice based on energy and shape.
 *
 * Uses the full budget computation internally.
 *
 * @param energy ENERGY parameter (0.0-1.0)
 * @param patternLength Pattern length
 * @param voice Which voice to get hits for
 * @param shape SHAPE parameter for budget modulation
 * @return Target number of hits
 */
int ComputeTargetHits(float energy, int patternLength, Voice voice, float shape = 0.5f);

} // namespace daisysp_idm_grids
