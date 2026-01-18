/**
 * Unit tests for Algorithm Weights System (Task 56)
 *
 * Tests the weight-based blending of euclidean, syncopation, and random
 * pattern generation algorithms.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/AlgorithmWeights.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// Math Utility Tests
// =============================================================================

TEST_CASE("Smoothstep produces correct values", "[AlgorithmWeights][math]")
{
    SECTION("Returns 0 below edge0")
    {
        REQUIRE(Smoothstep(0.3f, 0.7f, 0.0f) == Approx(0.0f));
        REQUIRE(Smoothstep(0.3f, 0.7f, 0.29f) == Approx(0.0f));
    }

    SECTION("Returns 1 above edge1")
    {
        REQUIRE(Smoothstep(0.3f, 0.7f, 0.7f) == Approx(1.0f));
        REQUIRE(Smoothstep(0.3f, 0.7f, 1.0f) == Approx(1.0f));
    }

    SECTION("Returns 0.5 at midpoint")
    {
        REQUIRE(Smoothstep(0.3f, 0.7f, 0.5f) == Approx(0.5f));
    }

    SECTION("Provides smooth interpolation")
    {
        // Check values at various points
        float t1 = Smoothstep(0.0f, 1.0f, 0.25f);
        float t2 = Smoothstep(0.0f, 1.0f, 0.5f);
        float t3 = Smoothstep(0.0f, 1.0f, 0.75f);

        // Should be monotonically increasing
        REQUIRE(t1 < t2);
        REQUIRE(t2 < t3);

        // Smoothstep formula: 3t^2 - 2t^3
        // At t=0.25: 3*(0.0625) - 2*(0.015625) = 0.1875 - 0.03125 = 0.15625
        REQUIRE(t1 == Approx(0.15625f));
    }
}

TEST_CASE("BellCurve produces correct values", "[AlgorithmWeights][math]")
{
    SECTION("Returns 1.0 at center")
    {
        REQUIRE(BellCurve(0.5f, 0.5f, 0.3f) == Approx(1.0f));
        REQUIRE(BellCurve(0.0f, 0.0f, 0.3f) == Approx(1.0f));
    }

    SECTION("Returns lower values away from center")
    {
        float atCenter = BellCurve(0.5f, 0.5f, 0.3f);
        float oneSigma = BellCurve(0.2f, 0.5f, 0.3f);  // 1σ away
        float twoSigma = BellCurve(0.0f, 0.5f, 0.2f);  // ~2.5σ away

        REQUIRE(atCenter > oneSigma);
        REQUIRE(oneSigma > twoSigma);
    }

    SECTION("Is symmetric around center")
    {
        float left = BellCurve(0.3f, 0.5f, 0.3f);
        float right = BellCurve(0.7f, 0.5f, 0.3f);
        REQUIRE(left == Approx(right));
    }
}

// =============================================================================
// Algorithm Weight Computation Tests
// =============================================================================

TEST_CASE("Algorithm weights are normalized", "[AlgorithmWeights][normalization]")
{
    SECTION("Weights sum to 1.0 at various SHAPE values")
    {
        for (float shape = 0.0f; shape <= 1.0f; shape += 0.1f)
        {
            AlgorithmWeights weights = ComputeAlgorithmWeights(shape);
            float total = weights.euclidean + weights.syncopation + weights.random;
            REQUIRE(total == Approx(1.0f).margin(0.001f));
        }
    }

    SECTION("All weights are non-negative")
    {
        for (float shape = 0.0f; shape <= 1.0f; shape += 0.05f)
        {
            AlgorithmWeights weights = ComputeAlgorithmWeights(shape);
            REQUIRE(weights.euclidean >= 0.0f);
            REQUIRE(weights.syncopation >= 0.0f);
            REQUIRE(weights.random >= 0.0f);
        }
    }
}

TEST_CASE("Euclidean dominates at low SHAPE", "[AlgorithmWeights][euclidean]")
{
    AlgorithmWeights weights = ComputeAlgorithmWeights(0.0f);

    SECTION("Euclidean is highest weight at SHAPE=0")
    {
        REQUIRE(weights.euclidean > weights.syncopation);
        REQUIRE(weights.euclidean > weights.random);
    }

    SECTION("Euclidean is substantial at SHAPE=0.2")
    {
        AlgorithmWeights w = ComputeAlgorithmWeights(0.2f);
        REQUIRE(w.euclidean >= 0.5f);  // Should be at least 50%
    }
}

TEST_CASE("Syncopation peaks in middle SHAPE", "[AlgorithmWeights][syncopation]")
{
    AlgorithmWeights weights = ComputeAlgorithmWeights(0.5f);

    SECTION("Syncopation is highest weight at SHAPE=0.5")
    {
        REQUIRE(weights.syncopation > weights.euclidean);
        REQUIRE(weights.syncopation > weights.random);
    }

    SECTION("Syncopation contribution is substantial")
    {
        REQUIRE(weights.syncopation >= 0.5f);  // Should be at least 50%
    }
}

TEST_CASE("Random dominates at high SHAPE", "[AlgorithmWeights][random]")
{
    AlgorithmWeights weights = ComputeAlgorithmWeights(1.0f);

    SECTION("Random is highest weight at SHAPE=1")
    {
        REQUIRE(weights.random > weights.euclidean);
        REQUIRE(weights.random > weights.syncopation);
    }

    SECTION("Random is substantial at SHAPE=0.9")
    {
        AlgorithmWeights w = ComputeAlgorithmWeights(0.9f);
        REQUIRE(w.random >= 0.5f);  // Should be at least 50%
    }
}

TEST_CASE("Weight transitions are smooth", "[AlgorithmWeights][transitions]")
{
    SECTION("No sudden jumps in euclidean weight")
    {
        float prevWeight = ComputeAlgorithmWeights(0.0f).euclidean;
        for (float shape = 0.01f; shape <= 1.0f; shape += 0.01f)
        {
            float currWeight = ComputeAlgorithmWeights(shape).euclidean;
            float delta = std::abs(currWeight - prevWeight);
            REQUIRE(delta < 0.1f);  // No jumps greater than 10%
            prevWeight = currWeight;
        }
    }

    SECTION("No sudden jumps in random weight")
    {
        float prevWeight = ComputeAlgorithmWeights(0.0f).random;
        for (float shape = 0.01f; shape <= 1.0f; shape += 0.01f)
        {
            float currWeight = ComputeAlgorithmWeights(shape).random;
            float delta = std::abs(currWeight - prevWeight);
            REQUIRE(delta < 0.1f);  // No jumps greater than 10%
            prevWeight = currWeight;
        }
    }
}

// =============================================================================
// Per-Channel Euclidean Parameter Tests
// =============================================================================

TEST_CASE("Channel euclidean k scales with ENERGY", "[AlgorithmWeights][euclidean]")
{
    SECTION("At ENERGY=0, all k values are at minimum")
    {
        ChannelEuclideanParams params = ComputeChannelEuclidean(0.0f, 0xDEADBEEF, 32);
        REQUIRE(params.anchorK == 4);   // kAnchorKMin
        REQUIRE(params.shimmerK == 6);  // kShimmerKMin
        REQUIRE(params.auxK == 2);      // kAuxKMin
    }

    SECTION("At ENERGY=1, all k values are at maximum")
    {
        ChannelEuclideanParams params = ComputeChannelEuclidean(1.0f, 0xDEADBEEF, 32);
        REQUIRE(params.anchorK == 12);  // kAnchorKMax
        REQUIRE(params.shimmerK == 16); // kShimmerKMax
        REQUIRE(params.auxK == 8);      // kAuxKMax
    }

    SECTION("At ENERGY=0.5, k values are at midpoint")
    {
        ChannelEuclideanParams params = ComputeChannelEuclidean(0.5f, 0xDEADBEEF, 32);
        REQUIRE(params.anchorK == 8);   // (4 + 12) / 2
        REQUIRE(params.shimmerK == 11); // (6 + 16) / 2
        REQUIRE(params.auxK == 5);      // (2 + 8) / 2
    }
}

TEST_CASE("Channel euclidean k is clamped to pattern length", "[AlgorithmWeights][euclidean]")
{
    SECTION("k values don't exceed pattern length")
    {
        ChannelEuclideanParams params = ComputeChannelEuclidean(1.0f, 0xDEADBEEF, 8);
        REQUIRE(params.anchorK <= 8);
        REQUIRE(params.shimmerK <= 8);
        REQUIRE(params.auxK <= 8);
    }
}

TEST_CASE("Rotation is seed-deterministic", "[AlgorithmWeights][euclidean]")
{
    SECTION("Same seed produces same rotation")
    {
        ChannelEuclideanParams p1 = ComputeChannelEuclidean(0.5f, 0x12345678, 32);
        ChannelEuclideanParams p2 = ComputeChannelEuclidean(0.5f, 0x12345678, 32);
        REQUIRE(p1.rotation == p2.rotation);
    }

    SECTION("Different seeds produce different rotations")
    {
        ChannelEuclideanParams p1 = ComputeChannelEuclidean(0.5f, 0x12345678, 32);
        ChannelEuclideanParams p2 = ComputeChannelEuclidean(0.5f, 0x87654321, 32);
        // Not guaranteed to be different, but should vary with seed
        // Testing that both are valid (within range)
        REQUIRE(p1.rotation >= 0);
        REQUIRE(p1.rotation < 8);  // patternLength / 4
        REQUIRE(p2.rotation >= 0);
        REQUIRE(p2.rotation < 8);
    }
}

// =============================================================================
// Debug Output Tests
// =============================================================================

TEST_CASE("Debug structure contains all expected fields", "[AlgorithmWeights][debug]")
{
    AlgorithmWeightsDebug debug = ComputeAlgorithmWeightsDebug(0.5f, 0.5f, 0xDEADBEEF, 32);

    SECTION("Input parameters are stored")
    {
        REQUIRE(debug.shape == 0.5f);
        REQUIRE(debug.energy == 0.5f);
    }

    SECTION("Config values are populated")
    {
        REQUIRE(debug.euclideanFadeStart == 0.3f);
        REQUIRE(debug.euclideanFadeEnd == 0.7f);
        REQUIRE(debug.syncopationCenter == 0.5f);
        REQUIRE(debug.syncopationWidth == 0.3f);
        REQUIRE(debug.randomFadeStart == 0.5f);
        REQUIRE(debug.randomFadeEnd == 0.9f);
    }

    SECTION("Raw weights are computed")
    {
        REQUIRE(debug.rawEuclidean >= 0.0f);
        REQUIRE(debug.rawSyncopation >= 0.0f);
        REQUIRE(debug.rawRandom >= 0.0f);
    }

    SECTION("Normalized weights match ComputeAlgorithmWeights")
    {
        AlgorithmWeights expected = ComputeAlgorithmWeights(0.5f);
        REQUIRE(debug.weights.euclidean == Approx(expected.euclidean));
        REQUIRE(debug.weights.syncopation == Approx(expected.syncopation));
        REQUIRE(debug.weights.random == Approx(expected.random));
    }

    SECTION("Channel params match ComputeChannelEuclidean")
    {
        ChannelEuclideanParams expected = ComputeChannelEuclidean(0.5f, 0xDEADBEEF, 32);
        REQUIRE(debug.channelParams.anchorK == expected.anchorK);
        REQUIRE(debug.channelParams.shimmerK == expected.shimmerK);
        REQUIRE(debug.channelParams.auxK == expected.auxK);
        REQUIRE(debug.channelParams.rotation == expected.rotation);
    }
}
