#include <algorithm>
#include <catch2/catch_all.hpp>
#include "../src/Engine/Sequencer.h"
#include "../src/Engine/LedIndicator.h"

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
    const float knobY = 0.5f;

    // Knob at 0 -> Min Tempo (30)
    seq.ProcessControl(0.0f, knobX, knobY, 0.0f, false, 0);
    REQUIRE(seq.GetBpm() == Catch::Approx(30.0f));

    // Knob at 1 -> Max Tempo (200)
    seq.ProcessControl(1.0f, knobX, knobY, 0.0f, false, 0);
    REQUIRE(seq.GetBpm() == Catch::Approx(200.0f));

    // Knob at 0.5 -> Mid Tempo (115)
    // 30 + 0.5 * (200 - 30) = 30 + 85 = 115
    seq.ProcessControl(0.5f, knobX, knobY, 0.0f, false, 0);
    REQUIRE(seq.GetBpm() == Catch::Approx(115.0f));
}

TEST_CASE("Sequencer Tap Tempo", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    const float knobX = 0.5f;
    const float knobY = 0.5f;

    // First tap sets the baseline
    seq.ProcessControl(0.5f, knobX, knobY, 0.0f, true, 1000);
    
    // Second tap 500ms later (120 BPM)
    // 60000 / 500 = 120
    seq.ProcessControl(0.5f, knobX, knobY, 0.0f, true, 1500);
    REQUIRE(seq.GetBpm() == Catch::Approx(120.0f).margin(1.0f));

    // Third tap 1000ms later (60 BPM)
    seq.ProcessControl(0.5f, knobX, knobY, 0.0f, true, 2500);
    REQUIRE(seq.GetBpm() == Catch::Approx(60.0f).margin(1.0f));
    
    // Test ignore too fast taps (< 100ms)
    float currentBpm = seq.GetBpm();
    seq.ProcessControl(0.5f, knobX, knobY, 0.0f, true, 2550); // 50ms later
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

TEST_CASE("Kick accent CV occurs only on accented kicks", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.ProcessControl(0.5f, 0.5f, 0.5f, 0.0f, false, 0);

    auto accented = RunForcedStep(seq, true, false, false, true);
    REQUIRE(accented.gate0);
    REQUIRE(accented.accentMax > 0.5f);

    auto normal = RunForcedStep(seq, true, false, false, false);
    REQUIRE(normal.gate0);
    REQUIRE(normal.accentMax < 0.01f);
}

TEST_CASE("Hi-hat CV remains low on snares and high on hats", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.ProcessControl(0.5f, 0.5f, 0.5f, 0.0f, false, 0);

    auto snareOnly = RunForcedStep(seq, false, true, false, false);
    REQUIRE(snareOnly.gate1);
    REQUIRE(snareOnly.hihatMax < 0.01f);

    auto hihatOnly = RunForcedStep(seq, false, false, true, false);
    REQUIRE(hihatOnly.gate1);
    REQUIRE(hihatOnly.hihatMax > 0.5f);
}

TEST_CASE("Accent hold duration follows configuration", "[sequencer]")
{
    constexpr float sampleRate = 48000.0f;
    Sequencer       seq;
    seq.Init(sampleRate);
    seq.ProcessControl(0.5f, 0.5f, 0.5f, 0.0f, false, 0);
    seq.SetBpm(30.0f);

    seq.SetAccentHoldMs(20.0f);
    float shortHold = MeasureAccentHoldMs(seq, sampleRate);
    seq.SetAccentHoldMs(200.0f);
    float longHold = MeasureAccentHoldMs(seq, sampleRate);

    REQUIRE(shortHold == Catch::Approx(20.0f).margin(2.0f));
    REQUIRE(longHold == Catch::Approx(200.0f).margin(5.0f));
    REQUIRE(longHold > shortHold * 5.0f);
}

TEST_CASE("Hi-hat hold duration follows configuration", "[sequencer]")
{
    constexpr float sampleRate = 48000.0f;
    Sequencer       seq;
    seq.Init(sampleRate);
    seq.ProcessControl(0.5f, 0.5f, 0.5f, 0.0f, false, 0);
    seq.SetBpm(30.0f);

    seq.SetHihatHoldMs(15.0f);
    float shortHold = MeasureHihatHoldMs(seq, sampleRate);
    seq.SetHihatHoldMs(300.0f);
    float longHold = MeasureHihatHoldMs(seq, sampleRate);

    REQUIRE(shortHold == Catch::Approx(15.0f).margin(2.0f));
    REQUIRE(longHold == Catch::Approx(300.0f).margin(8.0f));
    REQUIRE(longHold > shortHold * 8.0f);
}

