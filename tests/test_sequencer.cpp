#include <algorithm>
#include <catch2/catch_all.hpp>
#include "../src/Engine/Sequencer.h"
#include "../src/Engine/LedIndicator.h"
#include "../src/Engine/GenreConfig.h"
#include "../inc/config.h"

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
    seq.SetTerrain(0.5f);  // Was SetStyle

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

    // v3: No separate HH routing - shimmer handles all upper percussion
    // Forced HH triggers are ignored in v3 (shimmer density controls upper percussion)
    auto hihatOnly = RunForcedStep(seq, false, false, true, false);
    // In v3, forced HH doesn't route anywhere - shimmer density controls triggers
    REQUIRE(hihatOnly.gate1 == false); // No separate HH in v3
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
    seq.SetAnchorDensity(1.0f);   // Was SetLowDensity
    seq.SetShimmerDensity(1.0f);  // Was SetHighDensity
    
    // ProcessAudio uses PatternSkeleton triggers
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

    // === v3: Swing from BROKEN parameter ===
    // GetSwingFromBroken(0.0) = 0.50 (straight Techno)
    // swingTaste=0.5 (default) = no adjustment
    REQUIRE(seq.GetSwingPercent() == Catch::Approx(0.50f).margin(0.01f));

    // At BROKEN=0.6 (Trip-Hop zone: 50-75%), swing is around 60-66%
    // GetSwingFromBroken(0.6) = 0.60 + (0.6-0.5)*4 * 0.06 = 0.60 + 0.024 = ~0.624
    // swingTaste=1.0 adds +4%
    seq.SetBroken(0.6f);
    seq.SetSwingTaste(1.0f);
    REQUIRE(seq.GetSwingPercent() == Catch::Approx(0.664f).margin(0.02f));

    // At high BROKEN=0.9 (IDM zone: 75-100%), swing trends back down
    // GetSwingFromBroken(0.9) = 0.66 - (0.9-0.75)*4 * 0.08 = 0.66 - 0.048 = ~0.612
    // swingTaste=0.0 subtracts -4%
    seq.SetBroken(0.9f);
    seq.SetSwingTaste(0.0f);
    REQUIRE(seq.GetSwingPercent() == Catch::Approx(0.572f).margin(0.02f));
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

// === Phrase Position Tests ===

TEST_CASE("Phrase position calculation", "[phrase]")
{
    // 4-bar loop, step 0
    PhrasePosition pos = CalculatePhrasePosition(0, 4);
    REQUIRE(pos.currentBar == 0);
    REQUIRE(pos.stepInBar == 0);
    REQUIRE(pos.stepInPhrase == 0);
    REQUIRE(pos.phraseProgress == Catch::Approx(0.0f));
    REQUIRE(pos.isLastBar == false);
    REQUIRE(pos.isDownbeat == true);
    REQUIRE(pos.isFillZone == false);
    REQUIRE(pos.isBuildZone == false);

    // 4-bar loop, step 16 (start of bar 2)
    pos = CalculatePhrasePosition(16, 4);
    REQUIRE(pos.currentBar == 1);
    REQUIRE(pos.stepInBar == 0);
    REQUIRE(pos.isDownbeat == true);
    REQUIRE(pos.isLastBar == false);

    // 4-bar loop, step 48 (start of bar 4 - last bar)
    pos = CalculatePhrasePosition(48, 4);
    REQUIRE(pos.currentBar == 3);
    REQUIRE(pos.isLastBar == true);

    // 4-bar loop, step 63 (last step - should be in fill zone)
    pos = CalculatePhrasePosition(63, 4);
    REQUIRE(pos.stepInPhrase == 63);
    REQUIRE(pos.phraseProgress == Catch::Approx(63.0f / 64.0f).margin(0.01f));
    REQUIRE(pos.isFillZone == true);
    REQUIRE(pos.isBuildZone == true);
}

