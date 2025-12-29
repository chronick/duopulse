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
#include "System/logging.h"

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

// Deferred flash save - prevents blocking in audio callback
struct DeferredSave {
    bool pending = false;
    PersistentConfig configToSave;
};
DeferredSave deferredSave;

// Debug: Track bar changes for deferred pattern logging
int lastLoggedBar = -1;

// =============================================================================
// Non-Blocking Gate Event Logger
// =============================================================================
// Ring buffer to capture gate events with true timestamps, flushed rate-limited.
// This prevents UART blocking from affecting timestamp accuracy.

struct GateEvent {
    uint32_t timestamp;   // System::GetNow() at event time
    uint8_t  gateType;    // 0=anchor, 1=shimmer
    uint8_t  step;        // Step number when event occurred
    bool     valid;       // Whether this slot has data
};

constexpr size_t kGateEventBufferSize = 32;  // Enough for 2 full bars

struct GateEventBuffer {
    GateEvent events[kGateEventBufferSize];
    size_t    writeIdx = 0;
    size_t    readIdx  = 0;
    size_t    count    = 0;

    void Push(uint32_t timestamp, uint8_t gateType, uint8_t step) {
        if (count < kGateEventBufferSize) {
            events[writeIdx] = {timestamp, gateType, step, true};
            writeIdx = (writeIdx + 1) % kGateEventBufferSize;
            count++;
        }
        // If full, oldest events are dropped (acceptable for debugging)
    }

    bool Pop(GateEvent& out) {
        if (count == 0) return false;
        out = events[readIdx];
        events[readIdx].valid = false;
        readIdx = (readIdx + 1) % kGateEventBufferSize;
        count--;
        return true;
    }

    bool HasEvents() const { return count > 0; }
};

GateEventBuffer gateEventBuffer;

// External clock detection state
bool lastGateIn1 = false;

// Periodic status logging (every 5 seconds)
uint32_t lastStatusLogTime = 0;
constexpr uint32_t kStatusLogInterval = 5000;  // 5 seconds in ms

// External clock monitoring (main loop only, not in audio callback)
uint32_t lastExternalClockTime = 0;
bool wasExternalClockActive = false;
constexpr uint32_t kExternalClockTimeout = 5000;  // 5 seconds

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
    // Production defaults: Musical starting point (not sparse minimal)
    float energy = 0.6f;  // K1: Hit density - mid-GROOVE zone
    float build  = 0.0f;  // K2: Phrase arc (0=flat, 1=dramatic build)
    float fieldX = 0.5f;  // K3: Center position = Groovy archetype
    float fieldY = 0.33f; // K4: Between minimal and driving = solid groove

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
    float phraseLengthKnob = 0.5f;  // K1+Shift: Phrase length (1/2/4/8 bars)
    float clockDivKnob     = 0.5f;  // K2+Shift: Clock division (center = ×1, normal speed)
    float auxDensity       = 0.5f;  // K3+Shift: AUX density (SPARSE/NORMAL/DENSE/BUSY)
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

