#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/DuoPulseTypes.h"
#include "../src/Engine/ArchetypeDNA.h"
#include "../src/Engine/ControlState.h"
#include "../src/Engine/SequencerState.h"
#include "../src/Engine/OutputState.h"
#include "../src/Engine/DuoPulseState.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// DuoPulseTypes.h Tests
// =============================================================================

TEST_CASE("DuoPulseTypes enum values", "[types]")
{
    SECTION("Genre enum has correct values")
    {
        REQUIRE(static_cast<int>(Genre::TECHNO) == 0);
        REQUIRE(static_cast<int>(Genre::TRIBAL) == 1);
        REQUIRE(static_cast<int>(Genre::IDM) == 2);
        REQUIRE(static_cast<int>(Genre::COUNT) == 3);
    }

    SECTION("Voice enum has correct values")
    {
        REQUIRE(static_cast<int>(Voice::ANCHOR) == 0);
        REQUIRE(static_cast<int>(Voice::SHIMMER) == 1);
        REQUIRE(static_cast<int>(Voice::AUX) == 2);
        REQUIRE(static_cast<int>(Voice::COUNT) == 3);
    }

    SECTION("EnergyZone enum has correct values")
    {
        REQUIRE(static_cast<int>(EnergyZone::MINIMAL) == 0);
        REQUIRE(static_cast<int>(EnergyZone::GROOVE) == 1);
        REQUIRE(static_cast<int>(EnergyZone::BUILD) == 2);
        REQUIRE(static_cast<int>(EnergyZone::PEAK) == 3);
        REQUIRE(static_cast<int>(EnergyZone::COUNT) == 4);
    }

    SECTION("AuxMode enum has correct values")
    {
        REQUIRE(static_cast<int>(AuxMode::HAT) == 0);
        REQUIRE(static_cast<int>(AuxMode::FILL_GATE) == 1);
        REQUIRE(static_cast<int>(AuxMode::PHRASE_CV) == 2);
        REQUIRE(static_cast<int>(AuxMode::EVENT) == 3);
        REQUIRE(static_cast<int>(AuxMode::COUNT) == 4);
    }

    SECTION("AuxDensity enum has correct values")
    {
        REQUIRE(static_cast<int>(AuxDensity::SPARSE) == 0);
        REQUIRE(static_cast<int>(AuxDensity::NORMAL) == 1);
        REQUIRE(static_cast<int>(AuxDensity::DENSE) == 2);
        REQUIRE(static_cast<int>(AuxDensity::BUSY) == 3);
        REQUIRE(static_cast<int>(AuxDensity::COUNT) == 4);
    }

    SECTION("VoiceCoupling enum has correct values")
    {
        REQUIRE(static_cast<int>(VoiceCoupling::INDEPENDENT) == 0);
        REQUIRE(static_cast<int>(VoiceCoupling::INTERLOCK) == 1);
        REQUIRE(static_cast<int>(VoiceCoupling::SHADOW) == 2);
        REQUIRE(static_cast<int>(VoiceCoupling::COUNT) == 3);
    }

    SECTION("ResetMode enum has correct values")
    {
        REQUIRE(static_cast<int>(ResetMode::PHRASE) == 0);
        REQUIRE(static_cast<int>(ResetMode::BAR) == 1);
        REQUIRE(static_cast<int>(ResetMode::STEP) == 2);
        REQUIRE(static_cast<int>(ResetMode::COUNT) == 3);
    }
}

