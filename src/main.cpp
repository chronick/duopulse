/**
 * Phase 4: Control Mapping & Chaos
 * - Knobs 1-4 summed with CV 5-8 (clamped 0-1)
 * - Gate 1: Kick (all hits)
 * - OUT_L (Audio L): Kick Accent CV (5 V only on accented kicks)
 * - Gate 2: Snare + Hi-hat triggers
 * - OUT_R (Audio R): Hi-hat CV (5 V on hats, 0 V on snares)
 * - CV_OUT_1: Master clock pulse, CV_OUT_2: LED mirror
 */

#include <array>

#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "Engine/Sequencer.h"
#include "Engine/LedIndicator.h"
#include "Engine/ControlUtils.h"

using namespace daisy;
using namespace daisysp;
using namespace daisy::patch_sm;
using namespace daisysp_idm_grids;

namespace
{
DaisyPatchSM patch;
Sequencer    sequencer;
Switch       tapButton;

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

        // Audio Out (AD envelopes)
        out[0][i] = frame[0];
        out[1][i] = frame[1];
    }
}

void ProcessControls()
{
    patch.ProcessAnalogControls();
    tapButton.Debounce();

    const float knobX = patch.GetAdcValue(CV_1);
    const float knobY = patch.GetAdcValue(CV_2);
    const float knobChaos = patch.GetAdcValue(CV_3);
    const float knobTempo = patch.GetAdcValue(CV_4);

    const float cvX = patch.GetAdcValue(CV_5);
    const float cvY = patch.GetAdcValue(CV_6);
    const float cvChaos = patch.GetAdcValue(CV_7);
    const float cvTempo = patch.GetAdcValue(CV_8);

    const float mapX = MixControl(knobX, cvX);
    const float mapY = MixControl(knobY, cvY);
    const float chaos = MixControl(knobChaos, cvChaos);
    const float tempo = MixControl(knobTempo, cvTempo);
    
    bool tapTrig = tapButton.RisingEdge();
    uint32_t now = System::GetNow();

    sequencer.ProcessControl(tempo, mapX, mapY, chaos, tapTrig, now);

    // User/front LED Sync (Blink on Kick)
    bool ledState = sequencer.IsGateHigh(0);
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

    // Initialize Sequencer
    sequencer.Init(sampleRate);

    // Ensure LEDs start in a known state.
    patch.SetLed(false);
    patch.WriteCvOut(patch_sm::CV_OUT_2, LedIndicator::kLedOffVoltage);
    patch.WriteCvOut(patch_sm::CV_OUT_1, LedIndicator::kLedOffVoltage);

    // Initialize Controls
    // Button B7
    tapButton.Init(DaisyPatchSM::B7, 1000.0f);

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}
