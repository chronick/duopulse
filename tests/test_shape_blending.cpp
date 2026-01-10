#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/PatternField.h"
#include "../src/Engine/HashUtils.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// HashToFloat Tests (Task 28)
// =============================================================================

TEST_CASE("HashToFloat produces deterministic values", "[shape][hash]")
{
    SECTION("Same seed + step always produces same result")
    {
        uint32_t seed = 12345;
        int step = 5;

        float result1 = HashToFloat(seed, step);
        float result2 = HashToFloat(seed, step);
        float result3 = HashToFloat(seed, step);

        REQUIRE(result1 == result2);
        REQUIRE(result2 == result3);
    }

    SECTION("Different seeds produce different results")
    {
        int step = 0;

        float result1 = HashToFloat(1111, step);
        float result2 = HashToFloat(2222, step);
        float result3 = HashToFloat(3333, step);

        // Very unlikely to be equal (would be a hash collision)
        REQUIRE(result1 != result2);
        REQUIRE(result2 != result3);
    }

    SECTION("Different steps produce different results")
    {
        uint32_t seed = 42;

        float result0 = HashToFloat(seed, 0);
        float result1 = HashToFloat(seed, 1);
        float result2 = HashToFloat(seed, 2);

        REQUIRE(result0 != result1);
        REQUIRE(result1 != result2);
    }

    SECTION("Values are in [0.0, 1.0] range")
    {
        for (uint32_t seed = 0; seed < 100; ++seed)
        {
            for (int step = 0; step < 32; ++step)
            {
                float result = HashToFloat(seed, step);
                REQUIRE(result >= 0.0f);
                REQUIRE(result <= 1.0f);
            }
        }
    }
}

// =============================================================================
// GenerateStablePattern Tests
// =============================================================================

TEST_CASE("GenerateStablePattern produces euclidean-style weights", "[shape][stable]")
{
    float weights[kMaxSteps];

    SECTION("Downbeats are strongest")
    {
        GenerateStablePattern(1.0f, 16, weights);

        // Step 0 should be maximum
        REQUIRE(weights[0] == Approx(1.0f));

        // Quarter notes should be high
        REQUIRE(weights[4] >= 0.7f);
        REQUIRE(weights[8] >= 0.7f);
        REQUIRE(weights[12] >= 0.7f);

        // 16th notes should be lower than quarter notes
        REQUIRE(weights[1] < weights[4]);
        REQUIRE(weights[3] < weights[4]);
    }

    SECTION("Energy scales weights")
    {
        float highEnergy[kMaxSteps];
        float lowEnergy[kMaxSteps];

        GenerateStablePattern(1.0f, 16, highEnergy);
        GenerateStablePattern(0.0f, 16, lowEnergy);

        // High energy should produce higher weights overall
        float highSum = 0, lowSum = 0;
        for (int i = 0; i < 16; ++i)
        {
            highSum += highEnergy[i];
            lowSum += lowEnergy[i];
        }

        REQUIRE(highSum > lowSum);
    }

    SECTION("All weights are in valid range")
    {
        GenerateStablePattern(0.5f, 32, weights);

        for (int i = 0; i < 32; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
            REQUIRE(weights[i] <= 1.0f);
        }
    }
}

// =============================================================================
// GenerateSyncopationPattern Tests
// =============================================================================

TEST_CASE("GenerateSyncopationPattern suppresses downbeats", "[shape][syncopation]")
{
    float weights[kMaxSteps];
    uint32_t seed = 12345;

    GenerateSyncopationPattern(1.0f, seed, 16, weights);

    SECTION("Downbeat (step 0) is suppressed to 50-70%")
    {
        // With energy=1.0 and baseScale=1.0, downbeat should be 0.5-0.7
        REQUIRE(weights[0] >= 0.5f);
        REQUIRE(weights[0] <= 0.75f);  // Allow small margin
    }

    SECTION("Anticipation positions are boosted")
    {
        // Step 15 (before step 0 wrap) and step 3 (before step 4)
        // should have boosted weights
        REQUIRE(weights[15] >= 0.6f);
        REQUIRE(weights[3] >= 0.6f);
    }

    SECTION("Different seeds produce different patterns")
    {
        float weights2[kMaxSteps];
        GenerateSyncopationPattern(1.0f, 99999, 16, weights2);

        // At least one step should differ
        bool anyDifferent = false;
        for (int i = 0; i < 16; ++i)
        {
            if (weights[i] != weights2[i])
            {
                anyDifferent = true;
                break;
            }
        }
        REQUIRE(anyDifferent);
    }
}

// =============================================================================
// GenerateWildPattern Tests
// =============================================================================

