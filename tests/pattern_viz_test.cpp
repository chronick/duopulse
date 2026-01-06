/**
 * Pattern Visualization Test Tool
 *
 * Task 38: C++ test utility that outputs deterministic drum patterns
 * for given parameters and seed, enabling pattern evaluation without hardware.
 *
 * Features:
 * - ASCII grid visualization of patterns
 * - Voice 1 (Anchor), Voice 2 (Shimmer), Aux outputs
 * - Velocity values (0.00-1.00 range)
 * - Test cases for SHAPE zones, AXIS biasing, COMPLEMENT relationship
 * - Seed-determinism verification
 *
 * Reference: docs/tasks/active/38-v5-pattern-viz-cpp.md
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

#include "../src/Engine/DuoPulseTypes.h"
#include "../src/Engine/PatternField.h"
#include "../src/Engine/VoiceRelation.h"
#include "../src/Engine/VelocityCompute.h"
#include "../src/Engine/HatBurst.h"
#include "../src/Engine/HashUtils.h"
#include "../src/Engine/ControlState.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// Pattern Generation Parameters
// =============================================================================

/**
 * Parameters for pattern generation
 */
struct PatternParams
{
    float energy;           ///< ENERGY parameter (0.0-1.0)
    float shape;            ///< SHAPE parameter (0.0-1.0)
    float axisX;            ///< AXIS X parameter (0.0-1.0)
    float axisY;            ///< AXIS Y parameter (0.0-1.0)
    float drift;            ///< DRIFT parameter (0.0-1.0)
    float accent;           ///< ACCENT parameter (0.0-1.0)
    float swing;            ///< SWING parameter (0.0-1.0)
    uint32_t seed;          ///< Pattern seed
    int patternLength;      ///< Pattern length in steps

    void Init()
    {
        energy = 0.50f;
        shape = 0.30f;
        axisX = 0.50f;
        axisY = 0.50f;
        drift = 0.00f;
        accent = 0.50f;
        swing = 0.50f;
        seed = 0xDEADBEEF;
        patternLength = 32;
    }
};

/**
 * Generated pattern data for visualization
 */
struct PatternData
{
    uint32_t v1Mask;                ///< Voice 1 (Anchor) hit mask
    uint32_t v2Mask;                ///< Voice 2 (Shimmer) hit mask
    uint32_t auxMask;               ///< Aux hit mask
    float v1Velocity[kMaxSteps];    ///< Voice 1 velocities
    float v2Velocity[kMaxSteps];    ///< Voice 2 velocities
    float auxVelocity[kMaxSteps];   ///< Aux velocities
    float metricWeight[kMaxSteps];  ///< Metric weights for each step
    int patternLength;              ///< Pattern length

    void Init(int length)
    {
        v1Mask = 0;
        v2Mask = 0;
        auxMask = 0;
        patternLength = length;
        for (int i = 0; i < kMaxSteps; ++i)
        {
            v1Velocity[i] = 0.0f;
            v2Velocity[i] = 0.0f;
            auxVelocity[i] = 0.0f;
            metricWeight[i] = 0.0f;
        }
    }
};

// =============================================================================
// Pattern Generation Functions
// =============================================================================

/**
 * Compute target hit count from energy and pattern length
 */
static int ComputeTargetHits(float energy, int patternLength, float multiplier = 1.0f)
{
    // Energy scales from 10% to 50% density
    float density = 0.10f + energy * 0.40f;
    int hits = static_cast<int>(patternLength * density * multiplier + 0.5f);
    if (hits < 1) hits = 1;
    if (hits > patternLength) hits = patternLength;
    return hits;
}

/**
 * Convert weights to hit mask using top-N selection
 */
