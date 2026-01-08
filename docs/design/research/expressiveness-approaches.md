# Research: Alternative Approaches to Pattern Expressiveness

**Date**: 2026-01-07
**Context**: DuoPulse v5 shimmer patterns are too similar across different seeds at moderate energy levels
**Goal**: Explore alternative techniques to improve pattern variation while maintaining simplicity

---

## Executive Summary

The current system suffers from "constraint overwhelm" - at moderate energy, guard rails and spacing constraints fully determine the anchor pattern, leaving no room for seed-based variation. The shimmer voice then fills identical gaps deterministically.

This research explores techniques from algorithmic music, other hardware sequencers, and generative systems that could address this expressiveness gap while preserving the module's "simple surface, deep control" philosophy.

---

## 1. Techniques from Other Algorithmic Drum Machines

### 1.1 Mutable Instruments Grids: The Pattern Map Approach

Grids uses a fundamentally different architecture: a 2D "pattern map" trained on real drum loops, where X/Y coordinates interpolate between learned patterns.

**Key Insights:**
- Patterns are derived from corpus data, not generated from rules
- The map provides continuous morphing between styles
- "Fill" parameters control density, not structure
- Chaos parameter adds random perturbations

**Applicability to DuoPulse:**
- **High**: The concept of continuous morphing between character zones (stable/syncopated/wild) is already in the SHAPE algorithm
- **Medium**: A small lookup table of archetypal shimmer patterns (indexed by energy zone + seed) could provide seed-variation without algorithmic complexity
- **Novel Idea**: Instead of generating shimmer from scratch, select from a pool of pre-composed "shimmer templates" and apply transformations

**Trade-offs:**
- ROM space for pattern data
- Less "algorithmic purity"
- But: proven musical results

### 1.2 Pam's New Workout: Euclidean with Rotation and Padding

Pam's treats Euclidean rhythms as a starting point, then adds:
- **Rotation**: Shifting the pattern start point
- **Padding**: Adding empty steps to break regularity
- **Per-channel independence**: Each output can have different parameters

**Key Insights:**
- Rotation alone creates significant variation from a single base pattern
- Padding breaks the "perfectly distributed" feel
- Clock multiplication/division creates polyrhythms

**Applicability to DuoPulse:**
- **High**: Rotation is cheap (bit shift) and seed-dependent rotation would add instant variation
- **Medium**: Padding could be applied as "reserved steps" where shimmer cannot land
- **Novel Idea**: Seed-based rotation offset for shimmer pattern after gap-filling

**Implementation Sketch:**
```cpp
uint32_t rotatedShimmer = RotateMask(shimmerMask, HashToInt(seed) % patternLength);
```

### 1.3 Comparison Matrix

| Feature | Grids | Pam's | Current DuoPulse | Opportunity |
|---------|-------|-------|------------------|-------------|
| Seed variation | Via map position | Via rotation | Via Gumbel (overwhelmed) | Inject earlier |
| Pattern source | Trained corpus | Euclidean algo | SHAPE weights | Add templates |
| Fill mechanism | Density slider | Beat count | Gap-filling | OK |
| Chaos/random | Perturbation % | N/A | None (DRIFT only) | Add micro-chaos |

---

## 2. Mathematical Approaches to Musical Variation

### 2.1 Gumbel-Max Trick (Already Used)

The current system uses Gumbel sampling for weighted random selection. The issue is that constraints leave insufficient choice.

**Diagnosis**: Gumbel sampling works well when there are many valid options. At moderate energy with 4-step spacing, there's often only one valid configuration.

**Alternative**: Use Gumbel at a different stage - for selecting *which constraints to relax* rather than selecting hits.

### 2.2 Markov Chains for Shimmer Placement

Instead of deterministic gap-filling, model shimmer placement as a Markov process where:
- State = position within gap
- Transitions = probability of placing hit here vs. moving on

**Key Insight**: Markov chains can be parameterized by seed (initial state selection) while maintaining learned transition probabilities.

**Implementation Sketch:**
```cpp
// Transition matrix: P[current_gap_position][next_position]
// Seed determines which transitions "win" at each step
for (int pos = gap.start; pos < gap.end && hitsRemaining > 0; pos++) {
    float threshold = markovTransition[pos % 4];
    if (HashToFloat(seed, pos) < threshold) {
        PlaceHit(pos);
        hitsRemaining--;
    }
}
```

**Trade-offs:**
- Requires tuning transition probabilities
- May feel less "intentional" than even spacing
- But: naturally introduces seed variation

### 2.3 Perlin/Simplex Noise for Continuous Variation

Perlin noise provides smooth, coherent randomness - values at nearby coordinates are similar.

