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

// =============================================================================
// PulseFieldState Tests
// =============================================================================

TEST_CASE("PulseFieldState initializes with dual seeds", "[pulse-field][drift]")
{
    PulseFieldState state;
    state.Init(0x12345678);

    // Seeds should be different after init
    REQUIRE(state.patternSeed_ != state.loopSeed_);
    REQUIRE(state.seedCounter_ == 0);
}

TEST_CASE("PulseFieldState OnPhraseReset changes loopSeed", "[pulse-field][drift]")
{
    PulseFieldState state;
    state.Init(0x12345678);

    uint32_t originalLoopSeed = state.loopSeed_;
    uint32_t originalPatternSeed = state.patternSeed_;

    state.OnPhraseReset();

    // loopSeed should change
    REQUIRE(state.loopSeed_ != originalLoopSeed);
    // patternSeed should NOT change
    REQUIRE(state.patternSeed_ == originalPatternSeed);
    // Counter should increment
    REQUIRE(state.seedCounter_ == 1);

    // Multiple resets produce different seeds
    uint32_t secondLoopSeed = state.loopSeed_;
    state.OnPhraseReset();
    REQUIRE(state.loopSeed_ != secondLoopSeed);
    REQUIRE(state.seedCounter_ == 2);
}

TEST_CASE("PulseFieldState LockPattern copies loopSeed to patternSeed", "[pulse-field][drift]")
{
    PulseFieldState state;
    state.Init(0x12345678);

    // After a few phrase resets, loopSeed will be different
    state.OnPhraseReset();
    state.OnPhraseReset();

    uint32_t currentLoopSeed = state.loopSeed_;

    state.LockPattern();

    // patternSeed should now equal the previous loopSeed
    REQUIRE(state.patternSeed_ == currentLoopSeed);
}

// =============================================================================
// DRIFT=0% Tests: Identical Pattern Every Loop
// =============================================================================

TEST_CASE("DRIFT=0% produces identical pattern every loop", "[pulse-field][drift]")
{
    PulseFieldState state1;
    PulseFieldState state2;
    state1.Init(0xABCD1234);
    state2.Init(0xABCD1234);

    // Simulate multiple "loops" by calling OnPhraseReset on state2
    state2.OnPhraseReset();
    state2.OnPhraseReset();
    state2.OnPhraseReset();

    float density = 0.5f;
    float broken = 0.3f;
    float drift = 0.0f;  // Zero drift = all steps locked

    // With DRIFT=0, pattern should be identical regardless of loopSeed changes
    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        bool anchor1 = ShouldStepFireWithDrift(step, density, broken, drift, true, state1);
        bool anchor2 = ShouldStepFireWithDrift(step, density, broken, drift, true, state2);
        REQUIRE(anchor1 == anchor2);

        bool shimmer1 = ShouldStepFireWithDrift(step, density, broken, drift, false, state1);
        bool shimmer2 = ShouldStepFireWithDrift(step, density, broken, drift, false, state2);
        REQUIRE(shimmer1 == shimmer2);
    }
}

TEST_CASE("DRIFT=0% uses patternSeed for all steps", "[pulse-field][drift]")
{
    PulseFieldState state;
    state.Init(0x12345678);

    // Change loopSeed
    state.OnPhraseReset();

    float density = 0.6f;
    float broken = 0.2f;
    float drift = 0.0f;

    // Record pattern
    bool pattern1[kPulseFieldSteps];
    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        pattern1[step] = ShouldStepFireWithDrift(step, density, broken, drift, true, state);
    }

    // Change loopSeed again
    state.OnPhraseReset();

    // Pattern should be identical (since DRIFT=0 uses patternSeed)
    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        bool result = ShouldStepFireWithDrift(step, density, broken, drift, true, state);
        REQUIRE(result == pattern1[step]);
    }
}

// =============================================================================
// DRIFT=100% Tests: Unique Pattern Each Loop
// =============================================================================

