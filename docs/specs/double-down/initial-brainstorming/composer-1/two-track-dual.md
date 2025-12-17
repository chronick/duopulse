# Two-Track Dual Sequencer Specification

## Design Philosophy

This sequencer embraces the constraint of **two trigger outputs** and **two CV outputs** as a creative strength rather than a limitation. Instead of trying to map traditional drum sounds (kick, snare, hihat) to two channels, we design a system optimized for expressive control over two complementary or contrasting rhythmic tracks.

### Core Concept: Dual-Track Rhythm Engine

- **Track 1 (Low/Driving)**: The foundation rhythm. Can range from sparse, powerful accents to dense, evolving patterns. Optimized for low-frequency percussion modules (BIA, Tymp Legio, etc.)
- **Track 2 (Mid/High)**: The complementary rhythm. Can be tightly synchronized with Track 1 or operate with independent timing. Optimized for mid/high-frequency percussion modules.

The sequencer provides **deep, multi-dimensional control** over each track's character, timing, dynamics, and evolution.

## Hardware Control Layout

### Control Architecture
- **4 Performance Knobs** (CV_1-4) + **4 CV Inputs** (CV_5-8)
- **4 Shift Knobs** (same knobs with Button B7 held) + **4 Shift CV Inputs**
- **Mode Switch** (B8): Toggles between Performance Mode and Config Mode
- **Button B7**: Shift modifier (when held, accesses secondary controls)

**Total Controls**: 8 per mode × 2 modes = 16 distinct parameter sets

### Performance Mode (Switch DOWN)

Primary performance controls optimized for live manipulation during playback.

#### Base Controls (Button Released)

| Control | Function | Range | Description |
|---------|----------|-------|-------------|
| **K1** | **Track 1 Density** | 0.0-1.0 | Controls hit frequency for Track 1. Low = sparse, powerful accents. High = dense, busy patterns. |
| **K2** | **Track 2 Density** | 0.0-1.0 | Controls hit frequency for Track 2. Independent from Track 1. |
| **K3** | **Track 1 Variation** | 0.0-1.0 | Controls dynamics, velocity variance, and ghost note probability for Track 1. Low = steady, predictable. High = chaotic fills, velocity swells. |
| **K4** | **Track 2 Variation** | 0.0-1.0 | Controls dynamics and chaos for Track 2. Independent from Track 1. |
| **CV1** | Modulates K1 | ±0.5 | Adds/subtracts from Track 1 Density |
| **CV2** | Modulates K2 | ±0.5 | Adds/subtracts from Track 2 Density |
| **CV3** | Modulates K3 | ±0.5 | Adds/subtracts from Track 1 Variation |
| **CV4** | Modulates K4 | ±0.5 | Adds/subtracts from Track 2 Variation |

#### Shift Controls (Button B7 Held)

| Control | Function | Range | Description |
|---------|----------|-------|-------------|
| **K1+Shift** | **Track 1 Velocity Range** | 0.0-1.0 | Controls the dynamic range of Track 1. Low = narrow range (subtle dynamics). High = wide range (quiet ghost notes to powerful accents). |
| **K2+Shift** | **Track 2 Velocity Range** | 0.0-1.0 | Controls the dynamic range of Track 2. Independent from Track 1. |
| **K3+Shift** | **Track 1 Timing Offset** | 0.0-1.0 | Shifts Track 1 timing forward/backward in micro-steps. Creates polyrhythmic feel or tight syncopation. 0.5 = no offset. |
| **K4+Shift** | **Track 2 Timing Offset** | 0.0-1.0 | Shifts Track 2 timing independently. Can create complex interlocking patterns. |
| **CV5+Shift** | Modulates K1+Shift | ±0.5 | Velocity range modulation |
| **CV6+Shift** | Modulates K2+Shift | ±0.5 | Velocity range modulation |
| **CV7+Shift** | Modulates K3+Shift | ±0.5 | Timing offset modulation |
| **CV8+Shift** | Modulates K4+Shift | ±0.5 | Timing offset modulation |

### Config Mode (Switch UP)

Setup, pattern selection, and global configuration.

#### Base Controls (Button Released)

| Control | Function | Range | Description |
|---------|----------|-------|-------------|
| **K1** | **Pattern Style** | 0.0-1.0 | Selects base rhythmic character. Maps to different pattern algorithms/styles (e.g., straight 4/4 → syncopated → polyrhythmic → IDM/breakbeat). |
| **K2** | **Pattern Length** | 0.0-1.0 | Sets loop length: 1, 2, 4, 8, or 16 bars. |
| **K3** | **Track Relationship** | 0.0-1.0 | Controls how Tracks 1 and 2 interact. Low = complementary (fills gaps), Mid = independent, High = contrasting (competing rhythms). |
| **K4** | **Tempo** | 0.0-1.0 | Sets internal clock tempo (30-200 BPM). Ignored if external clock present. |
| **CV1** | Modulates K1 | ±0.5 | Pattern style modulation |
| **CV2** | Modulates K2 | ±0.5 | Pattern length modulation |
| **CV3** | Modulates K3 | ±0.5 | Track relationship modulation |
| **CV4** | Modulates K4 | ±0.5 | Tempo modulation |

