#include "Sequencer.h"
#include "config.h"
#include "../System/logging.h"
#include "EuclideanGen.h"  // For genre-aware Euclidean blending
#include "PatternGenerator.h"  // For GeneratePattern, RotateWithPreserve

#include <algorithm>
#include <cmath>

namespace daisysp_idm_grids
{

namespace
{
template <typename T>
inline T Clamp(T value, T minValue, T maxValue)
{
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}
} // namespace

// =============================================================================
// Initialization
// =============================================================================

void Sequencer::Init(float sampleRate)
{
    sampleRate_ = sampleRate;

    // Log sample rate for debugging (cast to int - nano.specs doesn't support %f)
    LOGI("Sequencer::Init called with sampleRate=%d", static_cast<int>(sampleRate_));

    // Initialize internal clock (16th notes at 120 BPM = 8 Hz)
    metro_.Init(8.0f, sampleRate_);
    LOGD("Metro initialized: freq=8 Hz, period=%d samples", static_cast<int>(sampleRate_ / 8.0f));

    // Initialize state
    state_.Init(sampleRate_);
    state_.SetBpm(120.0f);

    // V5: Genre fields no longer initialized - procedural generation handles pattern character

    // Calculate initial samples per step
    samplesPerStep_ = (sampleRate_ * 60.0f) / (state_.currentBpm * 4.0f);

    // Initialize timing
    stepSampleCounter_ = 0;
    clockDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);  // 10ms clock pulse
    clockTimer_ = 0;
    accentHoldSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    hihatHoldSamples_ = static_cast<int>(sampleRate_ * 0.01f);

    // Initialize phrase position
    phrasePos_ = CalculatePhrasePosition(0, state_.controls.GetDerivedPhraseLength());

    // External clock state (exclusive mode)
    externalClockActive_ = false;
    externalClockTick_ = false;
    lastTapTime_ = 0;

    // Axis change tracking (Task 23: Immediate Field Updates)
    // V5: Renamed from Field to Axis (Task 27)
    previousFieldX_ = state_.controls.GetEffectiveAxisX();
    previousFieldY_ = state_.controls.GetEffectiveAxisY();
    fieldChangeRegenPending_ = false;

    // Clock division/multiplication state
    clockPulseCounter_ = 0;
    lastExternalClockTime_ = 0;
    externalClockInterval_ = 0;
    multiplicationSubdivCounter_ = 0;

    // Log initial clock state for debugging
    LOGI("Clock init: div=%d, externalActive=%d, BPM=%d",
         state_.controls.clockDivision,
         externalClockActive_ ? 1 : 0,
         static_cast<int>(state_.currentBpm));

    // Force trigger state
    forceNextTriggers_ = false;
    for (int i = 0; i < 3; ++i)
    {
        forcedTriggers_[i] = false;
        triggerDelayRemaining_[i] = 0;
        triggerPending_[i] = false;
        pendingVelocity_[i] = 0.0f;
        pendingAccent_[i] = false;
    }
    forcedKickAccent_ = false;

    // Initial bar generation
    // V5: BlendArchetype() no longer needed - using procedural generation
    GenerateBar();
}

// =============================================================================
// Core Processing
// =============================================================================

