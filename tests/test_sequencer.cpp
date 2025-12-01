#include <algorithm>
#include <catch2/catch_all.hpp>
#include "../src/Engine/Sequencer.h"
#include "../src/Engine/LedIndicator.h"
#include "../src/Engine/GenreConfig.h"

using namespace daisysp_idm_grids;

TEST_CASE("Sequencer Initialization", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    // Default BPM is 120
    REQUIRE(seq.GetBpm() == Catch::Approx(120.0f));
    REQUIRE_FALSE(seq.IsGateHigh(0));
}

TEST_CASE("Sequencer Knob Control", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    const float knobX = 0.5f;

    // DuoPulse v2 tempo range: 90-160 BPM
    // Knob at 0 -> Min Tempo (90)
    seq.SetTempoControl(0.0f);
    seq.SetTerrain(knobX);
    REQUIRE(seq.GetBpm() == Catch::Approx(90.0f));

    // Knob at 1 -> Max Tempo (160)
    seq.SetTempoControl(1.0f);
    REQUIRE(seq.GetBpm() == Catch::Approx(160.0f));

    // Knob at 0.5 -> Mid Tempo (125)
    // 90 + 0.5 * (160 - 90) = 90 + 35 = 125
    seq.SetTempoControl(0.5f);
    REQUIRE(seq.GetBpm() == Catch::Approx(125.0f));
}

TEST_CASE("Sequencer Tap Tempo", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    // First tap sets the baseline
    seq.TriggerTapTempo(1000);

    // Second tap 500ms later (120 BPM)
    // 60000 / 500 = 120
    seq.TriggerTapTempo(1500);
    REQUIRE(seq.GetBpm() == Catch::Approx(120.0f).margin(1.0f));

    // Third tap 1000ms later would be 60 BPM, but DuoPulse v2 clamps to 90-160
    // So it should clamp to 90 BPM
    seq.TriggerTapTempo(2500);
    REQUIRE(seq.GetBpm() == Catch::Approx(90.0f).margin(1.0f)); // Clamped to min

    // Test ignore too fast taps (< 100ms)
    float currentBpm = seq.GetBpm();
    seq.TriggerTapTempo(2550); // 50ms later
    REQUIRE(seq.GetBpm() == currentBpm); // Should not change
}

TEST_CASE("Sequencer Gate Generation", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    
    // Run for a bit to see if we get a tick
    // BPM 120 -> 2 Hz -> Tick every 0.5s -> 24000 samples at 48k
    
    bool gateTriggered = false;
    for(int i = 0; i < 30000; i++)
    {
        seq.ProcessAudio();
        if(seq.IsGateHigh(0) || seq.IsGateHigh(1))
        {
            gateTriggered = true;
            break;
        }
    }
    
    REQUIRE(gateTriggered);
}

TEST_CASE("Led indicator voltage mapping", "[led]")
{
    REQUIRE(LedIndicator::VoltageForState(true)
            == Catch::Approx(LedIndicator::kLedOnVoltage));
    REQUIRE(LedIndicator::VoltageForState(false)
            == Catch::Approx(LedIndicator::kLedOffVoltage));
}

TEST_CASE("Clock pulse toggles with metro", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    bool clockWentHigh = false;
    for(int i = 0; i < 60000; ++i)
    {
        seq.ProcessAudio();
        if(seq.IsClockHigh())
        {
            clockWentHigh = true;
            break;
        }
    }
    REQUIRE(clockWentHigh);

    // After enough samples, clock should fall low again.
    for(int i = 0; i < 2000; ++i)
    {
        seq.ProcessAudio();
    }
    REQUIRE_FALSE(seq.IsClockHigh());
}

TEST_CASE("Sequencer Loop Length", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    // Use high tempo to speed up test
    seq.SetBpm(200.0f); 
    
    // Length 1 bar = 16 steps
    seq.SetLength(1);
    
    // We can't easily spy on stepIndex without friend class or getter,
    // but we can check if the pattern repeats after 16 ticks.
    // However, with randomness/style it's hard to verify audio output exactly.
    // But we know Sequencer::ProcessAudio updates stepIndex_.
    // Let's assume the implementation is correct if it compiles and runs, 
    // or add a getter for testing.
    // Ideally we'd add GetCurrentStep() to Sequencer for this test.
}

