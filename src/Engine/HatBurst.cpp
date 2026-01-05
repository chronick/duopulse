#include "HatBurst.h"
#include "HashUtils.h"
#include "DuoPulseTypes.h"

#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// SHAPE Zone Boundaries for Hat Burst
// =============================================================================

constexpr float kHatShapeEvenEnd = 0.30f;     // End of even spacing zone
constexpr float kHatShapeEuclidEnd = 0.70f;   // End of euclidean zone

// Hash magic numbers for different randomness sources (avoid correlation)
constexpr uint32_t kHatJitterMagic = 0x48415431;  // "HAT1"
constexpr uint32_t kHatRandomMagic = 0x48415432;  // "HAT2"
constexpr uint32_t kHatVelocityMagic = 0x48415433;  // "HAT3"

// =============================================================================
// Collision Detection Implementation
// =============================================================================

int FindNearestEmpty(int step, int fillDuration, uint32_t usedSteps)
{
    // Clamp fillDuration to valid range
    if (fillDuration <= 0)
        return -1;
    if (fillDuration > 32)
        fillDuration = 32;

    // Check if the target step itself is empty
    step = step % fillDuration;  // Wrap to valid range
    if (step < 0)
        step += fillDuration;

    if (!(usedSteps & (1U << step)))
        return step;

    // Search alternately left and right for nearest empty
    for (int offset = 1; offset < fillDuration; ++offset)
    {
        int left = (step - offset + fillDuration) % fillDuration;
        int right = (step + offset) % fillDuration;

        // Check left first
        if (!(usedSteps & (1U << left)))
            return left;

        // Then check right
        if (!(usedSteps & (1U << right)))
            return right;
    }

    // No empty position found (all steps used)
    return -1;
}

// =============================================================================
// Proximity Detection Implementation
// =============================================================================

bool CheckProximity(int step, int fillStart, uint32_t mainPattern, int proximityWindow)
{
    // Convert fill-relative step to pattern-relative step
    int patternStep = (fillStart + step) % kMaxSteps;

    // Check the step itself and neighbors within window
    for (int offset = -proximityWindow; offset <= proximityWindow; ++offset)
    {
        int checkStep = (patternStep + offset + kMaxSteps) % kMaxSteps;
        if (mainPattern & (1U << checkStep))
            return true;
    }

    return false;
}

// =============================================================================
// Timing Distribution Implementation
// =============================================================================

int EuclideanWithJitter(int triggerIndex, int triggerCount,
                        int fillDuration, float shape, uint32_t seed)
{
    // Prevent division by zero
    if (triggerCount <= 0)
        return 0;
    if (fillDuration <= 0)
        return 0;

    // Base euclidean position: evenly distribute triggers across fill
    // For 4 triggers in 8 steps: positions 0, 2, 4, 6
    int basePos = (triggerIndex * fillDuration) / triggerCount;

    // Add jitter based on seed and shape
    // Jitter increases as shape moves away from 0.5 (center of euclidean zone)
    // At shape=0.30: minimal jitter (just entering euclidean zone)
    // At shape=0.50: moderate jitter
    // At shape=0.70: maximum jitter (approaching random zone)
    float normalizedShape = (shape - kHatShapeEvenEnd) / (kHatShapeEuclidEnd - kHatShapeEvenEnd);
    normalizedShape = std::max(0.0f, std::min(1.0f, normalizedShape));

    // Use hash for deterministic jitter value in range [-0.5, +0.5]
    float jitter = HashToFloat(seed ^ kHatJitterMagic, triggerIndex) - 0.5f;

    // Scale jitter by shape and fillDuration
    // Maximum jitter of about 1-2 steps at high shape values
    int jitterAmount = static_cast<int>(jitter * normalizedShape * 2.5f);

    // Apply jitter with wrap-around
    int finalPos = (basePos + jitterAmount + fillDuration) % fillDuration;
    if (finalPos < 0)
        finalPos += fillDuration;

    return finalPos;
}

// =============================================================================
// Main Generation Function Implementation
// =============================================================================

void GenerateHatBurst(float energy, float shape,
                      uint32_t mainPattern, int fillStart,
                      int fillDuration, uint32_t seed,
                      HatBurst& burst)
{
    // Initialize burst
    burst.Init();

    // Clamp parameters first, before storing
    energy = std::max(0.0f, std::min(1.0f, energy));
    shape = std::max(0.0f, std::min(1.0f, shape));
    fillDuration = std::max(1, std::min(32, fillDuration));

    burst.fillStart = static_cast<uint8_t>(fillStart);
    burst.fillDuration = static_cast<uint8_t>(fillDuration);

    // Calculate trigger count: 2 + floor(energy * 10) = [2, 12]
    int triggerCount = kMinHatBurstTriggers + static_cast<int>(energy * 10.0f);
    triggerCount = std::min(triggerCount, kMaxHatBurstTriggers);
    triggerCount = std::min(triggerCount, fillDuration);  // Can't have more triggers than steps

    // Track used steps to avoid collisions
    uint32_t usedSteps = 0;

    // Calculate base velocity: 0.65 + 0.35 * energy
    float baseVelocity = kBaseVelocityMin + kBaseVelocityBonus * energy;

    // Generate triggers based on SHAPE zone
    for (int i = 0; i < triggerCount; ++i)
    {
        int targetStep = 0;

        if (shape < kHatShapeEvenEnd)
        {
            // Zone 1: Even spacing (straight divisions)
            // Distribute evenly across fill zone
            targetStep = (i * fillDuration) / triggerCount;
        }
        else if (shape < kHatShapeEuclidEnd)
        {
            // Zone 2: Euclidean with jitter
            targetStep = EuclideanWithJitter(i, triggerCount, fillDuration, shape, seed);
        }
        else
        {
            // Zone 3: Random steps
            // Use hash to pick a random position
            float randVal = HashToFloat(seed ^ kHatRandomMagic, i);
            targetStep = static_cast<int>(randVal * fillDuration) % fillDuration;
        }

        // Handle collision: find nearest empty step
        int finalStep = FindNearestEmpty(targetStep, fillDuration, usedSteps);

        // If no empty step available, skip this trigger
        if (finalStep < 0)
            continue;

        // Mark step as used
        usedSteps |= (1U << finalStep);

        // Calculate velocity with ducking
        float velocity = baseVelocity;

        // Check proximity to main pattern (1 step window)
        if (CheckProximity(finalStep, fillStart, mainPattern, 1))
        {
            velocity *= kVelocityDuckMultiplier;
        }

        // Add slight velocity variation based on position
        float velVariation = HashToFloat(seed ^ kHatVelocityMagic, finalStep);
        velocity *= 0.9f + 0.1f * velVariation;  // +-5% variation

        // Clamp velocity
        velocity = std::max(0.0f, std::min(1.0f, velocity));

        // Store trigger
        burst.triggers[burst.count].step = static_cast<uint8_t>(finalStep);
        burst.triggers[burst.count].velocity = velocity;
        burst.count++;
    }
}

} // namespace daisysp_idm_grids
