/**
 * Unit tests for Control Processing (Phase 8)
 *
 * Tests:
 * - CV modulation processing and clamping
 * - Fill input gate detection
 * - Mode switching
 * - Button gestures (tap, hold, double-tap, live fill)
 *
 * Reference: docs/specs/main.md sections 3.2, 3.3, 4.6, 11.4
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Engine/ControlUtils.h"
#include "Engine/ControlState.h"
#include "Engine/ControlProcessor.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// Helper function for tests - wraps ProcessFillInputRaw for FillInputState
static void ProcessFillInput(float rawFillCV, bool prevGateHigh,
                              FillInputState& outState)
{
    ProcessFillInputRaw(rawFillCV, prevGateHigh,
                        outState.gateHigh, outState.triggered, outState.intensity);
}

// =============================================================================
// CV Modulation Tests
// =============================================================================

TEST_CASE("CV Modulation Processing", "[ControlUtils][CV]")
{
    SECTION("ProcessCVModulation scales to ±0.5 range")
    {
        // Full positive range: +1.0 -> +0.5
        REQUIRE(ProcessCVModulation(1.0f) == Approx(0.5f));

        // Full negative range: -1.0 -> -0.5
        REQUIRE(ProcessCVModulation(-1.0f) == Approx(-0.5f));

        // Center: 0.0 -> 0.0
        REQUIRE(ProcessCVModulation(0.0f) == Approx(0.0f));

        // Half positive: 0.5 -> 0.25
        REQUIRE(ProcessCVModulation(0.5f) == Approx(0.25f));

        // Half negative: -0.5 -> -0.25
        REQUIRE(ProcessCVModulation(-0.5f) == Approx(-0.25f));
    }

    SECTION("ProcessCVModulation clamps out-of-range values")
    {
        // Values > 1.0 should clamp to 0.5
        REQUIRE(ProcessCVModulation(2.0f) == Approx(0.5f));
        REQUIRE(ProcessCVModulation(10.0f) == Approx(0.5f));

        // Values < -1.0 should clamp to -0.5
        REQUIRE(ProcessCVModulation(-2.0f) == Approx(-0.5f));
        REQUIRE(ProcessCVModulation(-10.0f) == Approx(-0.5f));
    }

    SECTION("MixControl combines knob and CV with clamping")
    {
        // Normal case: 0.5 + 0.0 = 0.5
        REQUIRE(MixControl(0.5f, 0.0f) == Approx(0.5f));

        // Positive modulation: 0.5 + 0.25 = 0.75
        REQUIRE(MixControl(0.5f, 0.25f) == Approx(0.75f));

        // Negative modulation: 0.5 - 0.25 = 0.25
        REQUIRE(MixControl(0.5f, -0.25f) == Approx(0.25f));

        // Clamp at 1.0: 0.8 + 0.5 = 1.0 (clamped)
        REQUIRE(MixControl(0.8f, 0.5f) == Approx(1.0f));

        // Clamp at 0.0: 0.2 - 0.5 = 0.0 (clamped)
        REQUIRE(MixControl(0.2f, -0.5f) == Approx(0.0f));
    }
}

// =============================================================================
// Flavor CV Tests
// =============================================================================

TEST_CASE("Flavor CV Processing", "[ControlUtils][CV]")
{
    SECTION("ProcessFlavorCV passes through 0-1 range")
    {
        REQUIRE(ProcessFlavorCV(0.0f) == Approx(0.0f));
        REQUIRE(ProcessFlavorCV(0.5f) == Approx(0.5f));
        REQUIRE(ProcessFlavorCV(1.0f) == Approx(1.0f));
    }

    SECTION("ProcessFlavorCV clamps out-of-range values")
    {
        REQUIRE(ProcessFlavorCV(-0.5f) == Approx(0.0f));
        REQUIRE(ProcessFlavorCV(1.5f) == Approx(1.0f));
    }
}

// =============================================================================
// Fill Input Tests
// =============================================================================

TEST_CASE("Fill Input Gate Detection", "[ControlUtils][Fill]")
{
    FillInputState state;
    state.Init();

    SECTION("Gate detection with threshold")
    {
        // Below threshold: gate low
        ProcessFillInput(0.1f, false, state);
        REQUIRE(state.gateHigh == false);
        REQUIRE(state.triggered == false);

        // Above threshold (0.2 = 1V): gate high
        ProcessFillInput(0.3f, false, state);
        REQUIRE(state.gateHigh == true);
        REQUIRE(state.triggered == true);  // Rising edge
    }

    SECTION("Rising edge detection")
    {
        // First call, gate goes high
        ProcessFillInput(0.5f, false, state);
        REQUIRE(state.triggered == true);

        // Second call, gate stays high - no new trigger
        bool prevGate = state.gateHigh;
        ProcessFillInput(0.5f, prevGate, state);
        REQUIRE(state.triggered == false);
    }

    SECTION("Intensity tracks CV level")
    {
        ProcessFillInput(0.0f, false, state);
        REQUIRE(state.intensity == Approx(0.0f));

        ProcessFillInput(0.5f, false, state);
        REQUIRE(state.intensity == Approx(0.5f));

        ProcessFillInput(1.0f, false, state);
        REQUIRE(state.intensity == Approx(1.0f));
    }

    SECTION("Hysteresis prevents oscillation")
    {
        // Go above threshold
        ProcessFillInput(0.25f, false, state);
        REQUIRE(state.gateHigh == true);

        // Drop slightly below threshold but within hysteresis
        bool prevGate = state.gateHigh;
        ProcessFillInput(0.18f, prevGate, state);
        REQUIRE(state.gateHigh == true);  // Should stay high due to hysteresis

        // Drop well below threshold
        prevGate = state.gateHigh;
        ProcessFillInput(0.1f, prevGate, state);
        REQUIRE(state.gateHigh == false);  // Now goes low
    }
}

// =============================================================================
// Discrete Parameter Quantization Tests
// =============================================================================

TEST_CASE("Discrete Parameter Quantization", "[ControlUtils][Quantize]")
{
    SECTION("QuantizePatternLength")
    {
        REQUIRE(QuantizePatternLength(0.0f) == 16);
        REQUIRE(QuantizePatternLength(0.1f) == 16);
        REQUIRE(QuantizePatternLength(0.24f) == 16);
        REQUIRE(QuantizePatternLength(0.25f) == 24);
        REQUIRE(QuantizePatternLength(0.4f) == 24);
        REQUIRE(QuantizePatternLength(0.5f) == 32);
        REQUIRE(QuantizePatternLength(0.6f) == 32);
        REQUIRE(QuantizePatternLength(0.75f) == 64);
        REQUIRE(QuantizePatternLength(1.0f) == 64);
    }

    SECTION("QuantizePhraseLength")
    {
        REQUIRE(QuantizePhraseLength(0.0f) == 1);
        REQUIRE(QuantizePhraseLength(0.24f) == 1);
        REQUIRE(QuantizePhraseLength(0.25f) == 2);
        REQUIRE(QuantizePhraseLength(0.5f) == 4);
        REQUIRE(QuantizePhraseLength(0.75f) == 8);
        REQUIRE(QuantizePhraseLength(1.0f) == 8);
    }

    SECTION("QuantizeClockDivision")
    {
        REQUIRE(QuantizeClockDivision(0.0f) == 1);
        REQUIRE(QuantizeClockDivision(0.25f) == 2);
        REQUIRE(QuantizeClockDivision(0.5f) == 4);
        REQUIRE(QuantizeClockDivision(0.75f) == 8);
    }
}

// =============================================================================
// Clock Division Mapping Tests (Modification 0.5 - Bug Fix)
// =============================================================================

TEST_CASE("MapClockDivision centered at 1:1", "[ControlUtils][ClockDiv][regression]")
{
    // This test documents the bug fix from Modification 0.5:
    // Clock division was previously centered incorrectly (×1 at 30-40%).
    // Now ×1 is centered at 42-58% per spec Test 7F.

    SECTION("×1 (no change) in center range 42-58%")
    {
        // The center of the knob should produce 1:1 (no division/multiplication)
        REQUIRE(MapClockDivision(0.42f) == 1);
        REQUIRE(MapClockDivision(0.50f) == 1);  // Center of knob
        REQUIRE(MapClockDivision(0.57f) == 1);
    }

    SECTION("Division (slower) on left side")
    {
        // Left of center = divide (slower playback)
        REQUIRE(MapClockDivision(0.0f) == 8);   // ÷8 (slowest)
        REQUIRE(MapClockDivision(0.07f) == 8);
        REQUIRE(MapClockDivision(0.14f) == 4);  // ÷4
        REQUIRE(MapClockDivision(0.21f) == 4);
        REQUIRE(MapClockDivision(0.28f) == 2);  // ÷2
        REQUIRE(MapClockDivision(0.35f) == 2);
    }

    SECTION("Multiplication (faster) on right side")
    {
        // Right of center = multiply (faster playback)
        REQUIRE(MapClockDivision(0.58f) == -2);  // ×2
        REQUIRE(MapClockDivision(0.65f) == -2);
        REQUIRE(MapClockDivision(0.72f) == -4);  // ×4
        REQUIRE(MapClockDivision(0.79f) == -4);
        REQUIRE(MapClockDivision(0.86f) == -8);  // ×8 (fastest)
        REQUIRE(MapClockDivision(1.0f) == -8);
    }

    SECTION("Symmetry around center")
    {
        // Verify approximately symmetric ranges around 0.5
        // ÷8 range: 0.00-0.14 (14%)
        // ÷4 range: 0.14-0.28 (14%)
        // ÷2 range: 0.28-0.42 (14%)
        // ×1 range: 0.42-0.58 (16%) - slightly wider for stability
        // ×2 range: 0.58-0.72 (14%)
        // ×4 range: 0.72-0.86 (14%)
        // ×8 range: 0.86-1.00 (14%)

        // Boundaries
        REQUIRE(MapClockDivision(0.13f) == 8);   // Just under ÷4 threshold
        REQUIRE(MapClockDivision(0.14f) == 4);   // At ÷4 threshold
        REQUIRE(MapClockDivision(0.27f) == 4);   // Just under ÷2 threshold
        REQUIRE(MapClockDivision(0.28f) == 2);   // At ÷2 threshold
        REQUIRE(MapClockDivision(0.41f) == 2);   // Just under ×1 threshold
        REQUIRE(MapClockDivision(0.42f) == 1);   // At ×1 threshold
        REQUIRE(MapClockDivision(0.57f) == 1);   // Just under ×2 threshold
        REQUIRE(MapClockDivision(0.58f) == -2);  // At ×2 threshold
    }
}

// =============================================================================
// Button Gesture Tests
// =============================================================================

TEST_CASE("Button Gesture Detection", "[ControlProcessor][Button]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // V5 Task 32: ProcessButtonGestures now takes switch state parameters
    // Use fixed switch position (false) and matching prevSwitchUp to avoid AUX gesture
    const bool switchUp = false;
    const bool prevSwitchUp = false;

    SECTION("Tap detection (< 200ms press)")
    {
        // Press button
        processor.ProcessButtonGestures(true, switchUp, prevSwitchUp, 0, false, state.auxMode);
        REQUIRE(processor.GetButtonState().pressed == true);
        REQUIRE(processor.GetButtonState().tapDetected == false);

        // Hold for 100ms (still below tap threshold)
        processor.ProcessButtonGestures(true, switchUp, switchUp, 100, false, state.auxMode);
        REQUIRE(processor.GetButtonState().shiftActive == false);

        // Release
        processor.ProcessButtonGestures(false, switchUp, switchUp, 150, false, state.auxMode);
        REQUIRE(processor.GetButtonState().tapDetected == true);
        REQUIRE(processor.GetButtonState().pressDurationMs == 150);
    }

    SECTION("Hold detection (> 200ms press)")
    {
        // Press button
        processor.ProcessButtonGestures(true, switchUp, prevSwitchUp, 0, false, state.auxMode);

        // Hold for 250ms
        processor.ProcessButtonGestures(true, switchUp, switchUp, 250, false, state.auxMode);
        REQUIRE(processor.GetButtonState().shiftActive == true);

        // Release
        processor.ProcessButtonGestures(false, switchUp, switchUp, 300, false, state.auxMode);
        REQUIRE(processor.GetButtonState().shiftActive == false);
        REQUIRE(processor.GetButtonState().tapDetected == false);  // Too long for tap
    }

    SECTION("Live fill mode (> 500ms, no knob moved)")
    {
        // Press button
        processor.ProcessButtonGestures(true, switchUp, prevSwitchUp, 0, false, state.auxMode);

        // Hold for 500ms without moving knobs
        processor.ProcessButtonGestures(true, switchUp, switchUp, 500, false, state.auxMode);
        REQUIRE(processor.GetButtonState().liveFillActive == true);

        // Release
        processor.ProcessButtonGestures(false, switchUp, switchUp, 600, false, state.auxMode);
        REQUIRE(processor.GetButtonState().liveFillActive == false);
    }

    SECTION("Live fill cancelled by knob movement")
    {
        // Press button
        processor.ProcessButtonGestures(true, switchUp, prevSwitchUp, 0, false, state.auxMode);

        // Move a knob at 300ms
        processor.ProcessButtonGestures(true, switchUp, switchUp, 300, true, state.auxMode);

        // Hold for 600ms total
        processor.ProcessButtonGestures(true, switchUp, switchUp, 600, false, state.auxMode);
        REQUIRE(processor.GetButtonState().liveFillActive == false);  // Should NOT be active
    }

    SECTION("Double-tap detection")
    {
        // First tap
        processor.ProcessButtonGestures(true, switchUp, prevSwitchUp, 0, false, state.auxMode);
        processor.ProcessButtonGestures(false, switchUp, switchUp, 100, false, state.auxMode);
        REQUIRE(processor.GetButtonState().tapDetected == true);
        REQUIRE(processor.GetButtonState().doubleTapDetected == false);

        // Clear tap flag on next process
        processor.ProcessButtonGestures(false, switchUp, switchUp, 150, false, state.auxMode);
        REQUIRE(processor.GetButtonState().tapDetected == false);

        // Second tap within window (< 400ms from first release)
        processor.ProcessButtonGestures(true, switchUp, switchUp, 200, false, state.auxMode);
        processor.ProcessButtonGestures(false, switchUp, switchUp, 300, false, state.auxMode);
        REQUIRE(processor.GetButtonState().doubleTapDetected == true);
    }

    SECTION("Double-tap window expires")
    {
        // First tap
        processor.ProcessButtonGestures(true, switchUp, prevSwitchUp, 0, false, state.auxMode);
        processor.ProcessButtonGestures(false, switchUp, switchUp, 100, false, state.auxMode);

        // Wait for window to expire
        processor.ProcessButtonGestures(false, switchUp, switchUp, 600, false, state.auxMode);  // > 400ms later

        // Second tap should be a new tap, not double-tap
        processor.ProcessButtonGestures(true, switchUp, switchUp, 700, false, state.auxMode);
        processor.ProcessButtonGestures(false, switchUp, switchUp, 800, false, state.auxMode);
        REQUIRE(processor.GetButtonState().doubleTapDetected == false);
        REQUIRE(processor.GetButtonState().tapDetected == true);
    }
}

// =============================================================================
// Mode Switching Tests
// =============================================================================

TEST_CASE("Mode Switching", "[ControlProcessor][Mode]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    RawHardwareInput input;
    input.Init();

    SECTION("Mode switch from Performance to Config")
    {
        // Start in performance mode
        input.modeSwitch = true;
        processor.ProcessControls(input, state, 0.0f);
        REQUIRE(processor.GetModeState().performanceMode == true);

        // Switch to config mode
        input.modeSwitch = false;
        processor.ProcessControls(input, state, 0.0f);
        REQUIRE(processor.GetModeState().performanceMode == false);
    }

    SECTION("Shift toggle within mode")
    {
        input.modeSwitch = true;
        processor.ProcessControls(input, state, 0.0f);

        // Activate shift by setting buttonPressed and time in input, then calling ProcessControls
        // ProcessControls internally calls ProcessButtonGestures with input values
        input.buttonPressed = true;
        input.currentTimeMs = 0;
        processor.ProcessControls(input, state, 0.0f);  // Button press start

        input.currentTimeMs = 250;  // Hold > 200ms
        processor.ProcessControls(input, state, 0.0f);  // Shift activates
        REQUIRE(processor.GetButtonState().shiftActive == true);
        REQUIRE(processor.GetModeState().shiftActive == true);

        // Release shift
        input.buttonPressed = false;
        input.currentTimeMs = 300;
        processor.ProcessControls(input, state, 0.0f);
        REQUIRE(processor.GetModeState().shiftActive == false);
    }
}

// =============================================================================
// Control State Integration Tests
// =============================================================================

TEST_CASE("Control State Updates", "[ControlProcessor][Integration]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    RawHardwareInput input;
    input.Init();

    SECTION("CV modulation affects effective values")
    {
        // Set base energy to 0.5
        state.energy = 0.5f;

        // Apply positive CV modulation
        input.cvInputs[0] = 0.5f;  // +2.5V equivalent
        processor.ProcessControls(input, state, 0.0f);

        // energyCV should be +0.25
        REQUIRE(state.energyCV == Approx(0.25f));

        // Effective energy should be 0.75 (0.5 base + 0.25 CV)
        REQUIRE(state.GetEffectiveEnergy() == Approx(0.75f));
    }

    SECTION("Effective values are clamped")
    {
        state.energy = 0.9f;
        input.cvInputs[0] = 1.0f;  // Full +5V
        processor.ProcessControls(input, state, 0.0f);

        // Should clamp to 1.0
        REQUIRE(state.GetEffectiveEnergy() == Approx(1.0f));
    }

    SECTION("Fill input updates through control processing")
    {
        // Trigger a fill via CV
        input.fillCV = 0.5f;  // Above threshold
        processor.ProcessControls(input, state, 0.0f);

        REQUIRE(state.fillInput.gateHigh == true);
        REQUIRE(state.fillInput.intensity == Approx(0.5f));
    }

    SECTION("Flavor CV updates through control processing")
    {
        input.flavorCV = 0.75f;
        processor.ProcessControls(input, state, 0.0f);

        REQUIRE(state.flavorCV == Approx(0.75f));
    }

    SECTION("Derived parameters update")
    {
        // Set energy to put us in BUILD zone
        state.energy = 0.6f;
        processor.ProcessControls(input, state, 0.0f);

        REQUIRE(state.energyZone == EnergyZone::BUILD);
    }

    SECTION("Phrase progress affects build modifiers")
    {
        // Test BuildModifiers directly (integration with ControlProcessor
        // is tested via UpdateDerived)
        BuildModifiers early;
        BuildModifiers late;

        // High build value
        float build = 0.8f;

        // At phrase start (progress = 0)
        early.ComputeFromBuild(build, 0.0f);
        REQUIRE(early.inFillZone == false);
        REQUIRE(early.densityMultiplier == Approx(1.0f));

        // At phrase end (in fill zone, progress = 0.9)
        late.ComputeFromBuild(build, 0.9f);
        REQUIRE(late.inFillZone == true);
        // With build > 0 and progress > 0, density should increase
        REQUIRE(late.densityMultiplier > early.densityMultiplier);
    }
}

// =============================================================================
// Parameter Change Detection Tests
// =============================================================================

TEST_CASE("Parameter Change Detection", "[ControlProcessor][Flash]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    SECTION("Discrete parameter changes trigger flash")
    {
        // This test would require simulating knob movement across thresholds
        // For now, verify the flag mechanism exists
        REQUIRE(processor.ShouldFlashParameterChange() == false);
    }
}

// =============================================================================
// Reseed and Fill Queue Tests
// =============================================================================

TEST_CASE("Reseed and Fill Queue", "[ControlProcessor]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // V5 Task 32: ProcessButtonGestures now takes switch state parameters
    const bool switchUp = false;

    SECTION("Tap queues fill")
    {
        // Simulate tap
        processor.ProcessButtonGestures(true, switchUp, switchUp, 0, false, state.auxMode);
        processor.ProcessButtonGestures(false, switchUp, switchUp, 100, false, state.auxMode);

        REQUIRE(processor.FillQueued() == true);
    }

    SECTION("Double-tap requests reseed")
    {
        // First tap
        processor.ProcessButtonGestures(true, switchUp, switchUp, 0, false, state.auxMode);
        processor.ProcessButtonGestures(false, switchUp, switchUp, 100, false, state.auxMode);

        // Second tap within window
        processor.ProcessButtonGestures(true, switchUp, switchUp, 200, false, state.auxMode);
        processor.ProcessButtonGestures(false, switchUp, switchUp, 300, false, state.auxMode);

        REQUIRE(processor.ReseedRequested() == true);
    }
}
