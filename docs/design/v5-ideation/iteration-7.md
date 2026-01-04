# V5 Design Iteration 7: Final Specification

**Date**: 2026-01-04
**Status**: Implementation-Ready
**Focus**: Simplified voice relationship, fixed critical issues, deferred extensibility

---

## Changes from Iteration 6

| Aspect | Iteration 6 | Iteration 7 |
|--------|-------------|-------------|
| Voice relationship | 5 modes + auto-selection from AXIS Y | **COMPLEMENT only + DRIFT variation** |
| LED layering | Undefined | **Explicit layering math** |
| Fill LED | Suppresses trigger feedback | **Layers trigger on top** |
| Hat burst allocation | `new float[]` (heap) | **Pre-allocated arrays** |
| VariationProfile | Full struct + 3 profiles | **Extensibility notes only** |
| Hat velocity reduction | 50% | **70%** (less aggressive) |

---

## V5 Control Summary (Unchanged)

### Performance Mode (Zero Shift)

```
K1: ENERGY    K2: SHAPE     K3: AXIS X    K4: AXIS Y
CV1           CV2           CV3           CV4
```

### Config Mode (1 Shift)

```
K1: CLOCK DIV   K2: SWING   K3: DRIFT     K4: LENGTH
                            +S: AUX MODE
```

### Knob Pairing (Perf ↔ Config)

| Knob | Performance | Config | Domain |
|------|-------------|--------|--------|
| K1 | ENERGY | CLOCK DIV | Rate/Density |
| K2 | SHAPE | SWING | Timing Feel |
| K3 | AXIS X | DRIFT | Variation |
| K4 | AXIS Y | LENGTH | Structure |

---

## Voice 2 Relationship: COMPLEMENT + DRIFT (Simplified)

### Design Decision

**Removed**: Auto-selection based on AXIS Y
**Kept**: Single COMPLEMENT mode with DRIFT-controlled variation

### Why COMPLEMENT

COMPLEMENT is the ideal call/response relationship:
- Voice 2 fills gaps left by Voice 1
- Creates natural interplay without competition
- Predictable behavior - no hidden mode switching

### Implementation

```cpp
void ApplyVoiceRelationship(Pattern& v1, Pattern& v2, float drift) {
    // ═══════════════════════════════════════════════════════════════
    // COMPLEMENT: Voice 2 fills Voice 1's gaps
    // ═══════════════════════════════════════════════════════════════

    for (int step = 0; step < patternLength; step++) {
        if (v1.hits[step]) {
            // Suppress Voice 2 where Voice 1 hits
            v2.weight[step] *= 0.1f;

            // Boost Voice 2 on adjacent steps (fill the gaps)
            int prev = (step - 1 + patternLength) % patternLength;
            int next = (step + 1) % patternLength;
            v2.weight[prev] += 0.15f;
            v2.weight[next] += 0.15f;
        } else {
            // Boost Voice 2 in Voice 1's empty spaces
            v2.weight[step] += 0.1f;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DRIFT adds phrase-to-phrase variation to relationship
    // ═══════════════════════════════════════════════════════════════

    if (drift > 0.1f) {
        // Vary the suppression/boost amounts slightly each phrase
        float variationAmount = drift * 0.3f;  // 0% to 30% variation

        for (int step = 0; step < patternLength; step++) {
            float phraseNoise = HashToFloat(HashStep(phraseSeed, step));
            float variation = (phraseNoise - 0.5f) * 2.0f * variationAmount;

            v2.weight[step] *= (1.0f + variation);
            v2.weight[step] = Clamp(v2.weight[step], 0.0f, 1.5f);
        }
    }
}
```

### Result

- **DRIFT = 0%**: Voice 2 always complements Voice 1 identically
- **DRIFT = 50%**: Subtle variation in how Voice 2 responds each phrase
- **DRIFT = 100%**: Noticeable variation while maintaining complement relationship

---

## LED Feedback (Fixed Layering)

### LED Layering Math (Explicit)

