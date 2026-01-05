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
      prevSwitchUp_(true),  // V5 Task 32: Default to Perf mode position
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

    // V5: Initialize performance knobs with new parameter names
    // K1: ENERGY, K2: SHAPE, K3: AXIS X, K4: AXIS Y
    perfKnobs_[0].Init(initialState.energy);
    perfKnobs_[1].Init(initialState.shape);
    perfKnobs_[2].Init(initialState.axisX);
    perfKnobs_[3].Init(initialState.axisY);

    // V5: Initialize config knobs with new parameter assignments
    // K1: CLOCK DIV, K2: SWING, K3: DRIFT, K4: ACCENT
    // Clock division: map clockDiv to normalized knob value
    float clockDivNorm = 0.625f;  // Default to x1 (center-right)
    if (initialState.clockDiv == -4)
        clockDivNorm = 0.125f;  // ÷4
    else if (initialState.clockDiv == -2)
        clockDivNorm = 0.375f;  // ÷2
    else if (initialState.clockDiv == 4)
        clockDivNorm = 0.875f;  // x4
    configKnobs_[0].Init(clockDivNorm);

    configKnobs_[1].Init(initialState.swing);
    configKnobs_[2].Init(initialState.drift);
    configKnobs_[3].Init(initialState.accent);

    prevFillGateHigh_  = false;
    prevSwitchUp_      = true;  // V5 Task 32: Default to Perf mode position
    parameterChanged_  = false;
    currentContext_    = KnobContext::PERF;
    prevContext_       = KnobContext::PERF;
}

void ControlProcessor::ProcessControls(const RawHardwareInput& input,
                                       ControlState& state, float phraseProgress)
{
    parameterChanged_ = false;

    // V5 Task 32: Process button gestures first, which may consume switch event
    bool switchConsumed = ProcessButtonGestures(input.buttonPressed, input.modeSwitch,
                                                 prevSwitchUp_, input.currentTimeMs,
                                                 AnyKnobMoved(), state.auxMode);
    prevSwitchUp_ = input.modeSwitch;

    // Update mode state (only if switch wasn't consumed by AUX gesture)
    modeState_.prevPerformanceMode = modeState_.performanceMode;
    modeState_.prevShiftActive     = modeState_.shiftActive;
    if (!switchConsumed)
    {
        modeState_.performanceMode = input.modeSwitch;
    }
    // V5: Shift layer removed, shiftActive no longer used for context switching
    modeState_.shiftActive         = buttonState_.shiftActive;

    // V5: Determine current knob context (simplified - no shift layers)
    prevContext_ = currentContext_;
    currentContext_ = modeState_.performanceMode ? KnobContext::PERF : KnobContext::CONFIG;

    // Lock knobs on context change
    if (currentContext_ != prevContext_)
    {
        LockAllKnobs();
    }

    // V5: Process controls based on current context (simplified - 2 modes only)
    if (currentContext_ == KnobContext::PERF)
    {
        ProcessPerformanceMode(input, state);
    }
    else
    {
        ProcessConfigMode(input, state);
    }

    // V5: CV modulation (always active for performance mode params)
    // CV1-4 modulate ENERGY, SHAPE, AXIS X, AXIS Y
    state.energyCV = ProcessCVModulation(input.cvInputs[0]);
    state.shapeCV  = ProcessCVModulation(input.cvInputs[1]);
    state.axisXCV  = ProcessCVModulation(input.cvInputs[2]);
    state.axisYCV  = ProcessCVModulation(input.cvInputs[3]);

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

    // Update derived parameters
    state.UpdateDerived(phraseProgress);
}

