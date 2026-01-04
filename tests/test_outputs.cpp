#include <catch2/catch_all.hpp>

#include "../src/Engine/GateScaler.h"
#include "../src/Engine/VelocityOutput.h"
#include "../src/Engine/AuxOutput.h"
#include "../src/Engine/OutputState.h"
#include "../src/Engine/SequencerState.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// GateScaler Tests [outputs][trigger]
// =============================================================================

TEST_CASE("GateScaler initializes correctly", "[outputs][trigger]")
{
    GateScaler scaler;
    scaler.Init(48000.0f);

    REQUIRE(scaler.GetTargetVoltage() == Approx(GateScaler::kGateVoltageLimit));
}

TEST_CASE("GateScaler processes trigger state", "[outputs][trigger]")
{
    GateScaler scaler;
    scaler.Init(48000.0f);

    TriggerState trigger;
    trigger.Init(48);  // 1ms at 48kHz

    SECTION("Trigger low produces 0V equivalent")
    {
        trigger.high = false;
        float sample = scaler.ProcessTriggerOutput(trigger);
        REQUIRE(sample == Approx(0.0f).margin(1e-6f));
    }

    SECTION("Trigger high produces 5V equivalent")
    {
        trigger.high = true;
        float sample = scaler.ProcessTriggerOutput(trigger);
        float expected = -GateScaler::kGateVoltageLimit / GateScaler::kCodecMaxVoltage;
        REQUIRE(sample == Approx(expected).margin(1e-6f));
    }
}

TEST_CASE("Trigger pulse width is correct", "[outputs][trigger]")
{
    constexpr float kSampleRate = 48000.0f;
    constexpr float kTriggerMs  = 2.0f;  // 2ms trigger
    constexpr int kExpectedSamples = static_cast<int>(kTriggerMs * kSampleRate / 1000.0f);

    TriggerState trigger;
    trigger.SetDurationMs(kTriggerMs, kSampleRate);
    trigger.Init(trigger.triggerDurationSamples);

    // Fire the trigger
    trigger.Fire();
    REQUIRE(trigger.high == true);
    REQUIRE(trigger.samplesRemaining == kExpectedSamples);

    // Process until just before end
    for (int i = 0; i < kExpectedSamples - 1; ++i)
    {
        trigger.Process();
    }
    REQUIRE(trigger.high == true);
    REQUIRE(trigger.samplesRemaining == 1);

    // One more sample should turn off
    trigger.Process();
    REQUIRE(trigger.high == false);
    REQUIRE(trigger.samplesRemaining == 0);
}

TEST_CASE("TriggerState fire and process cycle", "[outputs][trigger]")
{
    TriggerState trigger;
    trigger.Init(10);  // 10 sample duration

    // Initially low
    REQUIRE(trigger.high == false);

    // Fire
    trigger.Fire();
    REQUIRE(trigger.high == true);
    REQUIRE(trigger.samplesRemaining == 10);

    // Process some samples
    for (int i = 0; i < 5; ++i)
    {
        trigger.Process();
    }
    REQUIRE(trigger.high == true);
    REQUIRE(trigger.samplesRemaining == 5);

    // Process remaining
    for (int i = 0; i < 5; ++i)
    {
        trigger.Process();
    }
    REQUIRE(trigger.high == false);
}

// =============================================================================
// VelocityOutput Tests [outputs][velocity]
// =============================================================================

TEST_CASE("VelocityOutput sample and hold behavior", "[outputs][velocity]")
{
    VelocityOutputState state;
    state.Init();

    // Initially at 0
    REQUIRE(state.heldVoltage == Approx(0.0f));
    REQUIRE(state.GetVoltage() == Approx(0.0f));

    SECTION("Velocity is sampled on trigger")
    {
        VelocityOutput::TriggerVelocity(state, 0.75f);
        REQUIRE(state.heldVoltage == Approx(0.75f));
        REQUIRE(state.triggered == true);
        REQUIRE(state.GetVoltage() == Approx(3.75f));  // 0.75 * 5V
    }

    SECTION("Velocity holds until next trigger")
    {
        VelocityOutput::TriggerVelocity(state, 0.5f);
        REQUIRE(state.heldVoltage == Approx(0.5f));

        // Process clears triggered flag but keeps held value
        state.Process();
        REQUIRE(state.triggered == false);
        REQUIRE(state.heldVoltage == Approx(0.5f));

        // Multiple processes don't change held value
        for (int i = 0; i < 100; ++i)
        {
            state.Process();
        }
        REQUIRE(state.heldVoltage == Approx(0.5f));
        REQUIRE(state.GetVoltage() == Approx(2.5f));
    }

    SECTION("New trigger updates held value")
    {
        VelocityOutput::TriggerVelocity(state, 0.3f);
        REQUIRE(state.heldVoltage == Approx(0.3f));

        VelocityOutput::TriggerVelocity(state, 0.9f);
        REQUIRE(state.heldVoltage == Approx(0.9f));
    }
}

