#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Engine/SoftKnob.h"

using namespace daisysp_idm_grids;

TEST_CASE("SoftKnob Value Scaling", "[SoftKnob]") {
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

    SECTION("Scaling Upwards") {
        // Value = 0.5, Knob = 0.1
        knob.Process(0.1f); 
        
        // Move Knob UP to 0.2 (+0.1)
        // Range Knob: 0.1 -> 1.0 (0.9)
        // Range Value: 0.5 -> 1.0 (0.5)
        // Scale: 0.5 / 0.9 = 0.555...
        
        float out = knob.Process(0.2f);
        REQUIRE(out > 0.5f);
        REQUIRE(out == Catch::Approx(0.5f + (0.1f * (0.5f/0.9f))).margin(0.0001f));
        REQUIRE(knob.IsLocked() == true);
    }

    SECTION("Scaling Downwards") {
        // Value = 0.5, Knob = 0.2
        knob.Process(0.2f);

        // Move Knob DOWN to 0.1 (-0.1)
        // Range Knob: 0.2 -> 0.0 (0.2)
        // Range Value: 0.5 -> 0.0 (0.5)
        // Scale: 0.5 / 0.2 = 2.5
        
        float out = knob.Process(0.1f);
        REQUIRE(out < 0.5f);
        REQUIRE(out == Catch::Approx(0.5f - (0.1f * 2.5f)).margin(0.0001f));
        
        // Should be unlocked if we hit limit or crossover?
        // Here we just moved part way (0.25 vs 0.1).
        REQUIRE(knob.GetValue() == 0.25f);
    }

    SECTION("Scaling Convergence") {
        // Value = 0.5, Knob = 0.4
        knob.Process(0.4f);
        
        // Move Knob towards limit. Value should scale.
        // 0.4 -> 0.6.
        // Dist_K = 0.6. Dist_V = 0.5. Scale = 0.8333...
        // Delta = 0.2. V_delta = 0.1666...
        // V_new = 0.666...
        // Raw = 0.6.
        // Gap = 0.0666... > 0.05. Still locked.
        
        float out = knob.Process(0.6f);
        REQUIRE(out == Catch::Approx(0.666666f).margin(0.001f));
        REQUIRE(knob.IsLocked() == true);
        
        // Move further to 0.8
        // Total Delta from 0.4 is 0.4. V_delta = 0.333...
        // V_new = 0.5 + 0.333 = 0.8333...
        // Raw = 0.8.
        // Gap = 0.0333... < 0.05. Unlock!
        
        out = knob.Process(0.8f);
        REQUIRE(out == 0.8f); // Snapped
        REQUIRE(knob.IsLocked() == false);
    }

    SECTION("Unlock on Match") {
        knob.Init(0.5f);
        knob.Process(0.49f); // Within pickup threshold?
        // If threshold check is still 0.05
        REQUIRE(knob.IsLocked() == false);
        REQUIRE(knob.GetValue() == 0.49f);
    }

    SECTION("Interaction Detection") {
        knob.Init(0.5f);
        knob.Process(0.1f);
        REQUIRE(knob.HasMoved() == false); // First process doesn't count as move? Or does it? 
        // Technically we haven't moved the knob yet, just established position.
        
        knob.Process(0.11f); // Move
        REQUIRE(knob.HasMoved() == true);
        REQUIRE(knob.HasMoved() == false); // Should reset
        
        knob.Process(0.11f); // No Move
        REQUIRE(knob.HasMoved() == false);
    }
}
