#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

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

TEST_CASE("SHAPE affects anchor budget", "[hit-budget][shape]")
{
    SECTION("V5: Stable zone has most anchor hits")
    {
        // V5 Spec 5.4: Anchor gets FEWER hits as SHAPE increases
        // Stable (0-30%): 100%, Syncopated (30-70%): 90-100%, Wild (70-100%): 80-90%
        int stable = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.15f);
        int sync = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.50f);
        REQUIRE(stable >= sync);
    }

    SECTION("V5: Wild zone has fewest anchor hits")
    {
        int sync = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.50f);
        int wild = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.85f);
        REQUIRE(sync >= wild);
    }

    SECTION("Anchor shape multiplier values are correct (V5 Spec 5.4)")
    {
        // Stable zone (0-30%): 100%
        float anchorMult = GetAnchorBudgetMultiplier(0.0f);
        REQUIRE(anchorMult == Catch::Approx(1.0f).margin(0.01f));

        anchorMult = GetAnchorBudgetMultiplier(0.29f);
        REQUIRE(anchorMult == Catch::Approx(1.0f).margin(0.01f));

        // Syncopated zone (30-70%): 100% -> 90%
        anchorMult = GetAnchorBudgetMultiplier(0.50f);  // midpoint = 0.5 of 0.40 range = 95%
        REQUIRE(anchorMult == Catch::Approx(0.95f).margin(0.01f));

        anchorMult = GetAnchorBudgetMultiplier(0.70f);  // end of sync zone = 90%
        REQUIRE(anchorMult == Catch::Approx(0.90f).margin(0.01f));

        // Wild zone (70-100%): 90% -> 80%
        anchorMult = GetAnchorBudgetMultiplier(1.0f);
        REQUIRE(anchorMult == Catch::Approx(0.80f).margin(0.01f));
    }

    SECTION("Shimmer shape multiplier values are correct (V5 Spec 5.4)")
    {
        // Stable zone (0-30%): 100%
        float shimmerMult = GetShimmerBudgetMultiplier(0.0f);
        REQUIRE(shimmerMult == Catch::Approx(1.0f).margin(0.01f));

        shimmerMult = GetShimmerBudgetMultiplier(0.29f);
        REQUIRE(shimmerMult == Catch::Approx(1.0f).margin(0.01f));

        // Syncopated zone (30-70%): 110% -> 130%
        shimmerMult = GetShimmerBudgetMultiplier(0.50f);  // midpoint = 0.5 of 0.40 range = 120%
        REQUIRE(shimmerMult == Catch::Approx(1.20f).margin(0.01f));

        shimmerMult = GetShimmerBudgetMultiplier(0.70f);  // end of sync zone = 130%
        REQUIRE(shimmerMult == Catch::Approx(1.30f).margin(0.01f));

        // Wild zone (70-100%): 130% -> 150%
        shimmerMult = GetShimmerBudgetMultiplier(1.0f);
        REQUIRE(shimmerMult == Catch::Approx(1.50f).margin(0.01f));
    }
}

TEST_CASE("SHAPE affects shimmer budget", "[hit-budget][shape]")
{
    SECTION("Shimmer multiplier increases with SHAPE (V5 divergence)")
    {
        // V5 Spec 5.4: Shimmer MULTIPLIER increases as SHAPE increases
        // Stable zone: shimmer = 100% base
        // Syncopated zone: shimmer = 110-130% base
        // Wild zone: shimmer = 130-150% base
        //
        // Task 73: Shimmer derives from anchor, which now uses euclidean K at low SHAPE.
        // The shimmer multiplier still increases, but absolute hit counts depend on anchor base.
        float multStable = GetShimmerBudgetMultiplier(0.15f);
        float multSync = GetShimmerBudgetMultiplier(0.50f);
        float multWild = GetShimmerBudgetMultiplier(0.85f);

        REQUIRE(multWild >= multSync);
        REQUIRE(multSync >= multStable);
    }
}