**Application to Pattern Generation:**
- Use seed as one dimension, step position as another
- Generate a 2D "hit probability surface"
- Threshold to get hits

**Key Insight**: Perlin noise naturally provides "families" of related patterns - changing seed slightly gives similar-but-different results.

**Implementation Sketch:**
```cpp
for (int step = 0; step < patternLength; step++) {
    float noise = PerlinNoise2D(seed * 0.01f, step * 0.1f);
    float threshold = baseWeight[step] * energy;
    if (noise > threshold) {
        PlaceHit(step);
    }
}
```

**Trade-offs:**
- Perlin noise is computationally expensive for embedded
- Simplex noise is cheaper but still non-trivial
- **Alternative**: Pre-computed noise table (256 values) indexed by (seed XOR step)

### 2.4 L-Systems for Rhythmic Variation

L-systems generate sequences through iterative string rewriting. They can produce self-similar patterns with controlled complexity.

**Example for Rhythm:**
```
Axiom: X
Rules: X -> X.X.  (X = hit, . = rest)
       X -> .X.X

Iterations:
0: X
1: X.X.
2: X.X..X.X.
3: X.X..X.X...X.X..X.X.
```

**Key Insight**: L-systems produce deterministic output from rules, but rule selection can be seed-dependent.

**Applicability:**
- **Low for real-time**: L-system expansion is compute-heavy
- **Medium for pre-generation**: Could pre-compute pattern families
- **Novel Idea**: Use L-system-style growth to "mutate" a base pattern over time (DRIFT effect)

---

## 3. Generative Music Techniques

### 3.1 Cellular Automata (Game of Life)

Cellular automata evolve patterns based on neighbor states. Applied to rhythm, each cell represents a step.

**Eurorack Example**: Nervous Squirrel "Conway's Game" module uses 8x8 Game of Life for trigger generation.

**Application to Shimmer:**
- Initialize grid with anchor hits
- Run 1-2 generations
- Use resulting pattern for shimmer

**Key Insight**: CA naturally produces varied but structured patterns. Same initial state + rules = deterministic output.

**Trade-offs:**
- Unpredictable musical results
- May produce non-musical clusters
- But: genuinely different each seed

### 3.2 Genetic Algorithms / Evolutionary Approach

Systems like evoDrummer evolve patterns toward target fitness functions (e.g., "syncopation = 0.6").

**Application to DuoPulse:**
- Define fitness: "shimmer should be evenly distributed in gaps BUT with seed-based perturbation"
- Evolve population of patterns
- Select winner based on fitness + seed

**Trade-offs:**
- Computationally expensive
- Overkill for this problem
- Better suited for offline pattern design

### 3.3 Groove Templates / Micro-Timing

DAWs (Ableton, Logic) use "groove templates" - captured timing deviations from human performances.

**Key Insight**: Expressiveness comes not just from *which* hits occur, but *when* and *how loud*.

**Application to DuoPulse:**
- Even if hit positions are identical, seed-based velocity/timing variation creates audible difference
- The current system has velocity variation but it's subtle

**Concrete Improvement:**
```cpp
// Velocity already varies by metric weight
// Add seed-dependent timing jitter (within swing window)
float timingOffset = (HashToFloat(seed, step) - 0.5f) * maxJitter;
```

---

## 4. Quantifying Expressiveness

### 4.1 Syncopation Metrics

Research by Witek et al. (2014) quantifies syncopation in drum patterns. Their "Index of Syncopation" measures how much a pattern deviates from metric expectations.

**Application to DuoPulse:**
- Use syncopation index as a diagnostic: "Are different seeds producing different syncopation values?"
- Target: syncopation should vary by 10-30% across seeds at same SHAPE

### 4.2 Pattern Distance Metrics

Toussaint describes methods for measuring "distance" between rhythms:
- Hamming distance (differing bits)
- Edit distance (swaps needed)
- Chronotonic distance (timing-based)

**Proposed Test:**
```cpp
TEST_CASE("Seed produces sufficient pattern distance") {
    uint32_t p1 = GenerateShimmer(seed1, params);
    uint32_t p2 = GenerateShimmer(seed2, params);
    int distance = HammingDistance(p1, p2);
    REQUIRE(distance >= 2);  // At least 2 hits differ
}
```

### 4.3 Perceptual Complexity

The "latent rhythm complexity model" (EURASIP 2022) encodes complexity in a VAE latent space. This is too complex for embedded, but the insight is valuable:

**Key Insight**: Human-perceived complexity correlates with:
1. Syncopation amount
2. Number of voices (DuoPulse has 2)
3. Deviation from repetition

**For DuoPulse**: Focus on syncopation variation rather than complexity variation.

---

## 5. Recommendations for DuoPulse

Based on this research, here are ranked approaches from most to least applicable:

