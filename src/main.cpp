/**
 * Phase 1 demo firmware for Daisy Patch.Init (Patch SM).
 * - Initializes Patch.SM hardware and audio chain.
 * - Outputs a constant sine wave on Audio L/R.
 * - Blinks the User LED at 1 Hz and alternates Gate Outs every second.
 * - Continuously ramps CV Out 1 from 0V to 5V.
 */

#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "System/SystemState.h"

using namespace daisy;
using namespace daisysp;
using namespace daisy::patch_sm;

namespace
{
constexpr float    kTestToneFrequency     = 220.0f;
constexpr float    kTestToneAmplitude     = 0.25f;
constexpr size_t   kAudioBlockSize        = 4;

DaisyPatchSM patch;
Oscillator   testOsc;
SystemState  systemState;

} // namespace

void AudioCallback(AudioHandle::InputBuffer  /*in*/,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        const float oscSample = testOsc.Process();
        out[0][i]             = oscSample;
        out[1][i]             = oscSample;
    }
}

void ProcessControls()
{
    patch.ProcessAnalogControls();

    const uint32_t nowMs = System::GetNow();
    auto state = systemState.Process(nowMs);
    
    patch.SetLed(state.ledOn);
    patch.gate_out_1.Write(state.gate1High);
    patch.gate_out_2.Write(state.gate2High);
    patch.WriteCvOut(CV_OUT_1, state.cvOutputVolts);
}

int main(void)
{
    patch.Init();
    patch.SetAudioBlockSize(kAudioBlockSize);
    patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    const float sampleRate = patch.AudioSampleRate();
    testOsc.Init(sampleRate);
    testOsc.SetWaveform(Oscillator::WAVE_SIN);
    testOsc.SetFreq(kTestToneFrequency);
    testOsc.SetAmp(kTestToneAmplitude);

    const uint32_t nowMs = System::GetNow();
    systemState.Init(nowMs);
    
    // Apply initial state immediately
    auto state = systemState.Process(nowMs);
    patch.SetLed(state.ledOn);
    patch.gate_out_1.Write(state.gate1High);
    patch.gate_out_2.Write(state.gate2High);
    patch.WriteCvOut(CV_OUT_1, state.cvOutputVolts);

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}

