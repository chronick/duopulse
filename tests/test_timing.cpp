#include <catch2/catch_all.hpp>
#include <cmath>

#include "../src/Engine/BrokenEffects.h"
#include "../src/Engine/VelocityCompute.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// v4.1 Zone-Bounded Swing Tests (with archetype base)
// =============================================================================

TEST_CASE("ComputeSwing multiplies archetype base by config", "[timing][swing][config]")
{
    // archetype = 0.50 (straight), config = 0% -> 1.0× archetype = 0.50
    REQUIRE(ComputeSwing(0.0f, 0.50f, EnergyZone::PEAK) == Approx(0.50f).margin(0.01f));

    // archetype = 0.50 (straight), config = 100% -> 2.0× archetype = 1.00 (capped at 0.70 for PEAK)
    REQUIRE(ComputeSwing(1.0f, 0.50f, EnergyZone::PEAK) == Approx(0.70f).margin(0.01f));

    // archetype = 0.55 (swung), config = 0% -> 1.0× archetype = 0.55
    REQUIRE(ComputeSwing(0.0f, 0.55f, EnergyZone::PEAK) == Approx(0.55f).margin(0.01f));

    // archetype = 0.55 (swung), config = 50% -> 1.5× archetype = 0.825 (capped at 0.70 for PEAK)
    REQUIRE(ComputeSwing(0.5f, 0.55f, EnergyZone::PEAK) == Approx(0.70f).margin(0.01f));
}

TEST_CASE("ComputeSwing uses swing config, not flavorCV", "[timing][swing][config][regression]")
{
    // This test documents the bug fix from Modification 0.6:
    // ComputeSwing was incorrectly using flavorCV instead of swing config.
    // The first parameter is now explicitly the swing CONFIG value (Config K2).

    // v4.1 update: Now multiplies archetype base instead of fixed offset
    const float archetypeBase = 0.50f;  // Straight archetype

    // Config K2 at 0% (CCW) -> 1.0× archetype = 0.50 (straight)
    REQUIRE(ComputeSwing(0.0f, archetypeBase, EnergyZone::PEAK) == Approx(0.50f).margin(0.001f));

    // Config K2 at 50% (noon) -> 1.5× archetype = 0.75 (capped at 0.70)
    REQUIRE(ComputeSwing(0.5f, archetypeBase, EnergyZone::PEAK) == Approx(0.70f).margin(0.001f));

    // Config K2 at 100% (CW) -> 2.0× archetype = 1.00 (capped at 0.70)
    REQUIRE(ComputeSwing(1.0f, archetypeBase, EnergyZone::PEAK) == Approx(0.70f).margin(0.001f));
}

TEST_CASE("ComputeSwing is bounded by energy zone (v4.1 widened caps)", "[timing][swing]")
{
    const float archetypeBase = 0.50f;

    // MINIMAL zone: max 60% (widened from 58%)
    REQUIRE(ComputeSwing(1.0f, archetypeBase, EnergyZone::MINIMAL) == Approx(0.60f).margin(0.01f));

    // GROOVE zone: max 65% (widened from 58%)
    REQUIRE(ComputeSwing(1.0f, archetypeBase, EnergyZone::GROOVE) == Approx(0.65f).margin(0.01f));

    // BUILD zone: max 68% (widened from 62%)
    REQUIRE(ComputeSwing(1.0f, archetypeBase, EnergyZone::BUILD) == Approx(0.68f).margin(0.01f));

    // PEAK zone: max 70% (widened from 66%)
    REQUIRE(ComputeSwing(1.0f, archetypeBase, EnergyZone::PEAK) == Approx(0.70f).margin(0.01f));
}

TEST_CASE("ApplySwingToStep only affects offbeats", "[timing][swing]")
{
    const float samplesPerStep = 1000.0f;  // Arbitrary for testing
    const float swingAmount = 0.60f;       // Some swing

    // Even steps (onbeats) should have no offset
    REQUIRE(ApplySwingToStep(0, swingAmount, samplesPerStep) == Approx(0.0f));
    REQUIRE(ApplySwingToStep(2, swingAmount, samplesPerStep) == Approx(0.0f));
    REQUIRE(ApplySwingToStep(4, swingAmount, samplesPerStep) == Approx(0.0f));

    // Odd steps (offbeats) should have positive offset (delayed)
    float offset1 = ApplySwingToStep(1, swingAmount, samplesPerStep);
    float offset3 = ApplySwingToStep(3, swingAmount, samplesPerStep);

    REQUIRE(offset1 > 0.0f);
    REQUIRE(offset3 > 0.0f);

    // All offbeats should have the same offset for same swing amount
    REQUIRE(offset1 == Approx(offset3));
}

