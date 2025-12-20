#include "VelocityCompute.h"
#include "PulseField.h"  // For HashStep, HashToFloat, Clamp

namespace daisysp_idm_grids
{

// Magic numbers for velocity hash mixing (to avoid correlation with other effects)
constexpr uint32_t kVelAccentHashMagic    = 0x41434E54;  // "ACNT"
constexpr uint32_t kVelVariationHashMagic = 0x56415249;  // "VARI"

// Default accent masks
// Anchor: Emphasize downbeats and quarter notes
constexpr uint32_t kAnchorAccentMask = 0x11111111;  // Steps 0, 4, 8, 12, 16, 20, 24, 28

// Shimmer: Emphasize backbeats and some syncopated positions
constexpr uint32_t kShimmerAccentMask = 0x01010101;  // Steps 0, 8, 16, 24 (backbeats)

// AUX: Offbeat 8ths for hi-hat character
constexpr uint32_t kAuxAccentMask = 0x44444444;  // Steps 2, 6, 10, 14, 18, 22, 26, 30

// =============================================================================
// PUNCH Parameter Computation
// =============================================================================

void ComputePunch(float punch, PunchParams& params)
{
    // Clamp input
    punch = Clamp(punch, 0.0f, 1.0f);

    // PUNCH = 0%: Flat dynamics (all similar velocity)
    // PUNCH = 100%: Maximum dynamics (huge contrasts)

    // Accent probability: 15% to 50%
    params.accentProbability = 0.15f + punch * 0.35f;

    // Velocity floor: 70% down to 30%
    // Low punch = high floor (flat), high punch = low floor (dynamics)
    params.velocityFloor = 0.70f - punch * 0.40f;

    // Accent boost: +10% to +35%
    params.accentBoost = 0.10f + punch * 0.25f;

    // Velocity variation: ±5% to ±20%
    params.velocityVariation = 0.05f + punch * 0.15f;
}

// =============================================================================
// BUILD Parameter Computation
// =============================================================================

void ComputeBuildModifiers(float build, float phraseProgress, BuildModifiers& modifiers)
{
    // Clamp inputs
    build = Clamp(build, 0.0f, 1.0f);
    phraseProgress = Clamp(phraseProgress, 0.0f, 1.0f);

    modifiers.phraseProgress = phraseProgress;

    // BUILD = 0%: Flat throughout (no density change)
    // BUILD = 100%: Dramatic arc (density increases toward end)

    // Density ramps up toward phrase end
    float rampAmount = build * phraseProgress * 0.5f;  // Up to 50% denser at end
    modifiers.densityMultiplier = 1.0f + rampAmount;

    // Fill zone is last 12.5% of phrase (last bar of 8-bar phrase)
    modifiers.inFillZone = (phraseProgress > 0.875f);

    // Fill intensity increases with BUILD and proximity to phrase end
    if (modifiers.inFillZone)
    {
        float fillProgress = (phraseProgress - 0.875f) / 0.125f;  // 0-1 within fill zone
        modifiers.fillIntensity = build * fillProgress;
    }
    else
    {
        modifiers.fillIntensity = 0.0f;
    }
}

// =============================================================================
// Velocity Computation
// =============================================================================

bool ShouldAccent(int step,
                  uint32_t accentMask,
                  float accentProbability,
                  uint32_t seed)
{
    // Check if step is accent-eligible
    bool eligible = (accentMask & (1u << (step & 31))) != 0;

    if (!eligible)
        return false;

    // Apply probability to determine if accent actually fires
    uint32_t hash = HashStep(seed ^ kVelAccentHashMagic, step);
    float roll = HashToFloat(hash);

    return roll < accentProbability;
}

float ComputeVelocity(const PunchParams& punchParams,
                      const BuildModifiers& buildMods,
                      bool isAccent,
                      uint32_t seed,
                      int step)
{
    // Start with velocity floor
    float velocity = punchParams.velocityFloor;

    // Add accent boost if accented
    if (isAccent)
    {
        velocity += punchParams.accentBoost;
    }

    // Apply BUILD modifiers
    if (buildMods.inFillZone && buildMods.fillIntensity > 0.0f)
    {
        // In fill zone: boost velocity toward phrase end (fills get louder)
        // Max boost of 0.15 at full fill intensity
        velocity += buildMods.fillIntensity * 0.15f;
    }

    // Apply random variation
    if (punchParams.velocityVariation > 0.001f)
    {
        uint32_t varHash = HashStep(seed ^ kVelVariationHashMagic, step);
        float varRoll = HashToFloat(varHash);  // 0.0 to 1.0
        float variation = (varRoll - 0.5f) * 2.0f * punchParams.velocityVariation;
        velocity += variation;
    }

    // Clamp to valid range (min 0.2 for audibility, max 1.0)
    return Clamp(velocity, 0.2f, 1.0f);
}

uint32_t GetDefaultAccentMask(Voice voice)
{
    switch (voice)
    {
        case Voice::ANCHOR:
            return kAnchorAccentMask;
        case Voice::SHIMMER:
            return kShimmerAccentMask;
        case Voice::AUX:
            return kAuxAccentMask;
        default:
            return kAnchorAccentMask;
    }
}

float ComputeAnchorVelocity(float punch,
                            float build,
                            float phraseProgress,
                            int step,
                            uint32_t seed,
                            uint32_t accentMask)
{
    // Use default mask if none provided
    if (accentMask == 0)
        accentMask = GetDefaultAccentMask(Voice::ANCHOR);

    // Compute PUNCH parameters
    PunchParams punchParams;
    ComputePunch(punch, punchParams);

    // Compute BUILD modifiers
    BuildModifiers buildMods;
    ComputeBuildModifiers(build, phraseProgress, buildMods);

    // Determine accent status
    bool isAccent = ShouldAccent(step, accentMask, punchParams.accentProbability, seed);

    // Compute final velocity
    return ComputeVelocity(punchParams, buildMods, isAccent, seed, step);
}

float ComputeShimmerVelocity(float punch,
                             float build,
                             float phraseProgress,
                             int step,
                             uint32_t seed,
                             uint32_t accentMask)
{
    // Use default mask if none provided
    if (accentMask == 0)
        accentMask = GetDefaultAccentMask(Voice::SHIMMER);

    // Compute PUNCH parameters
    PunchParams punchParams;
    ComputePunch(punch, punchParams);

    // Compute BUILD modifiers
    BuildModifiers buildMods;
    ComputeBuildModifiers(build, phraseProgress, buildMods);

    // Determine accent status
    bool isAccent = ShouldAccent(step, accentMask, punchParams.accentProbability, seed);

    // Compute final velocity
    return ComputeVelocity(punchParams, buildMods, isAccent, seed, step);
}

} // namespace daisysp_idm_grids
