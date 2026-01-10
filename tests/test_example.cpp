#include <catch2/catch_all.hpp>
#include <cmath>
#include <cstddef>

#include "../src/Engine/ControlUtils.h"
#include "../src/Engine/Sequencer.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

TEST_CASE("MixControl clamps combined CV + knob inputs", "[control]")
{
    REQUIRE(MixControl(0.3f, 0.4f) == Approx(0.7f).margin(1e-5f));
    REQUIRE(MixControl(0.9f, 0.5f) == Approx(1.0f).margin(1e-5f));
    REQUIRE(MixControl(0.1f, -0.5f) == Approx(0.0f).margin(1e-5f));
}

// ChaosModulator tests removed - v3 code deleted for v4 migration

TEST_CASE("Sequencer produces gates and CV pulses", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetBpm(125.0f);  // V5 API (was SetTempoControl)

    std::size_t kickSamples = 0;
    std::size_t snareSamples = 0;
    std::size_t cvSamples = 0;

    for(int i = 0; i < 96000; ++i)
    {
        auto frame = seq.ProcessAudio();
        if(seq.IsGateHigh(0))
        {
            ++kickSamples;
        }
        if(seq.IsGateHigh(1))
        {
            ++snareSamples;
        }
        if(frame[0] > 0.01f || frame[1] > 0.01f)
        {
            ++cvSamples;
        }
    }

    REQUIRE(kickSamples > 0);
    REQUIRE(snareSamples > 0);
    REQUIRE(cvSamples > 0);
}

TEST_CASE("Kick accents stay isolated from hi-hat CV", "[sequencer]")
{
    Sequencer seq;
    seq.Init(48000.0f);
    seq.SetBpm(125.0f);  // V5 API (was SetTempoControl)

    seq.ForceNextStepTriggers(true, false, false, true);

    bool accentTriggered = false;
    bool hihatStayedLow = true;
    for(int i = 0; i < 48000; ++i)
    {
        auto frame = seq.ProcessAudio();
        if(frame[0] > 0.5f)
        {
            accentTriggered = true;
            break;
        }
        if(frame[1] > 0.01f)
        {
            hihatStayedLow = false;
        }
    }

    REQUIRE(accentTriggered);
    REQUIRE(hihatStayedLow);
}