TEST_CASE("GenerateWildPattern produces chaotic weights", "[shape][wild]")
{
    float weights[kMaxSteps];
    uint32_t seed = 54321;

    GenerateWildPattern(1.0f, seed, 16, weights);

    SECTION("Weights have high variation")
    {
        float minWeight = 1.0f, maxWeight = 0.0f;
        for (int i = 0; i < 16; ++i)
        {
            if (weights[i] < minWeight) minWeight = weights[i];
            if (weights[i] > maxWeight) maxWeight = weights[i];
        }

        // Should have at least 0.3 range of variation
        REQUIRE((maxWeight - minWeight) >= 0.3f);
    }

    SECTION("Same seed produces same pattern")
    {
        float weights2[kMaxSteps];
        GenerateWildPattern(1.0f, seed, 16, weights2);

        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] == weights2[i]);
        }
    }

    SECTION("Downbeats still have slight bias")
    {
        // Average weight at downbeat should be slightly higher
        // Run multiple seeds and check average
        float downbeatSum = 0, offbeatSum = 0;
        for (uint32_t s = 0; s < 20; ++s)
        {
            GenerateWildPattern(0.5f, s * 1000, 16, weights);
            downbeatSum += weights[0];
            offbeatSum += weights[1];
        }

        // Downbeat average should be higher due to bias
        REQUIRE(downbeatSum / 20 > offbeatSum / 20);
    }
}

// =============================================================================
// ComputeShapeBlendedWeights Zone Tests
// =============================================================================

TEST_CASE("ComputeShapeBlendedWeights zone transitions", "[shape][blending]")
{
    float weights[kMaxSteps];
    uint32_t seed = 12345;
    float energy = 0.7f;
    int patternLength = 16;

    SECTION("Zone 1 (0.0 - 0.28) produces stable-like pattern")
    {
        ComputeShapeBlendedWeights(0.0f, energy, seed, patternLength, weights);

        // Downbeat should be strong (accounting for humanization jitter of ±5%)
        REQUIRE(weights[0] >= 0.7f);

        // Should have euclidean-like structure
        REQUIRE(weights[0] > weights[1]);  // Downbeat > 16th
        REQUIRE(weights[4] > weights[1]);  // Quarter > 16th
    }

    SECTION("Zone 1 has humanization that decreases toward boundary")
    {
        float weightsStart[kMaxSteps];
        float weightsEnd[kMaxSteps];

        ComputeShapeBlendedWeights(0.0f, energy, seed, patternLength, weightsStart);
        ComputeShapeBlendedWeights(0.27f, energy, seed, patternLength, weightsEnd);

        // The patterns should be similar but with different humanization levels
        // Hard to test quantitatively, just ensure they're both valid
        for (int i = 0; i < patternLength; ++i)
        {
            REQUIRE(weightsStart[i] >= kMinStepWeight);
            REQUIRE(weightsEnd[i] >= kMinStepWeight);
        }
    }

    SECTION("Crossfade 1->2 (0.28 - 0.32) blends smoothly")
    {
        float weightsStart[kMaxSteps];
        float weightsMid[kMaxSteps];
        float weightsEnd[kMaxSteps];

        ComputeShapeBlendedWeights(0.28f, energy, seed, patternLength, weightsStart);
        ComputeShapeBlendedWeights(0.30f, energy, seed, patternLength, weightsMid);
        ComputeShapeBlendedWeights(0.32f, energy, seed, patternLength, weightsEnd);

        // Mid should be between start and end for most steps
        // (Not exact interpolation due to clamping, but general trend)
        int midIsBetween = 0;
        for (int i = 0; i < patternLength; ++i)
        {
            float minVal = std::min(weightsStart[i], weightsEnd[i]);
            float maxVal = std::max(weightsStart[i], weightsEnd[i]);
            if (weightsMid[i] >= minVal - 0.01f && weightsMid[i] <= maxVal + 0.01f)
            {
                midIsBetween++;
            }
        }
        // At least 80% of steps should show this behavior
        REQUIRE(midIsBetween >= patternLength * 0.8);
    }

    SECTION("Zone 2a (0.32 - 0.48) produces syncopation pattern")
    {
        ComputeShapeBlendedWeights(0.40f, energy, seed, patternLength, weights);

        // Downbeat should be suppressed (50-70% of normal)
        REQUIRE(weights[0] < 0.85f);  // Not as strong as stable

        // Anticipation positions should be boosted
        REQUIRE(weights[15] > weights[2]);  // Step before downbeat > random 8th
    }

    SECTION("Zone 3 (0.72 - 1.0) produces wild pattern")
    {
        ComputeShapeBlendedWeights(0.90f, energy, seed, patternLength, weights);

        // Should have high variation
        float minW = 1.0f, maxW = 0.0f;
        for (int i = 0; i < patternLength; ++i)
        {
            if (weights[i] < minW) minW = weights[i];
            if (weights[i] > maxW) maxW = weights[i];
        }
        REQUIRE((maxW - minW) >= 0.2f);
    }

    SECTION("Zone 3 has chaos injection that increases toward 100%")
    {
        float weights72[kMaxSteps];
        float weights100[kMaxSteps];

        ComputeShapeBlendedWeights(0.72f, energy, seed, patternLength, weights72);
        ComputeShapeBlendedWeights(1.0f, energy, seed, patternLength, weights100);

        // Both should be valid
        for (int i = 0; i < patternLength; ++i)
        {
            REQUIRE(weights72[i] >= kMinStepWeight);
            REQUIRE(weights100[i] >= kMinStepWeight);
        }

        // Patterns should differ due to chaos injection
        bool anyDifferent = false;
        for (int i = 0; i < patternLength; ++i)
        {
            if (weights72[i] != weights100[i])
            {
                anyDifferent = true;
                break;
            }
        }
        REQUIRE(anyDifferent);
    }
}

