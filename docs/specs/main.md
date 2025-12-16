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

## Feature: Control System [duopulse-controls]

### Quick Reference

|    | **Performance Primary** | **Performance +Shift** | **Config Primary** | **Config +Shift** |
|----|-------------------------|------------------------|--------------------|--------------------|
| **K1** | Anchor Density | Terrain | Anchor Accent | Swing Taste |
| **K2** | Shimmer Density | Length | Shimmer Accent | Gate Time |
| **K3** | Flux | Grid | Contour | Humanize |
| **K4** | Fuse | Orbit | Tempo | Clock Div |

> **CV inputs 5-8 always modulate Performance Primary parameters** (Anchor Density, Shimmer Density, Flux, Fuse) regardless of current mode.

---

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
| **K1+Shift** | **TERRAIN** | Genre/style character | See table |
| **K2+Shift** | **LENGTH** | Loop length in bars | 1, 2, 4, 8, 16 |
| **K3+Shift** | **GRID** | Pattern family selection | 1-16 patterns |
| **K4+Shift** | **ORBIT** | Voice relationship mode | See table |

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

**ORBIT** (Voice Relationship):

| Range | Mode | Behavior |
|-------|------|----------|
| 0-33% | **Interlock** | Shimmer fills gaps in Anchor (call-response) |
| 33-67% | **Free** | Independent patterns, no collision logic |
| 67-100% | **Shadow** | Shimmer echoes Anchor with delay |

---

### Config Mode (Switch UP)

System configuration and expression parameters.

#### Primary Layer (No Shift)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1** | **ANCHOR ACCENT** | Accent intensity for Anchor | Subtle ← → Hard |
| **K2** | **SHIMMER ACCENT** | Accent intensity for Shimmer | Subtle ← → Hard |
| **K3** | **CONTOUR** | CV output shape | See table |
| **K4** | **TEMPO** | Internal BPM | 90 ← → 160 |

**ACCENT**: Controls the dynamic range and accent probability. Low = even dynamics. High = punchy accents with quiet ghost notes.

**CONTOUR** (CV Shape):

| Range | Mode | CV Behavior |
|-------|------|-------------|
| 0-25% | **Velocity** | CV = hit intensity (0-5V) |
| 25-50% | **Decay** | CV = envelope decay hint |
| 50-75% | **Pitch** | CV = pitch offset per hit |
| 75-100% | **Random** | CV = S&H random per trigger |

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

## Feature: CV-Driven Fills [duopulse-fills]

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

### The FLUX Sweet Spot

| FLUX Level | Behavior |
|------------|----------|
| 0-20% | Clean, minimal pattern |
| 20-50% | Some ghost notes, subtle variation |
| 50-70% | Active fills, velocity swells |
| 70-90% | Busy, lots of ghost notes and fills |
| 90-100% | Maximum chaos, fill on every opportunity |

---

## Feature: Genre-Aware Swing [duopulse-swing]

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

### Acceptance Criteria
- [x] Swing percentage calculated from terrain (genre) + swingTaste parameters
- [x] Off-beat steps delayed according to swing formula
- [x] Clock output applies swing timing
- [ ] Anchor receives 70% of swing amount, Shimmer receives 100% *(TODO: differential swing not yet implemented)*

---

## Feature: Pattern Generation [duopulse-patterns]

### Hybrid Approach

DuoPulse uses a **hybrid** pattern system:
1. **Lookup tables** for core skeletons (derived from proven rhythmic patterns)
2. **Algorithmic layers** for ghosts, bursts, and variations

This provides the musicality of curated patterns with the flexibility of algorithmic generation.

### Pattern Library

The sequencer includes 16 curated skeleton patterns organized by genre affinity:

