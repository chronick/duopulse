#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * ArchetypeDNA: Complete pattern archetype definition
 *
 * Each archetype stores more than just step weights—it includes timing
 * characteristics, voice relationships, and fill behavior. This allows
 * smooth morphing between distinct rhythmic personalities.
 *
 * There are 27 total archetypes: 9 per genre (3x3 grid).
 *
 * Reference: docs/specs/main.md section 5.2
 */
struct ArchetypeDNA
{
    // =========================================================================
    // Step Weights (0.0-1.0 probability weight per step)
    // =========================================================================

    /// Anchor voice weights for each step (kick-like patterns)
    float anchorWeights[kMaxSteps];

    /// Shimmer voice weights for each step (snare-like patterns)
    float shimmerWeights[kMaxSteps];

    /// Aux voice weights for each step (hi-hat patterns)
    float auxWeights[kMaxSteps];

    // =========================================================================
    // Accent Eligibility
    // =========================================================================

    /// Bitmask: which steps CAN accent for anchor (1 = accent-eligible)
    uint32_t anchorAccentMask;

    /// Bitmask: which steps CAN accent for shimmer (1 = accent-eligible)
    uint32_t shimmerAccentMask;

    // =========================================================================
    // Timing Characteristics
    // =========================================================================

    /// Base swing amount for this archetype (0.0-1.0)
    float swingAmount;

    /// Swing pattern type: 0=8ths, 1=16ths, 2=mixed
    float swingPattern;

    // =========================================================================
    // Voice Relationship Defaults
    // =========================================================================

    /// Suggested COUPLE value for this archetype (0.0-1.0)
    /// 0.0-0.33 = Independent, 0.33-0.67 = Interlock, 0.67-1.0 = Shadow
    float defaultCouple;

    // =========================================================================
    // Fill Behavior
    // =========================================================================

    /// How much denser patterns get during fills (1.0 = no change, 2.0 = double)
    float fillDensityMultiplier;

    /// Bitmask: which steps are eligible for ratcheting during fills
    uint32_t ratchetEligibleMask;

    // =========================================================================
    // Metadata
    // =========================================================================

    /// Position in grid (0-2) along X axis (syncopation)
    uint8_t gridX;

    /// Position in grid (0-2) along Y axis (complexity)
    uint8_t gridY;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize with default values (minimal techno-style pattern)
     */
    void Init()
    {
        // Default: four-on-floor kick, backbeat snare
        for (int i = 0; i < kMaxSteps; ++i)
        {
            // Downbeats strongest for anchor (steps 0, 16 = bar downbeats)
            if (i == 0 || i == 16)
            {
                anchorWeights[i] = 1.0f;
            }
            else if (i == 8 || i == 24)  // Half notes
            {
                anchorWeights[i] = 0.85f;
            }
            else if (i % 4 == 0)  // Quarter notes
            {
                anchorWeights[i] = 0.7f;
            }
            else if (i % 2 == 0)  // 8th notes
            {
                anchorWeights[i] = 0.3f;
            }
            else  // 16th notes
            {
                anchorWeights[i] = 0.15f;
            }

            // Backbeats strongest for shimmer (steps 8, 24)
            if (i == 8 || i == 24)
            {
                shimmerWeights[i] = 1.0f;
            }
            else if (i % 8 == 4)
            {
                shimmerWeights[i] = 0.6f;
            }
            else if (i % 2 == 0)
            {
                shimmerWeights[i] = 0.3f;
            }
            else
            {
                shimmerWeights[i] = 0.15f;
            }

            // Aux follows 8th notes
            auxWeights[i] = (i % 2 == 0) ? 0.6f : 0.3f;
        }

        // Default accent masks: downbeats and backbeats
        anchorAccentMask  = 0x01010101;  // Steps 0, 8, 16, 24
        shimmerAccentMask = 0x01000100;  // Steps 8, 24

        // Default timing
        swingAmount  = 0.0f;
        swingPattern = 0.0f;

        // Default voice relationship: slight interlock
        defaultCouple = 0.4f;

        // Default fill behavior
        fillDensityMultiplier = 1.5f;
        ratchetEligibleMask   = 0x11111111;  // Every 4th step

        // Default position: origin
        gridX = 0;
        gridY = 0;
    }
};

/**
 * GenreField: 3x3 grid of archetypes for a single genre
 *
 * FIELD X (0-2): Syncopation (straight → syncopated → broken)
 * FIELD Y (0-2): Complexity (sparse → medium → dense)
 *
 * Reference: docs/specs/main.md section 5.1
 */
struct GenreField
{
    /// 3x3 grid of archetypes indexed as [y][x]
    ArchetypeDNA archetypes[3][3];

    /**
     * Get archetype at grid position
     *
     * @param x Grid X position (0-2, syncopation axis)
     * @param y Grid Y position (0-2, complexity axis)
     * @return Reference to archetype at that position
     */
    const ArchetypeDNA& GetArchetype(int x, int y) const
    {
        // Clamp to valid range
        if (x < 0) x = 0;
        if (x > 2) x = 2;
        if (y < 0) y = 0;
        if (y > 2) y = 2;

        return archetypes[y][x];
    }

    /**
     * Initialize all archetypes with defaults
     */
    void Init()
    {
        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                archetypes[y][x].Init();
                archetypes[y][x].gridX = static_cast<uint8_t>(x);
                archetypes[y][x].gridY = static_cast<uint8_t>(y);
            }
        }
    }
};

} // namespace daisysp_idm_grids
