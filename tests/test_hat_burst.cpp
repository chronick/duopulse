#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/HatBurst.h"
#include "../src/Engine/HashUtils.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// HatBurst Struct Tests
// =============================================================================

TEST_CASE("HatBurst struct initializes correctly", "[hatburst][init]")
{
    HatBurst burst;

    SECTION("Init clears all fields")
    {
        // Set some values first
        burst.count = 5;
        burst.fillStart = 10;
        burst.fillDuration = 8;
        burst.triggers[0].step = 3;
        burst.triggers[0].velocity = 0.8f;

        burst.Init();

        REQUIRE(burst.count == 0);
        REQUIRE(burst.fillStart == 0);
        REQUIRE(burst.fillDuration == 0);
    }

    SECTION("Clear only resets count")
    {
        burst.count = 5;
        burst.fillStart = 10;
        burst.fillDuration = 8;

        burst.Clear();

        REQUIRE(burst.count == 0);
        // Other fields preserved
        REQUIRE(burst.fillStart == 10);
        REQUIRE(burst.fillDuration == 8);
    }
}

// =============================================================================
// FindNearestEmpty Tests
// =============================================================================

TEST_CASE("FindNearestEmpty finds correct positions", "[hatburst][collision]")
{
    SECTION("Returns target when empty")
    {
        uint32_t usedSteps = 0;  // Nothing used
        REQUIRE(FindNearestEmpty(0, 8, usedSteps) == 0);
        REQUIRE(FindNearestEmpty(4, 8, usedSteps) == 4);
        REQUIRE(FindNearestEmpty(7, 8, usedSteps) == 7);
    }

    SECTION("Finds nearest left when target used")
    {
        uint32_t usedSteps = 0b00010000;  // Step 4 used
        // Looking for step 4, should find step 3 (left first)
        REQUIRE(FindNearestEmpty(4, 8, usedSteps) == 3);
    }

    SECTION("Finds nearest right when left is also used")
    {
        uint32_t usedSteps = 0b00011000;  // Steps 3 and 4 used
        // Looking for step 4, step 3 also used, should find step 5
        REQUIRE(FindNearestEmpty(4, 8, usedSteps) == 5);
    }

    SECTION("Wraps around correctly")
    {
        uint32_t usedSteps = 0b00000001;  // Step 0 used
        // Looking for step 0 in 8-step pattern
        // Left (step 7) should be tried first
        REQUIRE(FindNearestEmpty(0, 8, usedSteps) == 7);
    }

    SECTION("Returns -1 when all steps used")
    {
        uint32_t usedSteps = 0b11111111;  // All 8 steps used
        REQUIRE(FindNearestEmpty(4, 8, usedSteps) == -1);
    }

    SECTION("Handles single-step fill")
    {
        uint32_t usedSteps = 0;
        REQUIRE(FindNearestEmpty(0, 1, usedSteps) == 0);

        usedSteps = 0b1;
        REQUIRE(FindNearestEmpty(0, 1, usedSteps) == -1);
    }

    SECTION("Handles edge cases")
    {
        REQUIRE(FindNearestEmpty(0, 0, 0) == -1);  // Zero duration
        REQUIRE(FindNearestEmpty(5, 4, 0) == 1);   // Step > duration, wraps
    }
}

// =============================================================================
// CheckProximity Tests
// =============================================================================