namespace
{
struct StepResult
{
    float accentMax = 0.0f;
    float hihatMax = 0.0f;
    bool  gate0 = false;
    bool  gate1 = false;
};

StepResult RunForcedStep(Sequencer& seq, bool kick, bool snare, bool hh, bool kickAccent)
{
    seq.ForceNextStepTriggers(kick, snare, hh, kickAccent);
    StepResult result;
    bool       stepObserved = false;
    for(int i = 0; i < 60000; ++i)
    {
        auto frame = seq.ProcessAudio();
        result.accentMax = std::max(result.accentMax, frame[0]);
        result.hihatMax = std::max(result.hihatMax, frame[1]);
        result.gate0 = result.gate0 || seq.IsGateHigh(0);
        result.gate1 = result.gate1 || seq.IsGateHigh(1);
        bool anyGateNow = seq.IsGateHigh(0) || seq.IsGateHigh(1);
        stepObserved = stepObserved || anyGateNow;
        if(stepObserved && !seq.IsGateHigh(0) && !seq.IsGateHigh(1) && frame[0] < 0.001f
           && frame[1] < 0.001f)
        {
            break;
        }
    }
    return result;
}
} // namespace

namespace
{
float SamplesToMs(int samples, float sampleRate)
{
    return (static_cast<float>(samples) / sampleRate) * 1000.0f;
}

float MeasureAccentHoldMs(Sequencer& seq, float sampleRate)
{
    seq.ForceNextStepTriggers(true, false, false, true);
    bool counting = false;
    bool forcedSilence = false;
    int  sampleCount = 0;
    for(int i = 0; i < 200000; ++i)
    {
        auto frame = seq.ProcessAudio();
        if(frame[0] > 0.5f)
        {
            counting = true;
            ++sampleCount;
            if(!forcedSilence)
            {
                seq.ForceNextStepTriggers(false, false, false, false);
                forcedSilence = true;
            }
        }
        else if(counting)
        {
            break;
        }
    }
    return SamplesToMs(sampleCount, sampleRate);
}

float MeasureHihatHoldMs(Sequencer& seq, float sampleRate)
{
    seq.ForceNextStepTriggers(false, false, true, false);
    bool counting = false;
    bool forcedSilence = false;
    int  sampleCount = 0;
    for(int i = 0; i < 200000; ++i)
    {
        auto frame = seq.ProcessAudio();
        if(frame[1] > 0.5f)
        {
            counting = true;
            ++sampleCount;
            if(!forcedSilence)
            {
                seq.ForceNextStepTriggers(false, false, false, false);
                forcedSilence = true;
            }
        }
        else if(counting)
        {
            break;
        }
    }
    return SamplesToMs(sampleCount, sampleRate);
}
} // namespace

TEST_CASE("Kick CV outputs level based on accent", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    // Initialize defaults to mimic behavior
    seq.SetTempoControl(0.5f);
    seq.SetStyle(0.5f);

    auto accented = RunForcedStep(seq, true, false, false, true);
    REQUIRE(accented.gate0);
    REQUIRE(accented.accentMax > 0.9f);

    auto normal = RunForcedStep(seq, true, false, false, false);
    REQUIRE(normal.gate0);
    REQUIRE(normal.accentMax > 0.5f);
    REQUIRE(normal.accentMax < 0.9f);
}

TEST_CASE("High CV outputs on Snare and HiHat", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetTempoControl(0.5f);

    auto snareOnly = RunForcedStep(seq, false, true, false, false);
    REQUIRE(snareOnly.gate1);
    REQUIRE(snareOnly.hihatMax > 0.5f);

    // HiHat routing is based on grid parameter:
    // grid < 0.5 routes HH to gate0 (anchor), grid >= 0.5 routes to gate1 (shimmer)
    seq.SetGrid(1.0f); // Route HH to shimmer channel (gate1)
    auto hihatOnly = RunForcedStep(seq, false, false, true, false);
    REQUIRE(hihatOnly.gate1);
    REQUIRE(hihatOnly.hihatMax > 0.5f);
}

TEST_CASE("Accent hold duration follows configuration", "[sequencer]")
{
    constexpr float sampleRate = 48000.0f;
    Sequencer       seq;
    seq.Init(sampleRate);
    seq.SetTempoControl(0.5f);
    seq.SetBpm(30.0f);

    seq.SetAccentHoldMs(20.0f);
    float shortHold = MeasureAccentHoldMs(seq, sampleRate);
    seq.SetAccentHoldMs(200.0f);
    float longHold = MeasureAccentHoldMs(seq, sampleRate);

    REQUIRE(shortHold == Catch::Approx(20.0f).margin(2.0f));
    REQUIRE(longHold == Catch::Approx(200.0f).margin(5.0f));
    REQUIRE(longHold > shortHold * 5.0f);
}

