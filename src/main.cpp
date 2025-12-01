/**
 * DuoPulse v2: 2-Voice Percussive Sequencer
 * 
 * Control System (4 modes × 4 knobs = 16 parameters):
 * 
 * Performance Mode (Switch DOWN):
 *   Primary:     K1=Anchor Density, K2=Shimmer Density, K3=Flux, K4=Fuse
 *   Shift (B7):  K1=Anchor Accent, K2=Shimmer Accent, K3=Orbit, K4=Contour
 * 
 * Config Mode (Switch UP):
 *   Primary:     K1=Terrain, K2=Length, K3=Grid, K4=Tempo
 *   Shift (B7):  K1=Swing Taste, K2=Gate Time, K3=Humanize, K4=Clock Div
 * 
 * CV inputs 5-8 always modulate performance parameters (Anchor Density, Shimmer 
 * Density, Flux, Fuse) regardless of mode.
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

// Control Mode indices for soft knob array
enum class ControlMode : uint8_t
{
    PerformancePrimary = 0, // Switch DOWN, no shift
    PerformanceShift   = 1, // Switch DOWN, shift held
    ConfigPrimary      = 2, // Switch UP, no shift
    ConfigShift        = 3  // Switch UP, shift held
};

constexpr int kKnobsPerMode = 4;
constexpr int kNumModes     = 4;
constexpr int kTotalKnobs   = kKnobsPerMode * kNumModes; // 16

// Shift/Tap timing thresholds
constexpr uint32_t kShiftThresholdMs = 150; // Hold >150ms = shift, tap <150ms = fill/tap-tempo

struct ControlState
{
    // === Performance Mode Primary (Switch DOWN, no shift) ===
    float anchorDensity  = 0.5f; // K1: Hit frequency for Anchor lane
    float shimmerDensity = 0.5f; // K2: Hit frequency for Shimmer lane
    float flux           = 0.0f; // K3: Global variation, fills, ghost notes
    float fuse           = 0.5f; // K4: Cross-lane energy exchange (center = balanced)

    // === Performance Mode Shift (Switch DOWN + B7 held) ===
    float anchorAccent  = 0.5f; // K1+Shift: Accent intensity for Anchor
    float shimmerAccent = 0.5f; // K2+Shift: Accent intensity for Shimmer
    float orbit         = 0.5f; // K3+Shift: Voice relationship (Interlock/Free/Shadow)
    float contour       = 0.0f; // K4+Shift: CV output shape (Velocity/Decay/Pitch/Random)

    // === Config Mode Primary (Switch UP, no shift) ===
    float terrain = 0.0f; // K1: Genre/style character (Techno/Tribal/Trip-Hop/IDM)
    float length  = 0.5f; // K2: Loop length in bars (1,2,4,8,16)
    float grid    = 0.0f; // K3: Pattern family selection (1-16)
    float tempo   = 0.5f; // K4: Internal BPM (90-160)

    // === Config Mode Shift (Switch UP + B7 held) ===
    float swingTaste = 0.5f; // K1+Shift: Fine-tune swing within genre range
    float gateTime   = 0.2f; // K2+Shift: Trigger duration (5-50ms, default ~14ms)
    float humanize   = 0.0f; // K3+Shift: Micro-timing jitter amount
    float clockDiv   = 0.5f; // K4+Shift: Clock output divider/multiplier (default ×1)

    // === Mode State ===
    bool configMode  = false; // Switch UP = config mode
    bool shiftActive = false; // B7 held = shift active

    // Helper to get current mode
    ControlMode GetCurrentMode() const
    {
        if(configMode)
        {
            return shiftActive ? ControlMode::ConfigShift
                               : ControlMode::ConfigPrimary;
        }
        else
        {
            return shiftActive ? ControlMode::PerformanceShift
                               : ControlMode::PerformancePrimary;
        }
    }

    // Get soft knob base index for current mode (0, 4, 8, or 12)
    int GetSoftKnobBaseIndex() const
    {
        return static_cast<int>(GetCurrentMode()) * kKnobsPerMode;
    }
};

ControlState controlState;
SoftKnob     softKnobs[kTotalKnobs]; // 16 slots: 4 knobs × 4 mode/shift combinations

// UX State
uint32_t lastInteractionTime  = 0;
float    activeParameterValue = 0.0f;

// Shift Button State (B7)
uint32_t buttonPressTime   = 0;     // When button was pressed (ms)
bool     buttonWasPressed  = false; // Previous button state
bool     shiftEngaged      = false; // True once hold threshold passed
bool     fillTriggered     = false; // Prevent double-trigger on release

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

// Helper to get pointer to parameter by mode and knob index
float* GetParameterPtr(ControlState& state, ControlMode mode, int knobIndex)
{
    switch(mode)
    {
        case ControlMode::PerformancePrimary:
            switch(knobIndex)
            {
                case 0: return &state.anchorDensity;
                case 1: return &state.shimmerDensity;
                case 2: return &state.flux;
                case 3: return &state.fuse;
            }
            break;
        case ControlMode::PerformanceShift:
            switch(knobIndex)
            {
                case 0: return &state.anchorAccent;
                case 1: return &state.shimmerAccent;
                case 2: return &state.orbit;
                case 3: return &state.contour;
            }
            break;
        case ControlMode::ConfigPrimary:
            switch(knobIndex)
            {
                case 0: return &state.terrain;
                case 1: return &state.length;
                case 2: return &state.grid;
                case 3: return &state.tempo;
            }
            break;
        case ControlMode::ConfigShift:
            switch(knobIndex)
            {
                case 0: return &state.swingTaste;
                case 1: return &state.gateTime;
                case 2: return &state.humanize;
                case 3: return &state.clockDiv;
            }
            break;
    }
    return nullptr; // Should never reach here
}

void ProcessControls()
{
    patch.ProcessAnalogControls();
    tapButton.Debounce();
    modeSwitch.Debounce();

    // Track previous mode for soft knob target loading
    ControlMode previousMode = controlState.GetCurrentMode();

    // Mode Switching (config mode from switch)
    bool newConfigMode = modeSwitch.Pressed();
    controlState.configMode = newConfigMode;

    // Shift Detection (B7 button: tap <150ms vs hold >150ms)
    bool     buttonPressed = tapButton.Pressed();
    uint32_t now           = System::GetNow();

    if(buttonPressed && !buttonWasPressed)
    {
        // Button just pressed - start timing
        buttonPressTime = now;
        shiftEngaged    = false;
        fillTriggered   = false;
    }
    else if(buttonPressed && buttonWasPressed)
    {
        // Button held - check if we've crossed shift threshold
        if(!shiftEngaged && (now - buttonPressTime) >= kShiftThresholdMs)
        {
            shiftEngaged             = true;
            controlState.shiftActive = true;
        }
    }
    else if(!buttonPressed && buttonWasPressed)
    {
        // Button just released
        if(!shiftEngaged && !fillTriggered)
        {
            // Short tap (<150ms) - trigger fill or tap tempo
            fillTriggered = true;
            if(!controlState.configMode)
            {
                // Performance mode: short tap = tap tempo
                sequencer.TriggerTapTempo(now);
            }
            // Config mode: short tap could trigger something else (future)
        }
        // Always clear shift on release
        controlState.shiftActive = false;
        shiftEngaged             = false;
    }
    buttonWasPressed = buttonPressed;

    // If mode changed, load new targets into soft knobs
    ControlMode currentMode = controlState.GetCurrentMode();
    if(currentMode != previousMode)
    {
        int baseIdx = controlState.GetSoftKnobBaseIndex();
        for(int i = 0; i < kKnobsPerMode; i++)
        {
            float* param = GetParameterPtr(controlState, currentMode, i);
            if(param)
            {
                softKnobs[baseIdx + i].SetValue(*param);
            }
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

    // Process Soft Knobs for current mode & Update State
    bool  interacted = false;
    int   baseIdx    = controlState.GetSoftKnobBaseIndex();
    float knobValues[kKnobsPerMode] = {knob1, knob2, knob3, knob4};

    for(int i = 0; i < kKnobsPerMode; i++)
    {
        float* param = GetParameterPtr(controlState, currentMode, i);
        if(param)
        {
            *param = softKnobs[baseIdx + i].Process(knobValues[i]);
            if(softKnobs[baseIdx + i].HasMoved())
            {
                interacted           = true;
                activeParameterValue = *param;
            }
        }
    }

    if(interacted)
    {
        lastInteractionTime = System::GetNow();
    }

    // CV Always Modulates Performance Parameters (regardless of mode)
    // CV centered at 0.5 (2.5V), additive modulation clamped 0-1
    // Note: cv values are 0-1, where 0.5 = 2.5V = neutral
    float finalAnchorDensity  = Clamp01(controlState.anchorDensity + (cv1 - 0.5f));
    float finalShimmerDensity = Clamp01(controlState.shimmerDensity + (cv2 - 0.5f));
    float finalFlux           = Clamp01(controlState.flux + (cv3 - 0.5f));
    float finalFuse           = Clamp01(controlState.fuse + (cv4 - 0.5f));

    // Apply all DuoPulse v2 parameters to sequencer using new interface
    // Performance parameters (CV-modulated)
    sequencer.SetAnchorDensity(finalAnchorDensity);
    sequencer.SetShimmerDensity(finalShimmerDensity);
    sequencer.SetFlux(finalFlux);
    sequencer.SetFuse(finalFuse);

    // Performance shift parameters (knob-only, no CV modulation)
    sequencer.SetAnchorAccent(controlState.anchorAccent);
    sequencer.SetShimmerAccent(controlState.shimmerAccent);
    sequencer.SetOrbit(controlState.orbit);
    sequencer.SetContour(controlState.contour);

    // Config primary parameters
    sequencer.SetTerrain(controlState.terrain);
    sequencer.SetLength(MapToLength(controlState.length));
    sequencer.SetGrid(controlState.grid);
    sequencer.SetTempoControl(controlState.tempo);

    // Config shift parameters
    sequencer.SetSwingTaste(controlState.swingTaste);
    sequencer.SetGateTime(controlState.gateTime);
    sequencer.SetHumanize(controlState.humanize);
    sequencer.SetClockDiv(controlState.clockDiv);

    // Tap Tempo is now handled in shift detection above (short tap <150ms)

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

    // Initialize all 16 Soft Knobs (4 knobs × 4 mode/shift combinations)
    // Performance Primary (indices 0-3)
    softKnobs[0].Init(controlState.anchorDensity);
    softKnobs[1].Init(controlState.shimmerDensity);
    softKnobs[2].Init(controlState.flux);
    softKnobs[3].Init(controlState.fuse);
    // Performance Shift (indices 4-7)
    softKnobs[4].Init(controlState.anchorAccent);
    softKnobs[5].Init(controlState.shimmerAccent);
    softKnobs[6].Init(controlState.orbit);
    softKnobs[7].Init(controlState.contour);
    // Config Primary (indices 8-11)
    softKnobs[8].Init(controlState.terrain);
    softKnobs[9].Init(controlState.length);
    softKnobs[10].Init(controlState.grid);
    softKnobs[11].Init(controlState.tempo);
    // Config Shift (indices 12-15)
    softKnobs[12].Init(controlState.swingTaste);
    softKnobs[13].Init(controlState.gateTime);
    softKnobs[14].Init(controlState.humanize);
    softKnobs[15].Init(controlState.clockDiv);
    
    // Initialize interaction state
    lastInteractionTime = 0; // Ensures we start in default mode

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}
