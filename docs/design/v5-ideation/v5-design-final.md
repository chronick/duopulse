# V5 Design Final Specification

**Date**: 2026-01-04
**Status**: Ready for Implementation
**Iterations**: 8 rounds of design review

---

## Executive Summary

V5 is a complete redesign of the control interface, eliminating all shift layers while maintaining expressiveness. The result is a clean 8-parameter system (4 performance + 4 config) with intuitive knob pairing and a single "secret mode" gesture for AUX output selection.

### Key Design Principles

1. **Zero shift layers** - No hidden functions behind button holds in either mode
2. **CV law** - CV1-4 always modulate performance parameters
3. **Knob pairing** - Each knob position has related functions across modes
4. **Secret mode** - HAT output as discoverable easter egg ("2.5 pulse")

---

## Control Layout

### Performance Mode

```
K1: ENERGY    K2: SHAPE     K3: AXIS X    K4: AXIS Y
CV1           CV2           CV3           CV4
```

| Parameter | Domain | 0% | 100% |
|-----------|--------|-----|------|
| ENERGY | Density | Sparse | Busy |
| SHAPE | Regularity | Stable (humanized euclidean) | Wild (weighted) |
| AXIS X | Beat Position | Grounded (downbeats) | Floating (offbeats) |
| AXIS Y | Intricacy | Simple | Complex |

### Config Mode

```
K1: CLOCK DIV   K2: SWING   K3: DRIFT   K4: ACCENT
```

| Parameter | Domain | 0% | 100% |
|-----------|--------|-----|------|
| CLOCK DIV | Tempo | ÷4 (slow) | ×4 (fast) |
| SWING | Groove | Straight | Heavy swing |
| DRIFT | Evolution | Locked (same each phrase) | Evolving |
| ACCENT | Velocity Depth | Flat (all hits equal) | Dynamic (ghost notes to accents) |

### Knob Pairing

| Knob | Performance | Config | Conceptual Link |
|------|-------------|--------|-----------------|
| K1 | ENERGY | CLOCK DIV | Rate/Density |
| K2 | SHAPE | SWING | Timing Feel |
| K3 | AXIS X | DRIFT | Variation/Movement |
| K4 | AXIS Y | ACCENT | Intricacy/Depth |

---

## Button & Switch Behavior

### Button Gestures

| Gesture | Effect |
|---------|--------|
| Tap (20-500ms) | Trigger Fill |
| Hold 3s | Reseed pattern on release |
| Hold + Switch UP | HAT mode ("2.5 pulse" secret) |
| Hold + Switch DOWN | FILL GATE mode (default) |

### Switch

| Position | Normal | With Button Held |
|----------|--------|------------------|
| UP | Performance mode | Set AUX to HAT |
| DOWN | Config mode | Set AUX to FILL GATE |

### AUX Mode Gesture Priority