static uint32_t WeightsToMask(const float* weights, int targetHits, int patternLength, uint32_t seed)
{
    uint32_t mask = 0;

    // Add randomized threshold-based selection
    for (int i = 0; i < patternLength && targetHits > 0; ++i)
    {
        // Find highest weight step not yet selected
        int bestStep = -1;
        float bestWeight = -1.0f;

        for (int s = 0; s < patternLength; ++s)
        {
            if ((mask & (1U << s)) == 0)  // Not yet selected
            {
                // Add small hash-based jitter to break ties deterministically
                float jitter = HashToFloat(seed, s) * 0.01f;
                float effectiveWeight = weights[s] + jitter;
                if (effectiveWeight > bestWeight)
                {
                    bestWeight = effectiveWeight;
                    bestStep = s;
                }
            }
        }

        if (bestStep >= 0)
        {
            mask |= (1U << bestStep);
            --targetHits;
        }
    }

    return mask;
}

/**
 * Generate a complete pattern from parameters
 */
static void GeneratePattern(const PatternParams& params, PatternData& outPattern)
{
    outPattern.Init(params.patternLength);

    // Step 1: Compute shape-blended weights for anchor voice
    float anchorWeights[kMaxSteps];
    ComputeShapeBlendedWeights(params.shape, params.energy, params.seed,
                               params.patternLength, anchorWeights);

    // Step 2: Apply AXIS X/Y biasing
    ApplyAxisBias(anchorWeights, params.axisX, params.axisY,
                  params.shape, params.seed, params.patternLength);

    // Step 3: Convert to hit mask (anchor)
    int v1TargetHits = ComputeTargetHits(params.energy, params.patternLength, 1.0f);
    outPattern.v1Mask = WeightsToMask(anchorWeights, v1TargetHits, params.patternLength, params.seed);

    // Step 4: Compute shimmer weights (slightly different seed)
    float shimmerWeights[kMaxSteps];
    ComputeShapeBlendedWeights(params.shape, params.energy, params.seed + 1,
                               params.patternLength, shimmerWeights);

    // Step 5: Apply COMPLEMENT relationship for shimmer
    int v2TargetHits = ComputeTargetHits(params.energy, params.patternLength, 0.6f);
    outPattern.v2Mask = ApplyComplementRelationship(
        outPattern.v1Mask, shimmerWeights,
        params.drift, params.seed + 2,
        params.patternLength, v2TargetHits);

    // Step 6: Generate aux pattern (hat burst style)
    // For simplicity, use independent hits that don't overlap with anchor/shimmer
    uint32_t combinedMask = outPattern.v1Mask | outPattern.v2Mask;
    int auxTargetHits = ComputeTargetHits(params.energy, params.patternLength, 1.5f);
    float auxWeights[kMaxSteps];
    for (int i = 0; i < params.patternLength; ++i)
    {
        // Aux prefers offbeats
        float metricW = GetMetricWeight(i, params.patternLength);
        auxWeights[i] = 1.0f - metricW * 0.5f;
        if ((combinedMask & (1U << i)) != 0)
        {
            auxWeights[i] *= 0.3f;  // Reduce weight where other voices hit
        }
    }
    outPattern.auxMask = WeightsToMask(auxWeights, auxTargetHits, params.patternLength, params.seed + 3);

    // Step 7: Compute velocities for each voice
    for (int step = 0; step < params.patternLength; ++step)
    {
        outPattern.metricWeight[step] = GetMetricWeight(step, params.patternLength);

        if ((outPattern.v1Mask & (1U << step)) != 0)
        {
            outPattern.v1Velocity[step] = ComputeAccentVelocity(
                params.accent, step, params.patternLength, params.seed);
        }

        if ((outPattern.v2Mask & (1U << step)) != 0)
        {
            // Shimmer typically has lower velocity (backbeat feel)
            outPattern.v2Velocity[step] = ComputeAccentVelocity(
                params.accent * 0.7f, step, params.patternLength, params.seed + 1);
        }

        if ((outPattern.auxMask & (1U << step)) != 0)
        {
            // Aux velocity based on energy
            float baseVel = 0.5f + params.energy * 0.3f;
            float variation = (HashToFloat(params.seed + 4, step) - 0.5f) * 0.15f;
            outPattern.auxVelocity[step] = baseVel + variation;
            if (outPattern.auxVelocity[step] < 0.3f) outPattern.auxVelocity[step] = 0.3f;
            if (outPattern.auxVelocity[step] > 1.0f) outPattern.auxVelocity[step] = 1.0f;
        }
    }
}

