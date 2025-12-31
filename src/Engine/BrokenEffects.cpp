#include "BrokenEffects.h"

namespace daisysp_idm_grids
{

// =============================================================================
// Effect 1: Swing (Tied to BROKEN)
// =============================================================================

float GetSwingFromBroken(float broken)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    if(broken < 0.25f)
    {
        // Techno: 50-54% (nearly straight)
        float t = broken * 4.0f;  // 0.0 to 1.0 within range
        return 0.50f + t * 0.04f; // 0.50 to 0.54
    }
    else if(broken < 0.50f)
    {
        // Tribal: 54-60% (shuffled)
        float t = (broken - 0.25f) * 4.0f; // 0.0 to 1.0 within range
        return 0.54f + t * 0.06f;          // 0.54 to 0.60
    }
    else if(broken < 0.75f)
    {
        // Trip-Hop: 60-66% (lazy)
        float t = (broken - 0.50f) * 4.0f; // 0.0 to 1.0 within range
        return 0.60f + t * 0.06f;          // 0.60 to 0.66
    }
    else
    {
        // IDM: 66-58% (variable swing combined with heavy jitter)
        // Continuity: start at 0.66 (where Trip-Hop ends), move toward 0.58
        // The micro-timing jitter provides the chaos, not swing reduction
        float t = (broken - 0.75f) * 4.0f; // 0.0 to 1.0 within range
        return 0.66f - t * 0.08f;          // 0.66 to 0.58
    }
}

// =============================================================================
// Effect 2: Micro-Timing Jitter
// =============================================================================

float GetJitterMsFromBroken(float broken)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    if(broken < 0.4f)
    {
        // Machine-tight: 0ms
        return 0.0f;
    }
    else if(broken < 0.7f)
    {
        // Subtle human feel: 0-3ms
        float t = (broken - 0.4f) / 0.3f; // 0.0 to 1.0 within range
        return t * 3.0f;                  // 0.0 to 3.0
    }
    else if(broken < 0.9f)
    {
        // Loose, organic: 3-6ms
        float t = (broken - 0.7f) / 0.2f; // 0.0 to 1.0 within range
        return 3.0f + t * 3.0f;           // 3.0 to 6.0
    }
    else
    {
        // Broken, glitchy: 6-12ms
        float t = (broken - 0.9f) / 0.1f; // 0.0 to 1.0 within range
        return 6.0f + t * 6.0f;           // 6.0 to 12.0
    }
}

float ApplyJitter(float maxJitterMs, uint32_t seed, int step)
{
    if(maxJitterMs <= 0.0f)
        return 0.0f;

    // Use a different hash offset to avoid correlation with other randomness
    uint32_t hash   = HashStep(seed ^ kJitterHashMagic, step);
    float randomVal = HashToFloat(hash); // 0.0 to 1.0
    // Map to [-1.0, +1.0] and scale
    return (randomVal - 0.5f) * 2.0f * maxJitterMs;
}

// =============================================================================
// Effect 3: Step Displacement
// =============================================================================

int GetDisplacedStep(int step, float broken, uint32_t seed)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // No displacement below 50% BROKEN
    if(broken < 0.5f)
        return step;

    // Calculate displacement chance based on BROKEN level
    float displaceChance;
    int maxShift;

    if(broken < 0.75f)
    {
        // 50-75%: 0-15% chance, ±1 step
        float t       = (broken - 0.5f) * 4.0f; // 0.0 to 1.0 within range
        displaceChance = t * 0.15f;             // 0.0 to 0.15
        maxShift       = 1;
    }
    else
    {
        // 75-100%: 15-40% chance, ±2 steps
        float t       = (broken - 0.75f) * 4.0f; // 0.0 to 1.0 within range
        displaceChance = 0.15f + t * 0.25f;      // 0.15 to 0.40
        maxShift       = 2;
    }

    // Determine if displacement happens
    uint32_t chanceHash = HashStep(seed ^ kDisplaceChanceHashMagic, step);
    float chanceRoll    = HashToFloat(chanceHash);

    if(chanceRoll >= displaceChance)
        return step; // No displacement

    // Determine shift direction and amount
    uint32_t shiftHash = HashStep(seed ^ kDisplaceShiftHashMagic, step);
    float shiftRoll    = HashToFloat(shiftHash);

    // Map [0, 1) to [-maxShift, +maxShift]
    int shift = static_cast<int>((shiftRoll * (2 * maxShift + 1))) - maxShift;

    // Wrap around to valid step range
    int newStep = (step + shift + 32) % 32;
    return newStep;
}

