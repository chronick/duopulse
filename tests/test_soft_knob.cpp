#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Engine/SoftKnob.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

TEST_CASE("SoftKnob Gradual Interpolation", "[SoftKnob]") {
    SoftKnob knob;
    knob.Init(0.5f);

    SECTION("Initial State") {
        REQUIRE(knob.GetValue() == 0.5f);
        REQUIRE(knob.IsLocked() == true);
    }

    SECTION("First Process sets reference") {
        // First call just sets the raw reference, shouldn't move value
        float out = knob.Process(0.1f);
        REQUIRE(out == 0.5f);
        REQUIRE(knob.GetValue() == 0.5f);
        REQUIRE(knob.IsLocked() == true);
    }

    SECTION("Gradual Interpolation Upwards") {
        // Value = 0.5, Knob = 0.1
        knob.Process(0.1f); 
        
        // Move Knob UP to 0.2 (+0.1)
        // Gradual interpolation: move 10% toward physical position
        // Distance = 0.2 - 0.5 = -0.3
        // New value = 0.5 + (-0.3 * 0.1) = 0.5 - 0.03 = 0.47
        
        float out = knob.Process(0.2f);
        REQUIRE(out < 0.5f); // Moving toward physical position (which is below value)
        REQUIRE(out == Approx(0.47f).margin(0.01f));
        REQUIRE(knob.IsLocked() == true);
    }

    SECTION("Gradual Interpolation Downwards") {
        // Value = 0.5, Knob = 0.8
        knob.Process(0.8f);

        // Move Knob DOWN to 0.7 (-0.1)
        // Distance = 0.7 - 0.5 = 0.2
        // New value = 0.5 + (0.2 * 0.1) = 0.52
        
        float out = knob.Process(0.7f);
        REQUIRE(out > 0.5f); // Moving toward physical position (which is above value)
        REQUIRE(out == Approx(0.52f).margin(0.01f));
        REQUIRE(knob.IsLocked() == true);
    }

    SECTION("No Interpolation When Knob Stationary") {
        // Value = 0.5, start far from physical
        knob.Process(0.0f);
        
        // Keep physical at 0.0 (stationary), value should NOT change
        // This is the fix for mode-switching parameter drift
        float prev = 0.5f;
        for(int i = 0; i < 5; i++) {
            float out = knob.Process(0.0f);
            REQUIRE(out == prev); // Should stay the same when knob not moved
        }
        
        // Value should still be at 0.5 since knob was stationary
        REQUIRE(knob.GetValue() == 0.5f);
        REQUIRE(knob.IsLocked() == true);
    }

    SECTION("Convergence When Knob Is Moved") {
        // Value = 0.5, knob starts at 0.0
        knob.Process(0.0f);
        
        // Move knob slightly each cycle, value should gradually decrease
        float prev = 0.5f;
        float knobPos = 0.0f;
        for(int i = 0; i < 5; i++) {
            knobPos += 0.01f; // Small movement to trigger interpolation
            float out = knob.Process(knobPos);
            REQUIRE(out < prev); // Should decrease each time since knob is moved
            prev = out;
        }
        
        // Value should have decreased significantly
        REQUIRE(prev < 0.4f);
        REQUIRE(knob.IsLocked() == true); // Still locked, not converged yet
    }

    SECTION("Cross Detection Unlocks Immediately") {
        // Value = 0.5, Knob starts at 0.3 (below value)
        knob.Process(0.3f);
        REQUIRE(knob.IsLocked() == true);
        
        // Move knob to 0.6 (above value) - crosses the value!
        float out = knob.Process(0.6f);
        REQUIRE(knob.IsLocked() == false); // Should unlock immediately
        REQUIRE(out == 0.6f); // Should snap to physical
    }

    SECTION("Unlock on Match within Threshold") {
        knob.Init(0.5f);
        // If physical is within 2% of value, unlock immediately
        knob.Process(0.49f);
        REQUIRE(knob.IsLocked() == false);
        REQUIRE(knob.GetValue() == 0.49f);
    }

    SECTION("Interaction Detection") {
        knob.Init(0.5f);
        knob.Process(0.1f);
        REQUIRE(knob.HasMoved() == false); // First process doesn't count as move
        
        knob.Process(0.11f); // Move
        REQUIRE(knob.HasMoved() == true);
        REQUIRE(knob.HasMoved() == false); // Should reset
        
        knob.Process(0.11f); // No Move
        REQUIRE(knob.HasMoved() == false);
    }

    SECTION("Lock prevents jumps on mode switch") {
        knob.Init(0.5f);
        // Simulate: mode switch, knob physical position is 0.1
        knob.Process(0.1f);
        knob.Process(0.1f);
        
        // Value should still be close to 0.5, not jumped to 0.1
        REQUIRE(knob.GetValue() > 0.4f);
        
        // Now switch modes - relock
        knob.Lock();
        REQUIRE(knob.IsLocked() == true);
        
        // Physical position changes to 0.9 after mode switch
        knob.Process(0.9f);
        // Should not jump - value stays locked
        REQUIRE(knob.GetValue() > 0.4f);
    }

    SECTION("SetInterpolationRate changes convergence speed") {
        knob.Init(0.5f);
        knob.SetInterpolationRate(0.5f); // 50% per cycle instead of 10%
        
        knob.Process(0.0f);  // First process sets reference
        float out = knob.Process(0.01f);  // Move knob slightly to trigger interpolation
        
        // With 50% rate: 0.5 + (0.01 - 0.5) * 0.5 = 0.5 - 0.245 = 0.255
        REQUIRE(out == Approx(0.255f).margin(0.02f));
    }
}
