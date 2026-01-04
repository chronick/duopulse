#pragma once

#include <cstdint>
#include "ControlState.h"
#include "ControlUtils.h"
#include "SoftKnob.h"

namespace daisysp_idm_grids
{

// =============================================================================
// Constants
// =============================================================================

/// Number of knobs on the Patch.Init
constexpr int kNumKnobs = 4;

/// Number of CV inputs on the Patch.Init
constexpr int kNumCVInputs = 4;

/// Button timing constants (in milliseconds)
constexpr uint32_t kTapMaxMs = 200;           ///< Max duration for tap gesture
constexpr uint32_t kHoldThresholdMs = 200;    ///< Min duration for hold gesture
constexpr uint32_t kLiveFillThresholdMs = 500; ///< Min duration for live fill mode
constexpr uint32_t kDoubleTapWindowMs = 400;  ///< Max gap between taps for double-tap

// =============================================================================
// Button Gesture State
// =============================================================================

/**
 * ButtonState: Tracks button input and gesture detection
 *
 * Supports tap, hold, live fill, and double-tap gestures.
 * Reference: docs/specs/main.md section 4.6
 */
struct ButtonState
{
    /// Current physical button state
    bool pressed;

    /// Time when button was pressed (ms since startup)
    uint32_t pressTimeMs;

    /// Time when button was released (ms since startup)
    uint32_t releaseTimeMs;

    /// Duration of last press (ms)
    uint32_t pressDurationMs;

    /// Whether shift is currently active (button held > threshold)
    bool shiftActive;

    /// Whether live fill mode is active (button held > fill threshold, no knob moved)
    bool liveFillActive;

    /// Whether a knob was moved during this press
    bool knobMovedDuringPress;

    /// Whether a tap was detected this frame (rising edge)
    bool tapDetected;

    /// Whether a double-tap was detected this frame
    bool doubleTapDetected;

    /// Counter for pending double-tap detection
    uint8_t tapCount;

    /**
     * Initialize button state
     */
    void Init()
    {
        pressed              = false;
        pressTimeMs          = 0;
        releaseTimeMs        = 0;
        pressDurationMs      = 0;
        shiftActive          = false;
        liveFillActive       = false;
        knobMovedDuringPress = false;
        tapDetected          = false;
        doubleTapDetected    = false;
        tapCount             = 0;
    }
};

// =============================================================================
// Mode State
// =============================================================================

/**
 * ModeState: Tracks current mode and shift state
 */
struct ModeState
{
    /// Performance mode (true) vs Config mode (false)
    bool performanceMode;

    /// Shift modifier active (button held)
    bool shiftActive;

    /// Previous shift state (for edge detection)
    bool prevShiftActive;

    /// Previous mode (for edge detection)
    bool prevPerformanceMode;

    /**
     * Initialize mode state
     */
    void Init()
    {
        performanceMode     = true;
        shiftActive         = false;
        prevShiftActive     = false;
        prevPerformanceMode = true;
    }
};

// =============================================================================
// Raw Hardware Input
// =============================================================================

/**
 * RawHardwareInput: Raw values from hardware (before processing)
 *
 * All values are normalized 0.0-1.0 (knobs, CVs) or boolean (gates).
 */
struct RawHardwareInput
{
    /// Knob positions (0.0-1.0)
    float knobs[kNumKnobs];

    /// CV input values (-1.0 to +1.0 for bipolar, 0.0-1.0 for unipolar)
    float cvInputs[kNumCVInputs];

    /// Fill CV input (0.0-1.0, from Audio In L)
    float fillCV;

    /// Flavor CV input (0.0-1.0, from Audio In R)
    float flavorCV;

    /// Button pressed state
    bool buttonPressed;

    /// Mode switch position (true = Performance/A, false = Config/B)
    bool modeSwitch;

    /// Current time in milliseconds (for gesture timing)
    uint32_t currentTimeMs;

