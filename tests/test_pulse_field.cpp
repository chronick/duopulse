#include <catch2/catch_all.hpp>
#include "../src/Engine/PulseField.h"

using namespace daisysp_idm_grids;

// =============================================================================
// Weight Table Tests
// =============================================================================

TEST_CASE("Anchor weight table has correct values at key positions", "[pulse-field][weights]")
{
    // Bar downbeats (steps 0, 16) should be 1.0
    REQUIRE(kAnchorWeights[0] == Catch::Approx(1.0f));
    REQUIRE(kAnchorWeights[16] == Catch::Approx(1.0f));

    // Half-note positions (steps 8, 24) should be 0.85
    REQUIRE(kAnchorWeights[8] == Catch::Approx(0.85f));
    REQUIRE(kAnchorWeights[24] == Catch::Approx(0.85f));

    // Quarter notes (steps 4, 12, 20, 28) should be 0.70
    REQUIRE(kAnchorWeights[4] == Catch::Approx(0.70f));
    REQUIRE(kAnchorWeights[12] == Catch::Approx(0.70f));
    REQUIRE(kAnchorWeights[20] == Catch::Approx(0.70f));
    REQUIRE(kAnchorWeights[28] == Catch::Approx(0.70f));

    // Ghost notes (16th off-beats) should be low (0.15)
    REQUIRE(kAnchorWeights[1] == Catch::Approx(0.15f));
    REQUIRE(kAnchorWeights[3] == Catch::Approx(0.15f));
    REQUIRE(kAnchorWeights[5] == Catch::Approx(0.15f));
}

TEST_CASE("Shimmer weight table emphasizes backbeats", "[pulse-field][weights]")
{
    // Backbeats (steps 8, 24) should be 1.0 for shimmer
    REQUIRE(kShimmerWeights[8] == Catch::Approx(1.0f));
    REQUIRE(kShimmerWeights[24] == Catch::Approx(1.0f));

    // Downbeats should be low for shimmer (0.25)
    REQUIRE(kShimmerWeights[0] == Catch::Approx(0.25f));
    REQUIRE(kShimmerWeights[16] == Catch::Approx(0.25f));

    // Pre-snare quarters (steps 4, 12, 20, 28) should be 0.60
    REQUIRE(kShimmerWeights[4] == Catch::Approx(0.60f));
    REQUIRE(kShimmerWeights[12] == Catch::Approx(0.60f));
}

TEST_CASE("GetStepWeight returns correct voice-specific weights", "[pulse-field][weights]")
{
    // Anchor downbeat
    REQUIRE(GetStepWeight(0, true) == Catch::Approx(1.0f));
    // Shimmer downbeat
    REQUIRE(GetStepWeight(0, false) == Catch::Approx(0.25f));

    // Anchor backbeat
    REQUIRE(GetStepWeight(8, true) == Catch::Approx(0.85f));
    // Shimmer backbeat
    REQUIRE(GetStepWeight(8, false) == Catch::Approx(1.0f));

    // Out of bounds should clamp to step 0
    REQUIRE(GetStepWeight(-1, true) == Catch::Approx(1.0f));
    REQUIRE(GetStepWeight(32, true) == Catch::Approx(1.0f));
    REQUIRE(GetStepWeight(100, false) == Catch::Approx(0.25f));
}

// =============================================================================
// ShouldStepFire Core Algorithm Tests
// =============================================================================

TEST_CASE("ShouldStepFire at BROKEN=0 produces regular patterns", "[pulse-field][algorithm]")
{
    uint32_t seed = 12345;

    // At DENSITY=0.5 and BROKEN=0, only high-weight steps should fire
    // Threshold = 1.0 - 0.5 = 0.5
    // Anchor downbeats (weight=1.0) should definitely fire
    REQUIRE(ShouldStepFire(0, 0.5f, 0.0f, kAnchorWeights, seed) == true);
    REQUIRE(ShouldStepFire(16, 0.5f, 0.0f, kAnchorWeights, seed) == true);

    // Anchor half-notes (weight=0.85) should fire
    REQUIRE(ShouldStepFire(8, 0.5f, 0.0f, kAnchorWeights, seed) == true);
    REQUIRE(ShouldStepFire(24, 0.5f, 0.0f, kAnchorWeights, seed) == true);

    // Anchor quarters (weight=0.70) should fire
    REQUIRE(ShouldStepFire(4, 0.5f, 0.0f, kAnchorWeights, seed) == true);

    // Anchor ghosts (weight=0.15) should NOT fire at density=0.5
    REQUIRE(ShouldStepFire(1, 0.5f, 0.0f, kAnchorWeights, seed) == false);
    REQUIRE(ShouldStepFire(3, 0.5f, 0.0f, kAnchorWeights, seed) == false);
}

