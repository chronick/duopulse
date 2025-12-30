# Task 21: Musicality Improvements Spike

**Status**: PENDING
**Branch**: TBD
**Parent Task**: Task 16 (Hardware Validation)
**Related**: Task 20 (Level 1 Archetype Debug), Task 22 Phase D (Swing)

---

## Problem Statement

During Level 2-3 hardware testing (Task 20) and config mode testing (Task 16 Test 7), user feedback indicated several musicality issues:

1. **Lack variety** - Patterns feel repetitive, not enough differentiation across the archetype grid
2. **Don't feel very musical** - Patterns lack the groove/feel expected from a techno sequencer
3. **K2 (BUILD) effect hard to notice** - Density-over-phrase not audibly impactful
4. **Pattern Length Bug** - At 64 steps (100%), latter half of patterns are blank
5. **Swing effectiveness** - Config K2 (Swing) doesn't noticeably affect timing

### User Quotes (from Task 20)
> "not feeling like I'm getting very musical beats, nor am I getting much variety"

> "patterns are changing and are generally following the intended flow, though they don't seem to have a ton of variation"

> "K2 does something but it's hard to notice much change"

### User Quotes (from Task 16 Test 7)

**Pattern Length (Test 7A)**:
> "turning the knob appears to change pattern, but when turning fully 100%, we expect a long pattern with variation, but, the latter half of the patterns are blank entirely. This behavior is consistent even when changing other settings. Pattern lengths feel not very musical either."

**Swing (Test 7B)**:
> "As mentioned before, swing does not appear to change timings very much by perception. It may be just my perception, so just double check implementation and how it works with Genre. I'd expect SWING to act as a modifier against the existing swing settings in GENRE, from 0x to 2x of GENRE swing."

---

## Observed Pattern Data

From Task 20 Level 3 logs, typical anchor masks observed:
```
0x11111111  - 4-on-floor (very common)
0x02040811  - sparse variation
0x04104111  - slight syncopation
0x24924915  - busier pattern
0x11124911  - minor variation
```

### Observations
- `0x11111111` (straight 4-on-floor) appears very frequently
- Limited variety in off-beat placement
- Shimmer patterns often minimal (`0x00000100`, `0x01000100`)
- Weights cluster around 85-95% at beat positions (little differentiation)

### Testing Mode Clarification (2025-12-28)
When testing anchor/shimmer variation, note the different control locations:
- **Balance** (shimmer hit count): Performance Mode + Shift + K4
- **Voice Coupling** (relationship mode): Config Mode + Shift + K4

Code review confirms both parameters ARE working correctly. The perceived "lack of variation" is due to:
1. Balance only scales shimmer from 30%→100% of anchor (1→4 hits at typical energy)
2. Voice coupling modes work but SHADOW produces a predictable delayed copy

---

## Areas to Investigate

### 1. Archetype Weight Tables
**File**: `src/Engine/ArchetypeData.h`

Current TECHNO archetypes may have:
- Weights too similar across archetypes (all ~0.85-1.0 at beats)
- Insufficient contrast between Minimal/Groovy/Chaos positions
- Off-beat weights may be too low to ever win selection

**Questions**:
- What weight distribution creates interesting techno patterns?
- How different should archetypes be from each other?
- Should we introduce more variation in off-beat weights?

### 2. Hit Budget Calculation
**File**: `src/Engine/HitBudget.h`, `HitBudget.cpp`

Current budget may be:
- Too conservative (limited hits per bar)
- Not scaling enough with ENERGY knob
- Eligibility masks too restrictive

**Questions**:
- What's the right hit budget range for techno (4-12 hits/bar?)
- How should budget scale across ENERGY range?

### 3. Gumbel Sampler Temperature
**File**: `src/Engine/GumbelSampler.h`, `GumbelSampler.cpp`

Temperature controls selection randomness:
- Too cold → always picks highest weight (predictable)
- Too hot → random selection (chaotic)

**Questions**:
- What temperature creates interesting "weighted random" selection?
- Should temperature vary with FIELD position?

### 4. Guard Rails
**File**: `src/Engine/GuardRails.h`, `GuardRails.cpp`

Guard rails may be:
- Over-constraining pattern generation
- Forcing too many beat-1 anchors
- Not allowing enough rhythmic freedom

**Questions**:
- Are rules too strict for IDM/experimental patterns?
- Should rules be configurable or softer?

### 5. Voice Relationships
**File**: `src/Engine/VoiceRelation.h`, `VoiceRelation.cpp`, `HitBudget.cpp`

#### Issue 5a: Balance Effect Too Subtle
The BALANCE parameter (Performance+Shift K4) controls shimmer hit budget as a ratio of anchor budget:
- `shimmerRatio = 0.3 + balance * 0.7`
- At balance=0.0: shimmer gets 30% of anchor hits
- At balance=1.0: shimmer gets 100% of anchor hits

**Problem**: At typical GROOVE energy (anchor=4 hits), this only produces 1→4 shimmer hits. The 3-hit difference per bar is audibly subtle.

**Potential Fixes**:
1. Allow shimmer budget to exceed anchor (balance=1.0 → 150% of anchor)
2. Add independent shimmer density control
3. Increase base anchor budget so shimmer has more room to scale

