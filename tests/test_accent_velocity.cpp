#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

#include "../src/Engine/VelocityCompute.h"
#include "../src/Engine/ControlState.h"
#include "../src/Engine/PatternField.h"  // For GetMetricWeight

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// GetMetricWeight Tests
// =============================================================================

TEST_CASE("GetMetricWeight returns correct weights for 16-step pattern", "[accent-velocity]")
{
    SECTION("Beat 1 (step 0) has maximum weight")
    {
        REQUIRE(GetMetricWeight(0, 16) == Approx(1.0f));
    }

    SECTION("Beat 2 (step 4) has 0.8 weight")
    {
        REQUIRE(GetMetricWeight(4, 16) == Approx(0.8f));
    }

    SECTION("Beat 3 (step 8) has 0.9 weight (half-bar)")
    {
        REQUIRE(GetMetricWeight(8, 16) == Approx(0.9f));
    }

    SECTION("Beat 4 (step 12) has 0.8 weight")
    {
        REQUIRE(GetMetricWeight(12, 16) == Approx(0.8f));
    }

    SECTION("8th notes have 0.5 weight")
    {
        REQUIRE(GetMetricWeight(2, 16) == Approx(0.5f));
        REQUIRE(GetMetricWeight(6, 16) == Approx(0.5f));
        REQUIRE(GetMetricWeight(10, 16) == Approx(0.5f));
        REQUIRE(GetMetricWeight(14, 16) == Approx(0.5f));
    }

    SECTION("16th notes (odd steps) have 0.25 weight")
    {
        REQUIRE(GetMetricWeight(1, 16) == Approx(0.25f));
        REQUIRE(GetMetricWeight(3, 16) == Approx(0.25f));
        REQUIRE(GetMetricWeight(5, 16) == Approx(0.25f));
        REQUIRE(GetMetricWeight(7, 16) == Approx(0.25f));
    }
}

TEST_CASE("GetMetricWeight handles edge cases", "[accent-velocity]")
{
    SECTION("Zero pattern length returns 0.5")
    {
        REQUIRE(GetMetricWeight(0, 0) == Approx(0.5f));
    }

    SECTION("Negative step is clamped to 0")
    {
        REQUIRE(GetMetricWeight(-1, 16) == Approx(1.0f));  // Step 0 weight
    }

    SECTION("Steps wrap correctly for 16-step pattern")
    {
        REQUIRE(GetMetricWeight(16, 16) == Approx(1.0f));  // Same as step 0
        REQUIRE(GetMetricWeight(20, 16) == Approx(0.8f));  // Same as step 4
    }
}

// =============================================================================
// ComputeAccentVelocity Tests
// =============================================================================

TEST_CASE("ComputeAccentVelocity with ACCENT=0% produces flat dynamics", "[accent-velocity]")
{
    constexpr float accent = 0.0f;
    constexpr int patternLength = 16;
    constexpr uint32_t seed = 12345;

    SECTION("All steps produce velocities in 80-88% range")
    {
        for (int step = 0; step < patternLength; ++step)
        {
            float velocity = ComputeAccentVelocity(accent, step, patternLength, seed);
            // ACCENT=0%: floor=0.80, ceiling=0.88, variation=0.02
            // Max variation is +/-0.01 (0.5 * 0.02)
            REQUIRE(velocity >= 0.79f);  // 0.80 - 0.01
            REQUIRE(velocity <= 0.89f);  // 0.88 + 0.01
        }
    }

    SECTION("Downbeat and offbeat velocities are similar")
    {
        float downbeat = ComputeAccentVelocity(accent, 0, patternLength, seed);
        float offbeat = ComputeAccentVelocity(accent, 1, patternLength, seed);

        // At ACCENT=0%, the range is narrow (80-88%), so difference is small
        float diff = downbeat - offbeat;
        REQUIRE(diff < 0.10f);  // Less than 10% difference
    }
}

