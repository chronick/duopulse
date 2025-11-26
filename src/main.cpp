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
#include "Engine/SoftKnob.h"

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

bool lastGateIn1 = false;

struct ControlState
{
    // Base Mode Parameters
    float lowDensity    = 0.5f;
    float highDensity   = 0.5f;
    float lowVariation  = 0.0f;
    float highVariation = 0.0f;

    // Config Mode Parameters
    float style    = 0.0f;
    float length   = 0.5f; // Maps to ~4 bars
    float emphasis = 0.5f;
    float tempo    = 0.5f;

    bool configMode = false;
};

ControlState controlState;
SoftKnob     softKnobs[4];

// UX State
uint32_t lastInteractionTime = 0;
float    activeParameterValue = 0.0f;

int MapToLength(float value)
{
    // 1, 2, 4, 8, 16 bars
    if(value < 0.2f) return 1;
    if(value < 0.4f) return 2;
    if(value < 0.6f) return 4;
    if(value < 0.8f) return 8;
    return 16;
}

} // namespace

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        // Handle External Clock (Gate In 1)
        bool gateIn1 = patch.gate_in_1.State();
        if(gateIn1 && !lastGateIn1)
        {
            sequencer.TriggerExternalClock();
        }
        lastGateIn1 = gateIn1;

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

    // Mode Switching
    bool newConfigMode = modeSwitch.Pressed();
    if(newConfigMode != controlState.configMode)
    {
        controlState.configMode = newConfigMode;
        if(controlState.configMode)
        {
            // Switching to Config Mode: Load Config targets
            softKnobs[0].SetValue(controlState.style);
            softKnobs[1].SetValue(controlState.length);
            softKnobs[2].SetValue(controlState.emphasis);
            softKnobs[3].SetValue(controlState.tempo);
        }
        else
        {
            // Switching to Base Mode: Load Base targets
            softKnobs[0].SetValue(controlState.lowDensity);
            softKnobs[1].SetValue(controlState.highDensity);
            softKnobs[2].SetValue(controlState.lowVariation);
            softKnobs[3].SetValue(controlState.highVariation);
        }
    }

    // Read Inputs
    const float knob1 = patch.GetAdcValue(CV_1);
    const float knob2 = patch.GetAdcValue(CV_2);
    const float knob3 = patch.GetAdcValue(CV_3);
    const float knob4 = patch.GetAdcValue(CV_4);

    const float cv1 = patch.GetAdcValue(CV_5);
    const float cv2 = patch.GetAdcValue(CV_6);
    const float cv3 = patch.GetAdcValue(CV_7);
    const float cv4 = patch.GetAdcValue(CV_8);

    // Process Soft Knobs & Update State
    // We check for interaction to drive the LED
    bool interacted = false;
    
    if(controlState.configMode)
    {
        controlState.style    = softKnobs[0].Process(knob1);
        if (softKnobs[0].HasMoved()) { interacted = true; activeParameterValue = controlState.style; }
        
        controlState.length   = softKnobs[1].Process(knob2);
        if (softKnobs[1].HasMoved()) { interacted = true; activeParameterValue = controlState.length; }
        
        controlState.emphasis = softKnobs[2].Process(knob3);
        if (softKnobs[2].HasMoved()) { interacted = true; activeParameterValue = controlState.emphasis; }
        
        controlState.tempo    = softKnobs[3].Process(knob4);
        if (softKnobs[3].HasMoved()) { interacted = true; activeParameterValue = controlState.tempo; }
    }
    else
    {
        controlState.lowDensity    = softKnobs[0].Process(knob1);
        if (softKnobs[0].HasMoved()) { interacted = true; activeParameterValue = controlState.lowDensity; }
        
        controlState.highDensity   = softKnobs[1].Process(knob2);
        if (softKnobs[1].HasMoved()) { interacted = true; activeParameterValue = controlState.highDensity; }
        
        controlState.lowVariation  = softKnobs[2].Process(knob3);
        if (softKnobs[2].HasMoved()) { interacted = true; activeParameterValue = controlState.lowVariation; }
        
        controlState.highVariation = softKnobs[3].Process(knob4);
        if (softKnobs[3].HasMoved()) { interacted = true; activeParameterValue = controlState.highVariation; }
    }
    
    if (interacted) {
        lastInteractionTime = System::GetNow();
    }

    // Calculate Final Parameters (Pot + CV) and Apply to Sequencer
    if(controlState.configMode)
    {
        // Config Mode
        float finalStyle    = MixControl(controlState.style, cv1);
        float finalLength   = MixControl(controlState.length, cv2);
        float finalEmphasis = MixControl(controlState.emphasis, cv3);
        float finalTempo    = MixControl(controlState.tempo, cv4);

        sequencer.SetStyle(finalStyle);
        sequencer.SetLength(MapToLength(finalLength));
        sequencer.SetEmphasis(finalEmphasis);
        sequencer.SetTempoControl(finalTempo);
    }
    else
    {
        // Base Mode
        // Tap Tempo
        bool tapTrig = tapButton.RisingEdge();
        if(tapTrig)
        {
            sequencer.TriggerTapTempo(System::GetNow());
        }

        float finalLowDens  = MixControl(controlState.lowDensity, cv1);
        float finalHighDens = MixControl(controlState.highDensity, cv2);
        float finalLowVar   = MixControl(controlState.lowVariation, cv3);
        float finalHighVar  = MixControl(controlState.highVariation, cv4);

        sequencer.SetLowDensity(finalLowDens);
        sequencer.SetHighDensity(finalHighDens);
        sequencer.SetLowVariation(finalLowVar);
        sequencer.SetHighVariation(finalHighVar);
    }

    // Reset Trigger
    if(patch.gate_in_2.Trig())
    {
        sequencer.TriggerReset();
    }

    // User/front LED Sync
    // If interaction active (< 1s), show parameter value brightness
    if (System::GetNow() - lastInteractionTime < 1000)
    {
        // Use full 5V range for brightness.
        // Assuming activeParameterValue is 0.0-1.0.
        // CV_OUT_2 drives the LED.
        patch.WriteCvOut(patch_sm::CV_OUT_2, activeParameterValue * 5.0f);
        // Also update digital LED state for redundancy if underlying impl differs?
        // SetLed takes boolean, so it's ON/OFF.
        // We probably shouldn't toggle it here if we want smooth analog brightness.
        // But if we don't SetLed(true), does it override DAC?
        // Usually SetLed writes to the GPIO connected to the LED.
        // If that GPIO is also the DAC, they might conflict or be the same.
        // On Patch SM, LED is on C1 (DAC 2).
        // Calling WriteCvOut is correct for brightness.
    }
    else
    {
        // Default Behavior
        const bool ledState = controlState.configMode ? true : sequencer.IsGateHigh(0);
        patch.SetLed(ledState); // Digital control
        patch.WriteCvOut(patch_sm::CV_OUT_2, LedIndicator::VoltageForState(ledState));
    }

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
    // Set default gate voltages and hold times
    accentGate.SetTargetVoltage(GateScaler::kGateVoltageLimit);
    hihatGate.SetTargetVoltage(GateScaler::kGateVoltageLimit);
    sequencer.SetAccentHoldMs(10.0f);
    sequencer.SetHihatHoldMs(10.0f);

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

    // Initialize Soft Knobs to Base Mode defaults
    softKnobs[0].Init(controlState.lowDensity);
    softKnobs[1].Init(controlState.highDensity);
    softKnobs[2].Init(controlState.lowVariation);
    softKnobs[3].Init(controlState.highVariation);
    
    // Initialize interaction state
    lastInteractionTime = 0; // Ensures we start in default mode

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}
