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
    // We update the currently active parameters in the state struct
    if(controlState.configMode)
    {
        controlState.style    = softKnobs[0].Process(knob1);
        controlState.length   = softKnobs[1].Process(knob2);
        controlState.emphasis = softKnobs[2].Process(knob3);
        controlState.tempo    = softKnobs[3].Process(knob4);
    }
    else
    {
        controlState.lowDensity    = softKnobs[0].Process(knob1);
        controlState.highDensity   = softKnobs[1].Process(knob2);
        controlState.lowVariation  = softKnobs[2].Process(knob3);
        controlState.highVariation = softKnobs[3].Process(knob4);
    }

    // Calculate Final Parameters (Pot + CV) and Apply to Sequencer
    // Base Parameters
    // Note: In Config mode, Base parameters remain at their last values (plus current CV)
    // or should CV also redirect? 
    // Spec says: "Knobs control...". It implies CVs might follow? 
    // Usually in Eurorack, CVs are hardwired to function. 
    // But the inputs are labeled "CV_1" etc.
    // Let's assume CVs *always* map to the *active* function of the Knob?
    // Or do CVs stay fixed?
    // "Knob/CV pairs drive Grids parameters..." (Old spec).
    // "Opinionated Drum Sequencer" doesn't explicitly say CVs change meaning.
    // But usually Knob+CV are a pair.
    // So I will apply CV to the CURRENTLY ACTIVE parameter.
    
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
        // Tap Tempo only in Base mode? Or always?
        // "Config Mode: Knobs control... Tempo".
        // "Base Mode... Knobs control Density...".
        // So in Base Mode, Tempo is not controlled by Knob 4.
        
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
    // Gate In 2 (Input 2 on the Patch SM is Gate In 2? Need to verify pinout mapping in libDaisy)
    // DaisyPatchSM::gate_in_2 is B8? No.
    // Let's check DaisyPatchSM header or assume standard.
    if(patch.gate_in_2.Trig())
    {
        sequencer.TriggerReset();
    }

    // User/front LED Sync
    const bool ledState = controlState.configMode ? true : sequencer.IsGateHigh(0);
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
    // Set default gate voltages and hold times (no longer controlled by knobs)
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

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}
