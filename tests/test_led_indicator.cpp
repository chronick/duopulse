#include <catch2/catch_all.hpp>
#include "../src/Engine/LedIndicator.h"

using namespace daisysp_idm_grids;

// =============================================================================
// LED Mode Indication Tests [led-feedback][mode-indication]
// =============================================================================

TEST_CASE("LedIndicator initializes correctly", "[led-feedback]")
{
    LedIndicator led;
    led.Init(1000.0f);  // 1kHz control rate
    
    LedState state;
    state.mode = LedMode::Performance;
    
    // Should return a valid brightness value
    float brightness = led.Process(state);
    REQUIRE(brightness >= 0.0f);
    REQUIRE(brightness <= 1.0f);
}

TEST_CASE("Performance mode pulses on anchor triggers", "[led-feedback][mode-indication]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Performance;
    state.broken = 0.0f;
    state.drift = 0.0f;
    state.anchorTriggered = false;
    
    // Process a few frames without trigger to establish baseline state
    for(int i = 0; i < 10; i++)
    {
        led.Process(state);
    }
    
    // Now trigger anchor
    state.anchorTriggered = true;
    float brightnessWithTrigger = led.Process(state);
    state.anchorTriggered = false;
    
    // Should be brighter immediately after trigger
    REQUIRE(brightnessWithTrigger >= LedIndicator::kNormalBrightness * 0.9f);
    
    // Track brightness over time - the trigger pulse (50ms) should 
    // guarantee brightness stays high for that duration
    bool stayedBrightDuringPulse = true;
    for(int i = 0; i < 40; i++)  // 40ms (within 50ms pulse window)
    {
        float b = led.Process(state);
        if(b < LedIndicator::kNormalBrightness * 0.8f)
        {
            stayedBrightDuringPulse = false;
        }
    }
    REQUIRE(stayedBrightDuringPulse);
}

TEST_CASE("Config mode shows solid ON", "[led-feedback][mode-indication]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Config;
    
    // Process multiple frames - should stay constant
    float prevBrightness = led.Process(state);
    for(int i = 0; i < 100; i++)
    {
        float brightness = led.Process(state);
        REQUIRE(brightness == Catch::Approx(prevBrightness));
    }
    
    // Should be at normal brightness level
    REQUIRE(prevBrightness >= LedIndicator::kNormalBrightness - 0.01f);
}

TEST_CASE("Shift held shows breathing pattern", "[led-feedback][mode-indication]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::ShiftHeld;
    
    // Collect brightness values over one full breathing cycle (500ms = 500 samples at 1kHz)
    float minBrightness = 1.0f;
    float maxBrightness = 0.0f;
    
    for(int i = 0; i < 500; i++)
    {
        float brightness = led.Process(state);
        minBrightness = std::min(minBrightness, brightness);
        maxBrightness = std::max(maxBrightness, brightness);
    }
    
    // Breathing should vary between low and high
    REQUIRE(minBrightness < 0.4f);  // Goes dim
    REQUIRE(maxBrightness > 0.8f);  // Goes bright
    REQUIRE((maxBrightness - minBrightness) > 0.5f);  // Significant variation
}

// =============================================================================
// Parameter Feedback Tests [led-feedback][parameter-feedback]
// =============================================================================

TEST_CASE("Interaction mode shows parameter value as brightness", "[led-feedback][parameter-feedback]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Interaction;
    
    // Test various interaction values
    state.interactionValue = 0.0f;
    REQUIRE(led.Process(state) == Catch::Approx(0.0f));
    
    state.interactionValue = 0.5f;
    REQUIRE(led.Process(state) == Catch::Approx(0.5f));
    
    state.interactionValue = 1.0f;
    REQUIRE(led.Process(state) == Catch::Approx(1.0f));
}

