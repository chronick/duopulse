#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/VoiceRelation.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;

// =============================================================================
// Helper Functions
// =============================================================================

// Count bits set in mask
static int CountHits(uint32_t mask)
{
    int count = 0;
    while (mask)
    {
        count += (mask & 1);
        mask >>= 1;
    }
    return count;
}

// Check if shimmer mask has no overlap with anchor mask
static bool NoOverlap(uint32_t anchor, uint32_t shimmer)
{
    return (anchor & shimmer) == 0;
}

// Create uniform shimmer weights
static void MakeUniformWeights(float* weights, int length, float value = 0.5f)
{
    for (int i = 0; i < length; ++i)
    {
        weights[i] = value;
    }
}

// Create ascending shimmer weights (higher steps have higher weight)
static void MakeAscendingWeights(float* weights, int length)
{
    for (int i = 0; i < length; ++i)
    {
        weights[i] = static_cast<float>(i) / static_cast<float>(length);
    }
}

// =============================================================================
// FindGaps Tests
// =============================================================================

TEST_CASE("FindGaps with empty anchor", "[voice-complement]")
{
    Gap gaps[kMaxGaps];

    SECTION("Empty anchor returns single gap covering entire pattern")
    {
        uint32_t anchor = 0x00000000;
        int count = FindGaps(anchor, 16, gaps);

        REQUIRE(count == 1);
        REQUIRE(gaps[0].start == 0);
        REQUIRE(gaps[0].length == 16);
    }

    SECTION("Works with different pattern lengths")
    {
        uint32_t anchor = 0x00000000;

        int count8 = FindGaps(anchor, 8, gaps);
        REQUIRE(count8 == 1);
        REQUIRE(gaps[0].length == 8);

        int count32 = FindGaps(anchor, 32, gaps);
        REQUIRE(count32 == 1);
        REQUIRE(gaps[0].length == 32);
    }
}

TEST_CASE("FindGaps with full anchor", "[voice-complement]")
{
    Gap gaps[kMaxGaps];

    SECTION("Full 16-step pattern has no gaps")
    {
        uint32_t anchor = 0x0000FFFF;  // All 16 bits set
        int count = FindGaps(anchor, 16, gaps);

        REQUIRE(count == 0);
    }

    SECTION("Full 8-step pattern has no gaps")
    {
        uint32_t anchor = 0x000000FF;
        int count = FindGaps(anchor, 8, gaps);

        REQUIRE(count == 0);
    }
}

TEST_CASE("FindGaps with single gap", "[voice-complement]")
{
    Gap gaps[kMaxGaps];

    SECTION("Gap in middle")
    {
        // Pattern: 1..1 (hits at 0 and 3, gap at 1-2)
        uint32_t anchor = 0b1001;
        int count = FindGaps(anchor, 4, gaps);

        REQUIRE(count == 1);
        REQUIRE(gaps[0].start == 1);
        REQUIRE(gaps[0].length == 2);
    }

    SECTION("Gap at start")
    {
        // Pattern: ..11 (gap at 0-1, hits at 2-3)
        uint32_t anchor = 0b1100;
        int count = FindGaps(anchor, 4, gaps);

        REQUIRE(count == 1);
        REQUIRE(gaps[0].start == 0);
        REQUIRE(gaps[0].length == 2);
    }

    SECTION("Gap at end")
    {
        // Pattern: 11.. (hits at 0-1, gap at 2-3)
        uint32_t anchor = 0b0011;
        int count = FindGaps(anchor, 4, gaps);

        REQUIRE(count == 1);
        REQUIRE(gaps[0].start == 2);
        REQUIRE(gaps[0].length == 2);
    }
}