TEST_CASE("DuoPulseTypes helper functions", "[types]")
{
    SECTION("GetEnergyZone returns correct zones")
    {
        REQUIRE(GetEnergyZone(0.0f) == EnergyZone::MINIMAL);
        REQUIRE(GetEnergyZone(0.10f) == EnergyZone::MINIMAL);
        REQUIRE(GetEnergyZone(0.19f) == EnergyZone::MINIMAL);

        REQUIRE(GetEnergyZone(0.20f) == EnergyZone::GROOVE);
        REQUIRE(GetEnergyZone(0.35f) == EnergyZone::GROOVE);
        REQUIRE(GetEnergyZone(0.49f) == EnergyZone::GROOVE);

        REQUIRE(GetEnergyZone(0.50f) == EnergyZone::BUILD);
        REQUIRE(GetEnergyZone(0.60f) == EnergyZone::BUILD);
        REQUIRE(GetEnergyZone(0.74f) == EnergyZone::BUILD);

        REQUIRE(GetEnergyZone(0.75f) == EnergyZone::PEAK);
        REQUIRE(GetEnergyZone(0.90f) == EnergyZone::PEAK);
        REQUIRE(GetEnergyZone(1.0f) == EnergyZone::PEAK);
    }

    SECTION("GetAuxDensityMultiplier returns correct multipliers")
    {
        REQUIRE(GetAuxDensityMultiplier(AuxDensity::SPARSE) == Approx(0.5f));
        REQUIRE(GetAuxDensityMultiplier(AuxDensity::NORMAL) == Approx(1.0f));
        REQUIRE(GetAuxDensityMultiplier(AuxDensity::DENSE) == Approx(1.5f));
        REQUIRE(GetAuxDensityMultiplier(AuxDensity::BUSY) == Approx(2.0f));
    }

    SECTION("GetVoiceCouplingFromValue maps knob correctly")
    {
        // Task 22 Phase C1: INTERLOCK removed, now 2 modes
        // 0-50% = INDEPENDENT, 50-100% = SHADOW
        REQUIRE(GetVoiceCouplingFromValue(0.0f) == VoiceCoupling::INDEPENDENT);
        REQUIRE(GetVoiceCouplingFromValue(0.20f) == VoiceCoupling::INDEPENDENT);
        REQUIRE(GetVoiceCouplingFromValue(0.40f) == VoiceCoupling::INDEPENDENT);
        REQUIRE(GetVoiceCouplingFromValue(0.49f) == VoiceCoupling::INDEPENDENT);
        REQUIRE(GetVoiceCouplingFromValue(0.50f) == VoiceCoupling::SHADOW);
        REQUIRE(GetVoiceCouplingFromValue(0.60f) == VoiceCoupling::SHADOW);
        REQUIRE(GetVoiceCouplingFromValue(0.80f) == VoiceCoupling::SHADOW);
        REQUIRE(GetVoiceCouplingFromValue(1.0f) == VoiceCoupling::SHADOW);
    }

    SECTION("GetGenreFromValue maps knob correctly")
    {
        REQUIRE(GetGenreFromValue(0.0f) == Genre::TECHNO);
        REQUIRE(GetGenreFromValue(0.20f) == Genre::TECHNO);
        REQUIRE(GetGenreFromValue(0.40f) == Genre::TRIBAL);
        REQUIRE(GetGenreFromValue(0.60f) == Genre::TRIBAL);
        REQUIRE(GetGenreFromValue(0.80f) == Genre::IDM);
        REQUIRE(GetGenreFromValue(1.0f) == Genre::IDM);
    }
}

TEST_CASE("Constants are defined correctly", "[types]")
{
    REQUIRE(kMaxSteps == 32);
    REQUIRE(kMaxPhraseSteps == 256);
    REQUIRE(kArchetypesPerGenre == 9);
    REQUIRE(kNumGenres == 3);
}

// =============================================================================
// ArchetypeDNA.h Tests
// =============================================================================

TEST_CASE("ArchetypeDNA initialization", "[archetype]")
{
    ArchetypeDNA archetype;
    archetype.Init();

    SECTION("Weights are initialized")
    {
        // Downbeats should be strongest
        REQUIRE(archetype.anchorWeights[0] == Approx(1.0f));
        REQUIRE(archetype.anchorWeights[8] == Approx(0.85f));  // Half note
        REQUIRE(archetype.anchorWeights[16] == Approx(1.0f)); // Bar 2 downbeat

        // Backbeats should be strongest for shimmer
        REQUIRE(archetype.shimmerWeights[8] == Approx(1.0f));
        REQUIRE(archetype.shimmerWeights[24] == Approx(1.0f));
    }

    SECTION("Default values are reasonable")
    {
        REQUIRE(archetype.swingAmount >= 0.0f);
        REQUIRE(archetype.swingAmount <= 1.0f);
        REQUIRE(archetype.defaultCouple >= 0.0f);
        REQUIRE(archetype.defaultCouple <= 1.0f);
        REQUIRE(archetype.fillDensityMultiplier >= 1.0f);
    }

    SECTION("Grid position defaults to origin")
    {
        REQUIRE(archetype.gridX == 0);
        REQUIRE(archetype.gridY == 0);
    }
}

