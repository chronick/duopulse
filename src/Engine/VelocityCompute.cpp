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
// ACCENT Parameter Computation (V5: was PUNCH)
// =============================================================================

void ComputeAccent(float accent, AccentParams& params)
{
    // Clamp input
    accent = Clamp(accent, 0.0f, 1.0f);

    // ACCENT = 0%: Flat dynamics (all similar velocity)
    // ACCENT = 100%: Maximum dynamics (huge contrasts)
    // Task 21 Phase B: Widened velocity contrast ranges

    // Accent probability: 20% to 50% (was 15%-50%)
    params.accentProbability = 0.20f + accent * 0.30f;

    // Velocity floor: 65% down to 30% (was 70%-30%)
    // Low accent = high floor (flat), high accent = low floor (dynamics)
    params.velocityFloor = 0.65f - accent * 0.35f;

    // Accent boost: +15% to +45% (was +10%-35%)
    params.accentBoost = 0.15f + accent * 0.30f;

    // Velocity variation: ±3% to ±15% (was ±5%-20%)
    params.velocityVariation = 0.03f + accent * 0.12f;
}

// =============================================================================
// SHAPE Parameter Computation (V5: was BUILD)
// =============================================================================

void ComputeShapeModifiers(float shape, float phraseProgress, ShapeModifiers& modifiers)
{
    // Clamp inputs
    shape = Clamp(shape, 0.0f, 1.0f);
    phraseProgress = Clamp(phraseProgress, 0.0f, 1.0f);

    modifiers.phraseProgress = phraseProgress;

    // Task 21 Phase D: 3-phase SHAPE system
    // GROOVE (0-60%): Stable
    // BUILD (60-87.5%): Ramping density and velocity
    // FILL (87.5-100%): Maximum energy

    if (phraseProgress < 0.60f)
    {
        // GROOVE phase: stable, no modification
        modifiers.phase = ShapePhase::GROOVE;
        modifiers.densityMultiplier = 1.0f;
        modifiers.velocityBoost = 0.0f;
        modifiers.forceAccents = false;
    }
    else if (phraseProgress < 0.875f)
    {
        // BUILD phase: ramping density and velocity
        modifiers.phase = ShapePhase::BUILD;
        float phaseProgress = (phraseProgress - 0.60f) / 0.275f;  // 0-1 within phase
        modifiers.densityMultiplier = 1.0f + shape * 0.35f * phaseProgress;
        modifiers.velocityBoost = shape * 0.15f * phaseProgress;  // Task 21-05: increased from 0.08
        modifiers.forceAccents = false;
    }
    else
    {
        // FILL phase: maximum energy
        modifiers.phase = ShapePhase::FILL;
        modifiers.densityMultiplier = 1.0f + shape * 0.50f;
        modifiers.velocityBoost = shape * 0.20f;  // Task 21-05: increased from 0.12
        modifiers.forceAccents = (shape > 0.6f);
    }

    modifiers.inFillZone = (modifiers.phase == ShapePhase::FILL);
    modifiers.fillIntensity = modifiers.inFillZone ? shape : 0.0f;
}

// =============================================================================
// Velocity Computation
// =============================================================================

bool ShouldAccent(int step,
                  uint32_t accentMask,
                  float accentProbability,
                  const ShapeModifiers& shapeMods,
                  uint32_t seed)
{
    // Task 21 Phase D: Force all hits to accent in FILL phase at high SHAPE
    if (shapeMods.forceAccents)
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

float ComputeVelocity(const AccentParams& accentParams,
                      const ShapeModifiers& shapeMods,
                      bool isAccent,
                      uint32_t seed,
                      int step)
{
    // Start with velocity floor
    float velocity = accentParams.velocityFloor;

    // Task 21 Phase D: Apply SHAPE velocityBoost to floor
    velocity += shapeMods.velocityBoost;

    // Add accent boost if accented
    if (isAccent)
    {
        velocity += accentParams.accentBoost;
    }

    // Apply SHAPE modifiers (legacy fill boost, now redundant with velocityBoost)
    if (shapeMods.inFillZone && shapeMods.fillIntensity > 0.0f)
    {
        // In fill zone: boost velocity toward phrase end (fills get louder)
        // Max boost of 0.15 at full fill intensity
        velocity += shapeMods.fillIntensity * 0.15f;
    }

    // Apply random variation
    if (accentParams.velocityVariation > 0.001f)
    {
        uint32_t varHash = HashStep(seed ^ kVelVariationHashMagic, step);
        float varRoll = HashToFloat(varHash);  // 0.0 to 1.0
        float variation = (varRoll - 0.5f) * 2.0f * accentParams.velocityVariation;
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

float ComputeAnchorVelocity(float accent,
                            float shape,
                            float phraseProgress,
                            int step,
                            uint32_t seed,
                            uint32_t accentMask)
{
    // Use default mask if none provided
    if (accentMask == 0)
        accentMask = GetDefaultAccentMask(Voice::ANCHOR);

    // Compute ACCENT parameters
    AccentParams accentParams;
    ComputeAccent(accent, accentParams);

    // Compute SHAPE modifiers
    ShapeModifiers shapeMods;
    ComputeShapeModifiers(shape, phraseProgress, shapeMods);

    // Determine accent status (Task 21 Phase D: pass shapeMods for forceAccents)
    bool isAccented = ShouldAccent(step, accentMask, accentParams.accentProbability, shapeMods, seed);

    // Compute final velocity
    return ComputeVelocity(accentParams, shapeMods, isAccented, seed, step);
}

float ComputeShimmerVelocity(float accent,
                             float shape,
                             float phraseProgress,
                             int step,
                             uint32_t seed,
                             uint32_t accentMask)
{
    // Use default mask if none provided
    if (accentMask == 0)
        accentMask = GetDefaultAccentMask(Voice::SHIMMER);

    // Compute ACCENT parameters
    AccentParams accentParams;
    ComputeAccent(accent, accentParams);

    // Compute SHAPE modifiers
    ShapeModifiers shapeMods;
    ComputeShapeModifiers(shape, phraseProgress, shapeMods);

    // Determine accent status (Task 21 Phase D: pass shapeMods for forceAccents)
    bool isAccented = ShouldAccent(step, accentMask, accentParams.accentProbability, shapeMods, seed);

    // Compute final velocity
    return ComputeVelocity(accentParams, shapeMods, isAccented, seed, step);
}

} // namespace daisysp_idm_grids