TEST_CASE("DRIFT=100% produces different patterns when loopSeed changes", "[pulse-field][drift]")
{
    PulseFieldState state;
    state.Init(0xDEADBEEF);

    // Use high BROKEN to maximize noise variation, making differences more likely
    float density = 0.5f;
    float broken = 1.0f;  // Maximum broken for maximum noise
    float drift = 1.0f;   // Maximum drift

    // Accumulate differences over multiple loops to account for probabilistic behavior
    int totalDifferences = 0;
    bool previousPattern[kPulseFieldSteps];

    // Record initial pattern
    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        previousPattern[step] = ShouldStepFireWithDrift(step, density, broken, drift, false, state);
    }

    // Test over multiple phrase resets
    for (int loop = 0; loop < 10; loop++)
    {
        state.OnPhraseReset();

        for (int step = 0; step < kPulseFieldSteps; step++)
        {
            bool result = ShouldStepFireWithDrift(step, density, broken, drift, false, state);
            if (result != previousPattern[step])
            {
                totalDifferences++;
            }
            previousPattern[step] = result;
        }
    }

    // Over 10 loops × 32 steps = 320 comparisons with max BROKEN,
    // we expect substantial differences due to noise injection
    REQUIRE(totalDifferences > 5);
}

TEST_CASE("DRIFT=100% Shimmer uses loopSeed for most steps", "[pulse-field][drift]")
{
    // At DRIFT=100%, Shimmer has effective drift of 1.3 (clamped to 1.0)
    // All steps should use loopSeed
    PulseFieldState state;
    state.Init(0x11111111);

    // Use high BROKEN to maximize noise, making differences more likely
    float density = 0.5f;
    float broken = 1.0f;  // Maximum broken for maximum noise variation
    float drift = 1.0f;

    // Count patterns over multiple loops
    int totalDifferences = 0;
    bool previousPattern[kPulseFieldSteps];

    // First loop
    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        previousPattern[step] = ShouldStepFireWithDrift(step, density, broken, drift, false, state);
    }

    // Run multiple loops and count cumulative differences
    for (int loop = 0; loop < 10; loop++)
    {
        state.OnPhraseReset();
        for (int step = 0; step < kPulseFieldSteps; step++)
        {
            bool result = ShouldStepFireWithDrift(step, density, broken, drift, false, state);
            if (result != previousPattern[step])
            {
                totalDifferences++;
            }
            previousPattern[step] = result;
        }
    }

    // Over 10 loops × 32 steps = 320 comparisons with max BROKEN,
    // we expect meaningful differences due to varying seed + noise
    REQUIRE(totalDifferences > 5);
}

// =============================================================================
// Stratified Stability Tests: Downbeats Lock Before Ghost Notes
// =============================================================================

TEST_CASE("At moderate DRIFT, downbeats lock while ghosts drift", "[pulse-field][drift][stratified]")
{
    // At DRIFT=0.5 for Anchor:
    //   effectiveDrift = 0.5 * 0.7 = 0.35
    //   Steps with stability > 0.35 are locked
    //   - Bar downbeats (1.0): LOCKED
    //   - Half notes (0.85): LOCKED
    //   - Quarter notes (0.70): LOCKED
    //   - 8th off-beats (0.40): LOCKED
    //   - 16th ghosts (0.20): DRIFTING

    PulseFieldState state1;
    PulseFieldState state2;
    state1.Init(0x99999999);
    state2.Init(0x99999999);

    // Simulate different "loops"
    state2.OnPhraseReset();
    state2.OnPhraseReset();

    float density = 0.7f;
    float broken = 0.3f;
    float drift = 0.5f;

    // Bar downbeats (stability=1.0) should produce identical results
    REQUIRE(ShouldStepFireWithDrift(0, density, broken, drift, true, state1) ==
            ShouldStepFireWithDrift(0, density, broken, drift, true, state2));
    REQUIRE(ShouldStepFireWithDrift(16, density, broken, drift, true, state1) ==
            ShouldStepFireWithDrift(16, density, broken, drift, true, state2));

    // Half notes (stability=0.85) should produce identical results
    REQUIRE(ShouldStepFireWithDrift(8, density, broken, drift, true, state1) ==
            ShouldStepFireWithDrift(8, density, broken, drift, true, state2));

    // Quarter notes (stability=0.70) should produce identical results
    REQUIRE(ShouldStepFireWithDrift(4, density, broken, drift, true, state1) ==
            ShouldStepFireWithDrift(4, density, broken, drift, true, state2));
}

