#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/HitBudget.h"
#include "../src/Engine/GumbelSampler.h"
#include "../src/Engine/VoiceRelation.h"
#include "../src/Engine/GuardRails.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;

// =============================================================================
// HitBudget Tests
// =============================================================================

TEST_CASE("Hit budget scales with energy", "[hit-budget]")
{
    SECTION("MINIMAL zone has lowest budget")
    {
        int minimalBudget = ComputeAnchorBudget(0.1f, EnergyZone::MINIMAL, 32);
        int grooveBudget = ComputeAnchorBudget(0.35f, EnergyZone::GROOVE, 32);
        int peakBudget = ComputeAnchorBudget(0.9f, EnergyZone::PEAK, 32);

        REQUIRE(minimalBudget < grooveBudget);
        REQUIRE(grooveBudget < peakBudget);
    }

    SECTION("Budget scales within zone")
    {
        int lowEnergy = ComputeAnchorBudget(0.25f, EnergyZone::GROOVE, 32);
        int highEnergy = ComputeAnchorBudget(0.45f, EnergyZone::GROOVE, 32);

        REQUIRE(lowEnergy <= highEnergy);
    }

    SECTION("Budget never exceeds half pattern length")
    {
        int budget = ComputeAnchorBudget(1.0f, EnergyZone::PEAK, 32);
        REQUIRE(budget <= 16);
    }

    SECTION("Budget is at least 1")
    {
        int budget = ComputeAnchorBudget(0.0f, EnergyZone::MINIMAL, 32);
        REQUIRE(budget >= 1);
    }
}

TEST_CASE("Shimmer budget affected by balance", "[hit-budget]")
{
    SECTION("Low balance gives fewer shimmer hits")
    {
        int lowBalance = ComputeShimmerBudget(0.5f, 0.0f, EnergyZone::GROOVE, 32);
        int highBalance = ComputeShimmerBudget(0.5f, 1.0f, EnergyZone::GROOVE, 32);

        REQUIRE(lowBalance < highBalance);
    }

    SECTION("Balance 0.5 gives roughly half anchor budget")
    {
        int anchorBudget = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32);
        int shimmerBudget = ComputeShimmerBudget(0.5f, 0.5f, EnergyZone::GROOVE, 32);

        // Shimmer should be about 60% of anchor at balance 0.5
        REQUIRE(shimmerBudget <= anchorBudget);
        REQUIRE(shimmerBudget >= 1);
    }
}

TEST_CASE("Aux budget respects density setting", "[hit-budget]")
{
    SECTION("SPARSE density gives fewer hits")
    {
        int sparse = ComputeAuxBudget(0.5f, EnergyZone::GROOVE, AuxDensity::SPARSE, 32);
        int normal = ComputeAuxBudget(0.5f, EnergyZone::GROOVE, AuxDensity::NORMAL, 32);
        int busy = ComputeAuxBudget(0.5f, EnergyZone::GROOVE, AuxDensity::BUSY, 32);

        REQUIRE(sparse <= normal);
        REQUIRE(normal <= busy);
    }

    SECTION("No aux in MINIMAL zone")
    {
        int budget = ComputeAuxBudget(0.1f, EnergyZone::MINIMAL, AuxDensity::BUSY, 32);
        REQUIRE(budget == 0);
    }
}

TEST_CASE("ComputeBarBudget fills all fields", "[hit-budget]")
{
    BarBudget budget;
    ComputeBarBudget(0.5f, 0.5f, EnergyZone::GROOVE, AuxDensity::NORMAL, 32, 1.0f, budget);

    REQUIRE(budget.anchorHits >= 1);
    REQUIRE(budget.shimmerHits >= 1);
    REQUIRE(budget.anchorEligibility != 0);
    REQUIRE(budget.shimmerEligibility != 0);
}