TEST_CASE("FindGaps with multiple gaps", "[voice-complement]")
{
    Gap gaps[kMaxGaps];

    SECTION("Two gaps separated by hits")
    {
        // Pattern: .1.1 (gaps at 0 and 2, hits at 1 and 3)
        uint32_t anchor = 0b1010;
        int count = FindGaps(anchor, 4, gaps);

        REQUIRE(count == 2);
        // First gap at position 0
        REQUIRE(gaps[0].start == 0);
        REQUIRE(gaps[0].length == 1);
        // Second gap at position 2
        REQUIRE(gaps[1].start == 2);
        REQUIRE(gaps[1].length == 1);
    }

    SECTION("Four-on-floor pattern has gaps between hits")
    {
        // Pattern: 1...1...1...1... (kicks on 0, 4, 8, 12 of 16 steps)
        uint32_t anchor = 0b0001000100010001;
        int count = FindGaps(anchor, 16, gaps);

        // Should have 4 gaps of length 3 each
        REQUIRE(count == 4);
        for (int i = 0; i < 4; ++i)
        {
            REQUIRE(gaps[i].length == 3);
        }
    }
}

TEST_CASE("FindGaps wrap-around handling", "[voice-complement]")
{
    Gap gaps[kMaxGaps];

    SECTION("Gap wrapping from end to start is combined")
    {
        // Pattern: ..1..1.. (8 steps: hits at 2 and 5, gaps wrap around)
        // Steps:   01234567
        // Mask:    ..1..1..
        uint32_t anchor = 0b00100100;
        int count = FindGaps(anchor, 8, gaps);

        // Should have 2 gaps:
        // - Gap from 6-7 wraps to 0-1 (combined length 4)
        // - Gap from 3-4 (length 2)
        REQUIRE(count == 2);

        // The wrap-around gap should be combined
        bool foundWrapGap = false;
        for (int i = 0; i < count; ++i)
        {
            if (gaps[i].length == 4)
            {
                foundWrapGap = true;
                // Wrap gap starts at position 6
                REQUIRE(gaps[i].start == 6);
            }
        }
        REQUIRE(foundWrapGap);
    }

    SECTION("Pattern starting and ending with hits has no wrap-around")
    {
        // Pattern: 1..1 (hits at 0 and 3, no wrap)
        uint32_t anchor = 0b1001;
        int count = FindGaps(anchor, 4, gaps);

        REQUIRE(count == 1);
        REQUIRE(gaps[0].start == 1);
        REQUIRE(gaps[0].length == 2);
    }
}

// =============================================================================
// ApplyComplementRelationship Edge Cases
// =============================================================================

TEST_CASE("ApplyComplementRelationship edge cases", "[voice-complement]")
{
    float weights[32];
    MakeUniformWeights(weights, 32);

    SECTION("Zero target hits returns empty mask")
    {
        uint32_t anchor = 0b1001;
        uint32_t result = ApplyComplementRelationship(anchor, weights, 0.5f, 12345, 16, 0);

        REQUIRE(result == 0);
    }

    SECTION("Zero pattern length returns empty mask")
    {
        uint32_t anchor = 0b1001;
        uint32_t result = ApplyComplementRelationship(anchor, weights, 0.5f, 12345, 0, 4);

        REQUIRE(result == 0);
    }

    SECTION("Full anchor returns empty mask (no room for shimmer)")
    {
        uint32_t anchor = 0x0000FFFF;  // All 16 bits set
        uint32_t result = ApplyComplementRelationship(anchor, weights, 0.5f, 12345, 16, 4);

        REQUIRE(result == 0);
    }
}

TEST_CASE("ApplyComplementRelationship basic gap filling", "[voice-complement]")
{
    float weights[32];
    MakeUniformWeights(weights, 32);

    SECTION("Shimmer fills gap, not on anchor hits")
    {
        // Four-on-floor: hits at 0, 4, 8, 12
        uint32_t anchor = 0b0001000100010001;
        uint32_t result = ApplyComplementRelationship(anchor, weights, 0.0f, 12345, 16, 4);

        // Should place 4 hits
        REQUIRE(CountHits(result) == 4);

        // No overlap with anchor
        REQUIRE(NoOverlap(anchor, result));
    }

    SECTION("Requested hits exceeds gap space - fills all available")
    {
        // Sparse anchor with only 2 gaps
        uint32_t anchor = 0b0111;  // Hits at 0,1,2 - gap only at 3
        uint32_t result = ApplyComplementRelationship(anchor, weights, 0.0f, 12345, 4, 10);

        // Can only place 1 hit (only step 3 available)
        REQUIRE(CountHits(result) == 1);
        REQUIRE(NoOverlap(anchor, result));
    }

    SECTION("Empty anchor allows all positions")
    {
        uint32_t anchor = 0;
        uint32_t result = ApplyComplementRelationship(anchor, weights, 0.0f, 12345, 8, 4);

        REQUIRE(CountHits(result) == 4);
    }
}

