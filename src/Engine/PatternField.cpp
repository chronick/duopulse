#include "PatternField.h"
#include "ArchetypeData.h"
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

} // namespace daisysp_idm_grids
