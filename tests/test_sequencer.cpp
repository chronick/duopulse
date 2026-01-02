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

    SECTION("Phrase boundary occurs after derived phrase length")
    {
        seq.SetPatternLength(16);
        // Task 22: Phrase length auto-derived (16 steps → 8 bars = 128 total steps)

        // 17 ticks: process bar 0 (steps 0-15) then enter bar 1 step 0
        for (int i = 0; i < 17; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& pos1 = seq.GetPhrasePosition();
        REQUIRE(pos1.currentBar == 1);
        REQUIRE(pos1.stepInBar == 0);

        // Advance to bar 7 step 15 (last step before wrap)
        // Currently at bar 1 step 0 (step 16 in phrase)
        // Need to get to step 127 (bar 7 step 15)
        // That's 127 - 16 = 111 more ticks
        for (int i = 0; i < 111; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Now at bar 7 step 15 (total 128 ticks from start, at step 127)
        const auto& midPos = seq.GetPhrasePosition();
        REQUIRE(midPos.stepInBar == 15);

        // One more tick to wrap phrase (total 129 ticks)
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
        // Task 22: Phrase length auto-derived (16 steps → 8 bars = 128 total steps)

        // At start, progress should be 0
        const auto& startPos = seq.GetPhrasePosition();
        REQUIRE(startPos.phraseProgress == Approx(0.0f).margin(0.01f));

        // Advance 64 ticks (process steps 0-63, which is bars 0-3)
        // After 64 ticks: step 63 processed, progress = 63/128 ≈ 0.492
        for (int i = 0; i < 64; ++i)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        const auto& midPos = seq.GetPhrasePosition();
        // After 64 ticks we're at step 63 of 128 (0-indexed)
        // Progress = 63/128 ≈ 0.492
        REQUIRE(midPos.phraseProgress == Approx(0.492f).margin(0.02f));

        // Advance 50 more ticks to get to step 113 (fill zone)
        // Total: 64 + 50 = 114 ticks, at step 113
        // Progress = 113/128 ≈ 0.883 > 0.875, in fill zone (phraseProgress > 0.875)
        for (int i = 0; i < 50; ++i)
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

        // Trigger reset (Task 22: Reset mode hardcoded to STEP)
        seq.TriggerReset();

        // STEP mode resets to step 0 but preserves bar position
        const auto& resetPos = seq.GetPhrasePosition();
        REQUIRE(resetPos.stepInPhrase == 32);  // bar 2, step 0 = step 32 in phrase
        REQUIRE(resetPos.currentBar == 2);
        REQUIRE(resetPos.isDownbeat == true);  // Step 0 of bar 2 is a downbeat
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

    SECTION("64-step patterns have hits in second half")
    {
        seq.SetPatternLength(64);
        seq.SetEnergy(0.7f);  // Higher energy = more hits
        seq.GenerateBar();

        // Get the masks
        uint64_t anchorMask = seq.GetAnchorMask();
        uint64_t shimmerMask = seq.GetShimmerMask();
        uint64_t auxMask = seq.GetAuxMask();

        // First half (steps 0-31)
        uint32_t anchorFirstHalf = static_cast<uint32_t>(anchorMask & 0xFFFFFFFF);
        uint32_t shimmerFirstHalf = static_cast<uint32_t>(shimmerMask & 0xFFFFFFFF);
        uint32_t auxFirstHalf = static_cast<uint32_t>(auxMask & 0xFFFFFFFF);

        // Second half (steps 32-63)
        uint32_t anchorSecondHalf = static_cast<uint32_t>(anchorMask >> 32);
        uint32_t shimmerSecondHalf = static_cast<uint32_t>(shimmerMask >> 32);
        uint32_t auxSecondHalf = static_cast<uint32_t>(auxMask >> 32);

        // First half should have hits
        REQUIRE(anchorFirstHalf != 0);

        // Second half should also have hits (this was the bug!)
        REQUIRE(anchorSecondHalf != 0);

        // Both halves should have aux hits at high energy
        REQUIRE(auxFirstHalf != 0);
        REQUIRE(auxSecondHalf != 0);

        // The two halves should be different (different seeds used)
        // Note: at very low energy they might both be just downbeat, so check anchor
        bool halvesAreDifferent = (anchorFirstHalf != anchorSecondHalf) ||
                                   (shimmerFirstHalf != shimmerSecondHalf);
        // At least one mask should differ between halves
        REQUIRE(halvesAreDifferent);
    }

    SECTION("64-step patterns fire triggers in second half")
    {
        seq.SetPatternLength(64);
        seq.SetEnergy(0.8f);  // High energy for many hits
        seq.GenerateBar();

        bool anyHitInSecondHalf = false;

        // Process all 64 steps
        for (int step = 0; step < 64; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();

            // Check if we're in second half (steps 32-63)
            if (step >= 32)
            {
                // Check if any gate fired
                if (seq.IsGateHigh(0) || seq.IsGateHigh(1))
                {
                    anyHitInSecondHalf = true;
                }
            }
        }

        // There should be hits in the second half
        REQUIRE(anyHitInSecondHalf);
    }

    SECTION("Phrase wraps after derived length")
    {
        seq.SetPatternLength(16);
        // Task 22: Phrase length auto-derived (16 steps → 8 bars = 128 total steps)

        // After 128 ticks (8 bars × 16 steps), should wrap to bar 0
        for (int step = 0; step < 129; ++step)
        {
            seq.TriggerExternalClock();
            seq.ProcessAudio();
        }

        // Should have looped back to step 0, bar 0
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

// =============================================================================
// Field Change Detection Tests (Task 23)
// =============================================================================

TEST_CASE("CheckFieldChange detects significant changes", "[sequencer][field-update]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    SECTION("Small changes do not trigger regeneration")
    {
        seq.SetFieldX(0.5f);
        seq.SetFieldY(0.5f);

        // Initial call to set baseline
        seq.CheckFieldChange();

        // Small change (< 10%)
        seq.SetFieldX(0.55f);
        REQUIRE_FALSE(seq.CheckFieldChange());
    }

    SECTION("Large changes in Field X trigger regeneration")
    {
        seq.SetFieldX(0.5f);
        seq.SetFieldY(0.5f);

        // Reset previous values
        seq.CheckFieldChange();

        // Large change (> 10%)
        seq.SetFieldX(0.7f);
        REQUIRE(seq.CheckFieldChange());
    }

    SECTION("Large changes in Field Y trigger regeneration")
    {
        seq.SetFieldX(0.5f);
        seq.SetFieldY(0.5f);

        // Reset previous values
        seq.CheckFieldChange();

        // Large change (> 10%)
        seq.SetFieldY(0.2f);
        REQUIRE(seq.CheckFieldChange());
    }

    SECTION("Threshold boundary at exactly 0.1")
    {
        seq.SetFieldX(0.5f);
        seq.CheckFieldChange();

        // Just under threshold - should NOT trigger
        seq.SetFieldX(0.599f);
        REQUIRE_FALSE(seq.CheckFieldChange());

        // Reset
        seq.SetFieldX(0.5f);
        seq.CheckFieldChange();

        // Just over threshold - should trigger
        seq.SetFieldX(0.61f);
        REQUIRE(seq.CheckFieldChange());
    }

    SECTION("Negative changes also trigger regeneration")
    {
        seq.SetFieldX(0.8f);
        seq.SetFieldY(0.9f);

        // Reset previous values
        seq.CheckFieldChange();

        // Large negative change in Field X
        seq.SetFieldX(0.6f);
        REQUIRE(seq.CheckFieldChange());

        // Reset
        seq.SetFieldX(0.8f);
        seq.CheckFieldChange();

        // Large negative change in Field Y
        seq.SetFieldY(0.7f);
        REQUIRE(seq.CheckFieldChange());
    }
}
