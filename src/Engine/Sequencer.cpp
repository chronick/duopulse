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

void Sequencer::Init(float sampleRate)
{
    sampleRate_ = sampleRate;

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
    
    // Initialize DuoPulse v3 parameters
    anchorDensity_  = 0.5f;
    shimmerDensity_ = 0.5f;
    broken_         = 0.0f;
    drift_          = 0.0f;
    fuse_           = 0.5f;
    loopLengthBars_ = 4;
    couple_         = 0.5f;
    ratchet_        = 0.0f;
    anchorAccent_   = 0.5f;
    shimmerAccent_  = 0.5f;
    contour_        = 0.0f;
    swingTaste_     = 0.5f;
    gateTime_       = 0.2f;
    humanize_       = 0.0f;
    clockDiv_       = 0.5f;
    
    // Initialize deprecated v2 parameters (kept for compatibility)
    flux_    = 0.0f;
    orbit_   = 0.5f;
    terrain_ = 0.0f;
    grid_    = 0.0f;

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

    // Initialize pattern system
    currentPatternIndex_ = 0;

    // Initialize clock division
    clockDivCounter_ = 0;

    // Initialize ratchet state
    ratchetTimer_ = 0;
    ratchetAnchorPending_ = false;
    ratchetShimmerPending_ = false;
    ratchetAnchorVel_ = 0.0f;
    ratchetShimmerVel_ = 0.0f;

#ifdef USE_PULSE_FIELD_V3
    // Initialize v3 Pulse Field state with a seed based on initial entropy
    pulseFieldState_.Init(0x44554F50); // "DUOP" as seed
#endif

    UpdateSwingParameters();
}

// === DuoPulse v3 Setters ===

