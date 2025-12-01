#include "Sequencer.h"

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

void Sequencer::Init(float sampleRate)
{
    sampleRate_ = sampleRate;

    patternGen_.Init();

    metro_.Init(2.0f, sampleRate_);
    SetBpm(currentBpm_);

    chaosLow_.Init(0x4b1d2f3c);
    chaosHigh_.Init(0xd3f2a1b9); // Different seed
    chaosLow_.SetAmount(0.0f);
    chaosHigh_.SetAmount(0.0f);

    gateDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    gateTimers_[0] = 0;
    gateTimers_[1] = 0;
    clockDurationSamples_ = static_cast<int>(sampleRate_ * 0.01f);
    clockTimer_ = 0;
    accentTimer_ = 0;
    hihatTimer_ = 0;
    accentHoldSamples_ = gateDurationSamples_;
    hihatHoldSamples_  = gateDurationSamples_;
    
    usingExternalClock_ = false;
    externalClockTimeout_ = 0;
    mustTick_ = false;
    
    // Initialize DuoPulse v2 parameters
    anchorDensity_  = 0.5f;
    shimmerDensity_ = 0.5f;
    flux_           = 0.0f;
    fuse_           = 0.5f;
    anchorAccent_   = 0.5f;
    shimmerAccent_  = 0.5f;
    orbit_          = 0.5f;
    contour_        = 0.0f;
    terrain_        = 0.0f;
    loopLengthBars_ = 4;
    grid_           = 0.0f;
    swingTaste_     = 0.5f;
    gateTime_       = 0.2f;
    humanize_       = 0.0f;
    clockDiv_       = 0.5f;

    // Initialize swing state
    currentSwing_       = 0.5f;
    swingDelaySamples_  = 0;
    swingDelayCounter_  = 0;
    pendingAnchorTrig_  = false;
    pendingShimmerTrig_ = false;
    pendingAnchorVel_   = 0.0f;
    pendingShimmerVel_  = 0.0f;
    pendingClockTrig_   = false;
    stepDurationSamples_ = 0;

    // Initialize Orbit/Shadow state
    lastAnchorTrig_ = false;
    lastAnchorVel_  = 0.0f;

    // Initialize humanize RNG with step index for variety
    humanizeRngState_ = 0x12345678;

    // Initialize phrase position
    phrasePos_ = CalculatePhrasePosition(0, loopLengthBars_);

    // Initialize contour CV state
    anchorContourCV_  = 0.0f;
    shimmerContourCV_ = 0.0f;

    UpdateSwingParameters();
}

// === DuoPulse v2 Setters ===

