# Task 21: Musicality Improvements - Ideation Document

**Created**: 2025-12-30
**Parent**: Task 21 (Musicality Improvements Spike)
**Status**: IDEATION

---

## Executive Summary

This document explores improvements to DuoPulse v4's musicality, focusing on:
1. **Velocity CV as expressive element** - using dynamics to communicate energy/groove
2. **Hat-centric shimmer redesign** - repurposing shimmer + aux for hi-hat voices
3. **Pattern generation from first principles** - guaranteeing musical results
4. **BUILD and morphing opportunities** - phrase-level dynamics

---

## 1. Velocity CV for Energy & Rhythm

### Current State

```
PUNCH parameter controls:
- Floor velocity: 0.70 → 0.30 (as PUNCH increases)
- Accent boost: 0.10 → 0.35
- Random variation: ±5% → ±20%
- Result: Velocity range ~0.2-1.0
```

**Problem**: Velocity variation feels subtle. Ghost notes at 50-70% velocity don't create enough contrast with accented notes at 85-100%.

### Research Insights

From [MusicRadar](https://www.musicradar.com/tuition/tech/how-to-add-groove-and-pace-to-a-beat-using-ghost-notes-625526) and [Native Instruments](https://blog.native-instruments.com/drum-programming-101/):

> "Ghost notes need to be quieter and lower in velocity than the main hits, typically set around velocity 79" (out of 127, ~62%)

> "Accented notes are much better when quantized. The smaller notes in between can give variation and groove."

Key velocity zones in drum programming:
- **Accent hits**: 100-127 (80-100%) - strong downbeats
- **Normal hits**: 90-110 (70-85%) - standard groove
- **Ghost notes**: 50-80 (40-65%) - feel/pocket
- **Soft ghosts**: 30-50 (25-40%) - barely audible texture

### Proposed Velocity Model

**Three-tier velocity system:**

```cpp
enum class HitIntensity {
    ACCENT,      // 0.85-1.0 - downbeats, emphasized hits
    NORMAL,      // 0.55-0.80 - standard pattern hits
    GHOST        // 0.25-0.50 - groove/feel notes
};
```

**Intensity assignment by position:**

| Step Position | Default Intensity | Notes |
|--------------|-------------------|-------|
| Beat (0,8,16,24) | ACCENT | Downbeats always strong |
| Offbeat "&" (4,12,20,28) | NORMAL | Secondary accent points |
| "e" and "a" (odd steps) | GHOST | Subdivisions create groove |

**Archetype-driven intensity scaling:**

```cpp
// Per-archetype intensity maps (replacing flat probability weights)
struct ArchetypeIntensity {
    float accentWeight[32];   // Probability of ACCENT at each step
    float normalWeight[32];   // Probability of NORMAL
    float ghostWeight[32];    // Probability of GHOST
};
```

**BUILD modifier on velocity:**

| Phrase Progress | Effect |
|----------------|--------|
| 0-50% | Normal velocity distribution |
| 50-75% | Ghost notes become NORMAL (building) |
| 75-87.5% | NORMAL becomes ACCENT (tension) |
| 87.5-100% (fill) | All hits ACCENT (climax) |

### Open Questions

1. Should velocity CV output be **sample-and-hold** (current) or **decaying envelope**?
2. Should we add a **velocity sequence** separate from hit generation?
3. How does velocity interact with **external VCA/filter**? (0-5V range sufficient?)

---

## 2. Hat-Centric Shimmer + Aux Redesign

### Current State

- **Shimmer**: Clap/snare voice with backbeat focus
- **Aux** (HAT mode): Third trigger voice, flat velocity
- **Problem**: Hi-hats are critical to techno groove but treated as afterthought

### Research Insights

From [Production Expert](https://www.production-expert.com/production-expert-1/6-killer-hi-hat-programmingnbsptipsnbspfor-musicnbspproducers) and [ModeAudio](https://modeaudio.com/magazine/creating-natural-hi-hat-patterns):

> "Open and closed hi-hats can't play simultaneously - closed hats choke open hats"

> "With eighth-note hi-hat patterns, a drummer will semi-consciously emphasise the beat, playing offbeat hits slightly quieter than on-beat ones"

> "Use choke and accent to mimic a real TR-909"

### Proposed Architecture

**Voice assignment:**
- **Anchor**: Kick drum (unchanged)
- **Shimmer → Hi-Hat**: Closed hat primary, velocity-driven
- **Aux → Open/Closed Switch**: Gate high = open hat mode

**Hi-hat pattern philosophy:**

```
Classic 8th-note pattern (velocity shown):
Step:  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
Hat:   C   -   C   -   O   -   C   -   C   -   C   -   O   -   C   -
Vel:  100  -  70  -  80  -  70  -  100 -  70  -  80  -  70  -

Legend: C = closed, O = open, - = rest
Open hats on offbeats (&), closed everywhere else
```

**Aux as Open/Closed selector:**

| Aux Gate | Shimmer Behavior |
|----------|-----------------|
| LOW (0V) | Closed hi-hat - short decay, punchy |
| HIGH (5V) | Open hi-hat - long decay, washy |

**Implementation approach:**

```cpp
// In OutputManager or new HatOutput class
void ProcessHatOutput(uint32_t shimmerMask, uint32_t openMask, ...) {
    // shimmerMask = which steps have ANY hat hit
    // openMask = which steps should be OPEN (from aux pattern)

    for (int step = 0; step < 32; step++) {
        if (shimmerMask & (1 << step)) {
            bool isOpen = openMask & (1 << step);

            // Choke logic: open hat cuts short if closed follows
            if (isOpen && (shimmerMask & (1 << (step + 1)))) {
                // Schedule choke at next step
            }

            // Output: shimmer gate + aux gate for open/closed
        }
    }
}
```

**Archetype-driven open hat placement:**

```cpp
// New field in archetype data
constexpr uint32_t kMinimal_OpenHatMask = 0x10101010;  // "&" of each beat
constexpr uint32_t kGroovy_OpenHatMask  = 0x10100010;  // Sparse opens
constexpr uint32_t kChaos_OpenHatMask   = 0x5A5A5A5A;  // Irregular opens
```

### Config Mode Considerations

**Existing aux controls** (must remain compatible):
- Config K3: AUX MODE (HAT/Fill/Phrase/Event)
- Config K3 + Shift: AUX DENSITY

**Proposed additions:**
- When AUX MODE = HAT:
  - K3 + Shift could become "Open Hat Density" (0-100%)
  - New config parameter: "Hat Style" (8ths/16ths/Triplets)?

### Open Questions

1. Should we **rename shimmer** to "hat" in documentation/display?
2. How to handle **choke timing** in hardware? (need fast gate-off)
3. Should open/closed be **sample-based switching** or **CV to external module**?
4. What about users who want shimmer as clap/snare? **Config option for voice role?**

---

## 3. Guaranteed "Good" Patterns - First Principles

### The Problem

User feedback: "patterns feel repetitive, not musical"

**Root cause analysis:**
1. Weight tables are too similar across archetypes
2. Gumbel sampler with high temp → unpredictable, not structured
3. Guard rails either too strict (boring) or too loose (chaotic)
4. No underlying rhythmic theory ensuring musicality

### First Principles of Groove

From [Euclidean rhythm research](https://www.lawtonhall.com/blog/euclidean-rhythms-pt1) and traditional music theory:

**Principle 1: Maximum Evenness**
> "Euclidean rhythms distribute beats as evenly as possible across the pattern"

Good patterns have hits spread across the timeline, not clustered.

**Principle 2: Hierarchical Metric Structure**
```
Metric weight by position (16-step bar):
Step 0:  ████████████  (downbeat - strongest)
Step 8:  ██████████    (beat 3)
Step 4:  ████████      (beat 2)
Step 12: ████████      (beat 4)
Step 2:  ██████        (8th subdivisions)
Step 6:  ██████
Step 10: ██████
Step 14: ██████
Step 1,3,5...: ████    (16th subdivisions - weakest)
```

**Principle 3: Tension/Resolution Cycle**
- Patterns should have "stable" sections (hits on strong beats)
- Followed by "unstable" sections (syncopation, off-grid)
- Resolution back to stability (downbeat)

**Principle 4: Call and Response**
- Anchor and shimmer should interlock, not duplicate
- Silence in one voice = opportunity for other voice

### Proposed: Euclidean Foundation Layer

**Core idea**: Generate patterns using Euclidean algorithm, then distort based on archetype.

```cpp
// Euclidean rhythm generator
uint32_t GenerateEuclidean(int hits, int steps) {
    // E(5, 16) = 1001010010100101 (bossa nova)
    // E(4, 16) = 1000100010001000 (four-on-floor)
    // E(3, 8)  = 10010010 (tresillo)
    ...
}

// Pattern generation pipeline:
// 1. Pick Euclidean pattern based on hit budget
// 2. Rotate based on seed (different starting point)
// 3. Apply archetype displacement (shift some hits off-grid)
// 4. Apply guard rails (ensure musical constraints)
```

**Euclidean presets per complexity level:**

| Hit Budget | Euclidean | Character |
|------------|-----------|-----------|
| 2 | E(2,16) | Minimal - just 1 and "&" |
| 3 | E(3,16) | Tresillo feel |
| 4 | E(4,16) | Four-on-floor |
| 5 | E(5,16) | Bossa nova |
| 6 | E(6,16) | Afrobeat |
| 7 | E(7,16) | Dense groove |
| 8 | E(8,16) | Half-time 16ths |

**Displacement layer:**

After Euclidean foundation, apply archetype-specific displacement:

```cpp
struct DisplacementRule {
    uint32_t eligibleMask;  // Which Euclidean hits CAN be displaced
    float probability;      // Chance to displace (0.0-1.0)
    int8_t offset;          // Steps to shift (-2 to +2)
};

// Example: Groovy archetype
DisplacementRule groovyRules[] = {
    { 0x44444444, 0.3f, +1 },  // 30% chance to push beat 2 late
    { 0x11111111, 0.0f,  0 },  // Never displace downbeats
};
```

### Quality Assurance Checks

**Post-generation validation:**

```cpp
bool ValidatePattern(uint32_t mask, int patternLength) {
    // 1. Check hit density (2-12 hits per bar)
    int hits = __builtin_popcount(mask);
    if (hits < 2 || hits > 12) return false;

    // 2. Check for beat-1 presence (usually required)
    if (!(mask & 0x00010001)) return false;  // No beat 1 in either bar

    // 3. Check maximum gap (no more than 8 empty steps)
    for (int i = 0; i < patternLength; i++) {
        int gap = CountGapFrom(mask, i);
        if (gap > 8) return false;
    }

    // 4. Check for cluster avoidance (no more than 3 consecutive)
    if (HasCluster(mask, 4)) return false;

    return true;
}
```

### Open Questions

1. Should Euclidean be the **foundation** or a **fallback** when probabilistic fails?
2. How to balance **predictability** (Euclidean) with **variety** (current system)?
3. Should users be able to **choose** Euclidean mode explicitly?
4. How do Euclidean patterns interact with **swing**?

### User Answers
Seems like Euclidian/probabilistic dichotomy can be different beween each genre (Techno/Tribal more euclidian, IDM more probabilistic). 
Try to see if there's a weighted gradient we can achieve between archetypes/energy/build/genre values. Don't overcomplicate it too much though.

---

## 4. BUILD and Pattern Morphing

### Current BUILD Implementation

```cpp
// BUILD affects velocity and density
void ComputeBuildModifiers(float build, float phraseProgress, ...) {
    // Density ramps: 1.0 → 1.5× toward phrase end
    // Fill zone: last 12.5% gets velocity boost
}
```

**Problem**: BUILD is subtle - only affects velocity and slight density increase.

### Proposed BUILD Enhancements

**Phase-based BUILD:**

| Phrase % | Phase | Effects |
|----------|-------|---------|
| 0-25% | INTRO | Sparse pattern, minimal velocity variation |
| 25-50% | GROOVE | Full pattern, normal velocity |
| 50-75% | BUILD | +25% hit budget, ghost→normal velocity |
| 75-87.5% | TENSION | +50% budget, open hats increase, velocity rise |
| 87.5-100% | FILL | Maximum density, all accents, ratchets enabled |

**Pattern morphing across phrase:**

```cpp
// Morph anchor pattern toward next archetype during build
void MorphPatternOverPhrase(
    const ArchetypeDNA& current,
    const ArchetypeDNA& next,
    float phraseProgress,
    uint32_t& outMask
) {
    // At phrase start: 100% current
    // At phrase end: 50% current, 50% next
    float morphAmount = phraseProgress * 0.5f;

    // Blend weight tables
    for (int i = 0; i < 32; i++) {
        float weight = lerp(current.anchorWeights[i],
                           next.anchorWeights[i],
                           morphAmount);
        // Generate hits from blended weights
    }
}
```

**BUILD affects open/closed hat ratio:**

```cpp
// As BUILD increases, more open hats
float openHatProbability = baseOpen + build * phraseProgress * 0.5f;

// During fill zone: 80% open hats for wash effect
if (inFillZone) {
    openHatProbability = 0.8f;
}
```

**Velocity envelope over phrase:**

```
Phrase:   |--INTRO--|--GROOVE--|--BUILD--|--TENSION--|--FILL--|
Velocity: |   0.6   |   0.75   |   0.85  |    0.95   |  1.0   |
            ▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔
```

### Open Questions

1. Should BUILD affect **pattern content** (more hits) or just **velocity/timing**? // Both pattern content and velocity/timing.
2. How long is a "phrase"? Currently 8 bars - should this be configurable? // I don't want it to have its own config, but maybe it can change with either pattern length/archetype/genre? ok to lock it to 4 or 8 bars otherwise.
3. Should morphing target be **random** or **related archetype**? // Not sure, related archetype feels better though.
4. How does BUILD interact with **manual knob tweaks**? (user override?) // Should influence but not override.

---

## 5. Archetype Redesign Considerations

### Current Issues

Looking at current `ArchetypeData.h`:

1. **Weight tables are too uniform**: Weights of 0.5-0.7 create similar patterns
2. **Not enough contrast**: Minimal vs Chaos don't feel different enough
3. **Off-beat weights too low**: Ghost notes never win selection

### Proposed Weight Philosophy

**Stronger contrast:**

```cpp
// Current Minimal anchor (too uniform):
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f,  0.9f, 0.0f, 0.0f, 0.0f, ...
};

// Proposed Minimal (more zeros, binary feel):
constexpr float kMinimal_Anchor[32] = {
    1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,  // Pure 4/4
    1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f,
    // Weights: 1.0 or 0.0 only - deterministic
};

// Proposed Chaos (more non-zero, probabilistic):
constexpr float kChaos_Anchor[32] = {
    0.9f, 0.4f, 0.6f, 0.3f,  0.5f, 0.4f, 0.7f, 0.3f,  // Many options
    0.4f, 0.5f, 0.6f, 0.4f,  0.7f, 0.3f, 0.5f, 0.6f,
    // Weights: 0.3-0.9 range - unpredictable
};
```

**Ghost note integration:**

```cpp
// Add ghost layer to archetypes
constexpr float kGroovy_GhostLayer[32] = {
    0.0f, 0.3f, 0.0f, 0.4f,  0.0f, 0.3f, 0.0f, 0.35f,  // "e" and "a"
    0.0f, 0.3f, 0.0f, 0.4f,  0.0f, 0.3f, 0.0f, 0.35f,
    // Ghost weights: 0.3-0.4 range
};
```

---

## 6. Summary of Proposed Changes

### High Priority (Core Musicality)

| Change | Files Affected | Complexity |
|--------|---------------|------------|
| Three-tier velocity system | VelocityCompute.* | Medium |
| Euclidean foundation layer | New: EuclideanGen.*, HitBudget.* | Medium |
| Hat-centric shimmer | ArchetypeData.h, VoiceRelation.* | Medium |

### Medium Priority (Enhancement)

| Change | Files Affected | Complexity |
|--------|---------------|------------|
| Aux as open/closed gate | AuxOutput.*, OutputManager.* | Low |
| BUILD phase system | VelocityCompute.*, HitBudget.* | Medium |
| Archetype weight redesign | ArchetypeData.h | High (tuning) |

### Low Priority (Polish)

| Change | Files Affected | Complexity |
|--------|---------------|------------|
| Pattern morphing over phrase | PatternField.*, Sequencer.* | High |
| Quality assurance validation | GuardRails.* | Medium |
| Config mode for voice roles | UI/Config code | Medium |

---

## 7. Open Questions Summary

### Architecture Questions
1. Should Euclidean be foundation or fallback? // Genre-specific (techno full euclidian, tribal middle, IDM mostly probabilisitc)
2. How to handle users wanting clap instead of hat on shimmer? // Config setting (can it somehow merge with AUX/VOICE/BALANCE setting?)
3. Should voice roles be configurable in hardware? // Config mode setting possible, keep it simple though, bear AUX setting in mind (should act as additional trig for whatever is 'missing')

### Tuning Questions
4. What velocity ranges work best with typical VCAs? // We have a uVCA that allows for exponential/linear VCA response. Range should be flexible enough to express musicality preferences within genre but very low velocity values don't seem useful. Some reasonable minimum makes sense (30% maybe but flexible on that)
5. How much open hat in fill zones? // Whatever seems reasonable from a musicality perspective. Alternating CH/OH patterns are nice.
6. How different should archetypes be from each other? // Reasonably different, we want to have a lot of expressivity.

### UX Questions
7. Should phrase length be user-configurable? // No, pattern length only to keep it simple.
8. How to communicate BUILD phase to user? (LED feedback?) // No need, if build is reasonably different then we'll hear it.
9. Should there be a "safe mode" that guarantees 4/4 patterns? // Yes, root techno archetype at mid-energy should guarantee 4/4

---

## 8. References

### Music Theory
- [Native Instruments: Drum Programming 101](https://blog.native-instruments.com/drum-programming-101/)
- [LANDR: Euclidean Rhythms](https://blog.landr.com/euclidean-rhythms/)
- [MusicRadar: Ghost Notes](https://www.musicradar.com/tuition/tech/how-to-add-groove-and-pace-to-a-beat-using-ghost-notes-625526)
- [Production Expert: Hi-Hat Programming](https://www.production-expert.com/production-expert-1/6-killer-hi-hat-programmingnbsptipsnbspfor-musicnbspproducers)
- [ModeAudio: Natural Hi-Hat Patterns](https://modeaudio.com/magazine/creating-natural-hi-hat-patterns)
- [Toussaint: Euclidean Algorithm Generates Traditional Musical Rhythms](http://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf)

### Technical
- Existing codebase: `src/Engine/` directory
- Spec: `docs/specs/main.md` sections 5-8
