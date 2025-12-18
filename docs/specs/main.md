# DuoPulse: 2-Voice Percussive Sequencer

**Specification v3.0**  
**Algorithmic Pulse Field**  
**Date: 2025-12-16**

---

## Executive Summary

**DuoPulse** is an opinionated 2-voice percussion sequencer optimized for the Patch.Init hardware. Version 3 introduces a **continuous algorithmic pulse field** that replaces the discrete pattern lookup system, providing infinite variation along a musically coherent gradient from club-ready techno to experimental IDM.

The core mental model is simplified to two axes:
1. **BROKEN** — How regular/irregular the pattern is (4/4 techno → IDM chaos)
2. **DRIFT** — How much the pattern evolves over time (locked → generative)

Genre character (Techno/Tribal/Trip-Hop/IDM) emerges naturally from the BROKEN parameter. When using external clock, CV Out 1 becomes a multi-purpose auxiliary output.

---

## Design Philosophy

### Core Principles

1. **Embrace the Constraint**: Two voices, maximally expressive. No compromise mapping of 3 voices to 2 outputs.
2. **Abstract Vocabulary**: "Anchor" and "Shimmer" replace traditional drum names, encouraging modular synthesis thinking.
3. **Predictable Axes**: Every parameter change produces a predictable, musical result. Two clear axes: weird (BROKEN) × stable (DRIFT).
4. **Performance-First**: Core controls are immediate and tactile; deeper configuration is accessible but never in the way.
5. **CV is King**: CV inputs always map to the four main performance parameters, regardless of mode.

### The Problem Solved by v3

The v2 system had 16 hand-crafted pattern skeletons with TERRAIN (genre) and GRID (pattern selection) controls. This caused:
- Terrain and Grid mismatches (Tribal pattern with Techno swing)
- Unpredictable parameter combinations
- Abrupt pattern transitions

The v3 algorithmic approach replaces discrete patterns with a continuous algorithm where:
- Genre character **emerges** from the BROKEN parameter
- Every knob turn produces a **predictable, audible change**
- Patterns can be **locked** (DJ tool) or **generative** (experimental)

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
| **CV Out 1** | Auxiliary Output (mode-selectable, see [aux-output-modes]) |
| **CV Out 2** | LED Feedback (mode/trig/parameter brightness) |

---

## Feature: Control System [duopulse-controls]

### Quick Reference

|    | **Performance Primary** | **Performance +Shift** | **Config Primary** | **Config +Shift** |
|----|-------------------------|------------------------|--------------------|--------------------|
| **K1** | Anchor Density | Fuse | Anchor Accent | Swing Taste |
| **K2** | Shimmer Density | Length | Shimmer Accent | Gate Time |
| **K3** | Broken | Couple | Contour | Humanize |
| **K4** | Drift | Ratchet | Tempo / Clock Div* | Clock Div / Aux Mode* |

> **CV inputs 5-8 always modulate Performance Primary parameters** (Anchor Density, Shimmer Density, Broken, Drift) regardless of current mode.

> **v3 Control Changes**: FLUX→BROKEN (K3), FUSE→DRIFT (K4). TERRAIN/GRID removed (genre emerges from BROKEN). ORBIT→COUPLE simplified. See `docs/specs/double-down/simplified-algorithmic-approach.md` for full v3 algorithm spec.

> *\*K4 is context-aware:* When **internal clock** (no external clock patched): K4 = Tempo, K4+Shift = Clock Div. When **external clock** patched: K4 = Clock Div (sequencer rate), K4+Shift = Aux Output Mode. See [external-clock-behavior].

---

### Performance Mode (Switch DOWN)

Primary playing mode. Optimized for live manipulation.

#### Primary Layer (No Shift)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1** | **ANCHOR DENSITY** | How many Anchor triggers fire | 0%=silent → 100%=full |
| **K2** | **SHIMMER DENSITY** | How many Shimmer triggers fire | 0%=silent → 100%=full |
| **K3** | **BROKEN** | Pattern regularity (genre emerges) | 0%=4/4 → 100%=IDM |
| **K4** | **DRIFT** | Pattern stability over time | 0%=locked → 100%=evolving |

