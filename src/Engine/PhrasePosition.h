#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * Phrase position tracking for musical awareness.
 * The sequencer tracks its position to modulate pattern behavior.
 * 
 * This is a temporary extraction from GenreConfig.h for v4 migration.
 * Will be replaced by SequencerState.h in Phase 1.
 */
struct PhrasePosition
{
    int   currentBar     = 0;     // 0 to (loopLengthBars - 1)
    int   stepInBar      = 0;     // 0 to 15
    int   stepInPhrase   = 0;     // 0 to (loopLengthBars * 16 - 1)
    float phraseProgress = 0.0f;  // 0.0 to 1.0 (normalized position in loop)
    bool  isLastBar      = false; // Approaching loop point
    bool  isFillZone     = false; // In fill zone (last steps of phrase, 75-100%)
    bool  isBuildZone    = false; // In build zone (leading up to fill, 50-100%)
    bool  isMidPhrase    = false; // Mid-phrase zone (40-60% of phrase)
    bool  isDownbeat     = true;  // Step 0 of any bar
};

/**
 * Calculate phrase position from step index and loop length.
 */
inline PhrasePosition CalculatePhrasePosition(int stepIndex, int loopLengthBars)
{
    PhrasePosition pos;
    int totalSteps = loopLengthBars * 16;

    pos.stepInPhrase  = stepIndex % totalSteps;
    pos.stepInBar     = pos.stepInPhrase % 16;
    pos.currentBar    = pos.stepInPhrase / 16;
    pos.phraseProgress = static_cast<float>(pos.stepInPhrase) / static_cast<float>(totalSteps);
    pos.isLastBar     = (pos.currentBar == loopLengthBars - 1);
    pos.isDownbeat    = (pos.stepInBar == 0);

    // Fill zone and build zone scale with pattern length
    // Fill zone: last 4 steps per bar of loop length (min 4, max 32)
    // Build zone: last 8 steps per bar of loop length (min 8, max 64)
    int fillZoneSteps  = loopLengthBars * 4;
    int buildZoneSteps = loopLengthBars * 8;
    if(fillZoneSteps < 4)
        fillZoneSteps = 4;
    if(fillZoneSteps > 32)
        fillZoneSteps = 32;
    if(buildZoneSteps < 8)
        buildZoneSteps = 8;
    if(buildZoneSteps > 64)
        buildZoneSteps = 64;

    int stepsFromEnd = totalSteps - pos.stepInPhrase;
    pos.isFillZone  = (stepsFromEnd <= fillZoneSteps);
    pos.isBuildZone = (stepsFromEnd <= buildZoneSteps);
    
    // Mid-phrase zone (40-60% of phrase): potential mid-phrase fill point
    pos.isMidPhrase = (pos.phraseProgress >= 0.40f && pos.phraseProgress < 0.60f);

    return pos;
}

/**
 * Check if a step is an off-beat (should receive swing).
 * In 16th note patterns, off-beats are odd-numbered steps (1, 3, 5, 7...).
 */
inline bool IsOffBeat(int step)
{
    return (step & 1) != 0;
}

} // namespace daisysp_idm_grids