When button is held and switch moves:
1. Cancel any pending shift mode activation
2. Cancel any pending live fill
3. Set AUX mode based on switch direction
4. Consume switch event (don't change Perf/Config mode)
5. Button release returns to normal (no fill triggered)

---

## AUX Output Modes

### FILL GATE (Default)

- AUX output goes HIGH during fill duration
- Simple gate signal for triggering external effects
- Matches "duopulse" branding (2 voices)

### HAT Mode (Secret "2.5 Pulse")

- AUX output produces pattern-aware hat burst during fill
- Burst density follows ENERGY
- Burst regularity follows SHAPE
- Pre-allocated (max 12 triggers, no heap allocation)
- Velocity reduced 70% near main pattern hits

### LED Feedback

```
HAT mode unlock:    ∙∙∙ ● ● ● (triple rising flash)
FILL GATE reset:    ● ∙ (single fade)
```

---

## Boot-Time Configuration

### Pattern Length

Default: 16 steps (compile-time)

Optional boot-time override:
- Hold button during power-on
- Switch UP: 16 steps (1 LED flash)
- Switch DOWN: 24 steps (2 LED flashes)

Compile-time options for 32/64 steps available for custom builds.

### AUX Mode

Always defaults to FILL GATE on boot (volatile, not persisted).

---

## CV Behavior

CV inputs (CV1-4) **always modulate Performance mode parameters**, regardless of current mode:

- In Performance mode: CV modulates same parameters as knobs
- In Config mode: CV still modulates performance parameters in background

This allows CV sequences to keep running while adjusting config settings.

---

## Algorithm Summary

| Algorithm | Approach |
|-----------|----------|
| SHAPE blending | Deterministic score interpolation |
| AXIS biasing | Additive offsets (no dead zones) |
| Voice relationship | COMPLEMENT + DRIFT variation |
| Hat burst | Pattern-aware, pre-allocated arrays |
| Fill | Multi-attribute boost + AUX output |
| ACCENT | Musical weight → velocity mapping |

---

## LED Feedback Summary

| State | Behavior | Layer |
|-------|----------|-------|
| Idle (perf) | Gentle breath synced to clock | 1 (base) |
| Idle (config) | Slower breath | 1 (base) |
| Clock sync | Subtle pulse on beats | 2 (additive) |
| Trigger activity | Pulse on hits, envelope decay | 3 (maximum) |
| Fill active | Accelerating strobe + trigger overlay | 4 (maximum) |
| Reseed progress | Building pulse (1-5Hz over 3s) | 5 (replace) |
| Reseed confirm | POP POP POP (3 flashes) | 5 (replace) |
| Mode switch | Quick signature | 5 (replace) |
| AUX mode unlock | Triple rising flash | 5 (replace) |
| AUX mode reset | Single fade | 5 (replace) |

---

## Hardware I/O

| I/O | Function |
|-----|----------|
| CV1-4 | Modulate ENERGY, SHAPE, AXIS X, AXIS Y |
| Gate Out 1 | Voice 1 triggers |
| Gate Out 2 | Voice 2 triggers |
| AUX Out | Fill gate OR hat burst |
| Clock In | External clock (normalled to internal) |
| Button | Fill / Reseed / AUX mode gesture |
| Switch | Mode select (up=Perf, down=Config) |
| LED | Status feedback (brightness only) |

---

## Implementation Checklist

### Critical (Must Do)

- [ ] Pre-allocate HatBurst arrays (no heap in audio path)
- [ ] Use deterministic score interpolation for SHAPE
- [ ] Use additive biasing for AXIS X/Y
- [ ] Implement LED layering as specified
- [ ] Layer trigger feedback on top of fills
- [ ] Implement Hold+Switch gesture for AUX MODE with proper cancellation
- [ ] Implement ACCENT parameter using musical weight for velocity

### Important (Should Do)

- [ ] COMPLEMENT relationship with DRIFT variation
- [ ] 70% velocity reduction for hat burst near main hits
- [ ] Boot-time LENGTH selection (button + switch at power-on)
- [ ] PATTERN_LENGTH as compile-time constant with boot override

### Deferred (Future)

- [ ] Extensibility framework (when bassline/ambient variations designed)
- [ ] Additional voice relationships (if user feedback indicates need)

---

## Open Items for Implementation

1. **CV slew/response** - Suggest: instant with per-phrase regeneration
2. **Power-on seed** - Suggest: fixed default seed, reseed to randomize

---

## Design Session Reference

Full design history in `docs/design/v5-ideation/`:
- `feedback.md` - Complete feedback log (8 rounds)
- `iteration-1.md` through `iteration-8.md` - Design evolution
- `critic-response-*.md` - Critical reviews

Key decisions made through iteration:
- Eliminated genre selection (algorithm-focused instead)
- Removed BUILD control (CV modulation replaces it)
- Simplified voice relationship to COMPLEMENT-only
- Added ACCENT as replacement for LENGTH in Config
- Created Hold+Switch gesture for AUX MODE