TEST_CASE("VelocityOutput clamps input values", "[outputs][velocity]")
{
    VelocityOutputState state;
    state.Init();

    SECTION("Negative velocity clamped to 0")
    {
        VelocityOutput::TriggerVelocity(state, -0.5f);
        REQUIRE(state.heldVoltage == Approx(0.0f));
    }

    SECTION("Velocity > 1 clamped to 1")
    {
        VelocityOutput::TriggerVelocity(state, 1.5f);
        REQUIRE(state.heldVoltage == Approx(1.0f));
    }
}

TEST_CASE("VelocityOutput produces correct codec samples", "[outputs][velocity]")
{
    VelocityOutput processor;
    processor.Init(48000.0f);

    VelocityOutputState state;
    state.Init();

    SECTION("Zero velocity produces 0V")
    {
        VelocityOutput::TriggerVelocity(state, 0.0f);
        float sample = processor.ProcessVelocityOutput(state);
        REQUIRE(sample == Approx(0.0f).margin(1e-6f));
    }

    SECTION("Full velocity produces 5V equivalent")
    {
        VelocityOutput::TriggerVelocity(state, 1.0f);
        float sample = processor.ProcessVelocityOutput(state);
        float expected = -5.0f / GateScaler::kCodecMaxVoltage;
        REQUIRE(sample == Approx(expected).margin(1e-6f));
    }

    SECTION("Half velocity produces 2.5V equivalent")
    {
        VelocityOutput::TriggerVelocity(state, 0.5f);
        float sample = processor.ProcessVelocityOutput(state);
        float expected = -2.5f / GateScaler::kCodecMaxVoltage;
        REQUIRE(sample == Approx(expected).margin(1e-6f));
    }
}

TEST_CASE("VelocityOutput velocity curve", "[outputs][velocity]")
{
    SECTION("Linear curve (curveAmount = 0)")
    {
        REQUIRE(VelocityOutput::ApplyVelocityCurve(0.5f, 0.0f) == Approx(0.5f));
        REQUIRE(VelocityOutput::ApplyVelocityCurve(0.25f, 0.0f) == Approx(0.25f));
    }

    SECTION("Exponential curve (curveAmount = 1)")
    {
        // x^2 curve
        REQUIRE(VelocityOutput::ApplyVelocityCurve(0.5f, 1.0f) == Approx(0.25f));
        REQUIRE(VelocityOutput::ApplyVelocityCurve(1.0f, 1.0f) == Approx(1.0f));
    }

    SECTION("Blended curve")
    {
        // 50% blend between linear and exponential
        float result = VelocityOutput::ApplyVelocityCurve(0.5f, 0.5f);
        // 0.5 + 0.5 * (0.25 - 0.5) = 0.5 - 0.125 = 0.375
        REQUIRE(result == Approx(0.375f));
    }
}

// =============================================================================
// AuxOutput Tests [outputs][aux]
// =============================================================================

TEST_CASE("AuxOutput HAT mode fires triggers", "[outputs][aux]")
{
    AuxOutput processor;
    processor.Init(48000.0f);
    processor.SetMode(AuxMode::HAT);

    AuxOutputState auxState;
    auxState.Init(AuxMode::HAT);

    SequencerState seqState;
    seqState.Init();

    SECTION("AUX fires when auxFires is true")
    {
        processor.ComputeAuxOutput(auxState, seqState, false, true, false);
        REQUIRE(auxState.trigger.high == true);
    }

    SECTION("AUX does not fire when auxFires is false")
    {
        processor.ComputeAuxOutput(auxState, seqState, false, false, false);
        REQUIRE(auxState.trigger.high == false);
    }

    SECTION("HAT mode output voltage is correct")
    {
        auxState.trigger.high = true;
        REQUIRE(auxState.GetVoltage() == Approx(5.0f));

        auxState.trigger.high = false;
        REQUIRE(auxState.GetVoltage() == Approx(0.0f));
    }
}