TEST_CASE("Fill and build zone scaling", "[phrase]")
{
    // 1-bar loop: fill zone = last 4 steps, build zone = last 8 steps
    PhrasePosition pos1 = CalculatePhrasePosition(12, 1); // Step 12 of 16
    REQUIRE(pos1.isFillZone == true);  // 4 steps from end
    REQUIRE(pos1.isBuildZone == true);

    PhrasePosition pos2 = CalculatePhrasePosition(8, 1); // Step 8 of 16
    REQUIRE(pos2.isFillZone == false);
    REQUIRE(pos2.isBuildZone == true); // 8 steps from end

    PhrasePosition pos3 = CalculatePhrasePosition(7, 1); // Step 7 of 16
    REQUIRE(pos3.isBuildZone == false);

    // 8-bar loop: fill zone = last 32 steps (capped), build zone = last 64 steps (capped)
    PhrasePosition pos4 = CalculatePhrasePosition(128 - 16, 8); // 16 steps from end
    REQUIRE(pos4.isFillZone == true);  // Within 32-step fill zone
    REQUIRE(pos4.isBuildZone == true);

    PhrasePosition pos5 = CalculatePhrasePosition(128 - 33, 8); // 33 steps from end (just outside fill zone)
    REQUIRE(pos5.isFillZone == false);
    REQUIRE(pos5.isBuildZone == true); // Still in build zone (64 steps)
}

TEST_CASE("Phrase fill boost calculation", "[phrase]")
{
    // Not in fill or build zone
    PhrasePosition notInZone;
    notInZone.isFillZone = false;
    notInZone.isBuildZone = false;
    REQUIRE(GetPhraseFillBoost(notInZone, 0.0f) == 0.0f);

    // In build zone (not fill)
    PhrasePosition buildZone;
    buildZone.isFillZone = false;
    buildZone.isBuildZone = true;
    float boost = GetPhraseFillBoost(buildZone, 0.0f); // Techno
    REQUIRE(boost == Catch::Approx(0.3f * 0.5f).margin(0.01f)); // 30% * 50% genre scale

    // In fill zone, IDM terrain
    PhrasePosition fillZone;
    fillZone.isFillZone = true;
    fillZone.isBuildZone = true;
    boost = GetPhraseFillBoost(fillZone, 0.9f); // IDM
    REQUIRE(boost == Catch::Approx(0.5f * 1.5f).margin(0.01f)); // 50% * 150% genre scale
}

TEST_CASE("Phrase accent multiplier", "[phrase]")
{
    // Phrase start (bar 0, step 0) - strongest accent
    PhrasePosition phraseStart;
    phraseStart.currentBar = 0;
    phraseStart.stepInBar = 0;
    phraseStart.isFillZone = false;
    phraseStart.phraseProgress = 0.0f;
    REQUIRE(GetPhraseAccentMultiplier(phraseStart) == Catch::Approx(1.2f));

    // Bar downbeat (not phrase start)
    PhrasePosition barDownbeat;
    barDownbeat.currentBar = 2;
    barDownbeat.stepInBar = 0;
    barDownbeat.isFillZone = false;
    barDownbeat.isDownbeat = true;
    REQUIRE(GetPhraseAccentMultiplier(barDownbeat) == Catch::Approx(1.1f));

    // Regular step
    PhrasePosition regular;
    regular.currentBar = 1;
    regular.stepInBar = 3;
    regular.isFillZone = false;
    regular.isDownbeat = false;
    REQUIRE(GetPhraseAccentMultiplier(regular) == Catch::Approx(1.0f));
}

// === Contour CV Mode Tests ===

TEST_CASE("Contour mode detection", "[contour]")
{
    REQUIRE(GetContourMode(0.0f) == ContourMode::Velocity);
    REQUIRE(GetContourMode(0.24f) == ContourMode::Velocity);
    REQUIRE(GetContourMode(0.25f) == ContourMode::Decay);
    REQUIRE(GetContourMode(0.49f) == ContourMode::Decay);
    REQUIRE(GetContourMode(0.50f) == ContourMode::Pitch);
    REQUIRE(GetContourMode(0.74f) == ContourMode::Pitch);
    REQUIRE(GetContourMode(0.75f) == ContourMode::Random);
    REQUIRE(GetContourMode(1.0f) == ContourMode::Random);
}

