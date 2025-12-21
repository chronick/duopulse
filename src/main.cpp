/**
 * DuoPulse v4: Archetype-Based Pulse Field Sequencer
 * 
 * Control System (4 modes × 4 knobs = 16 parameters):
 * 
 * Performance Mode (Switch DOWN):
 *   Primary:     K1=ENERGY, K2=BUILD, K3=FIELD X, K4=FIELD Y
 *   Shift (B7):  K1=PUNCH, K2=GENRE, K3=DRIFT, K4=BALANCE
 * 
 * Config Mode (Switch UP):
 *   Primary:     K1=Pattern Length, K2=Swing, K3=AUX Mode, K4=Reset Mode
 *   Shift (B7):  K1=Phrase Length, K2=Clock Div, K3=AUX Density, K4=Voice Coupling
 * 
 * CV Inputs:
 *   CV 1-4: Modulate ENERGY, BUILD, FIELD X, FIELD Y respectively
 *   Audio In L: Fill CV (gate + intensity)
 *   Audio In R: Flavor CV (timing/broken effects)
 * 
 * Outputs:
 *   Gate Out 1: Anchor trigger
 *   Gate Out 2: Shimmer trigger
 *   Audio Out L: Anchor velocity (sample & hold, 0-5V)
 *   Audio Out R: Shimmer velocity (sample & hold, 0-5V)
 *   CV Out 1: AUX output (mode-dependent: HAT/FILL_GATE/PHRASE_CV/EVENT)
 *   CV Out 2: LED feedback
 * 
 * Reference: docs/specs/main.md
 */

#include <array>

#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "Engine/Sequencer.h"
// #include "Engine/LedIndicator.h"  // LED system simplified
#include "Engine/ControlUtils.h"
#include "Engine/GateScaler.h"
#include "Engine/SoftKnob.h"
#include "Engine/VelocityOutput.h"
#include "Engine/AuxOutput.h"
#include "Engine/Persistence.h"

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

// Output processors
GateScaler     anchorGate;
GateScaler     shimmerGate;
VelocityOutput velocityOutput;
AuxOutput      auxOutput;

// LED system simplified - no longer using LedIndicator or LedState
// LedIndicator ledIndicator;
// LedState     ledState;

// Persistence
PersistentConfig currentConfig;
AutoSaveState    autoSaveState;
bool             configLoaded = false;

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

// Shift timing threshold
// B7 is shift-only: hold for shift layer, no tap tempo
constexpr uint32_t kShiftThresholdMs = 100;   // Hold >100ms = shift active

struct MainControlState
{
    // === Performance Mode Primary (Switch DOWN, no shift) ===
    // CV-modulatable via CV1-CV4
    // NOTE: Defaults tuned for immediate musical results on power-up
#if defined(DEBUG_BASELINE_MODE)
    // Debug baseline: Active "groovy" center position
    float energy = 0.75f; // K1: Hit density - BUILD zone for active patterns
    float build  = 0.3f;  // K2: Phrase arc - moderate phrase development
    float fieldX = 0.5f;  // K3: Syncopation axis - center = Groovy archetype
    float fieldY = 0.5f;  // K4: Complexity axis - center = Groovy archetype
#else
    // Production defaults: Musical starting point (not sparse minimal)
    float energy = 0.6f;  // K1: Hit density - mid-GROOVE zone
    float build  = 0.0f;  // K2: Phrase arc (0=flat, 1=dramatic build)
    float fieldX = 0.5f;  // K3: Center position = Groovy archetype
    float fieldY = 0.33f; // K4: Between minimal and driving = solid groove
#endif

    // === Performance Mode Shift (Switch DOWN + B7 held) ===
    float punch   = 0.5f; // K1+Shift: Velocity dynamics (0=flat, 1=punchy)
    float genre   = 0.0f; // K2+Shift: Genre selection (TECHNO/TRIBAL/IDM)
    float drift   = 0.0f; // K3+Shift: Pattern evolution (0=locked, 1=generative)
    float balance = 0.5f; // K4+Shift: Voice ratio (0=anchor-heavy, 1=shimmer-heavy)