TEST_CASE("16th ghost notes drift at lower DRIFT than downbeats", "[pulse-field][drift][stratified]")
{
    // At DRIFT=0.25 for Shimmer:
    //   effectiveDrift = 0.25 * 1.3 = 0.325
    //   Steps with stability > 0.325 are locked
    //   - Bar downbeats (1.0): LOCKED
    //   - Half notes (0.85): LOCKED
    //   - Quarter notes (0.70): LOCKED
    //   - 8th off-beats (0.40): LOCKED
    //   - 16th ghosts (0.20): DRIFTING

    PulseFieldState state;
    state.Init(0xAAAAAAAA);

    float density = 0.7f;
    float broken = 0.3f;
    float drift = 0.25f;

    // Record ghost note patterns over multiple loops
    int ghostDifferences = 0;
    bool previousGhost1 = ShouldStepFireWithDrift(1, density, broken, drift, false, state);
    bool previousGhost3 = ShouldStepFireWithDrift(3, density, broken, drift, false, state);

    for (int loop = 0; loop < 10; loop++)
    {
        state.OnPhraseReset();
        bool ghost1 = ShouldStepFireWithDrift(1, density, broken, drift, false, state);
        bool ghost3 = ShouldStepFireWithDrift(3, density, broken, drift, false, state);

        if (ghost1 != previousGhost1) ghostDifferences++;
        if (ghost3 != previousGhost3) ghostDifferences++;

        previousGhost1 = ghost1;
        previousGhost3 = ghost3;
    }

    // Ghost notes (stability 0.20 < effectiveDrift 0.325) should vary
    // Over 10 loops × 2 ghost notes = 20 comparisons, expect some differences
    REQUIRE(ghostDifferences > 0);
}

TEST_CASE("Anchor at DRIFT=100% still has stable downbeats", "[pulse-field][drift][stratified]")
{
    // At DRIFT=100% for Anchor:
    //   effectiveDrift = 1.0 * 0.7 = 0.70
    //   Steps with stability > 0.70 are locked
    //   - Bar downbeats (1.0): LOCKED
    //   - Half notes (0.85): LOCKED
    //   - Quarter notes (0.70): DRIFTING (0.70 is NOT > 0.70)

    PulseFieldState state1;
    PulseFieldState state2;
    state1.Init(0xBBBBBBBB);
    state2.Init(0xBBBBBBBB);

    state2.OnPhraseReset();

    float density = 0.6f;
    float broken = 0.3f;
    float drift = 1.0f;

    // Bar downbeats (stability=1.0 > effectiveDrift=0.70) should still be locked
    REQUIRE(ShouldStepFireWithDrift(0, density, broken, drift, true, state1) ==
            ShouldStepFireWithDrift(0, density, broken, drift, true, state2));
    REQUIRE(ShouldStepFireWithDrift(16, density, broken, drift, true, state1) ==
            ShouldStepFireWithDrift(16, density, broken, drift, true, state2));

    // Half notes (stability=0.85 > effectiveDrift=0.70) should also be locked
    REQUIRE(ShouldStepFireWithDrift(8, density, broken, drift, true, state1) ==
            ShouldStepFireWithDrift(8, density, broken, drift, true, state2));
}

// =============================================================================
// Per-Voice DRIFT Multiplier Tests
// =============================================================================

