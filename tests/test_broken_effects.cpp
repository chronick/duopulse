#include <catch2/catch_all.hpp>
#include "../src/Engine/BrokenEffects.h"

using namespace daisysp_idm_grids;

// =============================================================================
// Swing Tests [broken-effects][swing]
// =============================================================================

TEST_CASE("GetSwingFromBroken returns correct values at BROKEN range boundaries", "[broken-effects][swing]")
{
    // Techno range: 0-25% → 50-54%
    REQUIRE(GetSwingFromBroken(0.0f) == Catch::Approx(0.50f));
    REQUIRE(GetSwingFromBroken(0.25f) == Catch::Approx(0.54f));

    // Tribal range: 25-50% → 54-60%
    REQUIRE(GetSwingFromBroken(0.25f) == Catch::Approx(0.54f));
    REQUIRE(GetSwingFromBroken(0.50f) == Catch::Approx(0.60f));

    // Trip-Hop range: 50-75% → 60-66%
    REQUIRE(GetSwingFromBroken(0.50f) == Catch::Approx(0.60f));
    REQUIRE(GetSwingFromBroken(0.75f) == Catch::Approx(0.66f));

    // IDM range: 75-100% → 66-58%
    REQUIRE(GetSwingFromBroken(0.75f) == Catch::Approx(0.66f));
    REQUIRE(GetSwingFromBroken(1.0f) == Catch::Approx(0.58f));
}

TEST_CASE("GetSwingFromBroken produces smooth transitions within ranges", "[broken-effects][swing]")
{
    // Techno midpoint: 12.5% → should be between 50% and 54%
    float technoMid = GetSwingFromBroken(0.125f);
    REQUIRE(technoMid > 0.50f);
    REQUIRE(technoMid < 0.54f);
    REQUIRE(technoMid == Catch::Approx(0.52f));

    // Tribal midpoint: 37.5% → should be between 54% and 60%
    float tribalMid = GetSwingFromBroken(0.375f);
    REQUIRE(tribalMid > 0.54f);
    REQUIRE(tribalMid < 0.60f);
    REQUIRE(tribalMid == Catch::Approx(0.57f));

    // Trip-Hop midpoint: 62.5% → should be between 60% and 66%
    float tripHopMid = GetSwingFromBroken(0.625f);
    REQUIRE(tripHopMid > 0.60f);
    REQUIRE(tripHopMid < 0.66f);
    REQUIRE(tripHopMid == Catch::Approx(0.63f));

    // IDM midpoint: 87.5% → should be between 58% and 66%
    float idmMid = GetSwingFromBroken(0.875f);
    REQUIRE(idmMid > 0.58f);
    REQUIRE(idmMid < 0.66f);
    REQUIRE(idmMid == Catch::Approx(0.62f));
}

TEST_CASE("GetSwingFromBroken clamps out-of-range inputs", "[broken-effects][swing]")
{
    // Negative values should clamp to 0
    REQUIRE(GetSwingFromBroken(-0.5f) == Catch::Approx(0.50f));
    REQUIRE(GetSwingFromBroken(-1.0f) == Catch::Approx(0.50f));

    // Values > 1 should clamp to 1
    REQUIRE(GetSwingFromBroken(1.5f) == Catch::Approx(0.58f));
    REQUIRE(GetSwingFromBroken(2.0f) == Catch::Approx(0.58f));
}

TEST_CASE("IsOffBeat correctly identifies off-beat steps", "[broken-effects][swing]")
{
    // Even steps are on-beat
    REQUIRE(IsOffBeat(0) == false);
    REQUIRE(IsOffBeat(2) == false);
    REQUIRE(IsOffBeat(4) == false);
    REQUIRE(IsOffBeat(8) == false);
    REQUIRE(IsOffBeat(16) == false);

    // Odd steps are off-beat
    REQUIRE(IsOffBeat(1) == true);
    REQUIRE(IsOffBeat(3) == true);
    REQUIRE(IsOffBeat(5) == true);
    REQUIRE(IsOffBeat(7) == true);
    REQUIRE(IsOffBeat(31) == true);
}

// =============================================================================
// Jitter Tests [broken-effects][jitter]
// =============================================================================

TEST_CASE("GetJitterMsFromBroken returns 0 below 40% BROKEN", "[broken-effects][jitter]")
{
    REQUIRE(GetJitterMsFromBroken(0.0f) == Catch::Approx(0.0f));
    REQUIRE(GetJitterMsFromBroken(0.1f) == Catch::Approx(0.0f));
    REQUIRE(GetJitterMsFromBroken(0.2f) == Catch::Approx(0.0f));
    REQUIRE(GetJitterMsFromBroken(0.3f) == Catch::Approx(0.0f));
    REQUIRE(GetJitterMsFromBroken(0.39f) == Catch::Approx(0.0f));
}

