#pragma once

#include "ArchetypeDNA.h"
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * ArchetypeData: Pre-defined archetype weight tables
 *
 * Contains 27 archetype definitions (9 per genre, 3x3 grid each).
 * These are placeholder values for development; final tuning happens in Phase 12.
 *
 * Grid layout per genre:
 *   Y (complexity)
 *   ^
 *   2  [0,2] [1,2] [2,2]  <- Busy/Poly/Chaos
 *   1  [0,1] [1,1] [2,1]  <- Driving/Groovy/Broken
 *   0  [0,0] [1,0] [2,0]  <- Minimal/Steady/Displaced
 *      0     1     2      -> X (syncopation)
 *
 * Reference: docs/specs/main.md section 5.1, 5.4
 */

// =============================================================================
// Forward Declaration
// =============================================================================

/**
 * Load archetype data into an ArchetypeDNA struct
 *
 * @param genre The genre to load from
 * @param archetypeIndex Index 0-8 (y*3 + x)
 * @param outArchetype The archetype struct to populate
 */
void LoadArchetypeData(Genre genre, int archetypeIndex, ArchetypeDNA& outArchetype);

// =============================================================================
// Weight Table Constants - Common Patterns
// =============================================================================

// Step masks for common metric positions
constexpr uint32_t kDownbeatMask     = 0x00010001;  // Steps 0, 16
constexpr uint32_t kQuarterNoteMask  = 0x01010101;  // Steps 0, 8, 16, 24
constexpr uint32_t kBackbeatMask     = 0x01000100;  // Steps 8, 24
constexpr uint32_t kEighthNoteMask   = 0x55555555;  // All even steps
constexpr uint32_t kSixteenthNoteMask = 0xFFFFFFFF; // All steps
constexpr uint32_t kOffbeatMask      = 0xAAAAAAAA;  // All odd steps

// =============================================================================
// Techno Genre Archetypes (Placeholder Values)
// =============================================================================

namespace techno
{

/**
 * [0,0] Minimal: Just kicks, quarter notes
 */
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Bar 1, beat 1-2
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Bar 1, beat 3-4
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // Bar 2, beat 1-2
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f   // Bar 2, beat 3-4
};

constexpr float kMinimal_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Aux[32] = {
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f
};

/**
 * [1,0] Steady: Basic groove, quarter + some 8ths
 */
constexpr float kSteady_Anchor[32] = {
    1.0f, 0.0f, 0.2f, 0.0f, 0.7f, 0.0f, 0.2f, 0.0f,
    0.9f, 0.0f, 0.2f, 0.0f, 0.7f, 0.0f, 0.2f, 0.0f,
    1.0f, 0.0f, 0.2f, 0.0f, 0.7f, 0.0f, 0.2f, 0.0f,
    0.9f, 0.0f, 0.2f, 0.0f, 0.7f, 0.0f, 0.3f, 0.0f
};

constexpr float kSteady_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f
};

constexpr float kSteady_Aux[32] = {
    0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f,
    0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f,
    0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f,
    0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f, 0.6f, 0.0f
};

/**
 * [2,0] Displaced: Skipped beat 3, off-grid sparse
 */
constexpr float kDisplaced_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f
};

constexpr float kDisplaced_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kDisplaced_Aux[32] = {
    0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f
};

/**
 * [0,1] Driving: Straight 8ths
 */
constexpr float kDriving_Anchor[32] = {
    1.0f, 0.0f, 0.5f, 0.0f, 0.8f, 0.0f, 0.5f, 0.0f,
    0.9f, 0.0f, 0.5f, 0.0f, 0.8f, 0.0f, 0.5f, 0.0f,
    1.0f, 0.0f, 0.5f, 0.0f, 0.8f, 0.0f, 0.5f, 0.0f,
    0.9f, 0.0f, 0.5f, 0.0f, 0.8f, 0.0f, 0.5f, 0.0f
};

constexpr float kDriving_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f
};

constexpr float kDriving_Aux[32] = {
    0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f, 0.7f, 0.0f
};

/**
 * [1,1] Groovy: Swung 8ths, shuffled feel
 */