```cpp
void UpdateLED() {
    float brightness = 0.0f;

    // ═══════════════════════════════════════════════════════════════
    // LAYER 1: Base state (lowest priority)
    // ═══════════════════════════════════════════════════════════════

    if (mode == CONFIG) {
        brightness = GetConfigIdleBreath();  // Slow breath, DIM-MEDIUM
    } else {
        brightness = GetPerformanceIdleBreath();  // Faster breath, DIM
    }

    // ═══════════════════════════════════════════════════════════════
    // LAYER 2: Clock sync accents (ADDITIVE)
    // ═══════════════════════════════════════════════════════════════

    if (isDownbeat) {
        brightness += 0.25f;  // Visible bump on bar start
    } else if (isQuarterNote) {
        brightness += 0.08f;  // Subtle quarter pulse
    }

    // ═══════════════════════════════════════════════════════════════
    // LAYER 3: Trigger activity (MAXIMUM - envelope follower)
    // ═══════════════════════════════════════════════════════════════

    if (triggerEnvelope.active) {
        brightness = fmaxf(brightness, triggerEnvelope.value);
    }

    // ═══════════════════════════════════════════════════════════════
    // LAYER 4: Fill effect (ADDITIVE on top of triggers)
    // ═══════════════════════════════════════════════════════════════

    if (fill.active) {
        float fillEffect = ComputeFillBrightness(fill.progress, fill.intensity);

        // Layer trigger feedback ON TOP of fill (don't suppress)
        if (triggerEnvelope.active) {
            fillEffect += triggerEnvelope.value * 0.3f;  // Subtle trigger overlay
        }

        brightness = fmaxf(brightness, fillEffect);
    }

    // ═══════════════════════════════════════════════════════════════
    // LAYER 5: High-priority overrides (REPLACE completely)
    // ═══════════════════════════════════════════════════════════════

    if (reseedPending) {
        brightness = ComputeReseedProgress(holdDuration);  // Building pulse
    }

    if (reseedConfirm) {
        brightness = GetReseedFlashBrightness();  // POP POP POP
    }

    if (modeJustSwitched) {
        brightness = GetModeSwitchBrightness();  // Quick signature
    }

    // ═══════════════════════════════════════════════════════════════
    // OUTPUT
    // ═══════════════════════════════════════════════════════════════

    led.SetBrightness(Clamp(brightness, 0.0f, 1.0f) * LED_MAX_VOLTAGE);
}
```

### Layering Summary

| Layer | Behavior | Blending |
|-------|----------|----------|
| 1 | Idle breath (perf/config) | Base |
| 2 | Clock sync accents | Additive (+) |
| 3 | Trigger activity | Maximum |
| 4 | Fill effect | Maximum + trigger overlay |
| 5 | Reseed/Mode switch | Replace |

---

## Hat Burst (Fixed Allocation)

### Pre-allocated Structure

```cpp
struct HatBurst {
    static constexpr int MAX_TRIGGERS = 12;

    int triggerCount;
    float triggerTimes[MAX_TRIGGERS];   // Pre-allocated
    float velocities[MAX_TRIGGERS];     // Pre-allocated
    bool scheduled;
    int currentIndex;
};

// Single static instance - no heap allocation
static HatBurst hatBurst;
```

### Generation (No Heap)

