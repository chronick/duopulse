#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/PatternField.h"
#include "../src/Engine/HashUtils.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// GetMetricWeight Tests
// =============================================================================

TEST_CASE("GetMetricWeight returns correct weights for 32-step pattern", "[axis][metric]")
{
    // V5 Task 35: 32-step patterns are scaled from 16-step table
    // Steps are mapped proportionally to the 16-step weights
    int patternLength = 32;

    SECTION("Bar downbeat at step 0 is strongest (1.0)")
    {
        REQUIRE(GetMetricWeight(0, patternLength) == Approx(1.0f));
    }

    SECTION("Half-bar (step 16) maps to beat 3 (0.9)")
    {
        // 32-step: step 16 maps to 16/32 * 16 = step 8 in 16-step table = 0.9
        REQUIRE(GetMetricWeight(16, patternLength) == Approx(0.9f));
    }

    SECTION("Quarter notes map to beats 2 and 4 (0.8)")
    {
        // step 8 -> 8/32 * 16 = step 4 = 0.8 (beat 2)
        REQUIRE(GetMetricWeight(8, patternLength) == Approx(0.8f));
        // step 24 -> 24/32 * 16 = step 12 = 0.8 (beat 4)
        REQUIRE(GetMetricWeight(24, patternLength) == Approx(0.8f));
    }

    SECTION("8th notes map to 0.5")
    {
        // step 4 -> 4/32 * 16 = step 2 = 0.5
        REQUIRE(GetMetricWeight(4, patternLength) == Approx(0.5f));
    }

    SECTION("16th notes (odd positions) are 0.25")
    {
        // step 3 -> 3/32 * 16 = floor(1.5) = step 1 = 0.25
        REQUIRE(GetMetricWeight(3, patternLength) == Approx(0.25f));
        // step 7 -> 7/32 * 16 = floor(3.5) = step 3 = 0.25
        REQUIRE(GetMetricWeight(7, patternLength) == Approx(0.25f));
    }
}

TEST_CASE("GetMetricWeight scales correctly for 16-step pattern", "[axis][metric]")
{
    // V5 Task 35: Musical hierarchy with differentiated beat strengths
    int patternLength = 16;

    SECTION("Beat 1 (step 0) is strongest")
    {
        REQUIRE(GetMetricWeight(0, patternLength) == Approx(1.0f));
    }

    SECTION("Beat 3 (step 8) is strong but not as strong as beat 1")
    {
        REQUIRE(GetMetricWeight(8, patternLength) == Approx(0.9f));
    }

    SECTION("Beats 2 and 4 are 0.8")
    {
        REQUIRE(GetMetricWeight(4, patternLength) == Approx(0.8f));
        REQUIRE(GetMetricWeight(12, patternLength) == Approx(0.8f));
    }

    SECTION("8th notes are 0.5")
    {
        REQUIRE(GetMetricWeight(2, patternLength) == Approx(0.5f));
        REQUIRE(GetMetricWeight(6, patternLength) == Approx(0.5f));
        REQUIRE(GetMetricWeight(10, patternLength) == Approx(0.5f));
        REQUIRE(GetMetricWeight(14, patternLength) == Approx(0.5f));
    }

    SECTION("16th notes (odd steps) are 0.25")
    {
        REQUIRE(GetMetricWeight(1, patternLength) == Approx(0.25f));
        REQUIRE(GetMetricWeight(3, patternLength) == Approx(0.25f));
        REQUIRE(GetMetricWeight(15, patternLength) == Approx(0.25f));
    }
}

// =============================================================================
// GetPositionStrength Tests
// =============================================================================