constexpr float kGroovy_Anchor[32] = {
    1.0f, 0.0f, 0.3f, 0.4f, 0.7f, 0.0f, 0.3f, 0.4f,
    0.9f, 0.0f, 0.3f, 0.4f, 0.7f, 0.0f, 0.3f, 0.4f,
    1.0f, 0.0f, 0.3f, 0.4f, 0.7f, 0.0f, 0.3f, 0.4f,
    0.9f, 0.0f, 0.3f, 0.4f, 0.7f, 0.0f, 0.3f, 0.4f
};

constexpr float kGroovy_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.0f, 0.0f, 0.3f,
    1.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.0f, 0.0f, 0.3f,
    0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.0f, 0.0f, 0.3f,
    1.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.0f, 0.0f, 0.3f
};

constexpr float kGroovy_Aux[32] = {
    0.6f, 0.0f, 0.6f, 0.3f, 0.6f, 0.0f, 0.6f, 0.3f,
    0.6f, 0.0f, 0.6f, 0.3f, 0.6f, 0.0f, 0.6f, 0.3f,
    0.6f, 0.0f, 0.6f, 0.3f, 0.6f, 0.0f, 0.6f, 0.3f,
    0.6f, 0.0f, 0.6f, 0.3f, 0.6f, 0.0f, 0.6f, 0.3f
};

/**
 * [2,1] Broken: Missing downbeats, syncopated
 */
constexpr float kBroken_Anchor[32] = {
    0.8f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f
};

constexpr float kBroken_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f
};

constexpr float kBroken_Aux[32] = {
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f
};

/**
 * [0,2] Busy: 16th kick patterns
 */
constexpr float kBusy_Anchor[32] = {
    1.0f, 0.3f, 0.6f, 0.3f, 0.8f, 0.3f, 0.6f, 0.3f,
    0.9f, 0.3f, 0.6f, 0.3f, 0.8f, 0.3f, 0.6f, 0.3f,
    1.0f, 0.3f, 0.6f, 0.3f, 0.8f, 0.3f, 0.6f, 0.3f,
    0.9f, 0.3f, 0.6f, 0.4f, 0.8f, 0.4f, 0.6f, 0.4f
};

constexpr float kBusy_Shimmer[32] = {
    0.0f, 0.0f, 0.3f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f,
    1.0f, 0.0f, 0.3f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.0f, 0.3f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f,
    1.0f, 0.0f, 0.3f, 0.0f, 0.5f, 0.0f, 0.3f, 0.0f
};

constexpr float kBusy_Aux[32] = {
    0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f,
    0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f,
    0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f,
    0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f, 0.7f, 0.4f
};

/**
 * [1,2] Polyrhythm: 3-over-4 feel
 */
constexpr float kPolyrhythm_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 0.0f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 0.0f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f
};

constexpr float kPolyrhythm_Shimmer[32] = {
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f
};

constexpr float kPolyrhythm_Aux[32] = {
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f,
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f,
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f,
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f
};

/**
 * [2,2] Chaos: Irregular clusters, fragmented
 */
constexpr float kChaos_Anchor[32] = {
    1.0f, 0.4f, 0.0f, 0.6f, 0.0f, 0.5f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.4f, 0.5f,
    0.9f, 0.4f, 0.0f, 0.0f, 0.6f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.6f, 0.4f, 0.0f
};

constexpr float kChaos_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.5f, 0.0f, 0.4f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f
};

constexpr float kChaos_Aux[32] = {
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f
};

// Archetype metadata arrays
constexpr float kSwingAmounts[9] = {
    0.0f, 0.1f, 0.2f,   // Row 0: minimal, steady, displaced
    0.0f, 0.3f, 0.4f,   // Row 1: driving, groovy, broken
    0.0f, 0.2f, 0.5f    // Row 2: busy, poly, chaos
};

constexpr float kSwingPatterns[9] = {
    0.0f, 0.0f, 1.0f,   // Row 0
    0.0f, 1.0f, 2.0f,   // Row 1
    1.0f, 1.0f, 2.0f    // Row 2
};

