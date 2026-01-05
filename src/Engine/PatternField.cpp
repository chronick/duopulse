#include "PatternField.h"
#include "ArchetypeData.h"
#include "HashUtils.h"
#include <cmath>
#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Module State
// =============================================================================

/// Genre fields storage (initialized once at startup)
static GenreField s_genreFields[static_cast<int>(Genre::COUNT)];

/// Initialization flag
static bool s_initialized = false;

// =============================================================================
// Softmax Implementation
// =============================================================================

void SoftmaxWithTemperature(float weights[4], float temperature)
{
    // Clamp temperature to avoid division by zero or overflow
    temperature = std::max(0.01f, std::min(temperature, 10.0f));

    // Find max for numerical stability
    float maxWeight = weights[0];
    for (int i = 1; i < 4; ++i)
    {
        if (weights[i] > maxWeight)
        {
            maxWeight = weights[i];
        }
    }

    // Compute exp((w - max) / temperature) for each weight
    float expSum = 0.0f;
    float expWeights[4];
    for (int i = 0; i < 4; ++i)
    {
        expWeights[i] = std::exp((weights[i] - maxWeight) / temperature);
        expSum += expWeights[i];
    }

    // Normalize
    if (expSum > kMinWeight)
    {
        for (int i = 0; i < 4; ++i)
        {
            weights[i] = expWeights[i] / expSum;
        }
    }
    else
    {
        // Fallback: equal weights
        for (int i = 0; i < 4; ++i)
        {
            weights[i] = 0.25f;
        }
    }
}

// =============================================================================
// Grid Weight Computation
// =============================================================================

void ComputeGridWeights(float fieldX, float fieldY,
                        float outWeights[4],
                        int& outGridX0, int& outGridX1,
                        int& outGridY0, int& outGridY1)
{
    // Clamp inputs to valid range
    fieldX = std::max(0.0f, std::min(1.0f, fieldX));
    fieldY = std::max(0.0f, std::min(1.0f, fieldY));

    // Map 0-1 to 0-2 grid coordinates
    float gridX = fieldX * 2.0f;
    float gridY = fieldY * 2.0f;

    // Find surrounding grid cells
    outGridX0 = static_cast<int>(std::floor(gridX));
    outGridY0 = static_cast<int>(std::floor(gridY));

    // Clamp to valid range (0-1 for lower indices, 1-2 for upper)
    outGridX0 = std::max(0, std::min(1, outGridX0));
    outGridY0 = std::max(0, std::min(1, outGridY0));
    outGridX1 = outGridX0 + 1;
    outGridY1 = outGridY0 + 1;

    // Compute interpolation factors
    float fracX = gridX - static_cast<float>(outGridX0);
    float fracY = gridY - static_cast<float>(outGridY0);

    // Clamp fractions (handles edge case when gridX or gridY = 2.0)
    fracX = std::max(0.0f, std::min(1.0f, fracX));
    fracY = std::max(0.0f, std::min(1.0f, fracY));

    // Bilinear interpolation weights
    // Order: [bottomLeft, bottomRight, topLeft, topRight]
    outWeights[0] = (1.0f - fracX) * (1.0f - fracY);  // Bottom-left
    outWeights[1] = fracX * (1.0f - fracY);           // Bottom-right
    outWeights[2] = (1.0f - fracX) * fracY;           // Top-left
    outWeights[3] = fracX * fracY;                    // Top-right
}

// =============================================================================
// Utility Functions
// =============================================================================

int FindDominantArchetype(const float weights[4])
{
    int dominant = 0;
    float maxWeight = weights[0];

    for (int i = 1; i < 4; ++i)
    {
        if (weights[i] > maxWeight)
        {
            maxWeight = weights[i];
            dominant = i;
        }
    }

    return dominant;
}

float InterpolateFloat(const float values[4], const float weights[4])
{
    float result = 0.0f;
    for (int i = 0; i < 4; ++i)
    {
        result += values[i] * weights[i];
    }
    return result;
}

float InterpolateStepWeight(const ArchetypeDNA* archetypes[4],
                            const float weights[4],
                            int stepIndex,
                            int voice)
{
    float values[4];

    for (int i = 0; i < 4; ++i)
    {
        switch (voice)
        {
            case 0:  // Anchor
                values[i] = archetypes[i]->anchorWeights[stepIndex];
                break;
            case 1:  // Shimmer
                values[i] = archetypes[i]->shimmerWeights[stepIndex];
                break;
            case 2:  // Aux
                values[i] = archetypes[i]->auxWeights[stepIndex];
                break;
            default:
                values[i] = 0.0f;
                break;
        }
    }

    return InterpolateFloat(values, weights);
}

