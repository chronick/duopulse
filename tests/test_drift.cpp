#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/DriftControl.h"
#include "../src/Engine/SequencerState.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// Step Stability Tests
// =============================================================================

TEST_CASE("GetStepStability returns correct hierarchy", "[drift]")
{
    SECTION("Downbeat has highest stability")
    {
        float downbeat = GetStepStability(0, 32);
        REQUIRE(downbeat == Approx(kStabilityDownbeat));
        REQUIRE(downbeat == Approx(1.0f));
    }

    SECTION("Half-bar is second highest")
    {
        float halfBar = GetStepStability(16, 32);
        REQUIRE(halfBar == Approx(kStabilityHalfBar));
        REQUIRE(halfBar < kStabilityDownbeat);
    }

    SECTION("Quarter notes follow half-bar")
    {
        float q8 = GetStepStability(8, 32);
        float q24 = GetStepStability(24, 32);

        REQUIRE(q8 == Approx(kStabilityQuarter));
        REQUIRE(q24 == Approx(kStabilityQuarter));
        REQUIRE(q8 < kStabilityHalfBar);
    }

    SECTION("Eighth notes follow quarters")
    {
        float e4 = GetStepStability(4, 32);
        float e12 = GetStepStability(12, 32);
        float e20 = GetStepStability(20, 32);
        float e28 = GetStepStability(28, 32);

        REQUIRE(e4 == Approx(kStabilityEighth));
        REQUIRE(e12 == Approx(kStabilityEighth));
        REQUIRE(e20 == Approx(kStabilityEighth));
        REQUIRE(e28 == Approx(kStabilityEighth));
        REQUIRE(e4 < kStabilityQuarter);
    }

    SECTION("Strong sixteenths are less stable")
    {
        float s2 = GetStepStability(2, 32);
        float s6 = GetStepStability(6, 32);
        float s10 = GetStepStability(10, 32);

        REQUIRE(s2 == Approx(kStabilitySixteenth));
        REQUIRE(s6 == Approx(kStabilitySixteenth));
        REQUIRE(s10 == Approx(kStabilitySixteenth));
        REQUIRE(s2 < kStabilityEighth);
    }

    SECTION("Off-beats have lowest stability")
    {
        float o1 = GetStepStability(1, 32);
        float o3 = GetStepStability(3, 32);
        float o15 = GetStepStability(15, 32);
        float o31 = GetStepStability(31, 32);

        REQUIRE(o1 == Approx(kStabilityOffbeat));
        REQUIRE(o3 == Approx(kStabilityOffbeat));
        REQUIRE(o15 == Approx(kStabilityOffbeat));
        REQUIRE(o31 == Approx(kStabilityOffbeat));
        REQUIRE(o1 < kStabilitySixteenth);
    }
}

TEST_CASE("GetStepStability handles edge cases", "[drift]")
{
    SECTION("Negative step returns off-beat stability")
    {
        float neg = GetStepStability(-1, 32);
        REQUIRE(neg == Approx(kStabilityOffbeat));
    }

    SECTION("Step beyond pattern length returns off-beat stability")
    {
        float beyond = GetStepStability(32, 32);
        REQUIRE(beyond == Approx(kStabilityOffbeat));
    }

    SECTION("Works with 16-step pattern")
    {
        // Downbeat should still be highest
        float down16 = GetStepStability(0, 16);
        REQUIRE(down16 == Approx(kStabilityDownbeat));

        // Step 8 in 16-step = step 16 in 32-step (half-bar)
        float half16 = GetStepStability(8, 16);
        REQUIRE(half16 == Approx(kStabilityHalfBar));
    }
}