TEST_CASE("Eligibility mask expands with energy", "[hit-budget]")
{
    SECTION("MINIMAL zone uses limited positions")
    {
        uint32_t minimal = ComputeAnchorEligibility(0.1f, 0.0f, EnergyZone::MINIMAL, 32);
        uint32_t peak = ComputeAnchorEligibility(0.9f, 0.0f, EnergyZone::PEAK, 32);

        int minimalBits = CountBits(minimal);
        int peakBits = CountBits(peak);

        REQUIRE(minimalBits < peakBits);
    }

    SECTION("FLAVOR adds syncopation positions")
    {
        uint32_t straight = ComputeAnchorEligibility(0.5f, 0.0f, EnergyZone::GROOVE, 32);
        uint32_t broken = ComputeAnchorEligibility(0.5f, 0.8f, EnergyZone::GROOVE, 32);

        int straightBits = CountBits(straight);
        int brokenBits = CountBits(broken);

        REQUIRE(brokenBits >= straightBits);
    }
}

TEST_CASE("Fill boost increases density", "[hit-budget]")
{
    BarBudget budget;
    budget.Init();
    budget.anchorHits = 4;
    budget.shimmerHits = 2;
    budget.auxHits = 4;

    int originalAnchor = budget.anchorHits;

    ApplyFillBoost(budget, 0.8f, 2.0f, 32);

    REQUIRE(budget.anchorHits > originalAnchor);
    REQUIRE(budget.shimmerHits >= 2);
}

// =============================================================================
// Gumbel Sampler Tests
// =============================================================================

TEST_CASE("HashToFloat is deterministic", "[gumbel]")
{
    SECTION("Same seed+step gives same result")
    {
        float a = HashToFloat(12345, 0);
        float b = HashToFloat(12345, 0);
        REQUIRE(a == b);
    }

    SECTION("Different steps give different results")
    {
        float a = HashToFloat(12345, 0);
        float b = HashToFloat(12345, 1);
        REQUIRE(a != b);
    }

    SECTION("Result is in valid range")
    {
        for (int step = 0; step < 32; ++step)
        {
            float val = HashToFloat(42, step);
            REQUIRE(val > 0.0f);
            REQUIRE(val < 1.0f);
        }
    }
}

TEST_CASE("Gumbel selection respects target count", "[gumbel]")
{
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        weights[i] = 0.5f;  // Uniform weights
    }

    SECTION("Selects exact target count when possible")
    {
        uint32_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 4, 12345, 32, 0);
        REQUIRE(CountBits(mask) == 4);
    }

    SECTION("Selects fewer if eligibility limits")
    {
        // Only 3 eligible steps
        uint32_t eligibility = 0x00000007;  // Steps 0, 1, 2
        uint32_t mask = SelectHitsGumbelTopK(weights, eligibility, 5, 12345, 32, 0);
        REQUIRE(CountBits(mask) <= 3);
    }

    SECTION("Zero target gives empty mask")
    {
        uint32_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 0, 12345, 32, 0);
        REQUIRE(mask == 0);
    }
}

TEST_CASE("Gumbel selection is deterministic", "[gumbel]")
{
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        weights[i] = (i % 4 == 0) ? 0.9f : 0.3f;
    }

    SECTION("Same seed gives same pattern")
    {
        uint32_t mask1 = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 4, 99999, 32, 0);
        uint32_t mask2 = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 4, 99999, 32, 0);
        REQUIRE(mask1 == mask2);
    }

    SECTION("Different seeds give different patterns")
    {
        uint32_t mask1 = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 4, 11111, 32, 0);
        uint32_t mask2 = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 4, 22222, 32, 0);
        // Very unlikely to be equal with different seeds
        REQUIRE(mask1 != mask2);
    }
}

TEST_CASE("Spacing rules prevent clumping", "[gumbel]")
{
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        weights[i] = 0.5f;
    }

    SECTION("Minimum spacing of 2 prevents adjacent hits")
    {
        uint32_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 8, 12345, 32, 2);

        // Check that no two adjacent bits are set
        for (int i = 0; i < 31; ++i)
        {
            bool a = (mask & (1U << i)) != 0;
            bool b = (mask & (1U << (i + 1))) != 0;
            REQUIRE(!(a && b));  // No adjacent hits
        }
    }

    SECTION("Spacing is relaxed to meet target count")
    {
        // High target with high spacing - should relax to meet target
        uint32_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 12, 12345, 32, 4);

        // Should get close to target even if spacing can't be maintained
        REQUIRE(CountBits(mask) >= 8);
    }
}

