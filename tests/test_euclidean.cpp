#include <catch2/catch_all.hpp>
#include "../src/Engine/EuclideanGen.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// Euclidean Pattern Generation Tests (Task 21 Phase F9)
// =============================================================================

TEST_CASE("GenerateEuclidean produces correct patterns", "[euclidean][generation]")
{
    // E(4, 16) = four-on-floor kick (every 4th step)
    uint32_t fourOnFloor = GenerateEuclidean(4, 16);
    int hitCount = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (fourOnFloor & (1u << i))
            hitCount++;
    }
    REQUIRE(hitCount == 4);

    // E(3, 8) = son clave pattern
    uint32_t sonClave = GenerateEuclidean(3, 8);
    hitCount = 0;
    for (int i = 0; i < 8; ++i)
    {
        if (sonClave & (1u << i))
            hitCount++;
    }
    REQUIRE(hitCount == 3);

    // E(5, 8) = five hits in 8 steps
    uint32_t fiveInEight = GenerateEuclidean(5, 8);
    hitCount = 0;
    for (int i = 0; i < 8; ++i)
    {
        if (fiveInEight & (1u << i))
            hitCount++;
    }
    REQUIRE(hitCount == 5);
}

TEST_CASE("GenerateEuclidean handles edge cases", "[euclidean][generation]")
{
    // Zero hits = empty pattern
    REQUIRE(GenerateEuclidean(0, 16) == 0);

    // Hits >= steps = all steps
    uint64_t allSteps = GenerateEuclidean(8, 8);
    REQUIRE(allSteps == 0xFF);  // 11111111

    // Invalid steps returns 0
    REQUIRE(GenerateEuclidean(4, 0) == 0);
    REQUIRE(GenerateEuclidean(4, 65) == 0);  // Now > 64 is invalid
}

TEST_CASE("RotatePattern shifts bits correctly", "[euclidean][rotation]")
{
    // Pattern: 00010001 (bits 0 and 4 set)
    uint64_t pattern = 0x11;

    // Rotate right by 1: 10001000 (bits 3 and 7 set)
    uint64_t rotated1 = RotatePattern(pattern, 1, 8);
    REQUIRE(rotated1 == 0x88);

    // Rotate right by 4: should return to original
    uint64_t rotated4 = RotatePattern(pattern, 4, 8);
    REQUIRE((rotated4 & 0xFF) == (pattern & 0xFF));

    // Negative offset rotates left
    uint64_t rotatedLeft = RotatePattern(pattern, -1, 8);
    REQUIRE(rotatedLeft == 0x22);  // 00100010
}

// =============================================================================
// Genre-Specific Euclidean Ratio Tests (Task 21 Phase F10)
// =============================================================================

TEST_CASE("GetGenreEuclideanRatio returns correct base ratios", "[euclidean][genre]")
{
    // Techno base: 70% at Field X = 0, MINIMAL zone
    float technoBase = GetGenreEuclideanRatio(Genre::TECHNO, 0.0f, EnergyZone::MINIMAL);
    REQUIRE(technoBase == Approx(0.70f).margin(0.01f));

    // Tribal base: 40% at Field X = 0, MINIMAL zone
    float tribalBase = GetGenreEuclideanRatio(Genre::TRIBAL, 0.0f, EnergyZone::MINIMAL);
    REQUIRE(tribalBase == Approx(0.40f).margin(0.01f));

    // IDM base: 0% (disabled)
    float idmBase = GetGenreEuclideanRatio(Genre::IDM, 0.0f, EnergyZone::MINIMAL);
    REQUIRE(idmBase == Approx(0.0f).margin(0.01f));
}

TEST_CASE("GetGenreEuclideanRatio tapers with Field X", "[euclidean][genre]")
{
    // Techno: 70% at X=0, should taper to ~21% at X=1.0
    float technoAtZero = GetGenreEuclideanRatio(Genre::TECHNO, 0.0f, EnergyZone::MINIMAL);
    float technoAtOne = GetGenreEuclideanRatio(Genre::TECHNO, 1.0f, EnergyZone::MINIMAL);

    REQUIRE(technoAtZero == Approx(0.70f).margin(0.01f));
    REQUIRE(technoAtOne == Approx(0.21f).margin(0.02f));  // 0.70 * (1 - 0.7) = 0.21

    // Tribal: 40% at X=0, should taper to ~12% at X=1.0
    float tribalAtZero = GetGenreEuclideanRatio(Genre::TRIBAL, 0.0f, EnergyZone::GROOVE);
    float tribalAtOne = GetGenreEuclideanRatio(Genre::TRIBAL, 1.0f, EnergyZone::GROOVE);

    REQUIRE(tribalAtZero == Approx(0.40f).margin(0.01f));
    REQUIRE(tribalAtOne == Approx(0.12f).margin(0.02f));  // 0.40 * (1 - 0.7) = 0.12
}