// =============================================================================
// Archetype Blending
// =============================================================================

void BlendArchetypes(const ArchetypeDNA* archetypes[4],
                     const float weights[4],
                     ArchetypeDNA& outArchetype)
{
    // Find dominant archetype for discrete properties
    int dominant = FindDominantArchetype(weights);
    const ArchetypeDNA* dominantArch = archetypes[dominant];

    // Interpolate step weights (continuous)
    for (int step = 0; step < kMaxSteps; ++step)
    {
        outArchetype.anchorWeights[step] = InterpolateStepWeight(
            archetypes, weights, step, 0);
        outArchetype.shimmerWeights[step] = InterpolateStepWeight(
            archetypes, weights, step, 1);
        outArchetype.auxWeights[step] = InterpolateStepWeight(
            archetypes, weights, step, 2);
    }

    // Discrete properties: use dominant archetype's values
    // (These define structural behavior that shouldn't be interpolated)
    outArchetype.anchorAccentMask = dominantArch->anchorAccentMask;
    outArchetype.shimmerAccentMask = dominantArch->shimmerAccentMask;
    outArchetype.ratchetEligibleMask = dominantArch->ratchetEligibleMask;

    // Interpolate timing characteristics (continuous)
    float swingAmounts[4] = {
        archetypes[0]->swingAmount,
        archetypes[1]->swingAmount,
        archetypes[2]->swingAmount,
        archetypes[3]->swingAmount
    };
    outArchetype.swingAmount = InterpolateFloat(swingAmounts, weights);

    float swingPatterns[4] = {
        archetypes[0]->swingPattern,
        archetypes[1]->swingPattern,
        archetypes[2]->swingPattern,
        archetypes[3]->swingPattern
    };
    outArchetype.swingPattern = InterpolateFloat(swingPatterns, weights);

    // Interpolate voice relationship (continuous)
    float couples[4] = {
        archetypes[0]->defaultCouple,
        archetypes[1]->defaultCouple,
        archetypes[2]->defaultCouple,
        archetypes[3]->defaultCouple
    };
    outArchetype.defaultCouple = InterpolateFloat(couples, weights);

    // Interpolate fill behavior (continuous)
    float fillMultipliers[4] = {
        archetypes[0]->fillDensityMultiplier,
        archetypes[1]->fillDensityMultiplier,
        archetypes[2]->fillDensityMultiplier,
        archetypes[3]->fillDensityMultiplier
    };
    outArchetype.fillDensityMultiplier = InterpolateFloat(fillMultipliers, weights);

    // Grid position from dominant (for reference)
    outArchetype.gridX = dominantArch->gridX;
    outArchetype.gridY = dominantArch->gridY;
}

// =============================================================================
// Main Entry Point
// =============================================================================

void GetBlendedArchetype(const GenreField& field,
                         float fieldX, float fieldY,
                         float temperature,
                         ArchetypeDNA& outArchetype)
{
    // Compute grid positions and bilinear weights
    float weights[4];
    int gridX0, gridX1, gridY0, gridY1;
    ComputeGridWeights(fieldX, fieldY, weights, gridX0, gridX1, gridY0, gridY1);

    // Apply softmax with temperature for winner-take-more behavior
    SoftmaxWithTemperature(weights, temperature);

    // Get the four surrounding archetypes
    const ArchetypeDNA* archetypes[4] = {
        &field.GetArchetype(gridX0, gridY0),  // Bottom-left
        &field.GetArchetype(gridX1, gridY0),  // Bottom-right
        &field.GetArchetype(gridX0, gridY1),  // Top-left
        &field.GetArchetype(gridX1, gridY1)   // Top-right
    };

    // Blend archetypes
    BlendArchetypes(archetypes, weights, outArchetype);
}

// =============================================================================
// Genre Field Management
// =============================================================================

void InitializeGenreFields()
{
    if (s_initialized)
    {
        return;
    }

    // Initialize each genre's field with archetype data
    for (int genre = 0; genre < static_cast<int>(Genre::COUNT); ++genre)
    {
        GenreField& field = s_genreFields[genre];

        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                // Get archetype data from constexpr tables
                int archetypeIndex = y * 3 + x;
                ArchetypeDNA& archetype = field.archetypes[y][x];

                // Load from archetype data tables
                LoadArchetypeData(static_cast<Genre>(genre), archetypeIndex, archetype);

                // Set grid position
                archetype.gridX = static_cast<uint8_t>(x);
                archetype.gridY = static_cast<uint8_t>(y);
            }
        }
    }

    s_initialized = true;
}