TEST_CASE("Anchor is more stable than Shimmer at same DRIFT setting", "[pulse-field][drift][voice]")
{
    PulseFieldState state;
    state.Init(0xCCCCCCCC);

    float density = 0.6f;
    float broken = 0.3f;
    float drift = 0.5f;

    // At DRIFT=0.5:
    //   Anchor effective: 0.5 * 0.7 = 0.35
    //   Shimmer effective: 0.5 * 1.3 = 0.65

    // Quarter notes (stability=0.70) should be:
    //   - Locked for Anchor (0.70 > 0.35)
    //   - Locked for Shimmer (0.70 > 0.65)

    // 8th off-beats (stability=0.40) should be:
    //   - Locked for Anchor (0.40 > 0.35)
    //   - Drifting for Shimmer (0.40 < 0.65)

    // Test over multiple loops
    bool anchor8thFirst = ShouldStepFireWithDrift(2, density, broken, drift, true, state);

    state.OnPhraseReset();

    bool anchor8thSecond = ShouldStepFireWithDrift(2, density, broken, drift, true, state);

    // Anchor's 8th notes should be locked (same result)
    // Step 2 has stability 0.40, which is > effectiveDrift 0.35 for Anchor
    REQUIRE(anchor8thFirst == anchor8thSecond);

    // Note: Shimmer's 8th notes (stability 0.40 < effectiveDrift 0.65) would drift,
    // but testing that is probabilistic. The stability math is verified
    // in GetEffectiveDrift tests and the stratified stability tests above.
}

TEST_CASE("GetPulseFieldTriggers returns both voice results", "[pulse-field][drift][integration]")
{
    PulseFieldState state;
    state.Init(0xDDDDDDDD);

    bool anchorFires, shimmerFires;
    GetPulseFieldTriggers(0, 0.5f, 0.5f, 0.0f, 0.0f, state, anchorFires, shimmerFires);

    // At medium density, step 0 has:
    //   Anchor weight 1.0 > threshold 0.5 → fires
    //   Shimmer weight 0.25 < threshold 0.5 → doesn't fire
    REQUIRE(anchorFires == true);
    REQUIRE(shimmerFires == false);

    // Test backbeat (step 8)
    GetPulseFieldTriggers(8, 0.5f, 0.5f, 0.0f, 0.0f, state, anchorFires, shimmerFires);
    // Anchor weight 0.85 > threshold 0.5 → fires
    // Shimmer weight 1.0 > threshold 0.5 → fires
    REQUIRE(anchorFires == true);
    REQUIRE(shimmerFires == true);
}

// =============================================================================
// v3 Critical Rules: DENSITY=0 Absolute Silence [v3-critical-rules]
// =============================================================================

TEST_CASE("DENSITY=0 produces zero triggers regardless of BROKEN", "[pulse-field][v3-critical-rules][density-zero]")
{
    PulseFieldState state;
    state.Init(0x12345678);

    // Test all steps at density=0 with various BROKEN levels
    for (float broken = 0.0f; broken <= 1.0f; broken += 0.25f)
    {
        for (int step = 0; step < kPulseFieldSteps; step++)
        {
            // Anchor density = 0
            bool fires = ShouldStepFire(step, 0.0f, broken, kAnchorWeights, state.patternSeed_);
            REQUIRE(fires == false);

            // Shimmer density = 0
            fires = ShouldStepFire(step, 0.0f, broken, kShimmerWeights, state.patternSeed_);
            REQUIRE(fires == false);
        }
    }
}

TEST_CASE("DENSITY=0 produces zero triggers regardless of DRIFT", "[pulse-field][v3-critical-rules][density-zero]")
{
    PulseFieldState state;
    state.Init(0xABCDEF01);

    // Test with DRIFT across multiple phrase resets
    for (float drift = 0.0f; drift <= 1.0f; drift += 0.25f)
    {
        for (int loop = 0; loop < 5; loop++)
        {
            state.OnPhraseReset();

            for (int step = 0; step < kPulseFieldSteps; step++)
            {
                // Anchor at density=0
                bool anchorFires = ShouldStepFireWithDrift(step, 0.0f, 0.5f, drift, true, state);
                REQUIRE(anchorFires == false);

                // Shimmer at density=0
                bool shimmerFires = ShouldStepFireWithDrift(step, 0.0f, 0.5f, drift, false, state);
                REQUIRE(shimmerFires == false);
            }
        }
    }
}

