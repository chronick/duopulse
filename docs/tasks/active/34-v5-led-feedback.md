---
id: 34
slug: 34-v5-led-feedback
title: "V5 LED Feedback System Update"
status: pending
created_date: 2026-01-04
updated_date: 2026-01-04
branch: feature/34-v5-led-feedback
spec_refs:
  - "v5-design-final.md#led-feedback-summary"
depends_on:
  - 27
---

# Task 34: V5 LED Feedback System Update

## Objective

Update the LED feedback system to support V5's layered feedback model with proper priority handling for all new states.

## Context

V5 specifies a 5-layer LED feedback system where higher layers can override or blend with lower layers:

| Layer | State | Behavior |
|-------|-------|----------|
| 1 (base) | Idle (perf) | Gentle breath synced to clock |
| 1 (base) | Idle (config) | Slower breath |
| 2 (additive) | Clock sync | Subtle pulse on beats |
| 3 (maximum) | Trigger activity | Pulse on hits, envelope decay |
| 4 (maximum) | Fill active | Accelerating strobe + trigger overlay |
| 5 (replace) | Reseed progress | Building pulse (1-5Hz over 3s) |
| 5 (replace) | Reseed confirm | POP POP POP (3 flashes) |
| 5 (replace) | Mode switch | Quick signature |
| 5 (replace) | AUX mode unlock | Triple rising flash |
| 5 (replace) | AUX mode reset | Single fade |

## Subtasks

- [ ] Implement 5-layer priority system in LedIndicator
- [ ] Add breath animation for idle states (Layer 1)
- [ ] Add clock sync pulse (Layer 2, additive)
- [ ] Update trigger activity with envelope decay (Layer 3)
- [ ] Add accelerating strobe for fill with trigger overlay (Layer 4)
- [ ] Add reseed progress animation (building pulse, Layer 5)
- [ ] Add reseed confirm animation (3 flashes, Layer 5)
- [ ] Add mode switch signature (Layer 5)
- [ ] Add AUX mode unlock (triple rising flash, Layer 5)
- [ ] Add AUX mode reset (single fade, Layer 5)
- [ ] Implement layer blending rules (replace vs additive vs maximum)
- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] Idle states show gentle breathing animation
- [ ] Triggers pulse and decay smoothly
- [ ] Fills show accelerating strobe with trigger overlay
- [ ] Reseed shows 3s building pulse then 3 confirmation flashes
- [ ] AUX mode unlock shows triple rising flash
- [ ] Layer 5 events properly interrupt lower layers
- [ ] Build compiles without errors
- [ ] All tests pass

## Implementation Notes

### Layer Priority System

```cpp
enum class LedLayer : uint8_t {
    BASE = 1,       // Breath animations
    ADDITIVE = 2,   // Clock sync
    MAXIMUM = 3,    // Triggers (take max of layers 1-3)
    FILL = 4,       // Fill strobe (take max of layers 1-4)
    REPLACE = 5     // Overrides everything
};

struct LedState {
    float brightness;
    LedLayer layer;
    uint32_t expiresAtMs;  // For timed events
};

class LedIndicator {
    LedState layers_[5];  // One per layer

    float ComputeFinalBrightness() {
        // Check for active replace layer first
        if (layers_[4].expiresAtMs > currentTimeMs_) {
            return layers_[4].brightness;
        }

        // For layers 1-4, use maximum blending
        float result = 0.0f;
        for (int i = 0; i < 4; ++i) {
            if (layers_[i].expiresAtMs > currentTimeMs_) {
                result = Max(result, layers_[i].brightness);
            }
        }
        return result;
    }
};
```

### Breath Animation

```cpp
void UpdateBreathAnimation(float bpm, bool isConfigMode) {
    // Slower breath in config mode
    float breathRate = isConfigMode ? 0.25f : 0.5f;  // Hz

    // Sync to clock
    float phase = fmodf(clockPhase_ * breathRate, 1.0f);

    // Smooth sine wave
    float brightness = 0.3f + 0.2f * sinf(phase * 2.0f * M_PI);

    SetLayer(LedLayer::BASE, brightness, UINT32_MAX);  // Never expires
}
```

### Accelerating Fill Strobe

```cpp
void UpdateFillStrobe(float fillProgress) {
    // Strobe rate accelerates: 2Hz at start, 8Hz at end
    float strobeRate = 2.0f + fillProgress * 6.0f;

    float phase = fmodf(fillProgress * strobeRate, 1.0f);
    float strobeBrightness = phase < 0.5f ? 1.0f : 0.0f;

    SetLayer(LedLayer::FILL, strobeBrightness, UINT32_MAX);
}
```

### AUX Mode Feedback

```cpp
void QueueAuxModeUnlock() {
    // Triple rising flash: ∙∙∙ ● ● ●
    QueueFlash(0.33f, 50);   // Dim
    QueueFlash(0.66f, 50);   // Medium
    QueueFlash(1.00f, 100);  // Bright
}

void QueueAuxModeReset() {
    // Single fade: ● ∙
    QueueFade(1.0f, 0.0f, 200);
}
```

### Files to Modify

- `src/Engine/LedIndicator.cpp` - Implement layer system and animations
- `src/Engine/LedIndicator.h` - Add layer enum and state struct
- `src/main.cpp` - Wire new LED states to events
- `tests/LedIndicatorTest.cpp` - Add layer priority tests

### Constraints

- LED updates must be non-blocking
- Animations computed per-frame (no timers)
- Layer state fits in small struct

### Risks

- Breath animation timing with external clock sync
- Visual distinction between states may need tuning