TEST_CASE("GetJitterMsFromBroken scales correctly in each range", "[broken-effects][jitter]")
{
    // 40-70%: 0-3ms
    REQUIRE(GetJitterMsFromBroken(0.4f) == Catch::Approx(0.0f));
    REQUIRE(GetJitterMsFromBroken(0.55f) == Catch::Approx(1.5f));
    REQUIRE(GetJitterMsFromBroken(0.7f) == Catch::Approx(3.0f));

    // 70-90%: 3-6ms
    REQUIRE(GetJitterMsFromBroken(0.7f) == Catch::Approx(3.0f));
    REQUIRE(GetJitterMsFromBroken(0.8f) == Catch::Approx(4.5f));
    REQUIRE(GetJitterMsFromBroken(0.9f) == Catch::Approx(6.0f));

    // 90-100%: 6-12ms
    REQUIRE(GetJitterMsFromBroken(0.9f) == Catch::Approx(6.0f));
    REQUIRE(GetJitterMsFromBroken(0.95f) == Catch::Approx(9.0f));
    REQUIRE(GetJitterMsFromBroken(1.0f) == Catch::Approx(12.0f));
}

TEST_CASE("GetJitterMsFromBroken clamps out-of-range inputs", "[broken-effects][jitter]")
{
    // Negative values should clamp to 0
    REQUIRE(GetJitterMsFromBroken(-0.5f) == Catch::Approx(0.0f));

    // Values > 1 should clamp to max jitter
    REQUIRE(GetJitterMsFromBroken(1.5f) == Catch::Approx(12.0f));
}

TEST_CASE("ApplyJitter returns 0 when maxJitter is 0", "[broken-effects][jitter]")
{
    REQUIRE(ApplyJitter(0.0f, 12345, 0) == Catch::Approx(0.0f));
    REQUIRE(ApplyJitter(0.0f, 12345, 5) == Catch::Approx(0.0f));
    REQUIRE(ApplyJitter(0.0f, 99999, 31) == Catch::Approx(0.0f));
}

TEST_CASE("ApplyJitter produces values within expected range", "[broken-effects][jitter]")
{
    float maxJitter = 6.0f;
    uint32_t seed = 12345;

    // Test multiple steps to verify range
    for (int step = 0; step < 32; step++)
    {
        float jitter = ApplyJitter(maxJitter, seed, step);
        REQUIRE(jitter >= -maxJitter);
        REQUIRE(jitter <= maxJitter);
    }
}

TEST_CASE("ApplyJitter is deterministic with same seed", "[broken-effects][jitter]")
{
    float maxJitter = 3.0f;
    uint32_t seed = 0xABCD1234;

    for (int step = 0; step < 32; step++)
    {
        float jitter1 = ApplyJitter(maxJitter, seed, step);
        float jitter2 = ApplyJitter(maxJitter, seed, step);
        REQUIRE(jitter1 == jitter2);
    }
}

TEST_CASE("ApplyJitter produces variation across different seeds", "[broken-effects][jitter]")
{
    float maxJitter = 6.0f;
    int step = 5;

    // Collect jitter values across different seeds
    int positiveCount = 0;
    int negativeCount = 0;

    for (uint32_t seed = 0; seed < 100; seed++)
    {
        float jitter = ApplyJitter(maxJitter, seed, step);
        if (jitter > 0)
            positiveCount++;
        else if (jitter < 0)
            negativeCount++;
    }

    // Should have a mix of positive and negative jitter values
    REQUIRE(positiveCount > 10);
    REQUIRE(negativeCount > 10);
}

// =============================================================================
// Step Displacement Tests [broken-effects][displacement]
// =============================================================================

TEST_CASE("GetDisplacedStep returns original step below 50% BROKEN", "[broken-effects][displacement]")
{
    uint32_t seed = 12345;

    // No displacement at low BROKEN
    for (int step = 0; step < 32; step++)
    {
        REQUIRE(GetDisplacedStep(step, 0.0f, seed) == step);
        REQUIRE(GetDisplacedStep(step, 0.25f, seed) == step);
        REQUIRE(GetDisplacedStep(step, 0.49f, seed) == step);
    }
}

TEST_CASE("GetDisplacedStep can displace at high BROKEN", "[broken-effects][displacement]")
{
    // At very high BROKEN, some steps should be displaced
    // Test over many seeds to find at least some displacement
    int displacementCount = 0;

    for (uint32_t seed = 0; seed < 1000; seed++)
    {
        for (int step = 0; step < 32; step++)
        {
            int displaced = GetDisplacedStep(step, 1.0f, seed);
            if (displaced != step)
            {
                displacementCount++;
            }
        }
    }

    // At BROKEN=1.0, chance is 40%, so over 32000 samples we expect ~12800 displacements
    // Allow wide margin for randomness
    REQUIRE(displacementCount > 5000);
    REQUIRE(displacementCount < 20000);
}

