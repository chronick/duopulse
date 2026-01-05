/**
 * Unit tests for V5 LED Layer System (Task 34)
 *
 * Tests:
 * - LedLayer enum values
 * - LedLayerState struct initialization
 * - SetLayer/ClearLayer functionality
 * - ComputeFinalBrightness priority logic
 * - Layer expiration
 * - UpdateBreath animation
 * - UpdateTriggerDecay animation
 * - UpdateFillStrobe animation
 * - TriggerFlash convenience method
 *
 * Reference: docs/tasks/active/34-v5-led-layers.md
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "Engine/LedIndicator.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// LedLayer Enum Tests
// =============================================================================

TEST_CASE("LedLayer enum values are correct", "[LedLayers][Enum]")
{
    REQUIRE(static_cast<uint8_t>(LedLayer::BASE) == 0);
    REQUIRE(static_cast<uint8_t>(LedLayer::TRIGGER) == 1);
    REQUIRE(static_cast<uint8_t>(LedLayer::FILL) == 2);
    REQUIRE(static_cast<uint8_t>(LedLayer::FLASH_EVT) == 3);
    REQUIRE(static_cast<uint8_t>(LedLayer::REPLACE) == 4);
}

TEST_CASE("kNumLedLayers is 5", "[LedLayers][Constant]")
{
    REQUIRE(kNumLedLayers == 5);
}

// =============================================================================
// LedLayerState Struct Tests
// =============================================================================

TEST_CASE("LedLayerState default initialization", "[LedLayers][Struct]")
{
    LedLayerState state;

    REQUIRE(state.brightness == Approx(0.0f));
    REQUIRE(state.expiresAtMs == 0);
    REQUIRE(state.active == false);
}

// =============================================================================
// SetLayer/ClearLayer Tests
// =============================================================================

TEST_CASE("SetLayer activates a layer with brightness", "[LedLayers][SetLayer]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::BASE, 0.5f);
    float brightness = led.ComputeFinalBrightness();

    REQUIRE(brightness == Approx(0.5f));
}

TEST_CASE("SetLayer clamps brightness to 0-1 range", "[LedLayers][SetLayer]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Over 1.0
    led.SetLayer(LedLayer::BASE, 1.5f);
    REQUIRE(led.ComputeFinalBrightness() == Approx(1.0f));

    // Under 0.0
    led.SetLayer(LedLayer::BASE, -0.5f);
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.0f));
}

TEST_CASE("ClearLayer deactivates a layer", "[LedLayers][ClearLayer]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::TRIGGER, 0.8f);
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.8f));

    led.ClearLayer(LedLayer::TRIGGER);
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.0f));
}

TEST_CASE("Multiple layers can be set independently", "[LedLayers][SetLayer]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::BASE, 0.3f);
    led.SetLayer(LedLayer::TRIGGER, 0.6f);

    // TRIGGER (higher priority) should win
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.6f));
}

// =============================================================================
// ComputeFinalBrightness Priority Tests
// =============================================================================

TEST_CASE("Higher priority layer overrides lower", "[LedLayers][Priority]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::BASE, 0.2f);
    led.SetLayer(LedLayer::FLASH_EVT, 1.0f);

    // FLASH (priority 3) overrides BASE (priority 0)
    REQUIRE(led.ComputeFinalBrightness() == Approx(1.0f));
}

TEST_CASE("REPLACE layer has highest priority", "[LedLayers][Priority]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::BASE, 0.2f);
    led.SetLayer(LedLayer::TRIGGER, 0.5f);
    led.SetLayer(LedLayer::FILL, 0.7f);
    led.SetLayer(LedLayer::FLASH_EVT, 1.0f);
    led.SetLayer(LedLayer::REPLACE, 0.1f);

    // REPLACE should win even with low brightness
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.1f));
}

TEST_CASE("Inactive layers are ignored in priority", "[LedLayers][Priority]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::BASE, 0.3f);
    led.SetLayer(LedLayer::FLASH_EVT, 1.0f);
    led.ClearLayer(LedLayer::FLASH_EVT);

    // BASE should now be active since FLASH is cleared
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.3f));
}

TEST_CASE("No active layers returns 0 brightness", "[LedLayers][Priority]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // No layers set
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.0f));
}

// =============================================================================
// Layer Expiration Tests
// =============================================================================

TEST_CASE("Layer with duration expires after time", "[LedLayers][Expiration]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Set FLASH layer with 100ms duration
    led.SetLayer(LedLayer::FLASH_EVT, 1.0f, 100);

    // Immediately should be active
    REQUIRE(led.ComputeFinalBrightness() == Approx(1.0f));

    // Advance time past expiration (using UpdateBreath to advance currentTimeMs_)
    // 150ms should be past the 100ms expiration
    led.UpdateBreath(150.0f);

    // Layer should have expired
    REQUIRE(led.ComputeFinalBrightness() < 1.0f);
}

TEST_CASE("Layer with zero duration never expires", "[LedLayers][Expiration]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Set layer with no duration (permanent)
    led.SetLayer(LedLayer::BASE, 0.5f, 0);

    // Advance significant time
    led.UpdateBreath(10000.0f);

    // Should still be at BASE brightness (UpdateBreath sets BASE, so check it's not 0)
    REQUIRE(led.ComputeFinalBrightness() > 0.0f);
}

// =============================================================================
// UpdateBreath Tests
// =============================================================================

TEST_CASE("UpdateBreath sets BASE layer with breathing pattern", "[LedLayers][Breath]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Collect brightness values over one breathing cycle (500ms)
    float minBrightness = 1.0f;
    float maxBrightness = 0.0f;

    for(int i = 0; i < 500; i++)
    {
        led.UpdateBreath(1.0f);  // 1ms per call
        float brightness = led.ComputeFinalBrightness();
        minBrightness = std::min(minBrightness, brightness);
        maxBrightness = std::max(maxBrightness, brightness);
    }

    // Breathing should vary between low and high
    REQUIRE(minBrightness < 0.4f);
    REQUIRE(maxBrightness > 0.8f);
}

// =============================================================================
// UpdateTriggerDecay Tests
// =============================================================================

TEST_CASE("UpdateTriggerDecay decays TRIGGER layer brightness", "[LedLayers][TriggerDecay]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Set TRIGGER layer to full brightness
    led.SetLayer(LedLayer::TRIGGER, 1.0f);
    REQUIRE(led.ComputeFinalBrightness() == Approx(1.0f));

    // Decay for 25ms at default rate (0.02 per ms = 0.5 decay)
    led.UpdateTriggerDecay(25.0f);
    float brightness = led.ComputeFinalBrightness();

    REQUIRE(brightness == Approx(0.5f).margin(0.05f));
}

TEST_CASE("UpdateTriggerDecay deactivates layer when brightness reaches 0", "[LedLayers][TriggerDecay]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::TRIGGER, 0.5f);

    // Decay enough to reach 0 (0.5 / 0.02 = 25ms)
    led.UpdateTriggerDecay(30.0f);

    // Should be 0 and deactivated
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.0f));
}

TEST_CASE("UpdateTriggerDecay does nothing if layer is inactive", "[LedLayers][TriggerDecay]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // TRIGGER layer not set
    led.UpdateTriggerDecay(100.0f);

    // Should still be 0 (no crash, no side effects)
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.0f));
}

// =============================================================================
// UpdateFillStrobe Tests
// =============================================================================

TEST_CASE("UpdateFillStrobe creates alternating pattern", "[LedLayers][FillStrobe]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Run for one period and collect values
    float minBrightness = 1.0f;
    float maxBrightness = 0.0f;

    for(int i = 0; i < 100; i++)
    {
        led.UpdateFillStrobe(1.0f);  // Default 100ms period
        float brightness = led.ComputeFinalBrightness();
        minBrightness = std::min(minBrightness, brightness);
        maxBrightness = std::max(maxBrightness, brightness);
    }

    // Should alternate between high and low
    REQUIRE(minBrightness < 0.5f);
    REQUIRE(maxBrightness > 0.8f);
}

// =============================================================================
// TriggerFlash Tests
// =============================================================================

TEST_CASE("TriggerFlash sets FLASH layer at full brightness", "[LedLayers][Flash]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.TriggerFlash(100);
    REQUIRE(led.ComputeFinalBrightness() == Approx(1.0f));
}

TEST_CASE("TriggerFlash expires after duration", "[LedLayers][Flash]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.TriggerFlash(50);  // 50ms duration
    REQUIRE(led.ComputeFinalBrightness() == Approx(1.0f));

    // Advance time past expiration
    led.UpdateBreath(100.0f);

    // FLASH should have expired, only BASE active from UpdateBreath
    float brightness = led.ComputeFinalBrightness();
    REQUIRE(brightness < 1.0f);
}

TEST_CASE("TriggerFlash overrides lower layers", "[LedLayers][Flash]")
{
    LedIndicator led;
    led.Init(1000.0f);

    led.SetLayer(LedLayer::BASE, 0.2f);
    led.SetLayer(LedLayer::TRIGGER, 0.5f);
    led.TriggerFlash(100);

    // FLASH should override all
    REQUIRE(led.ComputeFinalBrightness() == Approx(1.0f));
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_CASE("Layer system coexists with existing Process() method", "[LedLayers][Integration]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Use old Process() method
    LedState state;
    state.mode = LedMode::Performance;
    state.anchorTriggered = true;

    float oldBrightness = led.Process(state);

    // Use new layer system
    led.SetLayer(LedLayer::REPLACE, 0.25f);
    float layerBrightness = led.ComputeFinalBrightness();

    // Both should work independently
    REQUIRE(oldBrightness >= 0.0f);
    REQUIRE(oldBrightness <= 1.0f);
    REQUIRE(layerBrightness == Approx(0.25f));
}

TEST_CASE("Init resets all layers", "[LedLayers][Init]")
{
    LedIndicator led;
    led.Init(1000.0f);

    // Set some layers
    led.SetLayer(LedLayer::BASE, 0.5f);
    led.SetLayer(LedLayer::FLASH_EVT, 1.0f);

    // Re-init
    led.Init(1000.0f);

    // All layers should be cleared
    REQUIRE(led.ComputeFinalBrightness() == Approx(0.0f));
}
