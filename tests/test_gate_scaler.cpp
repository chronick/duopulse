#include <catch2/catch_all.hpp>

#include "Engine/GateScaler.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

TEST_CASE("GateScaler clamps programmed voltages to ±5 V", "[gate_scaler]")
{
    GateScaler scaler;
    scaler.SetTargetVoltage(7.0f);
    REQUIRE(scaler.GetTargetVoltage() == Approx(GateScaler::kGateVoltageLimit));

    scaler.SetTargetVoltage(-8.0f);
    REQUIRE(scaler.GetTargetVoltage() == Approx(-GateScaler::kGateVoltageLimit));
}

TEST_CASE("GateScaler renders codec samples with inverted polarity", "[gate_scaler]")
{
    GateScaler scaler;
    scaler.SetTargetVoltage(5.0f);

    SECTION("Gate low stays at 0 V equivalent")
    {
        REQUIRE(scaler.Render(0.0f) == Approx(0.0f).margin(1e-6f));
    }

    SECTION("Gate high saturates at -5/9 due to codec inversion")
    {
        float expected = -GateScaler::kGateVoltageLimit / GateScaler::kCodecMaxVoltage;
        REQUIRE(scaler.Render(1.0f) == Approx(expected).margin(1e-6f));
    }

    SECTION("Gate values above 1.0 get clamped")
    {
        float expected = -GateScaler::kGateVoltageLimit / GateScaler::kCodecMaxVoltage;
        REQUIRE(scaler.Render(2.0f) == Approx(expected).margin(1e-6f));
    }
}