    // === Config Mode Primary (Switch UP, no shift) ===
    float patternLengthKnob = 0.5f; // K1: Pattern length (16/24/32/64 steps)
    float swing             = 0.0f; // K2: Base swing amount
    float auxMode           = 0.0f; // K3: AUX output mode (HAT/FILL_GATE/PHRASE_CV/EVENT)
    float resetMode         = 0.0f; // K4: Reset behavior (PHRASE/BAR/STEP)

    // === Config Mode Shift (Switch UP + B7 held) ===
    float phraseLengthKnob = 0.5f; // K1+Shift: Phrase length (1/2/4/8 bars)
    float clockDivKnob     = 0.0f; // K2+Shift: Clock division (1/2/4/8)
    float auxDensity       = 0.5f; // K3+Shift: AUX density (SPARSE/NORMAL/DENSE/BUSY)
    float voiceCoupling    = 0.0f; // K4+Shift: Voice coupling (INDEPENDENT/INTERLOCK/SHADOW)

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

MainControlState controlState;
SoftKnob     softKnobs[kTotalKnobs]; // 16 slots: 4 knobs × 4 mode/shift combinations

// UX State
uint32_t lastInteractionTime  = 0;
float    activeParameterValue = 0.0f;

// Shift Button State (B7) - shift-only, no tap tempo
uint32_t buttonPressTime   = 0;     // When button was pressed (ms)
bool     buttonWasPressed  = false; // Previous button state
bool     shiftEngaged      = false; // True once hold threshold passed

// Helper functions for discrete parameter mapping
int MapToPatternLength(float value)
{
    // 16, 24, 32, 64 steps
    if(value < 0.25f) return 16;
    if(value < 0.5f) return 24;
    if(value < 0.75f) return 32;
    return 64;
}

int MapToPhraseLength(float value)
{
    // 1, 2, 4, 8 bars
    if(value < 0.25f) return 1;
    if(value < 0.5f) return 2;
    if(value < 0.75f) return 4;
    return 8;
}

int MapToClockDivision(float value)
{
    // 1, 2, 4, 8
    if(value < 0.25f) return 1;
    if(value < 0.5f) return 2;
    if(value < 0.75f) return 4;
    return 8;
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

        // Process one audio sample (returns velocity values)
        auto velocities = sequencer.ProcessAudio();

        // Write Gate Triggers (Gate Out 1 = Anchor, Gate Out 2 = Shimmer)
        patch.gate_out_1.Write(sequencer.IsGateHigh(0)); // Anchor
        patch.gate_out_2.Write(sequencer.IsGateHigh(1)); // Shimmer

        // Audio Outputs: Velocity Sample & Hold (0-5V range)
        // The codec uses inverted polarity, so we negate the sample
        // velocities[0] and velocities[1] are 0-1 range from the sequencer
        out[0][i] = GateScaler::VoltageToCodecSample(velocities[0] * 5.0f); // Anchor velocity
        out[1][i] = GateScaler::VoltageToCodecSample(velocities[1] * 5.0f); // Shimmer velocity

        // Process auto-save (sample-rate timing)
        if(ProcessAutoSave(autoSaveState))
        {
            // Build current config from control state
            PackConfig(
                MapToPatternLength(controlState.patternLengthKnob),
                controlState.swing,
                GetAuxModeFromValue(controlState.auxMode),
                GetResetModeFromValue(controlState.resetMode),
                MapToPhraseLength(controlState.phraseLengthKnob),
                MapToClockDivision(controlState.clockDivKnob),
                GetAuxDensityFromValue(controlState.auxDensity),
                GetVoiceCouplingFromValue(controlState.voiceCoupling),
                GetGenreFromValue(controlState.genre),
                currentConfig.patternSeed,
                currentConfig
            );

            // Save to flash (only if changed from last saved)
            if(ConfigChanged(currentConfig, autoSaveState.lastSaved))
            {
                SaveConfigToFlash(currentConfig);
                autoSaveState.lastSaved = currentConfig;
            }
            autoSaveState.ClearPending();
        }
    }
}

