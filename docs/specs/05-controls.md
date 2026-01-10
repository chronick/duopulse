# Control System

[← Back to Index](00-index.md) | [← Hardware Interface](04-hardware.md)

---

## 1. Mode Overview

| Mode | Switch Position | Knob Behavior |
|------|-----------------|---------------|
| **Performance** | UP | ENERGY / SHAPE / AXIS X / AXIS Y |
| **Config** | DOWN | CLOCK DIV / SWING / DRIFT / ACCENT |

**No shift layers exist.** Each mode has 4 direct parameters.

---

## 2. Performance Mode Parameters

| Knob | Parameter | Range | Function |
|------|-----------|-------|----------|
| K1 | **ENERGY** | 0-100% | Hit density: how many hits per bar |
| K2 | **SHAPE** | 0-100% | Pattern character: stable → syncopated → wild |
| K3 | **AXIS X** | 0-100% | Beat position: grounded (downbeats) → floating (offbeats) |
| K4 | **AXIS Y** | 0-100% | Intricacy: simple → complex |

---

## 3. Config Mode Parameters

| Knob | Parameter | Range | Function |
|------|-----------|-------|----------|
| K1 | **CLOCK DIV** | ÷4 to ×4 | Clock division/multiplication |
| K2 | **SWING** | 0-100% | Timing feel: straight → heavy swing |
| K3 | **DRIFT** | 0-100% | Evolution: locked → varies each phrase |
| K4 | **ACCENT** | 0-100% | Velocity range: flat → dynamic |

---

## 4. Button Gestures

| Gesture | Effect |
|---------|--------|
| **Tap** (20-500ms) | Trigger Fill |
| **Hold 3s** | Reseed pattern on release |
| **Hold + Switch UP** | Set AUX to HAT mode (secret "2.5 pulse") |
| **Hold + Switch DOWN** | Set AUX to FILL GATE mode (default) |

---

## 5. AUX Mode Gesture Priority

When button is held and switch moves:
1. Cancel any pending fill
2. Set AUX mode based on switch direction
3. Consume switch event (don't change Perf/Config mode)
4. Button release returns to normal (no fill triggered)

---

[Next: SHAPE Algorithm →](06-shape.md)