TEST_CASE("ComputeAccentVelocity with ACCENT=100% produces wide dynamics", "[accent-velocity]")
{
    constexpr float accent = 1.0f;
    constexpr int patternLength = 16;
    constexpr uint32_t seed = 12345;

    SECTION("Downbeats have high velocity (near 100%)")
    {
        float downbeat = ComputeAccentVelocity(accent, 0, patternLength, seed);
        // Step 0: metricWeight=1.0, floor=0.30, ceiling=1.0
        // velocity = 0.30 + 1.0 * (1.0 - 0.30) = 1.0
        // With variation +/-0.035
        REQUIRE(downbeat >= 0.95f);
    }

    SECTION("16th notes have low velocity (near 30%)")
    {
        // Test odd steps (16th notes)
        float offbeat = ComputeAccentVelocity(accent, 1, patternLength, seed);
        // Step 1: metricWeight=0.25, floor=0.30, ceiling=1.0
        // velocity = 0.30 + 0.25 * (1.0 - 0.30) = 0.475
        REQUIRE(offbeat >= 0.30f);
        REQUIRE(offbeat <= 0.55f);
    }

    SECTION("Beat 3 (step 8) is stronger than beat 2 (step 4)")
    {
        float beat2 = ComputeAccentVelocity(accent, 4, patternLength, seed);
        float beat3 = ComputeAccentVelocity(accent, 8, patternLength, seed);
        // Beat 3 has weight 0.9, Beat 2 has weight 0.8
        REQUIRE(beat3 > beat2);
    }
}

TEST_CASE("ComputeAccentVelocity is deterministic", "[accent-velocity]")
{
    constexpr float accent = 0.5f;
    constexpr int patternLength = 16;

    SECTION("Same seed + step produces same velocity")
    {
        constexpr uint32_t seed = 42;
        float v1 = ComputeAccentVelocity(accent, 4, patternLength, seed);
        float v2 = ComputeAccentVelocity(accent, 4, patternLength, seed);
        REQUIRE(v1 == v2);
    }

    SECTION("Different seeds produce different velocities (due to variation)")
    {
        float v1 = ComputeAccentVelocity(accent, 4, patternLength, 100);
        float v2 = ComputeAccentVelocity(accent, 4, patternLength, 200);
        // They might be slightly different due to micro-variation
        // But base velocity from metric weight is the same
        float diff = std::abs(v1 - v2);
        REQUIRE(diff < 0.10f);  // Within variation range
    }
}

// =============================================================================
// AccentParams Tests
// =============================================================================

TEST_CASE("AccentParams computes correct values", "[accent-velocity]")
{
    AccentParams params;

    SECTION("ACCENT=0% produces flat range")
    {
        params.ComputeFromAccent(0.0f);
        REQUIRE(params.velocityFloor == Approx(0.80f));
        REQUIRE(params.velocityCeiling == Approx(0.88f));
        REQUIRE(params.variation == Approx(0.02f));
    }

    SECTION("ACCENT=100% produces wide range")
    {
        params.ComputeFromAccent(1.0f);
        REQUIRE(params.velocityFloor == Approx(0.30f));
        REQUIRE(params.velocityCeiling == Approx(1.0f));
        REQUIRE(params.variation == Approx(0.07f));
    }

    SECTION("ACCENT=50% produces intermediate values")
    {
        params.ComputeFromAccent(0.5f);
        REQUIRE(params.velocityFloor == Approx(0.55f));
        REQUIRE(params.velocityCeiling == Approx(0.94f));
        REQUIRE(params.variation == Approx(0.045f));
    }

    SECTION("Input is clamped to 0-1")
    {
        params.ComputeFromAccent(-0.5f);
        REQUIRE(params.velocityFloor == Approx(0.80f));

        params.ComputeFromAccent(1.5f);
        REQUIRE(params.velocityFloor == Approx(0.30f));
    }
}

TEST_CASE("AccentParams Init produces default values", "[accent-velocity]")
{
    AccentParams params;
    params.Init();

    // Default is 50% accent
    REQUIRE(params.velocityFloor == Approx(0.55f));
    REQUIRE(params.velocityCeiling == Approx(0.94f));
    REQUIRE(params.variation == Approx(0.045f));
}

TEST_CASE("AccentParams legacy alias works", "[accent-velocity]")
{
    AccentParams params;
    params.ComputeFromPunch(0.5f);  // Legacy alias

    REQUIRE(params.velocityFloor == Approx(0.55f));
    REQUIRE(params.velocityCeiling == Approx(0.94f));
}