TEST_CASE("GetStabilityMask produces correct masks", "[drift]")
{
    SECTION("Threshold 1.0 only includes downbeat")
    {
        uint32_t mask = GetStabilityMask(32, 1.0f);
        REQUIRE(mask == 0x00000001);
    }

    SECTION("Threshold 0.9 includes downbeat and half-bar")
    {
        uint32_t mask = GetStabilityMask(32, 0.9f);
        // Step 0 (1.0) and step 16 (0.9)
        REQUIRE((mask & 0x00000001) != 0);  // Step 0
        REQUIRE((mask & 0x00010000) != 0);  // Step 16
    }

    SECTION("Threshold 0.0 includes all steps")
    {
        uint32_t mask = GetStabilityMask(32, 0.0f);
        REQUIRE(mask == 0xFFFFFFFF);
    }

    SECTION("Threshold 0.5 includes downbeat through eighth notes")
    {
        uint32_t mask = GetStabilityMask(32, 0.5f);

        // Should include: 0, 4, 8, 12, 16, 20, 24, 28
        REQUIRE((mask & 0x00000001) != 0);  // Step 0
        REQUIRE((mask & 0x00000010) != 0);  // Step 4
        REQUIRE((mask & 0x00000100) != 0);  // Step 8
        REQUIRE((mask & 0x00001000) != 0);  // Step 12
        REQUIRE((mask & 0x00010000) != 0);  // Step 16
        REQUIRE((mask & 0x00100000) != 0);  // Step 20
        REQUIRE((mask & 0x01000000) != 0);  // Step 24
        REQUIRE((mask & 0x10000000) != 0);  // Step 28

        // Should NOT include odd steps
        REQUIRE((mask & 0x00000002) == 0);  // Step 1
        REQUIRE((mask & 0x00000004) == 0);  // Step 2
    }
}

// =============================================================================
// Seed Selection Tests
// =============================================================================

TEST_CASE("DRIFT=0 uses locked seed for all steps", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0xABCDEF01);

    float drift = 0.0f;

    SECTION("All steps use pattern seed")
    {
        for (int step = 0; step < 32; ++step)
        {
            uint32_t seed = SelectSeed(state, drift, step, 32);
            REQUIRE(seed == state.patternSeed);
        }
    }
}

TEST_CASE("DRIFT=1 uses evolving seed for all steps", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0xABCDEF01);

    float drift = 1.0f;

    SECTION("All steps use phrase seed")
    {
        for (int step = 0; step < 32; ++step)
        {
            uint32_t seed = SelectSeed(state, drift, step, 32);
            REQUIRE(seed == state.phraseSeed);
        }
    }
}

TEST_CASE("Downbeats use locked seed longer", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0x11111111);

    SECTION("At DRIFT=0.5, downbeat is locked, off-beats evolve")
    {
        float drift = 0.5f;

        // Downbeat (stability 1.0) should use pattern seed
        uint32_t downbeatSeed = SelectSeed(state, drift, 0, 32);
        REQUIRE(downbeatSeed == state.patternSeed);

        // Off-beat (stability 0.1) should use phrase seed
        uint32_t offbeatSeed = SelectSeed(state, drift, 1, 32);
        REQUIRE(offbeatSeed == state.phraseSeed);
    }

    SECTION("At DRIFT=0.8, only downbeat is locked")
    {
        float drift = 0.8f;

        // Downbeat (stability 1.0 > 0.8) - locked
        uint32_t downbeatSeed = SelectSeed(state, drift, 0, 32);
        REQUIRE(downbeatSeed == state.patternSeed);

        // Half-bar (stability 0.9 > 0.8) - locked
        uint32_t halfBarSeed = SelectSeed(state, drift, 16, 32);
        REQUIRE(halfBarSeed == state.patternSeed);

        // Quarter (stability 0.7 <= 0.8) - evolving
        uint32_t quarterSeed = SelectSeed(state, drift, 8, 32);
        REQUIRE(quarterSeed == state.phraseSeed);
    }

    SECTION("Stability hierarchy is preserved across DRIFT values")
    {
        // Check that more stable steps lock before less stable ones
        for (float drift = 0.1f; drift <= 0.9f; drift += 0.1f)
        {
            bool downbeatLocked = IsStepLocked(0, 32, drift);
            bool halfBarLocked = IsStepLocked(16, 32, drift);
            bool quarterLocked = IsStepLocked(8, 32, drift);
            bool offbeatLocked = IsStepLocked(1, 32, drift);

            // If off-beat is locked, everything should be locked
            if (offbeatLocked)
            {
                REQUIRE(quarterLocked);
            }

            // If quarter is locked, half-bar should be locked
            if (quarterLocked)
            {
                REQUIRE(halfBarLocked);
            }

            // If half-bar is locked, downbeat should be locked
            if (halfBarLocked)
            {
                REQUIRE(downbeatLocked);
            }
        }
    }
}