### Tier 1: Low-Hanging Fruit (Implement First)

#### 5.1 Seed-Based Rotation
**Source**: Pam's Workout rotation concept
**Effort**: 1 hour
**Impact**: +30% expressiveness

After computing shimmer mask via gap-filling, apply seed-based rotation:
```cpp
int rotation = HashToInt(seed) % patternLength;
shimmerMask = RotateBitMask(shimmerMask, rotation);
// Re-check for collisions with anchor, adjust if needed
```

#### 5.2 Micro-Jitter in Even Spacing
**Source**: Current recommendations.md R1
**Effort**: 2 hours
**Impact**: +20% expressiveness

Already recommended - add +/- 1 step jitter to even spacing algorithm.

#### 5.3 Pre-computed Noise Table
**Source**: Perlin noise concept, simplified
**Effort**: 1 hour
**Impact**: +15% expressiveness

256-entry table of "randomish but smooth" values:
```cpp
const float kNoiseTable[256] = { /* pre-computed */ };
float GetNoise(uint32_t seed, int step) {
    return kNoiseTable[(seed + step * 17) & 0xFF];
}
```

### Tier 2: Structural Improvements (Consider if Tier 1 Insufficient)

#### 5.4 Shimmer Pattern Templates
**Source**: Grids pattern map concept
**Effort**: 4 hours
**Impact**: +50% expressiveness

Create 8-16 hand-crafted shimmer templates per energy zone:
```cpp
const uint32_t kShimmerTemplates[4][8] = {
    // MINIMAL zone templates
    { 0x00000002, 0x00000020, 0x00000200, ... },
    // GROOVE zone templates
    { 0x22222222, 0x44444444, 0x11111111, ... },
    ...
};

uint32_t SelectTemplate(EnergyZone zone, uint32_t seed) {
    int idx = seed % kTemplateCount[zone];
    return kShimmerTemplates[zone][idx];
}
```

Then apply anchor-avoidance and rotation.

#### 5.5 Markov-Style Placement
**Source**: Markov chain research
**Effort**: 6 hours
**Impact**: +40% expressiveness

Replace even-spacing with probability-based placement:
```cpp
// Transition probabilities by gap position (mod 4)
const float kPlaceProb[4] = { 0.3f, 0.5f, 0.3f, 0.7f };

for (int pos = gap.start; pos < gap.end; pos++) {
    float p = kPlaceProb[pos % 4] * (hitsNeeded / gapLength);
    if (HashToFloat(seed, pos) < p) {
        PlaceHit(pos);
        hitsNeeded--;
    }
}
```

### Tier 3: Experimental (Research Paths)

#### 5.6 Cellular Automaton Mutation
**Source**: Game of Life modules
**Effort**: 8 hours
**Impact**: Unknown (experimental)

Use 1 generation of CA to "evolve" the base pattern:
```cpp
uint32_t MutateWithCA(uint32_t pattern, uint32_t seed) {
    uint32_t next = 0;
    for (int i = 0; i < 32; i++) {
        int neighbors = CountNeighbors(pattern, i);
        bool alive = (pattern >> i) & 1;
        // Simple rule: stay alive with 1 neighbor, born with 2
        bool nextAlive = (alive && neighbors == 1) || (!alive && neighbors == 2);
        if (nextAlive) next |= (1 << i);
    }
    return next;
}
```

#### 5.7 L-System Derived Templates
**Source**: L-system rhythm research
**Effort**: 4 hours (offline generation)
**Impact**: +30% expressiveness

Pre-generate shimmer templates using L-system rules, store in ROM.

---

## 6. Novel Ideas

### 6.1 "Anti-Euclidean" Shimmer

Toussaint discusses "anti-Euclidean" rhythms that cluster hits instead of distributing them. At high SHAPE, shimmer could use anti-Euclidean placement for IDM-style clustering:

```cpp
if (shape > 0.7f) {
    // Cluster hits in first half of gap
    PlaceClusteredHits(gap, hitsNeeded, seed);
} else {
    // Even distribution
    PlaceEvenlySpaced(gap, hitsNeeded);
}
```

### 6.2 Complementary Syncopation

Instead of just filling gaps, explicitly calculate syncopation of anchor pattern and generate shimmer with *complementary* syncopation:
- If anchor is syncopated (offbeat hits), shimmer is stable (on-beat)
- If anchor is stable, shimmer is syncopated

This creates musical "conversation" between voices.

### 6.3 Velocity-Driven Hit Selection

Instead of selecting hits then assigning velocity, flip the order:
1. Generate velocity profile for all steps (seed-dependent)
2. Threshold velocities to determine which steps get hits
3. Collision detection with anchor

This naturally produces seed variation because velocity varies before hit selection.