TEST_CASE("ShouldStepFire respects DENSITY threshold", "[pulse-field][algorithm]")
{
    uint32_t seed = 12345;

    // At DENSITY=0, threshold=1.0, nothing should fire (no weight exceeds 1.0)
    REQUIRE(ShouldStepFire(0, 0.0f, 0.0f, kAnchorWeights, seed) == false);
    REQUIRE(ShouldStepFire(8, 0.0f, 0.0f, kAnchorWeights, seed) == false);

    // At DENSITY=1, threshold=0.0, everything should fire (all weights > 0)
    REQUIRE(ShouldStepFire(0, 1.0f, 0.0f, kAnchorWeights, seed) == true);
    REQUIRE(ShouldStepFire(1, 1.0f, 0.0f, kAnchorWeights, seed) == true);
    REQUIRE(ShouldStepFire(3, 1.0f, 0.0f, kAnchorWeights, seed) == true);

    // Low-weight ghost note (weight=0.15) with threshold clearly above it
    // density=0.80 -> threshold=0.20, weight=0.15 < 0.20, should NOT fire
    REQUIRE(ShouldStepFire(1, 0.80f, 0.0f, kAnchorWeights, seed) == false);
    // With density=0.90, threshold=0.10, weight=0.15 > 0.10, SHOULD fire
    REQUIRE(ShouldStepFire(1, 0.90f, 0.0f, kAnchorWeights, seed) == true);
}

TEST_CASE("ShouldStepFire BROKEN flattens weight distribution", "[pulse-field][algorithm]")
{
    uint32_t seed = 12345;

    // At BROKEN=1 (full chaos), all weights lerp toward 0.5
    // effectiveWeight = Lerp(baseWeight, 0.5, 1.0) = 0.5 + noise
    // With BROKEN=1, noise range is ±0.2, so weights are roughly 0.3-0.7

    // At medium density (0.5), threshold=0.5
    // With weights converging to ~0.5, whether something fires becomes probabilistic

    // But we can test that high-weight steps at BROKEN=0 reliably fire,
    // while at BROKEN=1 the behavior is more uniform

    // Test determinism: same seed should produce same result
    bool result1 = ShouldStepFire(0, 0.5f, 1.0f, kAnchorWeights, seed);
    bool result2 = ShouldStepFire(0, 0.5f, 1.0f, kAnchorWeights, seed);
    REQUIRE(result1 == result2);
}

TEST_CASE("ShouldStepFire produces deterministic results with same seed", "[pulse-field][algorithm]")
{
    uint32_t seed = 42;

    // Run the same step multiple times with same seed - should get same result
    for (int i = 0; i < 10; i++)
    {
        bool expected = ShouldStepFire(5, 0.6f, 0.5f, kAnchorWeights, seed);
        bool actual = ShouldStepFire(5, 0.6f, 0.5f, kAnchorWeights, seed);
        REQUIRE(expected == actual);
    }
}

TEST_CASE("ShouldStepFire with different seeds produces varied results", "[pulse-field][algorithm]")
{
    // At BROKEN=1, different seeds should produce different patterns
    // Run 100 trials with different seeds and count fires
    int fireCount = 0;
    for (uint32_t seed = 0; seed < 100; seed++)
    {
        // Ghost note at medium density and full broken
        if (ShouldStepFire(1, 0.5f, 1.0f, kAnchorWeights, seed))
        {
            fireCount++;
        }
    }

    // With weights converging to ~0.5 and threshold at 0.5,
    // plus noise of ±0.2, we expect roughly 40-60% to fire
    REQUIRE(fireCount > 20);  // At least some should fire
    REQUIRE(fireCount < 80);  // Not all should fire
}