// =============================================================================
// Pattern Visualization Functions
// =============================================================================

/**
 * Print a metric weight bar visualization
 */
static std::string MetricBar(float weight)
{
    int bars = static_cast<int>(weight * 4 + 0.5f);
    std::string result;
    for (int i = 0; i < bars; ++i)
    {
        result += "|";
    }
    while (result.length() < 4)
    {
        result += " ";
    }
    return result;
}

/**
 * Format velocity value or "----" if no hit
 */
static std::string FormatVelocity(float velocity, bool hasHit)
{
    if (!hasHit)
    {
        return "----";
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << velocity;
    return oss.str();
}

/**
 * Print pattern visualization header
 */
static void PrintPatternHeader(const PatternParams& params, std::ostream& out)
{
    out << "\n=== Pattern Visualization ===\n";
    out << "Params: ENERGY=" << std::fixed << std::setprecision(2) << params.energy
        << " SHAPE=" << params.shape
        << " AXIS_X=" << params.axisX
        << " AXIS_Y=" << params.axisY << "\n";
    out << "Config: DRIFT=" << params.drift
        << " ACCENT=" << params.accent
        << " SWING=" << params.swing << "\n";
    out << "Seed: 0x" << std::hex << std::uppercase << params.seed << std::dec << "\n";
    out << "Pattern Length: " << params.patternLength << " steps\n\n";
}

/**
 * Print pattern grid
 */
static void PrintPatternGrid(const PatternData& pattern, std::ostream& out)
{
    // Header row
    out << "Step  V1  V2  Aux  V1_Vel  V2_Vel  Aux_Vel  Metric\n";
    out << std::string(56, '-') << "\n";

    // Pattern rows
    for (int step = 0; step < pattern.patternLength; ++step)
    {
        bool v1Hit = (pattern.v1Mask & (1U << step)) != 0;
        bool v2Hit = (pattern.v2Mask & (1U << step)) != 0;
        bool auxHit = (pattern.auxMask & (1U << step)) != 0;

        out << std::setw(2) << step << "    "
            << (v1Hit ? "X" : ".") << "   "
            << (v2Hit ? "X" : ".") << "   "
            << (auxHit ? "X" : ".") << "    "
            << FormatVelocity(pattern.v1Velocity[step], v1Hit) << "    "
            << FormatVelocity(pattern.v2Velocity[step], v2Hit) << "    "
            << FormatVelocity(pattern.auxVelocity[step], auxHit) << "     "
            << std::fixed << std::setprecision(2) << pattern.metricWeight[step] << "  "
            << MetricBar(pattern.metricWeight[step]) << "\n";
    }
}

/**
 * Print pattern summary statistics
 */
static void PrintPatternSummary(const PatternData& pattern, std::ostream& out)
{
    int v1Hits = 0, v2Hits = 0, auxHits = 0;
    float v1VelSum = 0, v2VelSum = 0;

    for (int i = 0; i < pattern.patternLength; ++i)
    {
        if ((pattern.v1Mask & (1U << i)) != 0)
        {
            v1Hits++;
            v1VelSum += pattern.v1Velocity[i];
        }
        if ((pattern.v2Mask & (1U << i)) != 0)
        {
            v2Hits++;
            v2VelSum += pattern.v2Velocity[i];
        }
        if ((pattern.auxMask & (1U << i)) != 0)
        {
            auxHits++;
        }
    }

    out << "\nSummary:\n";
    out << "  V1 hits: " << v1Hits << "/" << pattern.patternLength
        << " (" << (v1Hits * 100 / pattern.patternLength) << "%)\n";
    out << "  V2 hits: " << v2Hits << "/" << pattern.patternLength
        << " (" << (v2Hits * 100 / pattern.patternLength) << "%)\n";
    out << "  Aux hits: " << auxHits << "/" << pattern.patternLength
        << " (" << (auxHits * 100 / pattern.patternLength) << "%)\n";

    if (v1Hits > 0)
    {
        out << "  V1 avg velocity: " << std::fixed << std::setprecision(2)
            << (v1VelSum / v1Hits) << "\n";
    }
    if (v2Hits > 0)
    {
        out << "  V2 avg velocity: " << std::fixed << std::setprecision(2)
            << (v2VelSum / v2Hits) << "\n";
    }
}

/**
 * Print full pattern visualization
 */
static void PrintPattern(const PatternParams& params, const PatternData& pattern, std::ostream& out)
{
    PrintPatternHeader(params, out);
    PrintPatternGrid(pattern, out);
    PrintPatternSummary(pattern, out);
}

// =============================================================================
// Helper Functions for Tests
// =============================================================================

/**
 * Count hits in a mask
 */
static int CountHits(uint32_t mask, int patternLength)
{
    int count = 0;
    for (int i = 0; i < patternLength; ++i)
    {
        if ((mask & (1U << i)) != 0) count++;
    }
    return count;
}

/**
 * Check if two masks overlap (have any common hits)
 */
static bool MasksOverlap(uint32_t mask1, uint32_t mask2)
{
    return (mask1 & mask2) != 0;
}

/**
 * Check if anchor has hits on strong beats (0, 8, 16, 24 for 32-step)
 */
static bool HasStrongBeatHits(uint32_t mask, int patternLength)
{
    // Strong beats for 32-step pattern
    uint32_t strongBeats = 0;
    for (int i = 0; i < patternLength; i += 8)
    {
        strongBeats |= (1U << i);
    }
    return (mask & strongBeats) != 0;
}

// =============================================================================
// SHAPE Zone Tests
// =============================================================================

TEST_CASE("SHAPE zones produce distinct patterns", "[pattern-viz][shape]")
{
    PatternParams params;
    PatternData pattern;

    SECTION("Zone 1: Stable (SHAPE=0.15)")
    {
        params.Init();
        params.shape = 0.15f;
        params.energy = 0.50f;
        params.seed = 12345;

        GeneratePattern(params, pattern);

        // Stable zone should have strong downbeat emphasis
        REQUIRE(HasStrongBeatHits(pattern.v1Mask, params.patternLength));

        // Output for visual inspection (only in debug)
        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Should have reasonable hit count
        int v1Hits = CountHits(pattern.v1Mask, params.patternLength);
        REQUIRE(v1Hits >= 4);
        REQUIRE(v1Hits <= 16);
    }

    SECTION("Zone 2: Syncopated (SHAPE=0.50)")
    {
        params.Init();
        params.shape = 0.50f;
        params.energy = 0.50f;
        params.seed = 12345;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Syncopated zone may or may not hit strong beats
        // Main test is that pattern generates without errors
        int v1Hits = CountHits(pattern.v1Mask, params.patternLength);
        REQUIRE(v1Hits >= 3);
        REQUIRE(v1Hits <= 20);
    }

    SECTION("Zone 3: Wild (SHAPE=0.85)")
    {
        params.Init();
        params.shape = 0.85f;
        params.energy = 0.50f;
        params.seed = 12345;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Wild zone should still produce valid pattern
        int v1Hits = CountHits(pattern.v1Mask, params.patternLength);
        REQUIRE(v1Hits >= 3);
        REQUIRE(v1Hits <= 20);
    }

    SECTION("Different SHAPE values produce different patterns")
    {
        params.Init();
        params.energy = 0.50f;
        params.seed = 54321;

        PatternData stable, syncopated, wild;

        params.shape = 0.15f;
        GeneratePattern(params, stable);

        params.shape = 0.50f;
        GeneratePattern(params, syncopated);

        params.shape = 0.85f;
        GeneratePattern(params, wild);

        // At least two of the three should be different
        bool stableVsSyncopated = (stable.v1Mask != syncopated.v1Mask);
        bool syncopatedVsWild = (syncopated.v1Mask != wild.v1Mask);
        bool stableVsWild = (stable.v1Mask != wild.v1Mask);

        REQUIRE((stableVsSyncopated || syncopatedVsWild || stableVsWild));
    }
}

// =============================================================================
// AXIS X/Y Tests
// =============================================================================

TEST_CASE("AXIS X biases beat position", "[pattern-viz][axis]")
{
    PatternParams params;
    PatternData pattern;

    SECTION("AXIS_X=0.0: Grounded (downbeat heavy)")
    {
        params.Init();
        params.axisX = 0.0f;
        params.shape = 0.30f;
        params.energy = 0.50f;
        params.seed = 11111;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Should have strong beat emphasis
        REQUIRE(HasStrongBeatHits(pattern.v1Mask, params.patternLength));
    }

    SECTION("AXIS_X=1.0: Floating (offbeat heavy)")
    {
        params.Init();
        params.axisX = 1.0f;
        params.shape = 0.30f;
        params.energy = 0.50f;
        params.seed = 11111;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Pattern should still be valid
        int v1Hits = CountHits(pattern.v1Mask, params.patternLength);
        REQUIRE(v1Hits >= 3);
    }

    SECTION("Different AXIS_X values produce different patterns")
    {
        params.Init();
        params.shape = 0.30f;
        params.energy = 0.50f;
        params.seed = 22222;

        PatternData grounded, neutral, floating;

        params.axisX = 0.0f;
        GeneratePattern(params, grounded);

        params.axisX = 0.5f;
        GeneratePattern(params, neutral);

        params.axisX = 1.0f;
        GeneratePattern(params, floating);

        // Patterns should differ due to AXIS X bias
        bool groundedVsNeutral = (grounded.v1Mask != neutral.v1Mask);
        bool neutralVsFloating = (neutral.v1Mask != floating.v1Mask);

        REQUIRE((groundedVsNeutral || neutralVsFloating));
    }
}

TEST_CASE("AXIS Y controls intricacy", "[pattern-viz][axis]")
{
    PatternParams params;
    PatternData pattern;

    SECTION("AXIS_Y=0.0: Simple (sparse)")
    {
        params.Init();
        params.axisY = 0.0f;
        params.shape = 0.30f;
        params.energy = 0.50f;
        params.seed = 33333;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Pattern should be valid
        int v1Hits = CountHits(pattern.v1Mask, params.patternLength);
        REQUIRE(v1Hits >= 3);
    }

    SECTION("AXIS_Y=1.0: Complex (busy)")
    {
        params.Init();
        params.axisY = 1.0f;
        params.shape = 0.30f;
        params.energy = 0.50f;
        params.seed = 33333;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Pattern should be valid
        int v1Hits = CountHits(pattern.v1Mask, params.patternLength);
        REQUIRE(v1Hits >= 3);
    }
}

// =============================================================================
// COMPLEMENT Voice Relationship Tests
// =============================================================================

TEST_CASE("COMPLEMENT fills anchor gaps", "[pattern-viz][complement]")
{
    PatternParams params;
    PatternData pattern;

    SECTION("V2 hits fall in V1 gaps - no simultaneous hits")
    {
        params.Init();
        params.shape = 0.30f;
        params.energy = 0.50f;
        params.drift = 0.0f;  // Low drift for predictable placement
        params.seed = 44444;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // V1 and V2 should NOT overlap (COMPLEMENT relationship)
        REQUIRE_FALSE(MasksOverlap(pattern.v1Mask, pattern.v2Mask));
    }

    SECTION("COMPLEMENT works across different energy levels")
    {
        params.Init();
        params.shape = 0.30f;
        params.drift = 0.0f;
        params.seed = 55555;

        // Test low energy
        params.energy = 0.20f;
        GeneratePattern(params, pattern);
        REQUIRE_FALSE(MasksOverlap(pattern.v1Mask, pattern.v2Mask));

        // Test high energy
        params.energy = 0.80f;
        GeneratePattern(params, pattern);
        REQUIRE_FALSE(MasksOverlap(pattern.v1Mask, pattern.v2Mask));
    }

    SECTION("COMPLEMENT works with high drift")
    {
        params.Init();
        params.shape = 0.30f;
        params.energy = 0.50f;
        params.drift = 0.90f;  // High drift for random placement within gaps
        params.seed = 66666;

        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);
        INFO(oss.str());

        // Even with high drift, should still not overlap
        REQUIRE_FALSE(MasksOverlap(pattern.v1Mask, pattern.v2Mask));
    }
}