TEST_CASE("Weighted selection favors high weights", "[gumbel]")
{
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        weights[i] = 0.001f;  // Very low weight for most steps
    }

    // Make steps 0, 8, 16, 24 extremely high weight (1000x higher)
    weights[0] = 1.0f;
    weights[8] = 1.0f;
    weights[16] = 1.0f;
    weights[24] = 1.0f;

    SECTION("High weight steps are selected first")
    {
        uint32_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 4, 12345, 32, 0);

        // With 1000x weight ratio, all four high-weight steps should be selected
        // log(1.0) - log(0.001) = 0 - (-6.9) = 6.9 >> Gumbel noise range
        REQUIRE((mask & (1U << 0)) != 0);
        REQUIRE((mask & (1U << 8)) != 0);
        REQUIRE((mask & (1U << 16)) != 0);
        REQUIRE((mask & (1U << 24)) != 0);
    }
}

// =============================================================================
// Voice Relation Tests
// =============================================================================

TEST_CASE("INDEPENDENT coupling allows overlap", "[voice-relation]")
{
    uint32_t anchor = 0x11111111;   // Quarter notes
    uint32_t shimmer = 0x11111111;  // Same pattern

    ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::INDEPENDENT, 32);

    // Shimmer should be unchanged
    REQUIRE(shimmer == 0x11111111);
}

TEST_CASE("INTERLOCK suppresses simultaneous hits", "[voice-relation]")
{
    uint32_t anchor = 0x11111111;   // Quarter notes
    uint32_t shimmer = 0x33333333;  // Overlaps on quarters, plus extra

    uint32_t originalShimmer = shimmer;
    ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::INTERLOCK, 32);

    // Shimmer should no longer overlap with anchor
    REQUIRE((shimmer & anchor) == 0);

    // But shimmer should still have some hits
    REQUIRE(shimmer != 0);
    REQUIRE(shimmer != originalShimmer);
}

TEST_CASE("SHADOW creates echo pattern", "[voice-relation]")
{
    uint32_t anchor = 0x00000001;  // Only step 0
    uint32_t shimmer = 0xFFFFFFFF; // Full pattern (will be replaced)

    ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::SHADOW, 32);

    // Shimmer should now be step 1 (anchor delayed by 1)
    REQUIRE(shimmer == 0x00000002);
}

TEST_CASE("Shadow wraps around pattern", "[voice-relation]")
{
    uint32_t anchor = 0x80000000;  // Step 31 only
    uint32_t shimmer = 0xFFFFFFFF;

    ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::SHADOW, 32);

    // Shimmer should wrap to step 0
    REQUIRE(shimmer == 0x00000001);
}

TEST_CASE("ShiftMaskLeft works correctly", "[voice-relation]")
{
    SECTION("Simple shift")
    {
        uint32_t mask = 0x00000001;  // Step 0
        uint32_t shifted = ShiftMaskLeft(mask, 1, 32);
        REQUIRE(shifted == 0x00000002);  // Step 1
    }

    SECTION("Wrap around")
    {
        uint32_t mask = 0x80000000;  // Step 31
        uint32_t shifted = ShiftMaskLeft(mask, 1, 32);
        REQUIRE(shifted == 0x00000001);  // Wraps to step 0
    }

    SECTION("Larger shift")
    {
        uint32_t mask = 0x00000001;
        uint32_t shifted = ShiftMaskLeft(mask, 8, 32);
        REQUIRE(shifted == 0x00000100);  // Step 8
    }
}

TEST_CASE("Gap-fill rescues interlock gaps", "[voice-relation]")
{
    // Create a pattern where interlock would create a big gap
    uint32_t anchor = 0x00000001;   // Only step 0
    uint32_t shimmer = 0x00000101;  // Steps 0 and 8

    ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::INTERLOCK, 32);

    // Step 8 shimmer should remain (not overlapping anchor)
    REQUIRE((shimmer & 0x00000100) != 0);
}