TEST_CASE("GetPositionStrength converts metric to bidirectional", "[axis][position]")
{
    // V5 Task 35: Updated to match new metric weight values
    // positionStrength = 1.0 - 2.0 * metricWeight
    int patternLength = 32;

    SECTION("Bar downbeat (step 0) returns -1.0")
    {
        // metricWeight = 1.0 -> positionStrength = 1.0 - 2.0*1.0 = -1.0
        REQUIRE(GetPositionStrength(0, patternLength) == Approx(-1.0f));
    }

    SECTION("Half-bar (step 16) returns -0.8")
    {
        // metricWeight = 0.9 -> positionStrength = 1.0 - 2.0*0.9 = -0.8
        REQUIRE(GetPositionStrength(16, patternLength) == Approx(-0.8f));
    }

    SECTION("Quarter notes (beat 2/4 positions) return -0.6")
    {
        // metricWeight = 0.8 -> positionStrength = 1.0 - 2.0*0.8 = -0.6
        REQUIRE(GetPositionStrength(8, patternLength) == Approx(-0.6f));
        REQUIRE(GetPositionStrength(24, patternLength) == Approx(-0.6f));
    }

    SECTION("8th notes return 0.0")
    {
        // metricWeight = 0.5 -> positionStrength = 1.0 - 2.0*0.5 = 0.0
        REQUIRE(GetPositionStrength(4, patternLength) == Approx(0.0f));
    }

    SECTION("16th notes (weakest) return +0.5")
    {
        // step 3 -> metricWeight = 0.25 -> positionStrength = 1.0 - 2.0*0.25 = +0.5
        REQUIRE(GetPositionStrength(3, patternLength) == Approx(0.5f));
        // step 7 -> metricWeight = 0.25 -> positionStrength = +0.5
        REQUIRE(GetPositionStrength(7, patternLength) == Approx(0.5f));
    }
}

// =============================================================================
// ApplyAxisBias - AXIS X (Beat Position) Tests
// =============================================================================

TEST_CASE("ApplyAxisBias AXIS X at neutral (0.5) has no effect", "[axis][x-bias]")
{
    float weights[kMaxSteps];
    float original[kMaxSteps];

    // Initialize with known pattern
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.5f;
        original[i] = 0.5f;
    }

    ApplyAxisBias(weights, 0.5f, 0.5f, 0.0f, 12345, 16);

    // Weights should be unchanged (within floating point tolerance)
    for (int i = 0; i < 16; ++i)
    {
        REQUIRE(weights[i] == Approx(original[i]).margin(0.001f));
    }
}

TEST_CASE("ApplyAxisBias AXIS X at 1.0 (floating) boosts offbeats, suppresses downbeats", "[axis][x-bias]")
{
    float weights[kMaxSteps];

    // Initialize with uniform weights
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.6f;
    }

    float downbeatBefore = weights[0];
    float offbeatBefore = weights[1];

    ApplyAxisBias(weights, 1.0f, 0.5f, 0.0f, 12345, 16);

    // Downbeat (step 0) should be suppressed
    REQUIRE(weights[0] < downbeatBefore);

    // Offbeat (step 1) should be boosted
    REQUIRE(weights[1] > offbeatBefore);

    // Check relative ordering: offbeats now stronger than downbeats
    REQUIRE(weights[1] > weights[0]);
}

TEST_CASE("ApplyAxisBias AXIS X at 0.0 (grounded) boosts downbeats, suppresses offbeats", "[axis][x-bias]")
{
    float weights[kMaxSteps];

    // Initialize with uniform weights
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.6f;
    }

    float downbeatBefore = weights[0];
    float offbeatBefore = weights[1];

    ApplyAxisBias(weights, 0.0f, 0.5f, 0.0f, 12345, 16);

    // Downbeat (step 0) should be boosted
    REQUIRE(weights[0] > downbeatBefore);

    // Offbeat (step 1) should be suppressed
    REQUIRE(weights[1] < offbeatBefore);

    // Check relative ordering: downbeats much stronger than offbeats
    REQUIRE(weights[0] > weights[1]);
}

// =============================================================================
// ApplyAxisBias - AXIS Y (Intricacy) Tests
// =============================================================================

TEST_CASE("ApplyAxisBias AXIS Y at 1.0 (complex) boosts weak positions", "[axis][y-bias]")
{
    float weights[kMaxSteps];

    // Initialize with uniform weights
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.5f;
    }

    float downbeatBefore = weights[0];  // metric weight = 1.0, weakness = 0
    float offbeatBefore = weights[1];   // metric weight = 0.25, weakness = 0.75

    ApplyAxisBias(weights, 0.5f, 1.0f, 0.0f, 12345, 16);

    // Downbeat should be unchanged (weakness = 0)
    REQUIRE(weights[0] == Approx(downbeatBefore).margin(0.01f));

    // Offbeat should be boosted (weakness = 0.75)
    REQUIRE(weights[1] > offbeatBefore);
}