TEST_CASE("GenreField initialization", "[archetype]")
{
    GenreField field;
    field.Init();

    SECTION("All 9 archetypes are initialized")
    {
        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                const auto& arch = field.GetArchetype(x, y);
                REQUIRE(arch.gridX == x);
                REQUIRE(arch.gridY == y);
            }
        }
    }

    SECTION("GetArchetype clamps out-of-range indices")
    {
        // Should not crash, should return corner archetypes
        const auto& corner00 = field.GetArchetype(-1, -1);
        REQUIRE(corner00.gridX == 0);
        REQUIRE(corner00.gridY == 0);

        const auto& corner22 = field.GetArchetype(5, 5);
        REQUIRE(corner22.gridX == 2);
        REQUIRE(corner22.gridY == 2);
    }
}

// =============================================================================
// ControlState.h Tests
// =============================================================================

TEST_CASE("PunchParams computation", "[control]")
{
    PunchParams params;

    SECTION("PUNCH=0 gives flat dynamics")
    {
        params.ComputeFromPunch(0.0f);
        REQUIRE(params.accentProbability == Approx(0.15f));
        REQUIRE(params.velocityFloor == Approx(0.70f));
        REQUIRE(params.accentBoost == Approx(0.10f));
        REQUIRE(params.velocityVariation == Approx(0.05f));
    }

    SECTION("PUNCH=1 gives maximum dynamics")
    {
        params.ComputeFromPunch(1.0f);
        REQUIRE(params.accentProbability == Approx(0.50f));
        REQUIRE(params.velocityFloor == Approx(0.30f));
        REQUIRE(params.accentBoost == Approx(0.35f));
        REQUIRE(params.velocityVariation == Approx(0.20f));
    }

    SECTION("PUNCH=0.5 gives medium dynamics")
    {
        params.ComputeFromPunch(0.5f);
        REQUIRE(params.accentProbability == Approx(0.325f));
        REQUIRE(params.velocityFloor == Approx(0.50f));
    }
}

TEST_CASE("BuildModifiers computation", "[control]")
{
    BuildModifiers mods;

    SECTION("BUILD=0 gives flat phrase")
    {
        mods.ComputeFromBuild(0.0f, 0.5f);
        REQUIRE(mods.densityMultiplier == Approx(1.0f));
        REQUIRE(mods.fillIntensity == Approx(0.0f));
        REQUIRE(mods.inFillZone == false);
    }

    SECTION("BUILD=1 at phrase end gives density boost")
    {
        mods.ComputeFromBuild(1.0f, 1.0f);
        REQUIRE(mods.densityMultiplier > 1.0f);
        REQUIRE(mods.densityMultiplier <= 1.5f);
    }

    SECTION("Fill zone detection works")
    {
        mods.ComputeFromBuild(1.0f, 0.5f);
        REQUIRE(mods.inFillZone == false);

        mods.ComputeFromBuild(1.0f, 0.9f);
        REQUIRE(mods.inFillZone == true);
        REQUIRE(mods.fillIntensity > 0.0f);
    }
}

TEST_CASE("ControlState initialization", "[control]")
{
    ControlState state;
    state.Init();

    SECTION("Performance controls have sensible defaults")
    {
        REQUIRE(state.energy == 0.6f);
        REQUIRE(state.build == 0.0f);
        REQUIRE(state.fieldX == 0.5f);
        REQUIRE(state.fieldY == 0.33f);
        REQUIRE(state.genre == Genre::TECHNO);
    }

    SECTION("Config controls have sensible defaults")
    {
        REQUIRE(state.patternLength == 32);
        REQUIRE(state.phraseLength == 4);
        REQUIRE(state.auxMode == AuxMode::HAT);
        REQUIRE(state.resetMode == ResetMode::STEP);  // Task 22: Reset mode hardcoded to STEP
    }

    SECTION("GetEffective* clamps CV modulation")
    {
        state.energy = 0.8f;
        state.energyCV = 0.5f;  // Would push to 1.3
        REQUIRE(state.GetEffectiveEnergy() == Approx(1.0f));

        state.energy = 0.2f;
        state.energyCV = -0.5f;  // Would push to -0.3
        REQUIRE(state.GetEffectiveEnergy() == Approx(0.0f));
    }
}

