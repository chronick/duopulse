#pragma once

#include "ArchetypeDNA.h"
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * PatternField: 2D pattern morphing and archetype blending
 *
 * The Pattern Field system provides smooth morphing between archetypes
 * using a "winner-take-more" blending approach. This preserves the
 * character of the dominant archetype during transitions while still
 * allowing smooth parameter changes.
 *
 * Reference: docs/specs/main.md section 5.3
 */

// =============================================================================
// Constants
// =============================================================================

/// Default temperature for softmax blending (lower = more winner-take-all)
constexpr float kDefaultSoftmaxTemperature = 0.5f;

/// Minimum weight to consider (avoid numerical issues)
constexpr float kMinWeight = 1e-6f;

// =============================================================================
// Blending Functions
// =============================================================================

/**
 * Apply softmax with temperature to sharpen weights
 *
 * Lower temperature = more winner-take-all behavior
 * Higher temperature = more equal blending
 *
 * @param weights Array of 4 weights (will be modified in place)
 * @param temperature Softmax temperature (0.1 to 2.0 typical range)
 */
void SoftmaxWithTemperature(float weights[4], float temperature);

/**
 * Compute bilinear interpolation weights for a position in the grid
 *
 * Given a position (fieldX, fieldY) in 0-1 range, computes weights for
 * the four surrounding archetypes. These weights can then be sharpened
 * with softmax for winner-take-more behavior.
 *
 * @param fieldX X position in pattern field (0.0-1.0)
 * @param fieldY Y position in pattern field (0.0-1.0)
 * @param outWeights Array of 4 weights: [bottomLeft, bottomRight, topLeft, topRight]
 * @param outGridX0 Output: lower X grid index (0 or 1)
 * @param outGridX1 Output: upper X grid index (1 or 2)
 * @param outGridY0 Output: lower Y grid index (0 or 1)
 * @param outGridY1 Output: upper Y grid index (1 or 2)
 */
void ComputeGridWeights(float fieldX, float fieldY,
                        float outWeights[4],
                        int& outGridX0, int& outGridX1,
                        int& outGridY0, int& outGridY1);

/**
 * Blend four archetypes using weighted combination
 *
 * Performs winner-take-more blending of archetypes based on FIELD X/Y
 * position. Continuous properties (weights, timing) are interpolated,
 * while discrete properties come from the dominant archetype.
 *
 * @param archetypes Array of 4 archetypes to blend [BL, BR, TL, TR]
 * @param weights Array of 4 weights (should sum to ~1.0)
 * @param outArchetype Output blended archetype
 */
void BlendArchetypes(const ArchetypeDNA* archetypes[4],
                     const float weights[4],
                     ArchetypeDNA& outArchetype);

/**
 * Get blended archetype from a genre field at a given position
 *
 * This is the main entry point for pattern field lookup. It combines
 * grid position calculation, weight computation, softmax sharpening,
 * and archetype blending into a single function.
 *
 * @param field The genre's 3x3 archetype grid
 * @param fieldX X position in pattern field (0.0-1.0)
 * @param fieldY Y position in pattern field (0.0-1.0)
 * @param temperature Softmax temperature (default 0.5 for winner-take-more)
 * @param outArchetype Output blended archetype
 */
void GetBlendedArchetype(const GenreField& field,
                         float fieldX, float fieldY,
                         float temperature,
                         ArchetypeDNA& outArchetype);

/**
 * Get the genre field for a specific genre
 *
 * Returns a reference to the pre-initialized genre field containing
 * the 9 archetypes for that genre.
 *
 * @param genre The genre to get the field for
 * @return Reference to the genre's archetype field
 */
const GenreField& GetGenreField(Genre genre);

/**
 * Initialize all genre fields with their archetype data
 *
 * Must be called once at startup before using GetGenreField().
 * Loads archetype data from the constexpr tables in ArchetypeData.h.
 */
void InitializeGenreFields();

/**
 * Check if genre fields have been initialized
 *
 * @return true if InitializeGenreFields() has been called
 */
bool AreGenreFieldsInitialized();

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Find the index of the dominant (highest weight) archetype
 *
 * @param weights Array of 4 weights
 * @return Index of the highest weight (0-3)
 */
int FindDominantArchetype(const float weights[4]);

/**
 * Interpolate a single float value using weights
 *
 * @param values Array of 4 values
 * @param weights Array of 4 weights
 * @return Weighted average
 */
float InterpolateFloat(const float values[4], const float weights[4]);

/**
 * Interpolate a step weight array using archetype weights
 *
 * @param archetypes Array of 4 archetypes
 * @param weights Blending weights
 * @param stepIndex Which step to interpolate
 * @param voice Which voice (0=anchor, 1=shimmer, 2=aux)
 * @return Interpolated weight for that step
 */
float InterpolateStepWeight(const ArchetypeDNA* archetypes[4],
                            const float weights[4],
                            int stepIndex,
                            int voice);

// =============================================================================
// Shape-Based Pattern Generation (Task 28)
// =============================================================================

/**
 * Minimum weight value to avoid completely silencing any step.
 * This ensures even "impossible" steps have some chance of firing.
 */
constexpr float kMinStepWeight = 0.05f;

/**
 * Zone boundaries for SHAPE parameter (0.0-1.0 range)
 *
 * The SHAPE parameter maps to a 7-zone system:
 *   Zone 1 pure:       [0.00, 0.28) - Stable humanized euclidean
 *   Crossfade 1->2a:   [0.28, 0.32) - Blend stable to syncopation
 *   Zone 2a:           [0.32, 0.48) - Pure syncopation (lower)
 *   Crossfade 2a->2b:  [0.48, 0.52) - Mid syncopation transition
 *   Zone 2b:           [0.52, 0.68) - Pure syncopation (upper)
 *   Crossfade 2->3:    [0.68, 0.72) - Blend syncopation to wild
 *   Zone 3 pure:       [0.72, 1.00] - Wild weighted random
 */
constexpr float kShapeZone1End = 0.28f;        // End of pure stable zone
constexpr float kShapeCrossfade1End = 0.32f;   // End of stable->syncopation crossfade
constexpr float kShapeZone2aEnd = 0.48f;       // End of lower syncopation zone
constexpr float kShapeCrossfade2End = 0.52f;   // End of mid syncopation crossfade
constexpr float kShapeZone2bEnd = 0.68f;       // End of upper syncopation zone
constexpr float kShapeCrossfade3End = 0.72f;   // End of syncopation->wild crossfade

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
                                 float* outWeights);

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

} // namespace daisysp_idm_grids