TEST_CASE("ApplySwingToStep offset scales with swing amount", "[timing][swing]")
{
    const float samplesPerStep = 1000.0f;

    // At 50% swing (straight), offset should be 0
    REQUIRE(ApplySwingToStep(1, 0.50f, samplesPerStep) == Approx(0.0f).margin(0.01f));

    // Higher swing = larger offset
    float offset_55 = ApplySwingToStep(1, 0.55f, samplesPerStep);
    float offset_60 = ApplySwingToStep(1, 0.60f, samplesPerStep);
    float offset_66 = ApplySwingToStep(1, 0.66f, samplesPerStep);

    REQUIRE(offset_55 < offset_60);
    REQUIRE(offset_60 < offset_66);
}

// =============================================================================
// v4 Zone-Bounded Jitter Tests
// =============================================================================

TEST_CASE("Jitter is zero at low flavor", "[timing][jitter]")
{
    const float sampleRate = 48000.0f;
    const uint32_t seed = 0x12345678;

    // At flavor = 0, jitter should be 0
    for (int step = 0; step < 16; step++)
    {
        float jitter = ComputeMicrotimingOffset(0.0f, EnergyZone::PEAK, sampleRate, seed, step);
        REQUIRE(jitter == Approx(0.0f).margin(0.001f));
    }
}

TEST_CASE("Jitter increases with flavor", "[timing][jitter]")
{
    const float sampleRate = 48000.0f;
    const uint32_t seed = 0x12345678;

    // Collect max jitter magnitude at different flavor levels
    float maxJitter_25 = 0.0f;
    float maxJitter_75 = 0.0f;

    for (int step = 0; step < 32; step++)
    {
        float j25 = std::abs(ComputeMicrotimingOffset(0.25f, EnergyZone::PEAK, sampleRate, seed, step));
        float j75 = std::abs(ComputeMicrotimingOffset(0.75f, EnergyZone::PEAK, sampleRate, seed, step));

        maxJitter_25 = std::max(maxJitter_25, j25);
        maxJitter_75 = std::max(maxJitter_75, j75);
    }

    // Higher flavor should produce larger maximum jitter
    REQUIRE(maxJitter_75 > maxJitter_25);
}

TEST_CASE("Jitter is bounded by energy zone", "[timing][jitter]")
{
    const float sampleRate = 48000.0f;
    const uint32_t seed = 0xDEADBEEF;

    // GROOVE zone max is ±3ms = ±144 samples at 48kHz
    float maxGrooveJitterSamples = 3.0f * sampleRate / 1000.0f;

    for (int step = 0; step < 32; step++)
    {
        float jitter = ComputeMicrotimingOffset(1.0f, EnergyZone::GROOVE, sampleRate, seed, step);
        REQUIRE(std::abs(jitter) <= maxGrooveJitterSamples + 0.01f);
    }

    // PEAK zone max is ±12ms = ±576 samples at 48kHz
    float maxPeakJitterSamples = 12.0f * sampleRate / 1000.0f;

    for (int step = 0; step < 32; step++)
    {
        float jitter = ComputeMicrotimingOffset(1.0f, EnergyZone::PEAK, sampleRate, seed, step);
        REQUIRE(std::abs(jitter) <= maxPeakJitterSamples + 0.01f);
    }
}

TEST_CASE("Jitter is deterministic with same seed", "[timing][jitter]")
{
    const float sampleRate = 48000.0f;
    const uint32_t seed = 0xCAFEBABE;

    for (int step = 0; step < 16; step++)
    {
        float jitter1 = ComputeMicrotimingOffset(0.5f, EnergyZone::BUILD, sampleRate, seed, step);
        float jitter2 = ComputeMicrotimingOffset(0.5f, EnergyZone::BUILD, sampleRate, seed, step);

        REQUIRE(jitter1 == Approx(jitter2));
    }
}