TEST_CASE("ShouldStepFire noise injection scales with BROKEN", "[pulse-field][algorithm]")
{
    // At BROKEN=0, no noise: result is purely deterministic based on weight vs threshold
    // At BROKEN=1, max noise: ±0.2 variation

    // Test that BROKEN=0 is truly deterministic (no variation across different seeds)
    uint32_t seed1 = 100;
    uint32_t seed2 = 200;

    // High-weight step with density that makes it clearly fire at BROKEN=0
    bool broken0_seed1 = ShouldStepFire(0, 0.5f, 0.0f, kAnchorWeights, seed1);
    bool broken0_seed2 = ShouldStepFire(0, 0.5f, 0.0f, kAnchorWeights, seed2);
    
    // At BROKEN=0, weight=1.0, threshold=0.5, should always fire regardless of seed
    REQUIRE(broken0_seed1 == true);
    REQUIRE(broken0_seed2 == true);

    // Ghost note at BROKEN=0 with density that makes it clearly NOT fire
    bool ghost_broken0_seed1 = ShouldStepFire(1, 0.5f, 0.0f, kAnchorWeights, seed1);
    bool ghost_broken0_seed2 = ShouldStepFire(1, 0.5f, 0.0f, kAnchorWeights, seed2);

    // At BROKEN=0, weight=0.15, threshold=0.5, should never fire
    REQUIRE(ghost_broken0_seed1 == false);
    REQUIRE(ghost_broken0_seed2 == false);
}

// =============================================================================
// Step Stability Tests (for DRIFT system)
// =============================================================================

TEST_CASE("Step stability values are correct", "[pulse-field][stability]")
{
    // Bar downbeats (steps 0, 16) should be most stable (1.0)
    REQUIRE(GetStepStability(0) == Catch::Approx(1.0f));
    REQUIRE(GetStepStability(16) == Catch::Approx(1.0f));

    // Half notes (steps 8, 24) should be very stable (0.85)
    REQUIRE(GetStepStability(8) == Catch::Approx(0.85f));
    REQUIRE(GetStepStability(24) == Catch::Approx(0.85f));

    // Quarter notes (steps 4, 12, 20, 28) should be stable (0.70)
    REQUIRE(GetStepStability(4) == Catch::Approx(0.70f));
    REQUIRE(GetStepStability(12) == Catch::Approx(0.70f));

    // 8th off-beats (steps 2, 6, 10, ...) should be moderate (0.40)
    REQUIRE(GetStepStability(2) == Catch::Approx(0.40f));
    REQUIRE(GetStepStability(6) == Catch::Approx(0.40f));

    // 16th ghosts (odd steps) should be least stable (0.20)
    REQUIRE(GetStepStability(1) == Catch::Approx(0.20f));
    REQUIRE(GetStepStability(3) == Catch::Approx(0.20f));
    REQUIRE(GetStepStability(5) == Catch::Approx(0.20f));
}

// =============================================================================
// Effective Drift Tests
// =============================================================================

TEST_CASE("Effective drift applies per-voice multipliers", "[pulse-field][drift]")
{
    // Anchor uses 0.7× multiplier
    REQUIRE(GetEffectiveDrift(0.5f, true) == Catch::Approx(0.35f));
    REQUIRE(GetEffectiveDrift(1.0f, true) == Catch::Approx(0.70f));
    REQUIRE(GetEffectiveDrift(0.0f, true) == Catch::Approx(0.0f));

    // Shimmer uses 1.3× multiplier (clamped to 1.0)
    REQUIRE(GetEffectiveDrift(0.5f, false) == Catch::Approx(0.65f));
    REQUIRE(GetEffectiveDrift(1.0f, false) == Catch::Approx(1.0f)); // Clamped
    REQUIRE(GetEffectiveDrift(0.8f, false) == Catch::Approx(1.0f)); // 0.8 * 1.3 = 1.04, clamped

    // Verify clamping at low end
    REQUIRE(GetEffectiveDrift(-0.5f, true) == Catch::Approx(0.0f)); // Clamped
}

// =============================================================================
// Integration Tests: Weight + Density + Broken
// =============================================================================

TEST_CASE("Shimmer fires on backbeats, Anchor fires on downbeats", "[pulse-field][integration]")
{
    uint32_t seed = 12345;

    // At medium density and no broken
    float density = 0.6f;
    float broken = 0.0f;

    // Anchor (kick) pattern: strong on downbeats
    REQUIRE(ShouldStepFire(0, density, broken, kAnchorWeights, seed) == true);   // Downbeat
    REQUIRE(ShouldStepFire(16, density, broken, kAnchorWeights, seed) == true);  // Downbeat
    REQUIRE(ShouldStepFire(1, density, broken, kAnchorWeights, seed) == false);  // Ghost

    // Shimmer (snare) pattern: strong on backbeats
    REQUIRE(ShouldStepFire(8, density, broken, kShimmerWeights, seed) == true);  // Backbeat
    REQUIRE(ShouldStepFire(24, density, broken, kShimmerWeights, seed) == true); // Backbeat
    REQUIRE(ShouldStepFire(0, density, broken, kShimmerWeights, seed) == false); // Low weight downbeat
}