**ANCHOR/SHIMMER DENSITY**: Controls how many triggers fire. Low density fires only high-weight positions (downbeats). High density fires nearly all steps.

**BROKEN**: Controls how regular/irregular the pattern is. Genre character emerges naturally: 0-25% Techno (straight), 25-50% Tribal (shuffled), 50-75% Trip-Hop (lazy), 75-100% IDM (broken). BROKEN also affects swing, micro-timing jitter, step displacement, and velocity variation.

**DRIFT**: Controls how much the pattern evolves over time. At 0%, pattern is fully locked (same every loop). At 100%, pattern is fully generative (unique each loop). Per-voice behavior: Anchor is more stable (0.7× multiplier), Shimmer is more drifty (1.3× multiplier).

#### Shift Layer (Button Held)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1+Shift** | **FUSE** | Voice energy balance | CCW=kick-heavy, CW=snare-heavy |
| **K2+Shift** | **LENGTH** | Loop length in bars | 1, 2, 4, 8, 16 |
| **K3+Shift** | **COUPLE** | Voice relationship strength | 0%=independent, 100%=interlock |
| **K4+Shift** | **RATCHET** | Fill intensity during phrase transitions | 0%=subtle, 100%=intense |

**FUSE**: Tilts energy between voices. CCW boosts Anchor density (+15%), reduces Shimmer (-15%). CW does the opposite. At center (50%), balanced.