constexpr float kDefaultCouples[9] = {
    0.2f, 0.3f, 0.4f,   // Row 0
    0.3f, 0.4f, 0.5f,   // Row 1
    0.4f, 0.5f, 0.6f    // Row 2
};

constexpr float kFillMultipliers[9] = {
    1.2f, 1.3f, 1.4f,
    1.3f, 1.5f, 1.6f,
    1.5f, 1.7f, 2.0f
};

constexpr uint32_t kAccentMasks[9] = {
    0x01010101, 0x01010101, 0x01010101,
    0x01010101, 0x11111111, 0x55555555,
    0x11111111, 0x55555555, 0xFFFFFFFF
};

constexpr uint32_t kRatchetMasks[9] = {
    0x00000000, 0x01010101, 0x01010101,
    0x01010101, 0x11111111, 0x55555555,
    0x11111111, 0x55555555, 0xFFFFFFFF
};

} // namespace techno

// =============================================================================
// Tribal Genre Archetypes (Placeholder Values)
// =============================================================================

namespace tribal
{

/**
 * [0,0] Minimal: Sparse, polyrhythmic foundation
 */
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Aux[32] = {
    0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f,
    0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f,
    0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f,
    0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f, 0.4f, 0.0f
};

/**
 * [1,0] Steady: African-influenced groove
 */
constexpr float kSteady_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f
};

constexpr float kSteady_Shimmer[32] = {
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f
};

constexpr float kSteady_Aux[32] = {
    0.5f, 0.0f, 0.5f, 0.3f, 0.5f, 0.0f, 0.5f, 0.3f,
    0.5f, 0.0f, 0.5f, 0.3f, 0.5f, 0.0f, 0.5f, 0.3f,
    0.5f, 0.0f, 0.5f, 0.3f, 0.5f, 0.0f, 0.5f, 0.3f,
    0.5f, 0.0f, 0.5f, 0.3f, 0.5f, 0.0f, 0.5f, 0.3f
};

/**
 * [2,0] Displaced: Off-grid tribal
 */
constexpr float kDisplaced_Anchor[32] = {
    0.9f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f
};

constexpr float kDisplaced_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f
};

constexpr float kDisplaced_Aux[32] = {
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f,
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f,
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f,
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f
};

/**
 * [0,1] Driving: Afro-house inspired
 */
constexpr float kDriving_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f,
    1.0f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f
};

constexpr float kDriving_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.4f, 0.0f
};

constexpr float kDriving_Aux[32] = {
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f,
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f,
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f,
    0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f
};

/**
 * [1,1] Groovy: Clave-based feel
 */
constexpr float kGroovy_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.6f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.6f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f
};

constexpr float kGroovy_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f
};

constexpr float kGroovy_Aux[32] = {
    0.6f, 0.0f, 0.6f, 0.4f, 0.6f, 0.0f, 0.6f, 0.4f,
    0.6f, 0.0f, 0.6f, 0.4f, 0.6f, 0.0f, 0.6f, 0.4f,
    0.6f, 0.0f, 0.6f, 0.4f, 0.6f, 0.0f, 0.6f, 0.4f,
    0.6f, 0.0f, 0.6f, 0.4f, 0.6f, 0.0f, 0.6f, 0.4f
};

/**
 * [2,1] Broken: Syncopated tribal
 */
constexpr float kBroken_Anchor[32] = {
    0.9f, 0.0f, 0.0f, 0.6f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.6f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f
};

constexpr float kBroken_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f
};

constexpr float kBroken_Aux[32] = {
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f
};

/**
 * [0,2] Busy: Dense polyrhythmic
 */
constexpr float kBusy_Anchor[32] = {
    1.0f, 0.3f, 0.0f, 0.6f, 0.4f, 0.0f, 0.5f, 0.3f,
    0.0f, 0.3f, 0.6f, 0.0f, 0.4f, 0.3f, 0.0f, 0.5f,
    0.9f, 0.3f, 0.0f, 0.6f, 0.4f, 0.0f, 0.5f, 0.3f,
    0.0f, 0.3f, 0.6f, 0.0f, 0.4f, 0.3f, 0.0f, 0.5f
};

