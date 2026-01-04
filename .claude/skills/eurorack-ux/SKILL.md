---
name: eurorack-ux
description: Ergonomic Eurorack module design principles. Use when designing control layouts, UI/UX for hardware modules, evaluating usability, or balancing simplicity with expressiveness. Covers knob behavior, LED feedback, mode design, and performer-centric workflows.
allowed-tools: Read, Grep, Glob
---

# Eurorack UX Design Skill

## Purpose

This skill provides ergonomic design principles for Eurorack module interfaces. It helps designers balance simplicity with expressiveness while maximizing both. Use it when critiquing control layouts, designing new interfaces, or evaluating existing module UX.

## When to Activate

Activate this skill when:
- Designing or critiquing control layouts for Eurorack modules
- Evaluating knob/button/CV input assignments
- Planning LED or visual feedback systems
- Balancing feature depth vs. immediate usability
- Reviewing mode-switching or shift-function designs

## Core Philosophy: The Simplicity-Expressiveness Dialectic

The goal is NOT to compromise between simplicity and expressiveness, but to **maximize both simultaneously**. This requires:

1. **Layered Complexity**: Simple by default, deep when needed
2. **Coherent Mental Models**: Controls map to musical intent, not technical parameters
3. **Immediate Feedback**: Every action produces visible/audible confirmation
4. **Forgiveness**: Easy to recover from mistakes, hard to "break" the patch

### The Quadrant Test

Evaluate designs against this matrix:

```
                    HIGH EXPRESSIVENESS
                           │
       "Expert's Dream"    │    "Sweet Spot"
       (Complex but        │    (Simple surface,
        powerful)          │     deep control)
    ───────────────────────┼───────────────────────
       "Useless"           │    "One-Trick Pony"
       (Hard to use AND    │    (Easy but limited)
        not expressive)    │
                           │
                    LOW EXPRESSIVENESS
           LOW SIMPLICITY ◄──► HIGH SIMPLICITY
```

**Goal**: Always aim for the upper-right quadrant.

## Design Principles

### 1. Domain Mapping (Musical Intent > Technical Parameters)

**Bad**: Controls labeled by what they technically do
- `LFO Rate`, `VCA Gain`, `Filter Cutoff`

**Good**: Controls labeled by musical effect
- `ENERGY`, `PUNCH`, `DRIFT`, `BUILD`

This shift means users think "I want more intensity" not "I need to increase this CV offset by 0.3V."

### 2. Ergonomic Pairing (Related Controls Together)

Group controls that work together musically:

| Domain | Primary | Shift/Secondary |
|--------|---------|-----------------|
| Intensity | Energy (density) | Punch (dynamics) |
| Drama | Build (phrase arc) | Genre (style) |
| Pattern | Field X (syncopation) | Drift (evolution) |
| Texture | Field Y (complexity) | Balance (voice mix) |

**Why pairs work**:
- Reduces cognitive load (4 domains, not 8 parameters)
- Primary is "how much," secondary is "what kind"
- Shift functions should modify the primary's *character*, not add unrelated features

### 3. The 3-Second Rule

A performer should be able to:
- **Understand** the current state within 3 seconds of looking at the module
- **Make a useful change** within 3 seconds of touching it
- **Recover from a mistake** within 3 seconds

This means:
- LED feedback must be immediately interpretable
- Default states must be musically useful
- No buried menus or multi-step operations for common tasks

### 4. CV Modulation Philosophy

**Golden Rule**: CV should be ADDITIVE to knob position

- 0V = "no modulation" (knob position unchanged)
- +V = "more" (increase toward maximum)
- -V = "less" (decrease toward minimum)

**Anti-pattern**: CV centered at 2.5V or 5V
- Confusing because unpatched input doesn't match "no modulation"
- Requires offset modules to use properly

**For attenuverters/bipolar parameters**:
- Still use 0V as neutral
- Positive CV pushes toward one extreme
- Negative CV pushes toward the other

### 5. Mode Design (Shift vs. Dedicated)

**When to use Shift function**:
- Secondary control modifies the primary's character
- Used less frequently than primary
- Changing it mid-performance is rare

