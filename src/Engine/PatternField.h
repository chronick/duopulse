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

} // namespace daisysp_idm_grids
