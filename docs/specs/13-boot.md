# Boot Behavior

[← Back to Index](00-index.md) | [← LED Feedback](12-led.md)

---

## 1. Boot Defaults

All parameters reset to musical defaults on power-on:

| Parameter | Default | Rationale |
|-----------|---------|-----------|
| ENERGY | 50% | Neutral density |
| SHAPE | 30% | Humanized euclidean zone |
| AXIS X | 50% | Neutral beat position |
| AXIS Y | 50% | Moderate intricacy |
| CLOCK DIV | ×1 | No division |
| SWING | 50% | Neutral |
| DRIFT | 0% | Locked pattern |
| ACCENT | 50% | Normal dynamics |
| AUX MODE | FILL GATE | Default (no HAT) |
| PATTERN LENGTH | 16 | Compile-time default |

---

## 2. Boot-Time AUX Mode Selection

Optional gesture during power-on:
- Hold button during power-on
- Switch UP → HAT mode (triple rising flash)
- Switch DOWN → FILL GATE mode (single fade)

AUX mode is **volatile** (not persisted across power cycles).

---

## 3. Performance Knob Reading

On boot, performance knobs (K1-K4) are read from hardware immediately:
- No soft takeover needed
- Values apply directly
- Provides immediate playability

---

[Next: Clock and Reset →](14-clock.md)