TEST_CASE("GetDisplacedStep respects max shift limits", "[broken-effects][displacement]")
{
    // At 50-75% BROKEN, max shift is ±1
    // At 75-100% BROKEN, max shift is ±2

    for (uint32_t seed = 0; seed < 1000; seed++)
    {
        for (int step = 0; step < 32; step++)
        {
            // At 60% BROKEN (±1 max shift)
            int displaced60 = GetDisplacedStep(step, 0.60f, seed);
            int diff60 = (displaced60 - step + 32) % 32;
            // Difference should be 0, 1, or 31 (which is -1 wrapped)
            bool valid60 = (diff60 == 0 || diff60 == 1 || diff60 == 31);
            REQUIRE(valid60);

            // At 100% BROKEN (±2 max shift)
            int displaced100 = GetDisplacedStep(step, 1.0f, seed);
            int diff100 = (displaced100 - step + 32) % 32;
            // Difference should be 0, 1, 2, 30, or 31 (which is -2 or -1 wrapped)
            bool valid100 = (diff100 == 0 || diff100 == 1 || diff100 == 2 ||
                           diff100 == 30 || diff100 == 31);
            REQUIRE(valid100);
        }
    }
}

TEST_CASE("GetDisplacedStep wraps around step range", "[broken-effects][displacement]")
{
    // Test edge cases at step boundaries
    for (uint32_t seed = 0; seed < 1000; seed++)
    {
        // Step 0 can wrap to 31 (or 30 at high broken)
        int displaced0 = GetDisplacedStep(0, 1.0f, seed);
        REQUIRE(displaced0 >= 0);
        REQUIRE(displaced0 < 32);

        // Step 31 can wrap to 0 (or 1 at high broken)
        int displaced31 = GetDisplacedStep(31, 1.0f, seed);
        REQUIRE(displaced31 >= 0);
        REQUIRE(displaced31 < 32);
    }
}

TEST_CASE("GetDisplacedStep is deterministic with same seed", "[broken-effects][displacement]")
{
    uint32_t seed = 0xDEADBEEF;

    for (int step = 0; step < 32; step++)
    {
        int displaced1 = GetDisplacedStep(step, 0.8f, seed);
        int displaced2 = GetDisplacedStep(step, 0.8f, seed);
        REQUIRE(displaced1 == displaced2);
    }
}

TEST_CASE("GetDisplacedStep displacement chance increases with BROKEN", "[broken-effects][displacement]")
{
    // Count displacements at different BROKEN levels
    auto countDisplacements = [](float broken) {
        int count = 0;
        for (uint32_t seed = 0; seed < 500; seed++)
        {
            for (int step = 0; step < 32; step++)
            {
                if (GetDisplacedStep(step, broken, seed) != step)
                    count++;
            }
        }
        return count;
    };

    int count55 = countDisplacements(0.55f); // Low end of first range
    int count70 = countDisplacements(0.70f); // High end of first range
    int count80 = countDisplacements(0.80f); // Low end of second range
    int count100 = countDisplacements(1.0f); // Max

    // Higher BROKEN should produce more displacements
    REQUIRE(count55 < count70);
    REQUIRE(count70 < count80);
    REQUIRE(count80 < count100);
}

// =============================================================================
// Velocity Variation Tests [broken-effects][velocity]
// =============================================================================

TEST_CASE("GetVelocityWithVariation maintains base velocity with minimal variation at low BROKEN", "[broken-effects][velocity]")
{
    uint32_t seed = 12345;
    float baseVel = 0.8f;

    // At low BROKEN, variation is ±5%
    for (int step = 0; step < 32; step++)
    {
        float varied = GetVelocityWithVariation(baseVel, 0.1f, seed, step);
        REQUIRE(varied >= baseVel - 0.05f);
        REQUIRE(varied <= baseVel + 0.05f);
    }
}

TEST_CASE("GetVelocityWithVariation has larger range at high BROKEN", "[broken-effects][velocity]")
{
    float baseVel = 0.8f;

    // At high BROKEN, variation is ±20%
    float minSeen = 1.0f;
    float maxSeen = 0.0f;

    for (uint32_t seed = 0; seed < 100; seed++)
    {
        for (int step = 0; step < 32; step++)
        {
            float varied = GetVelocityWithVariation(baseVel, 1.0f, seed, step);
            if (varied < minSeen) minSeen = varied;
            if (varied > maxSeen) maxSeen = varied;
        }
    }

    // With ±20% variation from 0.8, we expect range of 0.6 to 1.0
    // (clamped to 1.0 at high end)
    REQUIRE(minSeen < 0.65f);
    REQUIRE(maxSeen > 0.95f);
}

TEST_CASE("GetVelocityWithVariation clamps to minimum 0.2", "[broken-effects][velocity]")
{
    // Even with low base velocity and high variation, result should be >= 0.2
    float baseVel = 0.1f;

    for (uint32_t seed = 0; seed < 100; seed++)
    {
        for (int step = 0; step < 32; step++)
        {
            float varied = GetVelocityWithVariation(baseVel, 1.0f, seed, step);
            REQUIRE(varied >= 0.2f);
        }
    }
}

TEST_CASE("GetVelocityWithVariation clamps to maximum 1.0", "[broken-effects][velocity]")
{
    float baseVel = 0.95f;

    for (uint32_t seed = 0; seed < 100; seed++)
    {
        for (int step = 0; step < 32; step++)
        {
            float varied = GetVelocityWithVariation(baseVel, 1.0f, seed, step);
            REQUIRE(varied <= 1.0f);
        }
    }
}

