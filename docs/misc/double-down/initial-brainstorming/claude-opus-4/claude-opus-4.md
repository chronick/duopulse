# Double Down: A Purpose-Built 2-Voice Drum Sequencer

**Design Document v1.0**  
**Model: Claude Opus 4**  
**Date: 2025-11-30**

---

## Executive Summary

"Double Down" abandons the paradigm of squeezing a 3-voice drum machine (kick/snare/hh) into 2 outputs. Instead, it embraces the 2-voice constraint as a **feature**, designing around maximum expressiveness per voice. Each voice gets both a trigger AND a CV output, allowing for dynamic, velocity-sensitive percussion that responds to performance in real-time.

---

## Core Philosophy

### The Problem with Porting Grids to 2 Outputs

The original Grids module generates patterns for 3 distinct drum voices (Kick, Snare, Hi-Hat). When forced into 2 outputs, you must either:
- Drop a voice entirely (loses character)
- Merge voices via "emphasis" routing (unpredictable beat topology)
- Create awkward hybrid channels (neither fish nor fowl)

None of these solutions honor the musicality of a well-designed 2-voice groove.

### The Double Down Solution

**Embrace the constraint.** Two voices, maximally expressive:

| Voice | Identity | Trigger Output | CV Output |
|-------|----------|----------------|-----------|
| **A** | The Pulse | Gate Out 1 | Audio Out 1 (velocity) |
| **B** | The Counter | Gate Out 2 | Audio Out 2 (expression) |

**Voice A (The Pulse):** The foundational, driving rhythm. Tends toward downbeats, steady anchoring. CV outputs velocity/accent for dynamic control.

**Voice B (The Counter):** The syncopation, response, sparkle. Fills gaps, adds movement. CV outputs expression (can be velocity, decay, or pitch depending on mode).

---

## Hardware Mapping

### Inputs

| Input | Function |
|-------|----------|
| **Knobs 1-4** | Performance/Config controls (mode-dependent) |
| **CV 5-8** | Modulation for Knobs 1-4 (additive) |
| **Button (B7)** | **SHIFT** - Access secondary control layer |
| **Switch (B8)** | **DOWN** = Performance Mode, **UP** = Config Mode |
| **Gate In 1** | External Clock (overrides internal) |
| **Gate In 2** | Pattern Reset |

### Outputs

| Output | Function |
|--------|----------|
| **Gate Out 1** | Voice A Trigger (5V digital) |
| **Gate Out 2** | Voice B Trigger (5V digital) |
| **Audio Out 1** | Voice A CV (0-5V, velocity-scaled) |
| **Audio Out 2** | Voice B CV (0-5V, expression-scaled) |
| **CV Out 1** | Clock Output (for chaining) |
| **CV Out 2** | LED + Parameter Feedback |

---

## Control Scheme

### Performance Mode (Switch DOWN)

The primary playing mode. Optimized for live manipulation.

#### Primary Layer (No Shift)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1** | **PULSE** | Global density/activity | Sparse ← → Busy |
| **K2** | **TILT** | Balance between A and B | A-heavy ← → B-heavy |
| **K3** | **SWING** | Shuffle/groove amount | Straight ← → Swung |
| **K4** | **DRIVE** | Accent intensity | Soft ← → Hard |

**PULSE** controls overall "how much is happening" - both voices scale proportionally.

**TILT** controls the relative density. At center, both voices are balanced. CCW emphasizes Voice A (more kicks, fewer counters). CW emphasizes Voice B (sparse foundation, busy accents).

**SWING** applies shuffle timing, from machine-straight to deeply swung.

**DRIVE** controls accent probability and intensity. At low values, all hits are similar velocity. At high values, accented hits are louder and more frequent.

#### Shift Layer (Button Held)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1+Shift** | **A-FILL** | Voice A ghost notes/variations | None ← → Full |
| **K2+Shift** | **B-FILL** | Voice B ghost notes/variations | None ← → Full |
| **K3+Shift** | **CHAOS** | Random trigger injection | Stable ← → Chaotic |
| **K4+Shift** | **LOCK** | Voice interlock amount | Free ← → Locked |

**A-FILL / B-FILL** add ghost notes and micro-fills to each voice independently. Low settings = clean pattern. High settings = busy embellishments.

**CHAOS** injects random triggers into both voices. At low values, the pattern is deterministic. At high values, random hits appear anywhere.

**LOCK** controls the relationship between voices:
- **0% (Free)**: Voices follow their own patterns independently
- **50% (Complement)**: Voice B actively avoids hitting when A hits
- **100% (Shadow)**: Voice B follows A with a delay/offset (echo/doubling effect)

---

### Config Mode (Switch UP)

System configuration and deeper parameters.