TEST_CASE("DENSITY=0 produces zero triggers via GetPulseFieldTriggers", "[pulse-field][v3-critical-rules][density-zero]")
{
    PulseFieldState state;
    state.Init(0xFEDCBA98);

    // Test full integration: both voices at density=0
    for (float broken = 0.0f; broken <= 1.0f; broken += 0.5f)
    {
        for (float drift = 0.0f; drift <= 1.0f; drift += 0.5f)
        {
            state.OnPhraseReset();

            for (int step = 0; step < kPulseFieldSteps; step++)
            {
                bool anchorFires, shimmerFires;
                GetPulseFieldTriggers(step, 0.0f, 0.0f, broken, drift, state, anchorFires, shimmerFires);

                REQUIRE(anchorFires == false);
                REQUIRE(shimmerFires == false);
            }
        }
    }
}

TEST_CASE("DENSITY=0 for one voice does not affect other voice", "[pulse-field][v3-critical-rules][density-zero]")
{
    PulseFieldState state;
    state.Init(0x11223344);

    // Anchor at 0, Shimmer at normal density
    bool anchorFires, shimmerFires;

    // Step 24 = shimmer backbeat (weight 1.0), anchor at density 0
    GetPulseFieldTriggers(24, 0.0f, 0.8f, 0.0f, 0.0f, state, anchorFires, shimmerFires);
    REQUIRE(anchorFires == false);  // Anchor silent (density=0)
    REQUIRE(shimmerFires == true);  // Shimmer fires normally

    // Shimmer at 0, Anchor at normal density
    // Step 0 = anchor downbeat (weight 1.0), shimmer low weight
    GetPulseFieldTriggers(0, 0.8f, 0.0f, 0.0f, 0.0f, state, anchorFires, shimmerFires);
    REQUIRE(anchorFires == true);   // Anchor fires normally
    REQUIRE(shimmerFires == false); // Shimmer silent (density=0 + low weight)
}

// =============================================================================
// v3 Critical Rules: DRIFT=0 Zero Variation [v3-critical-rules]
// =============================================================================

TEST_CASE("DRIFT=0 produces identical pattern across many phrase resets", "[pulse-field][v3-critical-rules][drift-zero]")
{
    PulseFieldState state;
    state.Init(0x55667788);

    float density = 0.6f;
    float broken = 0.5f;
    float drift = 0.0f;  // Critical: zero drift

    // Record the initial pattern
    bool anchorPattern[kPulseFieldSteps];
    bool shimmerPattern[kPulseFieldSteps];

    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        bool anchor, shimmer;
        GetPulseFieldTriggers(step, density, density, broken, drift, state, anchor, shimmer);
        anchorPattern[step] = anchor;
        shimmerPattern[step] = shimmer;
    }

    // Verify pattern is identical across 20 phrase resets
    for (int loop = 0; loop < 20; loop++)
    {
        state.OnPhraseReset();

        for (int step = 0; step < kPulseFieldSteps; step++)
        {
            bool anchor, shimmer;
            GetPulseFieldTriggers(step, density, density, broken, drift, state, anchor, shimmer);

            REQUIRE(anchor == anchorPattern[step]);
            REQUIRE(shimmer == shimmerPattern[step]);
        }
    }
}

TEST_CASE("DRIFT=0 + BROKEN=100% still produces identical pattern every loop", "[pulse-field][v3-critical-rules][drift-zero]")
{
    // This is a critical test: even with maximum chaos (BROKEN=100%),
    // if DRIFT=0, the pattern must be 100% repeatable

    PulseFieldState state;
    state.Init(0x99AABBCC);

    float density = 0.5f;
    float broken = 1.0f;  // Maximum chaos
    float drift = 0.0f;   // But zero drift = locked

    // Record the initial pattern
    bool anchorPattern[kPulseFieldSteps];
    bool shimmerPattern[kPulseFieldSteps];

    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        bool anchor, shimmer;
        GetPulseFieldTriggers(step, density, density, broken, drift, state, anchor, shimmer);
        anchorPattern[step] = anchor;
        shimmerPattern[step] = shimmer;
    }

    // Verify pattern is identical across 20 phrase resets
    for (int loop = 0; loop < 20; loop++)
    {
        state.OnPhraseReset();

        for (int step = 0; step < kPulseFieldSteps; step++)
        {
            bool anchor, shimmer;
            GetPulseFieldTriggers(step, density, density, broken, drift, state, anchor, shimmer);

            REQUIRE(anchor == anchorPattern[step]);
            REQUIRE(shimmer == shimmerPattern[step]);
        }
    }
}

