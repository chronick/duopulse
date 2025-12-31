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
    // Task 21 Phase B: Widened velocity contrast ranges

    // Accent probability: 20% to 50% (was 15%-50%)
    params.accentProbability = 0.20f + punch * 0.30f;

    // Velocity floor: 65% down to 30% (was 70%-30%)
    // Low punch = high floor (flat), high punch = low floor (dynamics)
    params.velocityFloor = 0.65f - punch * 0.35f;

    // Accent boost: +15% to +45% (was +10%-35%)
    params.accentBoost = 0.15f + punch * 0.30f;

    // Velocity variation: ±3% to ±15% (was ±5%-20%)
    params.velocityVariation = 0.03f + punch * 0.12f;
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

    // Task 21 Phase D: 3-phase BUILD system
    // GROOVE (0-60%): Stable
    // BUILD (60-87.5%): Ramping density and velocity
    // FILL (87.5-100%): Maximum energy

    if (phraseProgress < 0.60f)
    {
        // GROOVE phase: stable, no modification
        modifiers.phase = BuildPhase::GROOVE;
        modifiers.densityMultiplier = 1.0f;
        modifiers.velocityBoost = 0.0f;
        modifiers.forceAccents = false;
    }
    else if (phraseProgress < 0.875f)
    {
        // BUILD phase: ramping density and velocity
        modifiers.phase = BuildPhase::BUILD;
        float phaseProgress = (phraseProgress - 0.60f) / 0.275f;  // 0-1 within phase
        modifiers.densityMultiplier = 1.0f + build * 0.35f * phaseProgress;
        modifiers.velocityBoost = build * 0.08f * phaseProgress;
        modifiers.forceAccents = false;
    }
    else
    {
        // FILL phase: maximum energy
        modifiers.phase = BuildPhase::FILL;
        modifiers.densityMultiplier = 1.0f + build * 0.50f;
        modifiers.velocityBoost = build * 0.12f;
        modifiers.forceAccents = (build > 0.6f);
    }

    modifiers.inFillZone = (modifiers.phase == BuildPhase::FILL);
    modifiers.fillIntensity = modifiers.inFillZone ? build : 0.0f;
}

// =============================================================================
// Velocity Computation
// =============================================================================

bool ShouldAccent(int step,
                  uint32_t accentMask,
                  float accentProbability,
                  const BuildModifiers& buildMods,
                  uint32_t seed)
{
    // Task 21 Phase D: Force all hits to accent in FILL phase at high BUILD
    if (buildMods.forceAccents)
        return true;

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

    // Task 21 Phase D: Apply BUILD velocityBoost to floor
    velocity += buildMods.velocityBoost;

    // Add accent boost if accented
    if (isAccent)
    {
        velocity += punchParams.accentBoost;
    }

    // Apply BUILD modifiers (legacy fill boost, now redundant with velocityBoost)
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

    // Clamp to valid range
    // Task 21 Phase B: min raised to 0.30 for VCA audibility (was 0.2)
    return Clamp(velocity, 0.30f, 1.0f);
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

    // Determine accent status (Task 21 Phase D: pass buildMods for forceAccents)
    bool isAccent = ShouldAccent(step, accentMask, punchParams.accentProbability, buildMods, seed);

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

    // Determine accent status (Task 21 Phase D: pass buildMods for forceAccents)
    bool isAccent = ShouldAccent(step, accentMask, punchParams.accentProbability, buildMods, seed);

    // Compute final velocity
    return ComputeVelocity(punchParams, buildMods, isAccent, seed, step);
}

} // namespace daisysp_idm_grids