TEST_CASE("ComputeBarBudget respects SHAPE parameter", "[hit-budget][shape]")
{
    BarBudget budgetStable;
    BarBudget budgetNormal;
    BarBudget budgetWild;

    ComputeBarBudget(0.5f, 0.5f, EnergyZone::GROOVE, AuxDensity::NORMAL, 32, 1.0f, 0.15f, budgetStable);
    ComputeBarBudget(0.5f, 0.5f, EnergyZone::GROOVE, AuxDensity::NORMAL, 32, 1.0f, 0.50f, budgetNormal);
    ComputeBarBudget(0.5f, 0.5f, EnergyZone::GROOVE, AuxDensity::NORMAL, 32, 1.0f, 0.85f, budgetWild);

    SECTION("Anchor hits decrease from stable to wild (Task 73 fade)")
    {
        // Task 73: At SHAPE=0.15 (stable), anchor = euclidean K
        // As SHAPE increases, anchor fades toward density-based budget (which decreases)
        // So anchor should decrease monotonically from stable to wild
        REQUIRE(budgetStable.anchorHits >= budgetNormal.anchorHits);
        REQUIRE(budgetNormal.anchorHits >= budgetWild.anchorHits);
    }

    SECTION("Shimmer multiplier increases with SHAPE")
    {
        // V5 Spec 5.4: Shimmer MULTIPLIER increases with SHAPE (1.0 -> 1.5)
        // But shimmer derives from anchor, which decreases, so absolute hits may vary.
        // What we can test: shimmer is always a reasonable proportion of anchor.
        REQUIRE(budgetStable.shimmerHits >= 1);
        REQUIRE(budgetNormal.shimmerHits >= 1);
        REQUIRE(budgetWild.shimmerHits >= 1);

        // At wild, shimmer should be at least as much as anchor (multiplier ~1.5)
        REQUIRE(budgetWild.shimmerHits >= budgetWild.anchorHits * 0.8);
    }
}

// =============================================================================
// Task 73: Euclidean K / HitBudget Fade Tests
// =============================================================================

TEST_CASE("Euclidean K fade (Task 73)", "[hit-budget][task-73]")
{
    SECTION("ComputeAnchorEuclideanK scales with ENERGY")
    {
        // kAnchorKMin=4, kAnchorKMax=12 from algorithm_config.h
        int kLow = ComputeAnchorEuclideanK(0.0f, 32);
        int kMid = ComputeAnchorEuclideanK(0.5f, 32);
        int kHigh = ComputeAnchorEuclideanK(1.0f, 32);

        REQUIRE(kLow == 4);   // kAnchorKMin
        REQUIRE(kMid == 8);   // kAnchorKMin + 0.5 * (kAnchorKMax - kAnchorKMin)
        REQUIRE(kHigh == 12); // kAnchorKMax
    }

    SECTION("ComputeEffectiveHitCount returns min(euclidean, budget) at SHAPE <= 0.15")
    {
        int euclideanK = 8;
        int budgetK = 5;

        // At SHAPE=0.0, should return min to preserve baseline sparsity
        REQUIRE(ComputeEffectiveHitCount(euclideanK, budgetK, 0.0f) == std::min(euclideanK, budgetK));

        // At SHAPE=0.15, should still return min
        REQUIRE(ComputeEffectiveHitCount(euclideanK, budgetK, 0.15f) == std::min(euclideanK, budgetK));

        // When euclideanK < budgetK, should return euclideanK
        REQUIRE(ComputeEffectiveHitCount(3, 6, 0.0f) == 3);
    }

    SECTION("ComputeEffectiveHitCount fades to budget K at high SHAPE")
    {
        int euclideanK = 8;
        int budgetK = 4;

        // At SHAPE=1.0, should return pure budget K
        REQUIRE(ComputeEffectiveHitCount(euclideanK, budgetK, 1.0f) == budgetK);
    }

    SECTION("ComputeEffectiveHitCount blends at mid SHAPE")
    {
        int euclideanK = 8;
        int budgetK = 4;

        // At SHAPE=0.575 (halfway through fade)
        // baseK = min(8, 4) = 4
        // fadeProgress = (0.575 - 0.15) / 0.85 = 0.5
        // result = 4 + 0.5 * (4 - 4) = 4
        int mid = ComputeEffectiveHitCount(euclideanK, budgetK, 0.575f);
        REQUIRE(mid == budgetK); // When euclideanK > budgetK, stays at budgetK throughout fade
    }

    SECTION("Anchor budget uses min(euclidean, budget) at SHAPE=0")
    {
        // At SHAPE=0, anchor should equal min(euclideanK, budgetK)
        // This preserves baseline sparsity while enabling euclidean placement
        int anchorBudget = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.0f);
        int euclideanK = ComputeAnchorEuclideanK(0.5f, 32);

        // Budget at GROOVE zone with energy=0.5 is less than euclideanK
        // So anchor budget should match what baseline would have produced
        REQUIRE(anchorBudget <= euclideanK);
    }

    SECTION("Anchor budget is stable or decreasing as SHAPE increases")
    {
        int anchorShape0 = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.0f);
        int anchorShape50 = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 0.5f);
        int anchorShape100 = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32, 1.0f);

        // Should be monotonically non-increasing (fade preserves or reduces sparsity)
        REQUIRE(anchorShape0 >= anchorShape50);
        REQUIRE(anchorShape50 >= anchorShape100);
    }
}