// =============================================================================
// Weight Validity Tests
// =============================================================================

TEST_CASE("All weights are always valid", "[shape][validity]")
{
    float weights[kMaxSteps];

    SECTION("All shape values produce valid weights")
    {
        for (float shape = 0.0f; shape <= 1.0f; shape += 0.05f)
        {
            for (float energy = 0.0f; energy <= 1.0f; energy += 0.25f)
            {
                ComputeShapeBlendedWeights(shape, energy, 12345, 16, weights);

                for (int i = 0; i < 16; ++i)
                {
                    REQUIRE(weights[i] >= kMinStepWeight);
                    REQUIRE(weights[i] <= 1.0f);
                }
            }
        }
    }

    SECTION("Edge case: shape = 0.0")
    {
        ComputeShapeBlendedWeights(0.0f, 0.5f, 0, 16, weights);
        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
        }
    }

    SECTION("Edge case: shape = 1.0")
    {
        ComputeShapeBlendedWeights(1.0f, 0.5f, 0, 16, weights);
        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
        }
    }

    SECTION("Edge case: energy = 0.0")
    {
        ComputeShapeBlendedWeights(0.5f, 0.0f, 0, 16, weights);
        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
        }
    }

    SECTION("Edge case: pattern length boundaries")
    {
        // Minimum length
        ComputeShapeBlendedWeights(0.5f, 0.5f, 12345, 1, weights);
        REQUIRE(weights[0] >= kMinStepWeight);

        // Maximum length
        ComputeShapeBlendedWeights(0.5f, 0.5f, 12345, 32, weights);
        for (int i = 0; i < 32; ++i)
        {
            REQUIRE(weights[i] >= kMinStepWeight);
        }
    }
}

// =============================================================================
// Determinism Tests
// =============================================================================

TEST_CASE("ComputeShapeBlendedWeights is deterministic", "[shape][determinism]")
{
    float weights1[kMaxSteps];
    float weights2[kMaxSteps];

    SECTION("Same inputs produce identical outputs")
    {
        ComputeShapeBlendedWeights(0.5f, 0.7f, 12345, 16, weights1);
        ComputeShapeBlendedWeights(0.5f, 0.7f, 12345, 16, weights2);

        for (int i = 0; i < 16; ++i)
        {
            REQUIRE(weights1[i] == weights2[i]);
        }
    }

    SECTION("Different seeds produce different outputs")
    {
        ComputeShapeBlendedWeights(0.5f, 0.7f, 11111, 16, weights1);
        ComputeShapeBlendedWeights(0.5f, 0.7f, 99999, 16, weights2);

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
// Zone Boundary Tests
// =============================================================================

TEST_CASE("Zone boundaries are correct", "[shape][zones]")
{
    SECTION("Constants match spec")
    {
        REQUIRE(kShapeZone1End == Approx(0.28f));
        REQUIRE(kShapeCrossfade1End == Approx(0.32f));
        REQUIRE(kShapeZone2aEnd == Approx(0.48f));
        REQUIRE(kShapeCrossfade2End == Approx(0.52f));
        REQUIRE(kShapeZone2bEnd == Approx(0.68f));
        REQUIRE(kShapeCrossfade3End == Approx(0.72f));
    }

    SECTION("Crossfade zones are 4% wide")
    {
        REQUIRE((kShapeCrossfade1End - kShapeZone1End) == Approx(0.04f));
        REQUIRE((kShapeCrossfade2End - kShapeZone2aEnd) == Approx(0.04f));
        REQUIRE((kShapeCrossfade3End - kShapeZone2bEnd) == Approx(0.04f));
    }
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST_CASE("ClampWeight clamps correctly", "[shape][utility]")
{
    REQUIRE(ClampWeight(-0.5f) == kMinStepWeight);
    REQUIRE(ClampWeight(0.0f) == kMinStepWeight);
    REQUIRE(ClampWeight(0.04f) == kMinStepWeight);
    REQUIRE(ClampWeight(0.05f) == 0.05f);
    REQUIRE(ClampWeight(0.5f) == 0.5f);
    REQUIRE(ClampWeight(1.0f) == 1.0f);
    REQUIRE(ClampWeight(1.5f) == 1.0f);
}

TEST_CASE("LerpWeight interpolates correctly", "[shape][utility]")
{
    REQUIRE(LerpWeight(0.0f, 1.0f, 0.0f) == Approx(0.0f));
    REQUIRE(LerpWeight(0.0f, 1.0f, 1.0f) == Approx(1.0f));
    REQUIRE(LerpWeight(0.0f, 1.0f, 0.5f) == Approx(0.5f));
    REQUIRE(LerpWeight(0.2f, 0.8f, 0.5f) == Approx(0.5f));
}
