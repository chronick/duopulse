# DuoPulse: 2-Voice Percussive Sequencer

**Specification v2.0**  
**Second Iteration Synthesis**  
**Date: 2025-11-30**

---

## Executive Summary

**DuoPulse** is an opinionated 2-voice percussion sequencer optimized for the Patch.Init hardware. It embraces the two-output constraint as a creative feature, treating the pair of lanes as a unified rhythmic instrument with complementary roles. The sequencer is tuned for electronic genres—techno, tribal, trip-hop, and IDM—with genre-aware swing, algorithmic pattern generation, and deep performance controls.

---

## Design Philosophy

### Core Principles

1. **Embrace the Constraint**: Two voices, maximally expressive. No compromise mapping of 3 voices to 2 outputs.
2. **Abstract Vocabulary**: "Anchor" and "Shimmer" replace traditional drum names, encouraging modular synthesis thinking.
3. **Genre-Opinionated**: The sequencer knows what sounds good in techno, tribal, trip-hop, and IDM—and applies that knowledge to swing, density, and accent behavior.
4. **Performance-First**: Core controls are immediate and tactile; deeper configuration is accessible but never in the way.
5. **CV is King**: CV inputs always map to the four main performance parameters, regardless of mode.

### Voice Architecture

| Voice | Name | Character | Typical Use |
|-------|------|-----------|-------------|
| **1** | **Anchor** | Foundation, grounded, pulse | Kicks, bass hits, low toms |
| **2** | **Shimmer** | Contrast, sparkle, movement | Snares, hi-hats, clicks, transients |

**Anchor** provides momentum and groove. **Shimmer** provides contrast and syncopation. Together, they create interlocked rhythms.

---

## Hardware Mapping

### Inputs

| Input | Function |
|-------|----------|
| **Knobs 1-4** | Performance or Config controls (mode-dependent) |
| **CV 5-8** | **Always** modulates Knobs 1-4 performance parameters (additive) |
| **Button (B7)** | **SHIFT** - Access secondary control layer (hold) |
| **Switch (B8)** | **DOWN** = Performance Mode, **UP** = Config Mode |
| **Gate In 1** | External Clock (overrides internal) |
| **Gate In 2** | Reset (return to step 0) |

### Outputs

| Output | Function |
|--------|----------|
| **Gate Out 1** | Anchor Trigger (5V digital) |
| **Gate Out 2** | Shimmer Trigger (5V digital) |
| **Audio Out 1** | Anchor CV (0-5V, velocity/expression) |
| **Audio Out 2** | Shimmer CV (0-5V, velocity/expression) |
| **CV Out 1** | Clock Output (**respects swing timing**) |
| **CV Out 2** | LED Feedback (mode/trig/parameter brightness) |

---

## Control Scheme

### Performance Mode (Switch DOWN)

Primary playing mode. Optimized for live manipulation.

#### Primary Layer (No Shift)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1** | **ANCHOR DENSITY** | Hit frequency for Anchor lane | Sparse ← → Busy |
| **K2** | **SHIMMER DENSITY** | Hit frequency for Shimmer lane | Sparse ← → Busy |
| **K3** | **FLUX** | Global variation, fills, ghost notes | Steady ← → Chaotic |
| **K4** | **FUSE** | Cross-lane energy exchange | Anchor-heavy ← → Shimmer-heavy |

**ANCHOR/SHIMMER DENSITY**: Controls the probability of hits per step. At low values, only downbeats fire. At high values, dense patterns with ghost notes emerge.

**FLUX**: Injects variation into both lanes—fills at loop boundaries, velocity swells, ghost notes. At high values, patterns evolve and mutate.

**FUSE**: Tilts energy between lanes. CCW emphasizes Anchor (more kick, less shimmer). CW emphasizes Shimmer (sparse foundation, busy accents). At center, balanced.