```cpp
void GenerateHatBurst(float energy, float shape, float burstDurationMs,
                      const Pattern& mainPattern, int currentStep) {

    // ═══════════════════════════════════════════════════════════════
    // ENERGY determines burst density (uses pre-allocated arrays)
    // ═══════════════════════════════════════════════════════════════

    int minTriggers = 2;
    int maxTriggers = HatBurst::MAX_TRIGGERS;
    hatBurst.triggerCount = minTriggers + (int)(energy * (maxTriggers - minTriggers));

    float intervalBase = burstDurationMs / hatBurst.triggerCount;

    for (int i = 0; i < hatBurst.triggerCount; i++) {
        // ═══════════════════════════════════════════════════════════════
        // SHAPE determines regularity
        // ═══════════════════════════════════════════════════════════════

        if (shape < 0.3f) {
            // Regular spacing
            hatBurst.triggerTimes[i] = i * intervalBase;
        }
        else if (shape < 0.7f) {
            // Slight variation
            float jitter = (HashToFloat(i) - 0.5f) * intervalBase * 0.3f * shape;
            hatBurst.triggerTimes[i] = i * intervalBase + jitter;
        }
        else {
            // Irregular/clustered
            float t = (float)i / hatBurst.triggerCount;
            float curve = powf(t, 0.5f + shape);
            hatBurst.triggerTimes[i] = curve * burstDurationMs;

            float jitter = (HashToFloat(i + 100) - 0.5f) * intervalBase * 0.5f;
            hatBurst.triggerTimes[i] += jitter;
        }

        // ═══════════════════════════════════════════════════════════════
        // Velocity: accented start, decay toward end
        // ═══════════════════════════════════════════════════════════════

        float position = (float)i / hatBurst.triggerCount;
        hatBurst.velocities[i] = 1.0f - position * 0.4f;

        // Add SHAPE-based variation
        hatBurst.velocities[i] += (HashToFloat(i + 200) - 0.5f) * shape * 0.3f;
        hatBurst.velocities[i] = Clamp(hatBurst.velocities[i], 0.3f, 1.0f);
    }

    // ═══════════════════════════════════════════════════════════════
    // Pattern sensitivity: reduce velocity near main hits (70%, not 50%)
    // ═══════════════════════════════════════════════════════════════

    AdjustBurstForMainPattern(mainPattern, currentStep);

    hatBurst.scheduled = true;
    hatBurst.currentIndex = 0;
}

void AdjustBurstForMainPattern(const Pattern& mainPattern, int currentStep) {
    float stepDurationMs = 60000.0f / bpm / 4.0f;

    for (int i = 0; i < hatBurst.triggerCount; i++) {
        float burstTimeMs = hatBurst.triggerTimes[i];
        int nearestStep = currentStep + (int)roundf(burstTimeMs / stepDurationMs);
        nearestStep = nearestStep % patternLength;

        // Check if main pattern has a hit near this burst trigger
        bool nearMainHit = mainPattern.v1.hits[nearestStep] ||
                           mainPattern.v2.hits[nearestStep] ||
                           mainPattern.v1.hits[(nearestStep + 1) % patternLength] ||
                           mainPattern.v2.hits[(nearestStep + 1) % patternLength];

        if (nearMainHit) {
            // Reduce velocity to let main hit punch through (70%, not 50%)
            hatBurst.velocities[i] *= 0.7f;
        }
    }
}
```

---

## Extensibility Notes (No Framework)

### Intent

Future firmware variations (bassline, ambient) will reinterpret the abstract controls differently. The current architecture supports this without a formal framework.

### Extension Points

When a non-percussive variation is designed:

1. **Control Interpretation**: Remap what ENERGY/SHAPE/AXIS X/AXIS Y mean
   - Bassline: ENERGY = note density, SHAPE = melodic contour, etc.

2. **Output Modes**: Change what the outputs produce
   - Bassline: Gate Out 2 could become pitch CV

3. **Pattern Generation**: Use a different generation algorithm
   - Ambient: Slow, evolving modulation instead of triggers

4. **Voice Relationship**: May use different relationship modes
   - Bassline: UNISON (both voices play same notes) or independent

### Implementation Guidance

```cpp
// Future: When bassline variation is designed, create:
// class BasslinePatternGenerator : public PatternGenerator { ... }
//
// The existing generation loop can call different generators
// based on a compile-time or boot-time variation selection.
//
// DO NOT implement VariationProfile struct until requirements are known.
```

---

## Button Behavior (Unchanged from Iteration 5)