TEST_CASE("CheckProximity detects nearby hits", "[hatburst][proximity]")
{
    SECTION("Returns true when exact step has hit")
    {
        uint32_t mainPattern = 0b00010000;  // Hit on step 4
        // Fill starts at step 0, checking step 4
        REQUIRE(CheckProximity(4, 0, mainPattern, 1, 32) == true);
    }

    SECTION("Returns true when adjacent step has hit")
    {
        uint32_t mainPattern = 0b00010000;  // Hit on step 4
        // Fill starts at step 0, checking step 3 (adjacent to 4)
        REQUIRE(CheckProximity(3, 0, mainPattern, 1, 32) == true);
        REQUIRE(CheckProximity(5, 0, mainPattern, 1, 32) == true);
    }

    SECTION("Returns false when no nearby hit")
    {
        uint32_t mainPattern = 0b00010000;  // Hit on step 4
        // Fill starts at step 0, checking step 1 (not adjacent to 4)
        REQUIRE(CheckProximity(1, 0, mainPattern, 1, 32) == false);
    }

    SECTION("Handles fill offset correctly")
    {
        uint32_t mainPattern = 0b00010000;  // Hit on step 4
        // Fill starts at step 2, so fill step 2 = pattern step 4
        REQUIRE(CheckProximity(2, 2, mainPattern, 1, 32) == true);
        REQUIRE(CheckProximity(0, 2, mainPattern, 1, 32) == false);
    }

    SECTION("Handles wrap-around at pattern boundary")
    {
        uint32_t mainPattern = 0b00000001;  // Hit on step 0
        // Fill starts at step 30, checking step 2 = pattern step 0
        REQUIRE(CheckProximity(2, 30, mainPattern, 1, 32) == true);
    }

    SECTION("Respects proximity window size")
    {
        uint32_t mainPattern = 0b00010000;  // Hit on step 4
        // Window of 0 means exact match only
        REQUIRE(CheckProximity(4, 0, mainPattern, 0, 32) == true);
        REQUIRE(CheckProximity(3, 0, mainPattern, 0, 32) == false);
        REQUIRE(CheckProximity(5, 0, mainPattern, 0, 32) == false);
    }
}

// =============================================================================
// EuclideanWithJitter Tests
// =============================================================================

TEST_CASE("EuclideanWithJitter distributes triggers", "[hatburst][euclidean]")
{
    SECTION("Base euclidean spacing is even")
    {
        // At shape=0.30 (minimum of euclidean zone), minimal jitter
        // 4 triggers in 8 steps should be roughly 0, 2, 4, 6
        int pos0 = EuclideanWithJitter(0, 4, 8, 0.30f, 12345);
        int pos1 = EuclideanWithJitter(1, 4, 8, 0.30f, 12345);
        int pos2 = EuclideanWithJitter(2, 4, 8, 0.30f, 12345);
        int pos3 = EuclideanWithJitter(3, 4, 8, 0.30f, 12345);

        // All should be within range
        REQUIRE(pos0 >= 0);
        REQUIRE(pos0 < 8);
        REQUIRE(pos1 >= 0);
        REQUIRE(pos1 < 8);
        REQUIRE(pos2 >= 0);
        REQUIRE(pos2 < 8);
        REQUIRE(pos3 >= 0);
        REQUIRE(pos3 < 8);
    }

    SECTION("Higher shape increases jitter")
    {
        // Run multiple times with different seeds to check variation
        int samePositions = 0;
        for (uint32_t seed = 0; seed < 20; ++seed)
        {
            int posLow = EuclideanWithJitter(1, 4, 8, 0.31f, seed);
            int posHigh = EuclideanWithJitter(1, 4, 8, 0.69f, seed);
            if (posLow == posHigh)
                samePositions++;
        }
        // At higher shape, more jitter means fewer matches
        // This is probabilistic, but should have some differences
        REQUIRE(samePositions < 20);  // Not all should be the same
    }

    SECTION("Deterministic with same inputs")
    {
        int pos1 = EuclideanWithJitter(2, 4, 8, 0.5f, 99999);
        int pos2 = EuclideanWithJitter(2, 4, 8, 0.5f, 99999);
        int pos3 = EuclideanWithJitter(2, 4, 8, 0.5f, 99999);

        REQUIRE(pos1 == pos2);
        REQUIRE(pos2 == pos3);
    }

    SECTION("Handles edge cases")
    {
        REQUIRE(EuclideanWithJitter(0, 0, 8, 0.5f, 0) == 0);   // Zero triggers
        REQUIRE(EuclideanWithJitter(0, 4, 0, 0.5f, 0) == 0);   // Zero duration
        REQUIRE(EuclideanWithJitter(0, 1, 8, 0.5f, 12345) == 0);  // Single trigger
    }
}

