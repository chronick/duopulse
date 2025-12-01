#pragma once

#include "PatternSkeleton.h"

namespace daisysp_idm_grids
{

/**
 * DuoPulse v2 Pattern Library
 * 
 * 16 skeleton patterns optimized for 2-voice output (Anchor/Shimmer).
 * Patterns are organized by genre affinity:
 *   0-3:   Techno (four-on-floor, minimal, driving, pounding)
 *   4-7:   Tribal (clave, interlocking, polyrhythmic, circular)
 *   8-11:  Trip-Hop (sparse, lazy, heavy, behind-beat)
 *   12-15: IDM (broken, glitch, irregular, chaos)
 * 
 * Intensity values (0-15):
 *   0     = Step off
 *   1-4   = Ghost note
 *   5-10  = Normal hit
 *   11-15 = Strong hit (accent candidate)
 * 
 * Reference: docs/specs/main.md section "Pattern Generation [duopulse-patterns]"
 */

// Helper macro to pack two 4-bit values into one byte
// HIGH nibble = even step, LOW nibble = odd step
#define PACK(even, odd) (((even) << 4) | ((odd) & 0x0F))

/**
 * Pattern 0: Techno Four-on-Floor
 * Classic techno kick pattern with straight hi-hats
 * Anchor: Strong kicks on quarter notes (0, 4, 8, 12, 16, 20, 24, 28)
 * Shimmer: 8th note hats, accent on off-beats
 */
constexpr PatternSkeleton kPattern0_TechnoFour = {
    // Anchor: Kicks on quarter notes
    // Steps: 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15  16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    {
        PACK(15, 0), PACK(0, 0), PACK(15, 0), PACK(0, 0),  // Steps 0-7
        PACK(15, 0), PACK(0, 0), PACK(15, 0), PACK(0, 0),  // Steps 8-15
        PACK(15, 0), PACK(0, 0), PACK(15, 0), PACK(0, 0),  // Steps 16-23
        PACK(15, 0), PACK(0, 0), PACK(15, 0), PACK(0, 0),  // Steps 24-31
    },
    // Shimmer: 8th notes with accents on off-beats (2, 6, 10, 14...)
    {
        PACK(6, 0), PACK(12, 0), PACK(6, 0), PACK(12, 0),  // Steps 0-7
        PACK(6, 0), PACK(12, 0), PACK(6, 0), PACK(12, 0),  // Steps 8-15
        PACK(6, 0), PACK(12, 0), PACK(6, 0), PACK(12, 0),  // Steps 16-23
        PACK(6, 0), PACK(12, 0), PACK(6, 0), PACK(12, 0),  // Steps 24-31
    },
    0x11111111,               // Accent on every quarter note
    PatternRelationship::Free,
    GenreAffinity::Techno
};

/**
 * Pattern 1: Techno Minimal
 * Sparse, hypnotic pattern
 * Anchor: Kick on 1 and occasional ghost
 * Shimmer: Minimal hats, emphasis on space
 */