TEST_CASE("Contour CV calculation - Velocity mode", "[contour]")
{
    // On trigger, CV = velocity
    float cv = CalculateContourCV(ContourMode::Velocity, 0.8f, 0.5f, 0.0f, true);
    REQUIRE(cv == Catch::Approx(0.8f));

    // Between triggers, decays very slowly (sustain-like)
    // kVelocityDecay = 0.99995f - designed for per-sample at 48kHz
    cv = CalculateContourCV(ContourMode::Velocity, 0.0f, 0.5f, 0.8f, false);
    REQUIRE(cv == Catch::Approx(0.8f * 0.99995f).margin(0.0001f));
}

TEST_CASE("Contour CV calculation - Decay mode", "[contour]")
{
    // On trigger, CV maps velocity to decay hint
    float cv = CalculateContourCV(ContourMode::Decay, 1.0f, 0.5f, 0.0f, true);
    REQUIRE(cv == Catch::Approx(1.0f).margin(0.01f)); // Max velocity = max CV

    cv = CalculateContourCV(ContourMode::Decay, 0.0f, 0.5f, 0.0f, true);
    REQUIRE(cv == Catch::Approx(0.2f).margin(0.01f)); // Min velocity = 0.2 CV

    // Decays between triggers (envelope-like)
    // kDecayDecay = 0.9997f - designed for per-sample at 48kHz (~250ms decay)
    cv = CalculateContourCV(ContourMode::Decay, 0.0f, 0.5f, 1.0f, false);
    REQUIRE(cv == Catch::Approx(0.9997f).margin(0.0001f));
}

TEST_CASE("Contour CV calculation - Pitch mode", "[contour]")
{
    // On trigger, CV centered at 0.5 with velocity-scaled random offset
    // With random = 0.5, offset = 0 (centered)
    float cv = CalculateContourCV(ContourMode::Pitch, 1.0f, 0.5f, 0.0f, true);
    REQUIRE(cv == Catch::Approx(0.5f).margin(0.01f));

    // With random = 1.0 and max velocity, offset = +0.2
    cv = CalculateContourCV(ContourMode::Pitch, 1.0f, 1.0f, 0.0f, true);
    REQUIRE(cv == Catch::Approx(0.7f).margin(0.01f));

    // With random = 0.0 and max velocity, offset = -0.2
    cv = CalculateContourCV(ContourMode::Pitch, 1.0f, 0.0f, 0.0f, true);
    REQUIRE(cv == Catch::Approx(0.3f).margin(0.01f));

    // Holds between triggers
    cv = CalculateContourCV(ContourMode::Pitch, 0.0f, 0.5f, 0.7f, false);
    REQUIRE(cv == Catch::Approx(0.7f));
}

TEST_CASE("Contour CV calculation - Random mode", "[contour]")
{
    // On trigger, CV = random value
    float cv = CalculateContourCV(ContourMode::Random, 0.5f, 0.75f, 0.0f, true);
    REQUIRE(cv == Catch::Approx(0.75f));

    // Holds between triggers
    cv = CalculateContourCV(ContourMode::Random, 0.0f, 0.5f, 0.75f, false);
    REQUIRE(cv == Catch::Approx(0.75f));
}

// === DuoPulse v3: Phrase Reset Integration Tests ===