std::array<float, 2> Sequencer::ProcessAudio()
{
    bool tick = false;

    // Handle external vs internal clock (exclusive mode - spec section 3.4)
    if (externalClockActive_)
    {
        // External clock mode: steps advance based on rising edges and clock division
        // Internal Metro is completely disabled (no parallel operation)
        if (externalClockTick_)
        {
            tick = true;
            externalClockTick_ = false;  // Consume the tick
        }

        // Handle external clock multiplication: generate subdivided ticks
        int clockDiv = state_.controls.clockDivision;
        if (clockDiv < 0 && externalClockInterval_ > 0)
        {
            int multiplier = std::abs(clockDiv);
            uint32_t subdivInterval = externalClockInterval_ / static_cast<uint32_t>(multiplier);

            // Increment subdivision counter every sample
            multiplicationSubdivCounter_++;

            // Check if it's time for next subdivision tick
            if (static_cast<uint32_t>(multiplicationSubdivCounter_) >= subdivInterval &&
                static_cast<uint32_t>(multiplicationSubdivCounter_) < externalClockInterval_)
            {
                tick = true;
                multiplicationSubdivCounter_ += subdivInterval;  // Advance to next subdivision
            }
        }
        // Note: No timeout logic - external clock remains active until explicitly disabled
    }
    else
    {
        // Internal clock mode: Metro drives step advancement
        bool metroPulse = metro_.Process();

        int clockDiv = state_.controls.clockDivision;

        if (clockDiv > 1)
        {
            // DIVISION mode: Count Metro pulses, only tick when threshold reached
            if (metroPulse)
            {
                clockPulseCounter_++;
                if (clockPulseCounter_ >= clockDiv)
                {
                    tick = true;
                    clockPulseCounter_ = 0;
                }
            }
        }
        else if (clockDiv < 0)
        {
            // MULTIPLICATION mode: Metro frequency already multiplied in SetClockDivision()
            tick = metroPulse;
        }
        else
        {
            // 1:1 mode: Direct pass-through
            tick = metroPulse;
        }
    }

    // Process step on clock tick
    if (tick)
    {
        bool isFirstStep = (state_.sequencer.totalSteps == 0);

        // Advance to next step (unless we're processing the very first step)
        if (!isFirstStep)
        {
            AdvanceStep();
        }
        else
        {
            // Mark that we've started (so subsequent ticks will advance)
            state_.sequencer.totalSteps = 1;
        }

        // Task 23: Check for Field X/Y changes on every step (sets fieldChangeRegenPending_ flag)
        CheckFieldChange();

        // A beat is every 4 steps on the 16th-note grid (4 steps = 1 beat = 1 quarter note)
        static constexpr int kStepsPerBeat = 4;
        const bool isBeatBoundary = (state_.sequencer.currentStep % kStepsPerBeat == 0);

        // Regenerate at beat boundaries when field has changed (but not at bar boundaries to avoid double-regen)
        if (fieldChangeRegenPending_ && isBeatBoundary && !state_.sequencer.isBarBoundary)
        {
            // V5: BlendArchetype() no longer needed - using procedural generation
            GenerateBar();
            ComputeTimingOffsets();
            fieldChangeRegenPending_ = false;
        }

        // Generate new bar if at bar boundary
        if (state_.sequencer.isBarBoundary || isFirstStep)
        {
            // V5: BlendArchetype() no longer needed - using procedural generation
            GenerateBar();
            ComputeTimingOffsets();
            fieldChangeRegenPending_ = false;  // Also clear flag here
        }

        // Process the step (fire triggers)
        ProcessStep();

        // Fire clock output
        clockTimer_ = clockDurationSamples_;

        // Update phrase position for v3 compatibility
        UpdatePhrasePosition();
    }

    // Process pending delayed triggers
    for (int v = 0; v < 3; ++v)
    {
        if (triggerPending_[v])
        {
            triggerDelayRemaining_[v]--;
            if (triggerDelayRemaining_[v] <= 0)
            {
                triggerPending_[v] = false;

                // Fire the trigger
                switch (static_cast<Voice>(v))
                {
                    case Voice::ANCHOR:
                        state_.outputs.FireAnchor(pendingVelocity_[v], pendingAccent_[v]);
                        break;
                    case Voice::SHIMMER:
                        state_.outputs.FireShimmer(pendingVelocity_[v], pendingAccent_[v]);
                        break;
                    case Voice::AUX:
                        state_.outputs.FireAux();
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // Process all output states (decay triggers, etc.)
    state_.outputs.Process(sampleRate_);

    // Decrement clock timer
    if (clockTimer_ > 0)
    {
        clockTimer_--;
    }

    // Return velocity outputs
    return {
        state_.outputs.anchorVelocity.heldVoltage,
        state_.outputs.shimmerVelocity.heldVoltage
    };
}

void Sequencer::GenerateBar()
{
    // Get effective control values (with CV modulation)
    const float energy = state_.controls.GetEffectiveEnergy();
    const int patternLength = state_.controls.patternLength;

    // Update derived parameters
    UpdateDerivedControls();

    // Get phrase progress for SHAPE modifiers
    const float phraseProgress = state_.GetPhraseProgress();
    state_.controls.UpdateDerived(phraseProgress);

    // For patterns > 32 steps, we generate two 32-step halves and combine
    const bool isLongPattern = patternLength > 32;
    const int halfLength = isLongPattern ? 32 : patternLength;

    // Select seed for generation
    const uint32_t seed = SelectSeed(
        state_.sequencer.driftState, state_.controls.drift, 0, patternLength
    );

    // Compute Euclidean blend ratio (genre-aware)
    const float fieldX = state_.controls.GetEffectiveAxisX();
    const float euclideanRatio = GetGenreEuclideanRatio(
        state_.controls.genre, fieldX, state_.controls.energyZone);

    // Populate parameters for GeneratePattern
    PatternParams params;
    params.energy = energy;
    params.shape = state_.controls.shape;
    params.axisX = state_.controls.GetEffectiveAxisX();
    params.axisY = state_.controls.GetEffectiveAxisY();
    params.drift = state_.controls.drift;
    params.accent = state_.controls.accent;
    params.seed = seed;
    params.patternLength = halfLength;

    // Firmware-specific options
    params.balance = state_.controls.balance;
    params.densityMultiplier = state_.controls.shapeModifiers.densityMultiplier;
    params.inFillZone = state_.controls.shapeModifiers.inFillZone;
    params.fillIntensity = state_.controls.shapeModifiers.fillIntensity;
    params.fillDensityMultiplier = 1.5f;
    params.euclideanRatio = euclideanRatio;
    params.genre = state_.controls.genre;
    params.auxDensity = state_.controls.auxDensity;
    params.applySoftRepair = true;
    params.voiceCoupling = state_.controls.voiceCoupling;

    // Generate first half
    PatternResult result1;
    GeneratePattern(params, result1);

    // Generate second half for long patterns
    PatternResult result2;
    if (isLongPattern)
    {
        // Use different seed for second half
        params.seed = seed ^ 0xDEADBEEF;
        GeneratePattern(params, result2);
    }

    // Store hit masks in sequencer state (combine halves for 64-bit)
    state_.sequencer.anchorMask = static_cast<uint64_t>(result1.anchorMask) |
                                   (static_cast<uint64_t>(result2.anchorMask) << 32);
    state_.sequencer.shimmerMask = static_cast<uint64_t>(result1.shimmerMask) |
                                    (static_cast<uint64_t>(result2.shimmerMask) << 32);
    state_.sequencer.auxMask = static_cast<uint64_t>(result1.auxMask) |
                                (static_cast<uint64_t>(result2.auxMask) << 32);

    // V5: Procedural accent masks (downbeats for anchor, backbeats for shimmer)
    uint64_t anchorAccent = 0x0101010101010101ULL;
    uint64_t shimmerAccent = 0x1010101010101010ULL;
    if (!isLongPattern)
    {
        uint64_t lengthMask = (1ULL << patternLength) - 1;
        anchorAccent &= lengthMask;
        shimmerAccent &= lengthMask;
    }
    state_.sequencer.anchorAccentMask = anchorAccent;
    state_.sequencer.shimmerAccentMask = shimmerAccent;
}

void Sequencer::ProcessStep()
{
    const int step = state_.sequencer.currentStep;
    // NOTE: Do NOT log here - called from audio ISR!

    const float phraseProgress = state_.GetPhraseProgress();
    const uint32_t seed = SelectSeed(
        state_.sequencer.driftState,
        state_.controls.drift,
        step,
        state_.controls.patternLength
    );

    // Handle forced triggers (for testing)
    if (forceNextTriggers_)
    {
        if (forcedTriggers_[0])
        {
            // Compute velocity with forced accent
            // V5: Use accent/shape instead of punch/build (Task 27)
            AccentParams accentParams;
            accentParams.ComputeFromAccent(state_.controls.accent);
            ShapeModifiers shapeMods;
            shapeMods.ComputeFromShape(state_.controls.shape, phraseProgress);

            // For forced accent, use higher velocity (0.8 = accent floor + boost)
            float velocity = forcedKickAccent_ ? 0.85f : 0.6f;
            velocity = ComputeVelocity(accentParams, shapeMods, forcedKickAccent_, seed, step);

            state_.outputs.FireAnchor(velocity, forcedKickAccent_);
        }
        if (forcedTriggers_[1])
        {
            // V5: Use accent/shape instead of punch/build (Task 27)
            AccentParams accentParams;
            accentParams.ComputeFromAccent(state_.controls.accent);
            ShapeModifiers shapeMods;
            shapeMods.ComputeFromShape(state_.controls.shape, phraseProgress);

            float velocity = ComputeVelocity(accentParams, shapeMods, false, seed, step);
            state_.outputs.FireShimmer(velocity, false);
        }
        if (forcedTriggers_[2])
        {
            state_.outputs.FireAux();
        }
        forceNextTriggers_ = false;
        return;
    }

    // Get timing offset for this step
    int timingOffset = GetStepTimingOffset();

    // Check anchor
    if (state_.sequencer.AnchorFires())
    {
        bool isAccent = state_.sequencer.AnchorAccented();
        // V5: Use accent/shape instead of punch/build (Task 27)
        // V5: Procedural accent mask - downbeats (steps 0,8,16,24,32,40,48,56)
        float velocity = ComputeAnchorVelocity(
            state_.controls.accent, state_.controls.shape,
            phraseProgress, step, seed,
            0x0101010101010101ULL  // V5: Anchor accent on downbeats
        );

        if (timingOffset <= 0)
        {
            // Fire immediately
            state_.outputs.FireAnchor(velocity, isAccent);
        }
        else
        {
            // Schedule delayed trigger
            triggerPending_[static_cast<int>(Voice::ANCHOR)] = true;
            triggerDelayRemaining_[static_cast<int>(Voice::ANCHOR)] = timingOffset;
            pendingVelocity_[static_cast<int>(Voice::ANCHOR)] = velocity;
            pendingAccent_[static_cast<int>(Voice::ANCHOR)] = isAccent;
        }

        // Update guard rail state
        state_.sequencer.guardRailState.OnAnchorHit();
    }
    else
    {
        state_.sequencer.guardRailState.OnNoHit();
    }

    // Check shimmer
    if (state_.sequencer.ShimmerFires())
    {
        bool isAccent = state_.sequencer.ShimmerAccented();
        // V5: Use accent/shape instead of punch/build (Task 27)
        // V5: Procedural accent mask - backbeats (steps 4,12,20,28,36,44,52,60)
        float velocity = ComputeShimmerVelocity(
            state_.controls.accent, state_.controls.shape,
            phraseProgress, step, seed,
            0x1010101010101010ULL  // V5: Shimmer accent on backbeats
        );

        if (timingOffset <= 0)
        {
            state_.outputs.FireShimmer(velocity, isAccent);
        }
        else
        {
            triggerPending_[static_cast<int>(Voice::SHIMMER)] = true;
            triggerDelayRemaining_[static_cast<int>(Voice::SHIMMER)] = timingOffset;
            pendingVelocity_[static_cast<int>(Voice::SHIMMER)] = velocity;
            pendingAccent_[static_cast<int>(Voice::SHIMMER)] = isAccent;
        }

        if (!state_.sequencer.AnchorFires())
        {
            state_.sequencer.guardRailState.OnShimmerOnlyHit();
        }
    }

    // Check aux
    if (state_.sequencer.AuxFires())
    {
        if (timingOffset <= 0)
        {
            state_.outputs.FireAux();
        }
        else
        {
            triggerPending_[static_cast<int>(Voice::AUX)] = true;
            triggerDelayRemaining_[static_cast<int>(Voice::AUX)] = timingOffset;
        }
    }

    // Update fill zone state
    // V5: Renamed from buildModifiers to shapeModifiers (Task 27)
    state_.sequencer.inFillZone = state_.controls.shapeModifiers.inFillZone;
}

void Sequencer::AdvanceStep()
{
    // Advance position
    state_.sequencer.AdvanceStep(
        state_.controls.patternLength,
        state_.controls.GetDerivedPhraseLength()
    );

    // Handle phrase boundary (update drift state)
    if (state_.sequencer.isPhraseBoundary)
    {
        OnPhraseEnd(state_.sequencer.driftState);
    }
}

// =============================================================================
// External Triggers
// =============================================================================

void Sequencer::TriggerReset()
{
    state_.TriggerReset();
    UpdatePhrasePosition();

    // Regenerate bar on reset
    // V5: BlendArchetype() no longer needed - using procedural generation
    GenerateBar();
    ComputeTimingOffsets();
}

void Sequencer::TriggerExternalClock()
{
    // Enable exclusive external clock mode (spec section 3.4)
    // - Internal Metro is disabled
    // - Steps advance based on external clock rising edges and clock division setting
    externalClockActive_ = true;

    int clockDiv = state_.controls.clockDivision;

    if (clockDiv > 0)
    {
        // DIVISION mode (÷2, ÷4, ÷8): Count pulses, only tick when threshold reached
        clockPulseCounter_++;
        if (clockPulseCounter_ >= clockDiv)
        {
            externalClockTick_ = true;  // Queue one step tick
            clockPulseCounter_ = 0;      // Reset counter
        }
    }
    else if (clockDiv < 0)
    {
        // MULTIPLICATION mode (×2, ×4, ×8): Measure interval and subdivide
        uint32_t now = stepSampleCounter_;  // Use sample counter as timestamp

        if (lastExternalClockTime_ != 0)
        {
            // Measure interval between this pulse and last pulse
            externalClockInterval_ = now - lastExternalClockTime_;

            // Reset subdivision counter for new interval
            multiplicationSubdivCounter_ = 0;
        }

        lastExternalClockTime_ = now;

        // Always tick once on the pulse itself
        externalClockTick_ = true;
    }
    else
    {
        // 1:1 mode: Direct pass-through
        externalClockTick_ = true;
    }
}

void Sequencer::DisableExternalClock()
{
    // Restore internal clock immediately (spec section 3.4)
    externalClockActive_ = false;
    externalClockTick_ = false;  // Clear any pending external ticks
    clockPulseCounter_ = 0;       // Reset clock division counter
    // Note: Metro continues running, so internal clock resumes seamlessly
}

void Sequencer::TriggerTapTempo(uint32_t nowMs)
{
    if (lastTapTime_ != 0)
    {
        uint32_t interval = nowMs - lastTapTime_;
        if (interval > 100 && interval < 2000)
        {
            float newBpm = 60000.0f / static_cast<float>(interval);
            SetBpm(newBpm);
        }
    }
    lastTapTime_ = nowMs;
}

void Sequencer::TriggerReseed()
{
    state_.RequestReseed();
}

// =============================================================================
// Parameter Setters (Performance Mode Primary)
// =============================================================================

void Sequencer::SetEnergy(float value)
{
    state_.controls.energy = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetBuild(float value)
{
    // V5: Renamed to shape internally (Task 27)
    state_.controls.shape = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFieldX(float value)
{
    // V5: Renamed to axisX internally (Task 27)
    state_.controls.axisX = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFieldY(float value)
{
    // V5: Renamed to axisY internally (Task 27)
    state_.controls.axisY = Clamp(value, 0.0f, 1.0f);
}

// =============================================================================
// Parameter Setters (Performance Mode Shift)
// =============================================================================

void Sequencer::SetPunch(float value)
{
    // V5: Renamed to accent internally (Task 27)
    state_.controls.accent = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetGenre(float value)
{
    state_.controls.genre = GetGenreFromValue(Clamp(value, 0.0f, 1.0f));
    // V5: Genre field no longer used - procedural generation handles pattern character
}

void Sequencer::SetDrift(float value)
{
    state_.controls.drift = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetBalance(float value)
{
    state_.controls.balance = Clamp(value, 0.0f, 1.0f);
}

// =============================================================================
// Parameter Setters (Config Mode Primary)
// =============================================================================

void Sequencer::SetPatternLength(int steps)
{
    if (steps < 16) steps = 16;
    if (steps > 64) steps = 64;
    state_.controls.patternLength = steps;
}

void Sequencer::SetSwing(float value)
{
    state_.controls.swing = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetAuxMode(float value)
{
    state_.controls.auxMode = GetAuxModeFromValue(Clamp(value, 0.0f, 1.0f));
    state_.outputs.aux.mode = state_.controls.auxMode;
}

void Sequencer::SetResetMode(float value)
{
    state_.controls.resetMode = GetResetModeFromValue(Clamp(value, 0.0f, 1.0f));
}

// =============================================================================
// Parameter Setters (Config Mode Shift)
// =============================================================================

void Sequencer::SetPhraseLength(int bars)
{
    // Task 22: Phrase length is now auto-derived from pattern length
    // This method is kept for backward compatibility but is a no-op
    (void)bars;
}

void Sequencer::SetClockDivision(int div)
{
    // Accept division (1, 2, 4, 8) and multiplication (-2, -4, -8)
    // Clamp to valid range: ÷8 to ×8
    if (div < -8) div = -8;
    if (div > 8) div = 8;
    if (div == 0) div = 1;  // 0 is invalid, default to 1:1
    // Only allow specific values: -8, -4, -2, 1, 2, 4, 8
    if (div > 0 && div != 1 && div != 2 && div != 4 && div != 8) div = 1;
    if (div < 0 && div != -2 && div != -4 && div != -8) div = 1;

    state_.controls.clockDivision = div;

    // Update Metro frequency if using internal clock and multiplication
    if (!externalClockActive_ && div < 0)
    {
        // Multiply internal clock frequency
        float baseFreq = state_.currentBpm / 60.0f * 4.0f;  // 16th notes
        metro_.SetFreq(baseFreq * std::abs(div));
    }
    else if (!externalClockActive_ && div > 0)
    {
        // Division handled by pulse counter, restore base frequency
        float baseFreq = state_.currentBpm / 60.0f * 4.0f;
        metro_.SetFreq(baseFreq);
    }
}

void Sequencer::SetAuxDensity(float value)
{
    state_.controls.auxDensity = GetAuxDensityFromValue(Clamp(value, 0.0f, 1.0f));
}

void Sequencer::SetVoiceCoupling(float value)
{
    state_.controls.voiceCoupling = GetVoiceCouplingFromValue(Clamp(value, 0.0f, 1.0f));
}

// =============================================================================
// CV Modulation Inputs
// =============================================================================

void Sequencer::SetEnergyCV(float value)
{
    state_.controls.energyCV = Clamp(value, -0.5f, 0.5f);
}

void Sequencer::SetBuildCV(float value)
{
    // V5: Renamed to shapeCV internally (Task 27)
    state_.controls.shapeCV = Clamp(value, -0.5f, 0.5f);
}

void Sequencer::SetFieldXCV(float value)
{
    // V5: Renamed to axisXCV internally (Task 27)
    state_.controls.axisXCV = Clamp(value, -0.5f, 0.5f);
}

void Sequencer::SetFieldYCV(float value)
{
    // V5: Renamed to axisYCV internally (Task 27)
    state_.controls.axisYCV = Clamp(value, -0.5f, 0.5f);
}

void Sequencer::SetFlavorCV(float value)
{
    state_.controls.flavorCV = Clamp(value, 0.0f, 1.0f);
}

// =============================================================================
// State Queries
// =============================================================================

bool Sequencer::IsGateHigh(int channel) const
{
    if (channel == 0)
        return state_.outputs.anchorTrigger.high;
    if (channel == 1)
        return state_.outputs.shimmerTrigger.high;
    return false;
}

bool Sequencer::IsClockHigh() const
{
    return clockTimer_ > 0;
}

bool Sequencer::HasPendingTrigger(int channel) const
{
    if (channel == 0)
        return state_.outputs.anchorTrigger.HasPendingEvent();
    if (channel == 1)
        return state_.outputs.shimmerTrigger.HasPendingEvent();
    return false;
}

void Sequencer::AcknowledgeTrigger(int channel)
{
    if (channel == 0)
        state_.outputs.anchorTrigger.AcknowledgeEvent();
    else if (channel == 1)
        state_.outputs.shimmerTrigger.AcknowledgeEvent();
}

float Sequencer::GetSwingPercent() const
{
    // V5: Derive swing from SHAPE parameter - more swing at mid SHAPE values
    // Maximum swing (~0.15) at SHAPE=0.5, zero at extremes (0.0 and 1.0)
    float archetypeSwing = state_.controls.shape * (1.0f - state_.controls.shape) * 0.6f;
    return ComputeSwing(state_.controls.swing, archetypeSwing, state_.controls.energyZone);
}

void Sequencer::SetBpm(float bpm)
{
    bpm = Clamp(bpm, kMinTempo, kMaxTempo);
    state_.SetBpm(bpm);

    // Update metro frequency (16th notes = BPM * 4 / 60)
    // If clock multiplication is active, multiply the frequency
    float baseFreq = bpm / 60.0f * 4.0f;
    int clockDiv = state_.controls.clockDivision;

    if (clockDiv < 0)
    {
        // Multiplication mode: multiply Metro frequency
        metro_.SetFreq(baseFreq * std::abs(clockDiv));
    }
    else
    {
        // Division or 1:1 mode: use base frequency (division handled by pulse counter)
        metro_.SetFreq(baseFreq);
    }

    // Update samples per step
    samplesPerStep_ = (sampleRate_ * 60.0f) / (bpm * 4.0f);
}

void Sequencer::SetAccentHoldMs(float milliseconds)
{
    accentHoldSamples_ = HoldMsToSamples(milliseconds);
}

void Sequencer::SetHihatHoldMs(float milliseconds)
{
    hihatHoldSamples_ = HoldMsToSamples(milliseconds);
}

void Sequencer::ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent)
{
    forcedTriggers_[0] = kick;
    forcedTriggers_[1] = snare;
    forcedTriggers_[2] = hh;
    forceNextTriggers_ = true;
    forcedKickAccent_ = kickAccent;
}

// =============================================================================
// Internal Methods
// =============================================================================

void Sequencer::UpdatePhrasePosition()
{
    int step = state_.sequencer.currentStep;
    int bar = state_.sequencer.currentBar;
    int phraseLength = state_.controls.GetDerivedPhraseLength();
    int patternLength = state_.controls.patternLength;

    int totalStepsInPhrase = patternLength * phraseLength;
    int currentStepInPhrase = bar * patternLength + step;

    phrasePos_.phraseProgress = static_cast<float>(currentStepInPhrase)
                                / static_cast<float>(totalStepsInPhrase);
    phrasePos_.stepInPhrase = currentStepInPhrase;
    phrasePos_.currentBar = bar;
    phrasePos_.stepInBar = step;
    phrasePos_.isDownbeat = (step == 0);  // Bar downbeat
    phrasePos_.isLastBar = (bar == phraseLength - 1);
    phrasePos_.isFillZone = (phrasePos_.phraseProgress > 0.875f);
    phrasePos_.isBuildZone = (phrasePos_.phraseProgress > 0.5f);
    phrasePos_.isMidPhrase = (phrasePos_.phraseProgress >= 0.4f &&
                              phrasePos_.phraseProgress < 0.6f);
}

void Sequencer::ComputeTimingOffsets()
{
    // Apply swing from config and microtiming jitter from FLAVOR CV
    const int patternLength = state_.controls.patternLength;
    const float swing = state_.controls.swing;      // Config K2: base swing amount
    const float flavor = state_.controls.flavorCV;  // Audio In R: jitter modulation
    const EnergyZone zone = state_.controls.energyZone;
    const uint32_t seed = state_.sequencer.driftState.phraseSeed;

    // V5: Derive swing from SHAPE parameter - more swing at mid SHAPE values
    // Maximum swing (~0.15) at SHAPE=0.5, zero at extremes (0.0 and 1.0)
    float archetypeSwing = state_.controls.shape * (1.0f - state_.controls.shape) * 0.6f;
    float swingAmount = ComputeSwing(swing, archetypeSwing, zone);

    for (int step = 0; step < patternLength && step < kMaxSteps; ++step)
    {
        // Swing offset (only affects odd steps)
        float swingOffset = ApplySwingToStep(step, swingAmount, samplesPerStep_);

        // Microtiming jitter (still uses flavorCV for humanization)
        float jitterOffset = ComputeMicrotimingOffset(
            flavor, zone, sampleRate_, seed, step
        );

        // Store combined offset
        state_.sequencer.swingOffsets[step] = static_cast<int16_t>(swingOffset);
        state_.sequencer.jitterOffsets[step] = static_cast<int16_t>(jitterOffset);
    }
}

void Sequencer::UpdateDerivedControls()
{
    float phraseProgress = state_.GetPhraseProgress();
    state_.controls.UpdateDerived(phraseProgress);
}

int Sequencer::GetStepTimingOffset() const
{
    int step = state_.sequencer.currentStep;
    if (step >= kMaxSteps) return 0;

    int offset = state_.sequencer.swingOffsets[step]
               + state_.sequencer.jitterOffsets[step];

    // Clamp to reasonable range (don't delay more than half a step)
    int maxDelay = static_cast<int>(samplesPerStep_ * 0.5f);
    if (offset > maxDelay) offset = maxDelay;
    if (offset < 0) offset = 0;

    return offset;
}

int Sequencer::HoldMsToSamples(float milliseconds) const
{
    const float clampedMs = milliseconds < 0.5f ? 0.5f
                          : (milliseconds > 2000.0f ? 2000.0f : milliseconds);
    const float samples = (clampedMs / 1000.0f) * sampleRate_;
    int asInt = static_cast<int>(samples);
    return asInt < 1 ? 1 : asInt;
}

bool Sequencer::CheckFieldChange()
{
    // Get current effective Axis X/Y (with CV modulation)
    // V5: Renamed from Field to Axis (Task 27)
    const float currentFieldX = state_.controls.GetEffectiveAxisX();
    const float currentFieldY = state_.controls.GetEffectiveAxisY();

    // Check if change exceeds threshold (10% of full range)
    static constexpr float kFieldChangeThreshold = 0.1f;
    const float deltaX = std::abs(currentFieldX - previousFieldX_);
    const float deltaY = std::abs(currentFieldY - previousFieldY_);

    if (deltaX > kFieldChangeThreshold || deltaY > kFieldChangeThreshold)
    {
        // Update previous values
        previousFieldX_ = currentFieldX;
        previousFieldY_ = currentFieldY;

        // Set regeneration pending flag
        fieldChangeRegenPending_ = true;

        return true;
    }

    return false;
}

} // namespace daisysp_idm_grids