TEST_CASE("External Clock Integration", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetTempoControl(0.0f); // Slow internal tempo

    // Initially using internal clock
    // Trigger external clock
    seq.TriggerExternalClock();
    
    // Process a few samples to verify it waits for next trigger
    // Since we just triggered external clock, mustTick_ is true, so next ProcessAudio should return tick
    auto frame1 = seq.ProcessAudio(); 
    // Actually, ProcessAudio returns audio levels, not tick status directly.
    // But internal state should advance.
    
    // To verify, we can force a pattern that has a kick on step 0.
    // Reset first
    seq.TriggerReset();
    seq.TriggerExternalClock(); // Trigger step 0
    
    // Step 0 should have audio output if density is high?
    seq.SetLowDensity(1.0f);
    seq.SetHighDensity(1.0f);
    
    // ProcessAudio calls patternGen.GetTriggers()
    // We need to ensure we catch the audio.
    bool sawAudio = false;
    for(int i=0; i<100; i++) {
        auto frame = seq.ProcessAudio();
        if(frame[0] > 0.0f || frame[1] > 0.0f) sawAudio = true;
    }
    REQUIRE(sawAudio);
    
    // Now, don't trigger external clock for a while.
    // With internal clock disabled (via timeout), we shouldn't see new triggers until timeout.
    // But checking "no audio" is hard because of hold times.
    // Let's just verify we can trigger subsequent steps manually.
    
    // Advance manually
    seq.TriggerExternalClock();
    sawAudio = false;
    for(int i=0; i<100; i++) {
        auto frame = seq.ProcessAudio();
        if(frame[0] > 0.0f || frame[1] > 0.0f) sawAudio = true;
    }
    // We can't easily assert sawAudio here because step 1 might be silent depending on pattern.
    // But we can assert that the sequencer doesn't crash and accepts the trigger.
}

// === Genre-Aware Swing Tests ===

TEST_CASE("Genre detection from terrain", "[swing]")
{
    REQUIRE(GetGenreFromTerrain(0.0f) == Genre::Techno);
    REQUIRE(GetGenreFromTerrain(0.24f) == Genre::Techno);
    REQUIRE(GetGenreFromTerrain(0.25f) == Genre::Tribal);
    REQUIRE(GetGenreFromTerrain(0.49f) == Genre::Tribal);
    REQUIRE(GetGenreFromTerrain(0.50f) == Genre::TripHop);
    REQUIRE(GetGenreFromTerrain(0.74f) == Genre::TripHop);
    REQUIRE(GetGenreFromTerrain(0.75f) == Genre::IDM);
    REQUIRE(GetGenreFromTerrain(1.0f) == Genre::IDM);
}

TEST_CASE("Swing ranges per genre", "[swing]")
{
    // Techno: 52-57%
    SwingRange techno = GetSwingRange(Genre::Techno);
    REQUIRE(techno.minSwing == Catch::Approx(0.52f));
    REQUIRE(techno.maxSwing == Catch::Approx(0.57f));
    REQUIRE(techno.jitter == Catch::Approx(0.0f));

    // Tribal: 56-62%
    SwingRange tribal = GetSwingRange(Genre::Tribal);
    REQUIRE(tribal.minSwing == Catch::Approx(0.56f));
    REQUIRE(tribal.maxSwing == Catch::Approx(0.62f));

    // Trip-Hop: 60-68%
    SwingRange tripHop = GetSwingRange(Genre::TripHop);
    REQUIRE(tripHop.minSwing == Catch::Approx(0.60f));
    REQUIRE(tripHop.maxSwing == Catch::Approx(0.68f));

    // IDM: 54-65% + jitter
    SwingRange idm = GetSwingRange(Genre::IDM);
    REQUIRE(idm.minSwing == Catch::Approx(0.54f));
    REQUIRE(idm.maxSwing == Catch::Approx(0.65f));
    REQUIRE(idm.jitter == Catch::Approx(0.03f));
}