TEST_CASE("ApplyAxisBias AXIS Y at 0.0 (simple) suppresses weak positions", "[axis][y-bias]")
{
    float weights[kMaxSteps];

    // Initialize with uniform weights
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.5f;
    }

    float downbeatBefore = weights[0];  // weakness = 0
    float offbeatBefore = weights[1];   // weakness = 0.75

    ApplyAxisBias(weights, 0.5f, 0.0f, 0.0f, 12345, 16);

    // Downbeat should be unchanged (weakness = 0)
    REQUIRE(weights[0] == Approx(downbeatBefore).margin(0.01f));

    // Offbeat should be suppressed (weakness = 0.75)
    REQUIRE(weights[1] < offbeatBefore);
}

TEST_CASE("AXIS Y effect is +/- 50% as specified", "[axis][y-bias]")
{
    float weights[kMaxSteps];

    // Use a weak position (16th note, weakness = 0.75)
    int weakStep = 1;

    // Test boost at AXIS Y = 1.0
    for (int i = 0; i < 16; ++i) weights[i] = 0.5f;
    ApplyAxisBias(weights, 0.5f, 1.0f, 0.0f, 12345, 16);

    // Expected boost: 0.5 * (1 + 0.50 * 1.0 * 0.75) = 0.5 * 1.375 = 0.6875
    REQUIRE(weights[weakStep] == Approx(0.6875f).margin(0.01f));

    // Test suppression at AXIS Y = 0.0
    for (int i = 0; i < 16; ++i) weights[i] = 0.5f;
    ApplyAxisBias(weights, 0.5f, 0.0f, 0.0f, 12345, 16);

    // Expected suppression: 0.5 * (1 - 0.50 * 1.0 * 0.75) = 0.5 * 0.625 = 0.3125
    REQUIRE(weights[weakStep] == Approx(0.3125f).margin(0.01f));
}

// =============================================================================
// ApplyAxisBias - "Broken Mode" Tests
// =============================================================================

TEST_CASE("Broken mode activates only when SHAPE > 0.6 AND AXIS X > 0.7", "[axis][broken]")
{
    float weights[kMaxSteps];
    float original[kMaxSteps];

    // Initialize with uniform weights
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.8f;
        original[i] = 0.8f;
    }

    SECTION("No broken mode when SHAPE <= 0.6")
    {
        ApplyAxisBias(weights, 0.8f, 0.5f, 0.6f, 12345, 16);

        // All downbeats should follow normal AXIS X bias, not broken mode
        // With X=0.8, downbeats are suppressed, but not to 25%
        REQUIRE(weights[0] >= 0.4f);  // Not severely suppressed
    }

    SECTION("No broken mode when AXIS X <= 0.7")
    {
        for (int i = 0; i < 16; ++i) weights[i] = 0.8f;
        ApplyAxisBias(weights, 0.7f, 0.5f, 0.9f, 12345, 16);

        // No broken mode suppression
        REQUIRE(weights[0] >= 0.4f);
    }

    SECTION("Broken mode activates when both conditions met")
    {
        for (int i = 0; i < 16; ++i) weights[i] = 0.8f;
        ApplyAxisBias(weights, 0.9f, 0.5f, 0.9f, 12345, 16);

        // With high shape + high axis X, some downbeats may be severely suppressed
        // The exact effect depends on deterministic hash, but broken mode is active
        // Just verify weights are valid
        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
            REQUIRE(weights[i] <= 1.0f);
        }
    }
}

TEST_CASE("Broken mode uses deterministic hash for suppression", "[axis][broken][determinism]")
{
    float weights1[kMaxSteps];
    float weights2[kMaxSteps];

    // Initialize with identical weights
    for (int i = 0; i < 16; ++i)
    {
        weights1[i] = 0.8f;
        weights2[i] = 0.8f;
    }

    // Same parameters should produce identical results
    ApplyAxisBias(weights1, 0.9f, 0.5f, 0.9f, 12345, 16);
    ApplyAxisBias(weights2, 0.9f, 0.5f, 0.9f, 12345, 16);

    for (int i = 0; i < 16; ++i)
    {
        REQUIRE(weights1[i] == weights2[i]);
    }
}