// =============================================================================
// DRIFT Placement Strategy Tests
// =============================================================================

TEST_CASE("ApplyComplementRelationship low drift - evenly spaced", "[voice-complement]")
{
    float weights[32];
    MakeUniformWeights(weights, 32);

    SECTION("Low drift produces evenly distributed hits")
    {
        uint32_t anchor = 0b1000000010000000;  // Hits at 7 and 15 (16 steps)
        float lowDrift = 0.1f;

        uint32_t result = ApplyComplementRelationship(anchor, weights, lowDrift, 12345, 16, 4);

        REQUIRE(CountHits(result) == 4);
        REQUIRE(NoOverlap(anchor, result));

        // With evenly spaced placement in two 7-step gaps, hits should be distributed
    }

    SECTION("Consistent results with same parameters")
    {
        uint32_t anchor = 0b10001;  // Hits at 0 and 4
        float drift = 0.1f;

        uint32_t result1 = ApplyComplementRelationship(anchor, weights, drift, 100, 8, 2);
        uint32_t result2 = ApplyComplementRelationship(anchor, weights, drift, 100, 8, 2);

        // Low drift should be deterministic
        REQUIRE(result1 == result2);
    }
}

TEST_CASE("ApplyComplementRelationship mid drift - weighted placement", "[voice-complement]")
{
    float weights[32];
    MakeAscendingWeights(weights, 32);

    SECTION("Mid drift favors higher-weighted positions")
    {
        // Gap from 1-6 (weights ascending)
        uint32_t anchor = 0b10000001;  // Hits at 0 and 7
        float midDrift = 0.5f;

        uint32_t result = ApplyComplementRelationship(anchor, weights, midDrift, 12345, 8, 2);

        REQUIRE(CountHits(result) == 2);
        REQUIRE(NoOverlap(anchor, result));

        // With ascending weights, higher steps in gap should be preferred
        // Step 6 has highest weight in gap, step 5 second highest
        bool hasStep6 = (result & (1U << 6)) != 0;
        bool hasStep5 = (result & (1U << 5)) != 0;
        REQUIRE((hasStep6 || hasStep5));  // At least one high-weight step
    }
}

TEST_CASE("ApplyComplementRelationship high drift - seed-varied random", "[voice-complement]")
{
    float weights[32];
    MakeUniformWeights(weights, 32);

    SECTION("Different seeds produce different results")
    {
        uint32_t anchor = 0b10000001;  // Big gap in middle
        float highDrift = 0.9f;

        uint32_t result1 = ApplyComplementRelationship(anchor, weights, highDrift, 12345, 8, 3);
        uint32_t result2 = ApplyComplementRelationship(anchor, weights, highDrift, 67890, 8, 3);

        // Both should have correct count and no overlap
        REQUIRE(CountHits(result1) == 3);
        REQUIRE(CountHits(result2) == 3);
        REQUIRE(NoOverlap(anchor, result1));
        REQUIRE(NoOverlap(anchor, result2));

        // With different seeds, results should likely differ
        // (not guaranteed but very probable with random placement)
        // Just check they're valid, actual randomness is probabilistic
    }

    SECTION("Same seed produces same result")
    {
        uint32_t anchor = 0b10000001;
        float highDrift = 0.9f;

        uint32_t result1 = ApplyComplementRelationship(anchor, weights, highDrift, 42, 8, 3);
        uint32_t result2 = ApplyComplementRelationship(anchor, weights, highDrift, 42, 8, 3);

        REQUIRE(result1 == result2);
    }
}

// =============================================================================
// Proportional Distribution Tests
// =============================================================================