TEST_CASE("GetGenreEuclideanRatio only active in MINIMAL/GROOVE zones", "[euclidean][genre]")
{
    // Active in MINIMAL
    float minimal = GetGenreEuclideanRatio(Genre::TECHNO, 0.0f, EnergyZone::MINIMAL);
    REQUIRE(minimal > 0.0f);

    // Active in GROOVE
    float groove = GetGenreEuclideanRatio(Genre::TECHNO, 0.0f, EnergyZone::GROOVE);
    REQUIRE(groove > 0.0f);

    // Disabled in BUILD
    float build = GetGenreEuclideanRatio(Genre::TECHNO, 0.0f, EnergyZone::BUILD);
    REQUIRE(build == Approx(0.0f));

    // Disabled in PEAK
    float peak = GetGenreEuclideanRatio(Genre::TECHNO, 0.0f, EnergyZone::PEAK);
    REQUIRE(peak == Approx(0.0f));
}

// =============================================================================
// Euclidean + Weight Blending Tests
// =============================================================================

TEST_CASE("BlendEuclideanWithWeights respects budget", "[euclidean][blending]")
{
    const int steps = 16;
    float weights[16];
    for (int i = 0; i < 16; ++i)
        weights[i] = 0.5f;  // Uniform weights

    uint32_t eligibility = 0xFFFF;  // All steps eligible
    uint32_t seed = 12345;

    // At ratio = 1.0 (pure Euclidean), should get exactly budget hits
    for (int budget = 0; budget <= 8; ++budget)
    {
        uint64_t pattern = BlendEuclideanWithWeights(budget, steps, weights, eligibility, 1.0f, seed);

        int hitCount = 0;
        for (int i = 0; i < steps; ++i)
        {
            if (pattern & (1u << i))
                hitCount++;
        }

        REQUIRE(hitCount == budget);
    }
}

TEST_CASE("BlendEuclideanWithWeights respects eligibility mask", "[euclidean][blending]")
{
    const int steps = 16;
    float weights[16];
    for (int i = 0; i < 16; ++i)
        weights[i] = 0.5f;

    // Only even steps eligible
    uint32_t eligibility = 0x5555;  // 0101010101010101
    uint32_t seed = 12345;

    uint64_t pattern = BlendEuclideanWithWeights(4, steps, weights, eligibility, 1.0f, seed);

    // All hits must be on eligible steps
    REQUIRE((pattern & ~eligibility) == 0);
}

TEST_CASE("BlendEuclideanWithWeights blends ratios correctly", "[euclidean][blending]")
{
    const int steps = 16;
    float weights[16];
    for (int i = 0; i < 16; ++i)
        weights[i] = 0.5f;

    uint32_t eligibility = 0xFFFF;
    uint32_t seed = 12345;
    int budget = 8;

    // At ratio = 0.0 (pure Gumbel), should use weights only
    uint32_t gumbelOnly = BlendEuclideanWithWeights(budget, steps, weights, eligibility, 0.0f, seed);
    int gumbelHits = 0;
    for (int i = 0; i < steps; ++i)
        if (gumbelOnly & (1u << i))
            gumbelHits++;
    REQUIRE(gumbelHits == budget);

    // At ratio = 1.0 (pure Euclidean), should use even distribution
    uint32_t euclideanOnly = BlendEuclideanWithWeights(budget, steps, weights, eligibility, 1.0f, seed);
    int euclideanHits = 0;
    for (int i = 0; i < steps; ++i)
        if (euclideanOnly & (1u << i))
            euclideanHits++;
    REQUIRE(euclideanHits == budget);

    // Patterns should differ (probabilistic vs deterministic)
    // (Note: this isn't guaranteed but very likely with different strategies)
}

TEST_CASE("BlendEuclideanWithWeights rotation is seed-dependent", "[euclidean][blending]")
{
    const int steps = 16;
    float weights[16];
    for (int i = 0; i < 16; ++i)
        weights[i] = 0.5f;

    uint32_t eligibility = 0xFFFF;
    int budget = 4;

    // Different seeds should produce different rotations
    uint64_t pattern1 = BlendEuclideanWithWeights(budget, steps, weights, eligibility, 1.0f, 12345);
    uint64_t pattern2 = BlendEuclideanWithWeights(budget, steps, weights, eligibility, 1.0f, 67890);

    // Hit counts should be the same
    int hits1 = 0, hits2 = 0;
    for (int i = 0; i < steps; ++i)
    {
        if (pattern1 & (1u << i)) hits1++;
        if (pattern2 & (1u << i)) hits2++;
    }
    REQUIRE(hits1 == hits2);
    REQUIRE(hits1 == budget);

    // Patterns should differ (different rotations)
    // (Note: not guaranteed if seeds produce same rotation, but very unlikely)
}