// =============================================================================
// v4 Zone-Bounded Step Displacement Tests
// =============================================================================

TEST_CASE("Step displacement only occurs in BUILD/PEAK zones", "[timing][displacement]")
{
    const uint32_t seed = 0x12345678;

    // MINIMAL and GROOVE zones should never displace
    for (int step = 0; step < 32; step++)
    {
        REQUIRE(ComputeStepDisplacement(step, 1.0f, EnergyZone::MINIMAL, seed) == step);
        REQUIRE(ComputeStepDisplacement(step, 1.0f, EnergyZone::GROOVE, seed) == step);
    }
}

TEST_CASE("Step displacement can occur in BUILD zone", "[timing][displacement]")
{
    const uint32_t baseSeed = 0x12345678;

    // With high flavor in BUILD zone, some steps should displace
    int displacedCount = 0;

    for (int step = 0; step < 32; step++)
    {
        // Try different seeds to get variety
        uint32_t seed = baseSeed + step * 0x1000;
        int newStep = ComputeStepDisplacement(step, 1.0f, EnergyZone::BUILD, seed);
        if (newStep != step)
        {
            displacedCount++;
            // BUILD zone max shift is ±1
            int diff = std::abs(newStep - step);
            // Account for wrap-around
            if (diff > 16) diff = 32 - diff;
            REQUIRE(diff <= 1);
        }
    }

    // Should have at least some displacements
    REQUIRE(displacedCount > 0);
}

TEST_CASE("Step displacement can occur in PEAK zone with larger shift", "[timing][displacement]")
{
    const uint32_t baseSeed = 0xABCDEF01;

    // With high flavor in PEAK zone, some steps should displace
    int displacedCount = 0;
    int maxShiftObserved = 0;

    for (int step = 0; step < 32; step++)
    {
        uint32_t seed = baseSeed + step * 0x2000;
        int newStep = ComputeStepDisplacement(step, 1.0f, EnergyZone::PEAK, seed);
        if (newStep != step)
        {
            displacedCount++;
            int diff = std::abs(newStep - step);
            // Account for wrap-around
            if (diff > 16) diff = 32 - diff;
            maxShiftObserved = std::max(maxShiftObserved, diff);
            // PEAK zone max shift is ±2
            REQUIRE(diff <= 2);
        }
    }

    // Should have displacements
    REQUIRE(displacedCount > 0);
}

TEST_CASE("Step displacement never occurs at low flavor", "[timing][displacement]")
{
    const uint32_t seed = 0x12345678;

    // At flavor = 0, no displacement should occur
    for (int step = 0; step < 32; step++)
    {
        REQUIRE(ComputeStepDisplacement(step, 0.0f, EnergyZone::PEAK, seed) == step);
        REQUIRE(ComputeStepDisplacement(step, 0.0f, EnergyZone::BUILD, seed) == step);
    }
}

// =============================================================================
// v4 Velocity Chaos Tests
// =============================================================================

TEST_CASE("Velocity chaos is zero at low flavor", "[timing][velocity]")
{
    const uint32_t seed = 0x12345678;

    for (int step = 0; step < 16; step++)
    {
        float velocity = ComputeVelocityChaos(0.7f, 0.0f, seed, step);
        REQUIRE(velocity == Approx(0.7f).margin(0.001f));
    }
}

TEST_CASE("Velocity chaos increases with flavor", "[timing][velocity]")
{
    const uint32_t seed = 0x12345678;
    const float baseVelocity = 0.7f;

    float maxDeviation_25 = 0.0f;
    float maxDeviation_100 = 0.0f;

    for (int step = 0; step < 32; step++)
    {
        float v25 = ComputeVelocityChaos(baseVelocity, 0.25f, seed, step);
        float v100 = ComputeVelocityChaos(baseVelocity, 1.0f, seed, step);

        maxDeviation_25 = std::max(maxDeviation_25, std::abs(v25 - baseVelocity));
        maxDeviation_100 = std::max(maxDeviation_100, std::abs(v100 - baseVelocity));
    }

    // Higher flavor should produce larger deviations
    REQUIRE(maxDeviation_100 > maxDeviation_25);
}

