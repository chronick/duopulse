/**
 * Example Unit Tests
 * 
 * This file demonstrates how to write unit tests for DSP algorithms.
 * Replace with your actual test cases.
 */

#include <catch2/catch_all.hpp>
#include <cmath>
#include <limits>

// Example: Test a simple math utility
namespace {
    // Simple utility function for testing
    float clamp(float value, float min_val, float max_val)
    {
        if (value < min_val) return min_val;
        if (value > max_val) return max_val;
        return value;
    }
}

TEST_CASE("Clamp function works correctly", "[math]")
{
    SECTION("Values within range")
    {
        REQUIRE(clamp(0.5f, 0.0f, 1.0f) == 0.5f);
        REQUIRE(clamp(0.0f, 0.0f, 1.0f) == 0.0f);
        REQUIRE(clamp(1.0f, 0.0f, 1.0f) == 1.0f);
    }

    SECTION("Values below minimum")
    {
        REQUIRE(clamp(-1.0f, 0.0f, 1.0f) == 0.0f);
        REQUIRE(clamp(-10.0f, 0.0f, 1.0f) == 0.0f);
    }

    SECTION("Values above maximum")
    {
        REQUIRE(clamp(2.0f, 0.0f, 1.0f) == 1.0f);
        REQUIRE(clamp(10.0f, 0.0f, 1.0f) == 1.0f);
    }
}

TEST_CASE("Audio level normalization", "[audio]")
{
    SECTION("Normalize to Eurorack range")
    {
        // Eurorack audio is typically ±5V (±1.0 normalized)
        float normalized = 0.5f;
        REQUIRE(normalized >= -1.0f);
        REQUIRE(normalized <= 1.0f);
    }

    SECTION("Handle edge cases")
    {
        REQUIRE(std::isfinite(0.0f));
        REQUIRE(std::isfinite(1.0f));
        REQUIRE(std::isfinite(-1.0f));
    }
}

TEST_CASE("CV input scaling", "[cv]")
{
    SECTION("Scale CV input (0-5V) to normalized (0-1)")
    {
        float cv_voltage = 2.5f; // 2.5V
        float normalized = cv_voltage / 5.0f;
        REQUIRE(normalized == 0.5f);
    }

    SECTION("Scale bipolar CV (-5V to +5V) to normalized (-1 to +1)")
    {
        float cv_voltage = 0.0f; // 0V
        float normalized = cv_voltage / 5.0f;
        REQUIRE(normalized == 0.0f);

        cv_voltage = 2.5f; // +2.5V
        normalized = cv_voltage / 5.0f;
        REQUIRE(normalized == 0.5f);

        cv_voltage = -2.5f; // -2.5V
        normalized = cv_voltage / 5.0f;
        REQUIRE(normalized == -0.5f);
    }
}

// Add more test cases for your DSP modules here