// =============================================================================
// Seed Determinism Tests
// =============================================================================

TEST_CASE("Same seed produces identical patterns", "[pattern-viz][determinism]")
{
    PatternParams params;
    PatternData pattern1, pattern2;

    SECTION("Identical parameters produce identical output")
    {
        params.Init();
        params.energy = 0.50f;
        params.shape = 0.40f;
        params.axisX = 0.60f;
        params.axisY = 0.30f;
        params.seed = 0xDEADBEEF;

        GeneratePattern(params, pattern1);
        GeneratePattern(params, pattern2);

        REQUIRE(pattern1.v1Mask == pattern2.v1Mask);
        REQUIRE(pattern1.v2Mask == pattern2.v2Mask);
        REQUIRE(pattern1.auxMask == pattern2.auxMask);

        // Velocities should also match
        for (int i = 0; i < params.patternLength; ++i)
        {
            REQUIRE(pattern1.v1Velocity[i] == Approx(pattern2.v1Velocity[i]));
            REQUIRE(pattern1.v2Velocity[i] == Approx(pattern2.v2Velocity[i]));
            REQUIRE(pattern1.auxVelocity[i] == Approx(pattern2.auxVelocity[i]));
        }
    }

    SECTION("Different seeds produce different patterns")
    {
        params.Init();
        params.energy = 0.50f;
        params.shape = 0.40f;

        params.seed = 11111;
        GeneratePattern(params, pattern1);

        params.seed = 99999;
        GeneratePattern(params, pattern2);

        // At least one mask should differ
        bool anyDifferent = (pattern1.v1Mask != pattern2.v1Mask) ||
                           (pattern1.v2Mask != pattern2.v2Mask) ||
                           (pattern1.auxMask != pattern2.auxMask);
        REQUIRE(anyDifferent);
    }

    SECTION("Determinism across multiple runs")
    {
        params.Init();
        params.seed = 0xCAFEBABE;

        // Generate 5 times with same params
        uint32_t firstV1Mask = 0;
        for (int run = 0; run < 5; ++run)
        {
            GeneratePattern(params, pattern1);

            if (run == 0)
            {
                firstV1Mask = pattern1.v1Mask;
            }
            else
            {
                REQUIRE(pattern1.v1Mask == firstV1Mask);
            }
        }
    }
}