#### Shift Layer (Button Held)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1+Shift** | **ANCHOR ACCENT** | Accent intensity for Anchor | Subtle ← → Hard |
| **K2+Shift** | **SHIMMER ACCENT** | Accent intensity for Shimmer | Subtle ← → Hard |
| **K3+Shift** | **ORBIT** | Voice relationship mode | See table |
| **K4+Shift** | **CONTOUR** | CV output shape | See table |

**ACCENT**: Controls the dynamic range and accent probability. Low = even dynamics. High = punchy accents with quiet ghost notes.

**ORBIT** (Voice Relationship):

| Range | Mode | Behavior |
|-------|------|----------|
| 0-33% | **Interlock** | Shimmer fills gaps in Anchor (call-response) |
| 33-67% | **Free** | Independent patterns, no collision logic |
| 67-100% | **Shadow** | Shimmer echoes Anchor with delay |

**CONTOUR** (CV Shape):

| Range | Mode | CV Behavior |
|-------|------|-------------|
| 0-25% | **Velocity** | CV = hit intensity (0-5V) |
| 25-50% | **Decay** | CV = envelope decay hint |
| 50-75% | **Pitch** | CV = pitch offset per hit |
| 75-100% | **Random** | CV = S&H random per trigger |

---

### Config Mode (Switch UP)

System configuration and structural parameters.

#### Primary Layer (No Shift)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1** | **TERRAIN** | Genre/style character | See table |
| **K2** | **LENGTH** | Loop length in bars | 1, 2, 4, 8, 16 |
| **K3** | **GRID** | Pattern family selection | 1-16 patterns |
| **K4** | **TEMPO** | Internal BPM | 90 ← → 160 |

**TERRAIN** (Genre Character):

| Range | Genre | Swing Base | Character |
|-------|-------|------------|-----------|
| 0-25% | **Techno** | 52-57% | Straight, driving, minimal swing |
| 25-50% | **Tribal** | 56-62% | Circular, percussive, moderate swing |
| 50-75% | **Trip-Hop** | 60-68% | Lazy, behind-beat, heavy swing |
| 75-100% | **IDM** | 54-65% + jitter | Broken, glitchy, micro-timing chaos |

TERRAIN affects:
- Base swing amount (genre-specific range)
- Accent placement patterns
- Ghost note probability curves
- Micro-timing jitter (especially for IDM)

#### Shift Layer (Button Held)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1+Shift** | **SWING TASTE** | Fine-tune swing within genre range | Low ← → High |
| **K2+Shift** | **GATE TIME** | Trigger duration | 5ms ← → 50ms |
| **K3+Shift** | **HUMANIZE** | Micro-timing jitter amount | None ← → Loose |
| **K4+Shift** | **CLOCK DIV** | Clock output divider/multiplier | See table |

**SWING TASTE**: Adjusts swing amount *within* the genre's opinionated range. You're not setting absolute swing—you're choosing your taste within what's appropriate for the genre. This keeps swing musically valid while allowing personal preference.

**HUMANIZE**: Adds micro-timing jitter to trigger timing. Creates a more human, less mechanical feel. At high values, triggers can drift ±10ms from the grid.

| Range | Jitter | Feel |
|-------|--------|------|
| 0% | ±0ms | Machine-tight |
| 50% | ±5ms | Subtle human feel |
| 100% | ±10ms | Loose, drummer-like |

**CLOCK DIV**:

| Range | Division |
|-------|----------|
| 0-20% | ÷4 |
| 20-40% | ÷2 |
| 40-60% | ×1 (unity) |
| 60-80% | ×2 |
| 80-100% | ×4 |

---

## CV-Driven Fills

Rather than a dedicated fill trigger, **fills emerge naturally from high CV values**. This is more modular and more expressive.

### How It Works

The FLUX parameter controls variation, ghost notes, and fill probability. When FLUX is high (via knob or CV), the pattern becomes busier with more fills. This means:

