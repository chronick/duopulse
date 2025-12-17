#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * DuoPulse v2 Pattern Skeleton Structure
 * 
 * 32-step patterns optimized for 2-voice output (Anchor/Shimmer).
 * Each step has a 4-bit intensity value (0-15) that works with density
 * threshold to determine which steps fire.
 * 
 * Density Application:
 *   - Density controls a threshold against pattern intensity
 *   - Low density: only high-intensity steps fire
 *   - High density: all steps including ghosts fire
 * 
 * Reference: docs/specs/main.md section "Pattern Generation [duopulse-patterns]"
 */

// Number of skeleton patterns available
constexpr int kNumPatterns = 16;

// Pattern length in steps
constexpr int kPatternSteps = 32;

// Genre affinity bitfield flags
// Patterns can be suitable for multiple genres
namespace GenreAffinity
{
constexpr uint8_t Techno  = (1 << 0);  // 0-25% terrain
constexpr uint8_t Tribal  = (1 << 1);  // 25-50% terrain
constexpr uint8_t TripHop = (1 << 2);  // 50-75% terrain
constexpr uint8_t IDM     = (1 << 3);  // 75-100% terrain
constexpr uint8_t All     = Techno | Tribal | TripHop | IDM;
} // namespace GenreAffinity

// Default relationship modes for patterns
namespace PatternRelationship
{
constexpr uint8_t Interlock = 0;  // Shimmer fills gaps in anchor
constexpr uint8_t Free      = 1;  // Independent patterns
constexpr uint8_t Shadow    = 2;  // Shimmer echoes anchor
} // namespace PatternRelationship

/**
 * PatternSkeleton - 32-step pattern with 4-bit intensity per step.
 * 
 * Step intensities are packed: 2 steps per byte (high nibble = even step, low nibble = odd step)
 * This gives 16 bytes per skeleton (32 steps × 4 bits = 128 bits).
 * 
 * Intensity values (0-15):
 *   0     = Step off (never fires)
 *   1-4   = Ghost note (fires at high density only)
 *   5-10  = Normal hit (fires at medium density)
 *   11-15 = Strong hit (fires at low density, accent candidate)
 */
struct PatternSkeleton
{
    // 32-step skeleton for anchor voice
    // Packed: 2 steps per byte, high nibble = even step, low nibble = odd step
    uint8_t anchorIntensity[16];

    // 32-step skeleton for shimmer voice
    uint8_t shimmerIntensity[16];

    // 32-bit mask indicating which steps can receive accents
    // Bit N = 1 means step N is accent-eligible
    uint32_t accentMask;

    // Default voice relationship mode (PatternRelationship values)
    uint8_t relationship;

    // Which genres this pattern suits (GenreAffinity bitfield)
    uint8_t genreAffinity;

    // Reserved for alignment/future use
    uint8_t reserved[2];
};

// Verify struct size for memory efficiency (32 + 4 + 1 + 1 + 2 = 40 bytes)
static_assert(sizeof(PatternSkeleton) == 40, "PatternSkeleton size changed unexpectedly");

/**
 * Get step intensity (0-15) from packed skeleton data.
 * 
 * @param skeleton Pointer to 16-byte packed intensity array
 * @param step Step index (0-31)
 * @return Intensity value (0-15)
 */
inline uint8_t GetStepIntensity(const uint8_t* skeleton, int step)
{
    if(step < 0 || step >= kPatternSteps)
        return 0;

    int byteIndex = step / 2;
    bool isOddStep = (step & 1) != 0;

    uint8_t packed = skeleton[byteIndex];
    return isOddStep ? (packed & 0x0F) : (packed >> 4);
}

/**
 * Set step intensity (0-15) in packed skeleton data.
 * 
 * @param skeleton Pointer to 16-byte packed intensity array
 * @param step Step index (0-31)
 * @param intensity Intensity value (0-15, clamped)
 */
