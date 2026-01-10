# Fill System

[← Back to Index](00-index.md) | [← ACCENT Velocity](09-accent.md)

---

## 1. Fill Triggering

| Trigger | Behavior |
|---------|----------|
| Button tap (20-500ms) | Queue fill for next phrase |
| Fill CV (>1V) | Immediate fill |

---

## 2. Fill Modifiers

```
IF fillActive:
  // Exponential density curve
  maxBoost = 0.6 + energy * 0.4
  densityMultiplier = 1.0 + maxBoost * (fillProgress^2)

  // Velocity boost
  velocityBoost = 0.10 + 0.15 * fillProgress

  // Accent probability ramp
  accentProbability = 0.50 + 0.50 * fillProgress
  forceAccents = (fillProgress > 0.85)

  // Eligibility expansion
  expandEligibility = (fillProgress > 0.5)
```

---

[Next: AUX Output →](11-aux.md)
