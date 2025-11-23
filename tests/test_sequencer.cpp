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
    seq.ProcessControl(0.0f, knobX, knobY, false, 0);
    REQUIRE(seq.GetBpm() == Catch::Approx(30.0f));

    // Knob at 1 -> Max Tempo (200)
    seq.ProcessControl(1.0f, knobX, knobY, false, 0);
    REQUIRE(seq.GetBpm() == Catch::Approx(200.0f));

    // Knob at 0.5 -> Mid Tempo (115)
    // 30 + 0.5 * (200 - 30) = 30 + 85 = 115
    seq.ProcessControl(0.5f, knobX, knobY, false, 0);
    REQUIRE(seq.GetBpm() == Catch::Approx(115.0f));
}

TEST_CASE("Sequencer Tap Tempo", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);

    const float knobX = 0.5f;
    const float knobY = 0.5f;

    // First tap sets the baseline
    seq.ProcessControl(0.5f, knobX, knobY, true, 1000);
    
    // Second tap 500ms later (120 BPM)
    // 60000 / 500 = 120
    seq.ProcessControl(0.5f, knobX, knobY, true, 1500);
    REQUIRE(seq.GetBpm() == Catch::Approx(120.0f).margin(1.0f));

    // Third tap 1000ms later (60 BPM)
    seq.ProcessControl(0.5f, knobX, knobY, true, 2500);
    REQUIRE(seq.GetBpm() == Catch::Approx(60.0f).margin(1.0f));
    
    // Test ignore too fast taps (< 100ms)
    float currentBpm = seq.GetBpm();
    seq.ProcessControl(0.5f, knobX, knobY, true, 2550); // 50ms later
    REQUIRE(seq.GetBpm() == currentBpm); // Should not change
}

TEST_CASE("Sequencer Audio Generation", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    
    // Run for a bit to see if we get a tick
    // BPM 120 -> 2 Hz -> Tick every 0.5s -> 24000 samples at 48k
    
    bool gateTriggered = false;
    for(int i = 0; i < 30000; i++)
    {
        float sample = seq.ProcessAudio();
        (void)sample;
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