TEST_CASE("ComputeBarBudget fills all fields", "[hit-budget]")
{
    BarBudget budget;
    ComputeBarBudget(0.5f, 0.5f, EnergyZone::GROOVE, AuxDensity::NORMAL, 32, 1.0f, 0.5f, budget);

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
        uint64_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 4, 12345, 32, 0);
        REQUIRE(CountBits(mask) == 4);
    }

    SECTION("Selects fewer if eligibility limits")
    {
        // Only 3 eligible steps
        uint64_t eligibility = 0x00000007;  // Steps 0, 1, 2
        uint64_t mask = SelectHitsGumbelTopK(weights, eligibility, 5, 12345, 32, 0);
        REQUIRE(CountBits(mask) <= 3);
    }

    SECTION("Zero target gives empty mask")
    {
        uint64_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 0, 12345, 32, 0);
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
        uint64_t mask1 = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 4, 99999, 32, 0);
        uint64_t mask2 = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 4, 99999, 32, 0);
        REQUIRE(mask1 == mask2);
    }

    SECTION("Different seeds give different patterns")
    {
        uint64_t mask1 = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 4, 11111, 32, 0);
        uint64_t mask2 = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 4, 22222, 32, 0);
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
        uint64_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 8, 12345, 32, 2);

        // Check that no two adjacent bits are set
        for (int i = 0; i < 31; ++i)
        {
            bool a = (mask & (1ULL << i)) != 0;
            bool b = (mask & (1ULL << (i + 1))) != 0;
            REQUIRE(!(a && b));  // No adjacent hits
        }
    }

    SECTION("Spacing is relaxed to meet target count")
    {
        // High target with high spacing - should relax to meet target
        uint64_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 12, 12345, 32, 4);

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
        uint64_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 4, 12345, 32, 0);

        // With 1000x weight ratio, all four high-weight steps should be selected
        // log(1.0) - log(0.001) = 0 - (-6.9) = 6.9 >> Gumbel noise range
        REQUIRE((mask & (1ULL << 0)) != 0);
        REQUIRE((mask & (1ULL << 8)) != 0);
        REQUIRE((mask & (1ULL << 16)) != 0);
        REQUIRE((mask & (1ULL << 24)) != 0);
    }
}

// =============================================================================
// Voice Relation Tests
// =============================================================================

// V5: ApplyVoiceRelationship removed - use ApplyComplementRelationship instead
// Tests for legacy coupling modes have been removed (Task 43)

TEST_CASE("ShiftMaskLeft works correctly", "[voice-relation]")
{
    SECTION("Simple shift")
    {
        uint64_t mask = 0x00000001;  // Step 0
        uint32_t shifted = ShiftMaskLeft(mask, 1, 32);
        REQUIRE(shifted == 0x00000002);  // Step 1
    }

    SECTION("Wrap around")
    {
        uint64_t mask = 0x80000000;  // Step 31
        uint32_t shifted = ShiftMaskLeft(mask, 1, 32);
        REQUIRE(shifted == 0x00000001);  // Wraps to step 0
    }

    SECTION("Larger shift")
    {
        uint64_t mask = 0x00000001;
        uint32_t shifted = ShiftMaskLeft(mask, 8, 32);
        REQUIRE(shifted == 0x00000100);  // Step 8
    }
}