TEST_CASE("SelectSeedWithStability works correctly", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0x22222222);

    SECTION("High stability uses pattern seed")
    {
        uint32_t seed = SelectSeedWithStability(state, 0.5f, 0.8f);
        REQUIRE(seed == state.patternSeed);
    }

    SECTION("Low stability uses phrase seed")
    {
        uint32_t seed = SelectSeedWithStability(state, 0.5f, 0.3f);
        REQUIRE(seed == state.phraseSeed);
    }

    SECTION("Edge case: stability equals drift uses phrase seed")
    {
        uint32_t seed = SelectSeedWithStability(state, 0.5f, 0.5f);
        REQUIRE(seed == state.phraseSeed);  // Not strictly greater
    }
}

// =============================================================================
// Phrase Seed Changes at Boundary Tests
// =============================================================================

TEST_CASE("Phrase seed changes at boundary", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0x33333333);

    uint32_t initialPhraseSeed = state.phraseSeed;
    uint32_t initialPatternSeed = state.patternSeed;

    SECTION("OnPhraseEnd changes phrase seed")
    {
        OnPhraseEnd(state);

        REQUIRE(state.phraseSeed != initialPhraseSeed);
        REQUIRE(state.phraseCounter == 1);
    }

    SECTION("Pattern seed unchanged without reseed request")
    {
        OnPhraseEnd(state);

        REQUIRE(state.patternSeed == initialPatternSeed);
    }

    SECTION("Multiple phrase ends produce different seeds")
    {
        uint32_t seeds[5];

        for (int i = 0; i < 5; ++i)
        {
            OnPhraseEnd(state);
            seeds[i] = state.phraseSeed;
        }

        // All seeds should be unique
        for (int i = 0; i < 5; ++i)
        {
            for (int j = i + 1; j < 5; ++j)
            {
                REQUIRE(seeds[i] != seeds[j]);
            }
        }
    }

    SECTION("Phrase counter increments")
    {
        REQUIRE(state.phraseCounter == 0);

        OnPhraseEnd(state);
        REQUIRE(state.phraseCounter == 1);

        OnPhraseEnd(state);
        REQUIRE(state.phraseCounter == 2);

        OnPhraseEnd(state);
        REQUIRE(state.phraseCounter == 3);
    }
}

// =============================================================================
// Reseed Tests
// =============================================================================

TEST_CASE("Reseed generates new pattern", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0x44444444);

    uint32_t originalPatternSeed = state.patternSeed;

    SECTION("RequestReseed takes effect at phrase end")
    {
        RequestReseed(state);

        // Pattern seed unchanged until phrase end
        REQUIRE(state.patternSeed == originalPatternSeed);
        REQUIRE(state.reseedRequested == true);

        // Now end the phrase
        OnPhraseEnd(state);

        // Pattern seed should change
        REQUIRE(state.patternSeed != originalPatternSeed);
        REQUIRE(state.reseedRequested == false);
    }

    SECTION("Hard reseed takes effect immediately")
    {
        Reseed(state, 0);  // Generate new seed

        REQUIRE(state.patternSeed != originalPatternSeed);
        REQUIRE(state.phraseCounter == 0);  // Reset
    }

    SECTION("Hard reseed with specific seed uses that seed")
    {
        uint32_t specificSeed = 0x99887766;
        Reseed(state, specificSeed);

        REQUIRE(state.patternSeed == specificSeed);
    }

    SECTION("Reseed clears pending reseed request")
    {
        RequestReseed(state);
        REQUIRE(state.reseedRequested == true);

        Reseed(state, 0);
        REQUIRE(state.reseedRequested == false);
    }

    SECTION("Multiple reseeds produce different patterns")
    {
        uint32_t seeds[5];

        for (int i = 0; i < 5; ++i)
        {
            Reseed(state, 0);
            seeds[i] = state.patternSeed;
            // Advance counter to ensure different mixing
            state.phraseCounter = i + 100;
        }

        // All seeds should be unique
        for (int i = 0; i < 5; ++i)
        {
            for (int j = i + 1; j < 5; ++j)
            {
                REQUIRE(seeds[i] != seeds[j]);
            }
        }
    }
}