// Performance Primary
void Sequencer::SetAnchorDensity(float value)
{
    anchorDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetShimmerDensity(float value)
{
    shimmerDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFlux(float value)
{
    flux_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetFuse(float value)
{
    fuse_ = Clamp(value, 0.0f, 1.0f);
}

// Performance Shift
void Sequencer::SetAnchorAccent(float value)
{
    anchorAccent_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetShimmerAccent(float value)
{
    shimmerAccent_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetOrbit(float value)
{
    orbit_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetContour(float value)
{
    contour_ = Clamp(value, 0.0f, 1.0f);
}

// Config Primary
void Sequencer::SetTerrain(float value)
{
    terrain_ = Clamp(value, 0.0f, 1.0f);
    UpdateSwingParameters();
}

void Sequencer::SetLength(int bars)
{
    if(bars < 1)
        bars = 1;
    if(bars > 16)
        bars = 16;
    loopLengthBars_ = bars;
}

void Sequencer::SetGrid(float value)
{
    grid_ = Clamp(value, 0.0f, 1.0f);
}

// Config Shift
void Sequencer::SetSwingTaste(float value)
{
    swingTaste_ = Clamp(value, 0.0f, 1.0f);
    UpdateSwingParameters();
}

void Sequencer::SetGateTime(float value)
{
    gateTime_ = Clamp(value, 0.0f, 1.0f);
    // Update gate duration in samples
    float gateMs         = kMinGateMs + (gateTime_ * (kMaxGateMs - kMinGateMs));
    gateDurationSamples_ = static_cast<int>(sampleRate_ * gateMs / 1000.0f);
    if(gateDurationSamples_ < 1)
        gateDurationSamples_ = 1;
}

void Sequencer::SetHumanize(float value)
{
    humanize_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetClockDiv(float value)
{
    clockDiv_ = Clamp(value, 0.0f, 1.0f);
}

// Legacy interface
void Sequencer::SetLowVariation(float value)
{
    // Map to flux (both variations combined into single flux control)
    SetFlux(value);
}

void Sequencer::SetHighVariation(float value)
{
    // Map to flux (both variations combined into single flux control)
    SetFlux(value);
}

void Sequencer::SetTempoControl(float value)
{
    float tempoControl = Clamp(value, 0.0f, 1.0f);
    if(std::abs(tempoControl - lastTempoControl_) > 0.01f)
    {
        float newBpm = kMinTempo + (tempoControl * (kMaxTempo - kMinTempo));
        SetBpm(newBpm);
        lastTempoControl_ = tempoControl;
    }
}

void Sequencer::TriggerTapTempo(uint32_t nowMs)
{
    if(lastTapTime_ != 0)
    {
        uint32_t interval = nowMs - lastTapTime_;
        if(interval > 100 && interval < 2000)
        {
            float newBpm = 60000.0f / static_cast<float>(interval);
            SetBpm(newBpm);
        }
    }
    lastTapTime_ = nowMs;
}

void Sequencer::TriggerReset()
{
    stepIndex_ = -1; // Next tick will be 0
    metro_.Reset();
}

void Sequencer::TriggerExternalClock()
{
    // 2 seconds timeout for external clock
    externalClockTimeout_ = static_cast<int>(sampleRate_ * 2.0f);
    usingExternalClock_ = true;
    mustTick_ = true;
}

std::array<float, 2> Sequencer::ProcessAudio()
{
    uint8_t tick = 0;

    if(usingExternalClock_)
    {
        if(mustTick_)
        {
            tick = 1;
            mustTick_ = false;
        }
        
        externalClockTimeout_--;
        if(externalClockTimeout_ <= 0)
        {
            usingExternalClock_ = false;
            metro_.Reset(); // Sync internal clock when fallback happens
        }
    }
    else
    {
        tick = metro_.Process();
    }

    if(tick)
    {
        // Handle Loop Length
        int effectiveLoopSteps = loopLengthBars_ * 16;
        if(effectiveLoopSteps > PatternGenerator::kPatternLength)
            effectiveLoopSteps = PatternGenerator::kPatternLength;

        stepIndex_ = (stepIndex_ + 1) % effectiveLoopSteps;

        // Update phrase position tracking
        phrasePos_ = CalculatePhrasePosition(stepIndex_, loopLengthBars_);

        // Apply flux to chaos modulators (flux controls variation for both voices)
        // Add phrase-based ghost boost
        float ghostBoost    = GetPhraseGhostBoost(phrasePos_);
        float effectiveFlux = Clamp(flux_ + ghostBoost, 0.0f, 1.0f);
        chaosLow_.SetAmount(effectiveFlux);
        chaosHigh_.SetAmount(effectiveFlux);
        
        bool trigs[3] = {false, false, false};
        
        const auto chaosSampleLow = chaosLow_.NextSample();
        const auto chaosSampleHigh = chaosHigh_.NextSample();

        bool kickTrig = false;
        bool snareTrig = false;
        bool hhTrig = false;
        
        float kickVel = 0.0f;
        float snareVel = 0.0f;
        float hhVel = 0.0f;

        if(forceNextTriggers_)
        {
            kickTrig = forcedTriggers_[0];
            snareTrig = forcedTriggers_[1];
            hhTrig = forcedTriggers_[2];
            
            // For forced triggers, assume standard velocity or accent
            kickVel = forcedKickAccent_ ? 1.0f : 0.8f;
            snareVel = 0.8f;
            hhVel = 0.8f;

            forceNextTriggers_ = false;
            forcedKickAccent_ = false;
        }
        else
        {
            // Apply Chaos to Terrain (Map X).
            float avgJitterX     = (chaosSampleLow.jitterX + chaosSampleHigh.jitterX) * 0.5f;
            float jitteredTerrain = Clamp(terrain_ + avgJitterX, 0.0f, 1.0f);

            // Apply fuse as density tilt: fuse < 0.5 boosts anchor, > 0.5 boosts shimmer
            float fuseBias     = (fuse_ - 0.5f) * 0.3f; // ±15% tilt
            float anchorDensMod  = Clamp(anchorDensity_ - fuseBias + chaosSampleLow.densityBias, 0.05f, 0.95f);
            float shimmerDensMod = Clamp(shimmerDensity_ + fuseBias + chaosSampleHigh.densityBias, 0.05f, 0.95f);

            patternGen_.GetTriggers(jitteredTerrain, stepIndex_, anchorDensMod, shimmerDensMod, trigs);
            
            kickTrig = trigs[0];
            snareTrig = trigs[1];
            hhTrig = trigs[2];

            // Get Levels for Velocity (Map Y fixed at 0.5)
            float mapY = 0.5f;
            kickVel  = static_cast<float>(patternGen_.GetLevel(jitteredTerrain, mapY, 0, stepIndex_)) / 255.0f;
            snareVel = static_cast<float>(patternGen_.GetLevel(jitteredTerrain, mapY, 1, stepIndex_)) / 255.0f;
            hhVel    = static_cast<float>(patternGen_.GetLevel(jitteredTerrain, mapY, 2, stepIndex_)) / 255.0f;

            // Apply phrase-based accent multiplier (strongest on downbeats)
            float accentMult = GetPhraseAccentMultiplier(phrasePos_);
            kickVel  = Clamp(kickVel * accentMult, 0.0f, 1.0f);
            snareVel = Clamp(snareVel * accentMult, 0.0f, 1.0f);
            hhVel    = Clamp(hhVel * accentMult, 0.0f, 1.0f);
        }

        // Apply Ghost Triggers to HH/Perc stream (High Variation)
        if(!hhTrig && chaosSampleHigh.ghostTrigger)
        {
            hhTrig = true;
            // Ghost triggers are usually quieter
            hhVel = 0.3f + (static_cast<float>(rand() % 100) / 200.0f);
        }

        // --- CV-Driven Fills (FLUX + Phrase Position) ---
        // High FLUX values add fill triggers, boosted in fill/build zones
        float phraseFillBoost = GetPhraseFillBoost(phrasePos_, terrain_);
        float effectiveFillFlux = Clamp(flux_ + phraseFillBoost, 0.0f, 1.0f);

        if(effectiveFillFlux >= kFluxFillThreshold)
        {
            // Check for anchor fill (kick fills)
            if(!kickTrig && ShouldTriggerFill(effectiveFillFlux, NextHumanizeRandom()))
            {
                kickTrig = true;
                kickVel  = CalculateFillVelocity(effectiveFillFlux, NextHumanizeRandom());
            }

            // Check for shimmer fill (snare fills)
            if(!snareTrig && ShouldTriggerFill(effectiveFillFlux, NextHumanizeRandom()))
            {
                snareTrig = true;
                snareVel  = CalculateFillVelocity(effectiveFillFlux, NextHumanizeRandom());
            }
        }

        // --- Orbit Voice Relationship Logic ---
        OrbitMode orbitMode = GetOrbitMode(orbit_);

        // Gate 0 (Anchor/Low/Kick) - determined by pattern
        bool  gate0 = kickTrig;
        float vel0  = kickTrig ? kickVel : 0.0f;

        // Gate 1 (Shimmer/High/Snare) - affected by Orbit mode
        bool  gate1 = false;
        float vel1  = 0.0f;

        switch(orbitMode)
        {
            case OrbitMode::Interlock:
            {
                // Shimmer fills gaps - when anchor fires, reduce shimmer;
                // when anchor silent, boost shimmer
                float interlockMod = GetInterlockModifier(gate0, orbit_);
                float modifiedShimmerDens =
                    Clamp(shimmerDensity_ + interlockMod, 0.0f, 1.0f);

                // Re-evaluate shimmer trigger with modified density
                // Simple approach: if snare was going to trigger, apply probability check
                if(snareTrig)
                {
                    // Interlock reduces probability when anchor fires
                    float prob = modifiedShimmerDens;
                    gate1      = (static_cast<float>(rand() % 100) / 100.0f) < prob;
                    vel1       = gate1 ? snareVel : 0.0f;
                }
                else if(!gate0 && interlockMod > 0.0f)
                {
                    // Anchor silent - chance to add shimmer hit
                    float prob = interlockMod;
                    if((static_cast<float>(rand() % 100) / 100.0f) < prob)
                    {
                        gate1 = true;
                        vel1  = snareVel > 0.0f ? snareVel : 0.6f;
                    }
                }
                break;
            }

            case OrbitMode::Free:
                // Independent patterns, no collision logic (default behavior)
                gate1 = snareTrig;
                vel1  = snareTrig ? snareVel : 0.0f;
                break;

            case OrbitMode::Shadow:
                // Shimmer echoes anchor with 1-step delay at 70% velocity
                if(lastAnchorTrig_)
                {
                    gate1 = true;
                    vel1  = lastAnchorVel_ * 0.7f; // 70% velocity
                }
                break;
        }

        // Store current anchor state for next step's Shadow mode
        lastAnchorTrig_ = gate0;
        lastAnchorVel_  = vel0;

        // Route HH/Perc based on Grid (pattern selection also affects routing)
        if(hhTrig)
        {
            if(grid_ < 0.5f)
            {
                // Route to Anchor (add tom/perc flavor to kick channel)
                gate0 = true;
                vel0  = std::max(vel0, hhVel);
            }
            else
            {
                // Route to Shimmer (add hh/perc flavor to snare channel)
                gate1 = true;
                vel1  = std::max(vel1, hhVel);
            }
        }

        // --- Swing + Humanize Application ---
        // Off-beats (odd steps) get delayed according to swing amount
        // Anchor receives 70% of swing, Shimmer receives 100%
        // Humanize adds random jitter to all triggers
        bool isOffBeat = IsOffBeat(stepIndex_);

        // Calculate humanize jitter (applied to all triggers)
        float effectiveHumanize = CalculateEffectiveHumanize(humanize_, terrain_);
        int   humanizeJitter    = 0;
        if(effectiveHumanize > 0.0f && (gate0 || gate1))
        {
            float randomVal  = NextHumanizeRandom();
            humanizeJitter   = CalculateHumanizeJitterSamples(effectiveHumanize, sampleRate_, randomVal);
        }

        // Calculate total delay (swing + humanize jitter)
        // Note: humanize jitter can be negative (early), but we clamp to 0 minimum
        int totalDelay = 0;
        if(isOffBeat && swingDelaySamples_ > 0)
        {
            totalDelay = swingDelaySamples_ + humanizeJitter;
        }
        else
        {
            totalDelay = humanizeJitter;
        }
        if(totalDelay < 0)
            totalDelay = 0;

        if(totalDelay > 0)
        {
            // Queue triggers for delayed firing
            swingDelayCounter_ = totalDelay;

            if(gate0)
            {
                pendingAnchorTrig_ = true;
                pendingAnchorVel_  = vel0;
            }
            if(gate1)
            {
                pendingShimmerTrig_ = true;
                pendingShimmerVel_  = vel1;
            }
            pendingClockTrig_ = true; // Clock also follows timing
        }
        else
        {
            // No delay - fire immediately
            TriggerClock();

            if(gate0)
            {
                TriggerGate(0);
                accentTimer_     = accentHoldSamples_;
                outputLevels_[0] = vel0;
            }

            if(gate1)
            {
                TriggerGate(1);
                hihatTimer_      = hihatHoldSamples_;
                outputLevels_[1] = vel1;
            }
        }
    }

    // Process swing delayed triggers (must run every sample)
    ProcessSwingDelay();

    ProcessGates();

    // Apply Contour mode to CV outputs
    // For now, use Velocity mode (direct velocity output) as default
    // Contour modes will be fully implemented in a follow-up
    // This preserves backward compatibility with existing tests
    
    float out1 = accentTimer_ > 0 ? outputLevels_[0] : 0.0f;
    float out2 = hihatTimer_ > 0 ? outputLevels_[1] : 0.0f;

    // Apply contour processing only when contour parameter is > 0
    // (Velocity mode at contour=0 is pass-through)
    if(contour_ > 0.0f)
    {
        ContourMode cmode = GetContourMode(contour_);

        // Detect new triggers (timer just started)
        bool anchorTriggered  = (accentTimer_ == accentHoldSamples_);
        bool shimmerTriggered = (hihatTimer_ == hihatHoldSamples_);

        // Only generate random values when needed to preserve RNG state
        float randVal1 = (anchorTriggered || shimmerTriggered) ? NextHumanizeRandom() : 0.0f;
        float randVal2 = (anchorTriggered || shimmerTriggered) ? NextHumanizeRandom() : 0.0f;

        anchorContourCV_ = CalculateContourCV(cmode, outputLevels_[0], randVal1,
                                              anchorContourCV_, anchorTriggered);
        shimmerContourCV_ = CalculateContourCV(cmode, outputLevels_[1], randVal2,
                                               shimmerContourCV_, shimmerTriggered);

        out1 = accentTimer_ > 0 ? anchorContourCV_ : 0.0f;
        out2 = hihatTimer_ > 0 ? shimmerContourCV_ : 0.0f;
    }

    return {out1, out2};
}

bool Sequencer::IsGateHigh(int channel) const
{
    if(channel >= 0 && channel < 2)
    {
        return gateTimers_[channel] > 0;
    }
    return false;
}

void Sequencer::SetBpm(float bpm)
{
    currentBpm_ = Clamp(bpm, kMinTempo, kMaxTempo);
    metro_.SetFreq(currentBpm_ / 60.0f * 4.0f);
    // Only update swing if sample rate is initialized (avoid issues during Init)
    if(sampleRate_ > 0.0f)
    {
        UpdateSwingParameters();
    }
}

void Sequencer::TriggerGate(int channel)
{
    if(channel < 2)
    {
        gateTimers_[channel] = gateDurationSamples_;
    }
}

void Sequencer::TriggerClock()
{
    clockTimer_ = clockDurationSamples_;
}

void Sequencer::ProcessGates()
{
    for(int& timer : gateTimers_)
    {
        if(timer > 0)
        {
            --timer;
        }
    }
    if(clockTimer_ > 0)
    {
        --clockTimer_;
    }
    if(accentTimer_ > 0)
    {
        --accentTimer_;
    }
    if(hihatTimer_ > 0)
    {
        --hihatTimer_;
    }
}

void Sequencer::ForceNextStepTriggers(bool kick, bool snare, bool hh, bool kickAccent)
{
    forcedTriggers_[0] = kick;
    forcedTriggers_[1] = snare;
    forcedTriggers_[2] = hh;
    forceNextTriggers_ = true;
    forcedKickAccent_ = kickAccent;
}

void Sequencer::SetAccentHoldMs(float milliseconds)
{
    accentHoldSamples_ = HoldMsToSamples(milliseconds);
}

void Sequencer::SetHihatHoldMs(float milliseconds)
{
    hihatHoldSamples_ = HoldMsToSamples(milliseconds);
}

int Sequencer::HoldMsToSamples(float milliseconds) const
{
    const float clampedMs = milliseconds < 0.5f ? 0.5f : (milliseconds > 2000.0f ? 2000.0f : milliseconds);
    const float samples   = (clampedMs / 1000.0f) * sampleRate_;
    int         asInt     = static_cast<int>(samples);
    return asInt < 1 ? 1 : asInt;
}

void Sequencer::UpdateSwingParameters()
{
    // Calculate current swing percentage from terrain (genre) and taste
    currentSwing_ = CalculateSwing(terrain_, swingTaste_);

    // Calculate step duration in samples (16th note at current BPM)
    // BPM = beats per minute, 4 16th notes per beat
    // stepDuration = 60 / (BPM * 4) seconds = sampleRate * 60 / (BPM * 4) samples
    if(currentBpm_ > 0.0f)
    {
        stepDurationSamples_ = static_cast<int>(sampleRate_ * 60.0f / (currentBpm_ * 4.0f));
    }

    // Calculate swing delay for off-beats
    swingDelaySamples_ = CalculateSwingDelaySamples(currentSwing_, stepDurationSamples_);
}

void Sequencer::ProcessSwingDelay()
{
    // Process any pending swung triggers
    if(swingDelayCounter_ > 0)
    {
        swingDelayCounter_--;

        // When counter reaches 0, fire the pending triggers
        if(swingDelayCounter_ == 0)
        {
            if(pendingAnchorTrig_)
            {
                TriggerGate(0);
                accentTimer_     = accentHoldSamples_;
                outputLevels_[0] = pendingAnchorVel_;
                pendingAnchorTrig_ = false;
            }

            if(pendingShimmerTrig_)
            {
                TriggerGate(1);
                hihatTimer_      = hihatHoldSamples_;
                outputLevels_[1] = pendingShimmerVel_;
                pendingShimmerTrig_ = false;
            }

            if(pendingClockTrig_)
            {
                TriggerClock();
                pendingClockTrig_ = false;
            }
        }
    }
}

float Sequencer::NextHumanizeRandom()
{
    // Simple xorshift RNG for humanize jitter
    humanizeRngState_ ^= humanizeRngState_ << 13;
    humanizeRngState_ ^= humanizeRngState_ >> 17;
    humanizeRngState_ ^= humanizeRngState_ << 5;
    return static_cast<float>(humanizeRngState_ & 0xFFFF) / 65535.0f;
}

} // namespace daisysp_idm_grids
