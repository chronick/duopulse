#include "PatternField.h"
#include "HashUtils.h"
#include <cmath>
#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Shape-Based Pattern Generation (V5)
// =============================================================================

void GenerateStablePattern(float energy, int patternLength, float* outWeights)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    patternLength = std::max(1, std::min(static_cast<int>(kMaxSteps), patternLength));

    // Base weight scaling by energy (0.3 at energy=0, 1.0 at energy=1)
    float baseScale = 0.3f + energy * 0.7f;

    for (int step = 0; step < patternLength; ++step)
    {
        float weight;

        // Determine weight based on metrical position (euclidean-style)
        // Steps 0, 16 = bar downbeats (strongest)
        // Steps 4, 8, 12, 20, 24, 28 = quarter notes
        // Steps 2, 6, 10, 14, 18, 22, 26, 30 = 8th notes
        // Odd steps = 16th notes (weakest)

        if (step == 0 || step == 16)
        {
            // Bar downbeats: always strong
            weight = 1.0f;
        }
        else if (step % 4 == 0)
        {
            // Quarter notes
            weight = 0.85f;
        }
        else if (step % 2 == 0)
        {
            // 8th notes
            weight = 0.5f;
        }
        else
        {
            // 16th notes (offbeats)
            weight = 0.25f;
        }

        // Scale by energy and clamp
        outWeights[step] = ClampWeight(weight * baseScale);
    }
}

void GenerateSyncopationPattern(float energy, uint32_t seed, int patternLength, float* outWeights)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    patternLength = std::max(1, std::min(static_cast<int>(kMaxSteps), patternLength));

    // Base weight scaling by energy
    float baseScale = 0.4f + energy * 0.6f;

    // Seed-based suppression factor for beat 1 (90-100% range - almost no suppression)
    float downbeatSuppression = 0.90f + HashToFloat(seed, 0) * 0.10f;  // 0.90 to 1.00

    for (int step = 0; step < patternLength; ++step)
    {
        float weight;

        // Check if this is a downbeat (beat 1 of a bar: steps 0, 16)
        bool isBarDownbeat = (step == 0 || step == 16);

        // Check if this is an anticipation position (step before downbeat)
        bool isAnticipation = (step == patternLength - 1) ||  // Before step 0 wrap
                              (step == 15) ||                   // Before step 16
                              (step == 3) || (step == 7) || (step == 11) ||  // Before quarter notes
                              (step == 19) || (step == 23) || (step == 27);

        // Check if this is a weak offbeat (upbeats, "ands")
        bool isWeakOffbeat = (step % 2 == 1);

        if (isBarDownbeat)
        {
            // Suppress beat 1 (P1 fix: 50-70% of normal)
            weight = downbeatSuppression;
        }
        else if (isAnticipation)
        {
            // Moderate weight for anticipation - allows ~40% selection
            // Targets syncopation metric 0.22-0.48
            float boost = 0.05f + HashToFloat(seed, step + 100) * 0.15f;
            weight = 0.40f + boost;  // 0.40-0.60 range
        }
        else if (isWeakOffbeat)
        {
            // Moderate weight for weak offbeats - allows ~35% selection
            float boost = 0.05f + HashToFloat(seed, step + 200) * 0.15f;
            weight = 0.35f + boost;  // 0.35-0.55 range
        }
        else if (step % 4 == 0)
        {
            // Non-bar-1 quarter notes: strong preference
            weight = 0.80f;
        }
        else
        {
            // 8th note positions (even): moderately strong
            weight = 0.70f;
        }

        // Scale by energy and clamp
        outWeights[step] = ClampWeight(weight * baseScale);
    }
}

