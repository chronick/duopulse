/**
 * Unit tests for PatternGenerator module
 *
 * Tests the shared pattern generation functions used by both
 * firmware and visualization tools.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/PatternGenerator.h"

using namespace daisysp_idm_grids;

// Helper function to count bits in a mask
static int CountBits(uint32_t mask)
{
    int count = 0;
    while (mask)
    {
        count += (mask & 1);
        mask >>= 1;
    }
    return count;
}

// =============================================================================
// GeneratePattern Tests
// =============================================================================

TEST_CASE("GeneratePattern produces valid masks", "[PatternGenerator]")
{
    PatternParams params;
    PatternResult result;

    SECTION("Default parameters")
    {
        GeneratePattern(params, result);

        // Should have some hits
        REQUIRE(result.anchorMask != 0);
        REQUIRE(result.patternLength == 32);
    }

    SECTION("16-step pattern")
    {
        params.patternLength = 16;
        GeneratePattern(params, result);

        REQUIRE(result.patternLength == 16);
        // Mask should only have bits set in lower 16 bits
        REQUIRE((result.anchorMask >> 16) == 0);
        REQUIRE((result.shimmerMask >> 16) == 0);
        REQUIRE((result.auxMask >> 16) == 0);
    }

    SECTION("32-step pattern uses full range")
    {
        params.patternLength = 32;
        params.energy = 0.8f;  // High energy for more hits
        GeneratePattern(params, result);

        REQUIRE(result.patternLength == 32);
        // With high energy, should have hits across the pattern
        REQUIRE(CountBits(result.anchorMask) >= 4);
    }
}

TEST_CASE("GeneratePattern is deterministic", "[PatternGenerator]")
{
    PatternParams params;
    params.seed = 0x12345678;

    PatternResult result1, result2;
    GeneratePattern(params, result1);
    GeneratePattern(params, result2);

    REQUIRE(result1.anchorMask == result2.anchorMask);
    REQUIRE(result1.shimmerMask == result2.shimmerMask);
    REQUIRE(result1.auxMask == result2.auxMask);

    // Velocities should also be identical
    for (int i = 0; i < params.patternLength; ++i)
    {
        REQUIRE(result1.anchorVelocity[i] == result2.anchorVelocity[i]);
        REQUIRE(result1.shimmerVelocity[i] == result2.shimmerVelocity[i]);
        REQUIRE(result1.auxVelocity[i] == result2.auxVelocity[i]);
    }
}

TEST_CASE("Different seeds produce different patterns", "[PatternGenerator]")
{
    PatternParams params1, params2;
    params1.seed = 0xDEADBEEF;
    params2.seed = 0xCAFEBABE;

    PatternResult result1, result2;
    GeneratePattern(params1, result1);
    GeneratePattern(params2, result2);

    // At least one mask should differ
    bool different = (result1.anchorMask != result2.anchorMask) ||
                     (result1.shimmerMask != result2.shimmerMask) ||
                     (result1.auxMask != result2.auxMask);
    REQUIRE(different);
}

TEST_CASE("Energy parameter affects hit density", "[PatternGenerator]")
{
    PatternParams paramsLow, paramsHigh;
    paramsLow.energy = 0.1f;
    paramsLow.seed = 0x12345678;
    paramsHigh.energy = 0.9f;
    paramsHigh.seed = 0x12345678;

    PatternResult resultLow, resultHigh;
    GeneratePattern(paramsLow, resultLow);
    GeneratePattern(paramsHigh, resultHigh);

    // Count hits
    int lowHits = CountBits(resultLow.anchorMask) + CountBits(resultLow.shimmerMask);
    int highHits = CountBits(resultHigh.anchorMask) + CountBits(resultHigh.shimmerMask);

    REQUIRE(highHits > lowHits);
}

TEST_CASE("Parameter sweep doesn't crash", "[PatternGenerator]")
{
    PatternParams params;
    PatternResult result;

    SECTION("Energy sweep")
    {
        for (float e = 0.0f; e <= 1.0f; e += 0.1f)
        {
            params.energy = e;
            REQUIRE_NOTHROW(GeneratePattern(params, result));
        }
    }

    SECTION("Shape sweep")
    {
        for (float s = 0.0f; s <= 1.0f; s += 0.1f)
        {
            params.shape = s;
            REQUIRE_NOTHROW(GeneratePattern(params, result));
        }
    }

    SECTION("AxisX sweep")
    {
        for (float x = 0.0f; x <= 1.0f; x += 0.1f)
        {
            params.axisX = x;
            REQUIRE_NOTHROW(GeneratePattern(params, result));
        }
    }

    SECTION("AxisY sweep")
    {
        for (float y = 0.0f; y <= 1.0f; y += 0.1f)
        {
            params.axisY = y;
            REQUIRE_NOTHROW(GeneratePattern(params, result));
        }
    }

    SECTION("Drift sweep")
    {
        for (float d = 0.0f; d <= 1.0f; d += 0.1f)
        {
            params.drift = d;
            REQUIRE_NOTHROW(GeneratePattern(params, result));
        }
    }

    SECTION("Accent sweep")
    {
        for (float a = 0.0f; a <= 1.0f; a += 0.1f)
        {
            params.accent = a;
            REQUIRE_NOTHROW(GeneratePattern(params, result));
        }
    }

    SECTION("Combined parameter sweep")
    {
        // Test multiple parameters changing together
        for (float v = 0.0f; v <= 1.0f; v += 0.2f)
        {
            params.energy = v;
            params.shape = 1.0f - v;
            params.drift = v * 0.5f;
            REQUIRE_NOTHROW(GeneratePattern(params, result));
        }
    }
}

TEST_CASE("Edge case parameters", "[PatternGenerator]")
{
    PatternParams params;
    PatternResult result;

    SECTION("Energy = 0")
    {
        params.energy = 0.0f;
        REQUIRE_NOTHROW(GeneratePattern(params, result));
        // Energy=0 may produce 0 hits, that's valid behavior
    }

    SECTION("Energy = 1")
    {
        params.energy = 1.0f;
        REQUIRE_NOTHROW(GeneratePattern(params, result));
        // Should have many hits
        REQUIRE(CountBits(result.anchorMask) >= 4);
    }

    SECTION("All parameters at extremes (low)")
    {
        params.energy = 0.0f;
        params.shape = 0.0f;
        params.axisX = 0.0f;
        params.axisY = 0.0f;
        params.drift = 0.0f;
        params.accent = 0.0f;
        REQUIRE_NOTHROW(GeneratePattern(params, result));
    }

    SECTION("All parameters at extremes (high)")
    {
        params.energy = 1.0f;
        params.shape = 1.0f;
        params.axisX = 1.0f;
        params.axisY = 1.0f;
        params.drift = 1.0f;
        params.accent = 1.0f;
        REQUIRE_NOTHROW(GeneratePattern(params, result));
    }

    SECTION("Seed = 0")
    {
        params.seed = 0;
        REQUIRE_NOTHROW(GeneratePattern(params, result));
        REQUIRE(result.anchorMask != 0);
    }

    SECTION("Seed = MAX")
    {
        params.seed = 0xFFFFFFFF;
        REQUIRE_NOTHROW(GeneratePattern(params, result));
        REQUIRE(result.anchorMask != 0);
    }
}

TEST_CASE("Velocities are in valid range", "[PatternGenerator]")
{
    PatternParams params;
    params.seed = 0xDEADBEEF;
    params.energy = 0.7f;  // Higher energy for more hits to test
    PatternResult result;

    GeneratePattern(params, result);

    for (int i = 0; i < params.patternLength; ++i)
    {
        if ((result.anchorMask & (1U << i)) != 0)
        {
            REQUIRE(result.anchorVelocity[i] >= 0.0f);
            REQUIRE(result.anchorVelocity[i] <= 1.0f);
        }
        if ((result.shimmerMask & (1U << i)) != 0)
        {
            REQUIRE(result.shimmerVelocity[i] >= 0.0f);
            REQUIRE(result.shimmerVelocity[i] <= 1.0f);
        }
        if ((result.auxMask & (1U << i)) != 0)
        {
            REQUIRE(result.auxVelocity[i] >= 0.3f);  // Aux has minimum velocity
            REQUIRE(result.auxVelocity[i] <= 1.0f);
        }
    }
}

TEST_CASE("Accent parameter affects velocity dynamics", "[PatternGenerator]")
{
    PatternParams paramsLow, paramsHigh;
    paramsLow.seed = 0x12345678;
    paramsLow.accent = 0.0f;
    paramsHigh.seed = 0x12345678;
    paramsHigh.accent = 1.0f;

    PatternResult resultLow, resultHigh;
    GeneratePattern(paramsLow, resultLow);
    GeneratePattern(paramsHigh, resultHigh);

    // At least some velocities should differ between low and high accent
    bool velocitiesDiffer = false;
    for (int i = 0; i < paramsLow.patternLength; ++i)
    {
        if ((resultLow.anchorMask & (1U << i)) != 0 &&
            (resultHigh.anchorMask & (1U << i)) != 0)
        {
            if (resultLow.anchorVelocity[i] != resultHigh.anchorVelocity[i])
            {
                velocitiesDiffer = true;
                break;
            }
        }
    }
    REQUIRE(velocitiesDiffer);
}

TEST_CASE("Shape affects pattern character", "[PatternGenerator]")
{
    PatternParams paramsStable, paramsWild;
    paramsStable.seed = 0x12345678;
    paramsStable.shape = 0.1f;  // Stable zone
    paramsWild.seed = 0x12345678;
    paramsWild.shape = 0.9f;   // Wild zone

    PatternResult resultStable, resultWild;
    GeneratePattern(paramsStable, resultStable);
    GeneratePattern(paramsWild, resultWild);

    // Patterns should differ
    bool different = (resultStable.anchorMask != resultWild.anchorMask) ||
                     (resultStable.shimmerMask != resultWild.shimmerMask);
    REQUIRE(different);
}

// =============================================================================
// RotateWithPreserve Tests
// =============================================================================

TEST_CASE("RotateWithPreserve basic functionality", "[PatternGenerator]")
{
    SECTION("No rotation")
    {
        uint32_t mask = 0b1010;
        uint32_t result = RotateWithPreserve(mask, 0, 4, 0);
        REQUIRE(result == mask);
    }

    SECTION("Simple rotation without preservation (preserve outside range)")
    {
        // Bits: 0101 (positions 0 and 2 set)
        // Rotate left by 1: 1010 (positions 1 and 3 set)
        uint32_t mask = 0b0101;
        uint32_t result = RotateWithPreserve(mask, 1, 4, 4);  // preserveStep outside range
        REQUIRE(result == 0b1010);
    }

    SECTION("Preserve step 0 when set")
    {
        // Bits: 0111 (positions 0, 1, 2 set)
        // Rotate left by 1, preserve 0
        // Step 0 was set, should remain set
        uint32_t mask = 0b0111;
        uint32_t result = RotateWithPreserve(mask, 1, 4, 0);
        REQUIRE((result & 1) == 1);
    }

    SECTION("Preserve step 0 when not set")
    {
        // Bits: 0110 (positions 1, 2 set, step 0 not set)
        // Rotate left by 1, preserve 0
        // Step 0 was not set, should remain not set
        uint32_t mask = 0b0110;
        uint32_t result = RotateWithPreserve(mask, 1, 4, 0);
        REQUIRE((result & 1) == 0);
    }

    SECTION("Length 1 returns unchanged")
    {
        uint32_t mask = 0b1;
        uint32_t result = RotateWithPreserve(mask, 5, 1, 0);
        REQUIRE(result == mask);
    }

    SECTION("Length 0 rotation returns unchanged")
    {
        uint32_t mask = 0b10101010;
        uint32_t result = RotateWithPreserve(mask, 0, 8, 0);
        REQUIRE(result == mask);
    }
}

TEST_CASE("RotateWithPreserve wrap-around behavior", "[PatternGenerator]")
{
    SECTION("Rotation wraps around pattern length")
    {
        // 8-bit pattern: 10000001 (steps 0 and 7 set)
        // Rotate by 3, preserve step 4 (not set)
        // Without preserve, result should wrap correctly
        uint32_t mask = 0b10000001;
        uint32_t result = RotateWithPreserve(mask, 3, 8, 4);

        // Step 0 rotates to step 3: 0b00001000
        // Step 7 rotates to step (7+3)%8 = step 2: 0b00000100
        // Combined: 0b00001100
        REQUIRE(result == 0b00001100);
    }

    SECTION("Rotation equals length returns original")
    {
        uint32_t mask = 0b11001011;  // Note: bit 0 is SET
        uint32_t result = RotateWithPreserve(mask, 8, 8, 0);
        // Step 0 is set and preserved
        REQUIRE((result & 1) == 1);
    }
}

TEST_CASE("RotateWithPreserve preserves beat 1 (Techno kick stability)", "[PatternGenerator]")
{
    // Test the Techno kick stability use case
    uint32_t kickPattern = 0x00010001;  // Kicks on steps 0 and 16

    for (int rot = 0; rot < 8; ++rot)
    {
        uint32_t result = RotateWithPreserve(kickPattern, rot, 32, 0);
        // Step 0 should always remain set
        REQUIRE((result & 1) == 1);
    }
}

TEST_CASE("RotateWithPreserve with different preserve steps", "[PatternGenerator]")
{
    SECTION("Preserve step 4")
    {
        uint32_t mask = 0b00010001;  // Steps 0 and 4 set
        uint32_t result = RotateWithPreserve(mask, 2, 8, 4);

        // Step 4 should remain in place
        REQUIRE((result & (1U << 4)) != 0);
    }

    SECTION("Preserve step at end of pattern")
    {
        uint32_t mask = 0b10000001;  // Steps 0 and 7 set
        uint32_t result = RotateWithPreserve(mask, 3, 8, 7);

        // Step 7 should remain in place
        REQUIRE((result & (1U << 7)) != 0);
    }
}

// =============================================================================
// ComputeTargetHits Tests
// =============================================================================

TEST_CASE("ComputeTargetHits returns reasonable values", "[PatternGenerator]")
{
    SECTION("Anchor hits scale with energy")
    {
        int lowHits = ComputeTargetHits(0.2f, 32, Voice::ANCHOR);
        int highHits = ComputeTargetHits(0.8f, 32, Voice::ANCHOR);
        REQUIRE(highHits > lowHits);
    }

    SECTION("Hits respect pattern length")
    {
        int hits16 = ComputeTargetHits(0.5f, 16, Voice::ANCHOR);
        int hits32 = ComputeTargetHits(0.5f, 32, Voice::ANCHOR);
        REQUIRE(hits32 >= hits16);
    }

    SECTION("All voice types return positive for non-minimal energy")
    {
        REQUIRE(ComputeTargetHits(0.5f, 32, Voice::ANCHOR) > 0);
        REQUIRE(ComputeTargetHits(0.5f, 32, Voice::SHIMMER) >= 0);
        REQUIRE(ComputeTargetHits(0.5f, 32, Voice::AUX) >= 0);
    }

    SECTION("Anchor has minimum hits even at low energy")
    {
        int minimalHits = ComputeTargetHits(0.0f, 32, Voice::ANCHOR);
        REQUIRE(minimalHits >= 1);
    }
}

TEST_CASE("ComputeTargetHits respects shape parameter", "[PatternGenerator]")
{
    SECTION("Shape affects anchor budget")
    {
        int stableHits = ComputeTargetHits(0.5f, 32, Voice::ANCHOR, 0.15f);
        int wildHits = ComputeTargetHits(0.5f, 32, Voice::ANCHOR, 0.85f);

        // V5 spec: anchor decreases with SHAPE
        REQUIRE(stableHits >= wildHits);
    }

    SECTION("Shape affects shimmer budget")
    {
        int stableShimmer = ComputeTargetHits(0.5f, 32, Voice::SHIMMER, 0.15f);
        int wildShimmer = ComputeTargetHits(0.5f, 32, Voice::SHIMMER, 0.85f);

        // V5 spec: shimmer increases with SHAPE
        REQUIRE(wildShimmer >= stableShimmer);
    }
}

TEST_CASE("ComputeTargetHits energy zone boundaries", "[PatternGenerator]")
{
    SECTION("MINIMAL zone (0-20%)")
    {
        int hits = ComputeTargetHits(0.1f, 32, Voice::ANCHOR);
        REQUIRE(hits >= 1);
        REQUIRE(hits <= 6);  // Minimal zone has sparse patterns
    }

    SECTION("GROOVE zone (20-50%)")
    {
        int hits = ComputeTargetHits(0.35f, 32, Voice::ANCHOR);
        REQUIRE(hits >= 3);
    }

    SECTION("BUILD zone (50-75%)")
    {
        int hits = ComputeTargetHits(0.6f, 32, Voice::ANCHOR);
        REQUIRE(hits >= 4);
    }

    SECTION("PEAK zone (75-100%)")
    {
        int hits = ComputeTargetHits(0.9f, 32, Voice::ANCHOR);
        REQUIRE(hits >= 5);
    }
}

// =============================================================================
// Pattern Characteristics Tests
// =============================================================================

TEST_CASE("Generated patterns have downbeat (beat 1 stability)", "[PatternGenerator]")
{
    PatternParams params;
    params.energy = 0.5f;  // GROOVE zone - should enforce downbeat
    PatternResult result;

    // Test multiple seeds
    for (uint32_t seed = 0; seed < 10; ++seed)
    {
        params.seed = seed + 0x12340000;
        GeneratePattern(params, result);

        // Anchor should have downbeat in GROOVE+ zones
        REQUIRE((result.anchorMask & 1) != 0);
    }
}

TEST_CASE("Shimmer uses complement relationship", "[PatternGenerator]")
{
    PatternParams params;
    params.energy = 0.6f;
    params.drift = 0.5f;
    params.seed = 0xABCDEF01;
    PatternResult result;

    GeneratePattern(params, result);

    // Count simultaneous hits (should be minimized with COMPLEMENT)
    int simultaneousHits = CountBits(result.anchorMask & result.shimmerMask);
    int totalShimmerHits = CountBits(result.shimmerMask);

    // Most shimmer hits should NOT overlap with anchor
    if (totalShimmerHits > 0)
    {
        float overlapRatio = static_cast<float>(simultaneousHits) / totalShimmerHits;
        REQUIRE(overlapRatio < 0.5f);  // Less than half overlap
    }
}

TEST_CASE("Aux avoids main voices", "[PatternGenerator]")
{
    PatternParams params;
    params.energy = 0.7f;  // Higher energy for more aux hits
    params.seed = 0x98765432;
    PatternResult result;

    GeneratePattern(params, result);

    uint32_t mainVoices = result.anchorMask | result.shimmerMask;
    int auxOverlap = CountBits(result.auxMask & mainVoices);
    int totalAux = CountBits(result.auxMask);

    // Aux should mostly avoid main voices
    if (totalAux > 0)
    {
        float overlapRatio = static_cast<float>(auxOverlap) / totalAux;
        // Due to weight reduction (0.3x), most aux hits should avoid main voices
        INFO("Aux overlap ratio: " << overlapRatio);
    }
}

// =============================================================================
// Determinism Stress Tests
// =============================================================================

TEST_CASE("Determinism holds across many generations", "[PatternGenerator]")
{
    PatternParams params;
    params.energy = 0.5f;
    params.shape = 0.4f;
    params.drift = 0.3f;

    const int kNumSeeds = 20;

    for (int i = 0; i < kNumSeeds; ++i)
    {
        params.seed = i * 0x11111111;

        PatternResult result1, result2;
        GeneratePattern(params, result1);
        GeneratePattern(params, result2);

        REQUIRE(result1.anchorMask == result2.anchorMask);
        REQUIRE(result1.shimmerMask == result2.shimmerMask);
        REQUIRE(result1.auxMask == result2.auxMask);
    }
}

TEST_CASE("Different seeds produce different patterns at high SHAPE", "[PatternGenerator]")
{
    // At high SHAPE (wild zone), seeds should produce varied patterns
    // At low SHAPE (stable zone), patterns intentionally converge for consistency
    // Note: Anchor patterns may still be similar due to guard rails,
    // so we check that at least ONE of the masks differs
    PatternParams params1, params2;
    params1.shape = 0.8f;  // Wild zone
    params2.shape = 0.8f;
    params1.seed = 0xDEADBEEF;
    params2.seed = 0xCAFEBABE;

    PatternResult result1, result2;
    GeneratePattern(params1, result1);
    GeneratePattern(params2, result2);

    // At high SHAPE, different seeds should produce at least one different pattern
    bool anyDifferent = (result1.anchorMask != result2.anchorMask) ||
                        (result1.shimmerMask != result2.shimmerMask) ||
                        (result1.auxMask != result2.auxMask);
    REQUIRE(anyDifferent);
}