TEST_CASE("AuxOutput FILL_GATE mode tracks fill zones", "[outputs][aux]")
{
    AuxOutput processor;
    processor.Init(48000.0f);
    processor.SetMode(AuxMode::FILL_GATE);

    AuxOutputState auxState;
    auxState.Init(AuxMode::FILL_GATE);

    SequencerState seqState;
    seqState.Init();

    SECTION("Gate high during fill zone")
    {
        processor.ComputeAuxOutput(auxState, seqState, true, false, false);
        REQUIRE(auxState.gateHigh == true);
        REQUIRE(auxState.GetVoltage() == Approx(5.0f));
    }

    SECTION("Gate low outside fill zone")
    {
        processor.ComputeAuxOutput(auxState, seqState, false, false, false);
        REQUIRE(auxState.gateHigh == false);
        REQUIRE(auxState.GetVoltage() == Approx(0.0f));
    }

    SECTION("Gate transitions correctly")
    {
        // Enter fill zone
        processor.ComputeAuxOutput(auxState, seqState, true, false, false);
        REQUIRE(auxState.gateHigh == true);

        // Exit fill zone
        processor.ComputeAuxOutput(auxState, seqState, false, false, false);
        REQUIRE(auxState.gateHigh == false);
    }
}

TEST_CASE("AuxOutput PHRASE_CV mode produces ramp", "[outputs][aux]")
{
    AuxOutput processor;
    processor.Init(48000.0f);
    processor.SetMode(AuxMode::PHRASE_CV);

    AuxOutputState auxState;
    auxState.Init(AuxMode::PHRASE_CV);

    SequencerState seqState;
    seqState.Init();

    SECTION("Phrase ramp starts at 0")
    {
        REQUIRE(auxState.phraseRamp == Approx(0.0f));
        REQUIRE(auxState.GetVoltage() == Approx(0.0f));
    }

    SECTION("Phrase ramp tracks progress")
    {
        AuxOutput::UpdatePhraseRamp(auxState, 0.5f);
        REQUIRE(auxState.phraseRamp == Approx(0.5f));
        REQUIRE(auxState.GetVoltage() == Approx(2.5f));

        AuxOutput::UpdatePhraseRamp(auxState, 1.0f);
        REQUIRE(auxState.phraseRamp == Approx(1.0f));
        REQUIRE(auxState.GetVoltage() == Approx(5.0f));
    }

    SECTION("Phrase ramp clamps to valid range")
    {
        AuxOutput::UpdatePhraseRamp(auxState, -0.5f);
        REQUIRE(auxState.phraseRamp == Approx(0.0f));

        AuxOutput::UpdatePhraseRamp(auxState, 1.5f);
        REQUIRE(auxState.phraseRamp == Approx(1.0f));
    }

    SECTION("Phrase ramp resets at boundary")
    {
        AuxOutput::UpdatePhraseRamp(auxState, 0.9f);
        REQUIRE(auxState.phraseRamp == Approx(0.9f));

        // Simulate phrase boundary reset
        AuxOutput::UpdatePhraseRamp(auxState, 0.0f);
        REQUIRE(auxState.phraseRamp == Approx(0.0f));
    }
}

TEST_CASE("AuxOutput EVENT mode fires on events", "[outputs][aux]")
{
    AuxOutput processor;
    processor.Init(48000.0f);
    processor.SetMode(AuxMode::EVENT);

    AuxOutputState auxState;
    auxState.Init(AuxMode::EVENT);

    SequencerState seqState;
    seqState.Init();

    SECTION("EVENT fires when isEvent is true")
    {
        processor.ComputeAuxOutput(auxState, seqState, false, false, true);
        REQUIRE(auxState.trigger.high == true);
    }

    SECTION("EVENT does not fire when isEvent is false")
    {
        processor.ComputeAuxOutput(auxState, seqState, false, false, false);
        REQUIRE(auxState.trigger.high == false);
    }

    SECTION("EVENT ignores auxFires")
    {
        // auxFires true but isEvent false = no trigger
        processor.ComputeAuxOutput(auxState, seqState, false, true, false);
        REQUIRE(auxState.trigger.high == false);
    }
}

TEST_CASE("AuxOutput mode switching", "[outputs][aux]")
{
    AuxOutput processor;
    processor.Init(48000.0f);

    REQUIRE(processor.GetMode() == AuxMode::HAT);  // Default mode

    processor.SetMode(AuxMode::FILL_GATE);
    REQUIRE(processor.GetMode() == AuxMode::FILL_GATE);

    processor.SetMode(AuxMode::PHRASE_CV);
    REQUIRE(processor.GetMode() == AuxMode::PHRASE_CV);

    processor.SetMode(AuxMode::EVENT);
    REQUIRE(processor.GetMode() == AuxMode::EVENT);
}

