# Two-Track Dual Sequencer - Control Map Reference

## Quick Reference: Control Layout

### Performance Mode (Switch DOWN) - Button Released

```
┌─────────────────────────────────────────────────────────┐
│  PERFORMANCE MODE (Base Controls)                       │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  K1: Track 1 Density      CV1: Modulate K1             │
│  K2: Track 2 Density      CV2: Modulate K2             │
│  K3: Track 1 Variation    CV3: Modulate K3             │
│  K4: Track 2 Variation    CV4: Modulate K4             │
│                                                          │
│  [Button B7: Hold for Shift Controls]                   │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Performance Mode (Switch DOWN) - Button Held (Shift)

```
┌─────────────────────────────────────────────────────────┐
│  PERFORMANCE MODE (Shift Controls)                      │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  K1+Shift: Track 1 Velocity Range  CV5: Modulate K1+Shift│
│  K2+Shift: Track 2 Velocity Range  CV6: Modulate K2+Shift│
│  K3+Shift: Track 1 Timing Offset  CV7: Modulate K3+Shift│
│  K4+Shift: Track 2 Timing Offset  CV8: Modulate K4+Shift│
│                                                          │
│  [Button B7: Release to return to Base Controls]         │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Config Mode (Switch UP) - Button Released

```
┌─────────────────────────────────────────────────────────┐
│  CONFIG MODE (Base Controls)                            │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  K1: Pattern Style         CV1: Modulate K1             │
│  K2: Pattern Length        CV2: Modulate K2             │
│  K3: Track Relationship     CV3: Modulate K3             │
│  K4: Tempo                 CV4: Modulate K4             │
│                                                          │
│  [Button B7: Hold for Shift Controls]                   │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Config Mode (Switch UP) - Button Held (Shift)

```
┌─────────────────────────────────────────────────────────┐
│  CONFIG MODE (Shift Controls)                          │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  K1+Shift: Swing Amount      CV5: Modulate K1+Shift    │
│  K2+Shift: Accent Pattern     CV6: Modulate K2+Shift    │
│  K3+Shift: Fill Probability   CV7: Modulate K3+Shift    │
│  K4+Shift: Gate Length        CV8: Modulate K4+Shift    │
│                                                          │
│  [Button B7: Release to return to Base Controls]         │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

## Control Flow Diagram

```
                    ┌─────────────────┐
                    │   Mode Switch   │
                    │   (B8)          │
                    └────────┬────────┘
                             │
                ┌────────────┴────────────┐
                │                         │
         Switch DOWN              Switch UP
         (Performance)            (Config)
                │                         │
    ┌───────────┴───────────┐   ┌────────┴──────────┐
    │                       │   │                   │
Button Released      Button Held    Button Released    Button Held
(Base)              (Shift)        (Base)            (Shift)
    │                       │   │                   │
    │                       │   │                   │
K1: Density         K1: Vel Range  K1: Style        K1: Swing
K2: Density         K2: Vel Range  K2: Length       K2: Accents
K3: Variation       K3: Timing     K3: Relationship K3: Fills
K4: Variation       K4: Timing     K4: Tempo        K4: Gate Len
```

## Parameter Interaction Matrix

### Performance Mode Interactions

| Parameter | Affects | Interaction Type |
|-----------|---------|------------------|
| Track 1 Density | Hit frequency on Track 1 | Independent |
| Track 2 Density | Hit frequency on Track 2 | Independent |
| Track 1 Variation | Dynamics, ghost notes, fills on Track 1 | Scales with Density |
| Track 2 Variation | Dynamics, ghost notes, fills on Track 2 | Scales with Density |
| Track 1 Velocity Range | Dynamic range of Track 1 hits | Multiplies Variation effect |
| Track 2 Velocity Range | Dynamic range of Track 2 hits | Multiplies Variation effect |
| Track 1 Timing Offset | Micro-timing shift of Track 1 | Independent |
| Track 2 Timing Offset | Micro-timing shift of Track 2 | Independent |

### Config Mode Interactions

| Parameter | Affects | Interaction Type |
|-----------|---------|------------------|
| Pattern Style | Base rhythm algorithm | Global (both tracks) |
| Pattern Length | Loop length in bars | Global |
| Track Relationship | How tracks interact | Cross-track |
| Tempo | Clock speed | Global |
| Swing Amount | Timing feel | Global (both tracks) |
| Accent Pattern | Accent placement | Global (both tracks) |
| Fill Probability | Fill frequency | Global (both tracks) |
| Gate Length | Trigger duration | Global (both tracks) |

## Performance Workflow Examples

### Example 1: Building a Sparse, Powerful Pattern

1. **Config Mode**:
   - Set Style to 0.2 (Straight 4/4)
   - Set Track Relationship to 0.3 (Complementary)
   - Set Length to 4 bars

2. **Performance Mode**:
   - Track 1 Density: 0.2 (sparse)
   - Track 1 Variation: 0.1 (steady)
   - Track 1 Velocity Range: 0.8 (wide range for accents)
   - Track 2 Density: 0.3 (fills gaps)
   - Track 2 Variation: 0.2 (subtle)

### Example 2: Creating Polyrhythmic Complexity

1. **Config Mode**:
   - Set Style to 0.6 (Polyrhythmic)
   - Set Track Relationship to 0.7 (Independent)
   - Set Swing to 0.3 (light swing)

2. **Performance Mode**:
   - Track 1 Density: 0.5
   - Track 1 Timing Offset: 0.3 (shifted back)
   - Track 2 Density: 0.6
   - Track 2 Timing Offset: 0.7 (shifted forward)
   - Creates interlocking 3:2 feel

### Example 3: Chaotic, Evolving Pattern

1. **Config Mode**:
   - Set Style to 0.9 (IDM/Breakbeat)
   - Set Fill Probability to 0.8 (frequent fills)
   - Set Accent Pattern to 0.7 (offbeat accents)

2. **Performance Mode**:
   - Track 1 Density: 0.7
   - Track 1 Variation: 0.9 (high chaos)
   - Track 1 Velocity Range: 1.0 (full dynamic range)
   - Track 2 Density: 0.8
   - Track 2 Variation: 0.8
   - Track 2 Timing Offset: 0.4 (slight offset)

## Control Priority Guide

### Quick Adjustments (Performance Mode, Base)
- **Density**: Immediate impact on pattern busyness
- **Variation**: Quick way to add/remove chaos

### Fine-Tuning (Performance Mode, Shift)
- **Velocity Range**: Shape dynamics without changing pattern
- **Timing Offset**: Create polyrhythmic feels

### Character Changes (Config Mode, Base)
- **Style**: Change overall rhythmic feel
- **Track Relationship**: Change how tracks interact

### Refinement (Config Mode, Shift)
- **Swing**: Add groove
- **Accents**: Emphasize different beats
- **Fills**: Add complexity at loop boundaries
- **Gate Length**: Adjust trigger character