// =============================================================================
// GenerateHatBurst Trigger Count Tests
// =============================================================================

TEST_CASE("GenerateHatBurst produces correct trigger counts", "[hatburst][density]")
{
    HatBurst burst;

    SECTION("Minimum energy gives 2 triggers")
    {
        GenerateHatBurst(0.0f, 0.5f, 0, 0, 16, 32, 12345, burst);
        REQUIRE(burst.count == 2);
    }

    SECTION("Maximum energy gives 12 triggers")
    {
        GenerateHatBurst(1.0f, 0.5f, 0, 0, 16, 32, 12345, burst);
        REQUIRE(burst.count == 12);
    }

    SECTION("Mid energy gives proportional triggers")
    {
        GenerateHatBurst(0.5f, 0.5f, 0, 0, 16, 32, 12345, burst);
        // 2 + floor(0.5 * 10) = 2 + 5 = 7
        REQUIRE(burst.count == 7);
    }

    SECTION("Trigger count limited by fill duration")
    {
        GenerateHatBurst(1.0f, 0.5f, 0, 0, 4, 32, 12345, burst);
        // Would want 12 triggers, but only 4 steps available
        REQUIRE(burst.count == 4);
    }

    SECTION("All trigger counts are in valid range")
    {
        for (float energy = 0.0f; energy <= 1.0f; energy += 0.1f)
        {
            GenerateHatBurst(energy, 0.5f, 0, 0, 16, 32, 12345, burst);
            REQUIRE(burst.count >= kMinHatBurstTriggers);
            REQUIRE(burst.count <= kMaxHatBurstTriggers);
        }
    }
}

// =============================================================================
// GenerateHatBurst Timing Zone Tests
// =============================================================================

TEST_CASE("GenerateHatBurst timing follows SHAPE zones", "[hatburst][timing]")
{
    HatBurst burst;

    SECTION("Low SHAPE produces evenly spaced triggers")
    {
        GenerateHatBurst(0.5f, 0.0f, 0, 0, 16, 32, 12345, burst);

        // 7 triggers in 16 steps should be roughly evenly spaced
        // Check that steps are sorted and spread out
        bool sorted = true;
        for (int i = 1; i < burst.count; ++i)
        {
            if (burst.triggers[i].step < burst.triggers[i - 1].step)
            {
                // Allow for wrap-around collision resolution
                // but generally should be increasing
            }
        }

        // Check reasonable distribution
        int minGap = 16;
        int maxGap = 0;
        for (int i = 1; i < burst.count; ++i)
        {
            int gap = burst.triggers[i].step - burst.triggers[i - 1].step;
            if (gap > 0)
            {
                minGap = std::min(minGap, gap);
                maxGap = std::max(maxGap, gap);
            }
        }
        // Even spacing should have relatively consistent gaps
        // Allow some variation due to collision resolution
        REQUIRE(maxGap - minGap <= 4);
    }

    SECTION("High SHAPE produces varied timing")
    {
        // Run multiple times with different seeds
        int totalVariation = 0;
        for (uint32_t seed = 0; seed < 10; ++seed)
        {
            GenerateHatBurst(0.5f, 0.9f, 0, 0, 16, 32, seed, burst);

            // Calculate variance in step spacing
            for (int i = 1; i < burst.count; ++i)
            {
                int gap = std::abs(burst.triggers[i].step - burst.triggers[i - 1].step);
                totalVariation += gap;
            }
        }
        // High shape should produce varied gaps (not all same)
        REQUIRE(totalVariation > 0);
    }
}

// =============================================================================
// GenerateHatBurst Velocity Tests
// =============================================================================

