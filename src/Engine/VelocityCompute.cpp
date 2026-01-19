#include "VelocityCompute.h"
#include "PulseField.h"    // For HashStep, HashToFloat, Clamp
#include "PatternField.h"  // For GetMetricWeight

namespace daisysp_idm_grids
{

// Magic numbers for velocity hash mixing (to avoid correlation with other effects)
constexpr uint32_t kVelAccentHashMagic    = 0x41434E54;  // "ACNT"
constexpr uint32_t kVelVariationHashMagic = 0x56415249;  // "VARI"

// Default accent masks (kept for legacy ShouldAccent function)
// Anchor: Emphasize downbeats and quarter notes
constexpr uint64_t kAnchorAccentMask = 0x1111111111111111ULL;  // Steps 0, 4, 8, 12, 16, 20, 24, 28

// Shimmer: Emphasize backbeats and some syncopated positions
constexpr uint64_t kShimmerAccentMask = 0x0101010101010101ULL;  // Steps 0, 8, 16, 24 (backbeats)

// AUX: Offbeat 8ths for hi-hat character
constexpr uint64_t kAuxAccentMask = 0x4444444444444444ULL;  // Steps 2, 6, 10, 14, 18, 22, 26, 30

// =============================================================================
// ACCENT Parameter Computation (V5: was PUNCH)
// =============================================================================

void ComputeAccent(float accent, AccentParams& params)
{
    // Clamp input
    accent = Clamp(accent, 0.0f, 1.0f);

    // V5 (Task 35): Metric weight-based velocity
    // ACCENT = 0%: Flat dynamics (80-88% range)
    // ACCENT = 100%: Wide dynamics (30-100% range)
    params.velocityFloor   = 0.80f - accent * 0.50f;   // 80% -> 30%
    params.velocityCeiling = 0.88f + accent * 0.12f;   // 88% -> 100%
    params.variation       = 0.02f + accent * 0.05f;   // 2% -> 7%
}

// =============================================================================
// Accent Velocity Computation (V5 Task 35)
// =============================================================================

float ComputeAccentVelocity(float accent, int step, int patternLength, uint32_t seed)
{
    // Clamp accent
    accent = Clamp(accent, 0.0f, 1.0f);

    // Get metric weight (0.0 = weak offbeat, 1.0 = strong downbeat)
    float metricWeight = GetMetricWeight(step, patternLength);

    // Velocity range scales with ACCENT
    float velocityFloor   = 0.80f - accent * 0.50f;   // 80% -> 30%
    float velocityCeiling = 0.88f + accent * 0.12f;   // 88% -> 100%

    // Map metric weight to velocity
    float velocity = velocityFloor + metricWeight * (velocityCeiling - velocityFloor);

    // Micro-variation for human feel
    // HashToFloat returns 0-1, subtracting 0.5 gives +/-0.5 range
    // Actual variation: +/-(0.5 x variation) = +/-1% to +/-3.5%
    float variation = 0.02f + accent * 0.05f;
    uint32_t varHash = HashStep(seed ^ kVelVariationHashMagic, step);
    velocity += (HashToFloat(varHash) - 0.5f) * variation;

    // Clamp to valid range
    return Clamp(velocity, 0.30f, 1.0f);
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
                  uint64_t accentMask,
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
                      int step,
                      int patternLength)
{
    // V5 (Task 35): Metric weight-based velocity
    // Get metric weight for step position
    float metricWeight = GetMetricWeight(step, patternLength);

    // Map metric weight to velocity range
    float velocity = accentParams.velocityFloor +
                     metricWeight * (accentParams.velocityCeiling - accentParams.velocityFloor);

    // Apply SHAPE velocityBoost
    velocity += shapeMods.velocityBoost;

    // Apply SHAPE fill zone boost
    if (shapeMods.inFillZone && shapeMods.fillIntensity > 0.0f)
    {
        // In fill zone: boost velocity toward phrase end (fills get louder)
        // Max boost of 0.15 at full fill intensity
        velocity += shapeMods.fillIntensity * 0.15f;
    }

    // Apply micro-variation for human feel
    if (accentParams.variation > 0.001f)
    {
        uint32_t varHash = HashStep(seed ^ kVelVariationHashMagic, step);
        float varRoll = HashToFloat(varHash);  // 0.0 to 1.0
        velocity += (varRoll - 0.5f) * accentParams.variation;
    }

    // Clamp to valid range
    return Clamp(velocity, 0.30f, 1.0f);
}

uint64_t GetDefaultAccentMask(Voice voice)
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
                            int patternLength,
                            uint32_t seed,
                            uint64_t accentMask)
{
    // V5 (Task 35): Use metric weight-based velocity
    // Get base velocity from accent and metric weight
    float velocity = ComputeAccentVelocity(accent, step, patternLength, seed);

    // Compute SHAPE modifiers for phrase arc
    ShapeModifiers shapeMods;
    ComputeShapeModifiers(shape, phraseProgress, shapeMods);

    // Apply SHAPE velocityBoost
    velocity += shapeMods.velocityBoost;

    // Apply SHAPE fill zone boost
    if (shapeMods.inFillZone && shapeMods.fillIntensity > 0.0f)
    {
        velocity += shapeMods.fillIntensity * 0.15f;
    }

    // Clamp to valid range
    return Clamp(velocity, 0.30f, 1.0f);
}

float ComputeShimmerVelocity(float accent,
                             float shape,
                             float phraseProgress,
                             int step,
                             int patternLength,
                             uint32_t seed,
                             uint64_t accentMask)
{
    // V5 (Task 35): Use metric weight-based velocity
    // Get base velocity from accent and metric weight
    float velocity = ComputeAccentVelocity(accent, step, patternLength, seed);

    // Compute SHAPE modifiers for phrase arc
    ShapeModifiers shapeMods;
    ComputeShapeModifiers(shape, phraseProgress, shapeMods);

    // Apply SHAPE velocityBoost
    velocity += shapeMods.velocityBoost;

    // Apply SHAPE fill zone boost
    if (shapeMods.inFillZone && shapeMods.fillIntensity > 0.0f)
    {
        velocity += shapeMods.fillIntensity * 0.15f;
    }

    // Clamp to valid range
    return Clamp(velocity, 0.30f, 1.0f);
}

} // namespace daisysp_idm_grids