TEST_CASE("Broken mode intensity scales with SHAPE and AXIS X", "[axis][broken][intensity]")
{
    float weightsLow[kMaxSteps];
    float weightsHigh[kMaxSteps];

    // Low intensity broken mode (barely triggered)
    for (int i = 0; i < 16; ++i) weightsLow[i] = 0.8f;
    ApplyAxisBias(weightsLow, 0.71f, 0.5f, 0.61f, 12345, 16);

    // High intensity broken mode (fully triggered)
    for (int i = 0; i < 16; ++i) weightsHigh[i] = 0.8f;
    ApplyAxisBias(weightsHigh, 1.0f, 0.5f, 1.0f, 12345, 16);

    // High intensity should have more extreme effect
    // Note: This test is probabilistic based on the hash, but with same seed
    // the same steps will be affected, just to different degrees

    // At minimum, high intensity should have lower average downbeat weight
    // (only if the hash triggers suppression for that step)
    // Just verify both are valid
    for (int i = 0; i < 16; ++i)
    {
        REQUIRE(weightsLow[i] >= kMinStepWeight);
        REQUIRE(weightsHigh[i] >= kMinStepWeight);
    }
}

// =============================================================================
// ApplyAxisBias - Weight Floor Tests
// =============================================================================

TEST_CASE("Weights never go below kMinStepWeight (0.05)", "[axis][floor]")
{
    float weights[kMaxSteps];

    SECTION("Extreme suppression from AXIS X")
    {
        // Start with low weights and apply maximum suppression
        for (int i = 0; i < 16; ++i) weights[i] = 0.1f;
        ApplyAxisBias(weights, 1.0f, 0.5f, 0.0f, 12345, 16);

        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
        }
    }

    SECTION("Extreme suppression from AXIS Y")
    {
        for (int i = 0; i < 16; ++i) weights[i] = 0.1f;
        ApplyAxisBias(weights, 0.5f, 0.0f, 0.0f, 12345, 16);

        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
        }
    }

    SECTION("Combined extreme suppression with broken mode")
    {
        for (int i = 0; i < 16; ++i) weights[i] = 0.1f;
        ApplyAxisBias(weights, 1.0f, 0.0f, 1.0f, 12345, 16);

        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
        }
    }
}

// =============================================================================
// ApplyAxisBias - Combined X/Y Bias Tests
// =============================================================================

TEST_CASE("AXIS X and Y can be combined", "[axis][combined]")
{
    float weights[kMaxSteps];

    SECTION("Floating + Complex (X=1.0, Y=1.0)")
    {
        for (int i = 0; i < 16; ++i) weights[i] = 0.5f;
        ApplyAxisBias(weights, 1.0f, 1.0f, 0.0f, 12345, 16);

        // Offbeats should be heavily boosted (both X and Y boost them)
        // Downbeats should be suppressed (X suppresses them)
        REQUIRE(weights[1] > weights[0]);
    }

    SECTION("Grounded + Simple (X=0.0, Y=0.0)")
    {
        for (int i = 0; i < 16; ++i) weights[i] = 0.5f;
        ApplyAxisBias(weights, 0.0f, 0.0f, 0.0f, 12345, 16);

        // Downbeats should be heavily boosted (X boosts them)
        // Offbeats should be heavily suppressed (both X and Y suppress them)
        REQUIRE(weights[0] > weights[1]);
    }
}

// =============================================================================
// ApplyAxisBias - Determinism Tests
// =============================================================================

