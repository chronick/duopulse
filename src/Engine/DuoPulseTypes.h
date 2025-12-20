#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * DuoPulse v4 Core Types and Enumerations
 *
 * These types define the fundamental vocabulary of the DuoPulse sequencer.
 * All other modules reference these definitions.
 *
 * Reference: docs/specs/main.md
 */

// =============================================================================
// Constants
// =============================================================================

/// Maximum steps in a pattern (32 = 2 bars at 16th notes)
constexpr int kMaxSteps = 32;

/// Maximum steps in a phrase (8 bars × 32 steps)
constexpr int kMaxPhraseSteps = 256;

/// Number of archetypes per genre (3x3 grid)
constexpr int kArchetypesPerGenre = 9;

/// Number of genres
constexpr int kNumGenres = 3;

// =============================================================================
// Core Enumerations
// =============================================================================

/**
 * Genre: Style bank selection
 *
 * Each genre has its own 3x3 grid of archetypes tuned to that style.
 * Reference: docs/specs/main.md section 5.4
 */
enum class Genre : uint8_t
{
    TECHNO = 0,  ///< Four-on-floor, driving, minimal-to-industrial
    TRIBAL = 1,  ///< Syncopated, polyrhythmic, off-beat emphasis
    IDM    = 2,  ///< Displaced, fragmented, controlled chaos

    COUNT  = 3
};

/**
 * Voice: Output channel identification
 *
 * Reference: docs/specs/main.md section 8.1
 */
enum class Voice : uint8_t
{
    ANCHOR  = 0,  ///< Primary voice (kick-like), Gate Out 1
    SHIMMER = 1,  ///< Secondary voice (snare-like), Gate Out 2
    AUX     = 2,  ///< Third voice (hi-hat/percussion), CV Out 1

    COUNT   = 3
};

/**
 * EnergyZone: Behavioral mode derived from ENERGY parameter
 *
 * ENERGY doesn't just scale density—it changes behavioral rules.
 * Reference: docs/specs/main.md section 4.7
 */
enum class EnergyZone : uint8_t
{
    MINIMAL = 0,  ///< 0-20%: Sparse, skeleton only, large gaps, tight timing
    GROOVE  = 1,  ///< 20-50%: Stable, danceable, locked pattern, tight timing
    BUILD   = 2,  ///< 50-75%: Increasing ghosts, phrase-end fills, timing loosens
    PEAK    = 3,  ///< 75-100%: Maximum activity, ratchets allowed, expressive timing

    COUNT   = 4
};

/**
 * AuxMode: What the AUX output (CV Out 1) produces
 *
 * Reference: docs/specs/main.md section 8.3
 */
enum class AuxMode : uint8_t
{
    HAT       = 0,  ///< Third trigger voice (ghost/hi-hat pattern)
    FILL_GATE = 1,  ///< Gate high during fill zones
    PHRASE_CV = 2,  ///< 0-5V ramp over phrase, resets at loop boundary
    EVENT     = 3,  ///< Trigger on "interesting" moments (accents, fills, changes)

    COUNT     = 4
};

/**
 * AuxDensity: Hit budget multiplier for AUX voice
 *
 * Reference: docs/specs/main.md section 4.5 (Config K3 Shift)
 */
enum class AuxDensity : uint8_t
{
    SPARSE = 0,  ///< 50% of base density
    NORMAL = 1,  ///< 100% (default)
    DENSE  = 2,  ///< 150%
    BUSY   = 3,  ///< 200%

    COUNT  = 4
};

/**
 * VoiceCoupling: How voices interact with each other
 *
 * Reference: docs/specs/main.md section 6.4
 */
enum class VoiceCoupling : uint8_t
{
    INDEPENDENT = 0,  ///< Voices fire freely, can overlap
    INTERLOCK   = 1,  ///< Suppress simultaneous hits, call-response feel
    SHADOW      = 2,  ///< Shimmer echoes anchor with 1-step delay

    COUNT       = 3
};

/**
 * ResetMode: What the reset input does
 *
 * Reference: docs/specs/main.md section 4.5 (Config K4 Primary)
 */
enum class ResetMode : uint8_t
{
    PHRASE = 0,  ///< Reset to phrase start (bar 0, step 0)
    BAR    = 1,  ///< Reset to current bar start (step 0 of current bar)
    STEP   = 2,  ///< Reset to step 0 only

    COUNT  = 3
};

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * Determine energy zone from ENERGY parameter value (0.0-1.0)
 *
 * @param energy ENERGY parameter value (0.0-1.0)
 * @return Corresponding energy zone
 */
inline EnergyZone GetEnergyZone(float energy)
{
    if (energy < 0.20f) return EnergyZone::MINIMAL;
    if (energy < 0.50f) return EnergyZone::GROOVE;
    if (energy < 0.75f) return EnergyZone::BUILD;
    return EnergyZone::PEAK;
}

/**
 * Get the density multiplier for an AuxDensity setting
 *
 * @param density AuxDensity enum value
 * @return Multiplier (0.5, 1.0, 1.5, or 2.0)
 */
inline float GetAuxDensityMultiplier(AuxDensity density)
{
    switch (density)
    {
        case AuxDensity::SPARSE: return 0.5f;
        case AuxDensity::NORMAL: return 1.0f;
        case AuxDensity::DENSE:  return 1.5f;
        case AuxDensity::BUSY:   return 2.0f;
        default:                 return 1.0f;
    }
}

/**
 * Get VoiceCoupling from a 0-1 knob value
 *
 * @param value Knob value (0.0-1.0)
 * @return VoiceCoupling enum value
 */
inline VoiceCoupling GetVoiceCouplingFromValue(float value)
{
    if (value < 0.33f) return VoiceCoupling::INDEPENDENT;
    if (value < 0.67f) return VoiceCoupling::INTERLOCK;
    return VoiceCoupling::SHADOW;
}

/**
 * Get Genre from a 0-1 knob value
 *
 * @param value Knob value (0.0-1.0)
 * @return Genre enum value
 */
inline Genre GetGenreFromValue(float value)
{
    if (value < 0.33f) return Genre::TECHNO;
    if (value < 0.67f) return Genre::TRIBAL;
    return Genre::IDM;
}

/**
 * Get AuxDensity from a 0-1 knob value
 *
 * @param value Knob value (0.0-1.0)
 * @return AuxDensity enum value
 */
inline AuxDensity GetAuxDensityFromValue(float value)
{
    if (value < 0.25f) return AuxDensity::SPARSE;
    if (value < 0.50f) return AuxDensity::NORMAL;
    if (value < 0.75f) return AuxDensity::DENSE;
    return AuxDensity::BUSY;
}

/**
 * Get AuxMode from a 0-1 knob value
 *
 * @param value Knob value (0.0-1.0)
 * @return AuxMode enum value
 */
inline AuxMode GetAuxModeFromValue(float value)
{
    if (value < 0.25f) return AuxMode::HAT;
    if (value < 0.50f) return AuxMode::FILL_GATE;
    if (value < 0.75f) return AuxMode::PHRASE_CV;
    return AuxMode::EVENT;
}

/**
 * Get ResetMode from a 0-1 knob value
 *
 * @param value Knob value (0.0-1.0)
 * @return ResetMode enum value
 */
inline ResetMode GetResetModeFromValue(float value)
{
    if (value < 0.33f) return ResetMode::PHRASE;
    if (value < 0.67f) return ResetMode::BAR;
    return ResetMode::STEP;
}

} // namespace daisysp_idm_grids
