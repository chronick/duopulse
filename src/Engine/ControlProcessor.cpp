#include "ControlProcessor.h"
#include "ControlState.h"

namespace daisysp_idm_grids
{

// Helper function to update FillInputState using the raw function
static void ProcessFillInput(float rawFillCV, bool prevGateHigh,
                              FillInputState& outState)
{
    ProcessFillInputRaw(rawFillCV, prevGateHigh,
                        outState.gateHigh, outState.triggered, outState.intensity);
}

ControlProcessor::ControlProcessor()
    : prevFillGateHigh_(false),
      parameterChanged_(false),
      currentContext_(KnobContext::PERF_PRIMARY),
      prevContext_(KnobContext::PERF_PRIMARY)
{
    buttonState_.Init();
    modeState_.Init();
}

void ControlProcessor::Init(const ControlState& initialState)
{
    // Initialize button and mode state
    buttonState_.Init();
    modeState_.Init();

    // Initialize performance primary knobs with current values
    perfPrimaryKnobs_[0].Init(initialState.energy);
    perfPrimaryKnobs_[1].Init(initialState.build);
    perfPrimaryKnobs_[2].Init(initialState.fieldX);
    perfPrimaryKnobs_[3].Init(initialState.fieldY);

    // Initialize performance shift knobs
    perfShiftKnobs_[0].Init(initialState.punch);
    // Genre is discrete (0-1 mapped to enum), init to middle
    perfShiftKnobs_[1].Init(static_cast<float>(initialState.genre) / 2.0f);
    perfShiftKnobs_[2].Init(initialState.drift);
    perfShiftKnobs_[3].Init(initialState.balance);

    // Initialize config primary knobs (discrete values need special handling)
    // Pattern length: 16/24/32/64 -> 0.125/0.375/0.625/0.875
    float patternLengthNorm = 0.625f;  // Default to 32
    if (initialState.patternLength == 16)
        patternLengthNorm = 0.125f;
    else if (initialState.patternLength == 24)
        patternLengthNorm = 0.375f;
    else if (initialState.patternLength == 64)
        patternLengthNorm = 0.875f;
    configPrimaryKnobs_[0].Init(patternLengthNorm);

    configPrimaryKnobs_[1].Init(initialState.swing);

    // AUX mode: discrete (0-3 -> 0.125/0.375/0.625/0.875)
    configPrimaryKnobs_[2].Init(
        (static_cast<float>(initialState.auxMode) + 0.5f) / 4.0f);

    // K4: FREE - Reset mode no longer exposed in UI, init to 0.0
    configPrimaryKnobs_[3].Init(0.0f);

    // Initialize config shift knobs
    // Phrase length: 1/2/4/8 -> 0.125/0.375/0.625/0.875
    float phraseLengthNorm = 0.625f;  // Default to 4
    if (initialState.phraseLength == 1)
        phraseLengthNorm = 0.125f;
    else if (initialState.phraseLength == 2)
        phraseLengthNorm = 0.375f;
    else if (initialState.phraseLength == 8)
        phraseLengthNorm = 0.875f;
    configShiftKnobs_[0].Init(phraseLengthNorm);

    // Clock division: 1/2/4/8 -> same as phrase length
    float clockDivNorm = 0.125f;  // Default to 1
    if (initialState.clockDivision == 2)
        clockDivNorm = 0.375f;
    else if (initialState.clockDivision == 4)
        clockDivNorm = 0.625f;
    else if (initialState.clockDivision == 8)
        clockDivNorm = 0.875f;
    configShiftKnobs_[1].Init(clockDivNorm);

    // AUX density: discrete (0-3)
    configShiftKnobs_[2].Init(
        (static_cast<float>(initialState.auxDensity) + 0.5f) / 4.0f);

    // Voice coupling: discrete (0-2)
    configShiftKnobs_[3].Init(
        (static_cast<float>(initialState.voiceCoupling) + 0.5f) / 3.0f);

    prevFillGateHigh_  = false;
    parameterChanged_  = false;
    currentContext_    = KnobContext::PERF_PRIMARY;
    prevContext_       = KnobContext::PERF_PRIMARY;
}

void ControlProcessor::ProcessControls(const RawHardwareInput& input,
                                       ControlState& state, float phraseProgress)
{
    parameterChanged_ = false;

    // Update mode state
    modeState_.prevPerformanceMode = modeState_.performanceMode;
    modeState_.prevShiftActive     = modeState_.shiftActive;
    modeState_.performanceMode     = input.modeSwitch;
    modeState_.shiftActive         = buttonState_.shiftActive;

    // Determine current knob context
    prevContext_ = currentContext_;
    if (modeState_.performanceMode)
    {
        currentContext_ = modeState_.shiftActive ? KnobContext::PERF_SHIFT
                                                 : KnobContext::PERF_PRIMARY;
    }
    else
    {
        currentContext_ = modeState_.shiftActive ? KnobContext::CONFIG_SHIFT
                                                 : KnobContext::CONFIG_PRIMARY;
    }

    // Lock knobs on context change
    if (currentContext_ != prevContext_)
    {
        LockAllKnobs();
    }

    // Process controls based on current context
    switch (currentContext_)
    {
        case KnobContext::PERF_PRIMARY:
            ProcessPerformancePrimary(input, state);
            break;
        case KnobContext::PERF_SHIFT:
            ProcessPerformanceShift(input, state);
            break;
        case KnobContext::CONFIG_PRIMARY:
            ProcessConfigPrimary(input, state);
            break;
        case KnobContext::CONFIG_SHIFT:
            ProcessConfigShift(input, state);
            break;
    }

    // Process CV modulation (always active for performance primary params)
    state.energyCV = ProcessCVModulation(input.cvInputs[0]);
    state.buildCV  = ProcessCVModulation(input.cvInputs[1]);
    state.fieldXCV = ProcessCVModulation(input.cvInputs[2]);
    state.fieldYCV = ProcessCVModulation(input.cvInputs[3]);

    // Process flavor CV
    state.flavorCV = ProcessFlavorCV(input.flavorCV);

    // Process fill input
    bool prevGate = prevFillGateHigh_;
    ProcessFillInput(input.fillCV, prevGate, state.fillInput);
    prevFillGateHigh_ = state.fillInput.gateHigh;

    // Update fill queue from button tap
    if (buttonState_.tapDetected)
    {
        state.fillInput.fillQueued = true;
    }

    // Update live fill mode from button state
    state.fillInput.liveFillMode = buttonState_.liveFillActive;

    // Process button gestures
    ProcessButtonGestures(input.buttonPressed, input.currentTimeMs,
                          AnyKnobMoved());

    // Update derived parameters
    state.UpdateDerived(phraseProgress);
}

void ControlProcessor::ProcessButtonGestures(bool pressed, uint32_t currentTimeMs,
                                             bool anyKnobMoved)
{
    // Clear single-frame flags
    buttonState_.tapDetected       = false;
    buttonState_.doubleTapDetected = false;

    bool wasPressed = buttonState_.pressed;
    buttonState_.pressed = pressed;

    // Track knob movement during press
    if (pressed && anyKnobMoved)
    {
        buttonState_.knobMovedDuringPress = true;
    }

    // Rising edge: button just pressed
    if (pressed && !wasPressed)
    {
        buttonState_.pressTimeMs          = currentTimeMs;
        buttonState_.knobMovedDuringPress = false;
        buttonState_.liveFillActive       = false;
    }

    // While held: update shift and live fill state
    if (pressed)
    {
        uint32_t holdDuration = currentTimeMs - buttonState_.pressTimeMs;

        // Shift becomes active after hold threshold
        buttonState_.shiftActive = (holdDuration >= kHoldThresholdMs);

        // Live fill mode: held long enough AND no knob moved
        if (holdDuration >= kLiveFillThresholdMs &&
            !buttonState_.knobMovedDuringPress)
        {
            buttonState_.liveFillActive = true;
        }
    }

    // Falling edge: button just released
    if (!pressed && wasPressed)
    {
        buttonState_.releaseTimeMs   = currentTimeMs;
        buttonState_.pressDurationMs = currentTimeMs - buttonState_.pressTimeMs;
        buttonState_.shiftActive     = false;
        buttonState_.liveFillActive  = false;

        // Detect tap (short press)
        if (buttonState_.pressDurationMs < kTapMaxMs)
        {
            // Check for double-tap
            uint32_t timeSinceLastTap =
                currentTimeMs - buttonState_.releaseTimeMs;
            if (buttonState_.tapCount > 0 &&
                timeSinceLastTap < kDoubleTapWindowMs)
            {
                buttonState_.doubleTapDetected = true;
                buttonState_.tapCount          = 0;
            }
            else
            {
                buttonState_.tapDetected = true;
                buttonState_.tapCount    = 1;
            }
        }
        else
        {
            buttonState_.tapCount = 0;
        }
    }

    // Clear tap count if double-tap window expired
    if (!pressed && buttonState_.tapCount > 0)
    {
        uint32_t timeSinceRelease = currentTimeMs - buttonState_.releaseTimeMs;
        if (timeSinceRelease > kDoubleTapWindowMs)
        {
            buttonState_.tapCount = 0;
        }
    }
}

void ControlProcessor::ProcessPerformancePrimary(const RawHardwareInput& input,
                                                  ControlState& state)
{
    // K1: ENERGY
    state.energy = perfPrimaryKnobs_[0].Process(input.knobs[0]);

    // K2: BUILD
    state.build = perfPrimaryKnobs_[1].Process(input.knobs[1]);

    // K3: FIELD X
    state.fieldX = perfPrimaryKnobs_[2].Process(input.knobs[2]);

    // K4: FIELD Y
    state.fieldY = perfPrimaryKnobs_[3].Process(input.knobs[3]);
}

void ControlProcessor::ProcessPerformanceShift(const RawHardwareInput& input,
                                                ControlState& state)
{
    // Shift+K1: PUNCH
    state.punch = perfShiftKnobs_[0].Process(input.knobs[0]);

    // Shift+K2: GENRE (discrete)
    float genreRaw = perfShiftKnobs_[1].Process(input.knobs[1]);
    Genre newGenre = GetGenreFromValue(genreRaw);
    if (newGenre != state.genre)
    {
        parameterChanged_ = true;
        state.genre       = newGenre;
    }

    // Shift+K3: DRIFT
    state.drift = perfShiftKnobs_[2].Process(input.knobs[2]);

    // Shift+K4: BALANCE
    state.balance = perfShiftKnobs_[3].Process(input.knobs[3]);
}

void ControlProcessor::ProcessConfigPrimary(const RawHardwareInput& input,
                                             ControlState& state)
{
    // K1: PATTERN LENGTH (discrete)
    float patternRaw = configPrimaryKnobs_[0].Process(input.knobs[0]);
    int newPatternLength = QuantizePatternLength(patternRaw);
    if (newPatternLength != state.patternLength)
    {
        parameterChanged_    = true;
        state.patternLength = newPatternLength;
    }

    // K2: SWING (continuous)
    state.swing = configPrimaryKnobs_[1].Process(input.knobs[1]);

    // K3: AUX MODE (discrete)
    float auxModeRaw   = configPrimaryKnobs_[2].Process(input.knobs[2]);
    AuxMode newAuxMode = GetAuxModeFromValue(auxModeRaw);
    if (newAuxMode != state.auxMode)
    {
        parameterChanged_ = true;
        state.auxMode     = newAuxMode;
    }

    // K4: FREE - Reset mode hardcoded to STEP in ControlState::Init()
    // Config K4 primary is now available for future features
    (void)input.knobs[3];  // Mark parameter as intentionally unused
}

void ControlProcessor::ProcessConfigShift(const RawHardwareInput& input,
                                           ControlState& state)
{
    // Shift+K1: FREE - Phrase length is now auto-derived from pattern length
    // See ControlState::GetDerivedPhraseLength()
    // Config+Shift K1 is now available for future features
    (void)input.knobs[0];  // Mark parameter as intentionally unused

    // Shift+K2: CLOCK DIV (discrete)
    float clockDivRaw = configShiftKnobs_[1].Process(input.knobs[1]);
    int newClockDiv   = QuantizeClockDivision(clockDivRaw);
    if (newClockDiv != state.clockDivision)
    {
        parameterChanged_    = true;
        state.clockDivision = newClockDiv;
    }

    // Shift+K3: AUX DENSITY (discrete)
    float auxDensityRaw     = configShiftKnobs_[2].Process(input.knobs[2]);
    AuxDensity newAuxDensity = GetAuxDensityFromValue(auxDensityRaw);
    if (newAuxDensity != state.auxDensity)
    {
        parameterChanged_  = true;
        state.auxDensity  = newAuxDensity;
    }

    // Shift+K4: VOICE COUPLING (discrete)
    float couplingRaw            = configShiftKnobs_[3].Process(input.knobs[3]);
    VoiceCoupling newVoiceCoupling = GetVoiceCouplingFromValue(couplingRaw);
    if (newVoiceCoupling != state.voiceCoupling)
    {
        parameterChanged_    = true;
        state.voiceCoupling = newVoiceCoupling;
    }
}

void ControlProcessor::LockAllKnobs()
{
    for (int i = 0; i < kNumKnobs; ++i)
    {
        perfPrimaryKnobs_[i].Lock();
        perfShiftKnobs_[i].Lock();
        configPrimaryKnobs_[i].Lock();
        configShiftKnobs_[i].Lock();
    }
}

bool ControlProcessor::AnyKnobMoved() const
{
    // Check all knobs in current context
    switch (currentContext_)
    {
        case KnobContext::PERF_PRIMARY:
            for (int i = 0; i < kNumKnobs; ++i)
            {
                // Note: HasMoved() resets the flag, so we need a const version
                // For now, we check if the knob is unlocked (indicates user
                // engaged)
                if (!perfPrimaryKnobs_[i].IsLocked()) return true;
            }
            break;
        case KnobContext::PERF_SHIFT:
            for (int i = 0; i < kNumKnobs; ++i)
            {
                if (!perfShiftKnobs_[i].IsLocked()) return true;
            }
            break;
        case KnobContext::CONFIG_PRIMARY:
            for (int i = 0; i < kNumKnobs; ++i)
            {
                if (!configPrimaryKnobs_[i].IsLocked()) return true;
            }
            break;
        case KnobContext::CONFIG_SHIFT:
            for (int i = 0; i < kNumKnobs; ++i)
            {
                if (!configShiftKnobs_[i].IsLocked()) return true;
            }
            break;
    }
    return false;
}

bool ControlProcessor::ShouldFlashParameterChange() const
{
    return parameterChanged_;
}

}  // namespace daisysp_idm_grids
