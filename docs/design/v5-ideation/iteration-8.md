# V5 Design Iteration 8: Final Control Simplification

**Date**: 2026-01-04
**Status**: Ready for Critic Review
**Focus**: Eliminate Config+Shift, add ACCENT, finalize control layout

---

## Changes from Iteration 7

| Aspect | Iteration 7 | Iteration 8 |
|--------|-------------|-------------|
| Config shift layer | K3+Shift = AUX MODE | **Eliminated** |
| AUX MODE access | Shift gesture | **Hold+Switch gesture** |
| LENGTH | K4 in Config (16/24/32/64) | **Compile-time only** |
| K4 Config | LENGTH | **ACCENT** |
| Config knob count | 4 + 1 shift | **4 knobs, zero shift** |

---

## Final Control Layout

### Performance Mode (Zero Shift)

```
K1: ENERGY    K2: SHAPE     K3: AXIS X    K4: AXIS Y
CV1           CV2           CV3           CV4
```

| Param | Domain | 0% | 100% |
|-------|--------|-----|------|
| ENERGY | Density | Sparse | Busy |
| SHAPE | Regularity | Stable (humanized euclidean) | Wild (weighted) |
| AXIS X | Beat Position | Grounded (downbeats) | Floating (offbeats) |
| AXIS Y | Intricacy | Simple | Complex |

### Config Mode (Zero Shift)

```
K1: CLOCK DIV   K2: SWING   K3: DRIFT   K4: ACCENT
```

| Param | Domain | 0% | 100% |
|-------|--------|-----|------|
| CLOCK DIV | Tempo | ÷4 (slow) | ×4 (fast) |
| SWING | Groove | Straight | Heavy swing |
| DRIFT | Evolution | Locked (same each phrase) | Evolving |
| ACCENT | Velocity Depth | Flat (all hits equal) | Dynamic (ghost notes to accents) |

### Knob Pairing (Performance ↔ Config)

| Knob | Performance | Config | Conceptual Link |
|------|-------------|--------|-----------------|
| K1 | ENERGY | CLOCK DIV | Rate/Density |
| K2 | SHAPE | SWING | Timing Feel |
| K3 | AXIS X | DRIFT | Variation/Movement |
| K4 | AXIS Y | ACCENT | Intricacy/Depth |

---

## AUX MODE: Hold+Switch Gesture

### The "2.5 Pulse" Secret Mode

FILL GATE is the default, matching "duopulse" branding. HAT mode is a deliberate unlock - the secret "2.5 pulse" mode.

### Gesture

```
Hold button + Switch UP   → HAT mode (secret "2.5 pulse")
Hold button + Switch DOWN → FILL GATE mode (default)
```

### LED Feedback

```
HAT mode unlock:    ∙∙∙ ● ● ● (triple rising flash - "something extra unlocked")
FILL GATE reset:    ● ∙ (single fade - "back to basics")
```

### Implementation

```cpp
void ProcessModeSwitch(bool switchUp, bool switchDown) {
    if (buttonHeld) {
        // ═══════════════════════════════════════════════════════════════
        // Secret AUX mode gesture
        // IMPORTANT: Suppress shift mode and live fill when this triggers
        // ═══════════════════════════════════════════════════════════════

        CancelShiftMode();      // Don't activate shift
        CancelLiveFill();       // Don't trigger fill

        if (switchUp) {
            auxMode = AUX_HAT;
            TriggerLEDSecretUnlock();  // Triple rising flash
        } else if (switchDown) {
            auxMode = AUX_FILL_GATE;
            TriggerLEDSecretReset();   // Single fade
        }

        consumeSwitchEvent = true;  // Don't also change perf/config mode
        return;
    }

    // Normal mode switch behavior (only if switch not consumed)
    if (switchUp) {
        mode = PERFORMANCE;
        TriggerLEDModeSwitch(PERFORMANCE);
    } else if (switchDown) {
        mode = CONFIG;
        TriggerLEDModeSwitch(CONFIG);
    }
}
```

### Gesture Priority

