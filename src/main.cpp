/**
 * Phase 2: The Clock & Simple Sequencer
 * - Internal Clock Engine (Metro)
 * - Knob 4 controls Tempo (30-200 BPM)
 * - Gate Outs trigger on beat (10ms pulse)
 * - Audio L/R outputs a "beep" on beat
 * - User LED syncs to beat
 * - Button B7 allows Tap Tempo
 */

#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "Engine/Sequencer.h"

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
        float sig = sequencer.ProcessAudio();
        bool gateState = sequencer.IsGateHigh();

        // Write Gates
        patch.gate_out_1.Write(gateState);
        patch.gate_out_2.Write(gateState);

        out[0][i] = sig;
        out[1][i] = sig;
    }
}

void ProcessControls()
{
    patch.ProcessAnalogControls();
    tapButton.Debounce();

    float knobVal = patch.GetAdcValue(CV_4);
    bool tapTrig = tapButton.RisingEdge();
    uint32_t now = System::GetNow();

    sequencer.ProcessControl(knobVal, tapTrig, now);

    // User LED Sync
    patch.SetLed(sequencer.IsGateHigh());
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
