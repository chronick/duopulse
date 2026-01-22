#pragma once

#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * PatternField: V5 SHAPE-based pattern generation
 *
 * The Pattern Field system generates per-step weights using a 7-zone SHAPE
 * parameter that morphs between stable (euclidean), syncopation, and wild
 * (chaotic) patterns. AXIS X/Y provide bidirectional biasing for beat emphasis
 * and rhythmic complexity.
 *
 * V5 replaces the V4 archetype grid system with procedural generation.
 *
 * Reference: docs/specs/main.md section 5
 */

// =============================================================================
// Shape-Based Pattern Generation
// =============================================================================

/**
 * Minimum weight value to avoid completely silencing any step.
 * This ensures even "impossible" steps have some chance of firing.
 */
constexpr float kMinStepWeight = 0.05f;

/**
 * Zone boundaries for SHAPE parameter (0.0-1.0 range)
 *
 * The SHAPE parameter maps to a 7-zone system, aligned with eval SHAPE zones:
 *   Zone 1 pure:       [0.00, 0.26) - Stable humanized euclidean
 *   Crossfade 1->2a:   [0.26, 0.30) - Blend stable to syncopation
 *   Zone 2a:           [0.30, 0.48) - Pure syncopation (lower)
 *   Crossfade 2a->2b:  [0.48, 0.52) - Mid syncopation transition
 *   Zone 2b:           [0.52, 0.66) - Pure syncopation (upper)
 *   Crossfade 2->3:    [0.66, 0.70) - Blend syncopation to wild
 *   Zone 3 pure:       [0.70, 1.00] - Wild weighted random
 *
 * Eval zones: stable [0, 0.30), syncopated [0.30, 0.70), wild [0.70, 1.0]
 */
constexpr float kShapeZone1End = 0.26f;        // End of pure stable zone
constexpr float kShapeCrossfade1End = 0.30f;   // End of stable->syncopation crossfade
constexpr float kShapeZone2aEnd = 0.48f;       // End of lower syncopation zone
constexpr float kShapeCrossfade2End = 0.52f;   // End of mid syncopation crossfade
constexpr float kShapeZone2bEnd = 0.66f;       // End of upper syncopation zone
constexpr float kShapeCrossfade3End = 0.70f;   // End of syncopation->wild crossfade

/**
 * Runtime-configurable zone thresholds for SHAPE parameter.
 * Default values align with eval SHAPE zones for correct metric targeting.
 */
struct PatternFieldConfig {
    float shapeZone1End = 0.26f;        // End of pure stable zone
    float shapeCrossfade1End = 0.30f;   // End of stable->syncopation crossfade
    float shapeZone2aEnd = 0.48f;       // End of lower syncopation zone
    float shapeCrossfade2End = 0.52f;   // End of mid syncopation crossfade
    float shapeZone2bEnd = 0.66f;       // End of upper syncopation zone
    float shapeCrossfade3End = 0.70f;   // End of syncopation->wild crossfade

    /// Validate that thresholds are monotonically increasing
    bool IsValid() const {
        return shapeZone1End < shapeCrossfade1End &&
               shapeCrossfade1End < shapeZone2aEnd &&
               shapeZone2aEnd < shapeCrossfade2End &&
               shapeCrossfade2End < shapeZone2bEnd &&
               shapeZone2bEnd < shapeCrossfade3End &&
               shapeCrossfade3End <= 1.0f;
    }
};

/// Default config matching original constexpr values (zero overhead when used)
static constexpr PatternFieldConfig kDefaultPatternFieldConfig{};

/**
 * Generate stable (euclidean-based) pattern weights
 *
 * Produces techno-style, four-on-floor patterns with:
 * - High weights on downbeats (steps 0, 4, 8, 12, 16, 20, 24, 28)
 * - Medium weights on half-beats (steps 2, 6, 10, 14, 18, 22, 26, 30)
 * - Lower weights on 16th note positions
 *
 * Energy scales the overall weight envelope.
 *
 * @param energy ENERGY parameter value (0.0-1.0), scales weight intensity
 * @param patternLength Number of steps in pattern (typically 16 or 32)
 * @param outWeights Output array of per-step weights (must be patternLength size)
 */
void GenerateStablePattern(float energy, int patternLength, float* outWeights);

/**
 * Generate syncopation pattern weights
 *
 * Produces funk-style, displaced patterns with:
 * - Suppressed downbeats (beat 1 at 50-70% of normal weight)
 * - Boosted anticipation positions (step before downbeat)
 * - Boosted weak offbeats
 * - Creates tension and forward motion
 *
 * Seed provides deterministic variation in exact suppression/boost amounts.
 *
 * @param energy ENERGY parameter value (0.0-1.0), scales weight intensity
 * @param seed Pattern seed for deterministic variation
 * @param patternLength Number of steps in pattern
 * @param outWeights Output array of per-step weights
 */
void GenerateSyncopationPattern(float energy, uint32_t seed, int patternLength, float* outWeights);