bool ControlProcessor::ProcessButtonGestures(bool pressed, bool switchUp,
                                              bool prevSwitchUp,
                                              uint32_t currentTimeMs,
                                              bool anyKnobMoved, AuxMode& auxMode)
{
    // Clear single-frame flags
    buttonState_.tapDetected       = false;
    buttonState_.doubleTapDetected = false;

    bool wasPressed = buttonState_.pressed;
    buttonState_.pressed = pressed;

    // V5 Task 32: Detect switch movement while button is held (AUX gesture)
    bool switchConsumed = false;
    if (buttonState_.pressed && (switchUp != prevSwitchUp))
    {
        // Switch moved while button held - this is the AUX gesture
        buttonState_.auxGestureActive = true;
        buttonState_.switchMovedWhileHeld = true;

        // Cancel pending operations
        buttonState_.liveFillActive = false;
        buttonState_.tapDetected = false;

        // Set AUX mode based on switch direction
        if (switchUp)
        {
            auxMode = AuxMode::HAT;
            // TODO(Task-34): Queue HAT unlock LED flash (triple rising)
        }
        else
        {
            auxMode = AuxMode::FILL_GATE;
            // TODO(Task-34): Queue FILL_GATE reset LED flash (single fade)
        }

        // Consume switch event - don't change perf/config mode
        switchConsumed = true;
    }

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
        buttonState_.auxGestureActive     = false;
        buttonState_.switchMovedWhileHeld = false;
    }

    // While held: update shift and live fill state (only if not in AUX gesture)
    if (pressed && !buttonState_.auxGestureActive)
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

        // V5 Task 32: If AUX gesture was active, don't trigger fill
        if (buttonState_.auxGestureActive)
        {
            // Reset gesture state
            buttonState_.auxGestureActive = false;
            buttonState_.switchMovedWhileHeld = false;

            // Don't trigger fill (gesture consumed the press)
            buttonState_.tapDetected = false;
            buttonState_.doubleTapDetected = false;
            buttonState_.tapCount = 0;
        }
        else
        {
            // Normal button release - detect tap (short press)
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

    return switchConsumed;
}

// V5: Process performance mode controls (no shift layer)
// K1: ENERGY, K2: SHAPE, K3: AXIS X, K4: AXIS Y
void ControlProcessor::ProcessPerformanceMode(const RawHardwareInput& input,
                                               ControlState& state)
{
    // K1: ENERGY
    state.energy = perfKnobs_[0].Process(input.knobs[0]);

    // K2: SHAPE (V5: was BUILD)
    state.shape = perfKnobs_[1].Process(input.knobs[1]);

    // K3: AXIS X (V5: was FIELD X)
    state.axisX = perfKnobs_[2].Process(input.knobs[2]);

    // K4: AXIS Y (V5: was FIELD Y)
    state.axisY = perfKnobs_[3].Process(input.knobs[3]);
}

// V5: Process config mode controls (no shift layer)
// K1: CLOCK DIV, K2: SWING, K3: DRIFT, K4: ACCENT
void ControlProcessor::ProcessConfigMode(const RawHardwareInput& input,
                                          ControlState& state)
{
    // K1: CLOCK DIV (V5: was shift+K2)
    // Maps: 0-25% = ÷4, 25-50% = ÷2, 50-75% = x1, 75-100% = x4
    float clockDivRaw = configKnobs_[0].Process(input.knobs[0]);
    int newClockDiv;
    if (clockDivRaw < 0.25f)
        newClockDiv = -4;  // ÷4
    else if (clockDivRaw < 0.50f)
        newClockDiv = -2;  // ÷2
    else if (clockDivRaw < 0.75f)
        newClockDiv = 1;   // x1
    else
        newClockDiv = 4;   // x4

    if (newClockDiv != state.clockDiv)
    {
        parameterChanged_ = true;
        state.clockDiv = newClockDiv;
        state.clockDivision = newClockDiv;  // Legacy alias
    }

    // K2: SWING (continuous)
    state.swing = configKnobs_[1].Process(input.knobs[1]);

    // K3: DRIFT (V5: was perf shift+K3)
    state.drift = configKnobs_[2].Process(input.knobs[2]);

    // K4: ACCENT (V5: was PUNCH in perf shift+K1)
    state.accent = configKnobs_[3].Process(input.knobs[3]);
}

void ControlProcessor::LockAllKnobs()
{
    // V5: Only 2 sets of knobs (no shift layers)
    for (int i = 0; i < kNumKnobs; ++i)
    {
        perfKnobs_[i].Lock();
        configKnobs_[i].Lock();
    }
}

bool ControlProcessor::AnyKnobMoved() const
{
    // V5: Check all knobs in current context (simplified - 2 contexts only)
    if (currentContext_ == KnobContext::PERF)
    {
        for (int i = 0; i < kNumKnobs; ++i)
        {
            // Note: HasMoved() resets the flag, so we need a const version
            // For now, we check if the knob is unlocked (indicates user engaged)
            if (!perfKnobs_[i].IsLocked()) return true;
        }
    }
    else
    {
        for (int i = 0; i < kNumKnobs; ++i)
        {
            if (!configKnobs_[i].IsLocked()) return true;
        }
    }
    return false;
}

bool ControlProcessor::ShouldFlashParameterChange() const
{
    return parameterChanged_;
}

}  // namespace daisysp_idm_grids