| Index | Pattern Name | Genre | Relationship | Description |
|-------|--------------|-------|--------------|-------------|
| **0** | Techno Four-on-Floor | Techno | Free | Classic techno kick pattern with straight hi-hats. Anchor: Strong kicks on quarter notes (0, 4, 8, 12, 16, 20, 24, 28). Shimmer: 8th note hats, accent on off-beats. |
| **1** | Techno Minimal | Techno | Free | Sparse, hypnotic pattern. Anchor: Kick on 1 and occasional ghost. Shimmer: Minimal hats, emphasis on space. |
| **2** | Techno Driving | Techno | Free | Relentless energy, 16th note hats. Anchor: Solid four-on-floor with some ghost notes. Shimmer: Constant 16ths with varying intensity. |
| **3** | Techno Pounding | Techno | Interlock | Heavy, industrial feel. Anchor: Double kicks and syncopation. Shimmer: Industrial clang accents. |
| **4** | Tribal Clave | Tribal | Interlock | Based on son clave rhythm. Anchor: 3-2 clave feel. Shimmer: Fills between clave hits. |
| **5** | Tribal Interlocking | Tribal | Interlock | Anchor and shimmer designed to perfectly interlock. Creates polyrhythmic texture. |
| **6** | Tribal Polyrhythmic | Tribal | Free | 3-against-4 polyrhythm feel. Anchor: 4-beat pattern. Shimmer: 3-beat pattern (every ~10.67 steps, approximated). |
| **7** | Tribal Circular | Tribal/Techno | Interlock | Hypnotic, circular pattern for extended grooves. Anchor: Rotating emphasis. Shimmer: Counter-rhythm. |
| **8** | Trip-Hop Sparse | Trip-Hop | Free | Minimal, spacious pattern. Heavy kick, sparse snare. Anchor: Heavy, sparse kicks. Shimmer: Very sparse snare. |
| **9** | Trip-Hop Lazy | Trip-Hop | Shadow | Behind-the-beat feel, ghost notes. Anchor: Lazy kick with ghost notes. Shimmer: Snare ghosts building to hit. |
| **10** | Trip-Hop Heavy | Trip-Hop | Free | Massive sound, emphasis on weight. Anchor: Crushing kicks. Shimmer: Heavy snare with drag. |
| **11** | Trip-Hop Groove | Trip-Hop/Tribal | Free | More active hip-hop influenced pattern. Anchor: Syncopated kick. Shimmer: Offbeat snares. |
| **12** | IDM Broken | IDM | Free | Fragmented, glitchy pattern. Anchor: Fragmented kicks. Shimmer: Irregular snare/hat bursts. |
| **13** | IDM Glitch | IDM | Free | Micro-edits, stutters. Anchor: Stutter kicks. Shimmer: Glitchy fills. |
| **14** | IDM Irregular | IDM | Free | Unpredictable placement. Anchor: Seemingly random but designed. Shimmer: Counter-irregular. |
| **15** | IDM Chaos | IDM | Free | Maximum complexity. Anchor: Dense, chaotic. Shimmer: Equally chaotic. |

**Pattern Organization:**
- **0-3**: Techno (four-on-floor, minimal, driving, pounding)
- **4-7**: Tribal (clave, interlocking, polyrhythmic, circular)
- **8-11**: Trip-Hop (sparse, lazy, heavy, behind-beat)
- **12-15**: IDM (broken, glitch, irregular, chaos)

**Intensity Values (0-15):**
- **0** = Step off (never fires)
- **1-4** = Ghost note (fires at high density only)
- **5-10** = Normal hit (fires at medium density)
- **11-15** = Strong hit (fires at low density, accent candidate)

**Relationship Modes:**
- **Free**: Independent patterns, no collision logic
- **Interlock**: Shimmer fills gaps in Anchor (call-response)
- **Shadow**: Shimmer echoes Anchor with delay

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

At low density, only high-intensity steps fire. At max density, all steps including ghosts fire.

### FLUX Application

FLUX adds probabilistic variations:
- Fill chance: up to 30% per step at max
- Ghost notes: up to 50% per step at max
- Velocity jitter: up to 20% variance
- Timing jitter: up to 1% (IDM only, scaled by genre)

**FLUX Expected Behavior** [control-layout-fixes]:
- At 0%: Clean skeleton pattern, no variation
- At 25%: Occasional ghost notes begin appearing
- At 50%: Noticeable variation, some fills at phrase boundaries
- At 75%: Busy pattern with frequent ghost notes and fills
- At 100%: Maximum chaos—ghost notes, fills, velocity swells throughout

### FUSE Application

FUSE tilts the energy balance between voices:
- At 0% (CCW): Anchor density boosted +15%, Shimmer reduced -15%
- At 50% (Center): Balanced, no bias
- At 100% (CW): Shimmer density boosted +15%, Anchor reduced -15%

**FUSE Expected Behavior** [control-layout-fixes]:
- Turning FUSE CCW should audibly increase kick/anchor density
- Turning FUSE CW should audibly increase snare/shimmer density
- The effect should be noticeable across the full range