**COUPLE**: Controls voice interlock strength (simplified from v2's 3-mode ORBIT):
- 0%: Fully independent — voices can collide or gap freely
- 50%: Soft interlock — slight collision avoidance
- 100%: Hard interlock — Shimmer fills Anchor gaps, suppresses collisions

**RATCHET**: Controls fill intensity during phrase transitions. Works with DRIFT:
- DRIFT controls fill PROBABILITY (when fills occur)
- RATCHET controls fill INTENSITY (how intense fills are)
- At DRIFT=0, RATCHET has no effect (no fills occur)
- At RATCHET > 50%, ratcheting (32nd note subdivisions) can occur

---

### Config Mode (Switch UP)

System configuration and expression parameters.

#### Primary Layer (No Shift)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1** | **ANCHOR ACCENT** | Accent intensity for Anchor | Subtle ← → Hard |
| **K2** | **SHIMMER ACCENT** | Accent intensity for Shimmer | Subtle ← → Hard |
| **K3** | **CONTOUR** | CV output shape | See table |
| **K4** | **TEMPO / CLOCK DIV** | Context-aware (see below) | See table |

**K4 Context-Aware Behavior** (see also [external-clock-behavior]):
- **Internal clock** (no external clock patched): TEMPO — sets internal BPM (90-160)
- **External clock** patched: CLOCK DIV — divides/multiplies external clock for sequencer step rate

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
| **K4+Shift** | **CLOCK DIV / AUX MODE** | Context-aware (see below) | See table |

**SWING TASTE**: Adjusts swing amount *within* the genre's opinionated range. You're not setting absolute swing—you're choosing your taste within what's appropriate for the genre. This keeps swing musically valid while allowing personal preference.

**HUMANIZE**: Adds micro-timing jitter to trigger timing. Creates a more human, less mechanical feel. At high values, triggers can drift ±10ms from the grid.

| Range | Jitter | Feel |
|-------|--------|------|
| 0% | ±0ms | Machine-tight |
| 50% | ±5ms | Subtle human feel |
| 100% | ±10ms | Loose, drummer-like |

**K4+Shift Context-Aware Behavior** (see also [external-clock-behavior], [aux-output-modes]):
- **Internal clock**: CLOCK DIV — divides/multiplies the clock *output* (CV Out 1)
- **External clock** patched: AUX MODE — selects what CV Out 1 outputs (see [aux-output-modes])

**CLOCK DIV** (applies to clock output when internal, or sequencer rate when external):

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

## Feature: Weighted Pulse Field Algorithm [pulse-field]

> **v3 Feature**: This replaces the discrete pattern lookup system (see v2 Pattern Generation below, kept for reference).

### Critical Design Rules [v3-critical-rules]

These rules are **absolute** and must be enforced at the algorithm level:

1. **DENSITY=0 is ABSOLUTE SILENCE.** No triggers fire, regardless of BROKEN, DRIFT, phrase position, fills, or any other modulation. This check must come FIRST in `ShouldStepFire()`.

2. **DRIFT=0 is ZERO VARIATION.** The pattern is 100% identical every loop. No beat variation whatsoever. All steps must use `patternSeed_` (locked seed).

3. **The Reference Point:** At BROKEN=0, DRIFT=0, DENSITIES at 50%: **classic 4/4 kick with snare on backbeat, repeated identically forever.**

### Concept

Each of the 32 steps in a pattern has a **weight** that represents its "likelihood" of triggering. The algorithm uses these weights combined with DENSITY and BROKEN to determine what fires.

### Grid Position Weights

| Step Positions | Weight | Musical Role |
|----------------|--------|--------------|
| 0, 16 | 1.0 | Bar downbeats ("THE ONE") |
| 8, 24 | 0.85 | Half-note positions |
| 4, 12, 20, 28 | 0.7 | Quarter notes |
| 2, 6, 10, 14, 18, 22, 26, 30 | 0.4 | 8th note off-beats |
| 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31 | 0.2 | 16th note off-beats |

### The Algorithm

1. Get base weight for step position
2. BROKEN flattens the weight distribution (at broken=1, all weights converge to 0.5)
3. Add randomness scaled by BROKEN (more broken = more random variation)
4. DENSITY sets the threshold (density=0 → threshold=1.0, nothing fires; density=1 → threshold=0.0, everything fires)
5. Fire if effective weight exceeds threshold

### Voice Differentiation [voice-weights]

**Anchor (Kick Character)**: Emphasizes downbeats (0, 8, 16, 24)
**Shimmer (Snare Character)**: Emphasizes backbeats (8, 24)

### Acceptance Criteria
- [x] Weight table for 32 steps implemented *(2025-12-17: PulseField.h)*
- [x] Effective weight calculation with BROKEN flattening *(2025-12-17: ShouldStepFire())*
- [x] Noise injection scaled by BROKEN *(2025-12-17: HashStep + noise scaling)*
- [x] Threshold comparison with DENSITY *(2025-12-17: threshold = 1 - density)*
- [x] Separate weight tables for Anchor and Shimmer *(2025-12-17: kAnchorWeights, kShimmerWeights)*
- [x] Deterministic mode (seeded RNG) for reproducibility *(2025-12-17: PulseFieldState dual seeds)*
- [x] **FIXED**: DENSITY=0 = absolute silence (no leakage from FUSE/COUPLE) *(2025-12-17: v3-ratchet-fixes)*
- [x] **FIXED**: DRIFT=0 = zero variation (densityBias skipped when DRIFT=0) *(2025-12-17: v3-ratchet-fixes)*

---

## Feature: BROKEN Effects Stack [broken-effects]

### Effect 1: Swing (Tied to BROKEN)

| BROKEN Range | Genre Feel | Swing % | Character |
|--------------|------------|---------|-----------|
| 0-25% | Techno | 50-54% | Nearly straight, driving |
| 25-50% | Tribal | 54-60% | Mild shuffle, groove |
| 50-75% | Trip-Hop | 60-66% | Lazy, behind-beat |
| 75-100% | IDM | 66-58% + jitter | Continuous chaos |

### Effect 2: Micro-Timing Jitter

| BROKEN Range | Max Jitter | Feel |
|--------------|------------|------|
| 0-40% | ±0ms | Machine-tight |
| 40-70% | ±3ms | Subtle human feel |
| 70-90% | ±6ms | Loose, organic |
| 90-100% | ±12ms | Broken, glitchy |

### Effect 3: Step Displacement

| BROKEN Range | Displacement Chance | Max Shift |
|--------------|---------------------|-----------|
| 0-50% | 0% | 0 steps |
| 50-75% | 0-15% | ±1 step |
| 75-100% | 15-40% | ±2 steps |

### Effect 4: Velocity Variation

| BROKEN Range | Velocity Variation | Character |
|--------------|-------------------|-----------|
| 0-30% | ±5% | Consistent, mechanical |
| 30-60% | ±10% | Subtle dynamics |
| 60-100% | ±20% | Expressive, uneven |

### Acceptance Criteria
- [x] Swing calculated from BROKEN parameter *(2025-12-17: GetSwingFromBroken() in BrokenEffects.h)*
- [x] Jitter scales with BROKEN (0 below 40%) *(2025-12-17: GetJitterMsFromBroken())*
- [x] Step displacement at BROKEN > 50% *(2025-12-17: GetDisplacedStep())*
- [x] Velocity variation scales with BROKEN *(2025-12-17: GetVelocityWithVariation())*
- [x] All effects combine coherently *(2025-12-17: Integrated in Sequencer.cpp)*

---

## Feature: DRIFT Variation Control [drift-control]

### Stratified Stability Model

Not all steps are equally "lockable". Important beats (downbeats) stay stable longer than ghost notes as DRIFT increases.

| Step Type | Stability | Behavior |
|-----------|-----------|----------|
| Bar downbeats (0, 16) | 1.0 | Lock until DRIFT > 100% |
| Half notes (8, 24) | 0.85 | Lock until DRIFT > 85% |
| Quarter notes (4, 12, 20, 28) | 0.7 | Lock until DRIFT > 70% |
| 8th off-beats | 0.4 | Lock until DRIFT > 40% |
| 16th ghosts | 0.2 | Vary unless DRIFT < 20% |

**DRIFT sets the threshold**: Steps with stability *above* DRIFT use a fixed seed (same every loop). Steps *below* DRIFT use a varying seed (different each loop).

### Per-Voice DRIFT

| DRIFT Knob | Anchor Effective | Shimmer Effective | Result |
|------------|------------------|-------------------|--------|
| 0% | 0% | 0% | Both fully locked |
| 30% | 21% | 39% | Anchor mostly locked, Shimmer ghosts vary |
| 50% | 35% | 65% | Anchor quarters locked, Shimmer quite loose |
| 70% | 49% | 91% | Anchor half-notes locked, Shimmer nearly full drift |
| 100% | 70% | 100% | Anchor still somewhat stable, Shimmer fully evolving |

### Acceptance Criteria
- [x] Step stability values implemented (1.0 for downbeats → 0.2 for 16ths) *(2025-12-17: kStepStability[])*
- [x] DRIFT threshold determines which steps use locked vs. varying seed *(2025-12-17: ShouldStepFireWithDrift())*
- [x] `patternSeed_` persists across loops (locked elements) *(2025-12-17: PulseFieldState)*
- [x] `loopSeed_` regenerates on phrase reset (drifting elements) *(2025-12-17: OnPhraseReset())*
- [x] DRIFT = 0% produces identical pattern every loop *(2025-12-17: tested)*
- [x] DRIFT = 100% produces unique pattern each loop *(2025-12-17: tested)*
- [x] Per-voice DRIFT: Anchor uses 0.7× multiplier, Shimmer uses 1.3× *(2025-12-17: GetEffectiveDrift())*
- [x] CV modulation of DRIFT works correctly *(2025-12-17: CV 8 maps to DRIFT)*

---

## Feature: FUSE Energy Balance [fuse-balance]

FUSE tilts the energy between Anchor and Shimmer voices.

| FUSE | Anchor Density | Shimmer Density |
|------|----------------|-----------------|
| 0% (CCW) | +15% | -15% |
| 50% (Center) | 0% | 0% |
| 100% (CW) | -15% | +15% |

### Acceptance Criteria
- [x] FUSE at 0.5 = no change *(2025-12-17: ApplyFuse() in BrokenEffects.h)*
- [x] FUSE CCW boosts Anchor, reduces Shimmer *(2025-12-17: bias = (fuse-0.5)*0.3)*
- [x] FUSE CW boosts Shimmer, reduces Anchor *(2025-12-17: tested)*
- [x] ±15% density shift at extremes *(2025-12-17: tested)*

---

## Feature: COUPLE Voice Interlock [couple-interlock]

COUPLE is a simplified replacement for the three-mode ORBIT system. It provides a single 0-100% axis for voice relationship strength.

| COUPLE | Behavior |
|--------|----------|
| 0% | Fully independent — voices can collide or gap freely |
| 50% | Soft interlock — slight collision avoidance |
| 100% | Hard interlock — Shimmer always fills Anchor gaps |

### Acceptance Criteria
- [x] COUPLE parameter (0-1) controls interlock strength *(2025-12-17: ApplyCouple() in BrokenEffects.h)*
- [x] At 0%: voices are fully independent *(2025-12-17: < 10% = no interaction)*
- [x] At 100%: shimmer strongly fills anchor gaps *(2025-12-17: boostChance up to 30%)*
- [x] Collision suppression scales with COUPLE *(2025-12-17: suppressChance = couple*0.8)*
- [x] Gap-filling boost at COUPLE > 50% *(2025-12-17: tested)*

---

## Feature: RATCHET Fill Intensity [ratchet-control]

RATCHET (K4+Shift) controls fill intensity during phrase transitions. While DRIFT controls WHEN fills occur, RATCHET controls HOW INTENSE they are.

### DRIFT × RATCHET Interaction

| DRIFT | RATCHET | Result |
|-------|---------|--------|
| 0% | any | No fills (pattern locked) |
| 50% | 0% | Occasional subtle fills |
| 50% | 100% | Occasional intense fills |
| 100% | 0% | Frequent subtle fills |
| 100% | 100% | Maximum fill frequency and intensity |

**Key distinction:**
- **DRIFT** = "Do fills happen?" (probability gate)
- **RATCHET** = "How intense are fills?" (intensity control)

### RATCHET Effects

| RATCHET Level | Behavior |
|---------------|----------|
| 0% | Subtle fills — slight density boost only |
| 25% | Moderate fills — noticeable energy increase |
| 50% | Standard fills — clear rhythmic intensification |
| 75% | Aggressive fills — ratcheted 16ths, strong build |
| 100% | Maximum intensity — rapid-fire hits, peak energy |

### Fill Zones

Fills are structured to target musically appropriate positions:

| Zone | Phrase Progress | Purpose |
|------|-----------------|---------|
| Groove | 0-40% | Stable pattern, minimal fills |
| Mid-Point | 40-60% | Potential mid-phrase fill |
| Build | 50-75% | Increasing energy toward phrase end |
| Fill | 75-100% | Maximum fill activity, resolving to downbeat |

### Acceptance Criteria
- [x] RATCHET parameter (K4+Shift) implemented *(2025-12-17)*
- [x] RATCHET controls fill density boost (0-30%) *(2025-12-17)*
- [x] Ratcheting (32nd subdivisions) at RATCHET > 50% *(2025-12-17)*
- [x] Velocity ramp in fills (louder toward phrase end) *(2025-12-17)*
- [x] Resolution accent on phrase downbeat *(2025-12-17)*
- [x] DRIFT=0 = no fills, regardless of RATCHET *(2025-12-17)*
- [x] Fills target phrase boundaries (mid-point, end) *(2025-12-17)*

---

## Feature: Pattern Generation [duopulse-patterns]

> **v2 Feature**: This section documents the discrete pattern lookup system. v3 replaces this with the Weighted Pulse Field Algorithm [pulse-field].

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
| **CV 7** | BROKEN |
| **CV 8** | DRIFT |

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

## Feature: External Clock Behavior [external-clock-behavior]

When an external clock is patched to **Gate In 1**, the sequencer automatically switches to external clock mode with context-aware control behavior.

### Clock Detection

- External clock is detected on rising edge of Gate In 1
- Timeout: 2 seconds without clock pulse reverts to internal clock
- Transition is automatic—no manual mode switch required

### Context-Aware K4 Controls

When external clock is patched, K4 controls change function:

| Control | Internal Clock | External Clock |
|---------|----------------|----------------|
| **K4 Primary** | TEMPO (90-160 BPM) | CLOCK DIV (sequencer rate) |
| **K4+Shift** | CLOCK DIV (output rate) | AUX MODE (CV Out 1 mode) |

### External Clock Rate Division

When external clock is active, K4 (Primary) controls the sequencer step rate relative to incoming clock:

| Range | Division | Effect |
|-------|----------|--------|
| 0-20% | ÷4 | 4 clock pulses per sequencer step (slow) |
| 20-40% | ÷2 | 2 clock pulses per sequencer step |
| 40-60% | ×1 | 1 clock pulse per step (unity) |
| 60-80% | ×2 | 2 sequencer steps per clock pulse |
| 80-100% | ×4 | 4 sequencer steps per clock pulse (fast) |

### Clock Output Timing

- Clock output (CV Out 1) is sample-accurate (updated in AudioCallback at 48kHz)
- Pulse width: configurable, default 10ms
- Swing timing is respected in clock output

### Acceptance Criteria
- [ ] Sample-accurate clock output (move from ProcessControls to AudioCallback)
- [ ] External clock detection with 2-second timeout
- [ ] Context-aware K4: TEMPO → CLOCK DIV when external clock patched
- [ ] Context-aware K4+Shift: CLOCK DIV → AUX MODE when external clock patched
- [ ] External clock rate division/multiplication (÷4 to ×4)
- [ ] Robust against noise/double-triggering on Gate In 1
- [ ] Smooth BPM estimation when slaved to external clock

---

## Feature: Auxiliary Output Modes [aux-output-modes]

**CV Out 1** is a multi-purpose auxiliary output. When external clock is patched, it can output various useful signals instead of redundant clock.

### Mode Selection

When external clock is patched, **K4+Shift** (Config Shift) selects the aux output mode:

| Range | Mode | Output Description |
|-------|------|-------------------|
| 0-17% | **Clock** | Swung clock output (default, same as internal clock mode) |
| 17-33% | **HiHat** | Third trigger output for hi-hat/ghost percussion |
| 33-50% | **Fill Gate** | High (5V) during fill zones, low otherwise |
| 50-67% | **Phrase CV** | 0-5V ramp through phrase, resets at loop boundary |
| 67-83% | **Accent** | Trigger only on accented hits |
| 83-100% | **Downbeat** | Trigger on bar/phrase downbeats |

### Mode Details

**Clock Mode** (0-17%):
- Outputs the sequencer clock with swing timing
- Respects CLOCK DIV setting (when internal clock)
- Default mode, compatible with previous behavior

**HiHat Mode** (17-33%):
- Outputs ghost/hi-hat triggers generated by ChaosModulator
- Fires on `ghostTrigger` events from the high-variation chaos stream
- Lower velocity than main voices (0.3-0.5 range)
- Great for driving a third VCA or percussion module

**Fill Gate Mode** (33-50%):
- Outputs a gate signal indicating fill/build zones
- High (5V) when `isFillZone` or `isBuildZone` is true
- Patch to effects depth, filter cutoff, or burst generators
- Follows phrase position tracking

**Phrase CV Mode** (50-67%):
- Outputs phrase progress as 0-5V CV
- Rises linearly from 0V at phrase start to 5V at phrase end
- Resets to 0V at loop boundary
- Scales with loop length (faster for 1-bar, slower for 16-bar)
- Patch to filter sweeps, VCA for dynamic arc, etc.

**Accent Mode** (67-83%):
- Outputs trigger only when accented hits occur
- Fires when either Anchor or Shimmer triggers with accent-eligible intensity
- Useful for emphasizing strong beats on external modules

**Downbeat Mode** (83-100%):
- Outputs trigger on bar downbeats (`isDownbeat` = step 0 of any bar)
- Optional: phrase reset pulse (step 0 of loop)
- Useful for syncing external sequencers, resetting envelopes

### Internal Clock Behavior

When using internal clock (no external clock patched):
- CV Out 1 always outputs clock (respecting CLOCK DIV setting)
- AUX MODE setting is ignored (K4+Shift = CLOCK DIV)

### Acceptance Criteria
- [ ] Mode selection via K4+Shift when external clock patched
- [ ] Clock mode: swung clock output (existing behavior)
- [ ] HiHat mode: ghost trigger output from ChaosModulator
- [ ] Fill Gate mode: high during fill/build zones
- [ ] Phrase CV mode: 0-5V ramp based on phraseProgress
- [ ] Accent mode: trigger on accent-eligible hits
- [ ] Downbeat mode: trigger on bar/phrase downbeats
- [ ] Mode persists across power cycles (save to flash)
- [ ] LED feedback indicates current mode when adjusting

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
| Clock Output | Swung timing, sample-accurate |
| External Clock Timeout | 2 seconds |
| Aux Output Modes | 6 (Clock, HiHat, Fill, Phrase, Accent, Downbeat) |

---

## Parameter Quick Reference

### Performance Mode (Switch DOWN)

|    | Primary | +Shift |
|----|---------|--------|
| K1 | ANCHOR DENSITY | FUSE |
| K2 | SHIMMER DENSITY | LENGTH |
| K3 | BROKEN | COUPLE |
| K4 | DRIFT | RATCHET |

### Config Mode (Switch UP)

|    | Primary (Internal Clock) | Primary (External Clock) | +Shift (Internal) | +Shift (External) |
|----|--------------------------|--------------------------|-------------------|-------------------|
| K1 | ANCHOR ACCENT | ANCHOR ACCENT | SWING TASTE | SWING TASTE |
| K2 | SHIMMER ACCENT | SHIMMER ACCENT | GATE TIME | GATE TIME |
| K3 | CONTOUR | CONTOUR | HUMANIZE | HUMANIZE |
| K4 | TEMPO | CLOCK DIV | CLOCK DIV | AUX MODE |

> *K4 is context-aware based on whether external clock is patched. See [external-clock-behavior] and [aux-output-modes].*

---

## Summary of Key Innovations

### v3 Algorithmic Approach

1. **BROKEN unifies genre**: Swing, jitter, displacement all scale with one knob. Genre character (Techno/Tribal/Trip-Hop/IDM) emerges naturally.
2. **DRIFT unifies predictability**: No more separate "deterministic mode". DRIFT = 0% locks the pattern; DRIFT = 100% makes it evolve.
3. **Stratified stability**: At intermediate DRIFT, downbeats stay locked while ghost notes vary. "Stable groove with living details."
4. **Per-voice DRIFT**: Anchor is more stable (0.7× drift), Shimmer is more drifty (1.3× drift). Even at max DRIFT, kicks have some stability.
5. **COUPLE simplifies voice relationship**: Single 0-100% interlock strength replaces three discrete modes.
6. **Weighted Pulse Field**: Continuous algorithm replaces discrete pattern lookups. No more Terrain/Grid mismatches.

### Core Architecture (v2/v3)

7. **Abstract Terminology**: Anchor/Shimmer instead of Kick/Snare—encourages modular thinking
8. **CV Always Performance**: CV inputs always modulate performance params, regardless of mode
9. **Swung Clock Output**: Clock respects swing for downstream module sync
10. **Phrase-Aware Composition**: Longer patterns have musical arc with builds and fills
11. **Gradual Soft Pickup**: Interpolated takeover instead of catch-up
12. **CONTOUR CV Modes**: Velocity/Decay/Pitch/Random expression
13. **HUMANIZE Jitter**: Micro-timing variation for organic feel
14. **Context-Aware Controls**: K4 automatically switches function when external clock is patched
15. **Auxiliary Output Modes**: CV Out 1 becomes HiHat trigger, Fill gate, Phrase CV, or more when using external clock
