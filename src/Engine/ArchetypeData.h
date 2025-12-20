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
// Techno Genre Archetypes - Four-on-floor, driving, minimal-to-industrial
// =============================================================================
//
// Grid layout:
//   Y (complexity)
//   ^
//   2  Busy         Polyrhythm     Chaos
//   1  Driving      Groovy         Broken
//   0  Minimal      Steady         Displaced
//      0            1              2          -> X (syncopation)
//
// Design philosophy:
// - Strong downbeats on 0 and 16 (bar starts)
// - Classic backbeat on 8 and 24
// - X axis adds syncopation and off-grid elements
// - Y axis adds density and 16th note activity
//

namespace techno
{

/**
 * [0,0] Minimal: Pure four-on-floor, quarter notes only
 * Classic minimal techno foundation - just the essentials
 */
constexpr float kMinimal_Anchor[32] = {
    // Bar 1: 1-e-&-a 2-e-&-a 3-e-&-a 4-e-&-a
    1.0f, 0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f,  // Beats 1, 2
    0.95f,0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f,  // Beats 3, 4
    // Bar 2: repeat
    1.0f, 0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f,
    0.95f,0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Shimmer[32] = {
    // Classic backbeat on 2 and 4
    0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Aux[32] = {
    // Sparse closed hats on off-8ths
    0.3f, 0.0f, 0.5f, 0.0f,  0.3f, 0.0f, 0.5f, 0.0f,
    0.3f, 0.0f, 0.5f, 0.0f,  0.3f, 0.0f, 0.5f, 0.0f,
    0.3f, 0.0f, 0.5f, 0.0f,  0.3f, 0.0f, 0.5f, 0.0f,
    0.3f, 0.0f, 0.5f, 0.0f,  0.3f, 0.0f, 0.6f, 0.0f
};

/**
 * [1,0] Steady: Basic techno groove with light syncopation
 * Standard backbeat with occasional upbeat kicks
 */
constexpr float kSteady_Anchor[32] = {
    // Four-on-floor with some "&" accents
    1.0f, 0.0f, 0.25f,0.0f,  0.9f, 0.0f, 0.2f, 0.0f,
    0.95f,0.0f, 0.3f, 0.0f,  0.9f, 0.0f, 0.2f, 0.0f,
    1.0f, 0.0f, 0.25f,0.0f,  0.9f, 0.0f, 0.2f, 0.0f,
    0.95f,0.0f, 0.35f,0.0f,  0.9f, 0.0f, 0.25f,0.0f
};

constexpr float kSteady_Shimmer[32] = {
    // Backbeat with upbeat pickups
    0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.35f,0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.4f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kSteady_Aux[32] = {
    // Consistent 8th note hats
    0.7f, 0.0f, 0.7f, 0.0f,  0.7f, 0.0f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.7f, 0.0f,  0.7f, 0.0f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.7f, 0.0f,  0.7f, 0.0f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.7f, 0.0f,  0.7f, 0.0f, 0.7f, 0.0f
};

/**
 * [2,0] Displaced: Off-grid sparse, skipped beat 3
 * Creates tension by avoiding expected downbeats
 */
constexpr float kDisplaced_Anchor[32] = {
    // Beat 1 strong, beat 3 skipped in both bars, displaced to "&" positions
    1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.45f, 0.0f, 0.0f, 0.0f, 0.0f,  // Beat 3 (step 8) now skipped
    1.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.55f,0.0f,
    0.0f, 0.0f, 0.4f, 0.0f,  0.0f, 0.5f, 0.0f, 0.0f
};

constexpr float kDisplaced_Shimmer[32] = {
    // Anticipated snares - before the beat
    0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.55f,
    0.85f,0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.5f,
    0.9f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kDisplaced_Aux[32] = {
    // Off-beat hats emphasizing the displaced feel
    0.0f, 0.4f, 0.0f, 0.3f,  0.0f, 0.4f, 0.0f, 0.35f,
    0.0f, 0.4f, 0.0f, 0.3f,  0.0f, 0.4f, 0.0f, 0.35f,
    0.0f, 0.4f, 0.0f, 0.3f,  0.0f, 0.4f, 0.0f, 0.35f,
    0.0f, 0.4f, 0.0f, 0.35f, 0.0f, 0.45f,0.0f, 0.4f
};

/**
 * [0,1] Driving: Straight 8th notes, high energy
 * Industrial edge with relentless kick pattern
 */
constexpr float kDriving_Anchor[32] = {
    // Strong 8th notes throughout
    1.0f, 0.0f, 0.6f, 0.0f,  0.85f,0.0f, 0.6f, 0.0f,
    0.9f, 0.0f, 0.6f, 0.0f,  0.85f,0.0f, 0.6f, 0.0f,
    1.0f, 0.0f, 0.6f, 0.0f,  0.85f,0.0f, 0.6f, 0.0f,
    0.9f, 0.0f, 0.6f, 0.0f,  0.85f,0.0f, 0.65f,0.0f
};

constexpr float kDriving_Shimmer[32] = {
    // Backbeat with ghost notes
    0.0f, 0.0f, 0.0f, 0.0f,  0.35f,0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,  0.35f,0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,  0.35f,0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,  0.4f, 0.0f, 0.0f, 0.0f
};

constexpr float kDriving_Aux[32] = {
    // Driving 8th note hats
    0.8f, 0.0f, 0.8f, 0.0f,  0.8f, 0.0f, 0.8f, 0.0f,
    0.8f, 0.0f, 0.8f, 0.0f,  0.8f, 0.0f, 0.8f, 0.0f,
    0.8f, 0.0f, 0.8f, 0.0f,  0.8f, 0.0f, 0.8f, 0.0f,
    0.8f, 0.0f, 0.8f, 0.0f,  0.8f, 0.0f, 0.8f, 0.0f
};

/**
 * [1,1] Groovy: Shuffled feel with swing pocket
 * The sweet spot - danceable with character
 */
constexpr float kGroovy_Anchor[32] = {
    // Swung feel - emphasis on "a" subdivisions (steps 3, 7, 11...)
    1.0f, 0.0f, 0.0f, 0.45f, 0.85f,0.0f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.0f, 0.45f, 0.85f,0.0f, 0.0f, 0.4f,
    1.0f, 0.0f, 0.0f, 0.45f, 0.85f,0.0f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.0f, 0.5f,  0.85f,0.0f, 0.0f, 0.45f
};

constexpr float kGroovy_Shimmer[32] = {
    // Shuffled backbeat with anticipation
    0.0f, 0.0f, 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.4f,
    1.0f, 0.0f, 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.4f,
    1.0f, 0.0f, 0.0f, 0.4f,  0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kGroovy_Aux[32] = {
    // Swung 8ths with ghost 16ths
    0.7f, 0.0f, 0.7f, 0.35f, 0.7f, 0.0f, 0.7f, 0.35f,
    0.7f, 0.0f, 0.7f, 0.35f, 0.7f, 0.0f, 0.7f, 0.35f,
    0.7f, 0.0f, 0.7f, 0.35f, 0.7f, 0.0f, 0.7f, 0.35f,
    0.7f, 0.0f, 0.7f, 0.4f,  0.7f, 0.0f, 0.7f, 0.4f
};

/**
 * [2,1] Broken: Missing expected beats, syncopated claps
 * Broken beat techno - keeps you guessing
 */
constexpr float kBroken_Anchor[32] = {
    // Beat 1 present, beat 2 displaced, beat 3 often missing
    0.95f,0.0f, 0.0f, 0.55f, 0.0f, 0.0f, 0.6f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.55f, 0.0f, 0.0f, 0.55f,0.0f,
    0.0f, 0.0f, 0.45f,0.0f,  0.0f, 0.55f,0.0f, 0.0f
};

constexpr float kBroken_Shimmer[32] = {
    // Syncopated claps - avoiding expected positions
    0.0f, 0.0f, 0.45f,0.0f,  0.0f, 0.0f, 0.0f, 0.5f,
    0.85f,0.0f, 0.0f, 0.0f,  0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.45f,0.0f,  0.0f, 0.0f, 0.0f, 0.5f,
    0.9f, 0.0f, 0.0f, 0.0f,  0.0f, 0.45f,0.0f, 0.0f
};

constexpr float kBroken_Aux[32] = {
    // Irregular hat pattern
    0.6f, 0.35f,0.6f, 0.0f,  0.6f, 0.35f,0.6f, 0.35f,
    0.6f, 0.0f, 0.6f, 0.35f, 0.6f, 0.35f,0.6f, 0.0f,
    0.6f, 0.35f,0.6f, 0.0f,  0.6f, 0.35f,0.6f, 0.35f,
    0.6f, 0.0f, 0.6f, 0.35f, 0.6f, 0.4f, 0.6f, 0.0f
};

/**
 * [0,2] Busy: Dense 16th note kick patterns
 * Industrial/gabber influence - relentless energy
 */
constexpr float kBusy_Anchor[32] = {
    // 16th note kicks with accented downbeats
    1.0f, 0.4f, 0.65f,0.35f, 0.85f,0.4f, 0.65f,0.35f,
    0.9f, 0.4f, 0.65f,0.35f, 0.85f,0.4f, 0.65f,0.35f,
    1.0f, 0.4f, 0.65f,0.35f, 0.85f,0.4f, 0.65f,0.35f,
    0.9f, 0.4f, 0.65f,0.45f, 0.85f,0.45f,0.7f, 0.45f
};

constexpr float kBusy_Shimmer[32] = {
    // Dense ghost snares
    0.0f, 0.0f, 0.35f,0.0f,  0.5f, 0.0f, 0.35f,0.0f,
    1.0f, 0.0f, 0.35f,0.0f,  0.5f, 0.0f, 0.35f,0.0f,
    0.0f, 0.0f, 0.35f,0.0f,  0.5f, 0.0f, 0.35f,0.0f,
    1.0f, 0.0f, 0.35f,0.0f,  0.5f, 0.0f, 0.4f, 0.0f
};

constexpr float kBusy_Aux[32] = {
    // Constant 16th note hats
    0.75f,0.5f, 0.75f,0.5f,  0.75f,0.5f, 0.75f,0.5f,
    0.75f,0.5f, 0.75f,0.5f,  0.75f,0.5f, 0.75f,0.5f,
    0.75f,0.5f, 0.75f,0.5f,  0.75f,0.5f, 0.75f,0.5f,
    0.75f,0.5f, 0.75f,0.55f, 0.75f,0.55f,0.75f,0.55f
};

/**
 * [1,2] Polyrhythm: 3-over-4 feel
 * Creates rhythmic tension with cross-rhythm
 */
constexpr float kPolyrhythm_Anchor[32] = {
    // Dotted 8ths creating 3-feel: steps 0, 3, 6, 9, 13, 16, 19, 22, 26, 29
    1.0f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f, 0.7f, 0.0f,
    0.0f, 0.65f,0.0f, 0.0f,  0.0f, 0.7f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f, 0.7f, 0.0f,
    0.0f, 0.0f, 0.65f,0.0f,  0.0f, 0.7f, 0.0f, 0.0f
};

constexpr float kPolyrhythm_Shimmer[32] = {
    // Counter-rhythm - offset from anchor
    0.0f, 0.0f, 0.55f,0.0f,  0.0f, 0.55f,0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.55f, 0.0f, 0.0f, 0.55f,0.0f,
    0.0f, 0.0f, 0.55f,0.0f,  0.0f, 0.55f,0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.55f, 0.0f, 0.0f, 0.55f,0.0f
};

constexpr float kPolyrhythm_Aux[32] = {
    // Regular 8ths as anchor point
    0.7f, 0.4f, 0.7f, 0.4f,  0.7f, 0.4f, 0.7f, 0.4f,
    0.7f, 0.4f, 0.7f, 0.4f,  0.7f, 0.4f, 0.7f, 0.4f,
    0.7f, 0.4f, 0.7f, 0.4f,  0.7f, 0.4f, 0.7f, 0.4f,
    0.7f, 0.4f, 0.7f, 0.4f,  0.7f, 0.45f,0.7f, 0.45f
};

/**
 * [2,2] Chaos: Maximum irregularity, fragmented
 * Controlled chaos - still danceable but unpredictable
 */
constexpr float kChaos_Anchor[32] = {
    // Irregular clusters with gaps
    1.0f, 0.45f,0.0f, 0.6f,  0.0f, 0.55f,0.0f, 0.5f,
    0.0f, 0.0f, 0.55f,0.0f,  0.65f,0.0f, 0.5f, 0.55f,
    0.9f, 0.5f, 0.0f, 0.0f,  0.6f, 0.0f, 0.55f,0.0f,
    0.0f, 0.55f,0.0f, 0.5f,  0.0f, 0.6f, 0.5f, 0.0f
};

constexpr float kChaos_Shimmer[32] = {
    // Fragmented snares
    0.0f, 0.0f, 0.5f, 0.0f,  0.55f,0.0f, 0.5f, 0.0f,
    0.85f,0.0f, 0.0f, 0.5f,  0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.55f, 0.0f, 0.5f, 0.0f, 0.5f,
    0.9f, 0.0f, 0.5f, 0.0f,  0.5f, 0.0f, 0.0f, 0.0f
};

constexpr float kChaos_Aux[32] = {
    // Erratic hats
    0.6f, 0.5f, 0.6f, 0.5f,  0.6f, 0.5f, 0.6f, 0.5f,
    0.6f, 0.5f, 0.6f, 0.5f,  0.6f, 0.5f, 0.6f, 0.5f,
    0.6f, 0.5f, 0.6f, 0.5f,  0.6f, 0.5f, 0.6f, 0.5f,
    0.6f, 0.5f, 0.6f, 0.55f, 0.6f, 0.55f,0.6f, 0.55f
};

// Archetype metadata arrays - tuned for techno character
constexpr float kSwingAmounts[9] = {
    0.0f, 0.08f,0.15f,  // Row 0: minimal, steady, displaced
    0.0f, 0.25f,0.35f,  // Row 1: driving, groovy, broken
    0.05f,0.15f,0.45f   // Row 2: busy, poly, chaos
};

constexpr float kSwingPatterns[9] = {
    0.0f, 0.0f, 1.0f,   // Row 0: 8ths, 8ths, 16ths
    0.0f, 1.0f, 2.0f,   // Row 1: 8ths, 16ths, mixed
    1.0f, 1.0f, 2.0f    // Row 2: 16ths, 16ths, mixed
};

constexpr float kDefaultCouples[9] = {
    0.15f,0.25f,0.35f,  // Row 0: independent to slight interlock
    0.2f, 0.4f, 0.5f,   // Row 1: slight interlock to moderate
    0.35f,0.45f,0.55f   // Row 2: moderate interlock
};

constexpr float kFillMultipliers[9] = {
    1.15f,1.25f,1.35f,
    1.3f, 1.45f,1.6f,
    1.5f, 1.65f,1.9f
};

constexpr uint32_t kAccentMasks[9] = {
    0x01010101, 0x01010101, 0x09090909,  // Row 0: quarters, quarters, displaced
    0x01010101, 0x11111111, 0x55555555,  // Row 1: quarters, 8ths, all 8ths
    0x11111111, 0x49494949, 0xFFFFFFFF   // Row 2: 8ths, poly (3s), all
};

constexpr uint32_t kRatchetMasks[9] = {
    0x00000000, 0x01010101, 0x09090909,
    0x01010101, 0x11111111, 0x55555555,
    0x11111111, 0x55555555, 0xFFFFFFFF
};

} // namespace techno

// =============================================================================
// Tribal Genre Archetypes - Polyrhythmic, Clave-Based, African/Latin Influenced
// =============================================================================
//
// Tribal grid emphasizes polyrhythmic patterns and clave-based grooves:
// - X axis (syncopation): traditional → displaced → fragmented
// - Y axis (complexity): sparse → interlocking → dense
//
// Design philosophy:
// - 3-2 and 2-3 clave patterns as foundation
// - Emphasis on swing and offbeat accents
// - Call-and-response between voices
// - More displacement than techno, less chaos than IDM
//

namespace tribal
{

/**
 * [0,0] Minimal: Sparse polyrhythmic foundation
 * Character: Afrobeat-influenced minimal - space with tension
 */
constexpr float kMinimal_Anchor[32] = {
    // 3-2 Son Clave inspired spacing
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f,  // Hit on 1 and "a" of 2
    0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,  // Hit on "&" of 3
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Shimmer[32] = {
    // Counter-rhythm to anchor
    0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f,  // Answer on "&" of 2
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f   // Strong on 4
};

constexpr float kMinimal_Aux[32] = {
    // Bell pattern inspired by Afrobeat
    0.6f, 0.0f, 0.0f, 0.4f, 0.6f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.4f, 0.6f, 0.0f,
    0.6f, 0.0f, 0.0f, 0.4f, 0.6f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.4f, 0.6f, 0.0f
};

/**
 * [1,0] Steady: Standard Afrobeat groove
 * Character: Fela Kuti influenced - strong pocket with syncopation
 */
constexpr float kSteady_Anchor[32] = {
    // Standard afrobeat kick pattern
    1.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.95f,0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f
};

constexpr float kSteady_Shimmer[32] = {
    // Snare on 2 and 4 with ghost notes
    0.0f, 0.0f, 0.35f,0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.35f,0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.35f,0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.35f,0.0f, 0.0f, 0.0f
};

constexpr float kSteady_Aux[32] = {
    // 12/8 feel bell pattern
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f
};

/**
 * [2,0] Displaced: Off-grid tribal, clave displaced
 * Character: Rumba clave feel with unexpected accents
 */
constexpr float kDisplaced_Anchor[32] = {
    // 2-3 Rumba clave spacing
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f,  // Delayed second hit
    0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f,  // Displaced from beat 3
    0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kDisplaced_Shimmer[32] = {
    // Counter to clave - fills the gaps
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f
};

constexpr float kDisplaced_Aux[32] = {
    // Offbeat shaker pattern
    0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f,
    0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f,
    0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.4f,
    0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.5f
};

/**
 * [0,1] Driving: Afro-house inspired forward motion
 * Character: Modern afro-house - 4/4 with triplet feel
 */
constexpr float kDriving_Anchor[32] = {
    // Four-on-floor with triplet ghost notes
    1.0f, 0.0f, 0.0f, 0.45f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.95f,0.0f, 0.0f, 0.45f, 0.0f, 0.0f, 0.0f, 0.4f,
    1.0f, 0.0f, 0.0f, 0.45f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.95f,0.0f, 0.0f, 0.5f,  0.0f, 0.0f, 0.0f, 0.45f
};

constexpr float kDriving_Shimmer[32] = {
    // Snare with triplet feel ghosts
    0.0f, 0.0f, 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.35f,
    1.0f, 0.0f, 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.35f,
    0.0f, 0.0f, 0.0f, 0.35f, 0.0f, 0.0f, 0.0f, 0.35f,
    1.0f, 0.0f, 0.0f, 0.4f,  0.0f, 0.0f, 0.0f, 0.35f
};

constexpr float kDriving_Aux[32] = {
    // Shaker pattern with triplet groupings
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f
};

/**
 * [1,1] Groovy: 3-2 Son Clave based feel
 * Character: The sweet spot - classic Afro-Cuban pocket
 */
constexpr float kGroovy_Anchor[32] = {
    // 3-2 Son Clave pattern
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f,  // 1 ... and a
    0.0f, 0.0f, 0.0f, 0.0f, 0.75f,0.0f, 0.0f, 0.0f,  //       &
    1.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f,  // 1     &
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f   //       a
};

constexpr float kGroovy_Shimmer[32] = {
    // Conga-style response pattern
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f
};

constexpr float kGroovy_Aux[32] = {
    // Cascara pattern
    0.7f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f
};

/**
 * [2,1] Broken: Syncopated tribal with missing downbeats
 * Character: Hypnotic and disorienting
 */
constexpr float kBroken_Anchor[32] = {
    // Missing beat 1, emphasis on offbeats
    0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.8f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f,  // Beat 1 appears late
    0.0f, 0.6f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f
};

constexpr float kBroken_Shimmer[32] = {
    // Cross-rhythm snare pattern
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.8f, 0.0f
};

constexpr float kBroken_Aux[32] = {
    // Irregular shaker - emphasizes displacement
    0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.0f,
    0.5f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f
};

/**
 * [0,2] Busy: Dense polyrhythmic layers
 * Character: West African ensemble - multiple interlocking parts
 */
constexpr float kBusy_Anchor[32] = {
    // Dense djembe-style pattern
    1.0f, 0.0f, 0.0f, 0.6f, 0.5f, 0.0f, 0.0f, 0.55f,
    0.5f, 0.0f, 0.6f, 0.0f, 0.5f, 0.0f, 0.0f, 0.55f,
    0.95f,0.0f, 0.0f, 0.6f, 0.5f, 0.0f, 0.0f, 0.55f,
    0.5f, 0.0f, 0.6f, 0.0f, 0.5f, 0.0f, 0.0f, 0.6f
};

constexpr float kBusy_Shimmer[32] = {
    // Kenkeni-style response
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.9f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f
};

constexpr float kBusy_Aux[32] = {
    // Full 16th note shaker/bell
    0.7f, 0.45f, 0.65f, 0.45f, 0.7f, 0.45f, 0.65f, 0.45f,
    0.7f, 0.45f, 0.65f, 0.45f, 0.7f, 0.45f, 0.65f, 0.45f,
    0.7f, 0.45f, 0.65f, 0.45f, 0.7f, 0.45f, 0.65f, 0.45f,
    0.7f, 0.45f, 0.65f, 0.5f,  0.7f, 0.5f,  0.65f, 0.5f
};

/**
 * [1,2] Polyrhythm: 5-over-4 and 3-over-4 interlocking
 * Character: Maximum polymetric tension - Ewe-inspired
 */
constexpr float kPolyrhythm_Anchor[32] = {
    // 5-over-4 pattern (hits at 0, 6.4, 12.8, 19.2, 25.6 approximated)
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.75f,0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.75f,0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f
};

constexpr float kPolyrhythm_Shimmer[32] = {
    // 3-over-4 counter (dotted 8ths)
    0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kPolyrhythm_Aux[32] = {
    // Standard 12/8 bell as anchor point
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f,
    0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.5f, 0.7f, 0.0f
};

/**
 * [2,2] Chaos: Maximum polyrhythmic complexity
 * Character: Controlled chaos - multiple cross-rhythms
 */
constexpr float kChaos_Anchor[32] = {
    // Irregular pattern with cluster accents
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.55f,
    0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.55f,0.0f,
    0.0f, 0.6f, 0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.55f,0.0f, 0.0f, 0.6f, 0.0f, 0.0f
};

constexpr float kChaos_Shimmer[32] = {
    // Fills gaps in anchor - call and response
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f
};

constexpr float kChaos_Aux[32] = {
    // Erratic shaker - breaks expected patterns
    0.6f, 0.0f, 0.55f,0.0f, 0.0f, 0.6f, 0.0f, 0.55f,
    0.0f, 0.6f, 0.0f, 0.0f, 0.55f,0.0f, 0.6f, 0.0f,
    0.0f, 0.55f,0.6f, 0.0f, 0.0f, 0.0f, 0.55f,0.6f,
    0.0f, 0.0f, 0.6f, 0.55f,0.0f, 0.0f, 0.0f, 0.6f
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
// IDM Genre Archetypes - Experimental, Glitchy, Algorithmic
// =============================================================================
//
// IDM grid emphasizes unpredictability and unconventional patterns:
// - X axis (syncopation): off-grid → displaced → fragmented
// - Y axis (complexity): sparse → fractured → dense chaos
//
// Design philosophy:
// - Avoid predictable downbeats (especially in high syncopation)
// - Clustered hits and irregular spacing
// - More silence than techno, but more unpredictable than tribal
// - Embrace the "wrong" placements that create tension
//

namespace idm
{

/**
 * [0,0] Minimal: Sparse ambient glitch
 * Character: Autechre-influenced - space and tension
 */
constexpr float kMinimal_Anchor[32] = {
    // Long gaps with unexpected placements
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f,  // Late hit
    0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,  // Off beat 3
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Shimmer[32] = {
    // Distant, unexpected responses
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f,  // Answer
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kMinimal_Aux[32] = {
    // Sparse, irregular hi-hat texture
    0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

/**
 * [1,0] Steady: Regular spacing but off-grid placement
 * Character: Boards of Canada - predictable rhythm, weird timing
 */
constexpr float kSteady_Anchor[32] = {
    // Regular intervals but shifted from grid
    0.95f,0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f,  // Kick, then "&a" of 2
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f,0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f
};

constexpr float kSteady_Shimmer[32] = {
    // Snare in expected place but with odd ghosts
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kSteady_Aux[32] = {
    // Asymmetric hi-hat pattern
    0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f,
    0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.4f, 0.0f, 0.5f, 0.0f
};

/**
 * [2,0] Displaced: Very off-grid, anti-groove
 * Character: Maximum tension through absence
 */
constexpr float kDisplaced_Anchor[32] = {
    // Only fires in unexpected places - weak downbeat for true anti-groove
    0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f,  // e of 2
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f   // Before and after 4
};

constexpr float kDisplaced_Shimmer[32] = {
    // Wrong places for snare
    0.0f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f,  // & of 2
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f,  // e of 3
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f
};

constexpr float kDisplaced_Aux[32] = {
    // Sparse, random-feeling texture
    0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f
};

/**
 * [0,1] Driving: IDM with forward motion
 * Character: Squarepusher-influenced - energetic but fractured
 */
constexpr float kDriving_Anchor[32] = {
    // Fast 8ths with irregular accents
    1.0f, 0.0f, 0.55f,0.0f, 0.8f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.55f,0.0f, 0.8f, 0.0f, 0.5f, 0.0f,  // No beat 2!
    0.95f,0.0f, 0.55f,0.0f, 0.8f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.55f,0.0f, 0.8f, 0.0f, 0.6f, 0.45f  // Pickup cluster
};

constexpr float kDriving_Shimmer[32] = {
    // Off-beat snares
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.45f,0.0f, 0.0f
};

constexpr float kDriving_Aux[32] = {
    // Fast 16th hi-hats with accents
    0.6f, 0.35f, 0.55f, 0.35f, 0.6f, 0.35f, 0.55f, 0.35f,
    0.6f, 0.35f, 0.55f, 0.35f, 0.6f, 0.35f, 0.55f, 0.35f,
    0.6f, 0.35f, 0.55f, 0.35f, 0.6f, 0.35f, 0.55f, 0.35f,
    0.6f, 0.35f, 0.55f, 0.4f,  0.6f, 0.4f,  0.55f, 0.4f
};

/**
 * [1,1] Groovy: Complex but danceable IDM
 * Character: Aphex Twin-influenced - head-nodding chaos
 */
constexpr float kGroovy_Anchor[32] = {
    // Danceable but with displaced accents
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f,0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f,0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.55f,0.0f
};

constexpr float kGroovy_Shimmer[32] = {
    // Snare on 2 and 4 but with wrong-feeling ghosts
    0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f
};

constexpr float kGroovy_Aux[32] = {
    // Broken 16th pattern with gaps
    0.55f,0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.55f,
    0.0f, 0.5f, 0.0f, 0.0f, 0.55f,0.0f, 0.5f, 0.0f,
    0.55f,0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.55f,
    0.0f, 0.5f, 0.0f, 0.0f, 0.55f,0.0f, 0.55f,0.0f
};

/**
 * [2,1] Broken: Heavily syncopated IDM
 * Character: Venetian Snares-influenced - broken but musical
 */
constexpr float kBroken_Anchor[32] = {
    // Beat 1 strong, everything else displaced
    0.95f,0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f,0.0f,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.85f,0.0f, 0.0f, 0.0f,  // Late beat 3
    0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f,0.0f
};

constexpr float kBroken_Shimmer[32] = {
    // Maximum syncopation - never where expected
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f
};

constexpr float kBroken_Aux[32] = {
    // Fractured hi-hat pattern
    0.5f, 0.0f, 0.0f, 0.45f,0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.45f,0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.45f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.45f,0.0f, 0.0f,
    0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.45f
};

/**
 * [0,2] Busy: Dense IDM patterns
 * Character: Drill'n'bass - fast and furious
 */
constexpr float kBusy_Anchor[32] = {
    // Fast kicks with irregular accents
    1.0f, 0.45f,0.55f,0.0f, 0.7f, 0.45f,0.55f,0.0f,
    0.0f, 0.45f,0.55f,0.0f, 0.7f, 0.45f,0.55f,0.0f,
    0.95f,0.45f,0.55f,0.0f, 0.7f, 0.45f,0.55f,0.0f,
    0.0f, 0.45f,0.55f,0.5f, 0.7f, 0.5f, 0.6f, 0.5f
};

constexpr float kBusy_Shimmer[32] = {
    // Breakbeat-style snare rolls
    0.0f, 0.0f, 0.0f, 0.45f,0.0f, 0.0f, 0.0f, 0.5f,
    0.9f, 0.0f, 0.0f, 0.45f,0.0f, 0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.0f, 0.45f,0.0f, 0.0f, 0.0f, 0.5f,
    0.9f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.55f,0.5f
};

constexpr float kBusy_Aux[32] = {
    // Maximum density hi-hats
    0.65f,0.5f, 0.6f, 0.5f, 0.65f,0.5f, 0.6f, 0.5f,
    0.65f,0.5f, 0.6f, 0.5f, 0.65f,0.5f, 0.6f, 0.5f,
    0.65f,0.5f, 0.6f, 0.5f, 0.65f,0.5f, 0.6f, 0.5f,
    0.65f,0.5f, 0.6f, 0.55f,0.65f,0.55f,0.65f,0.55f
};

/**
 * [1,2] Polyrhythm: Complex metric patterns
 * Character: Math-rock influenced IDM - 7s and 5s colliding
 */
constexpr float kPolyrhythm_Anchor[32] = {
    // 7-over-8 pattern approximation
    1.0f, 0.0f, 0.0f, 0.0f, 0.65f,0.0f, 0.0f, 0.0f,
    0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.65f,0.0f,
    0.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.65f,
    0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kPolyrhythm_Shimmer[32] = {
    // 5-over-4 counter
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f,0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.55f,0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f
};

constexpr float kPolyrhythm_Aux[32] = {
    // Irregular groupings
    0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.0f,
    0.5f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.0f,
    0.5f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.55f,0.0f
};

/**
 * [2,2] Chaos: Maximum IDM complexity
 * Character: Full Autechre - algorithmic destruction
 */
constexpr float kChaos_Anchor[32] = {
    // Cluster-based hits with large gaps
    1.0f, 0.55f,0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.6f, 0.55f,0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.55f,0.0f, 0.0f,
    0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.55f,0.6f
};

constexpr float kChaos_Shimmer[32] = {
    // Anti-pattern - fills gaps chaotically
    0.0f, 0.0f, 0.0f, 0.5f, 0.55f,0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.55f,
    0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.55f,0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

constexpr float kChaos_Aux[32] = {
    // Broken, glitchy texture
    0.55f,0.0f, 0.0f, 0.5f, 0.0f, 0.55f,0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f, 0.55f,0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.55f,0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.55f,0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.55f
};

// IDM metadata - experimental timing, heavy displacement
constexpr float kSwingAmounts[9] = {
    0.15f,0.25f,0.4f,   // Row 0: subtle to heavy
    0.2f, 0.35f,0.5f,   // Row 1: moderate to extreme
    0.25f,0.4f, 0.6f    // Row 2: heavy throughout
};

constexpr float kSwingPatterns[9] = {
    2.0f, 2.0f, 2.0f,   // All mixed (most chaotic)
    2.0f, 2.0f, 2.0f,
    2.0f, 2.0f, 2.0f
};

constexpr float kDefaultCouples[9] = {
    0.3f, 0.4f, 0.5f,   // Row 0: independent to moderate
    0.4f, 0.5f, 0.6f,   // Row 1: moderate
    0.5f, 0.6f, 0.7f    // Row 2: heavier interlock for density
};

constexpr float kFillMultipliers[9] = {
    1.35f,1.45f,1.55f,
    1.5f, 1.65f,1.8f,
    1.7f, 1.85f,2.1f
};

constexpr uint32_t kAccentMasks[9] = {
    0x11111111, 0x55555555, 0xAAAAAAAA,  // Row 0: off-grid emphasis
    0x55555555, 0xAAAAAAAA, 0xAAAAAAAA,  // Row 1: heavy off-beat
    0xAAAAAAAA, 0xFFFFFFFF, 0xFFFFFFFF   // Row 2: all positions
};

constexpr uint32_t kRatchetMasks[9] = {
    0x01010101, 0x11111111, 0x55555555,
    0x11111111, 0x55555555, 0xAAAAAAAA,
    0x55555555, 0xAAAAAAAA, 0xFFFFFFFF
};

} // namespace idm

} // namespace daisysp_idm_grids