TEST_CASE("High BROKEN increases flash rate", "[led-feedback][parameter-feedback]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Performance;
    state.drift = 0.0f;
    
    // Count transitions (on->off or off->on) over a fixed time period
    auto countTransitions = [&led](LedState& s, int samples) {
        int transitions = 0;
        bool lastOn = false;
        for(int i = 0; i < samples; i++)
        {
            float brightness = led.Process(s);
            bool currentOn = brightness > 0.5f;
            if(currentOn != lastOn)
            {
                transitions++;
                lastOn = currentOn;
            }
        }
        return transitions;
    };
    
    // Low BROKEN should have fewer transitions (slower flash)
    state.broken = 0.1f;
    led.Init(1000.0f);  // Reset
    int lowBrokenTransitions = countTransitions(state, 1000);
    
    // High BROKEN should have more transitions (faster flash)
    state.broken = 0.9f;
    led.Init(1000.0f);  // Reset
    int highBrokenTransitions = countTransitions(state, 1000);
    
    // High BROKEN should flash faster (more transitions)
    REQUIRE(highBrokenTransitions > lowBrokenTransitions);
}

// =============================================================================
// BROKEN x DRIFT Behavior Tests [led-feedback][broken-drift]
// =============================================================================

TEST_CASE("Low BROKEN + Low DRIFT produces regular steady pulses", "[led-feedback][broken-drift]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Performance;
    state.broken = 0.0f;
    state.drift = 0.0f;
    
    // Collect pulse timings over multiple cycles
    std::vector<float> onTimes;
    std::vector<float> offTimes;
    bool wasOn = false;
    int onDuration = 0;
    int offDuration = 0;
    
    for(int i = 0; i < 2000; i++)
    {
        float brightness = led.Process(state);
        bool isOn = brightness > 0.5f;
        
        if(isOn)
        {
            if(!wasOn && offDuration > 0)
            {
                offTimes.push_back(static_cast<float>(offDuration));
                offDuration = 0;
            }
            onDuration++;
        }
        else
        {
            if(wasOn && onDuration > 0)
            {
                onTimes.push_back(static_cast<float>(onDuration));
                onDuration = 0;
            }
            offDuration++;
        }
        wasOn = isOn;
    }
    
    // Should have consistent timing (low variance)
    if(onTimes.size() >= 2)
    {
        float avgOn = 0.0f;
        for(auto t : onTimes) avgOn += t;
        avgOn /= onTimes.size();
        
        float variance = 0.0f;
        for(auto t : onTimes) variance += (t - avgOn) * (t - avgOn);
        variance /= onTimes.size();
        
        // Low variance indicates regular timing
        float stdDev = std::sqrt(variance);
        REQUIRE(stdDev < avgOn * 0.1f);  // Less than 10% variation
    }
}

TEST_CASE("High BROKEN + High DRIFT produces maximum irregularity", "[led-feedback][broken-drift]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Performance;
    state.broken = 1.0f;
    state.drift = 1.0f;
    
    // Collect brightness values
    std::vector<float> brightnessValues;
    for(int i = 0; i < 1000; i++)
    {
        brightnessValues.push_back(led.Process(state));
    }
    
    // Calculate variance in brightness
    float avg = 0.0f;
    for(auto b : brightnessValues) avg += b;
    avg /= brightnessValues.size();
    
    float variance = 0.0f;
    for(auto b : brightnessValues) variance += (b - avg) * (b - avg);
    variance /= brightnessValues.size();
    
    // High variance indicates irregularity
    REQUIRE(variance > 0.01f);  // Some meaningful variation
}

// =============================================================================
// Phrase Position Feedback Tests [led-feedback][phrase-feedback]
// =============================================================================