constexpr float kBusy_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.4f, 0.0f
};

constexpr float kBusy_Aux[32] = {
    0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f,
    0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f,
    0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f,
    0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f, 0.6f, 0.4f
};

/**
 * [1,2] Polyrhythm: Complex interlocking
 */
constexpr float kPolyrhythm_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.6f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.6f, 0.0f, 0.0f, 0.7f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.7f, 0.0f,
    0.0f, 0.7f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.7f
};

constexpr float kPolyrhythm_Shimmer[32] = {
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f
};

constexpr float kPolyrhythm_Aux[32] = {
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f
};

/**
 * [2,2] Chaos: Maximum polyrhythmic complexity
 */
constexpr float kChaos_Anchor[32] = {
    1.0f, 0.4f, 0.0f, 0.6f, 0.0f, 0.5f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.6f, 0.0f, 0.5f, 0.0f, 0.4f, 0.5f,
    0.9f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.6f, 0.0f, 0.5f
};

constexpr float kChaos_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f,
    0.8f, 0.0f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.8f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f
};

constexpr float kChaos_Aux[32] = {
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f
};

// Tribal metadata - more swing overall
constexpr float kSwingAmounts[9] = {
    0.2f, 0.3f, 0.4f,
    0.2f, 0.4f, 0.5f,
    0.3f, 0.4f, 0.6f
};

constexpr float kSwingPatterns[9] = {
    1.0f, 1.0f, 2.0f,
    1.0f, 1.0f, 2.0f,
    1.0f, 2.0f, 2.0f
};

constexpr float kDefaultCouples[9] = {
    0.3f, 0.4f, 0.5f,
    0.4f, 0.5f, 0.6f,
    0.5f, 0.6f, 0.7f
};

constexpr float kFillMultipliers[9] = {
    1.3f, 1.4f, 1.5f,
    1.4f, 1.6f, 1.7f,
    1.6f, 1.8f, 2.0f
};

constexpr uint32_t kAccentMasks[9] = {
    0x01010101, 0x11111111, 0x11111111,
    0x11111111, 0x55555555, 0x55555555,
    0x55555555, 0xAAAAAAAA, 0xFFFFFFFF
};

constexpr uint32_t kRatchetMasks[9] = {
    0x00000000, 0x01010101, 0x11111111,
    0x01010101, 0x11111111, 0x55555555,
    0x11111111, 0x55555555, 0xFFFFFFFF
};

} // namespace tribal

// =============================================================================
// IDM Genre Archetypes (Placeholder Values)
// =============================================================================

namespace idm
{

/**
 * [0,0] Minimal: Sparse, glitchy
 */
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f,
    0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Aux[32] = {
    0.3f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.2f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.2f,
    0.3f, 0.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.2f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.2f
};

/**
 * [1,0] Steady: Regular but displaced
 */
constexpr float kSteady_Anchor[32] = {
    0.9f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kSteady_Shimmer[32] = {
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f
};

constexpr float kSteady_Aux[32] = {
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f,
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f,
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f,
    0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.4f, 0.2f
};

/**
 * [2,0] Displaced: Very off-grid
 */
constexpr float kDisplaced_Anchor[32] = {
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f
};

constexpr float kDisplaced_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f
};

constexpr float kDisplaced_Aux[32] = {
    0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f,
    0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f,
    0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f,
    0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f, 0.3f, 0.2f
};

/**
 * [0,1] Driving: IDM with forward motion
 */
constexpr float kDriving_Anchor[32] = {
    1.0f, 0.0f, 0.4f, 0.0f, 0.7f, 0.0f, 0.4f, 0.0f,
    0.8f, 0.0f, 0.4f, 0.0f, 0.7f, 0.0f, 0.4f, 0.0f,
    1.0f, 0.0f, 0.4f, 0.0f, 0.7f, 0.0f, 0.4f, 0.0f,
    0.8f, 0.0f, 0.4f, 0.0f, 0.7f, 0.0f, 0.5f, 0.3f
};

constexpr float kDriving_Shimmer[32] = {
    0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f,
    0.9f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f,
    0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f,
    0.9f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.3f
};

constexpr float kDriving_Aux[32] = {
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f
};

/**
 * [1,1] Groovy: Complex but danceable IDM
 */
constexpr float kGroovy_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.3f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.3f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.4f
};