// =============================================================================
// Effect 4: Velocity Variation
// =============================================================================

float GetVelocityWithVariation(float baseVel, float broken, uint32_t seed, int step)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // Calculate variation range based on BROKEN level
    float variationRange;

    if(broken < 0.3f)
    {
        // Consistent: ±5%
        variationRange = 0.05f;
    }
    else if(broken < 0.6f)
    {
        // Subtle dynamics: 5-10%
        float t        = (broken - 0.3f) / 0.3f; // 0.0 to 1.0 within range
        variationRange = 0.05f + t * 0.05f;      // 0.05 to 0.10
    }
    else
    {
        // Expressive, uneven: 10-20%
        float t        = (broken - 0.6f) / 0.4f; // 0.0 to 1.0 within range
        variationRange = 0.10f + t * 0.10f;      // 0.10 to 0.20
    }

    // Apply random variation
    uint32_t hash   = HashStep(seed ^ kVelocityHashMagic, step);
    float randomVal = HashToFloat(hash);                   // 0.0 to 1.0
    float variation = (randomVal - 0.5f) * 2.0f * variationRange;

    float result = baseVel + variation;

    // Clamp to valid velocity range
    // Task 21 Phase B: minimum raised to 0.30 for VCA audibility (was 0.2)
    if(result < 0.30f)
        result = 0.30f;
    if(result > 1.0f)
        result = 1.0f;

    return result;
}

float GetVelocityVariationRange(float broken)
{
    // Clamp input to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    if(broken < 0.3f)
    {
        return 0.05f;
    }
    else if(broken < 0.6f)
    {
        float t = (broken - 0.3f) / 0.3f;
        return 0.05f + t * 0.05f;
    }
    else
    {
        float t = (broken - 0.6f) / 0.4f;
        return 0.10f + t * 0.10f;
    }
}

// =============================================================================
// v4 BROKEN/FLAVOR Stack: Zone-Bounded Timing Effects
// =============================================================================

// Magic numbers for v4 timing hash mixing
constexpr uint32_t kV4JitterHashMagic  = 0x4A545434; // "JTT4"
constexpr uint32_t kV4DisplaceHashMagic = 0x44535034; // "DSP4"
constexpr uint32_t kV4VelChaosHashMagic = 0x56434834; // "VCH4"

float GetMaxSwingForZone(EnergyZone zone)
{
    switch (zone)
    {
        case EnergyZone::MINIMAL:
        case EnergyZone::GROOVE:
            return 0.58f;  // Tight timing
        case EnergyZone::BUILD:
            return 0.62f;  // Moderate looseness
        case EnergyZone::PEAK:
        default:
            return 0.66f;  // Full triplet swing
    }
}

float GetMaxJitterMsForZone(EnergyZone zone)
{
    switch (zone)
    {
        case EnergyZone::MINIMAL:
        case EnergyZone::GROOVE:
            return 3.0f;   // Tight timing
        case EnergyZone::BUILD:
            return 6.0f;   // Moderate looseness
        case EnergyZone::PEAK:
        default:
            return 12.0f;  // Expressive timing
    }
}

float ComputeSwing(float swing, EnergyZone zone)
{
    // Clamp swing config to valid range
    swing = Clamp(swing, 0.0f, 1.0f);

    // Base swing scales with config: 50% (straight) to 66% (heavy triplet)
    float baseSwing = 0.50f + swing * 0.16f;

    // Apply zone limit
    float maxSwing = GetMaxSwingForZone(zone);

    return (baseSwing < maxSwing) ? baseSwing : maxSwing;
}

float ApplySwingToStep(int step, float swingAmount, float samplesPerStep)
{
    // Only offbeats (odd 16th notes) receive swing
    if (!IsOffbeat(step))
        return 0.0f;

    // Swing amount is the ratio of 8th note duration for the offbeat
    // 50% = straight (offbeat at exactly half), 66% = triplet feel
    // Offset = (swingAmount - 0.5) * 2 * samplesPerStep
    // At 50%: offset = 0
    // At 66%: offset = 0.32 * samplesPerStep (offbeat delayed by 32%)
    float offset = (swingAmount - 0.5f) * 2.0f * samplesPerStep;

    return offset;
}

