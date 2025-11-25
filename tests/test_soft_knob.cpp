#include <catch2/catch_test_macros.hpp>
#include "Engine/SoftKnob.h"

using namespace daisysp_idm_grids;

TEST_CASE("SoftKnob Basic Operation", "[SoftKnob]") {
    SoftKnob knob;
    knob.Init(0.5f);

    SECTION("Initial State") {
        REQUIRE(knob.GetValue() == 0.5f);
        REQUIRE(knob.IsLocked() == true);
    }

    SECTION("Pickup Logic") {
        // Knob far away
        knob.Process(0.1f);
        REQUIRE(knob.GetValue() == 0.5f);
        REQUIRE(knob.IsLocked() == true);

        // Knob moves closer but not enough
        knob.Process(0.4f);
        REQUIRE(knob.GetValue() == 0.5f);
        REQUIRE(knob.IsLocked() == true);

        // Knob crosses threshold (0.5 +/- 0.05)
        // 0.52 is within threshold
        knob.Process(0.52f);
        REQUIRE(knob.GetValue() == 0.52f);
        REQUIRE(knob.IsLocked() == false);

        // Now tracks
        knob.Process(0.8f);
        REQUIRE(knob.GetValue() == 0.8f);
    }
    
    SECTION("Mode Switching Simulation") {
        // Mode A: Value 0.2
        knob.Init(0.2f);
        
        // User moves knob to 0.8 (while in Mode B technically, but let's say we switch back and read 0.8)
        
        // Switch to Mode A (simulated by Init/Lock state)
        // HW reads 0.8
        knob.Process(0.8f);
        REQUIRE(knob.GetValue() == 0.2f); // Still at stored value
        REQUIRE(knob.IsLocked() == true);
        
        // User moves knob down towards 0.2
        knob.Process(0.5f);
        REQUIRE(knob.GetValue() == 0.2f);
        
        knob.Process(0.22f); // Close enough
        REQUIRE(knob.GetValue() == 0.22f);
        REQUIRE(knob.IsLocked() == false);
        
        // Now tracking
        knob.Process(0.1f);
        REQUIRE(knob.GetValue() == 0.1f);
    }

    SECTION("Locking behavior") {
        knob.Init(0.5f);
        knob.Process(0.51f);
        REQUIRE(knob.IsLocked() == false);
        
        knob.Lock();
        REQUIRE(knob.IsLocked() == true);
        knob.Process(0.8f); // Jump
        REQUIRE(knob.GetValue() == 0.51f); // Should hold previous value
    }
}