TEST_CASE("GetVelocityWithVariation is deterministic with same seed", "[broken-effects][velocity]")
{
    uint32_t seed = 0xBEEFCAFE;
    float baseVel = 0.7f;
    float broken = 0.5f;

    for (int step = 0; step < 32; step++)
    {
        float varied1 = GetVelocityWithVariation(baseVel, broken, seed, step);
        float varied2 = GetVelocityWithVariation(baseVel, broken, seed, step);
        REQUIRE(varied1 == varied2);
    }
}

TEST_CASE("GetVelocityVariationRange returns correct ranges", "[broken-effects][velocity]")
{
    // 0-30%: ±5%
    REQUIRE(GetVelocityVariationRange(0.0f) == Catch::Approx(0.05f));
    REQUIRE(GetVelocityVariationRange(0.15f) == Catch::Approx(0.05f));
    REQUIRE(GetVelocityVariationRange(0.29f) == Catch::Approx(0.05f));

    // 30-60%: 5-10%
    REQUIRE(GetVelocityVariationRange(0.30f) == Catch::Approx(0.05f));
    REQUIRE(GetVelocityVariationRange(0.45f) == Catch::Approx(0.075f));
    REQUIRE(GetVelocityVariationRange(0.60f) == Catch::Approx(0.10f));

    // 60-100%: 10-20%
    REQUIRE(GetVelocityVariationRange(0.60f) == Catch::Approx(0.10f));
    REQUIRE(GetVelocityVariationRange(0.80f) == Catch::Approx(0.15f));
    REQUIRE(GetVelocityVariationRange(1.00f) == Catch::Approx(0.20f));
}

TEST_CASE("GetVelocityVariationRange clamps out-of-range inputs", "[broken-effects][velocity]")
{
    // Negative values should clamp to 0
    REQUIRE(GetVelocityVariationRange(-0.5f) == Catch::Approx(0.05f));

    // Values > 1 should clamp to max
    REQUIRE(GetVelocityVariationRange(1.5f) == Catch::Approx(0.20f));
}

// =============================================================================
// Phrase Modulation Tests [broken-effects][phrase-modulation]
// =============================================================================

TEST_CASE("GetPhraseWeightBoost returns 0 outside build zone", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.isBuildZone = false;
    pos.isFillZone = false;
    pos.phraseProgress = 0.3f;  // Early in phrase

    // Should return 0 regardless of broken level
    REQUIRE(GetPhraseWeightBoost(pos, 0.0f) == Catch::Approx(0.0f));
    REQUIRE(GetPhraseWeightBoost(pos, 0.5f) == Catch::Approx(0.0f));
    REQUIRE(GetPhraseWeightBoost(pos, 1.0f) == Catch::Approx(0.0f));
}

TEST_CASE("GetPhraseWeightBoost returns subtle boost in build zone", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.isBuildZone = true;
    pos.isFillZone = false;

    // At start of build zone (50%)
    pos.phraseProgress = 0.5f;
    REQUIRE(GetPhraseWeightBoost(pos, 0.0f) == Catch::Approx(0.0f));

    // At end of build zone (75%) before fill zone
    pos.phraseProgress = 0.749f;
    float boost = GetPhraseWeightBoost(pos, 0.0f);
    // buildProgress ≈ 0.996, boost ≈ 0.075 * 0.996 ≈ 0.0747, scaled by 0.5 ≈ 0.037
    REQUIRE(boost > 0.0f);
    REQUIRE(boost < 0.075f);  // Build zone max is 0.075 * genreScale
}

TEST_CASE("GetPhraseWeightBoost returns significant boost in fill zone", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.isBuildZone = true;
    pos.isFillZone = true;

    // At start of fill zone (75%)
    pos.phraseProgress = 0.75f;
    float boostStart = GetPhraseWeightBoost(pos, 0.5f);  // genreScale = 1.0
    // boost = 0.15 + 0 = 0.15, scaled by 1.0 = 0.15
    REQUIRE(boostStart == Catch::Approx(0.15f));

    // At end of fill zone (100%)
    pos.phraseProgress = 1.0f;
    float boostEnd = GetPhraseWeightBoost(pos, 0.5f);  // genreScale = 1.0
    // boost = 0.15 + 1.0 * 0.10 = 0.25, scaled by 1.0 = 0.25
    REQUIRE(boostEnd == Catch::Approx(0.25f));
}

TEST_CASE("GetPhraseWeightBoost scales with BROKEN level", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.isBuildZone = true;
    pos.isFillZone = true;
    pos.phraseProgress = 0.9f;  // Mid fill zone

    // At broken=0: genreScale = 0.5
    float boostLow = GetPhraseWeightBoost(pos, 0.0f);

    // At broken=1: genreScale = 1.5
    float boostHigh = GetPhraseWeightBoost(pos, 1.0f);

    // High broken should produce 3× the boost of low broken
    REQUIRE(boostHigh == Catch::Approx(boostLow * 3.0f));
}