float ComputeMicrotimingOffset(float flavor,
                               EnergyZone zone,
                               float sampleRate,
                               uint32_t seed,
                               int step)
{
    // Clamp flavor to valid range
    flavor = Clamp(flavor, 0.0f, 1.0f);

    // Get zone-bounded max jitter
    float maxJitterMs = GetMaxJitterMsForZone(zone);

    // Scale jitter with flavor (0% flavor = no jitter)
    float jitterMs = flavor * maxJitterMs;

    // No jitter if effectively zero
    if (jitterMs < 0.001f)
        return 0.0f;

    // Generate deterministic random offset in [-jitterMs, +jitterMs]
    uint32_t hash = HashStep(seed ^ kV4JitterHashMagic, step);
    float randomVal = HashToFloat(hash);  // 0.0 to 1.0
    float jitterMsBipolar = (randomVal - 0.5f) * 2.0f * jitterMs;

    // Convert milliseconds to samples
    float jitterSamples = jitterMsBipolar * sampleRate / 1000.0f;

    return jitterSamples;
}

int ComputeStepDisplacement(int step, float flavor, EnergyZone zone, uint32_t seed)
{
    // Clamp inputs
    flavor = Clamp(flavor, 0.0f, 1.0f);

    // Displacement only allowed in BUILD and PEAK zones
    if (zone == EnergyZone::MINIMAL || zone == EnergyZone::GROOVE)
        return step;

    // Compute displacement chance and max shift based on zone
    float displaceChance;
    int maxShift;

    if (zone == EnergyZone::BUILD)
    {
        // BUILD zone: up to 20% chance, ±1 step
        displaceChance = flavor * 0.20f;
        maxShift = 1;
    }
    else  // PEAK zone
    {
        // PEAK zone: up to 40% chance, ±2 steps
        displaceChance = flavor * 0.40f;
        maxShift = 2;
    }

    // Determine if displacement happens
    uint32_t chanceHash = HashStep(seed ^ kV4DisplaceHashMagic, step);
    float chanceRoll = HashToFloat(chanceHash);

    if (chanceRoll >= displaceChance)
        return step;  // No displacement

    // Determine shift direction and amount
    uint32_t shiftHash = HashStep(seed ^ kV4DisplaceHashMagic ^ 0x12345, step);
    float shiftRoll = HashToFloat(shiftHash);

    // Map [0, 1) to [-maxShift, +maxShift], excluding 0
    int shift = static_cast<int>((shiftRoll * (2 * maxShift + 1))) - maxShift;

    // Don't allow zero shift (if we're displacing, actually move)
    if (shift == 0)
        shift = (shiftRoll < 0.5f) ? -1 : 1;

    // Wrap to valid step range
    int newStep = (step + shift + 32) % 32;

    return newStep;
}

float ComputeVelocityChaos(float baseVelocity, float flavor, uint32_t seed, int step)
{
    // Clamp inputs
    flavor = Clamp(flavor, 0.0f, 1.0f);
    baseVelocity = Clamp(baseVelocity, 0.0f, 1.0f);

    // Velocity chaos: ±0% at flavor=0, ±25% at flavor=1
    float chaosRange = flavor * 0.25f;

    if (chaosRange < 0.001f)
        return baseVelocity;

    // Generate deterministic random variation
    uint32_t hash = HashStep(seed ^ kV4VelChaosHashMagic, step);
    float randomVal = HashToFloat(hash);  // 0.0 to 1.0
    float variation = (randomVal - 0.5f) * 2.0f * chaosRange;

    float result = baseVelocity + variation;

    // Clamp to valid velocity range (minimum 0.1 to ensure audibility)
    return Clamp(result, 0.1f, 1.0f);
}

// =============================================================================
// Phrase-Aware Modulation [phrase-modulation]
// =============================================================================

float GetPhraseWeightBoost(const PhrasePosition& pos, float broken)
{
    // No boost outside build zone
    if(!pos.isBuildZone)
        return 0.0f;

    // Clamp broken to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // Base boost: increases toward phrase end
    float boost = 0.0f;

    if(pos.isFillZone)
    {
        // Last 25%: significant boost to off-beat weights
        // phraseProgress goes from 0.75 to 1.0 in fill zone
        // boost goes from 0.15 to 0.25
        float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f; // 0.0 to 1.0
        boost              = 0.15f + fillProgress * 0.10f;        // 0.15 to 0.25
    }
    else
    {
        // Build zone but not fill zone (50-75%): subtle boost
        // phraseProgress goes from 0.5 to 0.75 in build zone
        // boost goes from 0 to 0.075
        float buildProgress = (pos.phraseProgress - 0.5f) * 4.0f; // 0.0 to 1.0
        boost               = buildProgress * 0.075f;             // 0.0 to 0.075
    }

    // Genre scale: Techno has subtle fills, IDM has dramatic fills
    // Scale ranges from 0.5 (at broken=0) to 1.5 (at broken=1)
    float genreScale = 0.5f + broken * 1.0f;

    return boost * genreScale;
}