```cpp
void ProcessButton(bool pressed, float holdDurationMs) {
    const float DEBOUNCE_MS = 20.0f;
    const float RESEED_THRESHOLD_MS = 3000.0f;

    if (!pressed) {
        if (state == RESEED_PENDING) {
            ReseedPattern();
            TriggerLEDReseedConfirm();  // POP POP POP
        } else if (state == FILL_ACTIVE) {
            EndFill();
        }
        state = IDLE;
        fillTriggered = false;
        return;
    }

    if (state == IDLE) {
        holdStartTime = currentTimeMs;
    }

    if (holdDurationMs >= RESEED_THRESHOLD_MS) {
        state = RESEED_PENDING;
        // LED: reseed ready indicator
    }
    else if (holdDurationMs > DEBOUNCE_MS) {
        if (!fillTriggered) {
            TriggerFill();  // Only once
            fillTriggered = true;
        }
        state = FILL_ACTIVE;
        // LED: fill active + progress toward reseed
    }
}
```

---

## Complete Specification Summary

### Performance Controls

| Param | Domain | 0% | 100% | CV |
|-------|--------|-----|------|-----|
| ENERGY | Density | Sparse | Busy | CV1 |
| SHAPE | Regularity | Stable (humanized euclidean) | Wild (weighted) | CV2 |
| AXIS X | Beat Position | Grounded (downbeats) | Floating (offbeats) | CV3 |
| AXIS Y | Intricacy | Simple | Complex | CV4 |

### Config Controls

| Param | Domain | Range | Shift |
|-------|--------|-------|-------|
| CLOCK DIV | Tempo | ÷4 to ×4 | - |
| SWING | Groove | 0-100% | - |
| DRIFT | Evolution | 0-100% | AUX MODE |
| LENGTH | Structure | 16/24/32/64 | - |

### Algorithms

| Algorithm | Status |
|-----------|--------|
| SHAPE blending | Deterministic score interpolation |
| AXIS biasing | Additive offsets |
| Voice relationship | COMPLEMENT + DRIFT variation |
| Hat burst | Pattern-aware, pre-allocated |
| DRIFT | Phrase seed mixing |
| Fill | Multi-attribute boost + aux output |

### LED Behaviors

| State | Behavior | Layer |
|-------|----------|-------|
| Idle (perf) | Gentle breath synced to clock | 1 (base) |
| Idle (config) | Slower breath | 1 (base) |
| Clock sync | Subtle pulse on beats | 2 (additive) |
| Trigger activity | Pulse on hits, envelope decay | 3 (maximum) |
| Fill active | Accelerating strobe + trigger overlay | 4 (maximum) |
| Reseed progress | Building pulse (1-5Hz over 3s) | 5 (replace) |
| Reseed confirm | POP POP POP (3 flashes) | 5 (replace) |
| Mode switch | Quick signature (tap-tap vs bloom) | 5 (replace) |

### Button

| Action | Effect |
|--------|--------|
| Tap | Fill (triggers hat burst or fill gate based on AUX MODE) |
| Hold 3s | Reseed pattern on release |

---

## Implementation Checklist

### Critical (Must Do)

- [ ] Pre-allocate HatBurst arrays (no heap in audio path)
- [ ] Use deterministic score interpolation for SHAPE
- [ ] Use additive biasing for AXIS X/Y
- [ ] Implement LED layering as specified
- [ ] Layer trigger feedback on top of fills

### Important (Should Do)

- [ ] COMPLEMENT relationship with DRIFT variation
- [ ] 70% velocity reduction for hat burst near main hits
- [ ] Use `roundf()` for step mapping in hat burst
- [ ] Check adjacent steps for pattern-awareness

### Deferred (Future)

- [ ] Extensibility framework (when bassline/ambient designed)
- [ ] Additional voice relationships (if user feedback indicates need)
- [ ] Config-mode relationship override (if users want explicit control)

---

## Open Items (Minimal)

1. **CV slew/response** - Instant or slewed? (Suggest: instant with per-phrase regeneration)
2. **Power-on state** - Fixed seed or random? (Suggest: fixed default seed, reseed to randomize)
3. **Hat burst overlap** - New fill cancels previous burst (suggest: replace, don't queue)
