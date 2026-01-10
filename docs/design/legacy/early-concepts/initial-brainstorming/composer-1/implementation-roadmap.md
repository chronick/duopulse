# Two-Track Dual Sequencer - Implementation Roadmap

## Overview

This document outlines the implementation plan for migrating from the current 3-voice→2-output sequencer to the new Two-Track Dual design.

## Phase 1: Core Architecture Changes

### 1.1 Update Control System
**File**: `src/main.cpp`

**Changes**:
- Add shift button detection (Button B7)
- Update `ControlState` struct to include new parameters:
  - Performance Shift: `track1VelRange`, `track2VelRange`, `track1TimingOffset`, `track2TimingOffset`
  - Config Shift: `swingAmount`, `accentPattern`, `fillProbability`, `gateLength`
- Update `ProcessControls()` to handle shift button state
- Map controls based on mode + shift state

**Estimated Effort**: 2-3 hours

### 1.2 Update Sequencer Interface
**File**: `src/Engine/Sequencer.h` / `Sequencer.cpp`

**New Methods**:
```cpp
// Performance Shift Controls
void SetTrack1VelocityRange(float value);
void SetTrack2VelocityRange(float value);
void SetTrack1TimingOffset(float value);
void SetTrack2TimingOffset(float value);

// Config Shift Controls
void SetSwingAmount(float value);
void SetAccentPattern(float value);
void SetFillProbability(float value);
void SetGateLength(float value);

// Config Base Controls (update existing)
void SetTrackRelationship(float value); // Rename from SetEmphasis
```

**New Members**:
```cpp
// Performance Shift
float track1VelRange_ = 0.7f;
float track2VelRange_ = 0.7f;
float track1TimingOffset_ = 0.5f; // 0.0-1.0, 0.5 = no offset
float track2TimingOffset_ = 0.5f;

// Config Shift
float swingAmount_ = 0.0f;
float accentPattern_ = 0.0f;
float fillProbability_ = 0.0f;
float gateLength_ = 0.5f; // Maps to 5-50ms

// Config Base (update)
float trackRelationship_ = 0.5f; // Rename from emphasis_
```

**Estimated Effort**: 1-2 hours

## Phase 2: Pattern Generation Rewrite

### 2.1 Algorithmic Pattern Generator
**File**: `src/Engine/PatternGenerator.h` / `PatternGenerator.cpp`

**New Approach**:
- Remove dependency on `GridsData` (or keep as fallback)
- Implement algorithmic pattern generation based on Style parameter
- Support for different pattern types:
  - Straight (0.0-0.25): 4/4, 8th note patterns
  - Syncopated (0.25-0.5): Offbeat accents, 16th syncopation
  - Polyrhythmic (0.5-0.75): 3:2, 5:4, 7:8 patterns
  - IDM/Breakbeat (0.75-1.0): Irregular, micro-timing variations

**New Methods**:
```cpp
// Generate pattern for a single track
void GenerateTrackPattern(
    float style,
    float density,
    float variation,
    float velocityRange,
    float timingOffset,
    float trackRelationship, // For complementary/contrasting modes
    int trackIndex, // 0 or 1
    int step,
    bool& trigger,
    float& velocity
);

// Apply swing to timing
int ApplySwing(int step, float swingAmount);

// Generate accent pattern
float GetAccentMultiplier(int step, float accentPattern);
```

**Estimated Effort**: 4-6 hours

### 2.2 Track Relationship Logic
**Implementation**:
- **Complementary (0.0-0.33)**: Track 2 hit probability increases when Track 1 is silent
- **Independent (0.33-0.67)**: Tracks use separate pattern generators
- **Contrasting (0.67-1.0)**: Track 2 hit probability decreases when Track 1 hits (ducking)

**Estimated Effort**: 2-3 hours

## Phase 3: Timing System

### 3.1 Timing Offset Implementation
**File**: `src/Engine/Sequencer.cpp`

**Changes**:
- Add timing offset calculation in `ProcessAudio()`
- Offset affects when triggers fire (micro-timing)
- Range: -16 to +16 steps (mapped from 0.0-1.0)

**Implementation**:
```cpp
// In ProcessAudio(), before generating triggers:
int track1Step = stepIndex_ + static_cast<int>((track1TimingOffset_ - 0.5f) * 32.0f);
int track2Step = stepIndex_ + static_cast<int>((track2TimingOffset_ - 0.5f) * 32.0f);

// Wrap steps
track1Step = WrapStep(track1Step, effectiveLoopSteps);
track2Step = WrapStep(track2Step, effectiveLoopSteps);
```

**Estimated Effort**: 1-2 hours

### 3.2 Swing Implementation
**File**: `src/Engine/PatternGenerator.cpp`

**Changes**:
- Apply swing to 16th note subdivisions
- Swing affects timing of hits within each step
- Implementation: Delay odd-numbered 16th notes

**Estimated Effort**: 2-3 hours

## Phase 4: Dynamics System

### 4.1 Velocity Range Control
**File**: `src/Engine/PatternGenerator.cpp`

**Changes**:
- Scale velocity output based on `velocityRange` parameter
- Low range: 0.7-1.0 normalized
- Medium range: 0.4-1.0 normalized
- High range: 0.1-1.0 normalized

**Implementation**:
```cpp
float baseVelocity = GetBaseVelocity(...); // 0.0-1.0
float minVel = 1.0f - velocityRange;
float scaledVel = minVel + (baseVelocity * velocityRange);
```

**Estimated Effort**: 1 hour

### 4.2 Accent Pattern Control
**File**: `src/Engine/PatternGenerator.cpp`