float GetPhraseWeightBoostWithRatchet(const PhrasePosition& pos,
                                     float broken,
                                     float drift,
                                     float ratchet)
{
    // CRITICAL: DRIFT=0 means no fills occur, regardless of RATCHET
    if(drift <= 0.0f)
        return 0.0f;

    // No boost outside fill-relevant zones
    if(!pos.isBuildZone && !pos.isMidPhrase)
        return 0.0f;

    // Clamp parameters to valid range
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;
    if(drift < 0.0f)
        drift = 0.0f;
    if(drift > 1.0f)
        drift = 1.0f;
    if(ratchet < 0.0f)
        ratchet = 0.0f;
    if(ratchet > 1.0f)
        ratchet = 1.0f;

    // Base boost depends on zone
    float boost = 0.0f;

    if(pos.isFillZone)
    {
        // Fill zone (75-100%): Maximum fill activity
        // phraseProgress goes from 0.75 to 1.0 in fill zone
        // Base boost: 0.15 to 0.25
        float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f;
        boost              = 0.15f + fillProgress * 0.10f;
    }
    else if(pos.isBuildZone)
    {
        // Build zone (50-75%): Increasing energy
        // phraseProgress goes from 0.5 to 0.75 in build zone
        // boost goes from 0 to 0.075
        float buildProgress = (pos.phraseProgress - 0.5f) * 4.0f;
        boost               = buildProgress * 0.075f;
    }
    else if(pos.isMidPhrase)
    {
        // Mid-phrase (40-60%): Potential mid-phrase fill
        // Subtle boost, only with higher RATCHET
        // boost goes from 0 to 0.05 based on RATCHET
        boost = 0.05f * ratchet;
    }

    // RATCHET scales fill intensity (0-30% additional boost at max)
    // At RATCHET=0: only base boost
    // At RATCHET=1: base boost + 30% extra
    float ratchetBoost = boost * ratchet * 0.6f; // Up to 60% extra on base boost
    boost += ratchetBoost;

    // DRIFT gates how much fill activity occurs
    // At DRIFT=0: no fills (handled above)
    // At DRIFT=1: full fill probability
    boost *= drift;

    // Genre scale: Techno has subtle fills, IDM has dramatic fills
    // Scale ranges from 0.5 (at broken=0) to 1.5 (at broken=1)
    float genreScale = 0.5f + broken * 1.0f;

    return boost * genreScale;
}

float GetEffectiveBroken(float broken, const PhrasePosition& pos)
{
    // No boost outside fill zone
    if(!pos.isFillZone)
        return Clamp(broken, 0.0f, 1.0f);

    // Clamp input
    if(broken < 0.0f)
        broken = 0.0f;
    if(broken > 1.0f)
        broken = 1.0f;

    // Boost BROKEN by up to 20% in fill zone
    // phraseProgress goes from 0.75 to 1.0 in fill zone
    // fillBoost goes from 0 to 0.2
    float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f; // 0.0 to 1.0
    float fillBoost    = fillProgress * 0.2f;                 // 0.0 to 0.2

    return Clamp(broken + fillBoost, 0.0f, 1.0f);
}

float GetPhraseAccent(const PhrasePosition& pos)
{
    // Phrase downbeat gets maximum accent
    if(pos.stepInPhrase == 0)
        return 1.2f;

    // Bar downbeat gets moderate accent
    if(pos.isDownbeat)
        return 1.1f;

    // No accent for other steps
    return 1.0f;
}