TEST_CASE("Velocity chaos is clamped to valid range", "[timing][velocity]")
{
    const uint32_t seed = 0xDEADBEEF;

    for (int step = 0; step < 32; step++)
    {
        // Test with extreme base velocities
        float vLow = ComputeVelocityChaos(0.1f, 1.0f, seed, step);
        float vHigh = ComputeVelocityChaos(0.95f, 1.0f, seed, step);

        REQUIRE(vLow >= 0.1f);
        REQUIRE(vLow <= 1.0f);
        REQUIRE(vHigh >= 0.1f);
        REQUIRE(vHigh <= 1.0f);
    }
}

// =============================================================================
// VelocityCompute PUNCH Tests
// =============================================================================

TEST_CASE("ComputePunch scales parameters with punch value", "[velocity][punch]")
{
    PunchParams low, mid, high;

    ComputePunch(0.0f, low);
    ComputePunch(0.5f, mid);
    ComputePunch(1.0f, high);

    // Accent probability increases with punch
    REQUIRE(low.accentProbability < mid.accentProbability);
    REQUIRE(mid.accentProbability < high.accentProbability);

    // Velocity floor decreases with punch (more dynamics)
    REQUIRE(low.velocityFloor > mid.velocityFloor);
    REQUIRE(mid.velocityFloor > high.velocityFloor);

    // Accent boost increases with punch
    REQUIRE(low.accentBoost < mid.accentBoost);
    REQUIRE(mid.accentBoost < high.accentBoost);

    // Velocity variation increases with punch
    REQUIRE(low.velocityVariation < mid.velocityVariation);
    REQUIRE(mid.velocityVariation < high.velocityVariation);
}

TEST_CASE("ComputePunch has expected range values", "[velocity][punch]")
{
    PunchParams low, high;

    ComputePunch(0.0f, low);
    ComputePunch(1.0f, high);

    // At punch=0: flat dynamics
    // Task 21 Phase B: Updated ranges for wider velocity contrast
    REQUIRE(low.accentProbability == Approx(0.20f).margin(0.01f));  // was 0.15
    REQUIRE(low.velocityFloor == Approx(0.65f).margin(0.01f));      // was 0.70
    REQUIRE(low.accentBoost == Approx(0.15f).margin(0.01f));        // was 0.10
    REQUIRE(low.velocityVariation == Approx(0.03f).margin(0.01f));  // was 0.05

    // At punch=1: maximum dynamics
    REQUIRE(high.accentProbability == Approx(0.50f).margin(0.01f));
    REQUIRE(high.velocityFloor == Approx(0.30f).margin(0.01f));
    REQUIRE(high.accentBoost == Approx(0.45f).margin(0.01f));       // was 0.35
    REQUIRE(high.velocityVariation == Approx(0.15f).margin(0.01f)); // was 0.20
}

// =============================================================================
// VelocityCompute BUILD Tests
// =============================================================================

TEST_CASE("ComputeBuildModifiers scales with build and progress", "[velocity][build]")
{
    // Task 21 Phase D: Test new 3-phase BUILD system
    BuildModifiers groove, building, peak;

    // GROOVE phase (0-60%): no density change
    ComputeBuildModifiers(0.0f, 0.5f, groove);
    REQUIRE(groove.densityMultiplier == Approx(1.0f).margin(0.01f));
    REQUIRE(groove.phase == BuildPhase::GROOVE);

    // BUILD phase (60-87.5%): ramping density
    ComputeBuildModifiers(0.5f, 0.7f, building);
    REQUIRE(building.densityMultiplier > 1.0f);
    REQUIRE(building.phase == BuildPhase::BUILD);

    // FILL phase (87.5-100%): maximum density
    ComputeBuildModifiers(1.0f, 1.0f, peak);
    REQUIRE(peak.densityMultiplier > building.densityMultiplier);
    REQUIRE(peak.phase == BuildPhase::FILL);
}