constexpr float kGroovy_Shimmer[32] = {
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.3f, 0.0f,
    0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.3f, 0.0f
};

constexpr float kGroovy_Aux[32] = {
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f,
    0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f, 0.5f, 0.3f
};

/**
 * [2,1] Broken: Heavily syncopated IDM
 */
constexpr float kBroken_Anchor[32] = {
    0.9f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.8f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.4f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f
};

constexpr float kBroken_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.7f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.7f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kBroken_Aux[32] = {
    0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f,
    0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f,
    0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f,
    0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f, 0.4f, 0.3f
};

/**
 * [0,2] Busy: Dense IDM patterns
 */
constexpr float kBusy_Anchor[32] = {
    1.0f, 0.4f, 0.5f, 0.4f, 0.7f, 0.4f, 0.5f, 0.4f,
    0.8f, 0.4f, 0.5f, 0.4f, 0.7f, 0.4f, 0.5f, 0.4f,
    1.0f, 0.4f, 0.5f, 0.4f, 0.7f, 0.4f, 0.5f, 0.4f,
    0.8f, 0.4f, 0.5f, 0.5f, 0.7f, 0.5f, 0.5f, 0.5f
};

constexpr float kBusy_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f, 0.0f,
    0.9f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f, 0.0f,
    0.9f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f, 0.0f
};

constexpr float kBusy_Aux[32] = {
    0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f,
    0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f,
    0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f,
    0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f, 0.6f, 0.5f
};

/**
 * [1,2] Polyrhythm: Complex metric patterns
 */
constexpr float kPolyrhythm_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.6f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.9f, 0.0f, 0.0f, 0.6f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.0f
};

constexpr float kPolyrhythm_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.4f, 0.8f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f
};

constexpr float kPolyrhythm_Aux[32] = {
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f,
    0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f, 0.5f, 0.4f
};

/**
 * [2,2] Chaos: Maximum IDM complexity
 */
constexpr float kChaos_Anchor[32] = {
    1.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.5f, 0.0f, 0.0f, 0.9f, 0.0f, 0.5f, 0.0f,
    0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.6f
};

constexpr float kChaos_Shimmer[32] = {
    0.0f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f,
    0.7f, 0.0f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.4f, 0.5f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.5f, 0.0f, 0.0f, 0.7f, 0.0f, 0.4f, 0.0f
};

constexpr float kChaos_Aux[32] = {
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f
};

// IDM metadata - maximum timing chaos
constexpr float kSwingAmounts[9] = {
    0.3f, 0.4f, 0.5f,
    0.3f, 0.5f, 0.6f,
    0.4f, 0.5f, 0.7f
};

constexpr float kSwingPatterns[9] = {
    2.0f, 2.0f, 2.0f,
    2.0f, 2.0f, 2.0f,
    2.0f, 2.0f, 2.0f
};

constexpr float kDefaultCouples[9] = {
    0.4f, 0.5f, 0.6f,
    0.5f, 0.6f, 0.7f,
    0.6f, 0.7f, 0.8f
};

constexpr float kFillMultipliers[9] = {
    1.4f, 1.5f, 1.6f,
    1.5f, 1.7f, 1.8f,
    1.7f, 1.9f, 2.2f
};

constexpr uint32_t kAccentMasks[9] = {
    0x11111111, 0x55555555, 0x55555555,
    0x55555555, 0xAAAAAAAA, 0xAAAAAAAA,
    0xAAAAAAAA, 0xFFFFFFFF, 0xFFFFFFFF
};

constexpr uint32_t kRatchetMasks[9] = {
    0x01010101, 0x11111111, 0x55555555,
    0x11111111, 0x55555555, 0xAAAAAAAA,
    0x55555555, 0xAAAAAAAA, 0xFFFFFFFF
};

} // namespace idm

} // namespace daisysp_idm_grids