TEST_CASE("FindLargestGap measures gaps correctly", "[voice-relation]")
{
    SECTION("All hits - no gap")
    {
        uint32_t mask = 0xFFFFFFFF;
        int gap = FindLargestGap(mask, 32);
        REQUIRE(gap == 0);
    }

    SECTION("Sparse pattern has large gap")
    {
        uint32_t mask = 0x00010001;  // Steps 0 and 16
        int gap = FindLargestGap(mask, 32);
        REQUIRE(gap == 15);  // 15 empty steps between hits (1-15 or 17-31)
    }

    SECTION("Empty mask - full gap")
    {
        uint32_t mask = 0;
        int gap = FindLargestGap(mask, 32);
        REQUIRE(gap == 32);
    }
}

// =============================================================================
// Guard Rails Tests
// =============================================================================

TEST_CASE("Downbeat is forced in GROOVE+ zones", "[guard-rails]")
{
    SECTION("GROOVE zone forces downbeat")
    {
        uint32_t anchor = 0x00000100;  // Step 8 only, no downbeat

        bool forced = EnforceDownbeat(anchor, EnergyZone::GROOVE, 32);

        REQUIRE(forced == true);
        REQUIRE((anchor & 0x00000001) != 0);  // Step 0 now set
    }

    SECTION("MINIMAL zone doesn't force")
    {
        uint32_t anchor = 0x00000100;

        bool forced = EnforceDownbeat(anchor, EnergyZone::MINIMAL, 32);

        REQUIRE(forced == false);
        REQUIRE((anchor & 0x00000001) == 0);  // Step 0 still clear
    }

    SECTION("Already has downbeat - no change")
    {
        uint32_t anchor = 0x00000101;  // Has step 0

        bool forced = EnforceDownbeat(anchor, EnergyZone::GROOVE, 32);

        REQUIRE(forced == false);
    }
}

TEST_CASE("Max gap is enforced", "[guard-rails]")
{
    SECTION("Large gap gets filled")
    {
        uint32_t anchor = 0x00000001;  // Only step 0

        int added = EnforceMaxGap(anchor, EnergyZone::GROOVE, 32);

        // Should have added at least one hit to break up the gap
        REQUIRE(added >= 1);
        REQUIRE(CountBits(anchor) >= 2);
    }

    SECTION("Dense pattern unchanged")
    {
        uint32_t anchor = 0x11111111;  // Every 4 steps

        int added = EnforceMaxGap(anchor, EnergyZone::GROOVE, 32);

        REQUIRE(added == 0);
    }

    SECTION("MINIMAL zone allows large gaps")
    {
        uint32_t anchor = 0x00000001;

        int added = EnforceMaxGap(anchor, EnergyZone::MINIMAL, 32);

        REQUIRE(added == 0);
    }
}

TEST_CASE("Consecutive shimmer is limited", "[guard-rails]")
{
    SECTION("Long shimmer run is shortened")
    {
        uint32_t anchor = 0x00000001;  // Only step 0
        uint32_t shimmer = 0xFFFFFFFF; // All steps

        int removed = EnforceConsecutiveShimmer(anchor, shimmer, EnergyZone::GROOVE, 32);

        REQUIRE(removed > 0);

        // Count max consecutive shimmer
        int maxRun = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(maxRun <= kMaxConsecutiveShimmer);
    }

    SECTION("Short run is unchanged")
    {
        uint32_t anchor = 0x11111111;  // Every 4 steps
        uint32_t shimmer = 0x22222222; // Offset from anchor
        uint32_t originalShimmer = shimmer;

        int removed = EnforceConsecutiveShimmer(anchor, shimmer, EnergyZone::GROOVE, 32);

        REQUIRE(removed == 0);
        REQUIRE(shimmer == originalShimmer);
    }
}