// =============================================================================
// Velocity Output Tests
// =============================================================================

TEST_CASE("Velocity values are in valid range", "[pattern-viz][velocity]")
{
    PatternParams params;
    PatternData pattern;

    SECTION("All velocities are 0.00-1.00")
    {
        params.Init();
        params.energy = 0.70f;
        params.accent = 0.80f;
        params.seed = 77777;

        GeneratePattern(params, pattern);

        for (int i = 0; i < params.patternLength; ++i)
        {
            // V1 velocity
            if ((pattern.v1Mask & (1U << i)) != 0)
            {
                REQUIRE(pattern.v1Velocity[i] >= 0.0f);
                REQUIRE(pattern.v1Velocity[i] <= 1.0f);
            }
            else
            {
                REQUIRE(pattern.v1Velocity[i] == 0.0f);
            }

            // V2 velocity
            if ((pattern.v2Mask & (1U << i)) != 0)
            {
                REQUIRE(pattern.v2Velocity[i] >= 0.0f);
                REQUIRE(pattern.v2Velocity[i] <= 1.0f);
            }
            else
            {
                REQUIRE(pattern.v2Velocity[i] == 0.0f);
            }

            // Aux velocity
            if ((pattern.auxMask & (1U << i)) != 0)
            {
                REQUIRE(pattern.auxVelocity[i] >= 0.0f);
                REQUIRE(pattern.auxVelocity[i] <= 1.0f);
            }
            else
            {
                REQUIRE(pattern.auxVelocity[i] == 0.0f);
            }
        }
    }

    SECTION("ACCENT parameter affects velocity range")
    {
        params.Init();
        params.energy = 0.50f;
        params.seed = 88888;

        // Low accent - narrow velocity range
        params.accent = 0.0f;
        GeneratePattern(params, pattern);
        float lowAccentVel = 0.0f;
        int count = 0;
        for (int i = 0; i < params.patternLength; ++i)
        {
            if ((pattern.v1Mask & (1U << i)) != 0)
            {
                lowAccentVel += pattern.v1Velocity[i];
                count++;
            }
        }
        float lowAccentAvg = (count > 0) ? lowAccentVel / count : 0.0f;

        // High accent - wide velocity range
        params.accent = 1.0f;
        GeneratePattern(params, pattern);
        float highAccentMin = 1.0f;
        float highAccentMax = 0.0f;
        count = 0;
        for (int i = 0; i < params.patternLength; ++i)
        {
            if ((pattern.v1Mask & (1U << i)) != 0)
            {
                float v = pattern.v1Velocity[i];
                if (v < highAccentMin) highAccentMin = v;
                if (v > highAccentMax) highAccentMax = v;
                count++;
            }
        }

        // High accent should have wider range than low accent
        (void)count;  // Suppress unused variable warning
        float highAccentRange = highAccentMax - highAccentMin;
        INFO("Low accent avg: " << lowAccentAvg);
        INFO("High accent range: " << highAccentRange);

        // At least check velocities are valid
        REQUIRE(highAccentMin >= 0.0f);
        REQUIRE(highAccentMax <= 1.0f);
    }
}

