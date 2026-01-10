# AUX Output System

[← Back to Index](00-index.md) | [← Fill System](10-fill.md)

---

## 1. AUX Modes

| Mode | Signal | Description |
|------|--------|-------------|
| **FILL GATE** (default) | Gate | HIGH during fill duration |
| **HAT** (secret) | Trigger | Pattern-aware hat burst during fill |

---

## 2. Hat Burst Generation

```
// Determine trigger count (2-12)
triggerCount = max(1, round(2 + energy * 10))

FOR i = 0 TO triggerCount - 1:
  IF burst.count >= 12: BREAK

  IF shape < 0.3:
    step = (i * fillDuration) / triggerCount  // Evenly spaced
  ELSE IF shape < 0.7:
    step = EuclideanWithJitter(i, fillDuration, shape, seed)
  ELSE:
    step = RandomStep(fillDuration, seed, i)

  // Collision detection: nudge to nearest empty
  IF step IN usedSteps:
    step = FindNearestEmpty(step, fillDuration, usedSteps)

  burst.triggers[burst.count++] = step

// Velocity ducking near main hits
FOR each trigger:
  nearMainHit = CheckProximity(trigger.step, mainPattern, 1)
  baseVelocity = 0.65 + 0.35 * energy
  trigger.velocity = nearMainHit ? baseVelocity * 0.30 : baseVelocity
```

---

## 3. Hat Burst Data Structure

```cpp
struct HatBurst {
    struct Trigger { uint8_t step; float velocity; };
    Trigger triggers[12];  // Pre-allocated, no heap
    uint8_t count, fillStart, fillDuration, _pad;
};
```

---

[Next: LED Feedback →](12-led.md)
