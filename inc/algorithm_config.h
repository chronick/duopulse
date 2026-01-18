/**
 * Algorithm Weight Configuration
 *
 * Configurable parameters for the weight-based algorithm blending system.
 * These control how SHAPE parameter blends between euclidean, syncopation,
 * and random pattern generation methods.
 *
 * This file is the source of truth for algorithm tuning. Task 59 will add
 * JSON configuration and code generation to make these adjustable without
 * recompilation.
 *
 * Reference: docs/tasks/active/56-weight-based-blending.md
 */

#pragma once

namespace AlgorithmConfig {

// =============================================================================
// EUCLIDEAN INFLUENCE CURVE
// Controls how strongly euclidean patterns dominate at low SHAPE values
// =============================================================================

/// SHAPE value where euclidean weight starts fading (0.0-1.0)
/// Below this: euclidean at full strength
constexpr float kEuclideanFadeStart = 0.30f;

/// SHAPE value where euclidean weight reaches zero (0.0-1.0)
/// Above this: no euclidean contribution
constexpr float kEuclideanFadeEnd = 0.70f;

// =============================================================================
// SYNCOPATION BELL CURVE
// Controls the middle "syncopated zone" peak
// =============================================================================

/// SHAPE value at peak of syncopation contribution (0.0-1.0)
constexpr float kSyncopationCenter = 0.50f;

/// Width of syncopation bell curve (standard deviation, 0.0-1.0)
/// Larger = wider curve, syncopation active over broader SHAPE range
constexpr float kSyncopationWidth = 0.30f;

// =============================================================================
// RANDOM INFLUENCE CURVE
// Controls how quickly randomness takes over at high SHAPE
// =============================================================================

/// SHAPE value where random weight starts appearing (0.0-1.0)
constexpr float kRandomFadeStart = 0.50f;

/// SHAPE value where random weight reaches full strength (0.0-1.0)
constexpr float kRandomFadeEnd = 0.90f;

// =============================================================================
// PER-CHANNEL EUCLIDEAN K RANGES
// k = number of hits in euclidean(n, k) pattern
// =============================================================================

/// Anchor (kick) euclidean k range: sparser, foundational
constexpr int kAnchorKMin = 4;   ///< Minimum hits at ENERGY=0
constexpr int kAnchorKMax = 12;  ///< Maximum hits at ENERGY=1

/// Shimmer (hi-hat/snare) euclidean k range: more active
constexpr int kShimmerKMin = 6;  ///< Minimum hits at ENERGY=0
constexpr int kShimmerKMax = 16; ///< Maximum hits at ENERGY=1

/// Aux (perc) euclidean k range: variable, often sparse
constexpr int kAuxKMin = 2;      ///< Minimum hits at ENERGY=0
constexpr int kAuxKMax = 8;      ///< Maximum hits at ENERGY=1

// =============================================================================
// BOOTSTRAP LEVER TABLE
// Manual heuristics for /iterate command until Task 63 (Sensitivity Analysis)
// provides data-driven recommendations.
//
// Format: {metric, primary_lever, direction, secondary_lever}
// Direction: "+" = increase lever to improve metric, "-" = decrease
// Confidence: how confident we are in this heuristic (1-5 scale)
//
// These are educated guesses based on algorithm understanding.
// Task 63 will generate empirical data to validate/replace these.
// =============================================================================

namespace BootstrapLevers {

// ---------------------------------------------------------------------------
// Syncopation Improvement
// To make patterns more syncopated (funk-style displaced rhythms):
// ---------------------------------------------------------------------------
// Primary: kSyncopationCenter (+) - shifts bell curve peak toward higher SHAPE
//          Higher center means syncopation dominates more of SHAPE range
// Secondary: kRandomFadeStart (-) - earlier random injection adds unpredictability
// Confidence: 4/5 - syncopation center directly controls syncopation zone
constexpr const char* kSyncopationPrimary = "kSyncopationCenter";
constexpr const char* kSyncopationDirection = "+";
constexpr const char* kSyncopationSecondary = "kRandomFadeStart";
constexpr int kSyncopationConfidence = 4;

// ---------------------------------------------------------------------------
// Density/Regularity Improvement
// To make patterns more structured and predictable:
// ---------------------------------------------------------------------------
// Primary: kEuclideanFadeEnd (+) - euclidean persists to higher SHAPE values
//          More euclidean = more regular, structured patterns
// Secondary: kSyncopationWidth (-) - narrower bell = more predictable middle zone
// Confidence: 4/5 - euclidean is inherently regular
constexpr const char* kRegularityPrimary = "kEuclideanFadeEnd";
constexpr const char* kRegularityDirection = "+";
constexpr const char* kRegularitySecondary = "kSyncopationWidth";
constexpr int kRegularityConfidence = 4;

// ---------------------------------------------------------------------------
// Voice Separation Improvement
// To create more distinct anchor vs shimmer patterns:
// ---------------------------------------------------------------------------
// Primary: shimmer drift parameter (+) - more offset from anchor
// Secondary: kAnchorKMax (-) - sparser anchor = more gaps for shimmer to fill
// Note: drift is a runtime param, not config - but affects voice separation
// Confidence: 3/5 - voice separation is complex, multi-factor
constexpr const char* kVoiceSeparationPrimary = "drift";  // runtime param
constexpr const char* kVoiceSeparationDirection = "+";
constexpr const char* kVoiceSeparationSecondary = "kAnchorKMax";
constexpr int kVoiceSeparationConfidence = 3;

// ---------------------------------------------------------------------------
// Velocity Variation Improvement
// To increase dynamic range in accent patterns:
// ---------------------------------------------------------------------------
// Primary: accent parameter (+) - direct control over velocity dynamics
// Secondary: kSyncopationCenter (+) - syncopation creates natural accent points
// Note: accent is a runtime param
// Confidence: 3/5 - velocity depends on multiple factors
constexpr const char* kVelocityVariationPrimary = "accent";  // runtime param
constexpr const char* kVelocityVariationDirection = "+";
constexpr const char* kVelocityVariationSecondary = "kSyncopationCenter";
constexpr int kVelocityVariationConfidence = 3;

// ---------------------------------------------------------------------------
// Wild Zone Responsiveness
// To make high-SHAPE patterns more chaotic/unpredictable:
// ---------------------------------------------------------------------------
// Primary: kRandomFadeStart (-) - random kicks in earlier
// Secondary: kEuclideanFadeStart (-) - euclidean fades earlier, less structure
// Confidence: 4/5 - direct control over wild zone behavior
constexpr const char* kWildZonePrimary = "kRandomFadeStart";
constexpr const char* kWildZoneDirection = "-";
constexpr const char* kWildZoneSecondary = "kEuclideanFadeStart";
constexpr int kWildZoneConfidence = 4;

// ---------------------------------------------------------------------------
// Stable Zone Tightness
// To make low-SHAPE patterns more four-on-floor:
// ---------------------------------------------------------------------------
// Primary: kEuclideanFadeStart (+) - euclidean stays pure longer
// Secondary: kAnchorKMin (-) - fewer anchor hits = sparser, more focused
// Confidence: 5/5 - euclidean directly creates stable patterns
constexpr const char* kStableZonePrimary = "kEuclideanFadeStart";
constexpr const char* kStableZoneDirection = "+";
constexpr const char* kStableZoneSecondary = "kAnchorKMin";
constexpr int kStableZoneConfidence = 5;

} // namespace BootstrapLevers

} // namespace AlgorithmConfig