| CV 7 Level | FLUX Effect | Result |
|------------|-------------|--------|
| 0V | No modulation | Base FLUX from knob |
| 2.5V | Neutral | No change |
| 4-5V | +50-100% boost | Fill-like behavior |

### Practical Patching

| Source | Patch To | Effect |
|--------|----------|--------|
| **Pressure plate** | CV 7 (FLUX) | Press harder → more fills |
| **Envelope follower** | CV 7 (FLUX) | Audio peaks trigger variations |
| **Slow LFO** | CV 7 (FLUX) | Periodic intensity swells |
| **Random/S&H** | CV 7 (FLUX) | Occasional random fills |
| **Sequencer CV** | CV 7 (FLUX) | Programmed fill points |
| **Manual CV source** | CV 7 (FLUX) | Twist for instant fill |

### Why This Approach?

1. **More expressive**: Fills scale with CV level, not just on/off
2. **Dual purpose**: CV modulates the parameter AND creates fills
3. **Truly modular**: Any CV source becomes a fill controller
4. **No wasted inputs**: Gate In 2 stays dedicated to Reset

### The FLUX Sweet Spot

| FLUX Level | Behavior |
|------------|----------|
| 0-20% | Clean, minimal pattern |
| 20-50% | Some ghost notes, subtle variation |
| 50-70% | Active fills, velocity swells |
| 70-90% | Busy, lots of ghost notes and fills |
| 90-100% | Maximum chaos, fill on every opportunity |

Patching a slow-rising CV to FLUX creates a natural build: clean start → increasing tension → peak fill → reset.

---

## Swing System

### Genre-Aware Swing

Swing is **opinionated by genre** but adjustable within a curated range. This prevents musically inappropriate swing (e.g., 70% swing in minimal techno) while still allowing personal taste.

| Genre | Base Range | Low Taste | High Taste |
|-------|------------|-----------|------------|
| **Techno** | 52-57% | 52% (nearly straight) | 57% (subtle groove) |
| **Tribal** | 56-62% | 56% (mild shuffle) | 62% (pronounced swing) |
| **Trip-Hop** | 60-68% | 60% (lazy) | 68% (very drunk) |
| **IDM** | 54-65% | 54% (tight) | 65% (broken) + timing jitter |

### Swing Application

- Swing applies to 16th note off-beats (steps 1, 3, 5, 7, etc. in 0-indexed)
- **Anchor** can optionally have reduced swing (keeps kick grounded)
- **Shimmer** receives full swing amount
- **Clock Output** respects swing timing (swung clock for downstream modules)

### Implementation

```
// Swing is a percentage where 50% = straight
// Formula: delay = (swing - 0.5) * stepDurationSamples
// At 66% swing, off-beats are delayed by 33% of step duration
float swingDelay = (swingAmount - 0.5f) * stepDurationSamples;
if (isOffBeatStep) {
    triggerTime += swingDelay;
}
```

---

## Pattern Generation

### Hybrid Approach

DuoPulse uses a **hybrid** pattern system:
1. **Lookup tables** for core skeletons (derived from proven rhythmic patterns)
2. **Algorithmic layers** for ghosts, bursts, and variations

This provides the musicality of curated patterns with the flexibility of algorithmic generation.

### Pattern Structure

```cpp
struct PatternData {
    // 32-step patterns, 4 bits per step (0-15 intensity)
    uint16_t anchorSkeleton[2];   // Packed: 32 steps × 4 bits = 128 bits
    uint16_t shimmerSkeleton[2];
    uint8_t accentMask;           // Which steps can receive accents
    uint8_t relationship;         // Default interlock behavior
    uint8_t genreAffinity;        // Which genres this pattern suits
};
```

### Density Application

Density controls a **threshold** against pattern intensity:

```cpp
float threshold = (1.0f - density) * 15.0f;  // 0-15 scale
for (int step = 0; step < 32; step++) {
    uint8_t intensity = GetPatternIntensity(step);
    if (intensity > threshold) {
        trigger = true;
        velocity = (intensity - threshold) / (15.0f - threshold);
    }
}
```