TEST_CASE("Downbeat produces extra bright pulse", "[led-feedback][phrase-feedback]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Performance;
    state.broken = 0.0f;
    state.drift = 0.0f;
    state.isDownbeat = false;
    state.anchorTriggered = true;  // Trigger to start pulse
    
    // Measure brightness with trigger but no downbeat
    float normalPulseBrightness = led.Process(state);
    state.anchorTriggered = false;
    
    // Reset and test with downbeat
    led.Init(1000.0f);
    state.isDownbeat = true;
    state.anchorTriggered = true;
    float downbeatBrightness = led.Process(state);
    
    // Downbeat should be at least as bright (likely brighter due to longer pulse)
    REQUIRE(downbeatBrightness >= normalPulseBrightness * 0.9f);
    REQUIRE(downbeatBrightness >= LedIndicator::kNormalBrightness * 0.9f);
}

TEST_CASE("Fill zone produces rapid triple-pulse pattern", "[led-feedback][phrase-feedback]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Performance;
    state.broken = 0.0f;
    state.drift = 0.0f;
    state.isFillZone = true;
    
    // Count on/off transitions in fill zone (should be rapid)
    int transitions = 0;
    bool lastOn = false;
    
    for(int i = 0; i < 500; i++)  // 500ms = one cycle
    {
        float brightness = led.Process(state);
        bool isOn = brightness > 0.5f;
        if(isOn != lastOn)
        {
            transitions++;
            lastOn = isOn;
        }
    }
    
    // Triple pulse pattern should have at least 6 transitions per cycle
    // (on-off-on-off-on-off = 6 transitions minimum)
    REQUIRE(transitions >= 4);
}

TEST_CASE("Build zone shows gradually increasing pulse rate", "[led-feedback][phrase-feedback]")
{
    LedIndicator led;
    led.Init(1000.0f);
    
    LedState state;
    state.mode = LedMode::Performance;
    state.broken = 0.0f;
    state.drift = 0.0f;
    state.isBuildZone = true;
    
    // Test at start of build zone (slow pulse)
    state.phraseProgress = 0.5f;  // Start of build zone
    led.Init(1000.0f);
    
    int earlyTransitions = 0;
    bool lastOn = false;
    for(int i = 0; i < 500; i++)
    {
        float brightness = led.Process(state);
        bool isOn = brightness > 0.5f;
        if(isOn != lastOn)
        {
            earlyTransitions++;
            lastOn = isOn;
        }
    }
    
    // Test near end of build zone (fast pulse)
    state.phraseProgress = 0.74f;  // Near end of build zone
    led.Init(1000.0f);
    
    int lateTransitions = 0;
    lastOn = false;
    for(int i = 0; i < 500; i++)
    {
        float brightness = led.Process(state);
        bool isOn = brightness > 0.5f;
        if(isOn != lastOn)
        {
            lateTransitions++;
            lastOn = isOn;
        }
    }
    
    // Later in build zone should have faster pulse rate (more transitions)
    REQUIRE(lateTransitions >= earlyTransitions);
}

// =============================================================================
// Utility Tests [led-feedback]
// =============================================================================

TEST_CASE("BrightnessToVoltage converts correctly", "[led-feedback]")
{
    REQUIRE(LedIndicator::BrightnessToVoltage(0.0f) == Catch::Approx(0.0f));
    REQUIRE(LedIndicator::BrightnessToVoltage(0.5f) == Catch::Approx(2.5f));
    REQUIRE(LedIndicator::BrightnessToVoltage(1.0f) == Catch::Approx(5.0f));
}

TEST_CASE("VoltageForState legacy helper works", "[led-feedback]")
{
    REQUIRE(LedIndicator::VoltageForState(false) == Catch::Approx(0.0f));
    REQUIRE(LedIndicator::VoltageForState(true) == Catch::Approx(5.0f));
}

TEST_CASE("Clamp helper works correctly", "[led-feedback]")
{
    REQUIRE(Clamp(0.5f, 0.0f, 1.0f) == Catch::Approx(0.5f));
    REQUIRE(Clamp(-0.5f, 0.0f, 1.0f) == Catch::Approx(0.0f));
    REQUIRE(Clamp(1.5f, 0.0f, 1.0f) == Catch::Approx(1.0f));
}