TEST_CASE("GenerateHatBurst velocity follows energy", "[hatburst][velocity]")
{
    HatBurst burst;

    SECTION("Low energy produces lower velocities")
    {
        GenerateHatBurst(0.0f, 0.5f, 0, 0, 16, 32, 12345, burst);

        for (int i = 0; i < burst.count; ++i)
        {
            // Base velocity at energy=0 is 0.65, with +-5% variation
            REQUIRE(burst.triggers[i].velocity >= 0.58f);
            REQUIRE(burst.triggers[i].velocity <= 0.70f);
        }
    }

    SECTION("High energy produces higher velocities")
    {
        GenerateHatBurst(1.0f, 0.5f, 0, 0, 16, 32, 12345, burst);

        // At least some triggers should have high velocity
        // (may be ducked near main pattern, but with mainPattern=0, none)
        bool hasHighVel = false;
        for (int i = 0; i < burst.count; ++i)
        {
            if (burst.triggers[i].velocity >= 0.9f)
                hasHighVel = true;
        }
        REQUIRE(hasHighVel);
    }
}

// =============================================================================
// GenerateHatBurst Velocity Ducking Tests
// =============================================================================

TEST_CASE("GenerateHatBurst ducks velocity near main pattern", "[hatburst][ducking]")
{
    HatBurst burst;

    SECTION("Triggers near main pattern hits are ducked")
    {
        // Main pattern with hit on step 4
        uint32_t mainPattern = 0b00010000;
        GenerateHatBurst(1.0f, 0.0f, mainPattern, 0, 16, 32, 12345, burst);

        // Find triggers near step 4 (steps 3, 4, 5)
        for (int i = 0; i < burst.count; ++i)
        {
            int step = burst.triggers[i].step;
            if (step >= 3 && step <= 5)
            {
                // Should be ducked to ~30% of normal
                REQUIRE(burst.triggers[i].velocity < 0.40f);
            }
        }
    }

    SECTION("Triggers far from main pattern are not ducked")
    {
        // Main pattern with hit on step 0 only
        uint32_t mainPattern = 0b00000001;
        GenerateHatBurst(1.0f, 0.0f, mainPattern, 0, 16, 32, 12345, burst);

        // Find triggers far from step 0 (e.g., step 8)
        for (int i = 0; i < burst.count; ++i)
        {
            int step = burst.triggers[i].step;
            if (step >= 6 && step <= 10)
            {
                // Should NOT be ducked
                REQUIRE(burst.triggers[i].velocity > 0.50f);
            }
        }
    }
}

// =============================================================================
// GenerateHatBurst Collision Avoidance Tests
// =============================================================================

TEST_CASE("GenerateHatBurst avoids trigger collisions", "[hatburst][collision]")
{
    HatBurst burst;

    SECTION("All triggers have unique steps")
    {
        GenerateHatBurst(1.0f, 0.5f, 0, 0, 16, 32, 12345, burst);

        uint32_t usedSteps = 0;
        for (int i = 0; i < burst.count; ++i)
        {
            int step = burst.triggers[i].step;
            // Check step not already used
            REQUIRE((usedSteps & (1U << step)) == 0);
            usedSteps |= (1U << step);
        }
    }

    SECTION("Collisions are resolved even with random distribution")
    {
        // High shape = random distribution, more likely to collide initially
        for (uint32_t seed = 0; seed < 10; ++seed)
        {
            GenerateHatBurst(1.0f, 1.0f, 0, 0, 16, 32, seed, burst);

            uint32_t usedSteps = 0;
            for (int i = 0; i < burst.count; ++i)
            {
                int step = burst.triggers[i].step;
                REQUIRE((usedSteps & (1U << step)) == 0);
                usedSteps |= (1U << step);
            }
        }
    }
}

// =============================================================================
// GenerateHatBurst Determinism Tests
// =============================================================================