**Changes**:
- Generate accent multipliers based on `accentPattern`
- Low (0.0-0.33): Accents on downbeats
- Medium (0.33-0.67): Accents on offbeats
- High (0.67-1.0): Complex accent patterns

**Estimated Effort**: 2-3 hours

## Phase 5: Fill System

### 5.1 Fill Probability Implementation
**File**: `src/Engine/Sequencer.cpp`

**Changes**:
- Detect loop boundaries (when `stepIndex_` wraps)
- Generate fill patterns based on `fillProbability`
- Fills are short bursts of hits (2-4 steps)

**Implementation**:
```cpp
bool isLoopBoundary = (stepIndex_ == 0);
if (isLoopBoundary && NextRandom() < fillProbability_)
{
    // Generate fill pattern
    GenerateFillPattern(track1Fill, track2Fill);
}
```

**Estimated Effort**: 2-3 hours

## Phase 6: Gate Length Control

### 6.1 Configurable Gate Duration
**File**: `src/Engine/Sequencer.cpp`

**Changes**:
- Map `gateLength_` (0.0-1.0) to duration (5ms-50ms)
- Update `gateDurationSamples_` based on `gateLength_`
- Apply to both tracks

**Implementation**:
```cpp
float gateMs = 5.0f + (gateLength_ * 45.0f); // 5-50ms
gateDurationSamples_ = static_cast<int>(gateMs * sampleRate_ / 1000.0f);
```

**Estimated Effort**: 30 minutes

## Phase 7: Testing & Refinement

### 7.1 Unit Tests
**File**: `tests/test_sequencer.cpp`

**New Tests**:
- Timing offset behavior
- Track relationship modes (complementary, independent, contrasting)
- Swing application
- Accent pattern generation
- Fill probability
- Velocity range scaling

**Estimated Effort**: 3-4 hours

### 7.2 Integration Testing
- Test all control combinations
- Verify soft takeover works with shift button
- Test external clock with new timing system
- Verify pattern generation across all style ranges

**Estimated Effort**: 2-3 hours

## Implementation Order

### Recommended Sequence

1. **Phase 1.1 & 1.2** (Control System + Sequencer Interface)
   - Establishes new parameter structure
   - Enables incremental feature addition

2. **Phase 2.1** (Algorithmic Pattern Generator)
   - Core pattern generation logic
   - Can be tested independently

3. **Phase 2.2** (Track Relationship)
   - Builds on pattern generator
   - Adds interaction between tracks

4. **Phase 3.1** (Timing Offset)
   - Adds micro-timing control
   - Relatively isolated feature

5. **Phase 3.2** (Swing)
   - Adds groove/timing feel
   - Works with timing offset

6. **Phase 4.1 & 4.2** (Velocity Range + Accents)
   - Dynamics system
   - Works with existing pattern generation

7. **Phase 5.1** (Fill Probability)
   - Adds complexity at loop boundaries
   - Independent feature

8. **Phase 6.1** (Gate Length)
   - Simple parameter mapping
   - Quick to implement

9. **Phase 7** (Testing)
   - Comprehensive testing of all features
   - Refinement based on results

## Total Estimated Effort

- **Phase 1**: 3-5 hours
- **Phase 2**: 6-9 hours
- **Phase 3**: 3-5 hours
- **Phase 4**: 3-4 hours
- **Phase 5**: 2-3 hours
- **Phase 6**: 30 minutes
- **Phase 7**: 5-7 hours

**Total**: ~22-33 hours

## Migration Strategy

### Option 1: Clean Slate (Recommended)
- Create new `TwoTrackSequencer` class
- Implement all features
- Test thoroughly
- Replace old sequencer in `main.cpp`

**Pros**: Clean implementation, no legacy code
**Cons**: More upfront work

### Option 2: Incremental Migration
- Update existing `Sequencer` class incrementally
- Add new parameters alongside old ones
- Gradually remove old code

**Pros**: Can test features incrementally
**Cons**: Temporary code complexity, potential conflicts

## Key Design Decisions

### 1. Pattern Generation Algorithm
**Decision**: Use algorithmic generation instead of Grids data
**Rationale**: 
- More flexible for 2-track output
- Allows smooth parameter interpolation
- Infinite pattern variations

**Implementation**: Start with simple probability-based patterns, add complexity incrementally

### 2. Timing Offset Range
**Decision**: ±16 steps maximum offset
**Rationale**: 
- Allows full bar offset (16 steps = 1 bar in 16th notes)
- Creates clear polyrhythmic feels
- Not so extreme as to be unusable

### 3. Velocity Range Mapping
**Decision**: Linear mapping (0.7-1.0, 0.4-1.0, 0.1-1.0)
**Rationale**: 
- Predictable behavior
- Easy to understand
- Can be refined based on testing

### 4. Track Relationship Modes
**Decision**: Three distinct modes (Complementary, Independent, Contrasting)
**Rationale**: 
- Clear, distinct behaviors
- Easy to understand and use
- Covers most use cases

## Success Criteria

1. ✅ All 32 parameters accessible and functional
2. ✅ Predictable 2-track output (no mapping confusion)
3. ✅ Smooth parameter interpolation
4. ✅ External clock support maintained
5. ✅ Soft takeover works with shift button
6. ✅ Pattern generation produces musical results
7. ✅ Timing offset creates polyrhythmic feels
8. ✅ Track relationship modes work as designed

## Next Steps

1. Review and approve specification
2. Create feature branch: `feature/two-track-dual`
3. Begin Phase 1 implementation
4. Regular testing and iteration
5. Documentation updates as features are added