// Helper to get pointer to parameter by mode and knob index
// DuoPulse v4 Control Layout:
//   Performance Primary: ENERGY, BUILD, FIELD X, FIELD Y
//   Performance Shift:   PUNCH, GENRE, DRIFT, BALANCE
//   Config Primary:      Pattern Length, Swing, AUX Mode, Reset Mode
//   Config Shift:        Phrase Length, Clock Div, AUX Density, Voice Coupling
float* GetParameterPtr(MainControlState& state, ControlMode mode, int knobIndex)
{
    switch(mode)
    {
        case ControlMode::PerformancePrimary:
            switch(knobIndex)
            {
                case 0: return &state.energy;
                case 1: return &state.build;
                case 2: return &state.fieldX;
                case 3: return &state.fieldY;
            }
            break;
        case ControlMode::PerformanceShift:
            switch(knobIndex)
            {
                case 0: return &state.punch;
                case 1: return &state.genre;
                case 2: return &state.drift;
                case 3: return &state.balance;
            }
            break;
        case ControlMode::ConfigPrimary:
            switch(knobIndex)
            {
                case 0: return &state.patternLengthKnob;
                case 1: return &state.swing;
                case 2: return &state.auxMode;
                case 3: return &state.resetMode;
            }
            break;
        case ControlMode::ConfigShift:
            switch(knobIndex)
            {
                case 0: return &state.phraseLengthKnob;
                case 1: return &state.clockDivKnob;
                case 2: return &state.auxDensity;
                case 3: return &state.voiceCoupling;
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

    // Shift Detection (B7 button: hold for shift layer)
    // Simplified: no tap tempo, just shift-only
    bool     buttonPressed = tapButton.Pressed();
    uint32_t now           = System::GetNow();

    if(buttonPressed && !buttonWasPressed)
    {
        // Button just pressed - start timing
        buttonPressTime = now;
        shiftEngaged    = false;
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
        // Button released - clear shift
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

    // Read Audio Inputs (used for Fill CV and Flavor CV)
    // Audio In L = Fill CV (pressure-sensitive fill trigger)
    // Audio In R = Flavor CV (timing/broken effects)
    // Note: fillCV will be used for live fill mode (future enhancement)
    (void)patch.GetAdcValue(ADC_9);  // Audio In L (fillCV - reserved for future)
    const float flavorCV = patch.GetAdcValue(ADC_10);  // Audio In R

    // CV Always Modulates Performance Primary Parameters (regardless of mode)
    // CV1-CV4 modulate ENERGY, BUILD, FIELD X, FIELD Y
    // Uses bipolar modulation: CV adds ±0.5 to knob value, clamped 0-1
    float finalEnergy = MixControl(controlState.energy, cv1);
    float finalBuild  = MixControl(controlState.build, cv2);
    float finalFieldX = MixControl(controlState.fieldX, cv3);
    float finalFieldY = MixControl(controlState.fieldY, cv4);

    // Apply all DuoPulse v4 parameters to sequencer
    // Performance Primary (CV-modulated)
    sequencer.SetEnergy(finalEnergy);
    sequencer.SetBuild(finalBuild);
    sequencer.SetFieldX(finalFieldX);
    sequencer.SetFieldY(finalFieldY);

    // Performance Shift (knob-only, no CV modulation)
    sequencer.SetPunch(controlState.punch);
    sequencer.SetGenre(controlState.genre);
    sequencer.SetDrift(controlState.drift);
    sequencer.SetBalance(controlState.balance);

    // Config Primary
    sequencer.SetPatternLength(MapToPatternLength(controlState.patternLengthKnob));
    sequencer.SetSwing(controlState.swing);
    sequencer.SetAuxMode(controlState.auxMode);
    sequencer.SetResetMode(controlState.resetMode);

    // Config Shift
    sequencer.SetPhraseLength(MapToPhraseLength(controlState.phraseLengthKnob));
    sequencer.SetClockDivision(MapToClockDivision(controlState.clockDivKnob));
    sequencer.SetAuxDensity(controlState.auxDensity);
    sequencer.SetVoiceCoupling(controlState.voiceCoupling);

    // CV modulation inputs
    sequencer.SetEnergyCV(cv1 - 0.5f);  // Convert to bipolar
    sequencer.SetBuildCV(cv2 - 0.5f);
    sequencer.SetFieldXCV(cv3 - 0.5f);
    sequencer.SetFieldYCV(cv4 - 0.5f);
    sequencer.SetFlavorCV(flavorCV);

    // Mark config dirty if any config parameter changed
    // (We track this via soft knob movements in config modes)
    if(currentMode == ControlMode::ConfigPrimary || currentMode == ControlMode::ConfigShift)
    {
        if(interacted)
        {
            MarkConfigDirty(autoSaveState);
        }
    }

    // Tap Tempo removed - tempo is internal-only or external clock

    // Reset Trigger
    if(patch.gate_in_2.Trig())
    {
        sequencer.TriggerReset();
    }

    // === LED Feedback System (Simplified) ===
    // Config mode: solid on
    // Anchor trigger (Gate 1): 50% brightness
    // Shimmer trigger (Gate 2): 30% brightness
    // Otherwise: off

    bool anchorGateHigh = sequencer.IsGateHigh(0);
    bool shimmerGateHigh = sequencer.IsGateHigh(1);

    float ledBrightness = 0.0f;

    if(controlState.configMode)
    {
        // Config mode: solid on
        ledBrightness = 1.0f;
    }
    else if(anchorGateHigh)
    {
        // Anchor trigger: 50% brightness
        ledBrightness = 0.5f;
    }
    else if(shimmerGateHigh)
    {
        // Shimmer trigger: 30% brightness
        ledBrightness = 0.3f;
    }

    // Convert brightness to voltage (0-5V range)
    float ledVoltage = ledBrightness * 5.0f;
    bool  ledDigital = (ledBrightness > 0.1f);

    patch.SetLed(ledDigital);
    patch.WriteCvOut(patch_sm::CV_OUT_2, ledVoltage);

    // === AUX Output (CV_OUT_1) ===
    // Mode-dependent output: HAT trigger, FILL_GATE, PHRASE_CV ramp, or EVENT trigger
    const auto& phrasePos = sequencer.GetPhrasePosition();
    float auxVoltage = 0.0f;
    AuxMode currentAuxMode = GetAuxModeFromValue(controlState.auxMode);
    
    switch(currentAuxMode)
    {
        case AuxMode::HAT:
        case AuxMode::EVENT:
            // Trigger output - use clock high as proxy for now
            // Real implementation uses aux hit mask from sequencer
            auxVoltage = sequencer.IsClockHigh() ? 5.0f : 0.0f;
            break;
        
        case AuxMode::FILL_GATE:
            // Gate high during fill zones
            auxVoltage = phrasePos.isFillZone ? 5.0f : 0.0f;
            break;
        
        case AuxMode::PHRASE_CV:
            // Ramp 0-5V over phrase
            auxVoltage = phrasePos.phraseProgress * 5.0f;
            break;
        
        default:
            // COUNT or unknown - default to 0V
            auxVoltage = 0.0f;
            break;
    }
    
    patch.WriteCvOut(patch_sm::CV_OUT_1, auxVoltage);
}

int main(void)
{
    patch.Init();

    // Initialize Audio
    patch.SetAudioBlockSize(4);
    patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    float sampleRate = patch.AudioSampleRate();

    // === Load Config from Flash ===
    currentConfig.Init();  // Initialize with defaults
    configLoaded = LoadConfigFromFlash(currentConfig);
    
    if(configLoaded)
    {
        // Unpack saved config values into control state
        int patternLength, phraseLength, clockDivision;
        float swing;
        AuxMode auxMode;
        ResetMode resetMode;
        AuxDensity auxDensity;
        VoiceCoupling voiceCoupling;
        Genre genre;
        uint32_t patternSeed;
        
        UnpackConfig(
            currentConfig,
            patternLength, swing, auxMode, resetMode,
            phraseLength, clockDivision, auxDensity, voiceCoupling,
            genre, patternSeed
        );
        
        // Map back to knob values (for soft knob initialization)
        // Pattern length: 16=0.125, 24=0.375, 32=0.625, 64=0.875
        if(patternLength == 16) controlState.patternLengthKnob = 0.125f;
        else if(patternLength == 24) controlState.patternLengthKnob = 0.375f;
        else if(patternLength == 32) controlState.patternLengthKnob = 0.625f;
        else controlState.patternLengthKnob = 0.875f;
        
        controlState.swing = swing;
        controlState.auxMode = static_cast<float>(auxMode) / 3.0f;
        controlState.resetMode = static_cast<float>(resetMode) / 2.0f;
        
        // Phrase length: 1=0.125, 2=0.375, 4=0.625, 8=0.875
        if(phraseLength == 1) controlState.phraseLengthKnob = 0.125f;
        else if(phraseLength == 2) controlState.phraseLengthKnob = 0.375f;
        else if(phraseLength == 4) controlState.phraseLengthKnob = 0.625f;
        else controlState.phraseLengthKnob = 0.875f;
        
        // Clock division: 1=0.125, 2=0.375, 4=0.625, 8=0.875
        if(clockDivision == 1) controlState.clockDivKnob = 0.125f;
        else if(clockDivision == 2) controlState.clockDivKnob = 0.375f;
        else if(clockDivision == 4) controlState.clockDivKnob = 0.625f;
        else controlState.clockDivKnob = 0.875f;
        
        controlState.auxDensity = static_cast<float>(auxDensity) / 3.0f;
        controlState.voiceCoupling = static_cast<float>(voiceCoupling) / 2.0f;
        controlState.genre = static_cast<float>(genre) / 2.0f;
    }
    
    // Initialize auto-save state
    autoSaveState.Init(sampleRate);
    autoSaveState.lastSaved = currentConfig;

    // Initialize Sequencer
    sequencer.Init(sampleRate);
    
    // Initialize gate scalers
    anchorGate.Init(sampleRate);
    shimmerGate.Init(sampleRate);
    anchorGate.SetTargetVoltage(GateScaler::kGateVoltageLimit);
    shimmerGate.SetTargetVoltage(GateScaler::kGateVoltageLimit);
    
    // Initialize velocity output processor
    velocityOutput.Init(sampleRate);
    
    // Initialize AUX output processor
    auxOutput.Init(sampleRate);
    
    // Set default hold times
    sequencer.SetAccentHoldMs(10.0f);
    sequencer.SetHihatHoldMs(10.0f);
    
    // LED feedback simplified - no initialization needed
    // ledIndicator.Init(1000.0f);

    // Ensure LEDs start in a known state
    patch.SetLed(false);
    patch.WriteCvOut(patch_sm::CV_OUT_2, 0.0f);  // LED output
    patch.WriteCvOut(patch_sm::CV_OUT_1, 0.0f);  // AUX output

    // Initialize Controls
    // Button B7 (tap/shift)
    tapButton.Init(DaisyPatchSM::B7, 1000.0f);
    // Switch B8 (mode toggle: Performance/Config)
    modeSwitch.Init(DaisyPatchSM::B8,
                    1000.0f,
                    Switch::TYPE_TOGGLE,
                    Switch::POLARITY_INVERTED);

    // Initialize all 16 Soft Knobs (4 knobs × 4 mode/shift combinations)
    // DuoPulse v4 Control Layout
    // Performance Primary (indices 0-3): ENERGY, BUILD, FIELD X, FIELD Y
    softKnobs[0].Init(controlState.energy);
    softKnobs[1].Init(controlState.build);
    softKnobs[2].Init(controlState.fieldX);
    softKnobs[3].Init(controlState.fieldY);
    // Performance Shift (indices 4-7): PUNCH, GENRE, DRIFT, BALANCE
    softKnobs[4].Init(controlState.punch);
    softKnobs[5].Init(controlState.genre);
    softKnobs[6].Init(controlState.drift);
    softKnobs[7].Init(controlState.balance);
    // Config Primary (indices 8-11): Pattern Length, Swing, AUX Mode, Reset Mode
    softKnobs[8].Init(controlState.patternLengthKnob);
    softKnobs[9].Init(controlState.swing);
    softKnobs[10].Init(controlState.auxMode);
    softKnobs[11].Init(controlState.resetMode);
    // Config Shift (indices 12-15): Phrase Length, Clock Div, AUX Density, Voice Coupling
    softKnobs[12].Init(controlState.phraseLengthKnob);
    softKnobs[13].Init(controlState.clockDivKnob);
    softKnobs[14].Init(controlState.auxDensity);
    softKnobs[15].Init(controlState.voiceCoupling);
    
    // Initialize interaction state
    lastInteractionTime = 0; // Ensures we start in default mode

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        System::Delay(1);
    }
}
