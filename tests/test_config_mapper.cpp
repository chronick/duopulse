#include <catch2/catch_all.hpp>

#include "Engine/ConfigMapper.h"

using namespace daisysp_idm_grids;

TEST_CASE("ConfigMapper converts normalized values to codec voltages",
          "[config_mapper]")
{
    REQUIRE(ConfigMapper::NormalizedToVoltage(0.0f)
            == Catch::Approx(-GateScaler::kGateVoltageLimit));
    REQUIRE(ConfigMapper::NormalizedToVoltage(1.0f)
            == Catch::Approx(GateScaler::kGateVoltageLimit));
    REQUIRE(ConfigMapper::NormalizedToVoltage(0.5f) == Catch::Approx(0.0f));
}

TEST_CASE("ConfigMapper clamps voltages and durations", "[config_mapper]")
{
    REQUIRE(ConfigMapper::NormalizedToVoltage(-1.0f)
            == Catch::Approx(-GateScaler::kGateVoltageLimit));
    REQUIRE(ConfigMapper::NormalizedToVoltage(2.0f)
            == Catch::Approx(GateScaler::kGateVoltageLimit));

    REQUIRE(ConfigMapper::NormalizedToHoldMs(0.0f)
            == Catch::Approx(ConfigMapper::kMinGateHoldMs));
    REQUIRE(ConfigMapper::NormalizedToHoldMs(1.0f)
            == Catch::Approx(ConfigMapper::kMaxGateHoldMs));
    const float mid = (ConfigMapper::kMaxGateHoldMs + ConfigMapper::kMinGateHoldMs)
                      * 0.5f;
    REQUIRE(ConfigMapper::NormalizedToHoldMs(0.5f) == Catch::Approx(mid));
}