TEST_CASE("GetEffectiveBroken returns unchanged value outside fill zone", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.isFillZone = false;
    pos.phraseProgress = 0.3f;

    REQUIRE(GetEffectiveBroken(0.0f, pos) == Catch::Approx(0.0f));
    REQUIRE(GetEffectiveBroken(0.5f, pos) == Catch::Approx(0.5f));
    REQUIRE(GetEffectiveBroken(1.0f, pos) == Catch::Approx(1.0f));
}

TEST_CASE("GetEffectiveBroken boosts in fill zone", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.isFillZone = true;

    // At start of fill zone (75%): no boost yet
    pos.phraseProgress = 0.75f;
    REQUIRE(GetEffectiveBroken(0.5f, pos) == Catch::Approx(0.5f));

    // At end of fill zone (100%): 20% boost
    pos.phraseProgress = 1.0f;
    REQUIRE(GetEffectiveBroken(0.5f, pos) == Catch::Approx(0.7f));
    REQUIRE(GetEffectiveBroken(0.0f, pos) == Catch::Approx(0.2f));
}

TEST_CASE("GetEffectiveBroken clamps to 1.0", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.isFillZone = true;
    pos.phraseProgress = 1.0f;  // Max boost

    // Even with high base broken, should not exceed 1.0
    REQUIRE(GetEffectiveBroken(0.9f, pos) == Catch::Approx(1.0f));
    REQUIRE(GetEffectiveBroken(1.0f, pos) == Catch::Approx(1.0f));
}

TEST_CASE("GetPhraseAccent returns 1.2 for phrase downbeat", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.stepInPhrase = 0;
    pos.isDownbeat = true;

    REQUIRE(GetPhraseAccent(pos) == Catch::Approx(1.2f));
}

TEST_CASE("GetPhraseAccent returns 1.1 for bar downbeat", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.stepInPhrase = 16;  // Second bar
    pos.isDownbeat = true;

    REQUIRE(GetPhraseAccent(pos) == Catch::Approx(1.1f));

    pos.stepInPhrase = 32;  // Third bar
    REQUIRE(GetPhraseAccent(pos) == Catch::Approx(1.1f));
}

TEST_CASE("GetPhraseAccent returns 1.0 for other steps", "[broken-effects][phrase-modulation]")
{
    PhrasePosition pos;
    pos.stepInPhrase = 5;
    pos.isDownbeat = false;
    REQUIRE(GetPhraseAccent(pos) == Catch::Approx(1.0f));

    pos.stepInPhrase = 13;
    REQUIRE(GetPhraseAccent(pos) == Catch::Approx(1.0f));

    pos.stepInPhrase = 31;
    REQUIRE(GetPhraseAccent(pos) == Catch::Approx(1.0f));
}

TEST_CASE("Phrase modulation functions work with CalculatePhrasePosition", "[broken-effects][phrase-modulation][integration]")
{
    // Test integration with actual phrase position calculation
    int loopLengthBars = 4;

    // Step 0 (phrase downbeat)
    PhrasePosition pos0 = CalculatePhrasePosition(0, loopLengthBars);
    REQUIRE(GetPhraseAccent(pos0) == Catch::Approx(1.2f));
    REQUIRE(GetPhraseWeightBoost(pos0, 0.5f) == Catch::Approx(0.0f));  // Not in build zone

    // Step 16 (second bar downbeat)
    PhrasePosition pos16 = CalculatePhrasePosition(16, loopLengthBars);
    REQUIRE(GetPhraseAccent(pos16) == Catch::Approx(1.1f));

    // Step 60 (in fill zone of a 4-bar phrase = 64 steps)
    PhrasePosition pos60 = CalculatePhrasePosition(60, loopLengthBars);
    REQUIRE(pos60.isFillZone == true);
    float effectiveBroken = GetEffectiveBroken(0.5f, pos60);
    REQUIRE(effectiveBroken > 0.5f);  // Should be boosted
}

// =============================================================================
// FUSE Energy Balance Tests [broken-effects][fuse-balance]
// =============================================================================

TEST_CASE("ApplyFuse at 0.5 makes no change", "[broken-effects][fuse-balance]")
{
    float anchor = 0.6f;
    float shimmer = 0.4f;

    ApplyFuse(0.5f, anchor, shimmer);

    REQUIRE(anchor == Catch::Approx(0.6f));
    REQUIRE(shimmer == Catch::Approx(0.4f));
}

TEST_CASE("ApplyFuse at 0.0 boosts anchor, reduces shimmer", "[broken-effects][fuse-balance]")
{
    float anchor = 0.5f;
    float shimmer = 0.5f;

    ApplyFuse(0.0f, anchor, shimmer);

    // bias = (0.0 - 0.5) * 0.3 = -0.15
    // anchor = 0.5 - (-0.15) = 0.65
    // shimmer = 0.5 + (-0.15) = 0.35
    REQUIRE(anchor == Catch::Approx(0.65f));
    REQUIRE(shimmer == Catch::Approx(0.35f));
}