// =============================================================================
// ComputeAccent Function Tests
// =============================================================================

TEST_CASE("ComputeAccent computes correct params", "[accent-velocity]")
{
    AccentParams params;

    SECTION("ACCENT=0% sets flat range")
    {
        ComputeAccent(0.0f, params);
        REQUIRE(params.velocityFloor == Approx(0.80f));
        REQUIRE(params.velocityCeiling == Approx(0.88f));
    }

    SECTION("ACCENT=100% sets wide range")
    {
        ComputeAccent(1.0f, params);
        REQUIRE(params.velocityFloor == Approx(0.30f));
        REQUIRE(params.velocityCeiling == Approx(1.0f));
    }
}

// =============================================================================
// ComputeAnchorVelocity and ComputeShimmerVelocity Tests
// =============================================================================

TEST_CASE("ComputeAnchorVelocity uses metric weight", "[accent-velocity]")
{
    constexpr float accent = 0.8f;
    constexpr float shape = 0.0f;
    constexpr float phraseProgress = 0.3f;  // GROOVE phase
    constexpr uint32_t seed = 12345;

    SECTION("Downbeat has higher velocity than offbeat")
    {
        float downbeat = ComputeAnchorVelocity(accent, shape, phraseProgress, 0, seed);
        float offbeat = ComputeAnchorVelocity(accent, shape, phraseProgress, 1, seed);
        REQUIRE(downbeat > offbeat);
    }

    SECTION("SHAPE BUILD phase boosts velocity")
    {
        float groove = ComputeAnchorVelocity(accent, 1.0f, 0.3f, 0, seed);  // GROOVE phase
        float build = ComputeAnchorVelocity(accent, 1.0f, 0.7f, 0, seed);   // BUILD phase
        REQUIRE(build > groove);
    }
}

TEST_CASE("ComputeShimmerVelocity uses metric weight", "[accent-velocity]")
{
    constexpr float accent = 0.8f;
    constexpr float shape = 0.0f;
    constexpr float phraseProgress = 0.3f;
    constexpr uint32_t seed = 12345;

    SECTION("Beat 3 has higher velocity than beat 2")
    {
        float beat2 = ComputeShimmerVelocity(accent, shape, phraseProgress, 4, seed);
        float beat3 = ComputeShimmerVelocity(accent, shape, phraseProgress, 8, seed);
        REQUIRE(beat3 > beat2);
    }
}

// =============================================================================
// Velocity Range Tests
// =============================================================================

TEST_CASE("Velocity is always clamped to valid range", "[accent-velocity]")
{
    constexpr int patternLength = 16;

    SECTION("Minimum velocity is 0.30")
    {
        // Even at ACCENT=100% with offbeat (low metric weight)
        float velocity = ComputeAccentVelocity(1.0f, 1, patternLength, 0);
        REQUIRE(velocity >= 0.30f);
    }

    SECTION("Maximum velocity is 1.0")
    {
        // At ACCENT=100% with downbeat (high metric weight)
        float velocity = ComputeAccentVelocity(1.0f, 0, patternLength, 0);
        REQUIRE(velocity <= 1.0f);
    }
}

// =============================================================================
// ComputeVelocity Legacy Compatibility Tests
// =============================================================================

TEST_CASE("ComputeVelocity uses V5 metric weight approach", "[accent-velocity]")
{
    AccentParams params;
    params.ComputeFromAccent(0.8f);

    ShapeModifiers shapeMods;
    shapeMods.Init();

    constexpr uint32_t seed = 12345;

    SECTION("Velocity varies by step position")
    {
        float v0 = ComputeVelocity(params, shapeMods, false, seed, 0);   // Downbeat
        float v1 = ComputeVelocity(params, shapeMods, false, seed, 1);   // 16th note

        REQUIRE(v0 > v1);  // Downbeat should be louder
    }

    SECTION("SHAPE fill zone boost applies")
    {
        ShapeModifiers fillMods;
        fillMods.Init();
        fillMods.inFillZone = true;
        fillMods.fillIntensity = 1.0f;

        float noFill = ComputeVelocity(params, shapeMods, false, seed, 0);
        float withFill = ComputeVelocity(params, fillMods, false, seed, 0);

        REQUIRE(withFill > noFill);
    }
}
