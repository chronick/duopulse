/**
 * Phase 1 demo firmware for Daisy Patch.Init (Patch SM).
 * - Initializes Patch.SM hardware and audio chain.
 * - Outputs a constant sine wave on Audio L/R.
 * - Blinks the User LED at 1 Hz and alternates Gate Outs every second.
 * - Continuously ramps CV Out 1 from 0V to 5V.
 */

#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace daisy::patch_sm;

namespace
{
constexpr float    kTestToneFrequency     = 220.0f;
constexpr float    kTestToneAmplitude     = 0.25f;
constexpr uint32_t kLedToggleIntervalMs   = 500;  // 1 Hz blink (500ms on/off)
constexpr uint32_t kGateToggleIntervalMs  = 1000; // Swap gates once per second
constexpr uint32_t kCvRampPeriodMs        = 4000; // Full ramp over 4 seconds
constexpr float    kCvRampMaxVoltage      = 5.0f;
constexpr size_t   kAudioBlockSize        = 4;

DaisyPatchSM patch;
Oscillator   testOsc;

uint32_t lastLedToggleMs  = 0;
uint32_t lastGateToggleMs = 0;
uint32_t lastCvUpdateMs   = 0;

bool  ledState        = false;
bool  gateOneIsHigh   = false;
float cvOutVoltage    = 0.0f;

float CvSlopePerMs()
{
    return kCvRampMaxVoltage / static_cast<float>(kCvRampPeriodMs);
}

void UpdateLed(uint32_t nowMs)
{
    if(nowMs - lastLedToggleMs >= kLedToggleIntervalMs)
    {
        ledState = !ledState;
        patch.SetLed(ledState);
        lastLedToggleMs = nowMs;
    }
}

void UpdateGates(uint32_t nowMs)
{
    if(nowMs - lastGateToggleMs >= kGateToggleIntervalMs)
    {
        gateOneIsHigh = !gateOneIsHigh;
        patch.gate_out_1.Write(gateOneIsHigh);
        patch.gate_out_2.Write(!gateOneIsHigh);
        lastGateToggleMs = nowMs;
    }
}

void UpdateCvOutput(uint32_t nowMs)
{
    const uint32_t elapsedMs = nowMs - lastCvUpdateMs;
    if(elapsedMs == 0)
    {
        return;
    }

    cvOutVoltage += static_cast<float>(elapsedMs) * CvSlopePerMs();
    while(cvOutVoltage >= kCvRampMaxVoltage)
    {
        cvOutVoltage -= kCvRampMaxVoltage;
    }

    patch.WriteCvOut(CV_OUT_1, cvOutVoltage);
    lastCvUpdateMs = nowMs;
}
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
    UpdateLed(nowMs);
    UpdateGates(nowMs);
    UpdateCvOutput(nowMs);
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

    patch.SetLed(false);
    patch.gate_out_1.Write(false);
    patch.gate_out_2.Write(true); // Ensure alternating starts immediately
    patch.WriteCvOut(CV_OUT_1, cvOutVoltage);

    const uint32_t nowMs = System::GetNow();
    lastLedToggleMs      = nowMs;
    lastGateToggleMs     = nowMs;
    lastCvUpdateMs       = nowMs;

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}