inline void SetStepIntensity(uint8_t* skeleton, int step, uint8_t intensity)
{
    if(step < 0 || step >= kPatternSteps)
        return;

    // Clamp to 4-bit range
    if(intensity > 15)
        intensity = 15;

    int byteIndex = step / 2;
    bool isOddStep = (step & 1) != 0;

    if(isOddStep)
    {
        // Low nibble
        skeleton[byteIndex] = (skeleton[byteIndex] & 0xF0) | intensity;
    }
    else
    {
        // High nibble
        skeleton[byteIndex] = (skeleton[byteIndex] & 0x0F) | (intensity << 4);
    }
}

/**
 * Check if a step should fire based on density threshold.
 * 
 * @param skeleton Pointer to 16-byte packed intensity array
 * @param step Step index (0-31)
 * @param density Density parameter (0.0 - 1.0)
 * @return True if step should fire
 */
inline bool ShouldStepFire(const uint8_t* skeleton, int step, float density)
{
    uint8_t intensity = GetStepIntensity(skeleton, step);
    if(intensity == 0)
        return false;

    // Convert density (0-1) to threshold (15-0)
    // High density = low threshold = more steps fire
    // Low density = high threshold = fewer steps fire
    int threshold = static_cast<int>((1.0f - density) * 15.0f);
    if(threshold < 0)
        threshold = 0;
    if(threshold > 15)
        threshold = 15;

    return intensity > threshold;
}

/**
 * Check if a step is accent-eligible.
 * 
 * @param accentMask 32-bit accent eligibility mask
 * @param step Step index (0-31)
 * @return True if step can receive accent
 */
inline bool IsAccentEligible(uint32_t accentMask, int step)
{
    if(step < 0 || step >= kPatternSteps)
        return false;
    return (accentMask & (1u << step)) != 0;
}

/**
 * Check if a pattern suits a given terrain (genre).
 * 
 * @param pattern Pattern to check
 * @param terrain Terrain parameter (0.0 - 1.0)
 * @return True if pattern is suitable for the terrain's genre
 */
inline bool PatternSuitsGenre(const PatternSkeleton& pattern, float terrain)
{
    uint8_t genreBit;
    if(terrain < 0.25f)
        genreBit = GenreAffinity::Techno;
    else if(terrain < 0.50f)
        genreBit = GenreAffinity::Tribal;
    else if(terrain < 0.75f)
        genreBit = GenreAffinity::TripHop;
    else
        genreBit = GenreAffinity::IDM;

    return (pattern.genreAffinity & genreBit) != 0;
}

/**
 * Get velocity scaling based on step intensity.
 * Higher intensity steps get higher base velocity.
 * 
 * @param intensity Step intensity (0-15)
 * @return Base velocity (0.3 - 1.0)
 */
inline float IntensityToVelocity(uint8_t intensity)
{
    if(intensity == 0)
        return 0.0f;

    // Map 1-15 to 0.3-1.0
    // Ghost notes (1-4): 0.3-0.5
    // Normal (5-10): 0.5-0.75
    // Strong (11-15): 0.75-1.0
    return 0.3f + (static_cast<float>(intensity) / 15.0f) * 0.7f;
}

/**
 * Classify step intensity level.
 */
enum class IntensityLevel : uint8_t
{
    Off    = 0,  // intensity = 0
    Ghost  = 1,  // intensity = 1-4
    Normal = 2,  // intensity = 5-10
    Strong = 3   // intensity = 11-15
};

/**
 * Get intensity level classification for a step.
 */
inline IntensityLevel GetIntensityLevel(uint8_t intensity)
{
    if(intensity == 0)
        return IntensityLevel::Off;
    if(intensity <= 4)
        return IntensityLevel::Ghost;
    if(intensity <= 10)
        return IntensityLevel::Normal;
    return IntensityLevel::Strong;
}

} // namespace daisysp_idm_grids