// =============================================================================
// SequencerState.h Tests
// =============================================================================

TEST_CASE("DriftState seed management", "[sequencer]")
{
    DriftState drift;
    drift.Init(0xABCD1234);

    SECTION("Initial seeds are set")
    {
        REQUIRE(drift.patternSeed == 0xABCD1234);
        REQUIRE(drift.phraseSeed != 0);  // Should be derived
        REQUIRE(drift.phraseCounter == 0);
    }

    SECTION("Phrase boundary changes phrase seed")
    {
        uint32_t oldPhraseSeed = drift.phraseSeed;
        drift.OnPhraseBoundary();
        REQUIRE(drift.phraseSeed != oldPhraseSeed);
        REQUIRE(drift.phraseCounter == 1);
    }

    SECTION("Reseed changes pattern seed")
    {
        uint32_t oldPatternSeed = drift.patternSeed;
        drift.RequestReseed();
        drift.OnPhraseBoundary();
        REQUIRE(drift.patternSeed != oldPatternSeed);
        REQUIRE(drift.reseedRequested == false);
    }

    SECTION("GetSeedForStep returns correct seed based on stability")
    {
        // High stability step (downbeat) should use pattern seed at low drift
        uint32_t seed = drift.GetSeedForStep(0.2f, 1.0f);
        REQUIRE(seed == drift.patternSeed);

        // Low stability step should use phrase seed
        seed = drift.GetSeedForStep(0.8f, 0.2f);
        REQUIRE(seed == drift.phraseSeed);
    }
}

TEST_CASE("GuardRailState tracking", "[sequencer]")
{
    GuardRailState rails;
    rails.Init();

    SECTION("Initial state is clean")
    {
        REQUIRE(rails.stepsSinceLastAnchor == 0);
        REQUIRE(rails.consecutiveShimmerHits == 0);
        REQUIRE(rails.downbeatForced == false);
    }

    SECTION("Anchor hit resets counters")
    {
        rails.OnNoHit();
        rails.OnNoHit();
        rails.OnShimmerOnlyHit();
        REQUIRE(rails.stepsSinceLastAnchor == 2);
        REQUIRE(rails.consecutiveShimmerHits == 1);

        rails.OnAnchorHit();
        REQUIRE(rails.stepsSinceLastAnchor == 0);
        REQUIRE(rails.consecutiveShimmerHits == 0);
    }
}

TEST_CASE("SequencerState position tracking", "[sequencer]")
{
    SequencerState state;
    state.Init();

    SECTION("Initial position is at start")
    {
        REQUIRE(state.currentStep == 0);
        REQUIRE(state.currentBar == 0);
        REQUIRE(state.currentPhrase == 0);
        REQUIRE(state.isBarBoundary == true);
        REQUIRE(state.isPhraseBoundary == true);
    }

    SECTION("AdvanceStep increments correctly")
    {
        state.AdvanceStep(32, 4);
        REQUIRE(state.currentStep == 1);
        REQUIRE(state.isBarBoundary == false);
        REQUIRE(state.isPhraseBoundary == false);
    }

    SECTION("AdvanceStep wraps at pattern length")
    {
        for (int i = 0; i < 32; ++i)
        {
            state.AdvanceStep(32, 4);
        }
        REQUIRE(state.currentStep == 0);
        REQUIRE(state.currentBar == 1);
        REQUIRE(state.isBarBoundary == true);
    }

    SECTION("AdvanceStep wraps at phrase length")
    {
        // Advance through 4 bars of 32 steps
        for (int i = 0; i < 32 * 4; ++i)
        {
            state.AdvanceStep(32, 4);
        }
        REQUIRE(state.currentStep == 0);
        REQUIRE(state.currentBar == 0);
        REQUIRE(state.currentPhrase == 1);
        REQUIRE(state.isPhraseBoundary == true);
    }

    SECTION("Reset modes work correctly")
    {
        // Advance to middle of phrase
        for (int i = 0; i < 50; ++i)
        {
            state.AdvanceStep(32, 4);
        }

        state.Reset(ResetMode::STEP, 32);
        REQUIRE(state.currentStep == 0);
        REQUIRE(state.currentBar == 1);  // Bar unchanged

        state.currentStep = 10;
        state.Reset(ResetMode::BAR, 32);
        REQUIRE(state.currentStep == 0);
        REQUIRE(state.isBarBoundary == true);

        state.currentBar = 2;
        state.currentStep = 10;
        state.Reset(ResetMode::PHRASE, 32);
        REQUIRE(state.currentStep == 0);
        REQUIRE(state.currentBar == 0);
        REQUIRE(state.isPhraseBoundary == true);
    }

    SECTION("Hit mask queries work")
    {
        state.anchorMask = 0x00000005;  // Steps 0 and 2
        state.shimmerMask = 0x00000002; // Step 1
        state.auxMask = 0x00000004;     // Step 2

        state.currentStep = 0;
        REQUIRE(state.AnchorFires() == true);
        REQUIRE(state.ShimmerFires() == false);

        state.currentStep = 1;
        REQUIRE(state.AnchorFires() == false);
        REQUIRE(state.ShimmerFires() == true);

        state.currentStep = 2;
        REQUIRE(state.AnchorFires() == true);
        REQUIRE(state.AuxFires() == true);
    }
}

