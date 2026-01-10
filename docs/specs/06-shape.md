# SHAPE Algorithm

[← Back to Index](00-index.md) | [← Control System](05-controls.md)

---

SHAPE (K2) controls pattern regularity through THREE character zones with crossfade transitions.

## 1. Zone Overview

| Zone | SHAPE Range | Character |
|------|-------------|-----------|
| **Stable** | 0-30% | Humanized euclidean, techno, four-on-floor |
| **Syncopated** | 30-70% | Funk, displaced, tension |
| **Wild** | 70-100% | IDM, chaos, weighted random |

---

## 2. Three-Way Blending

```
SHAPE  0%          30%          50%          70%         100%
       |--- Stable --|-- Crossfade --|-- Syncopated --|-- Crossfade --|-- Wild --|
                   (4%)                              (4%)
```

Crossfade windows (4% overlap) prevent discontinuities at zone boundaries.

---

## 3. Algorithm

```
FOR each step:
  IF shape < 0.28:
    // Pure Zone 1: stable with humanization
    weight = stableWeight[step]
    humanize = 0.05 * (1.0 - shape/0.28)
    weight += (hash(seed, step) - 0.5) * humanize

  ELSE IF shape < 0.32:
    // Crossfade Zone 1 → Zone 2
    fade = (shape - 0.28) / 0.04
    weight = lerp(stableWeight, syncopatedWeight, fade)

  ELSE IF shape < 0.68:
    // Zone 2: stable ↔ syncopated ↔ wild
    t = (shape - 0.32) / 0.36
    IF t < 0.5:
      weight = lerp(stableWeight, syncopatedWeight, t * 2)
    ELSE:
      weight = lerp(syncopatedWeight, wildWeight, (t - 0.5) * 2)

  ELSE IF shape < 0.72:
    // Crossfade Zone 2 → Zone 3
    fade = (shape - 0.68) / 0.04
    weight = lerp(syncopatedWeight, wildWeight, fade)

  ELSE:
    // Pure Zone 3: wild with chaos
    weight = wildWeight[step]
    chaos = (shape - 0.72) / 0.28 * 0.15
    weight += (hash(seed ^ 0xCAFEBABE, step) - 0.5) * chaos
```

---

## 4. SHAPE-Modulated Hit Budget

SHAPE also affects anchor/shimmer hit ratio:

| Zone | Anchor Budget | Shimmer Budget |
|------|---------------|----------------|
| Stable (0-30%) | 100% of base | 100% of base |
| Syncopated (30-70%) | 90-100% | 110-130% |
| Wild (70-100%) | 80-90% | 130-150% |

---

[Next: AXIS Biasing →](07-axis.md)