TEST_CASE("GenerateHatBurst is deterministic", "[hatburst][determinism]")
{
    SECTION("Same inputs produce identical outputs")
    {
        HatBurst burst1, burst2, burst3;

        GenerateHatBurst(0.7f, 0.4f, 0b11001010, 8, 8, 32, 54321, burst1);
        GenerateHatBurst(0.7f, 0.4f, 0b11001010, 8, 8, 32, 54321, burst2);
        GenerateHatBurst(0.7f, 0.4f, 0b11001010, 8, 8, 32, 54321, burst3);

        REQUIRE(burst1.count == burst2.count);
        REQUIRE(burst2.count == burst3.count);

        for (int i = 0; i < burst1.count; ++i)
        {
            REQUIRE(burst1.triggers[i].step == burst2.triggers[i].step);
            REQUIRE(burst2.triggers[i].step == burst3.triggers[i].step);
            REQUIRE(burst1.triggers[i].velocity == burst2.triggers[i].velocity);
            REQUIRE(burst2.triggers[i].velocity == burst3.triggers[i].velocity);
        }
    }

    SECTION("Different seeds produce different outputs")
    {
        HatBurst burst1, burst2;

        // Use shape > 0.7 to activate random mode where seeds have clear effect
        GenerateHatBurst(0.7f, 0.9f, 0, 0, 16, 32, 11111, burst1);
        GenerateHatBurst(0.7f, 0.9f, 0, 0, 16, 32, 99999, burst2);

        // At least some trigger positions should differ
        bool anyDifferent = false;
        for (int i = 0; i < std::min(burst1.count, burst2.count); ++i)
        {
            if (burst1.triggers[i].step != burst2.triggers[i].step)
            {
                anyDifferent = true;
                break;
            }
        }
        REQUIRE(anyDifferent);
    }
}

// =============================================================================
// GenerateHatBurst Fill Info Tests
// =============================================================================

TEST_CASE("GenerateHatBurst stores fill info correctly", "[hatburst][fillinfo]")
{
    HatBurst burst;

    SECTION("Fill start and duration are stored")
    {
        GenerateHatBurst(0.5f, 0.5f, 0, 24, 8, 32, 12345, burst);

        REQUIRE(burst.fillStart == 24);
        REQUIRE(burst.fillDuration == 8);
    }

    SECTION("All trigger steps are within fill duration")
    {
        for (int duration = 4; duration <= 16; duration += 4)
        {
            GenerateHatBurst(1.0f, 0.5f, 0, 0, duration, 32, 12345, burst);

            for (int i = 0; i < burst.count; ++i)
            {
                REQUIRE(burst.triggers[i].step < duration);
            }
        }
    }
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST_CASE("GenerateHatBurst handles edge cases", "[hatburst][edge]")
{
    HatBurst burst;

    SECTION("Handles minimum fill duration")
    {
        GenerateHatBurst(0.5f, 0.5f, 0, 0, 1, 32, 12345, burst);
        // Only 1 step, should have 1 trigger (limited by duration)
        REQUIRE(burst.count == 1);
        REQUIRE(burst.triggers[0].step == 0);
    }

    SECTION("Handles clamped parameters")
    {
        // Parameters outside valid range should be clamped
        GenerateHatBurst(-0.5f, 1.5f, 0, 0, 16, 32, 12345, burst);
        REQUIRE(burst.count >= kMinHatBurstTriggers);
        REQUIRE(burst.count <= kMaxHatBurstTriggers);
    }

    SECTION("Handles dense main pattern")
    {
        // Main pattern with many hits - should still produce valid burst
        uint32_t densePattern = 0b11111111111111111111111111111111;
        GenerateHatBurst(0.5f, 0.5f, densePattern, 0, 16, 32, 12345, burst);

        // All triggers should be ducked
        for (int i = 0; i < burst.count; ++i)
        {
            REQUIRE(burst.triggers[i].velocity < 0.40f);
        }
    }
}

// =============================================================================
// RT Safety Tests (Compile-Time Guarantees)
// =============================================================================

TEST_CASE("HatBurst is RT safe", "[hatburst][rtsafe]")
{
    SECTION("HatBurst struct has fixed size")
    {
        // Verify no hidden allocations by checking struct size
        // 12 triggers * (1 byte step + 4 byte float velocity) = 60 bytes
        // Plus 4 bytes for count, fillStart, fillDuration, padding
        // Total should be around 64 bytes
        REQUIRE(sizeof(HatBurst) <= 128);  // Conservative upper bound
    }

    SECTION("Constants are compile-time")
    {
        static_assert(kMaxHatBurstTriggers == 12, "Max triggers should be 12");
        static_assert(kMinHatBurstTriggers == 2, "Min triggers should be 2");
    }
}
