/**
 * Phase 3: The "Grids" Core
 * - Pattern Generator (Map X/Y)
 * - Knob 1: Map X
 * - Knob 2: Map Y
 * - Knob 4: Tempo
 * - Gate 1: Kick
 * - Gate 2: Snare
 * - Audio L/R: Hi-Hat
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
        
        // Write Gates
        patch.gate_out_1.Write(sequencer.IsGateHigh(0)); // Kick
        patch.gate_out_2.Write(sequencer.IsGateHigh(1)); // Snare

        // Audio Out (HH)
        out[0][i] = sig;
        out[1][i] = sig;
    }
}

void ProcessControls()
{
    patch.ProcessAnalogControls();
    tapButton.Debounce();

    float knobX = patch.GetAdcValue(CV_1);
    float knobY = patch.GetAdcValue(CV_2);
    float knobTempo = patch.GetAdcValue(CV_4);
    
    bool tapTrig = tapButton.RisingEdge();
    uint32_t now = System::GetNow();

    sequencer.ProcessControl(knobTempo, knobX, knobY, tapTrig, now);

    // User LED Sync (Blink on Kick)
    patch.SetLed(sequencer.IsGateHigh(0));
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