TEST_CASE("AuxOutput produces correct codec samples", "[outputs][aux]")
{
    AuxOutput processor;
    processor.Init(48000.0f);

    AuxOutputState auxState;
    auxState.Init(AuxMode::HAT);

    SECTION("Trigger high produces 5V equivalent")
    {
        auxState.mode = AuxMode::HAT;
        auxState.trigger.high = true;
        float sample = processor.ProcessAuxOutput(auxState);
        float expected = -5.0f / GateScaler::kCodecMaxVoltage;
        REQUIRE(sample == Approx(expected).margin(1e-6f));
    }

    SECTION("Phrase ramp produces scaled voltage")
    {
        auxState.mode = AuxMode::PHRASE_CV;
        auxState.phraseRamp = 0.5f;
        float sample = processor.ProcessAuxOutput(auxState);
        float expected = -2.5f / GateScaler::kCodecMaxVoltage;
        REQUIRE(sample == Approx(expected).margin(1e-6f));
    }
}

// =============================================================================
// OutputState Integration Tests [outputs][integration]
// =============================================================================

TEST_CASE("OutputState initializes all components", "[outputs][integration]")
{
    OutputState output;
    output.Init(48000.0f);

    REQUIRE(output.anchorTrigger.high == false);
    REQUIRE(output.shimmerTrigger.high == false);
    REQUIRE(output.anchorVelocity.heldVoltage == Approx(0.0f));
    REQUIRE(output.shimmerVelocity.heldVoltage == Approx(0.0f));
    REQUIRE(output.usingExternalClock == false);
}

TEST_CASE("OutputState FireAnchor triggers and sets velocity", "[outputs][integration]")
{
    OutputState output;
    output.Init(48000.0f);

    output.FireAnchor(0.8f, false);

    REQUIRE(output.anchorTrigger.high == true);
    REQUIRE(output.anchorVelocity.heldVoltage == Approx(0.8f));
    REQUIRE(output.led.brightness >= 0.7f);  // LED triggered
}

TEST_CASE("OutputState FireShimmer triggers and sets velocity", "[outputs][integration]")
{
    OutputState output;
    output.Init(48000.0f);

    output.FireShimmer(0.6f, true);  // Accented

    REQUIRE(output.shimmerTrigger.high == true);
    REQUIRE(output.shimmerVelocity.heldVoltage == Approx(0.6f));
}

TEST_CASE("OutputState Process advances all components", "[outputs][integration]")
{
    OutputState output;
    output.Init(48000.0f);

    // Fire triggers
    output.FireAnchor(1.0f, false);
    output.FireShimmer(1.0f, false);
    output.FireAux();

    // Verify triggers are high
    REQUIRE(output.anchorTrigger.high == true);
    REQUIRE(output.shimmerTrigger.high == true);

    // Process enough samples to expire triggers
    // Trigger pulse duration is 10ms (from task 18)
    // At 48kHz, that's 480 samples. Process 500 to be safe.
    for (int i = 0; i < 500; ++i)
    {
        output.Process(48000.0f);
    }

    // Triggers should be low now
    REQUIRE(output.anchorTrigger.high == false);
    REQUIRE(output.shimmerTrigger.high == false);

    // Velocity should still be held
    REQUIRE(output.anchorVelocity.heldVoltage == Approx(1.0f));
    REQUIRE(output.shimmerVelocity.heldVoltage == Approx(1.0f));
}

TEST_CASE("OutputState FireAux only fires in HAT or EVENT mode", "[outputs][integration]")
{
    OutputState output;
    output.Init(48000.0f);

    SECTION("FireAux in HAT mode triggers")
    {
        output.aux.mode = AuxMode::HAT;
        output.FireAux();
        REQUIRE(output.aux.trigger.high == true);
    }

    SECTION("FireAux in EVENT mode triggers")
    {
        output.aux.mode = AuxMode::EVENT;
        output.FireAux();
        REQUIRE(output.aux.trigger.high == true);
    }

    SECTION("FireAux in FILL_GATE mode does nothing")
    {
        output.aux.mode = AuxMode::FILL_GATE;
        output.FireAux();
        REQUIRE(output.aux.trigger.high == false);
    }

    SECTION("FireAux in PHRASE_CV mode does nothing")
    {
        output.aux.mode = AuxMode::PHRASE_CV;
        output.FireAux();
        REQUIRE(output.aux.trigger.high == false);
    }
}