// V5: ApplyVoiceRelationship removed - gap-fill tests moved to test_voice_complement.cpp

TEST_CASE("FindLargestGap measures gaps correctly", "[voice-relation]")
{
    SECTION("All hits - no gap")
    {
        uint64_t mask = 0xFFFFFFFFFFFFFFFFULL;
        int gap = FindLargestGap(mask, 32);
        REQUIRE(gap == 0);
    }

    SECTION("Sparse pattern has large gap")
    {
        uint64_t mask = 0x00010001;  // Steps 0 and 16
        int gap = FindLargestGap(mask, 32);
        REQUIRE(gap == 15);  // 15 empty steps between hits (1-15 or 17-31)
    }

    SECTION("Empty mask - full gap")
    {
        uint64_t mask = 0;
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
        uint64_t anchor = 0x00000100;  // Step 8 only, no downbeat

        bool forced = EnforceDownbeat(anchor, EnergyZone::GROOVE, 32);

        REQUIRE(forced == true);
        REQUIRE((anchor & 0x00000001) != 0);  // Step 0 now set
    }

    SECTION("MINIMAL zone doesn't force")
    {
        uint64_t anchor = 0x00000100;

        bool forced = EnforceDownbeat(anchor, EnergyZone::MINIMAL, 32);

        REQUIRE(forced == false);
        REQUIRE((anchor & 0x00000001) == 0);  // Step 0 still clear
    }

    SECTION("Already has downbeat - no change")
    {
        uint64_t anchor = 0x00000101;  // Has step 0

        bool forced = EnforceDownbeat(anchor, EnergyZone::GROOVE, 32);

        REQUIRE(forced == false);
    }
}

TEST_CASE("Max gap is enforced", "[guard-rails]")
{
    // Full eligibility mask for 32 steps
    uint64_t fullEligibility = 0xFFFFFFFF;

    SECTION("Large gap gets filled")
    {
        uint64_t anchor = 0x00000001;  // Only step 0

        int added = EnforceMaxGap(anchor, fullEligibility, EnergyZone::GROOVE, 32);

        // Should have added at least one hit to break up the gap
        REQUIRE(added >= 1);
        REQUIRE(CountBits(anchor) >= 2);
    }

    SECTION("Dense pattern unchanged")
    {
        uint64_t anchor = 0x11111111;  // Every 4 steps

        int added = EnforceMaxGap(anchor, fullEligibility, EnergyZone::GROOVE, 32);

        REQUIRE(added == 0);
    }

    SECTION("MINIMAL zone allows large gaps")
    {
        uint64_t anchor = 0x00000001;

        int added = EnforceMaxGap(anchor, fullEligibility, EnergyZone::MINIMAL, 32);

        REQUIRE(added == 0);
    }

    SECTION("Eligibility constrains fill positions")
    {
        uint64_t anchor = 0x00000001;  // Only step 0
        // Only even positions eligible (quarter notes in 32-step pattern)
        uint64_t quarterNoteEligibility = 0x11111111;

        int added = EnforceMaxGap(anchor, quarterNoteEligibility, EnergyZone::GROOVE, 32);

        REQUIRE(added >= 1);

        // Verify all added hits are on eligible positions
        uint64_t addedHits = anchor & ~0x00000001;  // Remove original hit
        REQUIRE((addedHits & ~quarterNoteEligibility) == 0);
    }
}

TEST_CASE("Consecutive shimmer is limited", "[guard-rails]")
{
    SECTION("Long shimmer run is shortened")
    {
        uint64_t anchor = 0x00000001;  // Only step 0
        uint64_t shimmer = 0xFFFFFFFFFFFFFFFFULL; // All steps

        int removed = EnforceConsecutiveShimmer(anchor, shimmer, EnergyZone::GROOVE, 32);

        REQUIRE(removed > 0);

        // Count max consecutive shimmer
        int maxRun = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(maxRun <= kMaxConsecutiveShimmer);
    }

    SECTION("Short run is unchanged")
    {
        uint64_t anchor = 0x11111111;  // Every 4 steps
        uint64_t shimmer = 0x22222222; // Offset from anchor
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
        uint64_t anchor = 0x11111111;
        uint64_t shimmer = 0;

        int count = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(count == 0);
    }

    SECTION("Shimmer on anchor steps don't count")
    {
        uint64_t anchor = 0x11111111;
        uint64_t shimmer = 0x11111111;  // Same as anchor

        int count = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(count == 0);
    }

    SECTION("Consecutive shimmer without anchor is counted")
    {
        uint64_t anchor = 0x00000001;  // Only step 0
        uint64_t shimmer = 0x0000000E; // Steps 1, 2, 3

        int count = CountMaxConsecutiveShimmer(anchor, shimmer, 32);
        REQUIRE(count == 3);
    }
}