At low density, only high-intensity steps fire. At max density, all steps including ghosts fire.

### FLUX Application

FLUX adds probabilistic variations:

```cpp
float fillChance = flux * 0.3f;        // Max 30% fill chance per step
float ghostChance = flux * 0.5f;       // Ghost notes
float velocityJitter = flux * 0.2f;    // Velocity variance
float timingJitter = flux * 0.01f;     // Micro-timing (IDM only, scaled by genre)
```

---

## Pattern Composition & Phrase Structure

### The Problem with Flat Patterns

A naive pattern generator treats every step equally. This works for 1-bar loops but creates monotonous, lifeless patterns at 4, 8, or 16 bars. Real drum programming has **phrase structure**—tension builds, fills appear at boundaries, syncopation increases before resolutions.

### Phrase Position Awareness

The sequencer tracks its position within the phrase to modulate pattern behavior:

```cpp
struct PhrasePosition {
    int currentBar;        // 0 to (loopLengthBars - 1)
    int stepInBar;         // 0 to 15
    int stepInPhrase;      // 0 to (loopLengthBars * 16 - 1)
    float phraseProgress;  // 0.0 to 1.0 (normalized position in loop)
    bool isLastBar;        // Approaching loop point
    bool isFillZone;       // Last 4 steps of phrase
    bool isDownbeat;       // Step 0 of any bar
};
```

### Phrase-Modulated Parameters

| Parameter | Phrase Effect | Rationale |
|-----------|---------------|-----------|
| **Fill Probability** | +30-50% in last bar | Natural fill placement |
| **Ghost Notes** | Increase toward phrase end | Building anticipation |
| **Syncopation** | More offbeats in bars 3-4 of 4-bar phrase | Tension before resolution |
| **Accent Intensity** | Strongest on bar 1, step 1 | Clear phrase downbeat |
| **Shimmer Density** | Can drift higher mid-phrase | Movement and evolution |
| **Velocity Variance** | Wider in fill zones | Expressive fills |

### Genre-Specific Phrase Behavior

#### Techno (0-25% Terrain)

```
Bar 1: Foundation - Strong downbeat, steady pattern
Bar 2: Hold - Maintain energy, minimal variation
Bar 3: Build - Slight increase in hat/shimmer activity
Bar 4: Tension - Fill zone in last 4 steps, more ghost kicks
→ Loop: Hard reset to foundation
```

**Techno Fill Zone**: Sparse but intentional. One extra kick or hat flurry. Never busy—just a nod to the coming downbeat.

#### Tribal (25-50% Terrain)

```
Bar 1: Anchor - Establish the clave/pattern
Bar 2: Develop - Percussion layers emerge
Bar 3: Peak - Maximum polyrhythmic density
Bar 4: Release & Fill - Opens up, then fills to reset
→ Loop: Circular feel, less hard reset
```

**Tribal Fill Zone**: Cascading toms, rolling congas. Fills are longer (8 steps) and more about momentum than punctuation.

#### Trip-Hop (50-75% Terrain)

```
Bar 1: Sparse, heavy - Single powerful kick
Bar 2: Ghost emerge - Quiet snare ghosts appear
Bar 3: Variation - Pattern shifts subtly
Bar 4: Drop & Drag - Sparser, behind-beat, anticipation
→ Loop: Lazy return, not a hard reset
```

**Trip-Hop Fill Zone**: Less "fill", more "dropout". Removing elements creates tension. Maybe one ghostly snare roll.

#### IDM (75-100% Terrain)

```
Bar 1: Establish (relative) - Some pattern emerges
Bar 2: Mutate - Parameters drift
Bar 3: Break - Unexpected gaps or bursts
Bar 4: Chaos → Reset - Maximum variation, then snap back
→ Loop: Can be jarring or smooth depending on Flux
```

**IDM Fill Zone**: Glitchy stutters, probability spikes, micro-timing extremes. The "fill" might be silence, or a ratchet burst.