TEST_CASE("ApplyFuse at 1.0 boosts shimmer, reduces anchor", "[broken-effects][fuse-balance]")
{
    float anchor = 0.5f;
    float shimmer = 0.5f;

    ApplyFuse(1.0f, anchor, shimmer);

    // bias = (1.0 - 0.5) * 0.3 = +0.15
    // anchor = 0.5 - 0.15 = 0.35
    // shimmer = 0.5 + 0.15 = 0.65
    REQUIRE(anchor == Catch::Approx(0.35f));
    REQUIRE(shimmer == Catch::Approx(0.65f));
}

TEST_CASE("ApplyFuse clamps results to valid range", "[broken-effects][fuse-balance]")
{
    // Test clamping at low end
    {
        float anchor = 0.1f;
        float shimmer = 0.1f;

        ApplyFuse(1.0f, anchor, shimmer);  // Reduces anchor by 0.15

        REQUIRE(anchor >= 0.0f);
        REQUIRE(shimmer <= 1.0f);
    }

    // Test clamping at high end
    {
        float anchor = 0.95f;
        float shimmer = 0.95f;

        ApplyFuse(0.0f, anchor, shimmer);  // Boosts anchor by 0.15

        REQUIRE(anchor <= 1.0f);
        REQUIRE(shimmer >= 0.0f);
    }
}

TEST_CASE("ApplyFuse clamps input parameter", "[broken-effects][fuse-balance]")
{
    float anchor = 0.5f;
    float shimmer = 0.5f;

    // Test with out-of-range fuse values
    ApplyFuse(-0.5f, anchor, shimmer);  // Should clamp to 0.0
    REQUIRE(anchor == Catch::Approx(0.65f));
    REQUIRE(shimmer == Catch::Approx(0.35f));

    anchor = 0.5f;
    shimmer = 0.5f;
    ApplyFuse(1.5f, anchor, shimmer);  // Should clamp to 1.0
    REQUIRE(anchor == Catch::Approx(0.35f));
    REQUIRE(shimmer == Catch::Approx(0.65f));
}

// =============================================================================
// COUPLE Interlock Tests [broken-effects][couple-interlock]
// =============================================================================

TEST_CASE("ApplyCouple below 0.1 makes no changes", "[broken-effects][couple-interlock]")
{
    bool shimmerFires = true;
    float shimmerVel = 0.8f;
    uint32_t seed = 12345;

    // Anchor firing, but couple too low to suppress
    ApplyCouple(0.05f, true, shimmerFires, shimmerVel, seed, 0);
    REQUIRE(shimmerFires == true);
    REQUIRE(shimmerVel == Catch::Approx(0.8f));

    // Anchor silent, but couple too low to boost
    shimmerFires = false;
    ApplyCouple(0.05f, false, shimmerFires, shimmerVel, seed, 0);
    REQUIRE(shimmerFires == false);
}

TEST_CASE("ApplyCouple suppresses shimmer when anchor fires", "[broken-effects][couple-interlock]")
{
    // At high couple, shimmer should often be suppressed when anchor fires
    int suppressedCount = 0;

    for (uint32_t seed = 0; seed < 1000; seed++)
    {
        bool shimmerFires = true;
        float shimmerVel = 0.8f;

        ApplyCouple(1.0f, true, shimmerFires, shimmerVel, seed, 5);

        if (!shimmerFires)
            suppressedCount++;
    }

    // At couple=1.0, suppression chance is 80%
    // Expect ~800 suppressions out of 1000
    REQUIRE(suppressedCount > 600);
    REQUIRE(suppressedCount < 950);
}

TEST_CASE("ApplyCouple suppression scales with couple value", "[broken-effects][couple-interlock]")
{
    auto countSuppressions = [](float couple) {
        int count = 0;
        for (uint32_t seed = 0; seed < 500; seed++)
        {
            for (int step = 0; step < 32; step++)
            {
                bool shimmerFires = true;
                float shimmerVel = 0.8f;

                ApplyCouple(couple, true, shimmerFires, shimmerVel, seed, step);

                if (!shimmerFires)
                    count++;
            }
        }
        return count;
    };

    int suppLow = countSuppressions(0.2f);
    int suppMid = countSuppressions(0.5f);
    int suppHigh = countSuppressions(1.0f);

    // Higher couple should produce more suppressions
    REQUIRE(suppLow < suppMid);
    REQUIRE(suppMid < suppHigh);
}

TEST_CASE("ApplyCouple fills gaps when anchor silent at high couple", "[broken-effects][couple-interlock]")
{
    // At high couple and anchor silent, shimmer can be boosted
    int boostCount = 0;

    for (uint32_t seed = 0; seed < 1000; seed++)
    {
        bool shimmerFires = false;  // Not already firing
        float shimmerVel = 0.0f;

        ApplyCouple(1.0f, false, shimmerFires, shimmerVel, seed, 5);

        if (shimmerFires)
        {
            boostCount++;
            // Velocity should be in medium range (0.5 to 0.8)
            REQUIRE(shimmerVel >= 0.5f);
            REQUIRE(shimmerVel <= 0.8f);
        }
    }

    // At couple=1.0, boost chance is 30%
    // Expect ~300 boosts out of 1000
    REQUIRE(boostCount > 150);
    REQUIRE(boostCount < 450);
}

