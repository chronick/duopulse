# Testing Requirements

[← Back to Index](00-index.md) | [← Clock and Reset](14-clock.md)

---

## 1. Unit Tests

| Test Area | Tests Required |
|-----------|----------------|
| **SHAPE Blending** | Zone transitions, crossfade smoothness, determinism |
| **AXIS Biasing** | Bidirectional effect, no dead zones, broken mode |
| **COMPLEMENT** | Gap finding, wrap-around handling, DRIFT placement |
| **ACCENT Velocity** | Range scaling, metric weight mapping, micro-variation |
| **Hat Burst** | Pre-allocation, collision detection, velocity ducking |
| **LED Layers** | Priority ordering, layer composition |

---

## 2. Integration Tests

| Test | Description |
|------|-------------|
| **Full Bar Generation** | Generate 100 bars, verify hit counts and patterns |
| **SHAPE Sweep** | Sweep 0-100%, verify smooth character transitions |
| **AXIS Navigation** | Full X/Y range, verify continuous effect |
| **Fill Behavior** | Button tap, CV trigger, AUX output |
| **Boot Defaults** | Power cycle, verify all parameters reset |

---

## 3. Musical Validation

| Settings | Expected Outcome |
|----------|------------------|
| ENERGY=45%, SHAPE=25% | Driving techno, stable groove |
| ENERGY=55%, SHAPE=50%, AXIS X=65% | Groovy funk, syncopated |
| ENERGY=65%, SHAPE=60%, AXIS X=75% | Broken beat mode |
| ENERGY=85%, SHAPE=90% | IDM chaos |
| DRIFT=0% | Identical pattern every phrase |
| DRIFT=100% | Varied placement, same density feel |

---

[Next: Style Presets →](A1-presets.md)