// =============================================================================
// OutputState.h Tests
// =============================================================================

TEST_CASE("TriggerState behavior", "[output]")
{
    TriggerState trigger;
    trigger.Init(48);  // 48 samples = 1ms at 48kHz

    SECTION("Initial state is low")
    {
        REQUIRE(trigger.high == false);
        REQUIRE(trigger.samplesRemaining == 0);
    }

    SECTION("Fire sets trigger high")
    {
        trigger.Fire();
        REQUIRE(trigger.high == true);
        REQUIRE(trigger.samplesRemaining == 48);
    }

    SECTION("Process decrements and clears")
    {
        trigger.Fire();
        for (int i = 0; i < 47; ++i)
        {
            trigger.Process();
            REQUIRE(trigger.high == true);
        }
        trigger.Process();  // 48th sample
        REQUIRE(trigger.high == false);
    }
}

TEST_CASE("VelocityOutputState sample & hold", "[output]")
{
    VelocityOutputState vel;
    vel.Init();

    SECTION("Initial voltage is zero")
    {
        REQUIRE(vel.heldVoltage == Approx(0.0f));
        REQUIRE(vel.GetVoltage() == Approx(0.0f));
    }

    SECTION("Trigger updates held voltage")
    {
        vel.Trigger(0.7f);
        REQUIRE(vel.heldVoltage == Approx(0.7f));
        REQUIRE(vel.GetVoltage() == Approx(3.5f));  // 0.7 * 5V
        REQUIRE(vel.triggered == true);
    }

    SECTION("Voltage persists until next trigger")
    {
        vel.Trigger(0.5f);
        for (int i = 0; i < 1000; ++i)
        {
            vel.Process();
        }
        REQUIRE(vel.heldVoltage == Approx(0.5f));  // Still held
    }

    SECTION("Velocity is clamped")
    {
        vel.Trigger(1.5f);
        REQUIRE(vel.heldVoltage == Approx(1.0f));

        vel.Trigger(-0.5f);
        REQUIRE(vel.heldVoltage == Approx(0.0f));
    }
}

TEST_CASE("LEDState behavior", "[output]")
{
    LEDState led;
    led.Init(48000.0f);

    SECTION("Initial brightness is zero")
    {
        REQUIRE(led.brightness == Approx(0.0f));
        REQUIRE(led.GetBrightness() == Approx(0.0f));
    }

    SECTION("Trigger sets brightness")
    {
        led.Trigger(0.8f);
        REQUIRE(led.brightness == Approx(0.8f));
    }

    SECTION("Flash overrides brightness")
    {
        led.Trigger(0.3f);
        led.Flash(100);
        REQUIRE(led.GetBrightness() == Approx(1.0f));
    }
}

TEST_CASE("OutputState integration", "[output]")
{
    OutputState outputs;
    outputs.Init(48000.0f);

    SECTION("FireAnchor triggers all related outputs")
    {
        outputs.FireAnchor(0.9f, true);
        REQUIRE(outputs.anchorTrigger.high == true);
        REQUIRE(outputs.anchorVelocity.heldVoltage == Approx(0.9f));
        REQUIRE(outputs.led.brightness > 0.0f);
    }

    SECTION("FireShimmer triggers all related outputs")
    {
        outputs.FireShimmer(0.6f, false);
        REQUIRE(outputs.shimmerTrigger.high == true);
        REQUIRE(outputs.shimmerVelocity.heldVoltage == Approx(0.6f));
        REQUIRE(outputs.led.brightness > 0.0f);
    }
}