### Acceptance Criteria
- [x] 16 skeleton patterns optimized for 2-voice output
- [x] Density threshold controls which pattern steps fire
- [x] FLUX adds probabilistic ghost notes, fills, and velocity variance
- [x] Patterns have genre affinity weighting
- [x] **VERIFY**: FLUX produces audible variation across full range *(Code verified 2025-12-03 - needs hardware tuning)*
- [x] **VERIFY**: FUSE produces audible energy tilt across full range *(Code verified 2025-12-03 - needs hardware tuning)*

---

## Feature: Phrase Structure [duopulse-phrase]

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
- Foundation - Strong downbeat, steady pattern
- Hold - Maintain energy, minimal variation
- Build - Slight increase in hat/shimmer activity
- Tension - Fill zone in last 4 steps, more ghost kicks
- Loop: Hard reset to foundation

#### Tribal (25-50% Terrain)
- Anchor - Establish the clave/pattern
- Develop - Percussion layers emerge
- Peak - Maximum polyrhythmic density
- Release & Fill - Opens up, then fills to reset
- Loop: Circular feel, less hard reset

#### Trip-Hop (50-75% Terrain)
- Sparse, heavy - Single powerful kick
- Ghost emerge - Quiet snare ghosts appear
- Variation - Pattern shifts subtly
- Drop & Drag - Sparser, behind-beat, anticipation
- Loop: Lazy return, not a hard reset

#### IDM (75-100% Terrain)
- Establish (relative) - Some pattern emerges
- Mutate - Parameters drift
- Break - Unexpected gaps or bursts
- Chaos → Reset - Maximum variation, then snap back
- Loop: Can be jarring or smooth depending on Flux

### Pattern Length Scaling

| Length | Fill Zone | Build Zone | Behavior |
|--------|-----------|------------|----------|
| 1 bar | Steps 12-15 | Steps 8-15 | Compact, every bar has mini-arc |
| 2 bars | Last 4 steps | Last 8 steps | Quick build and release |
| 4 bars | Last 8 steps | Last 16 steps | Standard phrase structure |
| 8 bars | Last 16 steps | Last 32 steps | Long-form tension building |
| 16 bars | Last 32 steps | Last 64 steps | Epic builds, DJ-friendly |

### Acceptance Criteria
- [x] PhrasePosition struct tracking current position in phrase
- [x] Fill/build zone lengths scale with pattern length
- [x] Phrase modulation applies genre-specific scaling
- [x] Fill probability, ghost notes, syncopation modulated by phrase position

---

## Feature: Orbit Voice Relationships [duopulse-orbit]

### Interlock Mode (0-33%)
Shimmer fills gaps in Anchor (call-response). When Anchor fires, Shimmer probability reduced. When Anchor silent, Shimmer probability boosted.

### Free Mode (33-67%)
Independent patterns, no collision logic. Both voices generate independently. Can create polyrhythmic feel.

### Shadow Mode (67-100%)
Shimmer echoes Anchor with 1-step delay. Creates doubling/echo effect. Shadow velocity reduced to 70%.

### Acceptance Criteria
- [x] Orbit parameter (0-1) selects mode
- [x] Interlock: reduces shimmer when anchor fires, boosts when silent
- [x] Free: no voice interaction
- [x] Shadow: shimmer copies previous anchor trigger at reduced velocity

---

## Feature: Contour CV Modes [duopulse-contour]

### Velocity Mode (0-25%)
CV = hit intensity (0-5V). Accent = high voltage. Ghost = low voltage. Slight hold/decay between triggers.

### Decay Mode (25-50%)
CV hints decay time to downstream. Accent = long decay. Ghost = short decay. CV decays over time.

### Pitch Mode (50-75%)
CV = pitch offset per hit. Random within scaled range. For melodic percussion.

### Random Mode (75-100%)
Sample & Hold random voltage. New value each trigger. For modulation/chaos.

### Acceptance Criteria
- [x] Contour parameter (0-1) selects mode
- [x] Each mode generates appropriate CV behavior
- [x] Both Anchor and Shimmer CV outputs respect contour mode

---

## Feature: Humanize Timing [duopulse-humanize]

Adds micro-timing jitter to trigger timing for organic feel.

| Range | Jitter | Feel |
|-------|--------|------|
| 0% | ±0ms | Machine-tight |
| 50% | ±5ms | Subtle human feel |
| 100% | ±10ms | Loose, drummer-like |