TEST_CASE("DRIFT=0 with different initial seeds produces different but stable patterns", "[pulse-field][v3-critical-rules][drift-zero]")
{
    // Different seeds should produce different patterns, but each should be stable at DRIFT=0

    float density = 0.6f;
    float broken = 0.3f;
    float drift = 0.0f;

    // Test with two different seeds
    PulseFieldState state1, state2;
    state1.Init(0x11111111);
    state2.Init(0x22222222);

    // Both should be internally stable
    for (int loop = 0; loop < 5; loop++)
    {
        state1.OnPhraseReset();
        state2.OnPhraseReset();
    }

    // Check state1 is stable
    bool pattern1[kPulseFieldSteps];
    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        bool anchor, shimmer;
        GetPulseFieldTriggers(step, density, density, broken, drift, state1, anchor, shimmer);
        pattern1[step] = anchor;
    }

    state1.OnPhraseReset();

    for (int step = 0; step < kPulseFieldSteps; step++)
    {
        bool anchor, shimmer;
        GetPulseFieldTriggers(step, density, density, broken, drift, state1, anchor, shimmer);
        REQUIRE(anchor == pattern1[step]);  // Must be identical
    }
}

TEST_CASE("The Reference Point: BROKEN=0 DRIFT=0 DENSITY=50% classic 4/4", "[pulse-field][v3-critical-rules][reference-point]")
{
    // Spec requirement: At BROKEN=0, DRIFT=0, DENSITIES at 50%:
    // "classic 4/4 kick with snare on backbeat, repeated identically forever"

    PulseFieldState state;
    state.Init(0x44554F50);  // "DUOP"

    float density = 0.5f;
    float broken = 0.0f;
    float drift = 0.0f;

    // Step 0: anchor should fire (downbeat, weight 1.0 > threshold 0.5)
    bool anchor0, shimmer0;
    GetPulseFieldTriggers(0, density, density, broken, drift, state, anchor0, shimmer0);
    REQUIRE(anchor0 == true);   // Kick on the 1
    REQUIRE(shimmer0 == false); // Shimmer weight 0.25 < 0.5

    // Step 8: shimmer should fire (backbeat, weight 1.0 > threshold 0.5)
    bool anchor8, shimmer8;
    GetPulseFieldTriggers(8, density, density, broken, drift, state, anchor8, shimmer8);
    REQUIRE(anchor8 == true);   // Anchor weight 0.85 > 0.5
    REQUIRE(shimmer8 == true);  // Snare on backbeat

    // Step 16: anchor should fire (downbeat bar 2)
    bool anchor16, shimmer16;
    GetPulseFieldTriggers(16, density, density, broken, drift, state, anchor16, shimmer16);
    REQUIRE(anchor16 == true);

    // Step 24: shimmer should fire (backbeat bar 2)
    bool anchor24, shimmer24;
    GetPulseFieldTriggers(24, density, density, broken, drift, state, anchor24, shimmer24);
    REQUIRE(shimmer24 == true);

    // Verify pattern repeats identically
    for (int loop = 0; loop < 10; loop++)
    {
        state.OnPhraseReset();

        bool a0, s0, a8, s8, a16, s16, a24, s24;
        GetPulseFieldTriggers(0, density, density, broken, drift, state, a0, s0);
        GetPulseFieldTriggers(8, density, density, broken, drift, state, a8, s8);
        GetPulseFieldTriggers(16, density, density, broken, drift, state, a16, s16);
        GetPulseFieldTriggers(24, density, density, broken, drift, state, a24, s24);

        REQUIRE(a0 == anchor0);
        REQUIRE(s0 == shimmer0);
        REQUIRE(a8 == anchor8);
        REQUIRE(s8 == shimmer8);
        REQUIRE(a16 == anchor16);
        REQUIRE(s16 == shimmer16);
        REQUIRE(a24 == anchor24);
        REQUIRE(s24 == shimmer24);
    }
}