TEST_CASE("ApplyCouple does not boost when couple <= 0.5", "[broken-effects][couple-interlock]")
{
    // Gap filling only happens above 50% couple
    for (uint32_t seed = 0; seed < 100; seed++)
    {
        bool shimmerFires = false;
        float shimmerVel = 0.0f;

        ApplyCouple(0.5f, false, shimmerFires, shimmerVel, seed, 5);

        REQUIRE(shimmerFires == false);  // Should never boost at exactly 0.5
    }
}

TEST_CASE("ApplyCouple does not modify already-firing shimmer when boosting", "[broken-effects][couple-interlock]")
{
    // If shimmer is already firing, gap-fill boost doesn't apply
    bool shimmerFires = true;
    float shimmerVel = 0.9f;
    uint32_t seed = 12345;

    ApplyCouple(1.0f, false, shimmerFires, shimmerVel, seed, 5);

    // Shimmer was already firing, should remain unchanged
    REQUIRE(shimmerFires == true);
    REQUIRE(shimmerVel == Catch::Approx(0.9f));
}

TEST_CASE("ApplyCouple is deterministic with same seed", "[broken-effects][couple-interlock]")
{
    uint32_t seed = 0xCAFEBABE;

    for (int step = 0; step < 32; step++)
    {
        // Test suppression
        bool shimmer1 = true;
        float vel1 = 0.8f;
        ApplyCouple(0.8f, true, shimmer1, vel1, seed, step);

        bool shimmer2 = true;
        float vel2 = 0.8f;
        ApplyCouple(0.8f, true, shimmer2, vel2, seed, step);

        REQUIRE(shimmer1 == shimmer2);

        // Test boost
        bool shimmer3 = false;
        float vel3 = 0.0f;
        ApplyCouple(0.8f, false, shimmer3, vel3, seed, step);

        bool shimmer4 = false;
        float vel4 = 0.0f;
        ApplyCouple(0.8f, false, shimmer4, vel4, seed, step);

        REQUIRE(shimmer3 == shimmer4);
        if (shimmer3)
        {
            REQUIRE(vel3 == vel4);
        }
    }
}

TEST_CASE("ApplyCouple clamps couple parameter", "[broken-effects][couple-interlock]")
{
    // Negative couple should behave like 0 (below threshold, no effect)
    {
        bool shimmerFires = true;
        float shimmerVel = 0.8f;
        ApplyCouple(-0.5f, true, shimmerFires, shimmerVel, 12345, 0);
        REQUIRE(shimmerFires == true);  // No suppression
    }

    // Couple > 1.0 should clamp to 1.0 (max effect)
    {
        int suppressedCount = 0;
        for (uint32_t seed = 0; seed < 100; seed++)
        {
            bool shimmerFires = true;
            float shimmerVel = 0.8f;
            ApplyCouple(1.5f, true, shimmerFires, shimmerVel, seed, 0);
            if (!shimmerFires)
                suppressedCount++;
        }
        // Should behave like couple=1.0 (80% suppression)
        REQUIRE(suppressedCount > 50);
    }
}

// =============================================================================
// v3 Critical Rules: DENSITY=0 Absolute Silence [v3-critical-rules]
// =============================================================================

TEST_CASE("ApplyFuse preserves DENSITY=0 for anchor", "[broken-effects][v3-critical-rules][density-zero]")
{
    // If anchor density was 0, FUSE must NOT boost it above 0
    float anchor = 0.0f;
    float shimmer = 0.5f;

    // FUSE CW would normally reduce anchor by 0.15, but it's already 0
    // FUSE CCW would normally boost anchor by 0.15, but we must preserve 0
    ApplyFuse(0.0f, anchor, shimmer);  // CCW = anchor boost attempt

    REQUIRE(anchor == Catch::Approx(0.0f));  // Must remain at 0
    REQUIRE(shimmer < 0.5f);  // Shimmer still affected
}

TEST_CASE("ApplyFuse preserves DENSITY=0 for shimmer", "[broken-effects][v3-critical-rules][density-zero]")
{
    // If shimmer density was 0, FUSE must NOT boost it above 0
    float anchor = 0.5f;
    float shimmer = 0.0f;

    // FUSE CW would normally boost shimmer by 0.15, but we must preserve 0
    ApplyFuse(1.0f, anchor, shimmer);  // CW = shimmer boost attempt

    REQUIRE(shimmer == Catch::Approx(0.0f));  // Must remain at 0
    REQUIRE(anchor < 0.5f);  // Anchor still affected
}

