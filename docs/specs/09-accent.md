# ACCENT Velocity

[← Back to Index](00-index.md) | [← Voice Relationship](08-complement.md)

---

ACCENT (Config K4) controls velocity range based on metric position.

## 1. Velocity Computation

```
metricWeight = GetMetricWeight(step)  // 0.0-1.0

// Velocity range scales with ACCENT
velocityFloor = 0.80 - accent * 0.50    // 80% → 30%
velocityCeiling = 0.88 + accent * 0.12  // 88% → 100%

// Map metric weight to velocity
velocity = velocityFloor + metricWeight * (velocityCeiling - velocityFloor)

// Micro-variation for human feel
variation = 0.02 + accent * 0.05
velocity += (hash(seed, step) - 0.5) * variation

RETURN clamp(velocity, 0.30, 1.0)
```

---

## 2. ACCENT Effect

| ACCENT | Velocity Floor | Velocity Ceiling | Feel |
|--------|----------------|------------------|------|
| 0% | 80% | 88% | Flat, machine-like |
| 50% | 55% | 94% | Normal dynamics |
| 100% | 30% | 100% | Extreme ghost/accent contrast |

---

[Next: Fill System →](10-fill.md)