void GenerateWildPattern(float energy, uint32_t seed, int patternLength, float* outWeights)
{
    // Clamp inputs
    energy = std::max(0.0f, std::min(1.0f, energy));
    patternLength = std::max(1, std::min(static_cast<int>(kMaxSteps), patternLength));

    // Energy affects both base level and variation range
    float baseLevel = 0.3f + energy * 0.3f;   // 0.3-0.6 base
    float variation = 0.3f + energy * 0.4f;   // 0.3-0.7 variation range

    for (int step = 0; step < patternLength; ++step)
    {
        // Get deterministic random value for this step
        float randomValue = HashToFloat(seed, step);

        // Apply variation around base level
        float weight = baseLevel + (randomValue - 0.5f) * variation * 2.0f;

        // Add slight structural hint: downbeats more likely
        if (step == 0 || step == 16)
        {
            weight += 0.15f;  // Small downbeat bias
        }
        else if (step % 4 == 0)
        {
            weight += 0.08f;  // Smaller quarter note bias
        }

        // Clamp to valid range
        outWeights[step] = ClampWeight(weight);
    }
}

void ComputeShapeBlendedWeights(float shape, float energy,
                                 uint32_t seed, int patternLength,
                                 float* outWeights,
                                 const PatternFieldConfig& config)
{
    // Clamp inputs
    shape = std::max(0.0f, std::min(1.0f, shape));
    energy = std::max(0.0f, std::min(1.0f, energy));
    patternLength = std::max(1, std::min(static_cast<int>(kMaxSteps), patternLength));

    // Temporary buffers for pattern generators (on stack, RT-safe)
    float stableWeights[kMaxSteps];
    float syncopationWeights[kMaxSteps];
    float wildWeights[kMaxSteps];

    // Determine which zone we're in and compute accordingly
    // This avoids generating all three patterns when not needed

    if (shape < config.shapeZone1End)
    {
        // Zone 1 pure: Stable with humanization
        GenerateStablePattern(energy, patternLength, outWeights);

        // Add humanization that decreases as shape approaches zone boundary
        // humanize = 0.05 * (1 - (shape / 0.28))
        float humanize = 0.05f * (1.0f - (shape / config.shapeZone1End));

        for (int step = 0; step < patternLength; ++step)
        {
            // Add seed-based jitter scaled by humanization factor
            float jitter = (HashToFloat(seed, step + 300) - 0.5f) * humanize * 2.0f;
            outWeights[step] = ClampWeight(outWeights[step] + jitter);
        }
    }
    else if (shape < config.shapeCrossfade1End)
    {
        // Crossfade Zone 1->2: Blend stable to syncopation
        GenerateStablePattern(energy, patternLength, stableWeights);
        GenerateSyncopationPattern(energy, seed, patternLength, syncopationWeights);

        // Compute blend factor (0.0 at zone1End, 1.0 at crossfade1End)
        float t = (shape - config.shapeZone1End) / (config.shapeCrossfade1End - config.shapeZone1End);

        for (int step = 0; step < patternLength; ++step)
        {
            outWeights[step] = ClampWeight(LerpWeight(stableWeights[step], syncopationWeights[step], t));
        }
    }
    else if (shape < config.shapeZone2aEnd)
    {
        // Zone 2a: Pure syncopation (lower)
        GenerateSyncopationPattern(energy, seed, patternLength, outWeights);
    }
    else if (shape < config.shapeCrossfade2End)
    {
        // Crossfade Zone 2a->2b: Mid syncopation transition
        // This is subtle - we vary the syncopation character slightly
        GenerateSyncopationPattern(energy, seed, patternLength, syncopationWeights);

        // Use a slightly different seed variation for the "upper" syncopation
        GenerateSyncopationPattern(energy, seed + 0x12345678, patternLength, stableWeights);

        float t = (shape - config.shapeZone2aEnd) / (config.shapeCrossfade2End - config.shapeZone2aEnd);

        for (int step = 0; step < patternLength; ++step)
        {
            outWeights[step] = ClampWeight(LerpWeight(syncopationWeights[step], stableWeights[step], t));
        }
    }
    else if (shape < config.shapeZone2bEnd)
    {
        // Zone 2b: Pure syncopation (upper) - uses offset seed
        GenerateSyncopationPattern(energy, seed + 0x12345678, patternLength, outWeights);
    }
    else if (shape < config.shapeCrossfade3End)
    {
        // Crossfade Zone 2->3: Blend syncopation to wild
        GenerateSyncopationPattern(energy, seed + 0x12345678, patternLength, syncopationWeights);
        GenerateWildPattern(energy, seed, patternLength, wildWeights);

        float t = (shape - config.shapeZone2bEnd) / (config.shapeCrossfade3End - config.shapeZone2bEnd);

        for (int step = 0; step < patternLength; ++step)
        {
            outWeights[step] = ClampWeight(LerpWeight(syncopationWeights[step], wildWeights[step], t));
        }
    }
    else
    {
        // Zone 3 pure: Wild with chaos injection
        GenerateWildPattern(energy, seed, patternLength, outWeights);

        // Add chaos injection that increases as shape approaches 100%
        // Chaos factor: 0 at 0.72, up to 0.15 at 1.0
        float chaosFactor = ((shape - config.shapeCrossfade3End) / (1.0f - config.shapeCrossfade3End)) * 0.15f;

        for (int step = 0; step < patternLength; ++step)
        {
            // Add seed-based chaos variation
            float chaos = (HashToFloat(seed, step + 500) - 0.5f) * chaosFactor * 2.0f;
            outWeights[step] = ClampWeight(outWeights[step] + chaos);
        }
    }
}