/**
 * Generate wild (chaotic) pattern weights
 *
 * Produces IDM-style, unpredictable patterns with:
 * - Weighted random distribution with high variation
 * - Seed-based deterministic chaos
 * - Some structural hints preserved (downbeats slightly more likely)
 *
 * @param energy ENERGY parameter value (0.0-1.0), scales weight intensity
 * @param seed Pattern seed for deterministic variation
 * @param patternLength Number of steps in pattern
 * @param outWeights Output array of per-step weights
 */
void GenerateWildPattern(float energy, uint32_t seed, int patternLength, float* outWeights);

/**
 * Compute shape-blended weights using 7-zone system
 *
 * Main entry point for SHAPE parameter processing. Blends between three
 * character zones (stable, syncopation, wild) with smooth crossfade
 * transitions.
 *
 * Zone behavior:
 * - Zone 1 (stable): Adds humanization that decreases toward boundary
 * - Zone 2 (syncopation): Pure displaced rhythm character
 * - Zone 3 (wild): Adds chaos injection that increases toward 100%
 *
 * Crossfade zones (4% each) provide smooth transitions without sudden jumps.
 *
 * @param shape SHAPE parameter value (0.0-1.0)
 * @param energy ENERGY parameter value (0.0-1.0)
 * @param seed Pattern seed for deterministic variation
 * @param patternLength Number of steps in pattern (1-32)
 * @param outWeights Output array of per-step weights (must be patternLength size)
 *
 * Guarantees:
 * - All output weights are in range [kMinStepWeight, 1.0]
 * - Same inputs always produce identical outputs (deterministic)
 * - No heap allocations (RT audio safe)
 */
void ComputeShapeBlendedWeights(float shape, float energy,
                                 uint32_t seed, int patternLength,
                                 float* outWeights,
                                 const PatternFieldConfig& config = kDefaultPatternFieldConfig);

/**
 * Linear interpolation helper for crossfade zones
 *
 * @param a First value
 * @param b Second value
 * @param t Blend factor (0.0 = a, 1.0 = b)
 * @return Interpolated value
 */
inline float LerpWeight(float a, float b, float t)
{
    return a + (b - a) * t;
}

/**
 * Clamp a weight to valid range [kMinStepWeight, 1.0]
 *
 * @param weight Raw weight value
 * @return Clamped weight
 */
inline float ClampWeight(float weight)
{
    if (weight < kMinStepWeight) return kMinStepWeight;
    if (weight > 1.0f) return 1.0f;
    return weight;
}

// =============================================================================
// AXIS X/Y Bidirectional Biasing (Task 29)
// =============================================================================

/**
 * Get the metric weight for a step position
 *
 * Returns a value in [0.0, 1.0] indicating how metrically strong the position is:
 * - 1.0 = bar downbeat (steps 0, 16)
 * - 0.75 = quarter notes (steps 4, 8, 12, 20, 24, 28)
 * - 0.5 = 8th notes
 * - 0.25 = 16th notes (weakest)
 *
 * @param step Step index within pattern (0 to patternLength-1)
 * @param patternLength Total pattern length
 * @return Metric weight in range [0.0, 1.0]
 */
float GetMetricWeight(int step, int patternLength);

/**
 * Get position strength for a step (bidirectional)
 *
 * Converts metric weight to a bidirectional value:
 * - -1.0 = strong downbeat
 * - 0.0 = neutral
 * - +1.0 = weak offbeat
 *
 * Formula: positionStrength = 1.0 - 2.0 * metricWeight
 *
 * @param step Step index within pattern
 * @param patternLength Total pattern length
 * @return Position strength in range [-1.0, +1.0]
 */
float GetPositionStrength(int step, int patternLength);

/**
 * Apply AXIS X/Y biasing to pattern weights
 *
 * Bidirectional AXIS X (beat position):
 * - 0.0 = Grounded (emphasize downbeats, suppress offbeats)
 * - 0.5 = Neutral (no bias)
 * - 1.0 = Floating (emphasize offbeats, suppress downbeats)
 *
 * Bidirectional AXIS Y (intricacy):
 * - 0.0 = Simple (suppress weak positions)
 * - 0.5 = Neutral (no bias)
 * - 1.0 = Complex (boost weak positions, add intricacy)
 *
 * "Broken Mode" emergent feature:
 * When SHAPE > 0.6 AND AXIS X > 0.7, some downbeats are stochastically
 * suppressed for an unstable, "broken" feel.
 *
 * @param baseWeights Array of weights to modify in-place (must be patternLength size)
 * @param axisX AXIS X parameter (0.0-1.0)
 * @param axisY AXIS Y parameter (0.0-1.0)
 * @param shape SHAPE parameter for broken mode detection (0.0-1.0)
 * @param seed Pattern seed for deterministic broken mode behavior
 * @param patternLength Number of steps in pattern (1-32)
 *
 * Guarantees:
 * - All output weights are clamped to [kMinStepWeight, 1.0]
 * - Same inputs always produce identical outputs (deterministic)
 * - No heap allocations (RT audio safe)
 */
void ApplyAxisBias(float* baseWeights, float axisX, float axisY,
                   float shape, uint32_t seed, int patternLength);

} // namespace daisysp_idm_grids