float GetPhraseAccentWithRatchet(const PhrasePosition& pos, float ratchet)
{
    // Clamp ratchet to valid range
    if(ratchet < 0.0f)
        ratchet = 0.0f;
    if(ratchet > 1.0f)
        ratchet = 1.0f;

    // Phrase downbeat gets resolution accent boosted by RATCHET
    // Base: 1.2×, max with RATCHET=1: 1.5×
    if(pos.stepInPhrase == 0)
        return 1.2f + (ratchet * 0.3f);

    // Bar downbeat gets moderate accent (unchanged by RATCHET)
    if(pos.isDownbeat)
        return 1.1f;

    // Fill zone: velocity ramp toward phrase end (fills get louder)
    // Ramp scales with RATCHET: 0 at RATCHET=0, up to 1.3× at RATCHET=1
    if(pos.isFillZone && ratchet > 0.0f)
    {
        // fillProgress: 0 at start of fill zone (75%), 1 at end (100%)
        float fillProgress = (pos.phraseProgress - 0.75f) * 4.0f;
        if(fillProgress < 0.0f)
            fillProgress = 0.0f;
        if(fillProgress > 1.0f)
            fillProgress = 1.0f;
        // Velocity ramp: 1.0 to 1.3× toward end, scaled by RATCHET
        return 1.0f + (fillProgress * 0.3f * ratchet);
    }

    // No accent for other steps
    return 1.0f;
}

// =============================================================================
// Voice Interaction: FUSE Energy Balance [fuse-balance]
// =============================================================================

void ApplyFuse(float fuse, float& anchorDensity, float& shimmerDensity)
{
    // v3 Critical Rule: DENSITY=0 = absolute silence
    // Store whether each voice was at zero before any modification
    bool anchorWasZero  = (anchorDensity <= 0.0f);
    bool shimmerWasZero = (shimmerDensity <= 0.0f);

    // Clamp fuse to valid range
    if(fuse < 0.0f)
        fuse = 0.0f;
    if(fuse > 1.0f)
        fuse = 1.0f;

    // Calculate bias: (fuse - 0.5) * 0.3 gives ±0.15 at extremes
    // fuse = 0.0 → bias = -0.15 (anchor boost, shimmer reduce)
    // fuse = 0.5 → bias = 0.0 (balanced)
    // fuse = 1.0 → bias = +0.15 (shimmer boost, anchor reduce)
    float bias = (fuse - 0.5f) * 0.3f;

    // Apply bias: subtract from anchor, add to shimmer
    anchorDensity  = Clamp(anchorDensity - bias, 0.0f, 1.0f);
    shimmerDensity = Clamp(shimmerDensity + bias, 0.0f, 1.0f);

    // Enforce DENSITY=0 invariant: if voice started at 0, keep it at 0
    if(anchorWasZero)
        anchorDensity = 0.0f;
    if(shimmerWasZero)
        shimmerDensity = 0.0f;
}

// =============================================================================
// Voice Interaction: COUPLE Interlock [couple-interlock]
// =============================================================================

void ApplyCouple(float couple,
                 bool anchorFires,
                 bool& shimmerFires,
                 float& shimmerVel,
                 uint32_t seed,
                 int step,
                 float shimmerDensity)
{
    // Clamp couple to valid range
    if(couple < 0.0f)
        couple = 0.0f;
    if(couple > 1.0f)
        couple = 1.0f;

    // Below 10% couple: fully independent, no interaction
    if(couple < 0.1f)
        return;

    if(anchorFires)
    {
        // Anchor is firing — reduce shimmer probability (collision avoidance)
        // Suppression chance scales from 0% at couple=0.1 to 80% at couple=1.0
        float suppressChance = couple * 0.8f;

        uint32_t hash   = HashStep(seed ^ kCoupleSuppressHashMagic, step);
        float randomVal = HashToFloat(hash);

        if(randomVal < suppressChance)
        {
            shimmerFires = false;
        }
    }
    else
    {
        // Anchor is silent — boost shimmer probability (gap filling)
        // Only applies when shimmer wasn't already firing and couple > 50%
        //
        // v3 Critical Rule: NEVER gap-fill if shimmerDensity is 0
        // DENSITY=0 = absolute silence
        bool densityAllowsFill = (shimmerDensity < 0.0f) || (shimmerDensity > 0.0f);
        
        if(!shimmerFires && couple > 0.5f && densityAllowsFill)
        {
            // Boost chance scales from 0% at couple=0.5 to 30% at couple=1.0
            float boostChance = (couple - 0.5f) * 0.6f;

            uint32_t hash   = HashStep(seed ^ kCoupleBoostHashMagic, step);
            float randomVal = HashToFloat(hash);

            if(randomVal < boostChance)
            {
                shimmerFires = true;

                // Generate medium velocity for fill (0.5 to 0.8)
                uint32_t velHash = HashStep(seed ^ kCoupleVelHashMagic, step);
                shimmerVel       = 0.5f + HashToFloat(velHash) * 0.3f;
            }
        }
    }
}

} // namespace daisysp_idm_grids