### Implementation: Phrase Modulation

```cpp
float Sequencer::GetPhraseModulation(PhraseModParam param) const {
    float progress = phrasePosition_.phraseProgress;
    float lastBarBoost = phrasePosition_.isLastBar ? 1.0f : 0.0f;
    float fillZoneBoost = phrasePosition_.isFillZone ? 1.0f : 0.0f;
    
    switch (param) {
        case PhraseModParam::FILL_PROBABILITY:
            // Exponential rise in last bar, spike in fill zone
            return lastBarBoost * 0.3f + fillZoneBoost * 0.5f;
            
        case PhraseModParam::GHOST_PROBABILITY:
            // Gradual rise through phrase
            return progress * 0.3f;
            
        case PhraseModParam::SYNCOPATION:
            // Rises in second half of phrase
            return (progress > 0.5f) ? (progress - 0.5f) * 0.4f : 0.0f;
            
        case PhraseModParam::ACCENT_INTENSITY:
            // Highest at phrase start, dips mid-phrase, rises for fill
            if (phrasePosition_.stepInPhrase == 0) return 0.3f;
            if (phrasePosition_.isFillZone) return 0.2f;
            return 0.0f;
            
        case PhraseModParam::VELOCITY_VARIANCE:
            // Wider variance in fill zones
            return fillZoneBoost * 0.2f;
            
        default:
            return 0.0f;
    }
}

// Apply phrase modulation to pattern generation
void Sequencer::GenerateStep() {
    // Base parameters from controls
    float effectiveFlux = flux_;
    float effectiveFillProb = fillProbability_;
    float effectiveGhostProb = ghostProbability_;
    
    // Add phrase modulation
    effectiveFlux += GetPhraseModulation(PhraseModParam::GHOST_PROBABILITY);
    effectiveFillProb += GetPhraseModulation(PhraseModParam::FILL_PROBABILITY);
    
    // Genre scaling (tribal gets more, techno gets less)
    float genreScale = GetGenrePhraseScale();
    effectiveFlux *= (1.0f + genreScale * 0.3f);
    
    // Generate pattern with modulated parameters...
}
```

### Phrase-Aware Fill Trigger

When a fill is triggered (via button or Gate In 2), the fill behavior also respects phrase position:

```cpp
void Sequencer::TriggerFill() {
    fillActive_ = true;
    
    // Fill duration depends on where we are in the phrase
    if (phrasePosition_.isFillZone) {
        // Already in fill zone: short burst (2-4 steps)
        fillDurationSteps_ = 2 + (rand() % 3);
    } else if (phrasePosition_.isLastBar) {
        // In last bar: fill to phrase end
        fillDurationSteps_ = 16 - phrasePosition_.stepInBar;
    } else {
        // Mid-phrase: fill to next bar boundary
        fillDurationSteps_ = 16 - phrasePosition_.stepInBar;
    }
    
    fillEndStep_ = (stepIndex_ + fillDurationSteps_) % totalSteps_;
}
```

### Visual Representation: 4-Bar Phrase

```
                    PHRASE STRUCTURE (4 bars)
    
    ┌─── Bar 1 ───┬─── Bar 2 ───┬─── Bar 3 ───┬─── Bar 4 ───┐
    │             │             │             │    FILL     │
    │  ANCHOR     │   STEADY    │   BUILD     │    ZONE     │
    │  DOWNBEAT   │             │             │             │
    └─────────────┴─────────────┴─────────────┴─────────────┘
    
    Fill Prob:    ░░░░░░░░░░░░░░░░░░░░░░░░░░▓▓▓▓▓▓▓▓████████
    Ghost Notes:  ░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓██████████████████
    Syncopation:  ░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    Accent:       ████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▓▓▓▓▓▓▓▓
    
    ░ = minimal  ▒ = low  ▓ = medium  █ = high
```

### Pattern Length Scaling

