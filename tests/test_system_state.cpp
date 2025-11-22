/**
 * Unit Tests for SystemState logic (Phase 1)
 */

#include <catch2/catch_all.hpp>
#include "../src/System/SystemState.h"

TEST_CASE("SystemState: LED Blinking Logic", "[system][phase1]")
{
    SystemState system;
    uint32_t now = 1000;
    system.Init(now);

    // Initial state: LED off
    auto state = system.Process(now);
    REQUIRE(state.ledOn == false);

    // Advance time by 499ms (less than 500ms toggle)
    now += 499;
    state = system.Process(now);
    REQUIRE(state.ledOn == false);

    // Advance 1ms more (total 500ms) -> Toggle ON
    now += 1;
    state = system.Process(now);
    REQUIRE(state.ledOn == true);

    // Advance 500ms -> Toggle OFF
    now += 500;
    state = system.Process(now);
    REQUIRE(state.ledOn == false);
}

TEST_CASE("SystemState: Gate Toggling Logic", "[system][phase1]")
{
    SystemState system;
    uint32_t now = 1000;
    system.Init(now);

    // Initial state: Gate 1 Low, Gate 2 High (based on logic !gate1)
    auto state = system.Process(now);
    REQUIRE(state.gate1High == false);
    REQUIRE(state.gate2High == true);

    // Advance 999ms -> No change
    now += 999;
    state = system.Process(now);
    REQUIRE(state.gate1High == false);

    // Advance 1ms (total 1000ms) -> Swap
    now += 1;
    state = system.Process(now);
    REQUIRE(state.gate1High == true);
    REQUIRE(state.gate2High == false);

    // Advance 1000ms -> Swap back
    now += 1000;
    state = system.Process(now);
    REQUIRE(state.gate1High == false);
    REQUIRE(state.gate2High == true);
}

TEST_CASE("SystemState: CV Ramp Logic", "[system][phase1]")
{
    SystemState system;
    uint32_t now = 0;
    system.Init(now);

    // Initial state 0V
    auto state = system.Process(now);
    REQUIRE(state.cvOutputVolts == Catch::Approx(0.0f));

    // Ramp is 0V to 5V over 4000ms.
    // Slope = 5 / 4000 = 0.00125 V/ms

    // Advance 2000ms (halfway)
    now += 2000;
    state = system.Process(now);
    REQUIRE(state.cvOutputVolts == Catch::Approx(2.5f));

    // Advance 2000ms (end of period) -> Should wrap or be near 5V/0V
    // The implementation wraps `while(>= 5.0)`.
    // If exactly 4000ms passed, it might be 5.0 and then wrap to 0.0
    now += 2000;
    state = system.Process(now);
    // 2.5 + 2.5 = 5.0 -> wraps to 0.0
    REQUIRE(state.cvOutputVolts == Catch::Approx(0.0f).margin(0.001f));

    // Advance 100ms
    now += 100;
    state = system.Process(now);
    REQUIRE(state.cvOutputVolts == Catch::Approx(0.125f));
}