const GenreField& GetGenreField(Genre genre)
{
    // Ensure fields are initialized
    if (!s_initialized)
    {
        InitializeGenreFields();
    }

    int index = static_cast<int>(genre);
    if (index < 0 || index >= static_cast<int>(Genre::COUNT))
    {
        index = 0;  // Default to Techno
    }

    return s_genreFields[index];
}

bool AreGenreFieldsInitialized()
{
    return s_initialized;
}

// =============================================================================
// Shape-Based Pattern Generation (Task 28)
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

    // Seed-based suppression factor for beat 1 (50-70% range per spec)
    float downbeatSuppression = 0.5f + HashToFloat(seed, 0) * 0.2f;  // 0.5 to 0.7

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
            // Boost anticipation positions
            float boost = 0.2f + HashToFloat(seed, step + 100) * 0.15f;
            weight = 0.7f + boost;  // 0.7-0.85 range
        }
        else if (isWeakOffbeat)
        {
            // Boost weak offbeats for funk feel
            float boost = 0.1f + HashToFloat(seed, step + 200) * 0.2f;
            weight = 0.5f + boost;  // 0.5-0.7 range
        }
        else if (step % 4 == 0)
        {
            // Non-bar-1 quarter notes: moderate
            weight = 0.6f;
        }
        else
        {
            // 8th note positions: light
            weight = 0.4f;
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
                                 float* outWeights)
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

    if (shape < kShapeZone1End)
    {
        // Zone 1 pure: Stable with humanization
        GenerateStablePattern(energy, patternLength, outWeights);

        // Add humanization that decreases as shape approaches zone boundary
        // humanize = 0.05 * (1 - (shape / 0.28))
        float humanize = 0.05f * (1.0f - (shape / kShapeZone1End));

        for (int step = 0; step < patternLength; ++step)
        {
            // Add seed-based jitter scaled by humanization factor
            float jitter = (HashToFloat(seed, step + 300) - 0.5f) * humanize * 2.0f;
            outWeights[step] = ClampWeight(outWeights[step] + jitter);
        }
    }
    else if (shape < kShapeCrossfade1End)
    {
        // Crossfade Zone 1->2: Blend stable to syncopation
        GenerateStablePattern(energy, patternLength, stableWeights);
        GenerateSyncopationPattern(energy, seed, patternLength, syncopationWeights);

        // Compute blend factor (0.0 at zone1End, 1.0 at crossfade1End)
        float t = (shape - kShapeZone1End) / (kShapeCrossfade1End - kShapeZone1End);

        for (int step = 0; step < patternLength; ++step)
        {
            outWeights[step] = ClampWeight(LerpWeight(stableWeights[step], syncopationWeights[step], t));
        }
    }
    else if (shape < kShapeZone2aEnd)
    {
        // Zone 2a: Pure syncopation (lower)
        GenerateSyncopationPattern(energy, seed, patternLength, outWeights);
    }
    else if (shape < kShapeCrossfade2End)
    {
        // Crossfade Zone 2a->2b: Mid syncopation transition
        // This is subtle - we vary the syncopation character slightly
        GenerateSyncopationPattern(energy, seed, patternLength, syncopationWeights);

        // Use a slightly different seed variation for the "upper" syncopation
        GenerateSyncopationPattern(energy, seed + 0x12345678, patternLength, stableWeights);

        float t = (shape - kShapeZone2aEnd) / (kShapeCrossfade2End - kShapeZone2aEnd);

        for (int step = 0; step < patternLength; ++step)
        {
            outWeights[step] = ClampWeight(LerpWeight(syncopationWeights[step], stableWeights[step], t));
        }
    }
    else if (shape < kShapeZone2bEnd)
    {
        // Zone 2b: Pure syncopation (upper) - uses offset seed
        GenerateSyncopationPattern(energy, seed + 0x12345678, patternLength, outWeights);
    }
    else if (shape < kShapeCrossfade3End)
    {
        // Crossfade Zone 2->3: Blend syncopation to wild
        GenerateSyncopationPattern(energy, seed + 0x12345678, patternLength, syncopationWeights);
        GenerateWildPattern(energy, seed, patternLength, wildWeights);

        float t = (shape - kShapeZone2bEnd) / (kShapeCrossfade3End - kShapeZone2bEnd);

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
        float chaosFactor = ((shape - kShapeCrossfade3End) / (1.0f - kShapeCrossfade3End)) * 0.15f;

        for (int step = 0; step < patternLength; ++step)
        {
            // Add seed-based chaos variation
            float chaos = (HashToFloat(seed, step + 500) - 0.5f) * chaosFactor * 2.0f;
            outWeights[step] = ClampWeight(outWeights[step] + chaos);
        }
    }
}

} // namespace daisysp_idm_grids