Phrase modulation scales with pattern length:

| Length | Fill Zone | Build Zone | Behavior |
|--------|-----------|------------|----------|
| 1 bar | Steps 12-15 | Steps 8-15 | Compact, every bar has mini-arc |
| 2 bars | Last 4 steps | Last 8 steps | Quick build and release |
| 4 bars | Last 8 steps | Last 16 steps | Standard phrase structure |
| 8 bars | Last 16 steps | Last 32 steps | Long-form tension building |
| 16 bars | Last 32 steps | Last 64 steps | Epic builds, DJ-friendly |

```cpp
int Sequencer::GetFillZoneLength() const {
    switch (loopLengthBars_) {
        case 1:  return 4;   // Last quarter of bar
        case 2:  return 4;   // Last quarter of last bar
        case 4:  return 8;   // Last half of last bar
        case 8:  return 16;  // Last bar
        case 16: return 32;  // Last 2 bars
        default: return 4;
    }
}

int Sequencer::GetBuildZoneStart() const {
    int totalSteps = loopLengthBars_ * 16;
    switch (loopLengthBars_) {
        case 1:  return 8;   // Second half
        case 2:  return 16;  // Second bar
        case 4:  return 32;  // Third bar onward
        case 8:  return 80;  // Bar 6 onward
        case 16: return 192; // Bar 13 onward
        default: return totalSteps / 2;
    }
}
```

---

## CV Output Expression

### Anchor CV (Audio Out 1)

Default: **Velocity**
- 0-5V proportional to hit intensity
- Accent hits output higher voltage
- Ghost notes output lower voltage

### Shimmer CV (Audio Out 2)

Default: **Velocity**
- Same as Anchor, independent scaling
- Can be set to different CONTOUR mode than Anchor

### Mode Behaviors

| Mode | Behavior | Use Case |
|------|----------|----------|
| **Velocity** | CV = hit intensity | VCA/filter control |
| **Decay** | CV = decay time hint | Envelope modulation |
| **Pitch** | CV = note offset | Melodic percussion |
| **Random** | CV = S&H on trigger | Chaos/modulation |

---

## Visual Feedback

### LED (CV Out 2) Behavior

| State | LED Behavior |
|-------|--------------|
| **Performance Mode** | Pulses on Anchor triggers |
| **Config Mode** | Solid ON |
| **Shift Held** | Double-brightness |
| **Knob Interaction** | Brightness = parameter value (1s timeout) |
| **High FLUX** | Faster pulse rate (indicates active variation) |

### Trig Indication

- Gate Out 1/2 LEDs (if present) indicate active triggers
- CV Out 1 (clock) provides visual clock reference

---

## Soft Takeover (Soft Pickup)

### Behavior

All knobs use **gradual interpolation** toward the physical position:

```cpp
// Instead of jumping to knob position, interpolate slowly
float currentValue = savedParameter;
float knobPosition = readKnob();

if (!hasCaughtUp) {
    // Check if we've crossed the saved value
    if (crossedSavedValue(knobPosition)) {
        hasCaughtUp = true;
    }
} else {
    // Smooth interpolation toward knob position
    float delta = knobPosition - currentValue;
    currentValue += delta * 0.1f;  // 10% per control cycle
}
```

This prevents jumps when:
- Switching between Performance/Config modes
- Switching between Shift layers
- On startup

---

## CV Input Behavior

### Always Performance-Mapped

**Critical**: CV inputs 1-4 **always** modulate the four primary Performance Mode parameters:

| CV Input | Always Modulates |
|----------|------------------|
| **CV 5** | ANCHOR DENSITY |
| **CV 6** | SHIMMER DENSITY |
| **CV 7** | FLUX |
| **CV 8** | FUSE |

This means:
- In **Config Mode**, CV still affects performance parameters
- External modulation is always immediate and predictable
- No confusion about what CV is controlling

### Additive Modulation