// =============================================================================
// Initialization Tests
// =============================================================================

TEST_CASE("InitDriftState sets up state correctly", "[drift]")
{
    DriftState state;

    SECTION("With specific seed")
    {
        InitDriftState(state, 0x55555555);

        REQUIRE(state.patternSeed == 0x55555555);
        REQUIRE(state.phraseSeed == (0x55555555 ^ kPhraseSeedXor));
        REQUIRE(state.phraseCounter == 0);
        REQUIRE(state.reseedRequested == false);
    }

    SECTION("With zero seed uses default")
    {
        InitDriftState(state, 0);

        REQUIRE(state.patternSeed == kDefaultPatternSeed);
    }

    SECTION("Phrase seed differs from pattern seed")
    {
        InitDriftState(state, 0x12345678);

        REQUIRE(state.patternSeed != state.phraseSeed);
    }
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST_CASE("GetLockedRatio reports correct fraction", "[drift]")
{
    SECTION("DRIFT=0 locks all steps")
    {
        float ratio = GetLockedRatio(0.0f, 32);
        REQUIRE(ratio == Approx(1.0f));
    }

    SECTION("DRIFT=1 locks no steps")
    {
        float ratio = GetLockedRatio(1.0f, 32);
        REQUIRE(ratio == Approx(0.0f));
    }

    SECTION("Intermediate DRIFT gives partial locking")
    {
        float ratio = GetLockedRatio(0.5f, 32);
        REQUIRE(ratio > 0.0f);
        REQUIRE(ratio < 1.0f);
    }

    SECTION("Higher DRIFT means lower locked ratio")
    {
        float low = GetLockedRatio(0.3f, 32);
        float high = GetLockedRatio(0.7f, 32);

        REQUIRE(low > high);
    }
}

TEST_CASE("IsStepLocked is consistent with SelectSeed", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0x66666666);

    for (float drift = 0.0f; drift <= 1.0f; drift += 0.1f)
    {
        for (int step = 0; step < 32; ++step)
        {
            bool locked = IsStepLocked(step, 32, drift);
            uint32_t seed = SelectSeed(state, drift, step, 32);

            if (locked)
            {
                REQUIRE(seed == state.patternSeed);
            }
            else
            {
                REQUIRE(seed == state.phraseSeed);
            }
        }
    }
}

TEST_CASE("HashCombine produces good distribution", "[drift]")
{
    SECTION("Different inputs produce different outputs")
    {
        uint32_t a = HashCombine(0x12345678, 1);
        uint32_t b = HashCombine(0x12345678, 2);
        uint32_t c = HashCombine(0x12345678, 3);

        REQUIRE(a != b);
        REQUIRE(b != c);
        REQUIRE(a != c);
    }

    SECTION("Order matters")
    {
        uint32_t a = HashCombine(0x11111111, 0x22222222);
        uint32_t b = HashCombine(0x22222222, 0x11111111);

        REQUIRE(a != b);
    }
}

TEST_CASE("GenerateNewSeed produces valid seeds", "[drift]")
{
    SECTION("Never returns zero")
    {
        // Zero seed with various counters
        for (uint32_t i = 0; i < 100; ++i)
        {
            uint32_t seed = GenerateNewSeed(0, i);
            REQUIRE(seed != 0);
        }
    }

    SECTION("Different counters produce different seeds")
    {
        uint32_t base = 0x77777777;
        uint32_t seeds[10];

        for (uint32_t i = 0; i < 10; ++i)
        {
            seeds[i] = GenerateNewSeed(base, i);
        }

        for (int i = 0; i < 10; ++i)
        {
            for (int j = i + 1; j < 10; ++j)
            {
                REQUIRE(seeds[i] != seeds[j]);
            }
        }
    }
}