IDM terrain (75-100%) automatically adds extra humanize on top of the knob setting.

### Acceptance Criteria
- [x] Humanize parameter (0-1) controls jitter amount
- [x] Max jitter = ±10ms (±480 samples at 48kHz)
- [x] IDM terrain adds up to 30% extra humanize
- [x] Jitter applied per-trigger, random within range

---

## Feature: Soft Takeover [duopulse-soft-pickup]

All knobs use **gradual interpolation** toward the physical position.

### Behavior
1. On mode switch or startup, knob value is "detached" from physical position
2. As user moves knob, system interpolates toward physical position
3. When knob crosses the stored value, full control is restored
4. Smooth 10% per control cycle interpolation prevents jumps

### Mode Switching Behavior [control-layout-fixes]

**Critical**: When switching between Performance and Config modes, parameters must persist correctly:

1. **Parameter Persistence**: Each mode/shift combination has independent parameter storage. Switching modes does NOT alter stored values.
2. **Soft Pickup on Mode Switch**: When returning to a mode, the knob physical position may differ from the stored value. Soft pickup engages—parameter only moves when knob crosses the stored value or after gradual interpolation.
3. **No Jumps**: Mode switching should never cause audible parameter jumps.

### Acceptance Criteria
- [x] SoftKnob class with gradual interpolation
- [x] Cross-detection for immediate catchup
- [x] 16 parameter slots (4 knobs × 4 mode/shift combinations)
- [x] No parameter jumps on mode switch
- [x] **BUG FIX**: Mode switching must not alter stored parameter values *(Fixed 2025-12-03: interpolation only when knob moved)*
- [x] **BUG FIX**: Returning to a mode must restore soft pickup state correctly *(Fixed 2025-12-03)*

---

## Feature: CV Input Behavior [duopulse-cv]

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

CV uses additive modulation: CV value adds directly to knob position, clamped 0-1. When CV input is unpatched (reads 0V), no modulation occurs—knob value is used directly. Patching a CV source adds to the knob position.

> **Implementation Note**: Earlier design specified bipolar modulation centered at 2.5V. Changed to additive (0V = neutral) because unpatched CV jacks on Patch.SM hardware read 0V, not 2.5V. This ensures predictable behavior when no CV is patched.

### Acceptance Criteria
- [x] CV always modulates performance parameters regardless of mode
- [x] Additive modulation (0V = no effect, positive CV adds to knob)
- [x] Clamping to 0-1 range
- [x] **BUG FIX**: Unpatched CV inputs don't affect parameter values *(Fixed 2025-12-03: use additive MixControl)*

---

## Feature: LED Visual Feedback [duopulse-led]

### LED (CV Out 2) Behavior

| State | LED Behavior |
|-------|--------------|
| **Performance Mode** | Pulses on Anchor triggers |
| **Config Mode** | Solid ON |
| **Shift Held** | Double-brightness |
| **Knob Interaction** | Brightness = parameter value (1s timeout) |
| **High FLUX** | Faster pulse rate (indicates active variation) |
| **Fill Active** | Rapid flash (50ms rate) |

### Acceptance Criteria
- [x] LED pulses on anchor trigger in performance mode
- [x] LED solid in config mode
- [x] Shift held increases brightness
- [x] Knob turn shows parameter value for 1 second
- [x] Fill active triggers rapid flash

---

## Feature: Parameter Change Behavior [duopulse-immediate]

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

### Acceptance Criteria
- [x] All setters apply immediately, no queuing
- [ ] Length change wraps step position if necessary *(TODO: SetLength doesn't wrap stepIndex when length decreases)*

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

## Parameter Quick Reference

### Performance Mode (Switch DOWN)

|    | Primary | +Shift |
|----|---------|--------|
| K1 | ANCHOR DENSITY | TERRAIN |
| K2 | SHIMMER DENSITY | LENGTH |
| K3 | FLUX | GRID |
| K4 | FUSE | ORBIT |

### Config Mode (Switch UP)

|    | Primary | +Shift |
|----|---------|--------|
| K1 | ANCHOR ACCENT | SWING TASTE |
| K2 | SHIMMER ACCENT | GATE TIME |
| K3 | CONTOUR | HUMANIZE |
| K4 | TEMPO | CLOCK DIV |

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