    /**
     * Initialize with default values
     */
    void Init()
    {
        for (int i = 0; i < kNumKnobs; ++i)
            knobs[i] = 0.5f;
        for (int i = 0; i < kNumCVInputs; ++i)
            cvInputs[i] = 0.0f;
        fillCV        = 0.0f;
        flavorCV      = 0.0f;
        buttonPressed = false;
        modeSwitch    = true;
        currentTimeMs = 0;
    }
};

// =============================================================================
// Control Processor
// =============================================================================

/**
 * ControlProcessor: Processes all control inputs into ControlState
 *
 * Handles:
 * - Soft takeover for knobs across mode/shift changes
 * - CV modulation
 * - Button gesture detection
 * - Mode switching
 * - Fill input processing
 *
 * Reference: docs/specs/main.md section 11.4
 */
class ControlProcessor
{
  public:
    ControlProcessor();

    /**
     * Initialize the control processor
     *
     * @param initialState Initial control state to use
     */
    void Init(const ControlState& initialState);

    /**
     * Process all controls and update control state
     *
     * @param input Raw hardware input values
     * @param state Control state to update
     * @param phraseProgress Current phrase progress (0.0-1.0) for build modifiers
     */
    void ProcessControls(const RawHardwareInput& input, ControlState& state,
                         float phraseProgress);

    /**
     * Process button gestures and update button state
     *
     * @param pressed Current button pressed state
     * @param currentTimeMs Current time in milliseconds
     * @param anyKnobMoved Whether any knob was moved this frame
     */
    void ProcessButtonGestures(bool pressed, uint32_t currentTimeMs,
                               bool anyKnobMoved);

    /**
     * Check if a parameter change flash should be shown
     *
     * @return true if a discrete parameter changed this frame
     */
    bool ShouldFlashParameterChange() const;

    /**
     * Get button state (for LED feedback, etc.)
     */
    const ButtonState& GetButtonState() const { return buttonState_; }

    /**
     * Get mode state (for LED feedback, etc.)
     */
    const ModeState& GetModeState() const { return modeState_; }

    /**
     * Check if a reseed was requested (double-tap detected)
     */
    bool ReseedRequested() const { return buttonState_.doubleTapDetected; }

    /**
     * Check if a fill was queued (tap detected)
     */
    bool FillQueued() const { return buttonState_.tapDetected; }

  private:
    /**
     * Process performance mode primary controls (no shift)
     */
    void ProcessPerformancePrimary(const RawHardwareInput& input,
                                   ControlState& state);

    /**
     * Process performance mode shift controls
     */
    void ProcessPerformanceShift(const RawHardwareInput& input,
                                 ControlState& state);

    /**
     * Process config mode primary controls (no shift)
     */
    void ProcessConfigPrimary(const RawHardwareInput& input,
                              ControlState& state);

    /**
     * Process config mode shift controls
     */
    void ProcessConfigShift(const RawHardwareInput& input, ControlState& state);

    /**
     * Lock all knobs for a mode/shift change
     */
    void LockAllKnobs();

    /**
     * Check if any soft knob was moved
     */
    bool AnyKnobMoved() const;

    // Soft knobs for each parameter context:
    // Performance primary: ENERGY, BUILD, FIELD X, FIELD Y
    SoftKnob perfPrimaryKnobs_[kNumKnobs];

    // Performance shift: PUNCH, GENRE, DRIFT, BALANCE
    SoftKnob perfShiftKnobs_[kNumKnobs];

    // Config primary: PATTERN LENGTH, SWING, AUX MODE, RESET MODE
    SoftKnob configPrimaryKnobs_[kNumKnobs];

    // Config shift: PHRASE LENGTH, CLOCK DIV, AUX DENSITY, VOICE COUPLING
    SoftKnob configShiftKnobs_[kNumKnobs];

    // Button and mode state
    ButtonState buttonState_;
    ModeState modeState_;

    // Previous fill gate state (for rising edge detection)
    bool prevFillGateHigh_;

    // Flag for parameter change flash
    bool parameterChanged_;

    // Track which knob set is currently active
    enum class KnobContext
    {
        PERF_PRIMARY,
        PERF_SHIFT,
        CONFIG_PRIMARY,
        CONFIG_SHIFT
    };
    KnobContext currentContext_;
    KnobContext prevContext_;
};

} // namespace daisysp_idm_grids