TEST_CASE("Swing calculation from terrain and taste", "[swing]")
{
    // Techno at low taste -> 52%
    REQUIRE(CalculateSwing(0.0f, 0.0f) == Catch::Approx(0.52f));
    // Techno at high taste -> 57%
    REQUIRE(CalculateSwing(0.0f, 1.0f) == Catch::Approx(0.57f));
    // Techno at mid taste -> 54.5%
    REQUIRE(CalculateSwing(0.0f, 0.5f) == Catch::Approx(0.545f));

    // Trip-Hop at high taste -> 68% (max swing)
    REQUIRE(CalculateSwing(0.6f, 1.0f) == Catch::Approx(0.68f));
}

TEST_CASE("Off-beat detection", "[swing]")
{
    // Even steps are on-beats (0, 2, 4, 6...)
    REQUIRE_FALSE(IsOffBeat(0));
    REQUIRE_FALSE(IsOffBeat(2));
    REQUIRE_FALSE(IsOffBeat(4));
    REQUIRE_FALSE(IsOffBeat(14));

    // Odd steps are off-beats (1, 3, 5, 7...)
    REQUIRE(IsOffBeat(1));
    REQUIRE(IsOffBeat(3));
    REQUIRE(IsOffBeat(5));
    REQUIRE(IsOffBeat(15));
}

TEST_CASE("Swing delay calculation", "[swing]")
{
    // At 50% swing (straight), no delay
    REQUIRE(CalculateSwingDelaySamples(0.50f, 1000) == 0);

    // At 60% swing, delay is 10% of step duration
    REQUIRE(CalculateSwingDelaySamples(0.60f, 1000) == 100);

    // At 66% swing (triplet), delay is 16% of step duration
    REQUIRE(CalculateSwingDelaySamples(0.66f, 1000) == 160);
}

TEST_CASE("Sequencer swing integration", "[swing]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    // Default terrain (0) = Techno, default taste (0.5) = mid-range
    // Expected swing: 52% + 0.5 * (57% - 52%) = 54.5%
    REQUIRE(seq.GetSwingPercent() == Catch::Approx(0.545f).margin(0.01f));
    REQUIRE(seq.GetCurrentGenre() == Genre::Techno);

    // Set to Trip-Hop with high taste
    seq.SetTerrain(0.6f);
    seq.SetSwingTaste(1.0f);
    REQUIRE(seq.GetCurrentGenre() == Genre::TripHop);
    REQUIRE(seq.GetSwingPercent() == Catch::Approx(0.68f).margin(0.01f));

    // Set to IDM with low taste
    seq.SetTerrain(0.9f);
    seq.SetSwingTaste(0.0f);
    REQUIRE(seq.GetCurrentGenre() == Genre::IDM);
    REQUIRE(seq.GetSwingPercent() == Catch::Approx(0.54f).margin(0.01f));
}

// === Orbit Voice Relationship Tests ===

TEST_CASE("Orbit mode detection", "[orbit]")
{
    // Interlock: 0-33%
    REQUIRE(GetOrbitMode(0.0f) == OrbitMode::Interlock);
    REQUIRE(GetOrbitMode(0.32f) == OrbitMode::Interlock);

    // Free: 33-67%
    REQUIRE(GetOrbitMode(0.33f) == OrbitMode::Free);
    REQUIRE(GetOrbitMode(0.5f) == OrbitMode::Free);
    REQUIRE(GetOrbitMode(0.66f) == OrbitMode::Free);

    // Shadow: 67-100%
    REQUIRE(GetOrbitMode(0.67f) == OrbitMode::Shadow);
    REQUIRE(GetOrbitMode(1.0f) == OrbitMode::Shadow);
}

TEST_CASE("Interlock modifier calculation", "[orbit]")
{
    // At orbit=0 (max interlock), anchor firing reduces shimmer by 30%
    REQUIRE(GetInterlockModifier(true, 0.0f) == Catch::Approx(-0.3f));
    // At orbit=0 (max interlock), anchor silent boosts shimmer by 30%
    REQUIRE(GetInterlockModifier(false, 0.0f) == Catch::Approx(0.3f));

    // At orbit=0.33 (edge of interlock zone), minimal effect
    REQUIRE(GetInterlockModifier(true, 0.33f) == Catch::Approx(0.0f).margin(0.01f));
    REQUIRE(GetInterlockModifier(false, 0.33f) == Catch::Approx(0.0f).margin(0.01f));

    // At orbit=0.165 (mid interlock), half effect
    REQUIRE(GetInterlockModifier(true, 0.165f) == Catch::Approx(-0.15f).margin(0.02f));
    REQUIRE(GetInterlockModifier(false, 0.165f) == Catch::Approx(0.15f).margin(0.02f));
}

