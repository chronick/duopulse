#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * DuoPulse v5 Core Types and Enumerations
 *
 * These types define the fundamental vocabulary of the DuoPulse sequencer.
 * All other modules reference these definitions.
 *
 * V5 Changes (Task 27):
 * - Removed Genre enum (hardcoded to TECHNO behavior)
 * - Removed AuxDensity enum (simplified to continuous parameter)
 * - Removed VoiceCoupling enum (replaced by COMPLEMENT in Task 30)
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

// =============================================================================
// Core Enumerations
// =============================================================================

/**
 * Genre: Style bank selection (V5: internal only, hardcoded to TECHNO)
 *
 * In V5, Genre is no longer exposed in the UI - TECHNO behavior is the default.
 * The enum is kept for backward compatibility with helper functions.
 *
 * Reference: docs/specs/main.md section 5
 */
enum class Genre : uint8_t
{
    TECHNO = 0,  ///< Four-on-floor, driving, minimal-to-industrial (V5 default)
    TRIBAL = 1,  ///< Syncopated, polyrhythmic, off-beat emphasis (internal only)
    IDM    = 2,  ///< Displaced, fragmented, controlled chaos (internal only)

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
 * AuxDensity: Hit budget multiplier for AUX voice (V5: internal only)
 *
 * V5: AuxDensity is no longer exposed in UI. Default is NORMAL.
 * The enum is kept for internal generation pipeline compatibility.
 *
 * Reference: docs/specs/main.md section 4.5
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
 * VoiceCoupling: How voices interact with each other (V5: internal only)
 *
 * V5: VoiceCoupling is no longer exposed in UI. Default is INDEPENDENT.
 * Task 30 will add COMPLEMENT mode for voice relationships.
 * The enum is kept for internal generation pipeline compatibility.
 *
 * Reference: docs/specs/main.md section 6.4
 */
enum class VoiceCoupling : uint8_t
{
    INDEPENDENT = 0,  ///< Voices fire freely, can overlap (V5 default)
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
 * Get VoiceCoupling from a 0-1 knob value (V5: deprecated, always returns INDEPENDENT)
 *
 * V5: VoiceCoupling is no longer exposed in UI. This function is kept for
 * backward compatibility but always returns INDEPENDENT.
 *
 * @param value Knob value (0.0-1.0) - ignored in V5
 * @return VoiceCoupling::INDEPENDENT always
 */
inline VoiceCoupling GetVoiceCouplingFromValue(float value)
{
    (void)value;  // V5: ignore knob value
    return VoiceCoupling::INDEPENDENT;
}

/**
 * Get Genre from a 0-1 knob value (V5: deprecated, always returns TECHNO)
 *
 * V5: Genre is no longer exposed in UI. This function is kept for
 * backward compatibility but always returns TECHNO.
 *
 * @param value Knob value (0.0-1.0) - ignored in V5
 * @return Genre::TECHNO always
 */
inline Genre GetGenreFromValue(float value)
{
    (void)value;  // V5: ignore knob value
    return Genre::TECHNO;
}

/**
 * Get AuxDensity from a 0-1 knob value (V5: deprecated, always returns NORMAL)
 *
 * V5: AuxDensity is no longer exposed in UI. This function is kept for
 * backward compatibility but always returns NORMAL.
 *
 * @param value Knob value (0.0-1.0) - ignored in V5
 * @return AuxDensity::NORMAL always
 */
inline AuxDensity GetAuxDensityFromValue(float value)
{
    (void)value;  // V5: ignore knob value
    return AuxDensity::NORMAL;
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