// =============================================================================
// AXIS X/Y Bidirectional Biasing (Task 29)
// =============================================================================

/**
 * Metric weight table for 16-step pattern (V5 Task 35).
 * Based on musical importance in 4/4 time signature.
 */
static constexpr float kMetricWeights16[16] = {
    1.00f,  // Step 0:  Beat 1 (strongest downbeat)
    0.25f,  // Step 1:  16th note
    0.50f,  // Step 2:  8th note
    0.25f,  // Step 3:  16th note
    0.80f,  // Step 4:  Beat 2
    0.25f,  // Step 5:  16th note
    0.50f,  // Step 6:  8th note
    0.25f,  // Step 7:  16th note
    0.90f,  // Step 8:  Beat 3 (half-bar, strong)
    0.25f,  // Step 9:  16th note
    0.50f,  // Step 10: 8th note
    0.25f,  // Step 11: 16th note
    0.80f,  // Step 12: Beat 4
    0.25f,  // Step 13: 16th note
    0.50f,  // Step 14: 8th note
    0.25f,  // Step 15: 16th note
};

float GetMetricWeight(int step, int patternLength)
{
    // Handle edge cases
    if (patternLength <= 0)
    {
        return 0.5f;  // Default neutral weight
    }

    // Clamp step to valid range
    if (step < 0)
    {
        step = 0;
    }

    // V5 Task 35: For 16-step pattern, use explicit musical hierarchy table
    // This provides differentiated beat weights (beat 1 > beat 3 > beat 2/4)
    if (patternLength == 16)
    {
        return kMetricWeights16[step & 15];
    }

    // For other pattern lengths, scale proportionally to 16-step table
    // This ensures consistent musical feel across pattern lengths
    if (patternLength > 0 && patternLength != 16)
    {
        float normalized = static_cast<float>(step % patternLength) / static_cast<float>(patternLength);
        int mappedStep = static_cast<int>(normalized * 16.0f) & 15;
        return kMetricWeights16[mappedStep];
    }

    // Fallback: shouldn't reach here
    return 0.5f;
}

float GetPositionStrength(int step, int patternLength)
{
    float metricWeight = GetMetricWeight(step, patternLength);
    // Convert [0,1] metric weight to [-1,+1] position strength
    // Strong downbeat (metric=1.0) -> position=-1.0
    // Weak offbeat (metric=0.25) -> position=+0.5
    return 1.0f - 2.0f * metricWeight;
}