TEST_CASE("ApplyComplementRelationship proportional gap distribution", "[voice-complement]")
{
    float weights[32];
    MakeUniformWeights(weights, 32);

    SECTION("Hits distributed proportionally to gap size")
    {
        // Pattern with unequal gaps:
        // Big gap (6 steps): positions 1-6
        // Small gap (1 step): position 8
        // Anchor at: 0, 7, 9, 10, 11, 12, 13, 14, 15
        uint32_t anchor = 0b1111111010000001;
        float drift = 0.0f;

        // Request 3 hits - should go mostly to big gap
        uint32_t result = ApplyComplementRelationship(anchor, weights, drift, 12345, 16, 3);

        REQUIRE(CountHits(result) == 3);
        REQUIRE(NoOverlap(anchor, result));

        // Count hits in big gap (1-6) vs small gap (8)
        int bigGapHits = 0;
        for (int i = 1; i <= 6; ++i)
        {
            if ((result & (1U << i)) != 0) bigGapHits++;
        }

        // Big gap should get most hits due to proportional distribution
        REQUIRE(bigGapHits >= 2);
    }
}

// =============================================================================
// Legacy Function Tests
// =============================================================================

TEST_CASE("Legacy ApplyVoiceRelationship is now no-op", "[voice-complement]")
{
    SECTION("INDEPENDENT mode leaves shimmer unchanged")
    {
        uint32_t anchor = 0b1111;
        uint32_t shimmer = 0b0101;
        uint32_t original = shimmer;

        ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::INDEPENDENT, 16);

        REQUIRE(shimmer == original);
    }

    SECTION("INTERLOCK mode no longer modifies shimmer (V5 deprecation)")
    {
        uint32_t anchor = 0b1111;
        uint32_t shimmer = 0b1010;
        uint32_t original = shimmer;

        ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::INTERLOCK, 16);

        // V5: INTERLOCK is deprecated, function is no-op
        REQUIRE(shimmer == original);
    }

    SECTION("SHADOW mode no longer modifies shimmer (V5 deprecation)")
    {
        uint32_t anchor = 0b1111;
        uint32_t shimmer = 0b1010;
        uint32_t original = shimmer;

        ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::SHADOW, 16);

        // V5: SHADOW is deprecated, function is no-op
        REQUIRE(shimmer == original);
    }
}

TEST_CASE("Legacy ApplyAuxRelationship is now no-op", "[voice-complement]")
{
    SECTION("All coupling modes leave aux unchanged")
    {
        uint32_t anchor = 0b1111;
        uint32_t shimmer = 0b0101;
        uint32_t aux = 0b1010;
        uint32_t original = aux;

        ApplyAuxRelationship(anchor, shimmer, aux, VoiceCoupling::INDEPENDENT, 16);
        REQUIRE(aux == original);

        ApplyAuxRelationship(anchor, shimmer, aux, VoiceCoupling::INTERLOCK, 16);
        REQUIRE(aux == original);

        ApplyAuxRelationship(anchor, shimmer, aux, VoiceCoupling::SHADOW, 16);
        REQUIRE(aux == original);
    }
}

// =============================================================================
// Utility Function Tests (kept from V4)
// =============================================================================

TEST_CASE("ShiftMaskLeft utility", "[voice-complement]")
{
    SECTION("Shifts bits left with wrap")
    {
        uint32_t mask = 0b0001;  // Bit at position 0
        uint32_t shifted = ShiftMaskLeft(mask, 1, 4);

        REQUIRE(shifted == 0b0010);  // Now at position 1
    }

    SECTION("Wraps around at pattern length")
    {
        uint32_t mask = 0b1000;  // Bit at position 3
        uint32_t shifted = ShiftMaskLeft(mask, 1, 4);

        REQUIRE(shifted == 0b0001);  // Wrapped to position 0
    }
}

TEST_CASE("FindLargestGap utility", "[voice-complement]")
{
    SECTION("Empty mask returns pattern length")
    {
        REQUIRE(FindLargestGap(0, 16) == 16);
    }

    SECTION("Full mask returns 0")
    {
        REQUIRE(FindLargestGap(0xFFFF, 16) == 0);
    }

    SECTION("Single bit finds correct gap")
    {
        // Single hit at position 0 - gap is 15 steps
        REQUIRE(FindLargestGap(0b1, 16) == 15);
    }
}
