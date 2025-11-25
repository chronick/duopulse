/**
 * Phase 5: Performance vs Config Mode
 * - Performance mode: Knob/CV pairs drive Grids parameters + tempo (tap-tempo enabled)
 * - Config mode: Knob/CV pairs re-map to accent/hi-hat gate voltage and hold times
 * - Mode switch (B8) toggles Performance/Config without interrupting the sequencer
 * - LED + CV_OUT_2 stay solid while in Config mode, blink on kicks otherwise
 * - OUT_L / OUT_R use GateScaler to keep codec-driven gates within ±5 V
 */

#include <array>

#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "Engine/Sequencer.h"
#include "Engine/LedIndicator.h"
#include "Engine/ControlUtils.h"
#include "Engine/GateScaler.h"
#include "Engine/ConfigMapper.h"

using namespace daisy;
using namespace daisysp;
using namespace daisy::patch_sm;
using namespace daisysp_idm_grids;

namespace
{
DaisyPatchSM patch;
Sequencer    sequencer;
Switch       tapButton;
Switch       modeSwitch;

GateScaler accentGate;
GateScaler hihatGate;

struct PerformanceControlState
{
    float mapX = 0.5f;
    float mapY = 0.5f;
    float chaos = 0.0f;
    float tempo = 0.5f;
};

struct GateLaneSettings
{
    float targetVoltage = GateScaler::kGateVoltageLimit;
    float holdMs = 10.0f;
};

PerformanceControlState performanceState;
GateLaneSettings        accentLane;
GateLaneSettings        hihatLane;

constexpr float kDefaultGateHoldMs = 10.0f;

void UpdateAccentLaneFromControls(float voltageNorm, float holdNorm)
{
    accentLane.targetVoltage = ConfigMapper::NormalizedToVoltage(voltageNorm);
    accentLane.holdMs        = ConfigMapper::NormalizedToHoldMs(holdNorm);
    accentGate.SetTargetVoltage(accentLane.targetVoltage);
    sequencer.SetAccentHoldMs(accentLane.holdMs);
}

void UpdateHihatLaneFromControls(float voltageNorm, float holdNorm)
{
    hihatLane.targetVoltage = ConfigMapper::NormalizedToVoltage(voltageNorm);
    hihatLane.holdMs        = ConfigMapper::NormalizedToHoldMs(holdNorm);
    hihatGate.SetTargetVoltage(hihatLane.targetVoltage);
    sequencer.SetHihatHoldMs(hihatLane.holdMs);
}

} // namespace

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        auto frame = sequencer.ProcessAudio();

        // Write Gates
        patch.gate_out_1.Write(sequencer.IsGateHigh(0)); // Kick
        patch.gate_out_2.Write(sequencer.IsGateHigh(1)); // Snare

        // Audio Out (scaled to ±5 V gate targets)
        out[0][i] = accentGate.Render(frame[0]);
        out[1][i] = hihatGate.Render(frame[1]);
    }
}

void ProcessControls()
{
    patch.ProcessAnalogControls();
    tapButton.Debounce();
    modeSwitch.Debounce();

    const float knobX = patch.GetAdcValue(CV_1);
    const float knobY = patch.GetAdcValue(CV_2);
    const float knobChaos = patch.GetAdcValue(CV_3);
    const float knobTempo = patch.GetAdcValue(CV_4);

    const float cvX = patch.GetAdcValue(CV_5);
    const float cvY = patch.GetAdcValue(CV_6);
    const float cvChaos = patch.GetAdcValue(CV_7);
    const float cvTempo = patch.GetAdcValue(CV_8);

    const float channel1 = MixControl(knobX, cvX);
    const float channel2 = MixControl(knobY, cvY);
    const float channel3 = MixControl(knobChaos, cvChaos);
    const float channel4 = MixControl(knobTempo, cvTempo);

    const bool configMode = modeSwitch.Pressed();
    const uint32_t now    = System::GetNow();

    if(configMode)
    {
        UpdateAccentLaneFromControls(channel1, channel2);
        UpdateHihatLaneFromControls(channel3, channel4);
        sequencer.SetTempoControl(performanceState.tempo);
        sequencer.SetStyle(performanceState.mapX);
        sequencer.SetLowVariation(performanceState.chaos);
        sequencer.SetHighVariation(performanceState.chaos);
        // mapY ignored as per new interface
    }
    else
    {
        bool tapTrig = tapButton.RisingEdge();
        if(tapTrig)
        {
            sequencer.TriggerTapTempo(now);
        }

        performanceState.mapX   = channel1;
        performanceState.mapY   = channel2;
        performanceState.chaos  = channel3;
        performanceState.tempo  = channel4;
        
        sequencer.SetTempoControl(performanceState.tempo);
        sequencer.SetStyle(performanceState.mapX);
        sequencer.SetLowVariation(performanceState.chaos);
        sequencer.SetHighVariation(performanceState.chaos);
    }

    // User/front LED Sync
    const bool ledState = configMode ? true : sequencer.IsGateHigh(0);
    patch.SetLed(ledState);
    patch.WriteCvOut(patch_sm::CV_OUT_2, LedIndicator::VoltageForState(ledState));
    patch.WriteCvOut(patch_sm::CV_OUT_1,
                     LedIndicator::VoltageForState(sequencer.IsClockHigh()));
}

int main(void)
{
    patch.Init();

    // Initialize Audio
    patch.SetAudioBlockSize(4);
    patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    float sampleRate = patch.AudioSampleRate();

    // Initialize Sequencer and gate lanes
    sequencer.Init(sampleRate);
    accentLane.targetVoltage = GateScaler::kGateVoltageLimit;
    hihatLane.targetVoltage  = GateScaler::kGateVoltageLimit;
    accentLane.holdMs        = kDefaultGateHoldMs;
    hihatLane.holdMs         = kDefaultGateHoldMs;
    accentGate.SetTargetVoltage(accentLane.targetVoltage);
    hihatGate.SetTargetVoltage(hihatLane.targetVoltage);
    sequencer.SetAccentHoldMs(accentLane.holdMs);
    sequencer.SetHihatHoldMs(hihatLane.holdMs);

    // Ensure LEDs start in a known state.
    patch.SetLed(false);
    patch.WriteCvOut(patch_sm::CV_OUT_2, LedIndicator::kLedOffVoltage);
    patch.WriteCvOut(patch_sm::CV_OUT_1, LedIndicator::kLedOffVoltage);

    // Initialize Controls
    // Button B7 (tap)
    tapButton.Init(DaisyPatchSM::B7, 1000.0f);
    // Switch B8 (mode toggle)
    modeSwitch.Init(DaisyPatchSM::B8,
                    1000.0f,
                    Switch::TYPE_TOGGLE,
                    Switch::POLARITY_INVERTED);

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}
