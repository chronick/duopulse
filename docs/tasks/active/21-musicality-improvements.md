# Task 21: Musicality Improvements Spike

**Status**: PENDING
**Branch**: TBD
**Parent Task**: Task 16 (Hardware Validation)
**Related**: Task 20 (Level 1 Archetype Debug)

---

## Problem Statement

During Level 2-3 hardware testing (Task 20), user feedback indicated that generated patterns:
1. **Lack variety** - Patterns feel repetitive, not enough differentiation across the archetype grid
2. **Don't feel very musical** - Patterns lack the groove/feel expected from a techno sequencer
3. **K2 (BUILD) effect hard to notice** - Density-over-phrase not audibly impactful

### User Quotes (from Task 20)
> "not feeling like I'm getting very musical beats, nor am I getting much variety"

> "patterns are changing and are generally following the intended flow, though they don't seem to have a ton of variation"

> "K2 does something but it's hard to notice much change"

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
**File**: `src/Engine/VoiceRelation.h`, `VoiceRelation.cpp`

Coupling between anchor/shimmer may:
- Be too tight, reducing independence
- Force shimmer to always follow anchor

**Questions**:
- What coupling creates interesting interplay?
- Should shimmer have more autonomy?

### 6. Genre Field Design
**File**: `src/Engine/PatternField.h`, `PatternField.cpp`

The 3x3 archetype grid may:
- Have archetypes that are too similar
- Not span the full techno→IDM range
- Need different archetypes entirely

**Questions**:
- What archetypes should live at each grid position?
- Is 3x3 the right grid size?

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

---

## Notes

This is a **spike task** - the goal is to investigate and prototype improvements, not necessarily ship production code. May result in multiple follow-up tasks for specific improvements.

Hardware testing Levels 4-5 should complete first to ensure timing effects (swing, jitter) work before tuning musicality.