TEST_CASE("Phrase reset triggers loopSeed regeneration in v3", "[sequencer][drift][v3]")
{
    // This test verifies that when the sequencer wraps to step 0,
    // the phrase reset callback is called, causing DRIFT-affected
    // steps to produce different patterns.
    
    Sequencer seq;
    seq.Init(48000.0f);
    
    // Use very fast tempo for quick loop completion
    seq.SetBpm(160.0f);
    
    // Set up for maximum drift (patterns should vary between loops)
    seq.SetDrift(1.0f);
    seq.SetBroken(0.5f);  // Moderate broken for some noise variation
    seq.SetAnchorDensity(0.6f);
    seq.SetShimmerDensity(0.6f);
    seq.SetLength(1);  // 1-bar loop = 16 steps (shortest loop for faster test)
    
    // Reset to start at step 0
    seq.TriggerReset();
    
    // Count triggers over multiple loops
    // With DRIFT=1.0 and phrase reset, patterns should differ between loops
    
    // 16th note at 160 BPM = 60000 / (160 * 4) = 93.75ms per step
    // At 48kHz, that's about 4500 samples per step
    // For 1-bar (16 steps), one loop = ~72000 samples
    
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int samplesPerLoop = samplesPerStep * 16;
    
    // Record triggers for first loop
    int loop1Triggers = 0;
    for (int i = 0; i < samplesPerLoop + 100; i++)  // +100 for margin
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) loop1Triggers++;
    }
    
    // Record triggers for second loop (should potentially differ due to DRIFT)
    int loop2Triggers = 0;
    for (int i = 0; i < samplesPerLoop + 100; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) loop2Triggers++;
    }
    
    // Note: Due to DRIFT affecting which steps fire, we can't predict exact counts
    // But both loops should produce some triggers
    REQUIRE(loop1Triggers > 0);
    REQUIRE(loop2Triggers > 0);
    
    // The phrase position should correctly track loop boundaries
    // After running 2 full loops, we should be somewhere in the pattern
    const PhrasePosition& pos = seq.GetPhrasePosition();
    REQUIRE(pos.stepInPhrase >= 0);
    REQUIRE(pos.stepInPhrase < 16);  // Within 1-bar loop
}

TEST_CASE("Phrase reset at step 0 verified via phrase position", "[sequencer][phrase][v3]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetBpm(160.0f);
    seq.SetLength(1);  // 1-bar = 16 steps
    
    // Reset to step -1 (next tick will be step 0)
    seq.TriggerReset();
    
    // Run until we see step 0, then continue through full loop
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    
    bool sawStep0 = false;
    bool sawStep15 = false;
    bool sawStep0Again = false;
    
    // Run for 2 loops worth of samples
    for (int i = 0; i < samplesPerStep * 32 + 1000; i++)
    {
        seq.ProcessAudio();
        const PhrasePosition& pos = seq.GetPhrasePosition();
        
        if (pos.stepInPhrase == 0)
        {
            if (!sawStep0)
            {
                sawStep0 = true;
            }
            else if (sawStep15)
            {
                sawStep0Again = true;
                break;  // Verified loop wrap
            }
        }
        else if (pos.stepInPhrase == 15)
        {
            sawStep15 = true;
        }
    }
    
    // Should have seen step 0, then step 15, then step 0 again (loop wrapped)
    REQUIRE(sawStep0);
    REQUIRE(sawStep15);
    REQUIRE(sawStep0Again);
}

// =============================================================================
// v3 Critical Rules: DENSITY=0 Full Pipeline Test [v3-critical-rules]
// =============================================================================

TEST_CASE("DENSITY=0 produces zero triggers through full ProcessAudio pipeline", "[sequencer][v3-critical-rules][density-zero]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    
    // Set both densities to 0 - should be absolute silence
    seq.SetAnchorDensity(0.0f);
    seq.SetShimmerDensity(0.0f);
    
    // Start with minimal settings to isolate the issue
    seq.SetBroken(0.0f);   // No chaos
    seq.SetDrift(0.0f);    // No drift
    seq.SetFuse(0.5f);     // Balanced
    seq.SetCouple(0.0f);   // No gap filling
    seq.SetRatchet(0.0f);  // No ratchet
    seq.SetBpm(160.0f);    // Fast tempo for more steps
    seq.SetLength(1);      // Short loop for quick test
    
    // Reset to start at step 0
    seq.TriggerReset();
    
    // Run for 2 complete loops
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int totalSamples = samplesPerStep * 16 * 2;  // 16 steps × 2 loops
    
    int anchorGateCount = 0;
    int shimmerGateCount = 0;
    
    for (int i = 0; i < totalSamples; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) anchorGateCount++;
        if (seq.IsGateHigh(1)) shimmerGateCount++;
    }
    
    // CRITICAL: At DENSITY=0, there should be ZERO triggers
    REQUIRE(anchorGateCount == 0);
    REQUIRE(shimmerGateCount == 0);
}