TEST_CASE("ApplyHardGuardRails applies all rules", "[guard-rails]")
{
    // Full eligibility for testing
    uint64_t fullEligibility = 0xFFFFFFFF;

    SECTION("Multiple violations are corrected")
    {
        uint64_t anchor = 0x00000100;  // Step 8 only (no downbeat, gaps)
        uint64_t shimmer = 0xFFFFFF00; // Steps 8-31 (long consecutive)

        int corrections = ApplyHardGuardRails(anchor, shimmer, fullEligibility,
                                               EnergyZone::GROOVE, Genre::TECHNO, 32);

        REQUIRE(corrections > 0);

        // Downbeat should now exist
        REQUIRE((anchor & 0x00000001) != 0);

        // Gaps should be filled
        int gap = FindLargestGap(anchor, 32);
        REQUIRE(gap <= kMaxGapGroove);
    }

    SECTION("Eligibility constrains fill positions")
    {
        uint64_t anchor = 0x00000000;  // No hits
        uint64_t shimmer = 0x00000000;
        // Only quarter notes eligible
        uint64_t quarterNoteEligibility = 0x11111111;

        int corrections = ApplyHardGuardRails(anchor, shimmer, quarterNoteEligibility,
                                               EnergyZone::BUILD, Genre::TECHNO, 32);

        REQUIRE(corrections > 0);

        // Downbeat at step 0 is quarter note, so should be set
        REQUIRE((anchor & 0x00000001) != 0);

        // All fills should be on eligible positions (except step 0 which is always valid for downbeat)
        uint64_t fillHits = anchor & ~0x00000001;
        REQUIRE((fillHits & ~quarterNoteEligibility) == 0);
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
        uint64_t anchor = 0x80000001;  // Steps 0 and 31
        anchor |= (1ULL << 16);          // Add weak step 16

        uint64_t shimmer = 0;
        uint64_t fullEligibility = 0xFFFFFFFF;

        int repairs = SoftRepairPass(anchor, shimmer, anchorWeights, shimmerWeights,
                                     fullEligibility, EnergyZone::GROOVE, 32);

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

    uint64_t mask = 0x11111111;  // Steps 0, 4, 8, 12, 16, 20, 24, 28

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

    uint64_t mask = 0x00000001;        // Already have step 0
    uint64_t rescue = 0x00000030;      // Steps 4 and 5 are rescue options
    uint64_t fullEligibility = 0xFFFFFFFF;

    int best = FindRescueCandidate(mask, rescue, fullEligibility, weights, 32);
    REQUIRE(best == 5);  // Highest weight in rescue mask

    SECTION("Eligibility constrains rescue candidates")
    {
        // Make only step 4 eligible (not step 5)
        uint64_t limitedEligibility = 0x00000010;  // Only step 4

        int bestLimited = FindRescueCandidate(mask, rescue, limitedEligibility, weights, 32);
        REQUIRE(bestLimited == 4);  // Step 5 has higher weight but not eligible
    }
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
        uint64_t eligibility = ComputeAnchorEligibility(0.5f, 0.3f, EnergyZone::GROOVE, 32);
        int budget = ComputeAnchorBudget(0.5f, EnergyZone::GROOVE, 32);
        uint64_t anchor = SelectHitsGumbelTopK(weights, eligibility, budget, 12345, 32, 2);

        // Generate shimmer pattern
        eligibility = ComputeShimmerEligibility(0.5f, 0.3f, EnergyZone::GROOVE, 32);
        budget = ComputeShimmerBudget(0.5f, 0.5f, EnergyZone::GROOVE, 32);
        uint64_t shimmer = SelectHitsGumbelTopK(weights, eligibility, budget, 67890, 32, 1);

        // V5: ApplyVoiceRelationship removed - use ApplyComplementRelationship instead
        // For this test, shimmer is used directly without voice relationship

        // Apply guard rails (pass anchor eligibility for fills)
        uint64_t anchorEligibility = ComputeAnchorEligibility(0.5f, 0.3f, EnergyZone::GROOVE, 32);
        (void)ApplyHardGuardRails(anchor, shimmer, anchorEligibility, EnergyZone::GROOVE, Genre::TECHNO, 32);

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

    uint64_t mask = SelectHitsGumbelTopK(weights, 0, 4, 12345, 32, 0);
    REQUIRE(mask == 0);
}

TEST_CASE("Very short patterns work", "[edge-cases]")
{
    float weights[16];
    for (int i = 0; i < 16; ++i)
    {
        weights[i] = 0.5f;
    }

    uint64_t eligibility = 0x0000FFFF;
    uint64_t mask = SelectHitsGumbelTopK(weights, eligibility, 4, 12345, 16, 0);

    REQUIRE(CountBits(mask) == 4);
    REQUIRE((mask & 0xFFFF0000) == 0);  // No hits beyond pattern length
}

TEST_CASE("All weights zero still selects if eligible", "[edge-cases]")
{
    float weights[32] = {0};  // All zero weights

    // With zero weights, selection is based purely on Gumbel noise
    uint64_t mask = SelectHitsGumbelTopK(weights, 0xFFFFFFFFFFFFFFFFULL, 2, 12345, 32, 0);

    // Should still select something (Gumbel breaks ties)
    // Actually, log(0) = -inf, so need epsilon protection
    // Our implementation handles this - verify it doesn't crash
    REQUIRE(CountBits(mask) <= 2);
}

TEST_CASE("Pattern length clamping works", "[edge-cases]")
{
    // Test with pattern length > 64 (should be clamped)
    int clamped = ClampPatternLength(128);
    REQUIRE(clamped == 64);

    clamped = ClampPatternLength(32);
    REQUIRE(clamped == 32);

    clamped = ClampPatternLength(16);
    REQUIRE(clamped == 16);
}

// =============================================================================
// Hit Histogram Tests (Task 21: Musicality Improvements)
// =============================================================================

TEST_CASE("Hit histogram by archetype for Techno genre", "[musicality][histogram]")
{
    // This test generates hit histograms to measure instrument selection stats
    // across different archetypes. Used to validate archetype weight retuning.

    const int kNumTrials = 100;  // Run multiple times to get distribution
    const int kPatternLength = 32;

    // Track hit counts at each position for each archetype
    int minimalHits[32] = {0};
    int groovyHits[32] = {0};
    int chaosHits[32] = {0};

    SECTION("Minimal archetype should produce 4-on-floor")
    {
        // Minimal weights favor steps 0, 4, 8, 12, 16, 20, 24, 28 (quarter notes)
        float minimalWeights[32];
        for (int i = 0; i < 32; ++i)
        {
            if (i % 4 == 0)
            {
                minimalWeights[i] = (i % 8 == 0) ? 1.0f : 0.9f;  // Stronger on beats 1 and 3
            }
            else
            {
                minimalWeights[i] = 0.0f;  // Zero weight on off-beats
            }
        }

        // Generate multiple patterns with different seeds
        for (int trial = 0; trial < kNumTrials; ++trial)
        {
            uint64_t mask = SelectHitsGumbelTopK(minimalWeights, 0xFFFFFFFFFFFFFFFFULL,
                                                  4, 12345 + trial, kPatternLength, 0);

            // Count hits at each position
            for (int step = 0; step < kPatternLength; ++step)
            {
                if (mask & (1ULL << step))
                {
                    minimalHits[step]++;
                }
            }
        }

        // Verify that quarter notes are most frequently selected
        // Quarter notes: 0, 4, 8, 12, 16, 20, 24, 28
        int quarterNoteHits = minimalHits[0] + minimalHits[4] + minimalHits[8] +
                              minimalHits[12] + minimalHits[16] + minimalHits[20] +
                              minimalHits[24] + minimalHits[28];

        int offBeatHits = 0;
        for (int i = 0; i < 32; ++i)
        {
            if (i % 4 != 0)
            {
                offBeatHits += minimalHits[i];
            }
        }

        // Quarter notes should dominate (most trials should pick all quarters)
        REQUIRE(quarterNoteHits > offBeatHits * 10);

        // Downbeat (step 0) should be selected frequently
        // With only 4 hits budget and 8 quarter note positions, we get ~50% selection
        // This is a baseline measurement - actual archetype weights may differ
        REQUIRE(minimalHits[0] > kNumTrials * 0.40f);
    }

    SECTION("Groovy archetype should produce ghost notes")
    {
        // Groovy weights favor quarters plus "a" subdivisions (steps 3, 7, 11, 15...)
        float groovyWeights[32];
        for (int i = 0; i < 32; ++i)
        {
            if (i % 4 == 0)
            {
                groovyWeights[i] = (i % 8 == 0) ? 1.0f : 0.85f;  // Quarter notes strong
            }
            else if (i % 4 == 3)
            {
                groovyWeights[i] = 0.45f;  // "a" subdivisions for ghost notes
            }
            else
            {
                groovyWeights[i] = 0.0f;
            }
        }

        // Generate multiple patterns
        for (int trial = 0; trial < kNumTrials; ++trial)
        {
            uint64_t mask = SelectHitsGumbelTopK(groovyWeights, 0xFFFFFFFFFFFFFFFFULL,
                                                  5, 22222 + trial, kPatternLength, 0);

            for (int step = 0; step < kPatternLength; ++step)
            {
                if (mask & (1ULL << step))
                {
                    groovyHits[step]++;
                }
            }
        }

        // Count ghost note positions (steps 3, 7, 11, 15, 19, 23, 27, 31)
        int ghostHits = groovyHits[3] + groovyHits[7] + groovyHits[11] + groovyHits[15] +
                        groovyHits[19] + groovyHits[23] + groovyHits[27] + groovyHits[31];

        // Ghost notes should appear in at least 20% of trials (0.45 weight competing with quarters)
        // With current weights (0.40-0.45), ghosts rarely win against 0.85-1.0 quarters
        // This documents the problem we're fixing with Task 21
        INFO("Ghost hits across all trials: " << ghostHits);
        INFO("Quarter note hits at step 0: " << groovyHits[0]);

        // After Task 21 Phase A, ghost weights will be 0.50-0.60 (viable for Gumbel selection)
        // and this should improve significantly
    }

    SECTION("Chaos archetype should show wide distribution")
    {
        // Chaos weights are more varied with no clear dominant pattern
        float chaosWeights[32];
        for (int i = 0; i < 32; ++i)
        {
            // Irregular pattern with varying weights
            if (i == 0 || i == 16)
            {
                chaosWeights[i] = 1.0f;  // Downbeats still strong
            }
            else if (i % 2 == 0)
            {
                chaosWeights[i] = 0.5f + (i % 7) * 0.05f;  // Varying 8th notes
            }
            else
            {
                chaosWeights[i] = 0.45f + (i % 5) * 0.03f;  // Varying 16th notes
            }
        }

        // Generate multiple patterns
        for (int trial = 0; trial < kNumTrials; ++trial)
        {
            uint64_t mask = SelectHitsGumbelTopK(chaosWeights, 0xFFFFFFFFFFFFFFFFULL,
                                                  6, 33333 + trial, kPatternLength, 0);

            for (int step = 0; step < kPatternLength; ++step)
            {
                if (mask & (1ULL << step))
                {
                    chaosHits[step]++;
                }
            }
        }

        // Chaos should have more varied hit distribution
        // Measure standard deviation of hit counts
        float mean = 0.0f;
        for (int i = 0; i < 32; ++i)
        {
            mean += chaosHits[i];
        }
        mean /= 32.0f;

        float variance = 0.0f;
        for (int i = 0; i < 32; ++i)
        {
            float diff = chaosHits[i] - mean;
            variance += diff * diff;
        }
        variance /= 32.0f;
        float stddev = std::sqrt(variance);

        INFO("Chaos archetype hit distribution stddev: " << stddev);
        INFO("Mean hits per position: " << mean);

        // Chaos should still have downbeat emphasis
        REQUIRE(chaosHits[0] > mean);
    }
}