// Performance Primary
void Sequencer::SetAnchorDensity(float value)
{
    anchorDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetShimmerDensity(float value)
{
    shimmerDensity_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetBroken(float value)
{
    broken_ = Clamp(value, 0.0f, 1.0f);
    // Keep deprecated parameters in sync for backward compatibility
    flux_ = broken_;
    terrain_ = broken_; // v3: BROKEN controls swing via terrain internally
    UpdateSwingParameters();
}

void Sequencer::SetDrift(float value)
{
    drift_ = Clamp(value, 0.0f, 1.0f);
}

// Performance Shift
void Sequencer::SetFuse(float value)
{
    fuse_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetLength(int bars)
{
    if(bars < 1)
        bars = 1;
    if(bars > 16)
        bars = 16;
    loopLengthBars_ = bars;
}

void Sequencer::SetCouple(float value)
{
    couple_ = Clamp(value, 0.0f, 1.0f);
    orbit_ = couple_; // Keep deprecated parameter in sync
}

void Sequencer::SetRatchet(float value)
{
    ratchet_ = Clamp(value, 0.0f, 1.0f);
}

// Config Primary
void Sequencer::SetAnchorAccent(float value)
{
    anchorAccent_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetShimmerAccent(float value)
{
    shimmerAccent_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetContour(float value)
{
    contour_ = Clamp(value, 0.0f, 1.0f);
}

// Config Shift
void Sequencer::SetSwingTaste(float value)
{
    swingTaste_ = Clamp(value, 0.0f, 1.0f);
    UpdateSwingParameters();
}

// === Deprecated v2 Setters (for backward compatibility) ===

void Sequencer::SetFlux(float value)
{
    // Map to BROKEN for backward compatibility
    SetBroken(value);
}

void Sequencer::SetOrbit(float value)
{
    // Map to COUPLE for backward compatibility
    SetCouple(value);
}

void Sequencer::SetTerrain(float value)
{
    // Deprecated: genre now emerges from BROKEN
    // Keep the parameter but don't use it actively
    terrain_ = Clamp(value, 0.0f, 1.0f);
}

void Sequencer::SetGrid(float value)
{
    // Deprecated: no pattern selection in v3
    // Keep the parameter but don't use it actively
    grid_ = Clamp(value, 0.0f, 1.0f);
    currentPatternIndex_ = GetPatternIndex(grid_);
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

int Sequencer::GetClockDivisionFactor() const
{
    // Clock division/multiplication based on clockDiv_ parameter
    // | Range     | Division |
    // |-----------|----------|
    // | 0-20%     | ÷4       |
    // | 20-40%    | ÷2       |
    // | 40-60%    | ×1       |
    // | 60-80%    | ×2       |
    // | 80-100%   | ×4       |
    // Returns: positive for division (4,2,1), negative for multiplication (-2,-4)
    if(clockDiv_ < 0.2f)
        return 4;  // ÷4: output every 4 steps
    if(clockDiv_ < 0.4f)
        return 2;  // ÷2: output every 2 steps
    if(clockDiv_ < 0.6f)
        return 1;  // ×1: output every step
    if(clockDiv_ < 0.8f)
        return -2; // ×2: output twice per step
    return -4;     // ×4: output four times per step
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
        if(effectiveLoopSteps > kPatternSteps)
            effectiveLoopSteps = kPatternSteps;

        stepIndex_ = (stepIndex_ + 1) % effectiveLoopSteps;

#ifdef USE_PULSE_FIELD_V3
        // Phrase reset: regenerate loopSeed_ for drifting pattern elements
        // This causes DRIFT-affected steps to produce different patterns each loop
        if(stepIndex_ == 0)
        {
            pulseFieldState_.OnPhraseReset();
        }
#endif

        // Update phrase position tracking
        phrasePos_ = CalculatePhrasePosition(stepIndex_, loopLengthBars_);

        // Apply flux to chaos modulators (flux controls variation for both voices)
        // Add phrase-based ghost boost
        float ghostBoost    = GetPhraseGhostBoost(phrasePos_);
        float effectiveFlux = Clamp(flux_ + ghostBoost, 0.0f, 1.0f);
        chaosLow_.SetAmount(effectiveFlux);
        chaosHigh_.SetAmount(effectiveFlux);
        
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
#ifdef USE_PULSE_FIELD_V3
            // === DuoPulse v3: Weighted Pulse Field Algorithm ===
            // FUSE is applied inside GetPulseFieldTriggers
            // 
            // v3 Critical Rule: DENSITY=0 must be absolute silence
            // v3 Critical Rule: DRIFT=0 must produce zero variation (identical every loop)
            // 
            // At DRIFT=0, skip chaos densityBias to ensure deterministic pattern.
            // ChaosModulator uses non-deterministic RNG that would break DRIFT=0 invariant.
            float anchorDensMod  = anchorDensity_;
            float shimmerDensMod = shimmerDensity_;
            
            if(drift_ > 0.0f)
            {
                // Only add chaos variation when DRIFT allows pattern evolution
                anchorDensMod  += chaosSampleLow.densityBias;
                shimmerDensMod += chaosSampleHigh.densityBias;
            }
            
            // Clamp to valid range (0.0f floor preserves DENSITY=0 = silence)
            anchorDensMod  = Clamp(anchorDensMod, 0.0f, 0.95f);
            shimmerDensMod = Clamp(shimmerDensMod, 0.0f, 0.95f);

            // Use PulseField algorithm with BROKEN/DRIFT controls
            GetPulseFieldTriggers(stepIndex_, anchorDensMod, shimmerDensMod,
                                  kickTrig, snareTrig, kickVel, snareVel);

            // Apply BROKEN effects (step displacement already handled in trigger generation)
            int displaceStep = stepIndex_;
            ApplyBrokenEffects(displaceStep, kickVel, snareVel, kickTrig, snareTrig);

            // No separate HH in v3 - shimmer handles all upper percussion
            hhTrig = false;
            hhVel  = 0.0f;

            // Phrase accent is already applied inside GetPulseFieldTriggers
#else
            // === DuoPulse v2: PatternSkeleton System ===
            // Apply fuse as density tilt: fuse < 0.5 boosts anchor, > 0.5 boosts shimmer
            float fuseBias     = (fuse_ - 0.5f) * 0.3f; // ±15% tilt
            float anchorDensMod  = Clamp(anchorDensity_ - fuseBias + chaosSampleLow.densityBias, 0.0f, 0.95f);
            float shimmerDensMod = Clamp(shimmerDensity_ + fuseBias + chaosSampleHigh.densityBias, 0.0f, 0.95f);

            // Use PatternSkeleton system with density threshold
            GetSkeletonTriggers(stepIndex_, anchorDensMod, shimmerDensMod,
                                kickTrig, snareTrig, kickVel, snareVel);
            
            // No separate HH in skeleton patterns - it's combined into shimmer
            hhTrig = false;
            hhVel = 0.0f;

            // Apply phrase-based accent multiplier (strongest on downbeats)
            float accentMult = GetPhraseAccentMultiplier(phrasePos_);
            kickVel  = Clamp(kickVel * accentMult, 0.0f, 1.0f);
            snareVel = Clamp(snareVel * accentMult, 0.0f, 1.0f);
#endif
        }

        // Apply Ghost Triggers to HH/Perc stream (High Variation)
        if(!hhTrig && chaosSampleHigh.ghostTrigger)
        {
            hhTrig = true;
            // Ghost triggers are usually quieter
            hhVel = 0.3f + (static_cast<float>(rand() % 100) / 200.0f);
        }

#ifndef USE_PULSE_FIELD_V3
        // --- CV-Driven Fills (v2: FLUX + Phrase Position) ---
        // v3 handles fills through phrase-aware weight boosts in the pulse field algorithm
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
#endif

#ifdef USE_PULSE_FIELD_V3
        // === v3: Voice relationship already handled by COUPLE in GetPulseFieldTriggers ===
        // Direct assignment from trigger results
        bool  gate0 = kickTrig;
        float vel0  = kickTrig ? kickVel : 0.0f;
        bool  gate1 = snareTrig;
        float vel1  = snareTrig ? snareVel : 0.0f;

        // Store current anchor state (for potential future Shadow-like features)
        lastAnchorTrig_ = gate0;
        lastAnchorVel_  = vel0;
#else
        // --- v2: Orbit Voice Relationship Logic ---
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
#endif

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
#ifdef USE_PULSE_FIELD_V3
        // v3: BROKEN adds jitter on top of humanize_ parameter
        float brokenJitterMs = GetJitterMsFromBroken(broken_);
        // humanize_ adds up to 10ms, BROKEN adds up to 12ms more
        float totalJitterMs  = (humanize_ * 10.0f) + brokenJitterMs;
        float effectiveHumanize = totalJitterMs / 22.0f; // Normalize to 0-1 range (max 22ms total)
#else
        float effectiveHumanize = CalculateEffectiveHumanize(humanize_, terrain_);
#endif
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

#ifdef USE_PULSE_FIELD_V3
        // === Ratchet Scheduling (32nd note subdivisions) ===
        // Ratchets fire mid-step (half of stepDurationSamples_) after primary trigger
        // Conditions for ratcheting:
        // - RATCHET > 50% (threshold for 32nd subdivisions)
        // - DRIFT > 0 (no ratchets when pattern is fully locked)
        // - In fill zone (or mid-phrase with high RATCHET)
        // - Primary trigger fired this step
        bool shouldRatchet = (ratchet_ > 0.5f) && (drift_ > 0.0f) &&
                             (phrasePos_.isFillZone || (phrasePos_.isMidPhrase && ratchet_ > 0.75f));
        
        if(shouldRatchet && (gate0 || gate1) && stepDurationSamples_ > 0)
        {
            // Calculate ratchet probability based on position and RATCHET level
            // Higher toward phrase end, scales with RATCHET
            float ratchetProb = (ratchet_ - 0.5f) * 2.0f; // 0 at 50%, 1 at 100%
            if(phrasePos_.isFillZone)
            {
                // Increase probability toward phrase end
                float fillProgress = (phrasePos_.phraseProgress - 0.75f) * 4.0f;
                ratchetProb *= (0.5f + fillProgress * 0.5f); // 50-100% of base prob
            }
            else
            {
                ratchetProb *= 0.3f; // Lower probability in mid-phrase
            }
            
            // Apply DRIFT gating
            ratchetProb *= drift_;
            
            // Check if ratchet should fire (use RNG)
            if(NextHumanizeRandom() < ratchetProb)
            {
                // Schedule ratchet for half-step later
                ratchetTimer_ = stepDurationSamples_ / 2;
                
                // Ratchet follows primary trigger with reduced velocity
                ratchetAnchorPending_  = gate0;
                ratchetShimmerPending_ = gate1;
                ratchetAnchorVel_      = vel0 * 0.7f;  // 70% velocity
                ratchetShimmerVel_     = vel1 * 0.7f;
            }
        }
#endif
    }

    // Process swing delayed triggers (must run every sample)
    ProcessSwingDelay();
    
#ifdef USE_PULSE_FIELD_V3
    // Process ratchet triggers (32nd note subdivisions)
    ProcessRatchet();
#endif

    ProcessGates();

    // Apply Contour mode to CV outputs
    // When contour=0: simple timer-gated velocity output (for gate/trigger use)
    // When contour>0: contour mode processing with sustained CV between triggers
    
    float out1, out2;

    if(contour_ > 0.0f)
    {
        // Contour modes: CV is sustained/decayed according to mode
        // NOT gated by timer - the CalculateContourCV function handles decay/hold
        ContourMode cmode = GetContourMode(contour_);

        // Detect new triggers (timer just started at max value)
        bool anchorTriggered  = (accentTimer_ == accentHoldSamples_);
        bool shimmerTriggered = (hihatTimer_ == hihatHoldSamples_);

        // Only generate random values when triggered to preserve RNG state
        float randVal1 = anchorTriggered ? NextHumanizeRandom() : 0.0f;
        float randVal2 = shimmerTriggered ? NextHumanizeRandom() : 0.0f;

        // Update contour CV state (handles decay/hold per mode)
        anchorContourCV_ = CalculateContourCV(cmode, outputLevels_[0], randVal1,
                                              anchorContourCV_, anchorTriggered);
        shimmerContourCV_ = CalculateContourCV(cmode, outputLevels_[1], randVal2,
                                               shimmerContourCV_, shimmerTriggered);

        // Output the contour CV directly - it sustains until next trigger
        // or decays gradually according to the mode
        out1 = anchorContourCV_;
        out2 = shimmerContourCV_;
    }
    else
    {
        // Default mode (contour=0): simple timer-gated velocity
        // CV is high during gate duration, then goes to 0
        out1 = accentTimer_ > 0 ? outputLevels_[0] : 0.0f;
        out2 = hihatTimer_ > 0 ? outputLevels_[1] : 0.0f;
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
    // Apply clock division
    int divFactor = GetClockDivisionFactor();

    if(divFactor > 1)
    {
        // Division mode (÷2, ÷4): Only fire every N steps
        clockDivCounter_++;
        if(clockDivCounter_ >= divFactor)
        {
            clockDivCounter_ = 0;
            clockTimer_ = clockDurationSamples_;
        }
        // Otherwise skip this clock trigger
    }
    else if(divFactor < 0)
    {
        // Multiplication mode (×2, ×4): Fire clock now
        // Note: True multiplication would require sub-step timing.
        // For now, we fire the clock on every step (same as ×1).
        // Future enhancement: Add a fast timer for ×2/×4 sub-pulses.
        clockTimer_ = clockDurationSamples_;
    }
    else
    {
        // Unity mode (×1): Fire every step
        clockTimer_ = clockDurationSamples_;
    }
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
#ifdef USE_PULSE_FIELD_V3
    // v3: Swing is derived from BROKEN parameter, fine-tuned by swingTaste_
    // GetSwingFromBroken returns 0.50-0.66 based on BROKEN level
    float baseSwing = GetSwingFromBroken(broken_);
    
    // swingTaste_ allows ±4% adjustment within the genre's range
    // swingTaste=0.5 = no change, 0=reduce swing, 1=increase swing
    float tasteAdjust = (swingTaste_ - 0.5f) * 0.08f; // ±4%
    currentSwing_ = Clamp(baseSwing + tasteAdjust, 0.5f, 0.70f);
#else
    // v2: Calculate swing from terrain (genre) and taste
    currentSwing_ = CalculateSwing(terrain_, swingTaste_);
#endif

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

void Sequencer::GetSkeletonTriggers(int step, float anchorDens, float shimmerDens,
                                    bool& anchorTrig, bool& shimmerTrig,
                                    float& anchorVel, float& shimmerVel)
{
    // Get the current pattern
    const PatternSkeleton& pattern = GetPattern(currentPatternIndex_);

    // Wrap step to pattern length (32 steps)
    int wrappedStep = step % kPatternSteps;

    // Apply density threshold to determine if each step fires
    // Low density = only high-intensity steps fire
    // High density = all steps including ghosts fire
    anchorTrig = ShouldStepFire(pattern.anchorIntensity, wrappedStep, anchorDens);
    shimmerTrig = ShouldStepFire(pattern.shimmerIntensity, wrappedStep, shimmerDens);

    // Get intensity levels for potential ghost note generation
    uint8_t anchorIntensity = GetStepIntensity(pattern.anchorIntensity, wrappedStep);
    uint8_t shimmerIntensity = GetStepIntensity(pattern.shimmerIntensity, wrappedStep);
    IntensityLevel anchorLevel = GetIntensityLevel(anchorIntensity);
    IntensityLevel shimmerLevel = GetIntensityLevel(shimmerIntensity);

    // FLUX probabilistic ghost note generation
    // If a ghost-level step (intensity 1-4) didn't fire via density, FLUX can trigger it
    if(!anchorTrig && anchorLevel == IntensityLevel::Ghost && flux_ > 0.0f)
    {
        if(ShouldTriggerGhost(flux_, NextHumanizeRandom()))
        {
            anchorTrig = true;
        }
    }
    if(!shimmerTrig && shimmerLevel == IntensityLevel::Ghost && flux_ > 0.0f)
    {
        if(ShouldTriggerGhost(flux_, NextHumanizeRandom()))
        {
            shimmerTrig = true;
        }
    }

    // Get velocity from step intensity
    if(anchorTrig)
    {
        anchorVel = IntensityToVelocity(anchorIntensity);

        // Apply accent parameter - boosts velocity for accent-eligible steps
        if(IsAccentEligible(pattern.accentMask, wrappedStep))
        {
            // Accent parameter scales the boost (0.5 = neutral, 1.0 = max accent)
            float accentBoost = (anchorAccent_ - 0.5f) * 0.4f; // ±0.2 range
            anchorVel = Clamp(anchorVel + accentBoost, 0.3f, 1.0f);
        }

        // Apply FLUX velocity jitter (up to ±20%)
        if(flux_ > 0.0f)
        {
            anchorVel = ApplyVelocityJitter(anchorVel, flux_, NextHumanizeRandom());
        }
    }
    else
    {
        anchorVel = 0.0f;
    }

    if(shimmerTrig)
    {
        shimmerVel = IntensityToVelocity(shimmerIntensity);

        // Apply accent parameter
        if(IsAccentEligible(pattern.accentMask, wrappedStep))
        {
            float accentBoost = (shimmerAccent_ - 0.5f) * 0.4f;
            shimmerVel = Clamp(shimmerVel + accentBoost, 0.3f, 1.0f);
        }

        // Apply FLUX velocity jitter
        if(flux_ > 0.0f)
        {
            shimmerVel = ApplyVelocityJitter(shimmerVel, flux_, NextHumanizeRandom());
        }
    }
    else
    {
        shimmerVel = 0.0f;
    }
}

#ifdef USE_PULSE_FIELD_V3

void Sequencer::GetPulseFieldTriggers(int step, float anchorDens, float shimmerDens,
                                      bool& anchorTrig, bool& shimmerTrig,
                                      float& anchorVel, float& shimmerVel)
{
    // Wrap step to pattern length (32 steps)
    int wrappedStep = step % kPulseFieldSteps;

    // Get effective BROKEN with phrase modulation (boost in fill zones)
    float effectiveBroken = GetEffectiveBroken(broken_, phrasePos_);

    // Apply FUSE energy balance (modifies densities in place)
    float fusedAnchorDens  = anchorDens;
    float fusedShimmerDens = shimmerDens;
    ApplyFuse(fuse_, fusedAnchorDens, fusedShimmerDens);
    
    // Apply fill zone density boost based on DRIFT × RATCHET interaction
    // DRIFT gates fill probability, RATCHET controls intensity
    float fillBoost = GetPhraseWeightBoostWithRatchet(phrasePos_, broken_, drift_, ratchet_);
    if(fillBoost > 0.0f)
    {
        // Boost densities in fill zones (respecting DENSITY=0 invariant)
        if(fusedAnchorDens > 0.0f)
            fusedAnchorDens = Clamp(fusedAnchorDens + fillBoost, 0.0f, 0.95f);
        if(fusedShimmerDens > 0.0f)
            fusedShimmerDens = Clamp(fusedShimmerDens + fillBoost, 0.0f, 0.95f);
    }

    // Get base triggers using the weighted pulse field algorithm with DRIFT
    daisysp_idm_grids::GetPulseFieldTriggers(
        wrappedStep,
        fusedAnchorDens,
        fusedShimmerDens,
        effectiveBroken,
        drift_,
        pulseFieldState_,
        anchorTrig,
        shimmerTrig);

    // Apply COUPLE interlock (suppresses collisions, fills gaps)
    // Needs a seed for deterministic randomness
    // Pass fusedShimmerDens to enforce DENSITY=0 invariant (no gap-fill when silent)
    uint32_t coupleSeed = pulseFieldState_.patternSeed_ ^ 0x434F5550; // "COUP"
    ApplyCouple(couple_, anchorTrig, shimmerTrig, shimmerVel, coupleSeed, wrappedStep, fusedShimmerDens);

    // Calculate base velocities from weight tables
    if(anchorTrig)
    {
        // Base velocity from weight (higher weight = stronger hit)
        float weight = GetStepWeight(wrappedStep, true);
        anchorVel    = 0.6f + weight * 0.4f; // 0.6 to 1.0 range

        // Apply phrase accent with RATCHET-enhanced resolution accent
        anchorVel *= GetPhraseAccentWithRatchet(phrasePos_, ratchet_);

        // Apply accent parameter - boosts velocity for strong positions
        if(weight >= 0.7f)
        {
            float accentBoost = (anchorAccent_ - 0.5f) * 0.4f;
            anchorVel         = Clamp(anchorVel + accentBoost, 0.3f, 1.0f);
        }

        // Apply BROKEN velocity variation
        anchorVel = GetVelocityWithVariation(anchorVel, effectiveBroken,
                                             pulseFieldState_.patternSeed_, wrappedStep);

        // Clamp to valid range
        anchorVel = Clamp(anchorVel, 0.2f, 1.0f);
    }
    else
    {
        anchorVel = 0.0f;
    }

    if(shimmerTrig)
    {
        // Base velocity from weight (check if already set by COUPLE gap-fill)
        if(shimmerVel <= 0.0f)
        {
            float weight = GetStepWeight(wrappedStep, false);
            shimmerVel   = 0.6f + weight * 0.4f;
        }

        // Apply phrase accent with RATCHET-enhanced resolution accent
        shimmerVel *= GetPhraseAccentWithRatchet(phrasePos_, ratchet_);

        // Apply accent parameter for strong positions
        float weight = GetStepWeight(wrappedStep, false);
        if(weight >= 0.7f)
        {
            float accentBoost = (shimmerAccent_ - 0.5f) * 0.4f;
            shimmerVel        = Clamp(shimmerVel + accentBoost, 0.3f, 1.0f);
        }

        // Apply BROKEN velocity variation (use different hash offset for shimmer)
        shimmerVel = GetVelocityWithVariation(shimmerVel, effectiveBroken,
                                              pulseFieldState_.patternSeed_ ^ 0x5348494D, // "SHIM"
                                              wrappedStep);

        // Clamp to valid range
        shimmerVel = Clamp(shimmerVel, 0.2f, 1.0f);
    }
    else
    {
        shimmerVel = 0.0f;
    }
}

void Sequencer::ApplyBrokenEffects(int& step, float& anchorVel, float& shimmerVel,
                                   bool anchorTrig, bool shimmerTrig)
{
    // Get effective BROKEN with phrase modulation
    float effectiveBroken = GetEffectiveBroken(broken_, phrasePos_);

    // Effect 1: Step Displacement (only at BROKEN > 50%)
    if(effectiveBroken > 0.5f)
    {
        step = GetDisplacedStep(step, effectiveBroken, pulseFieldState_.patternSeed_);
    }

    // Effect 2: Micro-timing jitter (handled in ProcessAudio via humanize system)
    // The jitter from BROKEN adds to the humanize_ parameter
    // This is done in ProcessAudio where timing is applied

    // Velocity variation is already applied in GetPulseFieldTriggers
    // Swing is already calculated from BROKEN in UpdateSwingParameters
}

void Sequencer::ProcessRatchet()
{
    // Process pending ratchet triggers (32nd note subdivisions)
    // Ratchets are scheduled to fire halfway through a step
    if(ratchetTimer_ > 0)
    {
        ratchetTimer_--;
        
        if(ratchetTimer_ == 0)
        {
            // Fire ratchet triggers
            if(ratchetAnchorPending_)
            {
                TriggerGate(0);
                accentTimer_     = accentHoldSamples_;
                outputLevels_[0] = ratchetAnchorVel_;
                ratchetAnchorPending_ = false;
            }
            
            if(ratchetShimmerPending_)
            {
                TriggerGate(1);
                hihatTimer_      = hihatHoldSamples_;
                outputLevels_[1] = ratchetShimmerVel_;
                ratchetShimmerPending_ = false;
            }
        }
    }
}

#endif // USE_PULSE_FIELD_V3

} // namespace daisysp_idm_grids