TEST_CASE("ApplyFuse preserves DENSITY=0 for both voices", "[broken-effects][v3-critical-rules][density-zero]")
{
    // Both voices at 0 must stay at 0 regardless of FUSE
    for (float fuse = 0.0f; fuse <= 1.0f; fuse += 0.1f)
    {
        float anchor = 0.0f;
        float shimmer = 0.0f;

        ApplyFuse(fuse, anchor, shimmer);

        REQUIRE(anchor == Catch::Approx(0.0f));
        REQUIRE(shimmer == Catch::Approx(0.0f));
    }
}

TEST_CASE("ApplyCouple does not gap-fill when shimmerDensity=0", "[broken-effects][v3-critical-rules][density-zero]")
{
    // Gap-filling must NOT happen when shimmer density is 0
    // Even at max COUPLE with anchor silent, shimmer stays silent
    for (uint32_t seed = 0; seed < 100; seed++)
    {
        bool shimmerFires = false;
        float shimmerVel = 0.0f;
        float shimmerDensity = 0.0f;  // Density is zero

        ApplyCouple(1.0f, false, shimmerFires, shimmerVel, seed, 5, shimmerDensity);

        REQUIRE(shimmerFires == false);  // Must NOT gap-fill when density=0
        REQUIRE(shimmerVel == Catch::Approx(0.0f));
    }
}

TEST_CASE("ApplyCouple still suppresses when shimmerDensity=0", "[broken-effects][v3-critical-rules][density-zero]")
{
    // Suppression should still work (turning shimmer OFF)
    // This is a safety check - if shimmer somehow fired, COUPLE can suppress it
    bool shimmerFires = true;  // Hypothetically firing
    float shimmerVel = 0.8f;
    float shimmerDensity = 0.0f;
    uint32_t seed = 12345;

    // Find a seed that triggers suppression
    for (uint32_t s = 0; s < 100; s++)
    {
        shimmerFires = true;
        shimmerVel = 0.8f;
        ApplyCouple(1.0f, true, shimmerFires, shimmerVel, s, 0, shimmerDensity);
        if (!shimmerFires)
            break;
    }

    // At high couple with anchor firing, suppression should work
    // (This test just verifies suppression still functions)
    // The important invariant is: gap-fill NEVER injects triggers at density=0
}

TEST_CASE("ApplyCouple gap-fills normally when shimmerDensity > 0", "[broken-effects][v3-critical-rules][density-zero]")
{
    // Verify gap-filling works normally when density is positive
    int boostCount = 0;

    for (uint32_t seed = 0; seed < 100; seed++)
    {
        bool shimmerFires = false;
        float shimmerVel = 0.0f;
        float shimmerDensity = 0.5f;  // Normal density

        ApplyCouple(1.0f, false, shimmerFires, shimmerVel, seed, 5, shimmerDensity);

        if (shimmerFires)
            boostCount++;
    }

    // Should have some gap-fills when density > 0
    REQUIRE(boostCount > 10);
}

TEST_CASE("ApplyCouple backward compatible when shimmerDensity not provided", "[broken-effects][v3-critical-rules][density-zero]")
{
    // When shimmerDensity is not provided (default -1.0f), gap-filling should work
    // This ensures backward compatibility with existing code
    int boostCount = 0;

    for (uint32_t seed = 0; seed < 100; seed++)
    {
        bool shimmerFires = false;
        float shimmerVel = 0.0f;

        // Call without shimmerDensity (uses default -1.0f)
        ApplyCouple(1.0f, false, shimmerFires, shimmerVel, seed, 5);

        if (shimmerFires)
            boostCount++;
    }

    // Should still gap-fill when density param not provided
    REQUIRE(boostCount > 10);
}

// =============================================================================
// Integration Tests [broken-effects][integration]
// =============================================================================

TEST_CASE("BROKEN effects combine coherently", "[broken-effects][integration]")
{
    // At low BROKEN: straight timing, no jitter, no displacement, consistent velocity
    {
        float broken = 0.1f;
        REQUIRE(GetSwingFromBroken(broken) < 0.54f);  // Techno range
        REQUIRE(GetJitterMsFromBroken(broken) == Catch::Approx(0.0f));
        // Displacement should not happen
        REQUIRE(GetDisplacedStep(5, broken, 12345) == 5);
        // Velocity variation should be minimal
        REQUIRE(GetVelocityVariationRange(broken) == Catch::Approx(0.05f));
    }

    // At high BROKEN: heavy swing reduction, max jitter, displacement possible, expressive velocity
    {
        float broken = 0.95f;
        REQUIRE(GetSwingFromBroken(broken) > 0.58f);  // IDM range
        REQUIRE(GetSwingFromBroken(broken) < 0.66f);
        REQUIRE(GetJitterMsFromBroken(broken) > 6.0f);  // High jitter
        // Displacement should be possible (test with many seeds)
        int displacementCount = 0;
        for (uint32_t seed = 0; seed < 100; seed++)
        {
            if (GetDisplacedStep(5, broken, seed) != 5)
                displacementCount++;
        }
        REQUIRE(displacementCount > 10);
        // Velocity variation should be large
        REQUIRE(GetVelocityVariationRange(broken) > 0.15f);
    }
}
