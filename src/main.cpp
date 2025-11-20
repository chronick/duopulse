/**
 * Main firmware entry point for Patch.Init Eurorack module
 * 
 * This file initializes the hardware and sets up the audio processing callback.
 * Customize the audio processing logic in the AudioCallback function.
 */

#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

// Hardware object
DaisyPatch patch;

// DSP objects (add your custom processors here)
// Example: Oscillator osc;
// Example: Adsr env;

// Audio callback function
// This runs in real-time and must complete within the sample period
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // Process audio samples
    for (size_t i = 0; i < size; i++)
    {
        // Read inputs
        float in_left  = in[0][i];
        float in_right = in[1][i];

        // Process audio here
        // Example:
        // float osc_out = osc.Process();
        // float env_out = env.Process();
        // float output = osc_out * env_out;

        // For now, pass through
        float out_left  = in_left;
        float out_right = in_right;

        // Write outputs
        out[0][i] = out_left;
        out[1][i] = out_right;
    }
}

// CV processing callback
// This runs periodically to update CV outputs based on CV inputs
void ProcessControls()
{
    // Read CV inputs
    // float cv1 = patch.GetAdcValue(0); // 0.0 to 1.0
    // float cv2 = patch.GetAdcValue(1); // 0.0 to 1.0

    // Process CV and update parameters
    // Example:
    // float freq = 20.0f + cv1 * 19980.0f; // 20Hz to 20kHz
    // osc.SetFreq(freq);

    // Write CV outputs
    // patch.seed.dac.WriteValue(DacHandle::Channel::ONE, cv1 * 4095);
    // patch.seed.dac.WriteValue(DacHandle::Channel::TWO, cv2 * 4095);
}

int main(void)
{
    // Initialize hardware
    patch.Init();

    // Configure audio settings
    float sample_rate = patch.AudioSampleRate();
    patch.StartAudio(AudioCallback);

    // Initialize DSP objects
    // Example:
    // osc.Init(sample_rate);
    // osc.SetFreq(440.0f);
    // osc.SetAmp(0.5f);
    //
    // env.Init(sample_rate);
    // env.SetTime(ADSR_SEG_ATTACK, 0.01f);
    // env.SetTime(ADSR_SEG_DECAY, 0.1f);
    // env.SetSustainLevel(0.7f);
    // env.SetTime(ADSR_SEG_RELEASE, 0.2f);

    // Main loop
    while (1)
    {
        // Process controls (CV, buttons, etc.)
        ProcessControls();

        // Small delay to prevent excessive CPU usage
        System::Delay(1);
    }
}