#### Issue 5b: Voice Coupling Modes Work But May Need Tuning
VoiceCoupling (Config+Shift K4) correctly applies:
- INDEPENDENT: Both patterns fire as generated
- INTERLOCK: `shimmerMask &= ~anchorMask` (no overlap)
- SHADOW: `shimmerMask = ShiftMaskLeft(anchorMask, 1)` (delayed copy)

**Verified Working**: Code review confirms modes are applied in `ApplyVoiceRelationship()`.

**Questions**:
- What coupling creates interesting interplay?
- Should shimmer have more autonomy?
- Is SHADOW mode too "locked in" (same pattern, just shifted)?

### 6. Genre Field Design
**File**: `src/Engine/PatternField.h`, `PatternField.cpp`

The 3x3 archetype grid may:
- Have archetypes that are too similar
- Not span the full techno→IDM range
- Need different archetypes entirely

**Questions**:
- What archetypes should live at each grid position?
- Is 3x3 the right grid size?

### 7. Pattern Length Bug (64 Steps) - FIXED

**Status**: ✅ RESOLVED (2025-12-29)

**Root Cause**: `GenerateBar()` only generated 32-bit masks, but the sequencer state stores 64-bit masks. For 64-step patterns, steps 32-63 had no hits because the mask bits were never set.

**Fix Applied** (`src/Engine/Sequencer.cpp`):
- For patterns > 32 steps, generate two 32-step halves with different seeds
- Combine into 64-bit mask: `(uint64_t)secondHalf << 32 | firstHalf`
- Each half gets its own hit budget, voice relationship, and guard rails
- Second half uses `seed ^ 0xDEADBEEF` for pattern variation

**Tests Added** (`tests/test_sequencer.cpp`):
- "64-step patterns have hits in second half" - verifies second half mask is non-zero
- "64-step patterns fire triggers in second half" - verifies gates fire in steps 32-63

**Related Changes**:
- Updated `Sequencer::GetAnchorMask()` and `GetShimmerMask()` to return `uint64_t`
- Added `Sequencer::GetAuxMask()` method

### 8. Swing Effectiveness
**File**: `src/Engine/BrokenEffects.cpp`

Swing config (Config K2) doesn't produce noticeable timing change.

**Note**: Modification 0.6 in Task 16 fixed the bug where swing config wasn't being used at all. This investigation is to determine if the effect is too subtle or needs different scaling.

**Current Implementation**:
- `ComputeSwing()` uses swing config for base swing amount
- GENRE controls jitter separately
- Swing range: 50% (straight) to 66% (triplet)

**Questions**:
- Is 50%-66% range too narrow?
- Should swing be a multiplier of GENRE's swing instead?
- Is the timing offset applied at the right point in the pipeline?

**Investigation**:
- Log actual timing offsets being applied
- Compare with reference swing implementations
- Test with external clock to verify timing changes

---

## Reference Material

### Spec Sections
- `docs/specs/main.md` section 6: Pattern Field (archetype grid)
- `docs/specs/main.md` section 7: Generation Pipeline
- `docs/specs/main.md` section 8: Guard Rails

### Related Code
```
src/Engine/
├── ArchetypeData.h      # Weight tables per archetype
├── PatternField.h/cpp   # 2D blending, softmax
├── HitBudget.h/cpp      # Budget calculation
├── GumbelSampler.h/cpp  # Top-K selection
├── GuardRails.h/cpp     # Musical constraints
├── VoiceRelation.h/cpp  # Anchor/Shimmer coupling
└── GenerationPipeline.h # Orchestrates generation
```

### Test Logs
- Task 20: `docs/tasks/completed/20-level1-archetype-debug.md` (Round 2 logs)

---

## Potential Approaches

1. **Archetype Tuning**: Redesign weight tables with more contrast
2. **Temperature Sweep**: Find optimal Gumbel temperature for variety
3. **Budget Expansion**: Allow more hits at high ENERGY
4. **Guard Rail Relaxation**: Soften constraints for experimental patterns
5. **Reference Analysis**: Analyze real drum machine patterns for weight inspiration
6. **A/B Testing**: Create multiple archetype sets, compare on hardware

---

## Success Criteria

- [ ] Patterns feel "musical" and "groovy" at center position (Groovy archetype)
- [ ] Clear audible difference between Minimal and Chaos positions
- [ ] K1 (ENERGY) creates smooth density progression
- [ ] K2 (BUILD) creates noticeable phrase-level evolution
- [ ] K3/K4 (FIELD X/Y) create distinct pattern characters
- [ ] Patterns inspire movement/dancing at moderate settings
- [ ] IDM/experimental feels chaotic but intentional at extreme settings
- [x] 64-step patterns have hits throughout (no blank sections) - FIXED 2025-12-29
- [ ] Swing creates audible timing difference (straight → triplet feel)

---

## Notes

This is a **spike task** - the goal is to investigate and prototype improvements, not necessarily ship production code. May result in multiple follow-up tasks for specific improvements.

Hardware testing Levels 4-5 should complete first to ensure timing effects (swing, jitter) work before tuning musicality.