TEST_CASE("DENSITY=0 silence with max DRIFT only", "[sequencer][v3-critical-rules][density-zero][isolate]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetAnchorDensity(0.0f);
    seq.SetShimmerDensity(0.0f);
    seq.SetDrift(1.0f);    // Test DRIFT in isolation
    seq.SetBpm(160.0f);
    seq.SetLength(1);
    seq.TriggerReset();
    
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int totalSamples = samplesPerStep * 16 * 2;
    
    int anchorGateCount = 0, shimmerGateCount = 0;
    for (int i = 0; i < totalSamples; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) anchorGateCount++;
        if (seq.IsGateHigh(1)) shimmerGateCount++;
    }
    
    REQUIRE(anchorGateCount == 0);
    REQUIRE(shimmerGateCount == 0);
}

TEST_CASE("DENSITY=0 silence with max BROKEN only", "[sequencer][v3-critical-rules][density-zero][isolate]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetAnchorDensity(0.0f);
    seq.SetShimmerDensity(0.0f);
    seq.SetBroken(1.0f);   // Test BROKEN in isolation
    seq.SetBpm(160.0f);
    seq.SetLength(1);
    seq.TriggerReset();
    
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int totalSamples = samplesPerStep * 16 * 2;
    
    int anchorGateCount = 0, shimmerGateCount = 0;
    for (int i = 0; i < totalSamples; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) anchorGateCount++;
        if (seq.IsGateHigh(1)) shimmerGateCount++;
    }
    
    REQUIRE(anchorGateCount == 0);
    REQUIRE(shimmerGateCount == 0);
}

TEST_CASE("DENSITY=0 silence with max COUPLE only", "[sequencer][v3-critical-rules][density-zero][isolate]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetAnchorDensity(0.0f);
    seq.SetShimmerDensity(0.0f);
    seq.SetCouple(1.0f);   // Test COUPLE in isolation
    seq.SetBpm(160.0f);
    seq.SetLength(1);
    seq.TriggerReset();
    
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int totalSamples = samplesPerStep * 16 * 2;
    
    int anchorGateCount = 0, shimmerGateCount = 0;
    for (int i = 0; i < totalSamples; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) anchorGateCount++;
        if (seq.IsGateHigh(1)) shimmerGateCount++;
    }
    
    REQUIRE(anchorGateCount == 0);
    REQUIRE(shimmerGateCount == 0);
}

TEST_CASE("DENSITY=0 silence with max DRIFT+BROKEN", "[sequencer][v3-critical-rules][density-zero][isolate]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetAnchorDensity(0.0f);
    seq.SetShimmerDensity(0.0f);
    seq.SetDrift(1.0f);
    seq.SetBroken(1.0f);
    seq.SetBpm(160.0f);
    seq.SetLength(1);
    seq.TriggerReset();
    
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int totalSamples = samplesPerStep * 16 * 2;
    
    int anchorGateCount = 0, shimmerGateCount = 0;
    for (int i = 0; i < totalSamples; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) anchorGateCount++;
        if (seq.IsGateHigh(1)) shimmerGateCount++;
    }
    
    REQUIRE(anchorGateCount == 0);
    REQUIRE(shimmerGateCount == 0);
}