// MapToClockDivision moved to ControlUtils.h as MapClockDivision
// for testability (see Modification 0.5 in task 16)

} // namespace

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // NOTE: Do NOT log from audio callback - blocks and crashes!
    for(size_t i = 0; i < size; i++)
    {
        // Handle External Clock (Gate In 1) - Exclusive mode
        // Simple rising edge detection, no timeout in audio callback
        bool gateIn1 = patch.gate_in_1.State();

        // Detect rising edge → trigger external clock
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

        // Auto-save timing check ONLY - no flash write here!
        if(ProcessAutoSave(autoSaveState))
        {
            // Build current config from control state (cheap operation)
            PackConfig(
                MapToPatternLength(controlState.patternLengthKnob),
                controlState.swing,
                GetAuxModeFromValue(controlState.auxMode),
                GetResetModeFromValue(controlState.resetMode),
                MapToPhraseLength(controlState.phraseLengthKnob),
                MapClockDivision(controlState.clockDivKnob),
                GetAuxDensityFromValue(controlState.auxDensity),
                GetVoiceCouplingFromValue(controlState.voiceCoupling),
                GetGenreFromValue(controlState.genre),
                currentConfig.patternSeed,
                currentConfig
            );

            // Check if save needed - if so, DEFER to main loop
            if(ConfigChanged(currentConfig, autoSaveState.lastSaved))
            {
                deferredSave.configToSave = currentConfig;
                deferredSave.pending = true;  // Flag for main loop
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
        // Log mode change
        const char* modeName = "";
        switch(currentMode)
        {
            case ControlMode::PerformancePrimary:
                modeName = "Performance";
                break;
            case ControlMode::PerformanceShift:
                modeName = "Performance+Shift";
                break;
            case ControlMode::ConfigPrimary:
                modeName = "Config";
                break;
            case ControlMode::ConfigShift:
                modeName = "Config+Shift";
                break;
        }
        LOGD("Mode: %s", modeName);

        // Log config values when entering config mode (for debugging)
        if (currentMode == ControlMode::ConfigPrimary || currentMode == ControlMode::ConfigShift)
        {
            // Note: cast to int*100 for percentage display (nano.specs doesn't support %f)
            LOGD("Config: AuxMode=%d%% ResetMode=%d%% PatLen=%d%% Swing=%d%%",
                 static_cast<int>(controlState.auxMode * 100),
                 static_cast<int>(controlState.resetMode * 100),
                 static_cast<int>(controlState.patternLengthKnob * 100),
                 static_cast<int>(controlState.swing * 100));
        }

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
    sequencer.SetClockDivision(MapClockDivision(controlState.clockDivKnob));
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

    // Gate event capture using LATCHED events (survives after pulse ends)
    // This fixes the race condition where trigger pulse fits in one audio block
    // and completes before main loop can detect it.
    uint8_t step = static_cast<uint8_t>(sequencer.GetPhrasePosition().stepInBar);

    if (sequencer.HasPendingTrigger(0)) {
        gateEventBuffer.Push(now, 0, step);  // 0 = anchor
        sequencer.AcknowledgeTrigger(0);
    }
    if (sequencer.HasPendingTrigger(1)) {
        gateEventBuffer.Push(now, 1, step);  // 1 = shimmer
        sequencer.AcknowledgeTrigger(1);
    }

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
    // Mode-dependent output: HAT/EVENT use aux trigger, FILL_GATE/PHRASE_CV use phrase state
    const auto& phrasePos = sequencer.GetPhrasePosition();
    AuxMode currentAuxMode = GetAuxModeFromValue(controlState.auxMode);
    float auxVoltage = 0.0f;

    switch(currentAuxMode)
    {
        case AuxMode::HAT:
        case AuxMode::EVENT:
            // Trigger output from aux hit mask (3rd voice pattern)
            auxVoltage = sequencer.IsAuxHigh() ? 5.0f : 0.0f;
            break;

        case AuxMode::FILL_GATE:
            // Gate high during fill zones (last 12.5% of phrase)
            auxVoltage = phrasePos.isFillZone ? 5.0f : 0.0f;
            break;

        case AuxMode::PHRASE_CV:
            // Ramp 0-5V over phrase
            auxVoltage = phrasePos.phraseProgress * 5.0f;
            break;

        default:
            auxVoltage = 0.0f;
            break;
    }

    patch.WriteCvOut(patch_sm::CV_OUT_1, auxVoltage);
}

int main(void)
{
    patch.Init();

    // Initialize Logging
    // Don't block waiting for host - allows normal boot without serial monitor
    logging::Init(false);
    LOGI("DuoPulse v4 boot");
    LOGI("Build: %s %s", __DATE__, __TIME__);

    // Initialize Audio
    patch.SetAudioBlockSize(32);
    patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);
    // NOTE: patch.AudioSampleRate() returns 0 before StartAudio()!
    // Use hardcoded constant matching SAI_32KHZ instead.
    constexpr float sampleRate = 32000.0f;

    // === Load Config from Flash ===
    currentConfig.Init();  // Initialize with defaults
    configLoaded = LoadConfigFromFlash(currentConfig);

    if(configLoaded)
    {
        LOGI("Config loaded from flash (CRC valid)");
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
        
        // Clock division: Map stored value to knob position (center of each range)
        // Centered mapping: ×1 at 42-58% (knob center)
        if(clockDivision == 8) controlState.clockDivKnob = 0.07f;        // ÷8 (0-14%)
        else if(clockDivision == 4) controlState.clockDivKnob = 0.21f;   // ÷4 (14-28%)
        else if(clockDivision == 2) controlState.clockDivKnob = 0.35f;   // ÷2 (28-42%)
        else if(clockDivision == 1) controlState.clockDivKnob = 0.50f;   // ×1 (42-58%) ← CENTER
        else if(clockDivision == -2) controlState.clockDivKnob = 0.65f;  // ×2 (58-72%)
        else if(clockDivision == -4) controlState.clockDivKnob = 0.79f;  // ×4 (72-86%)
        else controlState.clockDivKnob = 0.93f;                          // ×8 (86-100%)
        
        controlState.auxDensity = static_cast<float>(auxDensity) / 3.0f;
        controlState.voiceCoupling = static_cast<float>(voiceCoupling) / 2.0f;
        controlState.genre = static_cast<float>(genre) / 2.0f;
    }
    else
    {
        LOGI("No valid config in flash, using defaults");
    }

    // Initialize auto-save state
    autoSaveState.Init(sampleRate);
    autoSaveState.lastSaved = currentConfig;

    // Initialize Sequencer
    sequencer.Init(sampleRate);

    // Log tempo information for verification
    LOGI("Clock: 120 BPM, 8 Hz (16th notes), Pattern: 32 steps = 8 beats = 4s loop");
    LOGI("Sample rate: %d Hz, Block size: 32", static_cast<int>(sampleRate));

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

    LOGI("Initialization complete, starting audio");
    patch.StartAudio(AudioCallback);

    while(1)
    {
        uint32_t now = System::GetNow();

        ProcessControls();

        // Gate event logging disabled to prevent USB blocking freeze
        // When USB serial disconnects, LOGD blocks forever causing system hang
        // Uncomment for debugging, but expect freeze on Ctrl-C
#if 0
        GateEvent evt;
        if (gateEventBuffer.Pop(evt)) {
            const char* gateName = (evt.gateType == 0) ? "Anchor" : "Shimmer";
            LOGD("[%lu] GATE%d (%s) ON @ step %d",
                 static_cast<unsigned long>(evt.timestamp),
                 evt.gateType + 1,
                 gateName,
                 evt.step);
        }
#endif

        // Handle deferred flash write (safe here, outside audio callback)
        if(deferredSave.pending)
        {
            SaveConfigToFlash(deferredSave.configToSave);
            autoSaveState.lastSaved = deferredSave.configToSave;
            deferredSave.pending = false;
            // Note: Removed LOGD here to avoid USB blocking on disconnect
        }

        // External clock monitoring (main loop, not audio callback)
        // If Gate In 1 goes high, reset timeout. If low for 5 seconds, disable external clock.
        bool gateIn1High = patch.gate_in_1.State();
        if (gateIn1High)
        {
            lastExternalClockTime = now;
            if (!wasExternalClockActive)
            {
                LOGI("External clock detected");
                wasExternalClockActive = true;
            }
        }
        else if (wasExternalClockActive && (now - lastExternalClockTime) >= kExternalClockTimeout)
        {
            // Gate In 1 has been low for 5 seconds, disable external clock
            sequencer.DisableExternalClock();
            LOGI("External clock timeout - restored internal clock");
            wasExternalClockActive = false;
        }

        // Periodic status logging for debugging (every 5 seconds)
        if ((now - lastStatusLogTime) >= kStatusLogInterval)
        {
            lastStatusLogTime = now;
            int clockDiv = MapClockDivision(controlState.clockDivKnob);
            const char* clockMode = "";
            if (clockDiv < 0) clockMode = "MULTIPLY";
            else if (clockDiv > 1) clockMode = "DIVIDE";
            else clockMode = "1:1";

            LOGI("STATUS: BPM=%d ClockDiv=%d(%s) ExtClock=%s Energy=%d%% FieldX=%d%% FieldY=%d%%",
                 static_cast<int>(sequencer.GetBpm()),
                 clockDiv,
                 clockMode,
                 wasExternalClockActive ? "ACTIVE" : "internal",
                 static_cast<int>(controlState.energy * 100),
                 static_cast<int>(controlState.fieldX * 100),
                 static_cast<int>(controlState.fieldY * 100));
        }

        // Debug: Log pattern info on bar boundaries (deferred from audio callback)
        int currentBar = sequencer.GetCurrentBar();
        if (currentBar != lastLoggedBar)
        {
            lastLoggedBar = currentBar;
            LOGI("PATTERN: bar=%d anc=0x%08X shm=0x%08X w0=%d w4=%d w8=%d",
                 currentBar,
                 sequencer.GetAnchorMask(),
                 sequencer.GetShimmerMask(),
                 static_cast<int>(sequencer.GetBlendedAnchorWeight(0) * 100),
                 static_cast<int>(sequencer.GetBlendedAnchorWeight(4) * 100),
                 static_cast<int>(sequencer.GetBlendedAnchorWeight(8) * 100));
        }

        System::Delay(1);
    }
}
