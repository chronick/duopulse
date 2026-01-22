#include "EuclideanGen.h"
#include "PulseField.h"  // For Clamp, HashStep
#include "GumbelSampler.h"  // For SelectTopK

namespace daisysp_idm_grids
{

// =============================================================================
// Bjorklund Euclidean Algorithm
// =============================================================================

uint64_t GenerateEuclidean(int hits, int steps)
{
    // Clamp inputs to valid ranges
    if (hits < 0) hits = 0;
    if (hits > steps) hits = steps;
    if (steps < 1 || steps > 64) return 0;

    // Special cases
    if (hits == 0) return 0;
    if (hits >= steps) return (1ULL << steps) - 1;  // All steps

    // Bjorklund algorithm: distribute hits evenly
    // We use a simple bucket-fill approach:
    // For each step i, accumulate (hits / steps).
    // When accumulator >= 1, place a hit and decrement.
    //
    // Initialize accumulator to (1.0 - step_size) so the first hit lands at step 0.
    // This ensures euclidean(64,16) produces hits at 0,4,8,12... (quarter notes)
    // rather than 3,7,11,15... (offset by rounding error).

    uint64_t pattern = 0;
    int placedHits = 0;
    float step_size = static_cast<float>(hits) / static_cast<float>(steps);
    float accumulator = 1.0f - step_size;  // Start so first hit lands at position 0

    for (int i = 0; i < steps; ++i)
    {
        accumulator += step_size;
        if (accumulator >= 1.0f && placedHits < hits)
        {
            pattern |= (1ULL << i);
            placedHits++;
            accumulator -= 1.0f;
        }
    }

    // If we haven't placed all hits due to rounding, place remaining at the end
    for (int i = steps - 1; i >= 0 && placedHits < hits; --i)
    {
        if (!(pattern & (1ULL << i)))
        {
            pattern |= (1ULL << i);
            placedHits++;
        }
    }

    return pattern;
}

// =============================================================================
// Pattern Rotation
// =============================================================================

uint64_t RotatePattern(uint64_t pattern, int offset, int steps)
{
    if (steps < 1 || steps > 64) return pattern;

    // Normalize offset to [0, steps)
    offset = offset % steps;
    if (offset < 0) offset += steps;

    // Create a mask for the valid bits
    uint64_t mask = (1ULL << steps) - 1;

    // Rotate right by offset
    uint64_t rotated = ((pattern >> offset) | (pattern << (steps - offset))) & mask;

    return rotated;
}

// =============================================================================
// Euclidean + Weight Blending
// =============================================================================

uint64_t BlendEuclideanWithWeights(
    int budget,
    int steps,
    const float* weights,
    uint64_t eligibility,
    float euclideanRatio,
    uint32_t seed)
{
    // Clamp inputs
    euclideanRatio = Clamp(euclideanRatio, 0.0f, 1.0f);
    if (budget < 0) budget = 0;
    if (budget > steps) budget = steps;
    if (steps < 1 || steps > 64) return 0;

    // If ratio is zero or budget is zero, use pure Gumbel selection
    if (euclideanRatio < 0.01f || budget == 0)
    {
        return SelectHitsGumbelSimple(weights, eligibility, budget, seed, steps);
    }

    // If ratio is near 1.0, use pure Euclidean
    if (euclideanRatio > 0.99f)
    {
        uint64_t euclidean = GenerateEuclidean(budget, steps);

        // For four-on-floor patterns (budget = quarter notes), skip rotation
        // to preserve alignment with quarter-note eligibility mask
        // euclidean(64,16) or euclidean(32,8) should land on 0,4,8,12...
        int quarterNotes = steps / 4;
        if (budget == quarterNotes)
        {
            // No rotation - pure quarter-note grid
            return euclidean & eligibility;
        }

        // Rotate by seed-derived offset for other euclidean patterns
        int rotation = static_cast<int>(seed % static_cast<uint32_t>(steps));
        uint64_t rotated = RotatePattern(euclidean, rotation, steps);
        // Apply eligibility mask
        return rotated & eligibility;
    }

    // Hybrid blending: reserve Euclidean hits, fill remainder from Gumbel
    // 1. Determine how many hits come from Euclidean vs Gumbel
    int euclideanHits = static_cast<int>(budget * euclideanRatio);
    int gumbelHits = budget - euclideanHits;

    // 2. Generate Euclidean pattern and rotate
    uint64_t euclideanPattern = GenerateEuclidean(euclideanHits, steps);
    int rotation = static_cast<int>(seed % static_cast<uint32_t>(steps));
    euclideanPattern = RotatePattern(euclideanPattern, rotation, steps);
    euclideanPattern &= eligibility;  // Apply eligibility

    // 3. Select remaining hits from Gumbel, excluding Euclidean positions
    uint64_t remainingEligibility = eligibility & ~euclideanPattern;
    uint64_t gumbelPattern = SelectHitsGumbelSimple(weights, remainingEligibility, gumbelHits, seed ^ 0xE0C1, steps);

    // 4. Combine patterns
    return euclideanPattern | gumbelPattern;
}

// =============================================================================
// Genre-Specific Euclidean Ratios
// =============================================================================

float GetGenreEuclideanRatio(Genre genre, float fieldX, EnergyZone zone, float shape)
{
    // Only active in MINIMAL and GROOVE zones
    if (zone != EnergyZone::MINIMAL && zone != EnergyZone::GROOVE)
        return 0.0f;

    // At very low SHAPE (pure euclidean mode), return 1.0 for four-on-floor patterns
    // euclidean(64,16) produces perfect quarter-note grid
    shape = Clamp(shape, 0.0f, 1.0f);
    if (shape <= 0.05f)
        return 1.0f;

    // Clamp Field X
    fieldX = Clamp(fieldX, 0.0f, 1.0f);

    // Genre-specific base ratios at Field X = 0
    float baseRatio;
    switch (genre)
    {
        case Genre::TECHNO:
            baseRatio = 0.70f;  // Strong Euclidean foundation for 4-on-floor
            break;
        case Genre::TRIBAL:
            baseRatio = 0.40f;  // Moderate Euclidean for polyrhythmic balance
            break;
        case Genre::IDM:
        default:
            baseRatio = 0.0f;   // No Euclidean (maximum irregularity)
            break;
    }

    // Taper ratio with Field X: at X=1.0, ratio reduces by 70%
    // taper = 1.0 - 0.7 * fieldX
    // effectiveRatio = baseRatio * (1.0 - 0.7 * fieldX)
    float taper = 1.0f - 0.7f * fieldX;
    float effectiveRatio = baseRatio * taper;

    return Clamp(effectiveRatio, 0.0f, 1.0f);
}

} // namespace daisysp_idm_grids