#### Primary Layer (No Shift)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1** | **PATTERN** | Base pattern selection | 1 ← → 16 (or 32) |
| **K2** | **LENGTH** | Loop length in bars | 1, 2, 4, 8, 16 |
| **K3** | **FEEL** | Time signature / groove family | See table |
| **K4** | **TEMPO** | Internal BPM | 30 ← → 200 |

**FEEL** selects the underlying groove family:

| Range | Feel | Description |
|-------|------|-------------|
| 0-20% | **4/4 Straight** | Standard techno/house pulse |
| 20-40% | **4/4 Broken** | Breakbeat, hip-hop feels |
| 40-60% | **Triplet** | Shuffled, swing-based grooves |
| 60-80% | **Odd Meter** | 5/4, 7/8, asymmetric patterns |
| 80-100% | **Polyrhythm** | A and B in different time signatures |

#### Shift Layer (Button Held)

| Knob | Name | Function | Range |
|------|------|----------|-------|
| **K1+Shift** | **A-CV MODE** | What Voice A's CV outputs | See table |
| **K2+Shift** | **B-CV MODE** | What Voice B's CV outputs | See table |
| **K3+Shift** | **GATE TIME** | Global gate/trigger duration | 1ms ← → 100ms |
| **K4+Shift** | **MORPH** | Pattern interpolation/transition | Snap ← → Smooth |

**CV MODE** options for each voice:

| Range | Mode | CV Behavior |
|-------|------|-------------|
| 0-25% | **Velocity** | CV = hit intensity (0-5V) |
| 25-50% | **Decay** | CV = envelope decay time hint |
| 50-75% | **Pitch** | CV = random pitch offset per hit |
| 75-100% | **Ratchet** | CV = trigger subdivision count |

**MORPH** controls how pattern changes occur. At low values, pattern changes happen immediately on the next bar. At high values, patterns crossfade/interpolate over several bars.

---

## Pattern Generation

### Two-Voice Pattern Architecture

Rather than adapting 3-voice Grids data, Double Down uses patterns designed for 2 voices with inherent musical relationships.

#### Pattern Data Structure

```cpp
struct Pattern {
    uint32_t voiceA[32];    // 32 steps, 8 velocity levels per step (0-7)
    uint32_t voiceB[32];    // Same format
    uint8_t  relationship;   // How A and B interact
    uint8_t  accentMask;     // Which steps are accent candidates
    uint8_t  feel;           // Groove family index
};
```

Each step stores a 3-bit value (0-7):
- 0 = No hit
- 1-3 = Soft hit (ghost note territory)
- 4-5 = Normal hit
- 6-7 = Accent hit

#### Pattern Families

**1. Four-on-the-Floor** (4/4 Straight)
- Voice A: Steady quarters (1-1-1-1)
- Voice B: Offbeat eighths, hi-hat pattern

**2. Minimal Techno** (4/4 Straight)  
- Voice A: Sparse kick, downbeat focus
- Voice B: Sparse but syncopated rim/click

**3. Breakbeat** (4/4 Broken)
- Voice A: Syncopated kick pattern (think Amen break kick placement)
- Voice B: Snare on 2 and 4, with variations

**4. Hip-Hop** (4/4 Broken)
- Voice A: Boom-bap kick pattern
- Voice B: Snare with ghost notes

**5. Shuffle** (Triplet)
- Voice A: Triplet-feel kick
- Voice B: Shuffle hi-hat/ride

**6. IDM Glitch** (Broken/Poly)
- Voice A: Irregular, evolving kicks
- Voice B: Scattered, algorithmic accents

**7. Polyrhythm 3:4** (Polyrhythm)
- Voice A: 4-beat cycle
- Voice B: 3-beat cycle (creates 12-step polyrhythm)

**8. Polyrhythm 5:4** (Polyrhythm)
- Voice A: 4-beat cycle  
- Voice B: 5-beat cycle (creates 20-step polyrhythm)

### Density Application

The PULSE control scales pattern density by adjusting trigger probability thresholds:

```cpp
// Pseudocode for density application
float threshold = (1.0f - pulse) * 7.0f;  // 0-7 scale

for each step:
    if (pattern.voiceA[step] > threshold):
        triggerA = true;
        velocityA = (pattern.voiceA[step] - threshold) / (7 - threshold);
```

At low PULSE, only accent hits (6-7) fire. At max PULSE, all hits including ghosts fire.

### Tilt Application

TILT adjusts the threshold independently per voice:

```cpp
float tiltOffset = (tilt - 0.5f) * 2.0f;  // -1.0 to +1.0

thresholdA = baseThreshold - tiltOffset;  // Lower = more triggers
thresholdB = baseThreshold + tiltOffset;  // Higher = fewer triggers
```

---

## CV Output Expression

### Velocity Mode (Default)

CV output directly reflects hit intensity:

```cpp
cvOut = velocity * 5.0f;  // 0-5V proportional to velocity
```

### Decay Mode

CV output provides an envelope decay time hint for connected modules:

