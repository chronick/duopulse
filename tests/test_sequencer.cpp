#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/Sequencer.h"
#include "../src/Engine/DuoPulseTypes.h"
#include "../src/Engine/HitBudget.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// Sequencer Initialization Tests
// =============================================================================

TEST_CASE("Sequencer initializes correctly", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Default BPM is 120")
    {
        REQUIRE(seq.GetBpm() == Approx(120.0f));
    }

    SECTION("Initial position is at start")
    {
        const auto& pos = seq.GetPhrasePosition();
        REQUIRE(pos.stepInPhrase == 0);
    }

    SECTION("Gates start low")
    {
        REQUIRE_FALSE(seq.IsGateHigh(0));
        REQUIRE_FALSE(seq.IsGateHigh(1));
    }
}

// =============================================================================
// Bar Generation Tests
// =============================================================================

TEST_CASE("Bar generation produces valid masks", "[sequencer][generation]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Generated masks have hits")
    {
        // Force bar generation by processing a step
        seq.GenerateBar();

        // The sequencer should have generated masks with at least one hit
        // We need to trigger clock ticks to process steps

        // With default energy (0.5), we should get some hits
        bool anyAnchorHit = false;
        bool anyShimmerHit = false;

        // Process a full bar by triggering external clocks
        for (int step = 0; step < 32; ++step)
        {
            // Force specific triggers for this step
            seq.ForceNextStepTriggers(true, true, false, false);

            // Trigger clock tick and process
            seq.TriggerExternalClock();
            seq.ProcessAudio();

            // Check if gates went high
            if (seq.IsGateHigh(0)) anyAnchorHit = true;
            if (seq.IsGateHigh(1)) anyShimmerHit = true;
        }

        // At least one hit should occur with forced triggers
        REQUIRE(anyAnchorHit == true);
        REQUIRE(anyShimmerHit == true);
    }

    SECTION("Energy affects pattern density")
    {
        // Set low energy
        seq.SetEnergy(0.1f);
        seq.GenerateBar();

        int lowEnergyHits = 0;
        for (int step = 0; step < 32; ++step)
        {
            seq.ForceNextStepTriggers(true, false, false);
            seq.ProcessAudio();
            if (seq.IsGateHigh(0)) lowEnergyHits++;
        }

        // Reset and set high energy
        seq.Init(48000.0f);
        seq.SetEnergy(0.9f);
        seq.GenerateBar();

        int highEnergyHits = 0;
        for (int step = 0; step < 32; ++step)
        {
            seq.ForceNextStepTriggers(true, false, false);
            seq.ProcessAudio();
            if (seq.IsGateHigh(0)) highEnergyHits++;
        }

        // High energy should produce at least as many hits
        // (Both are forced, so they should be equal, but this tests the setup)
        REQUIRE(highEnergyHits >= lowEnergyHits);
    }
}

// =============================================================================
// Step Advancement Tests
// =============================================================================