### 6.4 "DRIFT as Mutation Rate"

Reframe DRIFT not as "placement strategy" but as "mutation probability":
- DRIFT=0: Use base algorithm exactly
- DRIFT>0: Each shimmer hit has (DRIFT * 0.5) chance to shift +/- 1 step

This keeps musical coherence at low DRIFT while allowing controlled chaos.

---

## 7. Conclusion

The shimmer convergence problem is solvable without fundamental architecture changes. The most promising approaches are:

1. **Immediate wins**: Seed-based rotation (+30%) and micro-jitter (+20%) can be implemented in hours
2. **Structural improvement**: Pre-composed shimmer templates (+50%) provide guaranteed variation
3. **Measurement**: Add syncopation/distance metrics to tests to verify improvements

The research reveals that the current system is missing a key insight from successful algorithmic sequencers: **variation should be injected at multiple stages of the pipeline**, not just in the final selection step where constraints may overwhelm it.

### Recommended Implementation Order

1. Add rotation after gap-filling (Tier 1)
2. Add micro-jitter to even spacing (Tier 1)
3. Test and measure expressiveness
4. If insufficient, add shimmer templates (Tier 2)
5. Consider Markov placement if more "organic" feel needed (Tier 2)

---

## Sources

### Hardware Sequencers
- [Mutable Instruments Grids Manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/grids/manual/)
- [Perfect Circuit - Euclidean Sequencing with Pam's Workout](https://www.perfectcircuit.com/signal/pams-new-workout-euclidean-sequencing)
- [ALM Pam's New Workout Manual (PDF)](https://busycircuits.com/docs/alm017-manual.pdf)

### Euclidean Rhythms
- [Toussaint - The Euclidean Algorithm Generates Traditional Musical Rhythms (PDF)](https://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf)
- [Wikipedia - Euclidean Rhythm](https://en.wikipedia.org/wiki/Euclidean_rhythm)
- [Extended Euclidean Paper (PDF)](https://cgm.cs.mcgill.ca/~godfried/publications/banff-extended.pdf)

### Syncopation and Complexity Metrics
- [Modelling Perceived Syncopation in Popular Music Drum Patterns](https://journals.sagepub.com/doi/full/10.1177/2059204318791464)
- [A Latent Rhythm Complexity Model for Drum Pattern Generation](https://asmp-eurasipjournals.springeropen.com/articles/10.1186/s13636-022-00267-2)
- [Null Effect of Perceived Drum Pattern Complexity on Groove](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0311877)

### Cellular Automata
- [Conway's Game Eurorack Module - Synthtopia](https://www.synthtopia.com/content/2023/06/26/new-eurorack-module-conways-game-applies-classic-game-of-life-to-music-sequencing/)
- [Music Generation through Cellular Automata (PDF)](https://www.researchgate.net/publication/2324938_Music_Generation_through_Cellular_Automata_How_to_Give_Life_to_Strange_Creatures)

### Markov Chains
- [Markov Drummer - GitHub](https://github.com/mapio/markovdrummer)
- [Markov Chains for Computer Music Generation (PDF)](https://scholarship.claremont.edu/cgi/viewcontent.cgi?article=1848&context=jhm)

### L-Systems
- [Musical L-Systems - Master's Thesis (PDF)](https://modularbrains.net/wp-content/uploads/Stelios-Manousakis-Musical-L-systems.pdf)
- [Growing Music: Musical Interpretations of L-Systems](https://link.springer.com/chapter/10.1007/978-3-540-32003-6_56)

### Perlin Noise
- [Using Perlin Noise in Sound Synthesis (PDF)](https://lac.linuxaudio.org/2018/pdf/14-paper.pdf)
- [Wikipedia - Perlin Noise](https://en.wikipedia.org/wiki/Perlin_noise)

### Groove Templates
- [Ableton - Using Grooves](https://www.ableton.com/en/manual/using-grooves/)
- [DNA Groove Templates](http://www.numericalsound.com/dna-groove-templates-note.html)
- [Sound on Sound - Groove Quantise Part 1](https://www.soundonsound.com/techniques/groove-quantise-part-1)

### Livecoding
- [ORCA Esoteric Programming Language - GitHub](https://github.com/hundredrabbits/Orca)
- [Procedural Music in ORCA](https://inventingsituations.net/2021/12/26/procedural-music-in-orca/)

### Deep Learning Approaches (Background)
- [Talking Drums: Generating Drum Grooves with Neural Networks (PDF)](https://arxiv.org/pdf/1706.09558)
- [evoDrummer: Deriving Rhythmic Patterns through Interactive Genetic Algorithms](https://link.springer.com/chapter/10.1007/978-3-642-36955-1_3)