TEST_CASE("ComputeBuildModifiers identifies fill zone", "[velocity][build]")
{
    // Task 21 Phase D: Test new 3-phase BUILD system
    BuildModifiers early, beforeFill, inFill, endFill;

    ComputeBuildModifiers(1.0f, 0.50f, early);      // GROOVE phase
    ComputeBuildModifiers(1.0f, 0.87f, beforeFill); // BUILD phase
    ComputeBuildModifiers(1.0f, 0.90f, inFill);     // FILL phase (start)
    ComputeBuildModifiers(1.0f, 1.0f, endFill);     // FILL phase (end)

    REQUIRE(early.inFillZone == false);
    REQUIRE(beforeFill.inFillZone == false);
    REQUIRE(inFill.inFillZone == true);
    REQUIRE(endFill.inFillZone == true);

    // In new 3-phase system, fillIntensity is constant in FILL phase (= build value)
    // Both should equal 1.0 since build=1.0
    REQUIRE(inFill.fillIntensity == Approx(1.0f).margin(0.01f));
    REQUIRE(endFill.fillIntensity == Approx(1.0f).margin(0.01f));

    // Velocity boost should be present in FILL
    REQUIRE(inFill.velocityBoost > 0.0f);
    REQUIRE(endFill.velocityBoost > 0.0f);
}

// =============================================================================
// VelocityCompute Velocity Computation Tests
// =============================================================================

TEST_CASE("ComputeVelocity produces valid output range", "[velocity]")
{
    PunchParams params;
    BuildModifiers mods;

    ComputePunch(0.5f, params);
    ComputeBuildModifiers(0.5f, 0.5f, mods);

    const uint32_t seed = 0x12345678;

    for (int step = 0; step < 32; step++)
    {
        float velAccent = ComputeVelocity(params, mods, true, seed, step);
        float velNoAccent = ComputeVelocity(params, mods, false, seed, step);

        REQUIRE(velAccent >= 0.2f);
        REQUIRE(velAccent <= 1.0f);
        REQUIRE(velNoAccent >= 0.2f);
        REQUIRE(velNoAccent <= 1.0f);

        // Accented velocity should generally be higher
        // (not always due to random variation, but on average)
    }
}

TEST_CASE("Velocity contrast scales with punch", "[velocity][punch]")
{
    PunchParams lowPunch, highPunch;
    BuildModifiers mods;

    ComputePunch(0.0f, lowPunch);
    ComputePunch(1.0f, highPunch);
    ComputeBuildModifiers(0.0f, 0.5f, mods);

    const uint32_t seed = 0x12345678;
    const int step = 0;

    // At low punch: accented and non-accented should be similar
    float lowAccent = ComputeVelocity(lowPunch, mods, true, seed, step);
    float lowNormal = ComputeVelocity(lowPunch, mods, false, seed, step);
    float lowContrast = lowAccent - lowNormal;

    // At high punch: accented should be much higher than non-accented
    float highAccent = ComputeVelocity(highPunch, mods, true, seed, step);
    float highNormal = ComputeVelocity(highPunch, mods, false, seed, step);
    float highContrast = highAccent - highNormal;

    // High punch should have more contrast
    REQUIRE(highContrast > lowContrast);
}

TEST_CASE("ShouldAccent respects accent mask", "[velocity][accent]")
{
    const uint32_t seed = 0x12345678;

    // Mask with only step 0 eligible
    uint32_t mask = 0x00000001;

    // Task 21 Phase D: Create BuildModifiers with forceAccents=false
    BuildModifiers buildMods;
    buildMods.Init();

    // Step 0 should potentially accent (probability dependent)
    // Steps 1-31 should never accent
    for (int step = 1; step < 32; step++)
    {
        REQUIRE(ShouldAccent(step, mask, 1.0f, buildMods, seed) == false);
    }
}

TEST_CASE("GetDefaultAccentMask returns valid masks", "[velocity][accent]")
{
    uint32_t anchorMask = GetDefaultAccentMask(Voice::ANCHOR);
    uint32_t shimmerMask = GetDefaultAccentMask(Voice::SHIMMER);
    uint32_t auxMask = GetDefaultAccentMask(Voice::AUX);

    // All masks should have some bits set
    REQUIRE(anchorMask != 0);
    REQUIRE(shimmerMask != 0);
    REQUIRE(auxMask != 0);

    // Anchor should emphasize quarter notes (step 0 at minimum)
    REQUIRE((anchorMask & 0x1) != 0);

    // Shimmer should emphasize backbeat (step 8)
    REQUIRE((shimmerMask & (1 << 8)) != 0);
}

// =============================================================================
// v4 Helper Function Tests
// =============================================================================