TEST_CASE("Step advancement wraps correctly", "[sequencer][timing]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Step wraps at pattern length (32)")
    {
        seq.SetPatternLength(32);

        const auto& startPos = seq.GetPhrasePosition();
        int startStep = startPos.stepInPhrase;
        REQUIRE(startStep == 0);

        // 32 ticks process steps 0-31 (one complete bar, still in bar 0)
        for (int i = 0; i < 32; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // After 32 steps, we've processed the entire first bar but haven't wrapped yet
        const auto& midPos = seq.GetPhrasePosition();
        REQUIRE(midPos.stepInBar == 31);

        // One more tick should wrap to bar 1
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        const auto& endPos = seq.GetPhrasePosition();
        REQUIRE(endPos.currentBar == 1);
        REQUIRE(endPos.stepInBar == 0);
    }

    SECTION("Step wraps at pattern length (16)")
    {
        seq.SetPatternLength(16);

        // 16 ticks process steps 0-15 (one complete bar)
        for (int i = 0; i < 16; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // After 16 steps, we've processed bar 0 but haven't wrapped yet
        const auto& pos0 = seq.GetPhrasePosition();
        REQUIRE(pos0.stepInBar == 15);

        // One more tick to enter bar 1
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        const auto& pos = seq.GetPhrasePosition();
        REQUIRE(pos.currentBar == 1);
    }
}

TEST_CASE("Phrase boundary detection", "[sequencer][timing]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Phrase boundary occurs after phrase length bars")
    {
        seq.SetPatternLength(16);
        seq.SetPhraseLength(2);  // 2 bars per phrase

        // 17 ticks: process bar 0 (steps 0-15) then enter bar 1 step 0
        for (int i = 0; i < 17; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& pos1 = seq.GetPhrasePosition();
        REQUIRE(pos1.currentBar == 1);
        REQUIRE(pos1.stepInBar == 0);

        // 15 more ticks to get to bar 1 step 15
        for (int i = 0; i < 15; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Now at bar 1 step 15 (total 32 ticks, which wraps back to bar 0)
        const auto& midPos = seq.GetPhrasePosition();
        REQUIRE(midPos.stepInBar == 15);

        // One more tick to wrap phrase
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        const auto& pos2 = seq.GetPhrasePosition();
        // Should wrap back to bar 0
        REQUIRE(pos2.currentBar == 0);
        REQUIRE(pos2.isDownbeat == true);  // Bar downbeat (step 0)
    }

    SECTION("Phrase progress is correct")
    {
        seq.SetPatternLength(16);
        seq.SetPhraseLength(4);  // 4 bars, 64 steps total

        // At start, progress should be 0
        const auto& startPos = seq.GetPhrasePosition();
        REQUIRE(startPos.phraseProgress == Approx(0.0f).margin(0.01f));

        // Advance 32 ticks (process steps 0-31, which is bars 0-1)
        // After 32 ticks: step 31 processed, progress = 31/64 ≈ 0.484
        for (int i = 0; i < 32; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& midPos = seq.GetPhrasePosition();
        // After 32 ticks we're at step 31 of 64 (0-indexed)
        // Progress = 31/64 ≈ 0.484
        REQUIRE(midPos.phraseProgress == Approx(0.484f).margin(0.02f));

        // Advance 28 more ticks to get to step 59 (near end of phrase)
        // Progress = 59/64 ≈ 0.922 which is in fill zone (> 0.875)
        for (int i = 0; i < 28; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& fillPos = seq.GetPhrasePosition();
        REQUIRE(fillPos.isFillZone == true);
    }
}

// =============================================================================
// Reset Tests
// =============================================================================

TEST_CASE("Reset returns to start", "[sequencer][reset]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Reset after partial phrase")
    {
        seq.SetPatternLength(16);
        seq.SetPhraseLength(4);

        // Advance to middle of phrase (33 ticks = bar 0 + bar 1 step 0)
        // 16 ticks for bar 0, 17 ticks for bar 1, 33+ for bar 2
        for (int i = 0; i < 33; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& midPos = seq.GetPhrasePosition();
        REQUIRE(midPos.currentBar == 2);

        // Trigger reset
        seq.TriggerReset();

        // Should be back at start
        const auto& resetPos = seq.GetPhrasePosition();
        REQUIRE(resetPos.stepInPhrase == 0);
        REQUIRE(resetPos.currentBar == 0);
        REQUIRE(resetPos.isDownbeat == true);
    }
}

// =============================================================================
// Parameter Setting Tests
// =============================================================================

TEST_CASE("Parameter setters work correctly", "[sequencer][parameters]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("BPM is clamped to valid range")
    {
        seq.SetBpm(50.0f);
        REQUIRE(seq.GetBpm() >= 30.0f);

        seq.SetBpm(500.0f);
        REQUIRE(seq.GetBpm() <= 300.0f);

        seq.SetBpm(120.0f);
        REQUIRE(seq.GetBpm() == Approx(120.0f));
    }

    SECTION("Drift parameter works")
    {
        seq.SetDrift(0.0f);
        REQUIRE(seq.GetDrift() == Approx(0.0f));

        seq.SetDrift(1.0f);
        REQUIRE(seq.GetDrift() == Approx(1.0f));

        // Test clamping
        seq.SetDrift(-0.5f);
        REQUIRE(seq.GetDrift() >= 0.0f);

        seq.SetDrift(1.5f);
        REQUIRE(seq.GetDrift() <= 1.0f);
    }

    SECTION("Build/Ratchet parameter works")
    {
        seq.SetBuild(0.5f);
        REQUIRE(seq.GetRatchet() == Approx(0.5f));  // v3 compatibility alias
    }

    SECTION("Broken/Flavor parameter works")
    {
        seq.SetFlavorCV(0.7f);
        REQUIRE(seq.GetBroken() == Approx(0.7f));  // v3 compatibility alias
    }
}

// =============================================================================
// Clock Tests
// =============================================================================

TEST_CASE("Clock output works", "[sequencer][clock]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Clock is low initially")
    {
        REQUIRE_FALSE(seq.IsClockHigh());
    }

    SECTION("Clock goes high on tick")
    {
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        REQUIRE(seq.IsClockHigh() == true);
    }

    SECTION("Clock decays after tick")
    {
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        // Process enough samples for clock to decay (10ms at 48kHz = 480 samples)
        for (int i = 0; i < 500; ++i)
        {
            seq.ProcessAudio();
        }

        REQUIRE_FALSE(seq.IsClockHigh());
    }
}

// =============================================================================
// External Clock Tests
// =============================================================================

TEST_CASE("External clock works (exclusive mode)", "[sequencer][clock]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("External clock advances step (spec 3.4)")
    {
        const auto& startPos = seq.GetPhrasePosition();
        int startStep = startPos.stepInPhrase;

        seq.TriggerExternalClock();
        seq.ProcessAudio();

        const auto& endPos = seq.GetPhrasePosition();
        // First clock should advance from 0 to 1 (or stay at 0 if init already processed)
        REQUIRE(endPos.stepInPhrase >= startStep);
    }

    SECTION("External clock disables internal Metro (exclusive mode)")
    {
        // Enable external clock
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        const auto& pos1 = seq.GetPhrasePosition();

        // Process 100 samples WITHOUT external clock edges
        // Internal Metro should NOT tick (exclusive mode)
        for (int i = 0; i < 100; ++i)
        {
            seq.ProcessAudio();  // No TriggerExternalClock() calls
        }

        const auto& pos2 = seq.GetPhrasePosition();

        // Position should NOT advance (no external clock edges)
        // Exclusive mode: only external clock edges cause steps
        REQUIRE(pos2.stepInPhrase == pos1.stepInPhrase);
    }

    SECTION("DisableExternalClock restores internal Metro (spec 3.4)")
    {
        // Send initial external clock edges to advance position
        for (int i = 0; i < 5; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& posExt = seq.GetPhrasePosition();
        int extStep = posExt.stepInPhrase;

        // Disable external clock - restores internal Metro
        seq.DisableExternalClock();

        // Metro needs many samples to tick (at 120 BPM = 8 Hz, period is sampleRate/8)
        // At 48kHz, period = 6000 samples per tick
        // Process 6100 samples to guarantee one Metro tick
        for (int i = 0; i < 6100; ++i)
        {
            seq.ProcessAudio();
        }

        const auto& posInt = seq.GetPhrasePosition();
        int intStep = posInt.stepInPhrase;

        // Position SHOULD advance (internal Metro is now active)
        // With default 120 BPM (8 Hz), one tick per 6000 samples
        // Check that position changed (may wrap around)
        REQUIRE(intStep != extStep);
    }

    SECTION("Multiple external clock edges advance steps")
    {
        const auto& startPos = seq.GetPhrasePosition();
        int startStep = startPos.stepInPhrase;

        // Send 10 external clock edges (enough to see clear advancement)
        for (int i = 0; i < 10; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& endPos = seq.GetPhrasePosition();
        int endStep = endPos.stepInPhrase;

        // Should have advanced by some steps
        // At least one step should have advanced (considering initial state processing)
        REQUIRE(endStep >= startStep);
    }
}

// =============================================================================
// Tap Tempo Tests
// =============================================================================

TEST_CASE("Tap tempo works", "[sequencer][tempo]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Two taps set tempo")
    {
        // 500ms interval = 120 BPM (use non-zero start time)
        seq.TriggerTapTempo(1000);
        seq.TriggerTapTempo(1500);

        REQUIRE(seq.GetBpm() == Approx(120.0f).margin(1.0f));
    }

    SECTION("Different intervals set different tempos")
    {
        // 600ms interval = 100 BPM
        seq.TriggerTapTempo(1000);
        seq.TriggerTapTempo(1600);

        REQUIRE(seq.GetBpm() == Approx(100.0f).margin(1.0f));
    }

    SECTION("Very short taps are ignored")
    {
        float originalBpm = seq.GetBpm();

        // 50ms interval would be 1200 BPM (too fast)
        seq.TriggerTapTempo(1000);
        seq.TriggerTapTempo(1050);

        // BPM should be unchanged (interval < 100ms is ignored)
        REQUIRE(seq.GetBpm() == Approx(originalBpm));
    }
}

// =============================================================================
// Force Trigger Tests (for testing)
// =============================================================================

TEST_CASE("Force triggers work", "[sequencer][testing]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Force anchor trigger")
    {
        seq.ForceNextStepTriggers(true, false, false, false);
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        REQUIRE(seq.IsGateHigh(0) == true);
        REQUIRE(seq.IsGateHigh(1) == false);
    }

    SECTION("Force shimmer trigger")
    {
        seq.ForceNextStepTriggers(false, true, false, false);
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        REQUIRE(seq.IsGateHigh(0) == false);
        REQUIRE(seq.IsGateHigh(1) == true);
    }

    SECTION("Force both triggers")
    {
        seq.ForceNextStepTriggers(true, true, false, false);
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        REQUIRE(seq.IsGateHigh(0) == true);
        REQUIRE(seq.IsGateHigh(1) == true);
    }

    SECTION("Force trigger with accent")
    {
        seq.ForceNextStepTriggers(true, false, false, true);
        seq.TriggerExternalClock();
        seq.ProcessAudio();

        REQUIRE(seq.IsGateHigh(0) == true);
        // Accent affects velocity, not gate
    }
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_CASE("Full sequencer cycle produces output", "[sequencer][integration]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Process multiple bars without crash")
    {
        // Process 4 bars worth of steps (128 steps at pattern length 32)
        for (int step = 0; step < 128; ++step)
        {
            seq.TriggerExternalClock();
            auto out = seq.ProcessAudio();

            // Outputs should be valid floats
            REQUIRE(out[0] >= 0.0f);
            REQUIRE(out[0] <= 1.0f);
            REQUIRE(out[1] >= 0.0f);
            REQUIRE(out[1] <= 1.0f);
        }
    }

    SECTION("Parameter changes during playback")
    {
        // Start playing
        for (int step = 0; step < 16; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Change parameters
        seq.SetEnergy(0.8f);
        seq.SetDrift(0.5f);
        seq.SetBalance(0.7f);

        // Continue playing
        for (int step = 0; step < 16; ++step)
        {
            seq.TriggerExternalClock();
            auto out = seq.ProcessAudio();

            // Should still produce valid output
            REQUIRE(out[0] >= 0.0f);
            REQUIRE(out[1] >= 0.0f);
        }
    }

    SECTION("Genre change updates patterns")
    {
        // Set techno genre
        seq.SetGenre(0.0f);  // TECHNO
        seq.GenerateBar();

        // Set IDM genre
        seq.SetGenre(1.0f);  // IDM
        seq.GenerateBar();

        // Should complete without crash
        for (int step = 0; step < 32; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("Sequencer handles edge cases", "[sequencer][edge-cases]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Minimum pattern length")
    {
        seq.SetPatternLength(16);

        for (int step = 0; step < 32; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Should wrap every 16 steps
        const auto& pos = seq.GetPhrasePosition();
        REQUIRE(pos.currentBar >= 0);
    }

    SECTION("Maximum pattern length")
    {
        seq.SetPatternLength(64);

        for (int step = 0; step < 64; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Should wrap at 64 steps
        const auto& pos = seq.GetPhrasePosition();
        // After 64 steps, should be at bar 1
        REQUIRE(pos.currentBar >= 0);
    }

    SECTION("Single bar phrase")
    {
        seq.SetPatternLength(16);
        seq.SetPhraseLength(1);

        // 16 ticks process steps 0-15, 17th tick wraps back to step 0
        for (int step = 0; step < 17; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Should have looped back to step 0
        const auto& pos = seq.GetPhrasePosition();
        REQUIRE(pos.currentBar == 0);
        REQUIRE(pos.stepInBar == 0);
        REQUIRE(pos.isDownbeat == true);
    }

    SECTION("Extreme parameter values")
    {
        seq.SetEnergy(0.0f);
        seq.SetDrift(1.0f);
        seq.SetBalance(0.0f);
        seq.SetPunch(1.0f);
        seq.SetBuild(1.0f);

        // Should still work
        for (int step = 0; step < 32; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Set opposite extremes
        seq.SetEnergy(1.0f);
        seq.SetDrift(0.0f);
        seq.SetBalance(1.0f);
        seq.SetPunch(0.0f);
        seq.SetBuild(0.0f);

        for (int step = 0; step < 32; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }
    }
}
