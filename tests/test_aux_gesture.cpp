/**
 * Unit tests for V5 Hold+Switch AUX Mode Gesture (Task 32)
 *
 * Tests:
 * - Gesture detection (switch while button held)
 * - Switch consumption (doesn't change Perf/Config mode)
 * - Fill cancellation during gesture
 * - Button release after gesture (no fill triggered)
 * - Boot default is FILL_GATE
 * - HAT mode set on switch UP
 * - FILL_GATE mode set on switch DOWN
 *
 * Reference: docs/tasks/active/32-v5-aux-gesture.md
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Engine/ControlProcessor.h"
#include "Engine/ControlState.h"
#include "Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;

// =============================================================================
// Boot Default Tests
// =============================================================================

TEST_CASE("AUX mode boot default is FILL_GATE", "[AuxGesture][Boot]")
{
    ControlState state;
    state.Init();

    REQUIRE(state.auxMode == AuxMode::FILL_GATE);
}

// =============================================================================
// Gesture Detection Tests
// =============================================================================

TEST_CASE("Hold+Switch UP sets HAT mode", "[AuxGesture][Detection]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // Verify initial state is FILL_GATE
    REQUIRE(state.auxMode == AuxMode::FILL_GATE);

    // Press button (switch is DOWN = Config mode)
    bool switchConsumed = processor.ProcessButtonGestures(
        true, false, false, 0, false, state.auxMode);
    REQUIRE(switchConsumed == false);
    REQUIRE(processor.GetButtonState().auxGestureActive == false);

    // Move switch UP while button held
    switchConsumed = processor.ProcessButtonGestures(
        true, true, false, 100, false, state.auxMode);

    // Gesture should be detected
    REQUIRE(switchConsumed == true);
    REQUIRE(processor.GetButtonState().auxGestureActive == true);
    REQUIRE(processor.GetButtonState().switchMovedWhileHeld == true);

    // AUX mode should be HAT
    REQUIRE(state.auxMode == AuxMode::HAT);
}

TEST_CASE("Hold+Switch DOWN sets FILL_GATE mode", "[AuxGesture][Detection]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // Set initial mode to HAT (to test switching back)
    state.auxMode = AuxMode::HAT;

    // Press button (switch is UP = Perf mode)
    bool switchConsumed = processor.ProcessButtonGestures(
        true, true, true, 0, false, state.auxMode);
    REQUIRE(switchConsumed == false);

    // Move switch DOWN while button held
    switchConsumed = processor.ProcessButtonGestures(
        true, false, true, 100, false, state.auxMode);

    // Gesture should be detected
    REQUIRE(switchConsumed == true);
    REQUIRE(processor.GetButtonState().auxGestureActive == true);

    // AUX mode should be FILL_GATE
    REQUIRE(state.auxMode == AuxMode::FILL_GATE);
}

// =============================================================================
// Switch Consumption Tests
// =============================================================================

TEST_CASE("Switch event consumed by AUX gesture", "[AuxGesture][Consumption]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    RawHardwareInput input;
    input.Init();

    // Start in Config mode (switch DOWN)
    input.modeSwitch = false;
    processor.ProcessControls(input, state, 0.0f);
    REQUIRE(processor.GetModeState().performanceMode == false);

    // Press button (simulated via ProcessControls)
    input.buttonPressed = true;
    input.currentTimeMs = 100;
    processor.ProcessControls(input, state, 0.0f);

    // Move switch UP while button held - should NOT change to Perf mode
    input.modeSwitch = true;
    input.currentTimeMs = 200;
    processor.ProcessControls(input, state, 0.0f);

    // Mode should NOT have changed (switch was consumed by AUX gesture)
    REQUIRE(processor.GetModeState().performanceMode == false);

    // But AUX mode should be HAT
    REQUIRE(state.auxMode == AuxMode::HAT);
}

TEST_CASE("Switch without button press changes mode normally", "[AuxGesture][Normal]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    RawHardwareInput input;
    input.Init();

    // Start in Perf mode (switch UP)
    input.modeSwitch = true;
    processor.ProcessControls(input, state, 0.0f);
    REQUIRE(processor.GetModeState().performanceMode == true);

    // Switch to Config mode (switch DOWN) without button press
    input.modeSwitch = false;
    input.currentTimeMs = 100;
    processor.ProcessControls(input, state, 0.0f);

    // Mode should have changed normally
    REQUIRE(processor.GetModeState().performanceMode == false);
}

// =============================================================================
// Fill Cancellation Tests
// =============================================================================

TEST_CASE("AUX gesture cancels pending fill", "[AuxGesture][Fill]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // Press button
    bool switchConsumed = processor.ProcessButtonGestures(
        true, false, false, 0, false, state.auxMode);

    // Wait for live fill threshold (500ms)
    switchConsumed = processor.ProcessButtonGestures(
        true, false, false, 600, false, state.auxMode);
    REQUIRE(processor.GetButtonState().liveFillActive == true);

    // Move switch while still holding - should cancel live fill
    switchConsumed = processor.ProcessButtonGestures(
        true, true, false, 700, false, state.auxMode);

    REQUIRE(processor.GetButtonState().liveFillActive == false);
    REQUIRE(processor.GetButtonState().auxGestureActive == true);
}

TEST_CASE("Button release after AUX gesture does not trigger fill", "[AuxGesture][Release]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // Press button briefly (would normally be a tap)
    bool switchConsumed = processor.ProcessButtonGestures(
        true, false, false, 0, false, state.auxMode);

    // Move switch while held
    switchConsumed = processor.ProcessButtonGestures(
        true, true, false, 50, false, state.auxMode);
    REQUIRE(processor.GetButtonState().auxGestureActive == true);

    // Release button
    switchConsumed = processor.ProcessButtonGestures(
        false, true, true, 100, false, state.auxMode);

    // Should NOT trigger tap (fill)
    REQUIRE(processor.GetButtonState().tapDetected == false);
    REQUIRE(processor.GetButtonState().doubleTapDetected == false);

    // Gesture state should be reset
    REQUIRE(processor.GetButtonState().auxGestureActive == false);
    REQUIRE(processor.GetButtonState().switchMovedWhileHeld == false);
}

TEST_CASE("Normal tap still works when no AUX gesture", "[AuxGesture][NormalTap]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // Press button (no switch movement)
    bool switchConsumed = processor.ProcessButtonGestures(
        true, false, false, 0, false, state.auxMode);
    REQUIRE(switchConsumed == false);

    // Release quickly (tap)
    switchConsumed = processor.ProcessButtonGestures(
        false, false, false, 100, false, state.auxMode);

    // Should trigger tap
    REQUIRE(processor.GetButtonState().tapDetected == true);
    REQUIRE(processor.GetButtonState().auxGestureActive == false);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("Multiple switch toggles during hold", "[AuxGesture][Edge]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // Press button
    processor.ProcessButtonGestures(true, false, false, 0, false, state.auxMode);

    // Move switch UP
    processor.ProcessButtonGestures(true, true, false, 100, false, state.auxMode);
    REQUIRE(state.auxMode == AuxMode::HAT);

    // Move switch DOWN
    processor.ProcessButtonGestures(true, false, true, 200, false, state.auxMode);
    REQUIRE(state.auxMode == AuxMode::FILL_GATE);

    // Move switch UP again
    processor.ProcessButtonGestures(true, true, false, 300, false, state.auxMode);
    REQUIRE(state.auxMode == AuxMode::HAT);

    // Release - should not trigger fill
    processor.ProcessButtonGestures(false, true, true, 400, false, state.auxMode);
    REQUIRE(processor.GetButtonState().tapDetected == false);
}

TEST_CASE("Button pressed after switch already moved", "[AuxGesture][Edge]")
{
    ControlProcessor processor;
    ControlState state;
    state.Init();
    processor.Init(state);

    // Switch is already in UP position, then press button
    // (This should NOT trigger the gesture because switch didn't move during hold)
    bool switchConsumed = processor.ProcessButtonGestures(
        true, true, true, 0, false, state.auxMode);
    REQUIRE(switchConsumed == false);
    REQUIRE(processor.GetButtonState().auxGestureActive == false);

    // Release quickly
    switchConsumed = processor.ProcessButtonGestures(
        false, true, true, 100, false, state.auxMode);

    // Should trigger normal tap
    REQUIRE(processor.GetButtonState().tapDetected == true);
}

TEST_CASE("AUX gesture works in both Perf and Config mode", "[AuxGesture][Modes]")
{
    SECTION("Starting in Perf mode (switch UP)")
    {
        ControlProcessor processor;
        ControlState state;
        state.Init();
        processor.Init(state);

        // Press button while in Perf mode
        processor.ProcessButtonGestures(true, true, true, 0, false, state.auxMode);

        // Move switch DOWN
        bool switchConsumed = processor.ProcessButtonGestures(
            true, false, true, 100, false, state.auxMode);

        REQUIRE(switchConsumed == true);
        REQUIRE(state.auxMode == AuxMode::FILL_GATE);
    }

    SECTION("Starting in Config mode (switch DOWN)")
    {
        ControlProcessor processor;
        ControlState state;
        state.Init();
        processor.Init(state);

        // Press button while in Config mode
        processor.ProcessButtonGestures(true, false, false, 0, false, state.auxMode);

        // Move switch UP
        bool switchConsumed = processor.ProcessButtonGestures(
            true, true, false, 100, false, state.auxMode);

        REQUIRE(switchConsumed == true);
        REQUIRE(state.auxMode == AuxMode::HAT);
    }
}