// =============================================================================
// Integration with DriftState Tests
// =============================================================================

TEST_CASE("DriftState.GetSeedForStep matches DriftControl", "[drift]")
{
    // Verify that the inline implementation in DriftState matches
    // the standalone DriftControl functions

    DriftState state;
    InitDriftState(state, 0x88888888);

    for (float drift = 0.0f; drift <= 1.0f; drift += 0.25f)
    {
        for (int step = 0; step < 32; ++step)
        {
            float stability = GetStepStability(step, 32);
            uint32_t fromDriftState = state.GetSeedForStep(drift, stability);
            uint32_t fromDriftControl = SelectSeedWithStability(state, drift, stability);

            REQUIRE(fromDriftState == fromDriftControl);
        }
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("Drift handles edge cases gracefully", "[drift]")
{
    DriftState state;
    InitDriftState(state, 0x99999999);

    SECTION("Negative drift treated as 0")
    {
        // All steps should use pattern seed
        uint32_t seed = SelectSeed(state, -1.0f, 5, 32);
        REQUIRE(seed == state.patternSeed);
    }

    SECTION("Drift > 1 treated as fully evolving")
    {
        // All steps should use phrase seed (stability never > 1.5)
        uint32_t seed = SelectSeed(state, 1.5f, 0, 32);
        REQUIRE(seed == state.phraseSeed);
    }

    SECTION("Pattern length 0 doesn't crash")
    {
        float ratio = GetLockedRatio(0.5f, 0);
        REQUIRE(ratio == 0.0f);
    }

    SECTION("Pattern length > 32 is clamped for mask")
    {
        uint32_t mask = GetStabilityMask(64, 0.5f);
        // Should only set bits in valid range (0-31)
        // No bits should be set beyond what a 32-bit mask can hold
        REQUIRE(mask != 0);
    }
}

// =============================================================================
// Musical Behavior Tests
// =============================================================================

TEST_CASE("DRIFT creates musically sensible evolution", "[drift][musical]")
{
    DriftState state;
    InitDriftState(state, 0xAAAAAAAA);

    SECTION("Low DRIFT keeps pattern recognizable")
    {
        float drift = 0.2f;

        // Count how many steps are locked
        int lockedCount = 0;
        for (int step = 0; step < 32; ++step)
        {
            if (IsStepLocked(step, 32, drift))
            {
                lockedCount++;
            }
        }

        // At low DRIFT (0.2), steps with stability > 0.2 are locked:
        // - 1 downbeat (1.0)
        // - 1 half-bar (0.9)
        // - 2 quarters (0.7)
        // - 4 eighths (0.5)
        // - 8 strong sixteenths (0.3)
        // = 16 steps locked (half the pattern)
        REQUIRE(lockedCount >= 16);  // At least 16 of 32
    }

    SECTION("High DRIFT allows significant evolution")
    {
        float drift = 0.8f;

        // Count how many steps are locked
        int lockedCount = 0;
        for (int step = 0; step < 32; ++step)
        {
            if (IsStepLocked(step, 32, drift))
            {
                lockedCount++;
            }
        }

        // At high DRIFT, few steps should be locked
        REQUIRE(lockedCount <= 4);  // At most 4 of 32 (downbeat + half-bar)
    }

    SECTION("Downbeats are last to evolve")
    {
        // Even at very high DRIFT, downbeat should still be locked
        float drift = 0.95f;

        bool downbeatLocked = IsStepLocked(0, 32, drift);
        bool offbeatLocked = IsStepLocked(1, 32, drift);

        REQUIRE(downbeatLocked == true);
        REQUIRE(offbeatLocked == false);
    }
}