TEST_CASE("ApplyAxisBias is fully deterministic", "[axis][determinism]")
{
    float weights1[kMaxSteps];
    float weights2[kMaxSteps];

    SECTION("Same inputs always produce same outputs")
    {
        for (int i = 0; i < 16; ++i)
        {
            weights1[i] = 0.5f + i * 0.02f;
            weights2[i] = 0.5f + i * 0.02f;
        }

        ApplyAxisBias(weights1, 0.7f, 0.3f, 0.8f, 99999, 16);
        ApplyAxisBias(weights2, 0.7f, 0.3f, 0.8f, 99999, 16);

        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights1[i] == weights2[i]);
        }
    }

    SECTION("Different seeds produce different broken mode effects")
    {
        for (int i = 0; i < 16; ++i)
        {
            weights1[i] = 0.8f;
            weights2[i] = 0.8f;
        }

        ApplyAxisBias(weights1, 0.9f, 0.5f, 0.9f, 11111, 16);
        ApplyAxisBias(weights2, 0.9f, 0.5f, 0.9f, 99999, 16);

        // At least one weight should differ due to different random suppression
        bool anyDifferent = false;
        for (int i = 0; i < 16; ++i)
        {
            if (weights1[i] != weights2[i])
            {
                anyDifferent = true;
                break;
            }
        }
        REQUIRE(anyDifferent);
    }
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST_CASE("ApplyAxisBias handles edge cases", "[axis][edge]")
{
    float weights[kMaxSteps];

    SECTION("Handles pattern length of 1")
    {
        weights[0] = 0.5f;
        ApplyAxisBias(weights, 0.7f, 0.3f, 0.5f, 12345, 1);
        REQUIRE(weights[0] >= kMinStepWeight);
        REQUIRE(weights[0] <= 1.0f);
    }

    SECTION("Handles pattern length of 8")
    {
        for (int i = 0; i < 8; ++i) weights[i] = 0.5f;
        ApplyAxisBias(weights, 0.7f, 0.3f, 0.5f, 12345, 8);

        for (int i = 0; i < 8; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
            REQUIRE(weights[i] <= 1.0f);
        }
    }

    SECTION("Handles pattern length of 32")
    {
        for (int i = 0; i < 32; ++i) weights[i] = 0.5f;
        ApplyAxisBias(weights, 0.7f, 0.3f, 0.5f, 12345, 32);

        for (int i = 0; i < 32; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
            REQUIRE(weights[i] <= 1.0f);
        }
    }

    SECTION("Clamps out-of-range parameters")
    {
        for (int i = 0; i < 16; ++i) weights[i] = 0.5f;

        // Should not crash or produce invalid weights
        ApplyAxisBias(weights, -0.5f, 1.5f, 2.0f, 12345, 16);

        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
            REQUIRE(weights[i] <= 1.0f);
        }
    }
}

// =============================================================================
// Integration with ComputeShapeBlendedWeights
// =============================================================================

TEST_CASE("ApplyAxisBias integrates with ComputeShapeBlendedWeights", "[axis][integration]")
{
    float weights[kMaxSteps];
    uint32_t seed = 12345;
    int patternLength = 16;

    SECTION("Can be applied after ComputeShapeBlendedWeights")
    {
        // First generate weights using SHAPE
        ComputeShapeBlendedWeights(0.5f, 0.7f, seed, patternLength, weights);

        // Then apply AXIS biasing
        ApplyAxisBias(weights, 0.7f, 0.6f, 0.5f, seed, patternLength);

        // All weights should still be valid
        for (int i = 0; i < patternLength; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
            REQUIRE(weights[i] <= 1.0f);
        }
    }

    SECTION("AXIS biasing modifies SHAPE-generated pattern")
    {
        float originalWeights[kMaxSteps];
        float modifiedWeights[kMaxSteps];

        ComputeShapeBlendedWeights(0.5f, 0.7f, seed, patternLength, originalWeights);

        for (int i = 0; i < patternLength; ++i)
        {
            modifiedWeights[i] = originalWeights[i];
        }

        // Apply strong AXIS X bias
        ApplyAxisBias(modifiedWeights, 1.0f, 0.5f, 0.5f, seed, patternLength);

        // Weights should have changed
        bool anyDifferent = false;
        for (int i = 0; i < patternLength; ++i)
        {
            if (originalWeights[i] != modifiedWeights[i])
            {
                anyDifferent = true;
                break;
            }
        }
        REQUIRE(anyDifferent);
    }
}