```cpp
float finalValue = knobValue + (cvValue - 0.5f);  // CV centered at 2.5V
finalValue = clamp(finalValue, 0.0f, 1.0f);
```

---

## Parameter Quick Reference

### Performance Mode (Switch DOWN)

|    | Primary | +Shift |
|----|---------|--------|
| K1 | ANCHOR DENSITY | ANCHOR ACCENT |
| K2 | SHIMMER DENSITY | SHIMMER ACCENT |
| K3 | FLUX | ORBIT |
| K4 | FUSE | CONTOUR |

### Config Mode (Switch UP)

|    | Primary | +Shift |
|----|---------|--------|
| K1 | TERRAIN | SWING TASTE |
| K2 | LENGTH | GATE TIME |
| K3 | GRID | HUMANIZE |
| K4 | TEMPO | CLOCK DIV |

---

## Technical Specifications

| Parameter | Value |
|-----------|-------|
| Sample Rate | 48kHz |
| Audio Block Size | 4 samples |
| Pattern Resolution | 32 steps |
| Swing Range | 50-68% |
| Tempo Range | 90-160 BPM |
| Gate Time Range | 5-50ms |
| CV Range | 0-5V |
| Clock Output | Swung timing |

---

## Parameter Change Behavior

### Immediate Application

**All parameter changes apply immediately**—there is no waiting for the next bar, pattern, or loop boundary.

| Parameter Type | Behavior |
|----------------|----------|
| **Density** | Next step uses new density |
| **Flux** | Variation changes immediately |
| **Fuse** | Energy balance shifts immediately |
| **Swing** | Applied to next off-beat step |
| **Terrain** | Genre character + swing range updates immediately |
| **Length** | Loop length changes immediately (see note) |
| **Pattern/Grid** | New pattern active on next step |
| **Accent** | Applied to next triggered hit |
| **Orbit** | Voice relationship changes on next step |
| **Contour** | CV mode switches immediately |
| **Gate Time** | Applied to next trigger |
| **Tempo** | BPM changes immediately (smooth) |

### Pattern Length Changes

When LENGTH changes mid-pattern:
- If current step > new loop length: immediately wrap to step 0
- If current step < new loop length: continue normally, wrap at new boundary
- No waiting for current pattern to complete

```cpp
// Example: Loop length change behavior
void Sequencer::SetLength(int bars) {
    loopLengthBars_ = bars;
    int maxStep = bars * 16;  // 16 steps per bar
    
    // Immediately wrap if past new boundary
    if (stepIndex_ >= maxStep) {
        stepIndex_ = stepIndex_ % maxStep;
    }
}
```

### Rationale

Immediate parameter changes provide:
1. **Responsive feel**: Knob turns have instant effect
2. **Performance flexibility**: No waiting for musical moments
3. **Modular compatibility**: CV modulation is sample-accurate
4. **Intuitive behavior**: What you dial is what you get

---

## Summary of Key Innovations

1. **Abstract Terminology**: Anchor/Shimmer instead of Kick/Snare—encourages modular thinking
2. **Genre-Aware Swing**: Opinionated ranges per genre, taste control for fine-tuning
3. **CV Always Performance**: CV inputs always modulate performance params, regardless of mode
4. **CV-Driven Fills**: High FLUX via CV naturally creates fill behavior—truly modular
5. **Swung Clock Output**: Clock respects swing for downstream module sync
6. **Hybrid Pattern Engine**: Curated skeletons + algorithmic variation
7. **Phrase-Aware Composition**: Longer patterns have musical arc with builds and fills
8. **Gradual Soft Pickup**: Interpolated takeover instead of catch-up
9. **ORBIT Relationship Modes**: Interlock/Free/Shadow for voice interaction
10. **CONTOUR CV Modes**: Velocity/Decay/Pitch/Random expression
11. **TERRAIN Genre Presets**: Techno/Tribal/Trip-Hop/IDM character tuning
12. **HUMANIZE Jitter**: Micro-timing variation for organic feel