**When to use a dedicated control**:
- Parameter is tweaked frequently in performance
- Needs its own CV input
- Mental model treats it as independent

**Anti-patterns**:
- Too many shift layers (max 1 shift level recommended)
- Shift functions that are unrelated to the primary
- Mode-switching that redefines all controls

### 6. LED Feedback Guidelines

**Brightness** should indicate intensity/activity:
- Brighter = more triggers, more activity
- Dim = low activity, waiting state

**Blink patterns** for states:
- Slow pulse = clock following, ready
- Fast flash = trigger just fired
- Solid = held state (sustain, gate high)

**Color** (if available) for domains:
- Keep consistent across the module
- Don't rely on color alone (colorblindness)

### 7. The "Blank Panel Test"

Imagine the module with no labels. Can a user:
1. Find the most important control by size/position?
2. Identify related controls by grouping?
3. See the current state without reading text?

If not, the physical design needs work.

### 8. Default States

**Defaults should be musically useful**, not technically neutral:
- "50%" on all knobs should produce interesting output
- Power-on state should immediately respond to clock
- No configuration required for basic operation

**Exception**: Config parameters can default to "safest" setting if musicality requires user intent.

### 9. Boundary Behavior

**Knob endpoints should be meaningful**:
- 0% = "none" (zero density, no effect, silence)
- 100% = "maximum" (full density, maximum effect)

**Avoid**:
- "Dead zones" at extremes
- Useful range only in middle 50%
- Discontinuities (sudden jumps at threshold)

### 10. Error Prevention > Error Recovery

**Prevent catastrophic states**:
- No "silence forever" settings (always some audible output pathway)
- No runaway feedback (clamp gains, limit rates)
- Impossible to corrupt patch memory accidentally

**Graceful degradation**:
- If clock is lost, internal clock takes over
- If CV is disconnected, knob position becomes absolute
- If parameter is invalid, clamp to nearest valid value

## Evaluation Checklist

When reviewing a design, check:

### Control Layout
- [ ] Controls grouped by musical domain, not technical function?
- [ ] Most important controls in most accessible position?
- [ ] Shift functions related to their primary?
- [ ] Maximum 1 shift layer?

### CV Inputs
- [ ] 0V = no modulation?
- [ ] Unpatched input = knob position?
- [ ] Modulation range matches musical use case?
- [ ] CV-able parameters are the ones performers want to modulate?

### Feedback
- [ ] Current state visible without interaction?
- [ ] LED brightness indicates activity?
- [ ] Changes produce immediate visible/audible feedback?

### Usability
- [ ] Passes 3-second rule?
- [ ] Defaults are musically useful?
- [ ] Knob endpoints are meaningful (not dead zones)?
- [ ] Error states recoverable?

### Simplicity-Expressiveness Balance
- [ ] Simple surface, deep control?
- [ ] New users can make interesting output immediately?
- [ ] Experts can achieve precise control when needed?
- [ ] Complexity is layered, not mandatory?

## Anti-Patterns to Avoid

1. **Menu Diving**: Anything that requires "hold button + turn knob" sequences
2. **Mode Explosion**: Too many modes that redefine all controls
3. **Hidden Gotchas**: Important behaviors only discoverable by accident
4. **Parameter Soup**: Many similar parameters that interact unpredictably
5. **Expert-Only Defaults**: Requires configuration before first useful output
6. **Disconnected Feedback**: LED/output doesn't reflect control changes
7. **Binary Cliffs**: Smooth parameter suddenly jumps at threshold

## Project-Specific Notes (DuoPulse)

This project specifically targets the **Patch.Init()** platform:
- 4 knobs, no buttons
- 2 gate outputs, 2 audio outputs, 2 CV outputs
- 2 gate inputs, 2 audio inputs
- No display

Design constraints:
- Must use knob + shift function pattern (no mode buttons)
- Config mode is separate from performance mode
- LED (CV Out 2) is the only visual feedback
- Audio outputs are DC-coupled (can output CV)

## References

- Mutable Instruments design philosophy (Emilie Gillet's writings)
- Make Noise panel design patterns
- Monome norns/grid principles
- "Designing Musical Devices" by Bjorn Hartmann