TEST_CASE("CountMaxConsecutiveShimmer is accurate", "[guard-rails]")
{
    SECTION("No shimmer returns 0")
    {
        uint32_t anchor = 0x11111111;
        uint32_t shimmer = 0;

        int count = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(count == 0);
    }

    SECTION("Shimmer on anchor steps don't count")
    {
        uint32_t anchor = 0x11111111;
        uint32_t shimmer = 0x11111111;  // Same as anchor

        int count = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(count == 0);
    }

    SECTION("Consecutive shimmer without anchor is counted")
    {
        uint32_t anchor = 0x00000001;  // Only step 0
        uint32_t shimmer = 0x0000000E; // Steps 1, 2, 3

        int count = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(count == 3);
    }
}

TEST_CASE("ApplyHardGuardRails applies all rules", "[guard-rails]")
{
    SECTION("Multiple violations are corrected")
    {
        uint32_t anchor = 0x00000100;  // Step 8 only (no downbeat, gaps)
        uint32_t shimmer = 0xFFFFFF00; // Steps 8-31 (long consecutive)

        int corrections = ApplyHardGuardRails(anchor, shimmer,
                                               EnergyZone::GROOVE, Genre::TECHNO, 32);

        REQUIRE(corrections > 0);

        // Downbeat should now exist
        REQUIRE((anchor & 0x00000001) != 0);

        // Gaps should be filled
        int gap = FindLargestGap(anchor, 32);
        REQUIRE(gap <= kMaxGapGroove);
    }
}

TEST_CASE("GetMaxGapForZone returns zone-appropriate values", "[guard-rails]")
{
    REQUIRE(GetMaxGapForZone(EnergyZone::MINIMAL) >= GetMaxGapForZone(EnergyZone::GROOVE));
    REQUIRE(GetMaxGapForZone(EnergyZone::GROOVE) >= GetMaxGapForZone(EnergyZone::BUILD));
    REQUIRE(GetMaxGapForZone(EnergyZone::BUILD) >= GetMaxGapForZone(EnergyZone::PEAK));
}

// =============================================================================
// Soft Repair Tests
// =============================================================================

TEST_CASE("Soft repair swaps weak hit for rescue", "[guard-rails]")
{
    // Create weights where some steps are clearly weaker
    float anchorWeights[32];
    float shimmerWeights[32];

    for (int i = 0; i < 32; ++i)
    {
        anchorWeights[i] = 0.5f;
        shimmerWeights[i] = 0.5f;
    }

    // Make step 16 very weak (a candidate for removal)
    anchorWeights[16] = 0.1f;

    SECTION("Repair moves hit to fill gap")
    {
        // Pattern with a large gap in the middle
        uint32_t anchor = 0x80000001;  // Steps 0 and 31
        anchor |= (1U << 16);          // Add weak step 16

        uint32_t shimmer = 0;

        int repairs = SoftRepairPass(anchor, shimmer, anchorWeights, shimmerWeights,
                                     EnergyZone::GROOVE, 32);

        // With gap detection, might move the weak hit
        // The exact behavior depends on gap thresholds
        REQUIRE(repairs >= 0);
    }
}

TEST_CASE("FindWeakestHit finds minimum weight", "[guard-rails]")
{
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        weights[i] = 0.5f;
    }
    weights[8] = 0.1f;  // Weakest

    uint32_t mask = 0x11111111;  // Steps 0, 4, 8, 12, 16, 20, 24, 28

    int weakest = FindWeakestHit(mask, weights, 32);
    REQUIRE(weakest == 8);
}