void ApplyAxisBias(float* baseWeights, float axisX, float axisY,
                   float shape, uint32_t seed, int patternLength)
{
    // Clamp inputs
    axisX = std::max(0.0f, std::min(1.0f, axisX));
    axisY = std::max(0.0f, std::min(1.0f, axisY));
    shape = std::max(0.0f, std::min(1.0f, shape));
    patternLength = std::max(1, std::min(static_cast<int>(kMaxSteps), patternLength));

    // Convert unipolar (0-1) to bipolar (-1 to +1)
    float xBias = (axisX - 0.5f) * 2.0f;  // [-1.0, +1.0]
    float yBias = (axisY - 0.5f) * 2.0f;  // [-1.0, +1.0]

    // Check for "broken mode" emergent feature
    // Activates when SHAPE > 0.6 AND AXIS X > 0.7
    bool brokenModeActive = (shape > 0.6f) && (axisX > 0.7f);
    float brokenIntensity = 0.0f;
    if (brokenModeActive)
    {
        // Calculate intensity: (shape - 0.6) * 2.5 * (axisX - 0.7) * 3.33
        brokenIntensity = (shape - 0.6f) * 2.5f * (axisX - 0.7f) * 3.33f;
        brokenIntensity = std::min(1.0f, brokenIntensity);
    }

    for (int step = 0; step < patternLength; ++step)
    {
        float weight = baseWeights[step];
        float metricWeight = GetMetricWeight(step, patternLength);
        float positionStrength = GetPositionStrength(step, patternLength);

        // Apply AXIS X bias (beat position emphasis)
        if (xBias > 0.0f)
        {
            // Toward offbeats (floating): suppress downbeats, boost offbeats
            if (positionStrength < 0.0f)
            {
                // Strong position (downbeat): suppress by up to 45%
                weight *= 1.0f - (0.45f * xBias * (-positionStrength));
            }
            else
            {
                // Weak position (offbeat): boost by up to 60%
                weight *= 1.0f + (0.60f * xBias * positionStrength);
            }
        }
        else if (xBias < 0.0f)
        {
            // Toward downbeats (grounded): boost downbeats, suppress offbeats
            float absXBias = -xBias;
            if (positionStrength < 0.0f)
            {
                // Strong position (downbeat): boost by up to 60%
                weight *= 1.0f + (0.60f * absXBias * (-positionStrength));
            }
            else
            {
                // Weak position (offbeat): suppress by up to 45%
                weight *= 1.0f - (0.45f * absXBias * positionStrength);
            }
        }

        // Apply AXIS Y bias (intricacy/complexity)
        // Uses metric weight directly (1.0 = strong, 0.25 = weak)
        if (yBias > 0.0f)
        {
            // Toward complex: boost weak positions by up to 50%
            // Weak positions have low metric weight
            float weakness = 1.0f - metricWeight;  // 0 for strong, 0.75 for weak
            weight *= 1.0f + (0.50f * yBias * weakness);
        }
        else if (yBias < 0.0f)
        {
            // Toward simple: suppress weak positions by up to 50%
            float absYBias = -yBias;
            float weakness = 1.0f - metricWeight;
            weight *= 1.0f - (0.50f * absYBias * weakness);
        }

        // Apply "broken mode" if active
        if (brokenModeActive && metricWeight >= 0.75f)
        {
            // This is a downbeat position - chance to suppress
            // Use deterministic hash for 60% chance per step
            float randomValue = HashToFloat(seed ^ 0xDEADBEEF, step);
            if (randomValue < 0.6f)
            {
                // Suppress to 25% * brokenIntensity + original * (1 - brokenIntensity)
                float suppressedWeight = weight * 0.25f;
                weight = LerpWeight(weight, suppressedWeight, brokenIntensity);
            }
        }

        // Clamp to valid range with minimum floor
        baseWeights[step] = ClampWeight(weight);
    }
}

} // namespace daisysp_idm_grids
