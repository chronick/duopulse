#include "Sequencer.h"
#include "config.h"

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

    // Initialize internal clock (16th notes at 120 BPM = 8 Hz)
    metro_.Init(8.0f, sampleRate_);

    // Initialize state
    state_.Init(sampleRate_);
    state_.SetBpm(120.0f);

    // Initialize genre fields (load archetype data)
    InitializeGenreFields();

    // Calculate initial samples per step
    samplesPerStep_ = (sampleRate_ * 60.0f) / (state_.currentBpm * 4.0f);

    // Initialize timing
    stepSampleCounter_ = 0;
    clockDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);  // 10ms clock pulse
    clockTimer_ = 0;
    accentHoldSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    hihatHoldSamples_ = static_cast<int>(sampleRate_ * 0.01f);

    // Initialize phrase position
    phrasePos_ = CalculatePhrasePosition(0, state_.controls.phraseLength);

    // External clock state
    usingExternalClock_ = false;
    externalClockTimeout_ = 0;
    mustTick_ = false;
    lastTapTime_ = 0;

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
    BlendArchetype();
    GenerateBar();
}

// =============================================================================
// Core Processing
// =============================================================================

std::array<float, 2> Sequencer::ProcessAudio()
{
    bool tick = false;

    // Handle external vs internal clock
    if (usingExternalClock_)
    {
        if (mustTick_)
        {
            tick = true;
            mustTick_ = false;
        }

        externalClockTimeout_--;
        if (externalClockTimeout_ <= 0)
        {
            usingExternalClock_ = false;
            metro_.Reset();
        }
    }
    else
    {
        tick = metro_.Process();
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

        // Generate new bar if at bar boundary
        if (state_.sequencer.isBarBoundary || isFirstStep)
        {
            // Update archetype blend at bar boundaries
            BlendArchetype();
            GenerateBar();
            ComputeTimingOffsets();
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
    const float balance = state_.controls.balance;
    const float drift = state_.controls.drift;
    const int patternLength = state_.controls.patternLength;

    // Update derived parameters
    UpdateDerivedControls();

    const EnergyZone zone = state_.controls.energyZone;
    const VoiceCoupling coupling = state_.controls.voiceCoupling;
    const AuxDensity auxDensity = state_.controls.auxDensity;
    const Genre genre = state_.controls.genre;

    // Get phrase progress for BUILD modifiers
    const float phraseProgress = state_.GetPhraseProgress();
    state_.controls.UpdateDerived(phraseProgress);

    // 1. Compute hit budget
    BarBudget budget;
    ComputeBarBudget(
        energy, balance, zone, auxDensity, patternLength,
        state_.controls.buildModifiers.densityMultiplier, budget
    );

    // Apply fill boost if in fill zone
    if (state_.controls.buildModifiers.inFillZone)
    {
        ApplyFillBoost(
            budget,
            state_.controls.buildModifiers.fillIntensity,
            state_.blendedArchetype.fillDensityMultiplier,
            patternLength
        );
    }

    // 2. Select seeds for generation
    const uint32_t seed = SelectSeed(
        state_.sequencer.driftState, drift, 0, patternLength
    );

    // 3. Generate anchor hits
    uint32_t anchorMask = SelectHitsGumbelTopK(
        state_.blendedArchetype.anchorWeights,
        budget.anchorEligibility,
        budget.anchorHits,
        seed,
        patternLength,
        GetMinSpacingForZone(zone)
    );

    // 4. Generate shimmer hits
    uint32_t shimmerMask = SelectHitsGumbelTopK(
        state_.blendedArchetype.shimmerWeights,
        budget.shimmerEligibility,
        budget.shimmerHits,
        seed ^ 0x12345678,  // Different seed for shimmer
        patternLength,
        GetMinSpacingForZone(zone)
    );

    // 5. Apply voice relationship
    ApplyVoiceRelationship(anchorMask, shimmerMask, coupling, patternLength);

    // 6. Soft repair pass
    SoftRepairPass(
        anchorMask, shimmerMask,
        state_.blendedArchetype.anchorWeights,
        state_.blendedArchetype.shimmerWeights,
        zone, patternLength
    );

    // 7. Hard guard rails
    ApplyHardGuardRails(anchorMask, shimmerMask, zone, genre, patternLength);

    // 8. Generate aux hits
    uint32_t auxMask = SelectHitsGumbelTopK(
        state_.blendedArchetype.auxWeights,
        budget.auxEligibility,
        budget.auxHits,
        seed ^ 0x87654321,
        patternLength,
        0  // No spacing constraint for aux
    );

    // Apply aux voice relationship
    ApplyAuxRelationship(anchorMask, shimmerMask, auxMask, coupling, patternLength);

    // 9. Store hit masks in sequencer state
    state_.sequencer.anchorMask = anchorMask;
    state_.sequencer.shimmerMask = shimmerMask;
    state_.sequencer.auxMask = auxMask;

    // 10. Compute accent masks from archetype
    state_.sequencer.anchorAccentMask = state_.blendedArchetype.anchorAccentMask;
    state_.sequencer.shimmerAccentMask = state_.blendedArchetype.shimmerAccentMask;
}

void Sequencer::ProcessStep()
{
    const int step = state_.sequencer.currentStep;
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
            PunchParams punchParams;
            punchParams.ComputeFromPunch(state_.controls.punch);
            BuildModifiers buildMods;
            buildMods.ComputeFromBuild(state_.controls.build, phraseProgress);

            // For forced accent, use higher velocity (0.8 = accent floor + boost)
            float velocity = forcedKickAccent_ ? 0.85f : 0.6f;
            velocity = ComputeVelocity(punchParams, buildMods, forcedKickAccent_, seed, step);

            state_.outputs.FireAnchor(velocity, forcedKickAccent_);
        }
        if (forcedTriggers_[1])
        {
            PunchParams punchParams;
            punchParams.ComputeFromPunch(state_.controls.punch);
            BuildModifiers buildMods;
            buildMods.ComputeFromBuild(state_.controls.build, phraseProgress);

            float velocity = ComputeVelocity(punchParams, buildMods, false, seed, step);
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
        float velocity = ComputeAnchorVelocity(
            state_.controls.punch, state_.controls.build,
            phraseProgress, step, seed,
            state_.blendedArchetype.anchorAccentMask
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
        float velocity = ComputeShimmerVelocity(
            state_.controls.punch, state_.controls.build,
            phraseProgress, step, seed,
            state_.blendedArchetype.shimmerAccentMask
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
    state_.sequencer.inFillZone = state_.controls.buildModifiers.inFillZone;
}

void Sequencer::AdvanceStep()
{
    // Advance position
    state_.sequencer.AdvanceStep(
        state_.controls.patternLength,
        state_.controls.phraseLength
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
    BlendArchetype();
    GenerateBar();
    ComputeTimingOffsets();
}

void Sequencer::TriggerExternalClock()
{
    externalClockTimeout_ = static_cast<int>(sampleRate_ * 2.0f);  // 2 second timeout
    usingExternalClock_ = true;
    mustTick_ = true;
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
    state_.controls.build = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFieldX(float value)
{
    state_.controls.fieldX = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFieldY(float value)
{
    state_.controls.fieldY = Clamp(value, 0.0f, 1.0f);
}

// =============================================================================
// Parameter Setters (Performance Mode Shift)
// =============================================================================

void Sequencer::SetPunch(float value)
{
    state_.controls.punch = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetGenre(float value)
{
    state_.controls.genre = GetGenreFromValue(Clamp(value, 0.0f, 1.0f));

    // Reload genre field
    state_.currentField = GetGenreField(state_.controls.genre);
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
    if (bars < 1) bars = 1;
    if (bars > 8) bars = 8;
    state_.controls.phraseLength = bars;
}

void Sequencer::SetClockDivision(int div)
{
    if (div < 1) div = 1;
    if (div > 8) div = 8;
    state_.controls.clockDivision = div;
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
    state_.controls.buildCV = Clamp(value, -0.5f, 0.5f);
}

void Sequencer::SetFieldXCV(float value)
{
    state_.controls.fieldXCV = Clamp(value, -0.5f, 0.5f);
}

void Sequencer::SetFieldYCV(float value)
{
    state_.controls.fieldYCV = Clamp(value, -0.5f, 0.5f);
}

void Sequencer::SetFlavorCV(float value)
{
    state_.controls.flavorCV = Clamp(value, 0.0f, 1.0f);
}

// =============================================================================
// Legacy v3 Compatibility
// =============================================================================

void Sequencer::SetTempoControl(float value)
{
    float tempoControl = Clamp(value, 0.0f, 1.0f);
    // Map 0-1 to 90-160 BPM (v3 range)
    float newBpm = 90.0f + (tempoControl * 70.0f);
    SetBpm(newBpm);
}

void Sequencer::SetGateTime(float value)
{
    float gateTime = Clamp(value, 0.0f, 1.0f);
    // Map 0-1 to 5-50ms
    float gateMs = 5.0f + (gateTime * 45.0f);
    int samples = static_cast<int>(sampleRate_ * gateMs / 1000.0f);
    if (samples < 1) samples = 1;

    state_.outputs.anchorTrigger.triggerDurationSamples = samples;
    state_.outputs.shimmerTrigger.triggerDurationSamples = samples;
    state_.outputs.aux.trigger.triggerDurationSamples = samples;
}

void Sequencer::SetClockDiv(float value)
{
    float div = Clamp(value, 0.0f, 1.0f);
    // Map to 1, 2, 4, 8
    if (div < 0.25f)
        SetClockDivision(1);
    else if (div < 0.5f)
        SetClockDivision(2);
    else if (div < 0.75f)
        SetClockDivision(4);
    else
        SetClockDivision(8);
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

float Sequencer::GetSwingPercent() const
{
    return ComputeSwing(state_.controls.flavorCV, state_.controls.energyZone);
}

void Sequencer::SetBpm(float bpm)
{
    bpm = Clamp(bpm, kMinTempo, kMaxTempo);
    state_.SetBpm(bpm);

    // Update metro frequency (16th notes = BPM * 4 / 60)
    metro_.SetFreq(bpm / 60.0f * 4.0f);

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
    int phraseLength = state_.controls.phraseLength;
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

void Sequencer::BlendArchetype()
{
    // Get current genre field
    const GenreField& field = GetGenreField(state_.controls.genre);

    // Get effective field position with CV modulation
    float fieldX = state_.controls.GetEffectiveFieldX();
    float fieldY = state_.controls.GetEffectiveFieldY();

    // Blend archetypes
    GetBlendedArchetype(
        field, fieldX, fieldY,
        kDefaultSoftmaxTemperature,
        state_.blendedArchetype
    );
}

void Sequencer::ComputeTimingOffsets()
{
    const int patternLength = state_.controls.patternLength;
    const float flavor = state_.controls.flavorCV;
    const EnergyZone zone = state_.controls.energyZone;
    const uint32_t seed = state_.sequencer.driftState.phraseSeed;

    // Compute swing amount
    float swingAmount = ComputeSwing(flavor, zone);

    for (int step = 0; step < patternLength && step < kMaxSteps; ++step)
    {
        // Swing offset (only affects odd steps)
        float swingOffset = ApplySwingToStep(step, swingAmount, samplesPerStep_);

        // Microtiming jitter
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

} // namespace daisysp_idm_grids