// =============================================================================
// DuoPulseState.h Tests
// =============================================================================

TEST_CASE("DuoPulseState initialization", "[state]")
{
    DuoPulseState state;
    state.Init(48000.0f);

    SECTION("Sample rate is set")
    {
        REQUIRE(state.sampleRate == 48000.0f);
    }

    SECTION("Default BPM is 120")
    {
        REQUIRE(state.currentBpm == 120.0f);
    }

    SECTION("Samples per step is calculated correctly")
    {
        // At 120 BPM, 16th notes: 60 / 120 / 4 = 0.125 seconds
        // 0.125 * 48000 = 6000 samples
        REQUIRE(state.samplesPerStep == Approx(6000.0f));
    }

    SECTION("Running state is true")
    {
        REQUIRE(state.running == true);
    }
}

TEST_CASE("DuoPulseState BPM changes", "[state]")
{
    DuoPulseState state;
    state.Init(48000.0f);

    SECTION("SetBpm updates samples per step")
    {
        state.SetBpm(60.0f);  // Half tempo
        REQUIRE(state.samplesPerStep == Approx(12000.0f));

        state.SetBpm(240.0f);  // Double tempo
        REQUIRE(state.samplesPerStep == Approx(3000.0f));
    }

    SECTION("BPM is clamped to valid range")
    {
        state.SetBpm(10.0f);  // Too slow
        REQUIRE(state.currentBpm == 30.0f);

        state.SetBpm(500.0f);  // Too fast
        REQUIRE(state.currentBpm == 300.0f);
    }
}

TEST_CASE("DuoPulseState step advancement", "[state]")
{
    DuoPulseState state;
    state.Init(48000.0f);

    SECTION("ShouldAdvanceStep works correctly")
    {
        REQUIRE(state.ShouldAdvanceStep() == false);

        state.stepSampleCounter = 6000;  // At samples per step
        REQUIRE(state.ShouldAdvanceStep() == true);
    }

    SECTION("ProcessSample increments counter")
    {
        REQUIRE(state.stepSampleCounter == 0);
        state.ProcessSample();
        REQUIRE(state.stepSampleCounter == 1);
    }

    SECTION("AdvanceStep resets counter and advances position")
    {
        state.stepSampleCounter = 6000;
        state.AdvanceStep();
        REQUIRE(state.stepSampleCounter == 0);
        REQUIRE(state.sequencer.currentStep == 1);
    }
}

// =============================================================================
// Struct Size Sanity Checks
// =============================================================================

TEST_CASE("Struct sizes are reasonable", "[sizes]")
{
    // These tests ensure structs don't accidentally grow too large
    // (which could indicate memory issues or padding problems)

    SECTION("ArchetypeDNA is reasonable size")
    {
        // 3 arrays of 32 floats (3 * 32 * 4 = 384 bytes)
        // Plus masks, timing, metadata (~40 bytes)
        // Should be under 512 bytes
        REQUIRE(sizeof(ArchetypeDNA) < 512);
    }

    SECTION("ControlState is reasonable size")
    {
        // Should be under 512 bytes
        REQUIRE(sizeof(ControlState) < 512);
    }

    SECTION("SequencerState is reasonable size")
    {
        // Hit masks (8 * 5 = 40 bytes)
        // Timing arrays (32 * 2 * 2 = 128 bytes)
        // Plus sub-structs and flags
        // Should be under 512 bytes
        REQUIRE(sizeof(SequencerState) < 512);
    }

    SECTION("OutputState is reasonable size")
    {
        // Multiple trigger states and velocity states
        // Should be under 256 bytes
        REQUIRE(sizeof(OutputState) < 256);
    }

    SECTION("DuoPulseState is reasonable size")
    {
        // Complete state including GenreField (9 archetypes)
        // Could be a few KB, but should be under 8KB
        REQUIRE(sizeof(DuoPulseState) < 8192);
    }
}
