#include "DriftControl.h"

#include <algorithm>

namespace daisysp_idm_grids
{

// =============================================================================
// Step Stability Functions
// =============================================================================

float GetStepStability(int step, int patternLength)
{
    // Clamp step to valid range
    if (step < 0 || step >= patternLength)
    {
        return kStabilityOffbeat;
    }

    // For 32-step patterns (2 bars of 16th notes):
    // Step 0 = beat 1 of bar 1 (downbeat)
    // Step 16 = beat 1 of bar 2 (half-bar)
    // Steps 8, 24 = beat 3 of each bar (quarter notes)
    // Steps 4, 12, 20, 28 = "and" of each beat (eighth notes)
    // Even steps = on-beat 16ths
    // Odd steps = off-beat 16ths

    // Scale step to 32-step equivalent for consistent behavior
    int normalizedStep = step;
    if (patternLength != 32 && patternLength > 0)
    {
        // Map to 32-step grid for stability calculation
        normalizedStep = (step * 32) / patternLength;
    }

    // Downbeat - most stable
    if (normalizedStep == 0)
    {
        return kStabilityDownbeat;
    }

    // Half-bar - very stable
    if (normalizedStep == 16)
    {
        return kStabilityHalfBar;
    }

    // Quarter notes - stable
    if (normalizedStep == 8 || normalizedStep == 24)
    {
        return kStabilityQuarter;
    }

    // Eighth notes - moderately stable
    if (normalizedStep % 8 == 4)
    {
        return kStabilityEighth;
    }

    // Even steps (strong sixteenths) - less stable
    if (normalizedStep % 2 == 0)
    {
        return kStabilitySixteenth;
    }

    // Odd steps (off-beats) - least stable
    return kStabilityOffbeat;
}

uint32_t GetStabilityMask(int patternLength, float stabilityThreshold)
{
    uint32_t mask = 0;

    int maxSteps = std::min(patternLength, static_cast<int>(kMaxSteps));

    for (int step = 0; step < maxSteps; ++step)
    {
        float stability = GetStepStability(step, patternLength);
        if (stability >= stabilityThreshold)
        {
            mask |= (1U << step);
        }
    }

    return mask;
}

// =============================================================================
// Seed Selection Functions
// =============================================================================

uint32_t SelectSeed(const DriftState& state, float drift, int step, int patternLength)
{
    float stability = GetStepStability(step, patternLength);
    return SelectSeedWithStability(state, drift, stability);
}

uint32_t SelectSeedWithStability(const DriftState& state, float drift, float stepStability)
{
    // If step stability is greater than drift threshold, use locked pattern seed
    // Otherwise use evolving phrase seed
    //
    // At DRIFT=0: all steps use patternSeed (stability > 0 always true)
    // At DRIFT=1: all steps use phraseSeed (stability > 1 never true)
    // At DRIFT=0.5: stable steps (>0.5) use patternSeed, unstable use phraseSeed

    if (stepStability > drift)
    {
        return state.patternSeed;
    }
    return state.phraseSeed;
}

// =============================================================================
// Phrase and Reseed Functions
// =============================================================================

void OnPhraseEnd(DriftState& state)
{
    // Handle any pending reseed request
    if (state.reseedRequested)
    {
        // Generate entirely new pattern seed
        state.patternSeed = GenerateNewSeed(state.patternSeed, state.phraseCounter);
        state.reseedRequested = false;
    }

    // Always generate new phrase seed
    state.phraseCounter++;
    state.phraseSeed = HashCombine(state.patternSeed, state.phraseCounter);
}

void RequestReseed(DriftState& state)
{
    state.reseedRequested = true;
}

void Reseed(DriftState& state, uint32_t newSeed)
{
    if (newSeed == 0)
    {
        // Generate from current state
        state.patternSeed = GenerateNewSeed(state.patternSeed, state.phraseCounter);
    }
    else
    {
        state.patternSeed = newSeed;
    }

    // Reset phrase tracking
    state.phraseCounter = 0;
    state.phraseSeed = HashCombine(state.patternSeed, state.phraseCounter);
    state.reseedRequested = false;
}

uint32_t GenerateNewSeed(uint32_t currentSeed, uint32_t counter)
{
    // Use a mixing function based on MurmurHash-style operations
    // This provides good avalanche properties

    uint32_t seed = currentSeed;

    // Mix in the counter
    seed ^= counter * 0x9e3779b9;  // Golden ratio constant

    // Avalanche mixing
    seed ^= (seed >> 16);
    seed *= 0x85ebca6b;
    seed ^= (seed >> 13);
    seed *= 0xc2b2ae35;
    seed ^= (seed >> 16);

    // Ensure non-zero result
    if (seed == 0)
    {
        seed = kDefaultPatternSeed;
    }

    return seed;
}

// =============================================================================
// Initialization
// =============================================================================

void InitDriftState(DriftState& state, uint32_t seed)
{
    if (seed == 0)
    {
        seed = kDefaultPatternSeed;
    }

    state.patternSeed = seed;
    state.phraseSeed = seed ^ kPhraseSeedXor;
    state.phraseCounter = 0;
    state.reseedRequested = false;
}

// =============================================================================
// Utility Functions
// =============================================================================

float GetLockedRatio(float drift, int patternLength)
{
    // Count how many steps would use the locked seed
    int lockedCount = 0;
    int maxSteps = std::min(patternLength, static_cast<int>(kMaxSteps));

    for (int step = 0; step < maxSteps; ++step)
    {
        float stability = GetStepStability(step, patternLength);
        if (stability > drift)
        {
            lockedCount++;
        }
    }

    if (maxSteps == 0)
    {
        return 0.0f;
    }

    return static_cast<float>(lockedCount) / static_cast<float>(maxSteps);
}

bool IsStepLocked(int step, int patternLength, float drift)
{
    float stability = GetStepStability(step, patternLength);
    return stability > drift;
}

uint32_t HashCombine(uint32_t seed, uint32_t value)
{
    // Standard hash combine function (Boost-style)
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

} // namespace daisysp_idm_grids