#### Shift Controls (Button B7 Held)

| Control | Function | Range | Description |
|---------|----------|-------|-------------|
| **K1+Shift** | **Swing Amount** | 0.0-1.0 | Applies swing/shuffle to both tracks. Low = straight timing. High = heavy swing (up to 66% swing). |
| **K2+Shift** | **Accent Pattern** | 0.0-1.0 | Controls accent placement pattern. Low = accents on downbeats. High = accents on offbeats/upbeats. |
| **K3+Shift** | **Fill Probability** | 0.0-1.0 | Controls likelihood of fill patterns at loop boundaries. Low = no fills. High = frequent fills. |
| **K4+Shift** | **Gate Length** | 0.0-1.0 | Controls trigger/gate duration for both tracks. Low = short triggers (5ms). High = longer gates (50ms). |
| **CV5+Shift** | Modulates K1+Shift | ±0.5 | Swing modulation |
| **CV6+Shift** | Modulates K2+Shift | ±0.5 | Accent pattern modulation |
| **CV7+Shift** | Modulates K3+Shift | ±0.5 | Fill probability modulation |
| **CV8+Shift** | Modulates K4+Shift | ±0.5 | Gate length modulation |

## Sequencer Engine Design

### Pattern Generation

The sequencer uses a **dual-track pattern generator** that creates patterns optimized for two-channel output.

#### Pattern Style Mapping (Config K1)

Instead of mapping to a 2D grid of pre-computed patterns, the sequencer uses **algorithmic pattern generation** with style parameters:

1. **Straight (0.0-0.25)**: 4/4, 8th note patterns. Predictable, techno-friendly.
2. **Syncopated (0.25-0.5)**: Offbeat accents, 16th note syncopation. Funk/hip-hop feel.
3. **Polyrhythmic (0.5-0.75)**: 3:2, 5:4, 7:8 patterns. Complex interlocking rhythms.
4. **IDM/Breakbeat (0.75-1.0)**: Irregular patterns, micro-timing variations. Experimental.

Each style affects:
- Step probability distribution
- Accent placement rules
- Syncopation amount
- Polyrhythmic ratios

#### Density Control

Density controls the **hit probability** per step, not just a threshold:

- **Low Density (0.0-0.3)**: Sparse hits, emphasis on strong accents
- **Medium Density (0.3-0.7)**: Balanced patterns
- **High Density (0.7-1.0)**: Busy patterns, fills, ghost notes

The density parameter affects:
- Base hit probability per step
- Ghost note probability (scaled by Variation)
- Fill pattern frequency

#### Variation/Chaos Control

Variation controls multiple dimensions:

1. **Velocity Variance**: Random variation in hit velocity (±range based on Velocity Range control)
2. **Ghost Note Probability**: Additional quiet hits between main accents
3. **Timing Jitter**: Micro-timing variations (±5ms at max variation)
4. **Fill Triggers**: Random fill patterns inserted at loop boundaries

#### Track Relationship (Config K3)

Controls how Tracks 1 and 2 interact:

- **Complementary (0.0-0.33)**: Track 2 fills gaps in Track 1. Creates call-and-response patterns.
- **Independent (0.33-0.67)**: Tracks operate independently. Can create polyrhythmic feel.
- **Contrasting (0.67-1.0)**: Tracks compete for space. Creates tension and complexity.

Implementation:
- Complementary: Track 2 hit probability increases when Track 1 is silent
- Independent: Tracks use separate pattern generators
- Contrasting: Track 2 hit probability decreases when Track 1 hits (ducking)

### Timing System

#### Timing Offset (Performance K3/K4+Shift)

Shifts the timing of a track forward or backward in micro-steps:

- **0.0**: Maximum backward shift (-16 steps)
- **0.5**: No offset (aligned with clock)
- **1.0**: Maximum forward shift (+16 steps)

This allows:
- Creating polyrhythmic feels
- Tight syncopation
- Phasing effects

#### Swing (Config K1+Shift)

Applies swing/shuffle timing:

- **0.0**: Straight timing (50/50)
- **0.5**: Light swing (55/45)
- **1.0**: Heavy swing (66/33)

Swing affects both tracks equally, applied to 16th note subdivisions.

### Velocity/Dynamics System

#### Velocity Range (Performance K1/K2+Shift)

Controls the dynamic range of hits:

