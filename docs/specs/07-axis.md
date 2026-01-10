# AXIS Biasing

[← Back to Index](00-index.md) | [← SHAPE Algorithm](06-shape.md)

---

AXIS X/Y provide continuous control with bidirectional effect (no dead zones).

## 1. AXIS X: Beat Position

```
xBias = (axisX - 0.5) * 2.0  // [-1.0, 1.0]

FOR each step:
  positionStrength = GetPositionStrength(step)  // negative=downbeat, positive=offbeat

  IF xBias > 0:
    // Moving toward offbeats
    IF positionStrength < 0:
      xEffect = -xBias * |positionStrength| * 0.45  // Suppress downbeats
    ELSE:
      xEffect = xBias * positionStrength * 0.60     // Boost offbeats
  ELSE:
    // Moving toward downbeats
    IF positionStrength < 0:
      xEffect = -xBias * |positionStrength| * 0.60  // Boost downbeats
    ELSE:
      xEffect = xBias * positionStrength * 0.45     // Suppress offbeats

  weight[step] = clamp(baseWeight[step] + xEffect, 0.05, 1.0)
```

---

## 2. AXIS Y: Intricacy

```
yBias = (axisY - 0.5) * 2.0  // [-1.0, 1.0]

FOR each step:
  isWeakPosition = GetMetricWeight(step) < 0.5

  IF yBias > 0:
    yEffect = yBias * (isWeakPosition ? 0.50 : 0.15)   // Boost weak positions
  ELSE:
    yEffect = yBias * (isWeakPosition ? 0.50 : -0.25)  // Suppress weak positions

  weight[step] = clamp(weight[step] + yEffect, 0.05, 1.0)
```

---

## 3. Broken Mode (High SHAPE + High AXIS X)

When SHAPE > 60% AND AXIS X > 70%, enter "broken" mode:

```
brokenIntensity = (shape - 0.6) * 2.5 * (axisX - 0.7) * 3.33

FOR step IN downbeatPositions:
  IF hash(seed ^ 0xDEADBEEF, step) < brokenIntensity * 0.6:
    weight[step] *= 0.25  // Suppress some downbeats completely
```

---

[Next: Voice Relationship →](08-complement.md)