constexpr PatternSkeleton kPattern1_TechnoMinimal = {
    // Anchor: Sparse kicks with ghost notes
    {
        PACK(15, 0), PACK(0, 0), PACK(0, 0), PACK(3, 0),   // Steps 0-7
        PACK(13, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(15, 0), PACK(0, 0), PACK(0, 0), PACK(3, 0),   // Steps 16-23
        PACK(13, 0), PACK(0, 0), PACK(2, 0), PACK(0, 0),   // Steps 24-31
    },
    // Shimmer: Very sparse
    {
        PACK(0, 0), PACK(8, 0), PACK(0, 0), PACK(0, 0),    // Steps 0-7
        PACK(0, 0), PACK(10, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(0, 0), PACK(8, 0), PACK(0, 0), PACK(0, 0),    // Steps 16-23
        PACK(0, 0), PACK(10, 0), PACK(0, 0), PACK(4, 0),   // Steps 24-31
    },
    0x01010101,               // Accent only on downbeats
    PatternRelationship::Free,
    GenreAffinity::Techno
};

/**
 * Pattern 2: Techno Driving
 * Relentless energy, 16th note hats
 * Anchor: Solid four-on-floor with some ghost notes
 * Shimmer: Constant 16ths with varying intensity
 */
constexpr PatternSkeleton kPattern2_TechnoDriving = {
    // Anchor: Four-on-floor with ghost notes filling in
    {
        PACK(15, 2), PACK(3, 2), PACK(14, 2), PACK(3, 2),  // Steps 0-7
        PACK(15, 2), PACK(3, 2), PACK(14, 2), PACK(3, 2),  // Steps 8-15
        PACK(15, 2), PACK(3, 2), PACK(14, 2), PACK(3, 2),  // Steps 16-23
        PACK(15, 2), PACK(3, 2), PACK(14, 2), PACK(4, 2),  // Steps 24-31
    },
    // Shimmer: Constant 16ths
    {
        PACK(8, 5), PACK(7, 5), PACK(12, 5), PACK(7, 5),   // Steps 0-7
        PACK(8, 5), PACK(7, 5), PACK(12, 5), PACK(7, 5),   // Steps 8-15
        PACK(8, 5), PACK(7, 5), PACK(12, 5), PACK(7, 5),   // Steps 16-23
        PACK(8, 5), PACK(7, 5), PACK(12, 5), PACK(7, 6),   // Steps 24-31
    },
    0x11111111,
    PatternRelationship::Free,
    GenreAffinity::Techno
};

/**
 * Pattern 3: Techno Pounding
 * Heavy, industrial feel
 * Anchor: Double kicks and syncopation
 * Shimmer: Industrial clang accents
 */
constexpr PatternSkeleton kPattern3_TechnoPounding = {
    // Anchor: Heavy with double kicks
    {
        PACK(15, 12), PACK(0, 0), PACK(14, 0), PACK(0, 0), // Steps 0-7
        PACK(15, 12), PACK(0, 0), PACK(14, 0), PACK(10, 0),// Steps 8-15
        PACK(15, 12), PACK(0, 0), PACK(14, 0), PACK(0, 0), // Steps 16-23
        PACK(15, 12), PACK(0, 0), PACK(14, 0), PACK(10, 0),// Steps 24-31
    },
    // Shimmer: Sparse industrial hits
    {
        PACK(0, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),   // Steps 0-7
        PACK(0, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),   // Steps 8-15
        PACK(0, 0), PACK(0, 0), PACK(13, 0), PACK(0, 6),   // Steps 16-23
        PACK(0, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),   // Steps 24-31
    },
    0x05050505,               // Accent on 1 and 3 of each bar
    PatternRelationship::Interlock,
    GenreAffinity::Techno
};

/**
 * Pattern 4: Tribal Clave
 * Based on son clave rhythm
 * Anchor: 3-2 clave feel
 * Shimmer: Fills between clave hits
 */
constexpr PatternSkeleton kPattern4_TribalClave = {
    // Anchor: Son clave pattern adapted to 32 steps
    // Classic clave: X..X..X...X.X...
    {
        PACK(15, 0), PACK(0, 0), PACK(0, 13), PACK(0, 0),  // Steps 0-7: X..X
        PACK(0, 0), PACK(12, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15: ..X.
        PACK(0, 0), PACK(0, 14), PACK(0, 12), PACK(0, 0),  // Steps 16-23: .X.X
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 24-31: ....
    },
    // Shimmer: Congas filling gaps
    {
        PACK(0, 8), PACK(6, 0), PACK(0, 0), PACK(9, 6),    // Steps 0-7
        PACK(7, 0), PACK(0, 8), PACK(6, 0), PACK(0, 8),    // Steps 8-15
        PACK(6, 0), PACK(0, 0), PACK(0, 0), PACK(7, 6),    // Steps 16-23
        PACK(0, 8), PACK(6, 0), PACK(0, 8), PACK(6, 0),    // Steps 24-31
    },
    0x00240024,               // Accent on clave hits
    PatternRelationship::Interlock,
    GenreAffinity::Tribal
};

/**
 * Pattern 5: Tribal Interlocking
 * Anchor and shimmer designed to perfectly interlock
 * Creates polyrhythmic texture
 */
constexpr PatternSkeleton kPattern5_TribalInterlock = {
    // Anchor: Djembe-like pattern
    {
        PACK(14, 0), PACK(0, 10), PACK(0, 0), PACK(12, 0), // Steps 0-7
        PACK(0, 0), PACK(0, 10), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(14, 0), PACK(0, 10), PACK(0, 0), PACK(12, 0), // Steps 16-23
        PACK(0, 0), PACK(0, 12), PACK(0, 0), PACK(0, 0),   // Steps 24-31
    },
    // Shimmer: Fills every gap
    {
        PACK(0, 9), PACK(8, 0), PACK(9, 8), PACK(0, 9),    // Steps 0-7
        PACK(10, 8), PACK(9, 0), PACK(8, 9), PACK(8, 9),   // Steps 8-15
        PACK(0, 9), PACK(8, 0), PACK(9, 8), PACK(0, 9),    // Steps 16-23
        PACK(10, 8), PACK(9, 0), PACK(8, 9), PACK(8, 9),   // Steps 24-31
    },
    0x09090909,
    PatternRelationship::Interlock,
    GenreAffinity::Tribal
};

/**
 * Pattern 6: Tribal Polyrhythmic
 * 3-against-4 polyrhythm feel
 */
constexpr PatternSkeleton kPattern6_TribalPoly = {
    // Anchor: 4-beat pattern
    {
        PACK(15, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),  // Steps 0-7
        PACK(14, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),  // Steps 8-15
        PACK(15, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),  // Steps 16-23
        PACK(14, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),  // Steps 24-31
    },
    // Shimmer: 3-beat pattern (every ~10.67 steps, approximated)
    {
        PACK(12, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 0-7
        PACK(0, 0), PACK(11, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(0, 0), PACK(0, 12), PACK(0, 0), PACK(0, 0),   // Steps 16-23
        PACK(0, 0), PACK(0, 0), PACK(0, 11), PACK(0, 0),   // Steps 24-31
    },
    0x01210121,
    PatternRelationship::Free,
    GenreAffinity::Tribal
};

/**
 * Pattern 7: Tribal Circular
 * Hypnotic, circular pattern for extended grooves
 */
constexpr PatternSkeleton kPattern7_TribalCircular = {
    // Anchor: Rotating emphasis
    {
        PACK(14, 0), PACK(8, 0), PACK(10, 0), PACK(8, 0),  // Steps 0-7
        PACK(12, 0), PACK(8, 0), PACK(10, 0), PACK(8, 0),  // Steps 8-15
        PACK(10, 0), PACK(8, 0), PACK(14, 0), PACK(8, 0),  // Steps 16-23
        PACK(12, 0), PACK(8, 0), PACK(10, 0), PACK(9, 0),  // Steps 24-31
    },
    // Shimmer: Counter-rhythm
    {
        PACK(0, 10), PACK(0, 8), PACK(0, 10), PACK(0, 8),  // Steps 0-7
        PACK(0, 12), PACK(0, 8), PACK(0, 10), PACK(0, 8),  // Steps 8-15
        PACK(0, 10), PACK(0, 8), PACK(0, 12), PACK(0, 8),  // Steps 16-23
        PACK(0, 10), PACK(0, 8), PACK(0, 10), PACK(0, 9),  // Steps 24-31
    },
    0x11111111,
    PatternRelationship::Interlock,
    GenreAffinity::Tribal | GenreAffinity::Techno
};

/**
 * Pattern 8: Trip-Hop Sparse
 * Minimal, spacious pattern
 * Heavy kick, sparse snare
 */
constexpr PatternSkeleton kPattern8_TripHopSparse = {
    // Anchor: Heavy, sparse kicks
    {
        PACK(15, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 0-7
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 8-15
        PACK(14, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 16-23
        PACK(0, 0), PACK(0, 3), PACK(0, 0), PACK(0, 0),    // Steps 24-31
    },
    // Shimmer: Very sparse snare
    {
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 0-7
        PACK(13, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 16-23
        PACK(12, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 24-31
    },
    0x01010101,
    PatternRelationship::Free,
    GenreAffinity::TripHop
};

/**
 * Pattern 9: Trip-Hop Lazy
 * Behind-the-beat feel, ghost notes
 */
constexpr PatternSkeleton kPattern9_TripHopLazy = {
    // Anchor: Lazy kick with ghost notes
    {
        PACK(15, 0), PACK(0, 0), PACK(0, 3), PACK(0, 0),   // Steps 0-7
        PACK(0, 0), PACK(0, 0), PACK(10, 0), PACK(0, 0),   // Steps 8-15
        PACK(14, 0), PACK(0, 0), PACK(0, 2), PACK(0, 0),   // Steps 16-23
        PACK(0, 0), PACK(0, 0), PACK(0, 11), PACK(0, 0),   // Steps 24-31
    },
    // Shimmer: Snare ghosts building to hit
    {
        PACK(0, 0), PACK(0, 0), PACK(0, 3), PACK(4, 0),    // Steps 0-7
        PACK(13, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(0, 0), PACK(0, 0), PACK(0, 2), PACK(3, 0),    // Steps 16-23
        PACK(12, 0), PACK(0, 0), PACK(0, 0), PACK(0, 4),   // Steps 24-31
    },
    0x01010101,
    PatternRelationship::Shadow,
    GenreAffinity::TripHop
};

/**
 * Pattern 10: Trip-Hop Heavy
 * Massive Sound, emphasis on weight
 */
constexpr PatternSkeleton kPattern10_TripHopHeavy = {
    // Anchor: Crushing kicks
    {
        PACK(15, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 0-7
        PACK(0, 0), PACK(14, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(15, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 16-23
        PACK(0, 0), PACK(0, 0), PACK(0, 13), PACK(0, 0),   // Steps 24-31
    },
    // Shimmer: Heavy snare with drag
    {
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 0-7
        PACK(14, 3), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 16-23
        PACK(13, 2), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 24-31
    },
    0x01010101,
    PatternRelationship::Free,
    GenreAffinity::TripHop
};

/**
 * Pattern 11: Trip-Hop Groove
 * More active hip-hop influenced pattern
 */
constexpr PatternSkeleton kPattern11_TripHopGroove = {
    // Anchor: Syncopated kick
    {
        PACK(15, 0), PACK(0, 0), PACK(0, 10), PACK(0, 0),  // Steps 0-7
        PACK(0, 0), PACK(0, 0), PACK(13, 0), PACK(0, 0),   // Steps 8-15
        PACK(14, 0), PACK(0, 0), PACK(0, 0), PACK(11, 0),  // Steps 16-23
        PACK(0, 0), PACK(0, 0), PACK(0, 12), PACK(0, 0),   // Steps 24-31
    },
    // Shimmer: Offbeat snares
    {
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 0-7
        PACK(12, 0), PACK(0, 4), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),    // Steps 16-23
        PACK(11, 0), PACK(0, 3), PACK(0, 0), PACK(0, 5),   // Steps 24-31
    },
    0x01210121,
    PatternRelationship::Free,
    GenreAffinity::TripHop | GenreAffinity::Tribal
};

/**
 * Pattern 12: IDM Broken
 * Fragmented, glitchy pattern
 */
constexpr PatternSkeleton kPattern12_IdmBroken = {
    // Anchor: Fragmented kicks
    {
        PACK(15, 0), PACK(0, 12), PACK(0, 0), PACK(0, 0),  // Steps 0-7
        PACK(0, 0), PACK(0, 0), PACK(13, 0), PACK(14, 0),  // Steps 8-15
        PACK(0, 0), PACK(0, 0), PACK(0, 0), PACK(11, 0),   // Steps 16-23
        PACK(0, 15), PACK(0, 0), PACK(0, 0), PACK(0, 12),  // Steps 24-31
    },
    // Shimmer: Irregular snare/hat bursts
    {
        PACK(0, 0), PACK(10, 0), PACK(0, 11), PACK(0, 0),  // Steps 0-7
        PACK(12, 0), PACK(0, 0), PACK(0, 0), PACK(0, 10),  // Steps 8-15
        PACK(0, 13), PACK(0, 0), PACK(9, 10), PACK(0, 0),  // Steps 16-23
        PACK(0, 0), PACK(11, 0), PACK(0, 0), PACK(0, 0),   // Steps 24-31
    },
    0x82418241,               // Irregular accents
    PatternRelationship::Free,
    GenreAffinity::IDM
};

/**
 * Pattern 13: IDM Glitch
 * Micro-edits, stutters
 */
constexpr PatternSkeleton kPattern13_IdmGlitch = {
    // Anchor: Stutter kicks
    {
        PACK(15, 13), PACK(11, 0), PACK(0, 0), PACK(0, 0), // Steps 0-7
        PACK(0, 0), PACK(14, 12), PACK(10, 0), PACK(0, 0), // Steps 8-15
        PACK(0, 0), PACK(0, 0), PACK(0, 15), PACK(13, 11), // Steps 16-23
        PACK(0, 0), PACK(0, 0), PACK(14, 0), PACK(0, 0),   // Steps 24-31
    },
    // Shimmer: Glitchy fills
    {
        PACK(0, 0), PACK(0, 0), PACK(12, 11), PACK(10, 9), // Steps 0-7
        PACK(8, 0), PACK(0, 0), PACK(0, 0), PACK(11, 10),  // Steps 8-15
        PACK(9, 8), PACK(7, 0), PACK(0, 0), PACK(0, 0),    // Steps 16-23
        PACK(0, 12), PACK(11, 10), PACK(0, 0), PACK(13, 0),// Steps 24-31
    },
    0x03830383,
    PatternRelationship::Free,
    GenreAffinity::IDM
};

/**
 * Pattern 14: IDM Irregular
 * Unpredictable placement
 */
constexpr PatternSkeleton kPattern14_IdmIrregular = {
    // Anchor: Seemingly random but designed
    {
        PACK(14, 0), PACK(0, 0), PACK(0, 0), PACK(0, 12),  // Steps 0-7
        PACK(0, 0), PACK(0, 15), PACK(0, 0), PACK(0, 0),   // Steps 8-15
        PACK(13, 0), PACK(0, 0), PACK(0, 0), PACK(0, 0),   // Steps 16-23
        PACK(0, 0), PACK(0, 0), PACK(0, 14), PACK(0, 0),   // Steps 24-31
    },
    // Shimmer: Counter-irregular
    {
        PACK(0, 0), PACK(11, 0), PACK(0, 0), PACK(0, 0),   // Steps 0-7
        PACK(0, 12), PACK(0, 0), PACK(0, 10), PACK(0, 0),  // Steps 8-15
        PACK(0, 0), PACK(0, 13), PACK(0, 0), PACK(11, 0),  // Steps 16-23
        PACK(0, 0), PACK(12, 0), PACK(0, 0), PACK(0, 10),  // Steps 24-31
    },
    0x42214221,
    PatternRelationship::Free,
    GenreAffinity::IDM
};

/**
 * Pattern 15: IDM Chaos
 * Maximum complexity
 */
constexpr PatternSkeleton kPattern15_IdmChaos = {
    // Anchor: Dense, chaotic
    {
        PACK(15, 8), PACK(0, 10), PACK(12, 0), PACK(0, 9), // Steps 0-7
        PACK(0, 11), PACK(0, 0), PACK(14, 7), PACK(0, 0),  // Steps 8-15
        PACK(10, 0), PACK(13, 8), PACK(0, 0), PACK(11, 0), // Steps 16-23
        PACK(0, 9), PACK(15, 0), PACK(0, 10), PACK(12, 0), // Steps 24-31
    },
    // Shimmer: Equally chaotic
    {
        PACK(0, 11), PACK(9, 0), PACK(0, 12), PACK(10, 0), // Steps 0-7
        PACK(13, 0), PACK(8, 11), PACK(0, 0), PACK(12, 9), // Steps 8-15
        PACK(0, 10), PACK(0, 0), PACK(11, 8), PACK(0, 12), // Steps 16-23
        PACK(9, 0), PACK(0, 11), PACK(10, 0), PACK(0, 13), // Steps 24-31
    },
    0xAAAAAAAA,               // Alternating accents for chaos
    PatternRelationship::Free,
    GenreAffinity::IDM
};

#undef PACK

/**
 * Array of all 16 patterns for indexed access
 */
constexpr PatternSkeleton kPatterns[kNumPatterns] = {
    kPattern0_TechnoFour,
    kPattern1_TechnoMinimal,
    kPattern2_TechnoDriving,
    kPattern3_TechnoPounding,
    kPattern4_TribalClave,
    kPattern5_TribalInterlock,
    kPattern6_TribalPoly,
    kPattern7_TribalCircular,
    kPattern8_TripHopSparse,
    kPattern9_TripHopLazy,
    kPattern10_TripHopHeavy,
    kPattern11_TripHopGroove,
    kPattern12_IdmBroken,
    kPattern13_IdmGlitch,
    kPattern14_IdmIrregular,
    kPattern15_IdmChaos,
};

/**
 * Get pattern by index (0-15).
 * Returns Pattern 0 if index out of range.
 */
inline const PatternSkeleton& GetPattern(int index)
{
    if(index < 0 || index >= kNumPatterns)
        index = 0;
    return kPatterns[index];
}

/**
 * Get pattern index from grid parameter (0.0-1.0).
 * Maps grid value to pattern index 0-15.
 */
inline int GetPatternIndex(float grid)
{
    if(grid < 0.0f)
        grid = 0.0f;
    if(grid > 1.0f)
        grid = 1.0f;
    return static_cast<int>(grid * 15.0f + 0.5f);
}

/**
 * Get a pattern suitable for the given terrain.
 * If current pattern doesn't match genre, suggests alternatives.
 * 
 * @param currentIndex Current pattern index
 * @param terrain Terrain parameter (0-1)
 * @return Suggested pattern index (may be same as input)
 */
inline int SuggestPatternForTerrain(int currentIndex, float terrain)
{
    const PatternSkeleton& current = GetPattern(currentIndex);
    if(PatternSuitsGenre(current, terrain))
        return currentIndex;

    // Find first pattern that suits this terrain
    // Patterns are organized: 0-3 Techno, 4-7 Tribal, 8-11 TripHop, 12-15 IDM
    if(terrain < 0.25f)
        return 0;  // Techno
    if(terrain < 0.50f)
        return 4;  // Tribal
    if(terrain < 0.75f)
        return 8;  // Trip-Hop
    return 12;     // IDM
}

} // namespace daisysp_idm_grids