- **Low (0.0-0.3)**: Narrow range (0.7-1.0 normalized). Subtle dynamics.
- **Medium (0.3-0.7)**: Standard range (0.4-1.0 normalized).
- **High (0.7-1.0)**: Wide range (0.1-1.0 normalized). Includes quiet ghost notes.

#### Accent Pattern (Config K2+Shift)

Controls accent placement:

- **Low (0.0-0.33)**: Accents on downbeats (1, 5, 9, 13...)
- **Medium (0.33-0.67)**: Accents on offbeats (3, 7, 11, 15...)
- **High (0.67-1.0)**: Complex accent patterns (syncopated, irregular)

Accents are applied as velocity multipliers (1.5x-2.0x).

### Output Generation

#### Track 1 Output (Gate Out 1, Audio Out 1, CV Out 1)

- **Gate Out 1**: Digital trigger (0V/5V)
- **Audio Out 1**: Velocity-scaled gate pulse (0V-5V DC-coupled)
- **CV Out 1**: Optional clock output or modulation

#### Track 2 Output (Gate Out 2, Audio Out 2, CV Out 2)

- **Gate Out 2**: Digital trigger (0V/5V)
- **Audio Out 2**: Velocity-scaled gate pulse (0V-5V DC-coupled)
- **CV Out 2**: LED brightness (visual feedback)

#### Gate Length (Config K4+Shift)

Controls trigger/gate duration:

- **Low (0.0)**: 5ms triggers (short, snappy)
- **Medium (0.5)**: 20ms gates (standard)
- **High (1.0)**: 50ms gates (longer, sustained)

## Implementation Notes

### Soft Takeover

All controls use soft takeover when switching modes or on startup. The system uses "value scaling" interpolation to smoothly converge to the physical knob position without parameter jumps.

### Pattern Storage

Patterns are generated algorithmically rather than stored as pre-computed sequences. This allows:
- Infinite pattern variations
- Smooth interpolation between styles
- Real-time parameter changes

### External Clock

- **Gate In 1**: External clock input (rising edge triggers step)
- **Gate In 2**: Pattern reset (resets to step 0)
- Auto-detection: If external clock stops, falls back to internal clock after timeout

### Visual Feedback

- **CV Out 2 / LED**: 
  - **Performance Mode**: Pulses on Track 1 hits
  - **Config Mode**: Solid ON
  - **Interaction**: Brightness reflects active parameter value when knob is turned (1s timeout)

## Control Permutations Summary

### Performance Mode (16 total controls)
- **Base (4)**: Density ×2, Variation ×2
- **Shift (4)**: Velocity Range ×2, Timing Offset ×2
- **CV Modulation (8)**: Each knob has corresponding CV input

### Config Mode (16 total controls)
- **Base (4)**: Style, Length, Track Relationship, Tempo
- **Shift (4)**: Swing, Accent Pattern, Fill Probability, Gate Length
- **CV Modulation (8)**: Each knob has corresponding CV input

**Total Expressive Dimensions**: 32 distinct parameters (16 per mode)

## Design Rationale

### Why Two Tracks?

Two tracks allow for:
- **Complementary rhythms**: One track drives, the other fills
- **Polyrhythmic complexity**: Independent timing creates rich interlocking patterns
- **Contrasting textures**: Sparse vs. dense, steady vs. chaotic
- **Performance control**: Each track independently manipulable

### Why Not Traditional Drum Mapping?

Traditional drum mapping (kick/snare/hihat) assumes:
- Fixed roles for each voice
- Limited expressive control per voice
- Pattern complexity comes from multiple voices

Our approach:
- **Flexible roles**: Tracks can be sparse or dense, steady or chaotic
- **Deep control**: Multiple dimensions per track (density, variation, timing, dynamics)
- **Pattern complexity** comes from interaction between two deeply controllable tracks

### Control Philosophy

The control layout maximizes **creative leverage**:
- **Density + Variation**: Core performance controls (always accessible)
- **Velocity Range + Timing Offset**: Expressive shaping (shift access)
- **Style + Relationship**: Pattern character (config mode)
- **Swing + Accents**: Timing and dynamics refinement (config shift)

This allows performers to:
1. **Quickly adjust** core rhythm (Density/Variation)
2. **Shape dynamics** (Velocity Range, Timing Offset)
3. **Change character** (Style, Relationship)
4. **Refine details** (Swing, Accents, Fills)

## Future Enhancements

Potential additions:
- **Pattern Memory**: Save/recall 8-16 patterns
- **Euclidean Patterns**: Generate Euclidean rhythms per track
- **Probability Curves**: Custom probability distributions per track
- **CV Output Modulation**: Use CV outs for LFOs or modulation
- **Pattern Morphing**: Smooth interpolation between saved patterns

