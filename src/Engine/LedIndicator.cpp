#include "LedIndicator.h"
#include <cmath>

#ifndef HOST_BUILD
#include "daisy_patch_sm.h"
#endif

namespace daisysp_idm_grids
{

void LedIndicator::Init(float sampleRateHz)
{
    sampleRate_            = sampleRateHz;
    msPerSample_           = 1000.0f / sampleRateHz;
    timeMs_                = 0.0f;
    anchorTriggerTimeMs_   = -1000.0f;
    shimmerTriggerTimeMs_  = -1000.0f;
    flashStartTimeMs_      = -1000.0f;
    seed_                  = 12345;
}

float LedIndicator::Process(const LedState& state)
{
    timeMs_ += msPerSample_;
    
    // Handle trigger events (record timing)
    if(state.anchorTriggered)
    {
        anchorTriggerTimeMs_ = timeMs_;
    }
    if(state.shimmerTriggered)
    {
        shimmerTriggerTimeMs_ = timeMs_;
    }
    
    // Handle flash events (mode change, reset, reseed)
    if(state.event != LedEvent::NONE)
    {
        flashStartTimeMs_ = timeMs_;
    }
    
    // Priority 1: Flash event (mode change, reset, reseed) - 100% brightness
    if(IsInFlashEvent())
    {
        return kFlashBrightness;
    }
    
    // Priority 2: Live fill mode - pulsing
    if(state.liveFillActive)
    {
        return ProcessFillPulse();
    }
    
    float brightness = 0.0f;
    
    // Mode-specific processing
    switch(state.mode)
    {
        case LedMode::Interaction:
            brightness = ProcessInteraction(state);
            break;
            
        case LedMode::Config:
            brightness = kNormalBrightness;
            break;
            
        case LedMode::ShiftHeld:
            brightness = ProcessBreathing();
            break;
            
        case LedMode::Performance:
        default:
            brightness = ProcessPerformance(state);
            break;
    }
    
    return Clamp(brightness, 0.0f, 1.0f);
}

// =============================================================================
// Flash and Fill Pulse Processing
// =============================================================================

bool LedIndicator::IsInFlashEvent() const
{
    return (timeMs_ - flashStartTimeMs_) < kFlashDurationMs;
}

float LedIndicator::ProcessFlashEvent()
{
    return kFlashBrightness;
}

float LedIndicator::ProcessFillPulse()
{
    // Rapid pulsing during live fill mode
    float phase = fmodf(timeMs_, kFillPulsePeriodMs) / kFillPulsePeriodMs;
    // Sine wave between shimmer brightness and flash brightness
    float sine = sinf(phase * 2.0f * 3.14159265f);
    return kShimmerBrightness + (kFlashBrightness - kShimmerBrightness) * (0.5f + 0.5f * sine);
}

// =============================================================================
// Mode-Specific Processing
// =============================================================================

float LedIndicator::ProcessInteraction(const LedState& state)
{
    // Show parameter value as brightness gradient
    return state.interactionValue;
}

float LedIndicator::ProcessBreathing()
{
    float phase = fmodf(timeMs_, kBreathingCycleMs) / kBreathingCycleMs;
    // Sine wave from 0.2 to 1.0
    float sine = sinf(phase * 2.0f * 3.14159265f);
    return 0.6f + 0.4f * sine;
}

float LedIndicator::ProcessPerformance(const LedState& state)
{
    float brightness = kOffBrightness;
    
    // v3 compatibility: phrase position modulation
    if(state.isFillZone)
    {
        brightness = ProcessFillZone(state);
    }
    else if(state.isBuildZone)
    {
        brightness = ProcessBuildZone(state);
    }
    else if(state.broken > 0.0f || state.drift > 0.0f)
    {
        // v3 BROKEN × DRIFT behavior
        brightness = ProcessBrokenDrift(state);
    }
    
    // v4 trigger-based brightness (overlay on top of v3 behavior)
    // Downbeat: extra bright pulse overlay (100%)
    if(state.isDownbeat && IsInAnchorPulse(kDownbeatPulseMs))
    {
        brightness = std::max(brightness, kFlashBrightness);
    }
    // Anchor trigger: 80% brightness
    else if(IsInAnchorPulse(kTriggerPulseMs))
    {
        brightness = std::max(brightness, kAnchorBrightness);
    }
    // Shimmer trigger: 30% brightness
    else if(IsInShimmerPulse(kTriggerPulseMs))
    {
        brightness = std::max(brightness, kShimmerBrightness);
    }
    
    return brightness;
}

// =============================================================================
// v3 Compatibility: Phrase Zone Processing
// =============================================================================

float LedIndicator::ProcessFillZone(const LedState& state)
{
    (void)state;  // Unused but kept for signature consistency
    
    // Triple pulse cycle
    float cyclePeriod = (kTriplePulseMs * 3) + (kTriplePulseGapMs * 3);
    float cyclePos = fmodf(timeMs_, cyclePeriod);
    
    // Three pulses with gaps
    float pulsePhase = cyclePos / cyclePeriod;
    bool inPulse = false;
    
    // Pulse 1: 0-33% of pulse portion
    // Pulse 2: 33-66% of pulse portion  
    // Pulse 3: 66-100% of pulse portion
    float pulseRatio = kTriplePulseMs / (kTriplePulseMs + kTriplePulseGapMs);
    float segmentSize = 1.0f / 3.0f;
    
    for(int i = 0; i < 3; i++)
    {
        float segmentStart = i * segmentSize;
        float pulseEnd = segmentStart + (segmentSize * pulseRatio);
        if(pulsePhase >= segmentStart && pulsePhase < pulseEnd)
        {
            inPulse = true;
            break;
        }
    }
    
    return inPulse ? kNormalBrightness : kDimBrightness;
}

float LedIndicator::ProcessBuildZone(const LedState& state)
{
    // Build zone typically spans 50-75% of phrase
    // Assume buildProgress goes 0-1 within build zone
    // (We use phraseProgress as proxy: 0.5-0.75 mapped to 0-1)
    float buildProgress = 0.0f;
    if(state.phraseProgress >= 0.5f && state.phraseProgress < 0.75f)
    {
        buildProgress = (state.phraseProgress - 0.5f) / 0.25f;
    }
    
    // Pulse rate: 400ms → 100ms as build progresses
    float pulsePeriod = 400.0f - (300.0f * buildProgress);
    float phase = fmodf(timeMs_, pulsePeriod) / pulsePeriod;
    
    // Square wave with increasing duty cycle
    float dutyCycle = 0.3f + (0.3f * buildProgress);
    return (phase < dutyCycle) ? kNormalBrightness : kDimBrightness;
}

float LedIndicator::ProcessBrokenDrift(const LedState& state)
{
    // Flash period decreases with BROKEN (faster = more chaotic)
    float flashPeriod = kMaxFlashPeriodMs - 
                       (state.broken * (kMaxFlashPeriodMs - kMinFlashPeriodMs));
    
    // Low DRIFT: use consistent timing based on absolute time
    // High DRIFT: add randomness to timing
    float effectiveTime = timeMs_;
    if(state.drift > 0.3f)
    {
        // Add time jitter proportional to drift
        float jitter = GetPseudoRandom() * state.drift * flashPeriod * 0.3f;
        effectiveTime += jitter;
    }
    
    float phase = fmodf(effectiveTime, flashPeriod) / flashPeriod;
    
    // Duty cycle: lower BROKEN = longer on-time (more steady)
    float dutyCycle = 0.5f - (state.broken * 0.3f);
    bool inPulse = (phase < dutyCycle);
    
    // Intensity variation with DRIFT
    float intensity = kNormalBrightness;
    if(state.drift > 0.2f)
    {
        // Vary intensity with drift level
        float variation = GetPseudoRandom() * state.drift * 0.4f;
        intensity = kNormalBrightness - variation;
    }
    
    // High BROKEN: add some irregularity to pulse shape
    if(state.broken > 0.5f && inPulse)
    {
        float chaosFactor = (state.broken - 0.5f) * 2.0f;
        if(GetPseudoRandom() < chaosFactor * 0.3f)
        {
            // Random dropout
            inPulse = false;
        }
    }
    
    return inPulse ? intensity : kMinBrightness;
}

// =============================================================================
// Trigger Pulse Detection
// =============================================================================

bool LedIndicator::IsInAnchorPulse(float pulseDurationMs) const
{
    return (timeMs_ - anchorTriggerTimeMs_) < pulseDurationMs;
}

bool LedIndicator::IsInShimmerPulse(float pulseDurationMs) const
{
    return (timeMs_ - shimmerTriggerTimeMs_) < pulseDurationMs;
}

// =============================================================================
// Utilities
// =============================================================================

float LedIndicator::GetPseudoRandom()
{
    seed_ = seed_ * 1103515245 + 12345;
    return static_cast<float>((seed_ >> 16) & 0x7FFF) / 32767.0f;
}

// =============================================================================
// Boot Flash Patterns (V5 Task 33)
// =============================================================================

void LedIndicator::FlashHatUnlock()
{
    // Rising triple flash: 33% -> 66% -> 100%
    // Pattern: dim -> medium -> bright (fancy rising flash)
    const float levels[] = {0.33f, 0.66f, 1.0f};
    for (int i = 0; i < 3; ++i)
    {
        SetBrightness(levels[i]);
#ifndef HOST_BUILD
        daisy::System::Delay(80);
#endif
        SetBrightness(0.0f);
#ifndef HOST_BUILD
        daisy::System::Delay(80);
#endif
    }
#ifndef HOST_BUILD
    daisy::System::Delay(200);
#endif
}

void LedIndicator::FlashFillGateReset()
{
    // Single fade from bright to dark (grounded fade)
    SetBrightness(1.0f);
#ifndef HOST_BUILD
    for (int i = 100; i >= 0; i -= 5)
    {
        SetBrightness(i / 100.0f);
        daisy::System::Delay(15);
    }
    daisy::System::Delay(200);
#endif
}

} // namespace daisysp_idm_grids