```cpp
// Short decay (0V) for tight hits, long decay (5V) for open sounds
cvOut = decayHint * 5.0f;

// Decay hint derived from:
// - Accent status (accents = longer decay)
// - Pattern position (downbeats = longer decay)
// - Randomization from CHAOS
```

### Pitch Mode

CV output provides a random or sequenced pitch offset:

```cpp
// Random within scaled range based on CHAOS
cvOut = baseNote + (randomOffset * chaosAmount) * 5.0f;
```

### Ratchet Mode

CV output indicates subdivision count for the current trigger:

```cpp
// 0V = single hit, 2.5V = double, 5V = quad
cvOut = ratchetCount * 1.25f;  // 1-4 ratchets
```

---

## Interlock Modes

The LOCK parameter controls how Voice B relates to Voice A:

### Free Mode (0%)

Both voices follow their independent patterns. No modification.

### Complement Mode (50%)

Voice B is modified to avoid simultaneous hits with A:

```cpp
if (triggerA && triggerB) {
    // Push B to the next available slot, or suppress
    if (nextSlotFree) {
        delayB(1);
    } else {
        triggerB = false;
    }
}
```

This creates interlocking, non-overlapping rhythms.

### Shadow Mode (100%)

Voice B echoes Voice A with a configurable offset:

```cpp
triggerB = triggerA_delayed(shadowOffset);
velocityB = velocityA * shadowDecay;
```

Shadow offset can be 1/16, 1/32, or triplet-based depending on FEEL.

---

## Implementation Roadmap

### Phase 1: Core Engine
1. Create `DoubleDownEngine` class with 2-voice pattern storage
2. Implement new pattern data format
3. Port PULSE/TILT/SWING controls
4. Remove Grids dependency

### Phase 2: CV Expression
1. Implement CV modes (velocity, decay, pitch, ratchet)
2. Add per-voice CV mode selection
3. Add envelope shaping for CV outputs

### Phase 3: Interlock System
1. Implement Free mode (baseline)
2. Implement Complement mode (gap-filling)
3. Implement Shadow mode (echo/double)

### Phase 4: Pattern Library
1. Design 16-32 core patterns across all FEEL categories
2. Implement pattern interpolation (MORPH)
3. Add pattern generation algorithms for procedural content

### Phase 5: Polish
1. Tune all parameter ranges for playability
2. Add soft-takeover behavior
3. LED feedback implementation
4. External clock integration

---

## Design Rationale

### Why Not Just Fix the Emphasis System?

The current emphasis system tries to route 3 voices (K/S/HH) into 2 outputs. This creates several problems:

1. **Unpredictable beat topology**: When HH routes to kick channel, the "kick" suddenly has hi-hat-like density, breaking the foundational groove.

2. **Loss of voice identity**: The kick channel is no longer "just kick" - it's kick+sometimes-hh, which is conceptually confusing.

3. **Grids patterns aren't designed for 2 outputs**: The patterns assume 3 independent voices with different characters. Forcing them into 2 loses the intended groove.

### Why Two Distinct Voice Personalities?

Rather than two generic "tracks," Double Down gives each voice a clear identity:

- **Voice A = Foundation**: Lower register, downbeat-focused, steady. Think kick drum, bass hits, toms.
- **Voice B = Ornament**: Higher register, syncopated, responsive. Think snare, hi-hats, clicks, pops.

This isn't just naming - the pattern data is designed with this relationship in mind. Voice A patterns emphasize stability; Voice B patterns emphasize movement.

### Why Interlock Modes?

The LOCK parameter solves a key problem: how do two voices relate? In acoustic drumming, the hands and feet have natural relationships - they interlock, they respond, they echo. The interlock modes give that same musicality to the sequencer.

---

## Appendix: Parameter Quick Reference

### Performance Mode (Switch DOWN)

| | Primary | +Shift |
|--|---------|--------|
| K1 | PULSE (density) | A-FILL (ghost notes) |
| K2 | TILT (A↔B balance) | B-FILL (ghost notes) |
| K3 | SWING (groove) | CHAOS (randomness) |
| K4 | DRIVE (accents) | LOCK (voice relationship) |

### Config Mode (Switch UP)

| | Primary | +Shift |
|--|---------|--------|
| K1 | PATTERN | A-CV MODE |
| K2 | LENGTH | B-CV MODE |
| K3 | FEEL | GATE TIME |
| K4 | TEMPO | MORPH |

---

## Open Questions

1. **Pattern count**: 16 patterns? 32? More with procedural generation?

2. **Grids data reuse**: Should we salvage any Grids patterns by extracting kick+snare as our A+B, or start completely fresh?

3. **Ratchet implementation**: Should ratchets be triggers (multiple gate pulses) or CV hints (let downstream module handle it)?

4. **Tap tempo**: Keep in performance mode? Move to config shift?

5. **CV input behavior**: Current system is additive. Should any be multiplicative or switchable?