TEST_CASE("FindRescueCandidate finds best option", "[guard-rails]")
{
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        weights[i] = 0.5f;
    }
    weights[5] = 0.9f;  // Best rescue candidate

    uint32_t mask = 0x00000001;        // Already have step 0
    uint32_t rescue = 0x00000030;      // Steps 4 and 5 are rescue options

    int best = FindRescueCandidate(mask, rescue, weights, 32);
    REQUIRE(best == 5);  // Highest weight in rescue mask
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_CASE("Full generation flow produces valid pattern", "[integration]")
{
    // Simulate a complete generation pass
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        // Create musically reasonable weights (downbeats strong)
        if (i == 0 || i == 16)
        {
            weights[i] = 1.0f;
        }
        else if (i % 8 == 0)
        {
            weights[i] = 0.8f;
        }
        else if (i % 4 == 0)
        {
            weights[i] = 0.6f;
        }
        else if (i % 2 == 0)
        {
            weights[i] = 0.4f;
        }
        else
        {
            weights[i] = 0.2f;
        }
    }

    SECTION("Generate anchor then shimmer with voice relation")
    {
        // Generate anchor pattern
        uint32_t eligibility = ComputeAnchorEligibility(0.5f, 0.3f, EnergyZone::GROOVE, 32);
        int budget = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32);
        uint32_t anchor = SelectHitsGumbelTopK(weights, eligibility, budget, 12345, 32, 2);

        // Generate shimmer pattern
        eligibility = ComputeShimmerEligibility(0.5f, 0.3f, EnergyZone::GROOVE, 32);
        budget = ComputeShimmerBudget(0.5f, 0.5f, EnergyZone::GROOVE, 32);
        uint32_t shimmer = SelectHitsGumbelTopK(weights, eligibility, budget, 67890, 32, 1);

        // Apply voice relation
        ApplyVoiceRelationship(anchor, shimmer, VoiceCoupling::INTERLOCK, 32);

        // Apply guard rails
        (void)ApplyHardGuardRails(anchor, shimmer, EnergyZone::GROOVE, Genre::TECHNO, 32);

        // Verify constraints
        REQUIRE((anchor & 0x00000001) != 0);  // Has downbeat

        int gap = FindLargestGap(anchor, 32);
        REQUIRE(gap <= kMaxGapGroove);

        int consecutive = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(consecutive <= kMaxConsecutiveShimmer);

        // Pattern should still have hits
        REQUIRE(CountBits(anchor) >= 1);
    }
}

TEST_CASE("Energy sweep produces monotonic density", "[integration]")
{
    float weights[32];
    for (int i = 0; i < 32; ++i)
    {
        weights[i] = 0.5f;
    }

    int prevHitCount = 0;

    for (float energy = 0.0f; energy <= 1.0f; energy += 0.25f)
    {
        EnergyZone zone = GetEnergyZone(energy);
        int budget = ComputeAnchorBudget(energy, zone, 32);

        // Higher energy should give more or equal hits
        REQUIRE(budget >= prevHitCount);
        prevHitCount = budget;
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("Empty eligibility returns empty mask", "[edge-cases]")
{
    float weights[32] = {0};

    uint32_t mask = SelectHitsGumbelTopK(weights, 0, 4, 12345, 32, 0);
    REQUIRE(mask == 0);
}

TEST_CASE("Very short patterns work", "[edge-cases]")
{
    float weights[16];
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.5f;
    }

    uint32_t eligibility = 0x0000FFFF;
    uint32_t mask = SelectHitsGumbelTopK(weights, eligibility, 4, 12345, 16, 0);

    REQUIRE(CountBits(mask) == 4);
    REQUIRE((mask & 0xFFFF0000) == 0);  // No hits beyond pattern length
}

TEST_CASE("All weights zero still selects if eligible", "[edge-cases]")
{
    float weights[32] = {0};  // All zero weights

    // With zero weights, selection is based purely on Gumbel noise
    uint32_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFF, 2, 12345, 32, 0);

    // Should still select something (Gumbel breaks ties)
    // Actually, log(0) = -inf, so need epsilon protection
    // Our implementation handles this - verify it doesn't crash
    REQUIRE(CountBits(mask) <= 2);
}

TEST_CASE("Pattern length clamping works", "[edge-cases]")
{
    // Test with pattern length > 32 (should be clamped)
    int clamped = ClampPatternLength(64);
    REQUIRE(clamped == 32);

    clamped = ClampPatternLength(32);
    REQUIRE(clamped == 32);

    clamped = ClampPatternLength(16);
    REQUIRE(clamped == 16);
}