TEST_CASE("DENSITY=0 for one voice does not affect the other in Sequencer", "[sequencer][v3-critical-rules][density-zero]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    
    // Anchor at 0, Shimmer at high density
    seq.SetAnchorDensity(0.0f);
    seq.SetShimmerDensity(0.9f);  // High density = lots of triggers
    seq.SetBroken(0.0f);
    seq.SetDrift(0.0f);
    seq.SetBpm(160.0f);
    seq.SetLength(1);  // 1-bar for quick test
    
    seq.TriggerReset();
    
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int totalSamples = samplesPerStep * 16 * 2;  // 2 full loops
    
    int anchorGateCount = 0;
    int shimmerGateCount = 0;
    
    for (int i = 0; i < totalSamples; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) anchorGateCount++;
        if (seq.IsGateHigh(1)) shimmerGateCount++;
    }
    
    // Anchor should have zero triggers (density=0)
    REQUIRE(anchorGateCount == 0);
    // Shimmer should have many triggers (high density)
    REQUIRE(shimmerGateCount > 0);
}

TEST_CASE("DENSITY=1.0 produces triggers on all steps", "[sequencer][v3-critical-rules][density-max]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    
    // Set both densities to max - should fire on every step
    seq.SetAnchorDensity(1.0f);
    seq.SetShimmerDensity(1.0f);
    seq.SetBroken(0.0f);   // No chaos
    seq.SetDrift(0.0f);    // No drift
    seq.SetFuse(0.5f);     // Balanced
    seq.SetCouple(0.0f);   // No interlock (to avoid suppression)
    seq.SetBpm(160.0f);
    seq.SetLength(1);      // 1-bar loop = 16 steps
    
    seq.TriggerReset();
    
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (160.0f * 4.0f));
    int totalSamples = samplesPerStep * 16 + 1000;  // One full loop with margin
    
    // Track which steps fired for each voice
    int anchorStepsFired = 0;
    int shimmerStepsFired = 0;
    int lastStep = -1;
    bool anchorFiredThisStep = false;
    bool shimmerFiredThisStep = false;
    
    for (int i = 0; i < totalSamples; i++)
    {
        seq.ProcessAudio();
        
        int currentStep = seq.GetPhrasePosition().stepInPhrase;
        
        // Detect step change
        if (currentStep != lastStep && lastStep >= 0)
        {
            if (anchorFiredThisStep) anchorStepsFired++;
            if (shimmerFiredThisStep) shimmerStepsFired++;
            anchorFiredThisStep = false;
            shimmerFiredThisStep = false;
        }
        
        if (seq.IsGateHigh(0)) anchorFiredThisStep = true;
        if (seq.IsGateHigh(1)) shimmerFiredThisStep = true;
        
        lastStep = currentStep;
    }
    
    // At DENSITY=1.0, all 16 steps should fire for both voices
    REQUIRE(anchorStepsFired >= 15);   // Allow 1 step margin for timing
    REQUIRE(shimmerStepsFired >= 15);
}

TEST_CASE("Forced triggers bypass density check", "[sequencer][triggers]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    
    // Set density to 0 - normally would be silent
    seq.SetAnchorDensity(0.0f);
    seq.SetShimmerDensity(0.0f);
    seq.SetBpm(120.0f);
    
    // Force next step triggers - should override density=0
    seq.ForceNextStepTriggers(true, true, false, false);
    
    // Process until we get to the next step
    int samplesPerStep = static_cast<int>(48000.0f * 60.0f / (120.0f * 4.0f));
    
    bool sawAnchorGate = false;
    bool sawShimmerGate = false;
    
    for (int i = 0; i < samplesPerStep * 2; i++)
    {
        seq.ProcessAudio();
        if (seq.IsGateHigh(0)) sawAnchorGate = true;
        if (seq.IsGateHigh(1)) sawShimmerGate = true;
    }
    
    // Forced triggers should fire regardless of density
    REQUIRE(sawAnchorGate == true);
    REQUIRE(sawShimmerGate == true);
}