When button is held and switch moves:
1. **Cancel** any pending shift mode activation
2. **Cancel** any pending live fill
3. **Set** AUX mode based on switch direction
4. **Consume** switch event (don't change Perf/Config mode)
5. **Release** button returns to normal (no fill triggered)

---

## ACCENT Parameter (New)

### What It Does

Controls internal velocity variation independent of output level.

- **0% (Flat)**: All hits have equal velocity - consistent, punchy
- **50% (Moderate)**: Some dynamic variation - musical accents on strong beats
- **100% (Full)**: Wide velocity range - ghost notes, accents, dynamic expression

### Why VCAs Can't Replace This

External VCAs scale the output level uniformly. ACCENT affects *which hits are louder than others* within the pattern. A VCA can make everything quieter, but it can't add ghost notes.

### Implementation Sketch

```cpp
// Called AFTER hit placement is determined (weight is for velocity, not probability)
float ComputeHitVelocity(float hitWeight, float accent, uint32_t seed) {
    if (accent < 0.01f) {
        // Flat dynamics - all hits equal
        return 1.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // Use the hit's musical weight to determine accent
    // High weight hits = accents, low weight hits = ghost notes
    // This works with SHAPE - irregular patterns still get musical accents
    // ═══════════════════════════════════════════════════════════════

    // Map weight (0.0-1.0) to velocity range based on ACCENT amount
    float minVelocity = 1.0f - (0.7f * accent);  // At accent=100%, min=0.3
    float maxVelocity = 1.0f;

    float velocity = minVelocity + hitWeight * (maxVelocity - minVelocity);

    // Add slight humanization
    float humanize = (HashToFloat(seed) - 0.5f) * 0.1f * accent;
    velocity += humanize;

    return Clamp(velocity, 0.1f, 1.0f);
}
```

**Key insight**: The pattern generator already assigns musical weights to hits (strong beats get high weight, weak beats get low weight). ACCENT just expands or compresses this into velocity range. This works correctly with both regular (low SHAPE) and irregular (high SHAPE) patterns.

### Interaction with Other Parameters

- **ENERGY**: More hits = more opportunities for velocity variation
- **SHAPE**: Irregular patterns may have less predictable accent placement
- **AXIS Y**: Complex patterns benefit from dynamic accents

---

## LENGTH: Compile-Time Default + Boot-Time Override

### Rationale

With CV-controllable ENERGY, SHAPE, AXIS X/Y, and DRIFT providing phrase evolution, variable pattern length adds little runtime value:

1. CV modulation creates phrase variation without longer lengths
2. 16-step is the techno/IDM standard
3. DRIFT already provides phrase-to-phrase evolution
4. One less runtime control = simpler mental model

However, power users may want to experiment without recompiling.

### Implementation

```cpp
// config.h - compile-time default
#define DEFAULT_PATTERN_LENGTH 16

// Boot-time override (optional):
// Hold button during power-on, switch position selects length:
//   Switch UP:   16 steps (default)
//   Switch DOWN: 24 steps (6/8 / triplet feel)
//
// LED confirms with N flashes (1 flash = 16, 2 flashes = 24)
```

### Boot-Time Config Gesture

```cpp
void CheckBootConfig() {
    if (ButtonHeldAtBoot()) {
        if (switchPosition == UP) {
            patternLength = 16;
            FlashLED(1);  // Single flash
        } else {
            patternLength = 24;
            FlashLED(2);  // Double flash
        }
        WaitForButtonRelease();
    } else {
        patternLength = DEFAULT_PATTERN_LENGTH;
    }
}
```

### Why Not Runtime?

- LENGTH changes require pattern regeneration
- Mid-performance LENGTH changes would be jarring
- Boot-time is "set and forget" - matches config philosophy
- Compile-time still available for 32/64 step builds

---

## Button Behavior (Updated)

| Gesture | Performance Mode | Config Mode |
|---------|------------------|-------------|
| Tap (>20ms) | Trigger Fill | (no action) |
| Hold 3s | Reseed on release | Reseed on release |
| Hold + Switch UP | HAT mode | HAT mode |
| Hold + Switch DOWN | FILL GATE mode | FILL GATE mode |

### Fill Behavior

Fill triggers based on current AUX MODE:
- **FILL GATE mode**: AUX output goes high for fill duration
- **HAT mode**: AUX output produces hat burst (pattern-aware, follows ENERGY/SHAPE)

---

## Complete Specification Summary

### Hardware I/O

| I/O | Function |
|-----|----------|
| CV1-4 | Modulate K1-4 (ENERGY, SHAPE, AXIS X, AXIS Y) |
| Gate Out 1 | Voice 1 triggers |
| Gate Out 2 | Voice 2 triggers |
| AUX Out | Fill gate OR hat burst (based on AUX MODE) |
| Clock In | External clock (normalled to internal) |
| Button | Fill / Reseed / AUX mode gesture |
| Switch | Mode select (up=Perf, down=Config) |
| LED | Status feedback |

### Control Count

| Mode | Knobs | Shift Functions | Total |
|------|-------|-----------------|-------|
| Performance | 4 | 0 | 4 |
| Config | 4 | 0 | 4 |
| **Total** | **8** | **0** | **8** |

Plus: 1 button gesture (Hold+Switch) for AUX MODE

### Algorithms (Unchanged from Iteration 7)

| Algorithm | Approach |
|-----------|----------|
| SHAPE blending | Deterministic score interpolation |
| AXIS biasing | Additive offsets |
| Voice relationship | COMPLEMENT + DRIFT variation |
| Hat burst | Pattern-aware, pre-allocated |
| Fill | Multi-attribute boost |

### LED Behaviors (Unchanged from Iteration 7)

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
| **AUX mode unlock** | **Triple rising flash** | **5 (replace)** |
| **AUX mode reset** | **Single fade** | **5 (replace)** |

---

## Implementation Checklist

### Critical (Must Do)

- [ ] Pre-allocate HatBurst arrays (no heap in audio path)
- [ ] Use deterministic score interpolation for SHAPE
- [ ] Use additive biasing for AXIS X/Y
- [ ] Implement LED layering as specified
- [ ] Layer trigger feedback on top of fills
- [ ] Implement Hold+Switch gesture for AUX MODE
- [ ] Implement ACCENT parameter for velocity variation

### Important (Should Do)

- [ ] COMPLEMENT relationship with DRIFT variation
- [ ] 70% velocity reduction for hat burst near main hits
- [ ] ACCENT affects velocity based on step position
- [ ] Make PATTERN_LENGTH a compile-time constant

### Deferred (Future)

- [ ] Extensibility framework (when bassline/ambient designed)
- [ ] Additional voice relationships (if user feedback indicates need)

---

## CV Behavior Clarification

CV inputs (CV1-4) **always modulate Performance mode parameters** (ENERGY, SHAPE, AXIS X, AXIS Y), regardless of whether the module is in Performance or Config mode.

- In Performance mode: CV modulates the same parameters you're adjusting with knobs
- In Config mode: CV still modulates performance parameters in the background (pattern evolves while you adjust config)

This allows CV sequences to keep running while you tweak config settings.

---

## Open Items

1. **CV slew/response** - Instant or slewed? (Suggest: instant with per-phrase regeneration)
2. **Power-on state** - Fixed seed or random? (Suggest: fixed default seed)
3. **AUX MODE on boot** - Always defaults to FILL GATE (volatile, not persisted)