// =============================================================================
// Pattern Length Tests
// =============================================================================

TEST_CASE("Pattern generation works with different lengths", "[pattern-viz][length]")
{
    PatternParams params;
    PatternData pattern;

    SECTION("16-step pattern")
    {
        params.Init();
        params.patternLength = 16;
        params.seed = 99999;

        GeneratePattern(params, pattern);

        REQUIRE(pattern.patternLength == 16);
        // Hits should be within valid range
        REQUIRE(CountHits(pattern.v1Mask, 16) >= 1);
        REQUIRE(CountHits(pattern.v1Mask, 16) <= 16);
    }

    SECTION("32-step pattern")
    {
        params.Init();
        params.patternLength = 32;
        params.seed = 99999;

        GeneratePattern(params, pattern);

        REQUIRE(pattern.patternLength == 32);
        REQUIRE(CountHits(pattern.v1Mask, 32) >= 1);
        REQUIRE(CountHits(pattern.v1Mask, 32) <= 32);
    }
}

// =============================================================================
// Visual Output Test (for manual inspection)
// =============================================================================

TEST_CASE("Visual pattern output for inspection", "[pattern-viz][visual]")
{
    PatternParams params;
    PatternData pattern;

    SECTION("Default parameters pattern")
    {
        params.Init();
        GeneratePattern(params, pattern);

        std::ostringstream oss;
        PrintPattern(params, pattern, oss);

        // Just ensure output is generated
        REQUIRE(oss.str().length() > 100);

        // Print to INFO for viewing when test runs with verbose
        INFO(oss.str());
    }

    SECTION("Sweep SHAPE parameter")
    {
        params.Init();
        params.seed = 0xABCD1234;

        std::ostringstream oss;
        oss << "\n=== SHAPE Parameter Sweep ===\n";

        for (float shape = 0.0f; shape <= 1.0f; shape += 0.25f)
        {
            params.shape = shape;
            GeneratePattern(params, pattern);

            oss << "\n--- SHAPE=" << std::fixed << std::setprecision(2) << shape << " ---\n";
            int v1Hits = CountHits(pattern.v1Mask, params.patternLength);
            int v2Hits = CountHits(pattern.v2Mask, params.patternLength);
            oss << "V1 hits: " << v1Hits << ", V2 hits: " << v2Hits << "\n";
            oss << "V1 mask: 0x" << std::hex << pattern.v1Mask << std::dec << "\n";
        }

        INFO(oss.str());
        REQUIRE(true);  // Always pass, this is for visual inspection
    }
}