TEST_CASE("GetMaxSwingForZone returns correct limits (v4.1 widened)", "[timing][swing]")
{
    REQUIRE(GetMaxSwingForZone(EnergyZone::MINIMAL) == Approx(0.60f));
    REQUIRE(GetMaxSwingForZone(EnergyZone::GROOVE) == Approx(0.65f));
    REQUIRE(GetMaxSwingForZone(EnergyZone::BUILD) == Approx(0.68f));
    REQUIRE(GetMaxSwingForZone(EnergyZone::PEAK) == Approx(0.70f));
}

TEST_CASE("GetMaxJitterMsForZone returns correct limits", "[timing][jitter]")
{
    REQUIRE(GetMaxJitterMsForZone(EnergyZone::MINIMAL) == Approx(3.0f));
    REQUIRE(GetMaxJitterMsForZone(EnergyZone::GROOVE) == Approx(3.0f));
    REQUIRE(GetMaxJitterMsForZone(EnergyZone::BUILD) == Approx(6.0f));
    REQUIRE(GetMaxJitterMsForZone(EnergyZone::PEAK) == Approx(12.0f));
}

TEST_CASE("IsOffbeat identifies correct steps", "[timing]")
{
    // Even steps are not offbeats
    REQUIRE(IsOffbeat(0) == false);
    REQUIRE(IsOffbeat(2) == false);
    REQUIRE(IsOffbeat(4) == false);
    REQUIRE(IsOffbeat(16) == false);

    // Odd steps are offbeats
    REQUIRE(IsOffbeat(1) == true);
    REQUIRE(IsOffbeat(3) == true);
    REQUIRE(IsOffbeat(5) == true);
    REQUIRE(IsOffbeat(31) == true);
}

// =============================================================================
// v4.1 External Clock + Swing Regression Test (Task 21 Phase E4)
// =============================================================================

TEST_CASE("External clock timing not violated by swing", "[timing][swing][clock][regression]")
{
    // Regression test for Task 08 (Bulletproof Clock) compatibility.
    // Swing should delay offbeat triggers but NEVER advance them beyond
    // the next clock edge. This ensures external clock edge timing is respected.

    const float samplesPerStep = 1000.0f;
    const float maxAllowedSwing = 0.70f;  // PEAK zone maximum

    // Test all swing values from 50% (straight) to 70% (max)
    for (float swing = 0.50f; swing <= maxAllowedSwing + 0.01f; swing += 0.05f)
    {
        // Compute swing offset for an offbeat step (step 1)
        float offset = ApplySwingToStep(1, swing, samplesPerStep);

        // Swing should NEVER advance the trigger (negative offset)
        REQUIRE(offset >= 0.0f);

        // Swing should NEVER delay beyond the next step boundary
        // (otherwise it would violate the next clock edge)
        REQUIRE(offset < samplesPerStep);
    }
}

TEST_CASE("Archetype swing blends correctly with config swing", "[timing][swing][archetype]")
{
    // Test that archetype base swing is correctly multiplied by config swing

    // Case 1: Zero archetype swing (straight archetype)
    // Should default to 0.50 and then multiply by config
    float result1 = ComputeSwing(0.5f, 0.0f, EnergyZone::PEAK);
    REQUIRE(result1 == Approx(0.70f).margin(0.01f));  // 0.50 * 1.5 = 0.75, capped at 0.70

    // Case 2: Moderate archetype swing (0.55)
    // Config 0% should preserve archetype base
    float result2 = ComputeSwing(0.0f, 0.55f, EnergyZone::PEAK);
    REQUIRE(result2 == Approx(0.55f).margin(0.01f));

    // Config 50% should give 1.5× archetype
    float result3 = ComputeSwing(0.5f, 0.55f, EnergyZone::PEAK);
    REQUIRE(result3 == Approx(0.70f).margin(0.01f));  // 0.55 * 1.5 = 0.825, capped at 0.70

    // Case 3: High archetype swing (0.60)
    // Config 0% should preserve archetype base
    float result4 = ComputeSwing(0.0f, 0.60f, EnergyZone::GROOVE);
    REQUIRE(result4 == Approx(0.60f).margin(0.01f));

    // Config 50% should exceed GROOVE cap and be clamped
    float result5 = ComputeSwing(0.5f, 0.60f, EnergyZone::GROOVE);
    REQUIRE(result5 == Approx(0.65f).margin(0.01f));  // 0.60 * 1.5 = 0.90, capped at 0.65
}
