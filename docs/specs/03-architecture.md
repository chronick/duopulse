# Architecture Overview

[← Back to Index](00-index.md) | [← Core Concepts](02-core-concepts.md)

---

## 1. High-Level Pipeline

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           CONTROL LAYER                                  │
│                                                                          │
│   ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐                    │
│   │ ENERGY  │  │  SHAPE  │  │ AXIS X  │  │ AXIS Y  │                    │
│   │  (K1)   │  │  (K2)   │  │  (K3)   │  │  (K4)   │                    │
│   └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘                    │
│        │            │            │            │                          │
│        ▼            ▼            └──────┬─────┘                          │
│   ┌─────────┐  ┌─────────────┐         │                                │
│   │   HIT   │  │  3-WAY      │         ▼                                │
│   │  BUDGET │  │  BLENDING   │  ┌────────────────┐                      │
│   └────┬────┘  └──────┬──────┘  │ AXIS BIASING   │                      │
│        │              │         └───────┬────────┘                      │
└────────┼──────────────┼─────────────────┼───────────────────────────────┘
         │              │                 │
         ▼              ▼                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         GENERATION LAYER                                 │
│                                                                          │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                     WEIGHTED SAMPLING                               │ │
│  │  (Gumbel Top-K with SHAPE-blended weights + AXIS-biased scores)    │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                              │                                           │
│                              ▼                                           │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                   COMPLEMENT RELATIONSHIP                           │ │
│  │  (Voice 2 fills gaps in Voice 1, placement varies with DRIFT)       │ │
│  └────────────────────────────────────────────────────────────────────┘ │
│                              │                                           │
│                              ▼                                           │
│  ┌────────────────────────────────────────────────────────────────────┐ │
│  │                    ACCENT VELOCITY                                  │ │
│  │  (Metric weight → velocity, range scaled by ACCENT param)           │ │
│  └────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          OUTPUT LAYER                                    │
│                                                                          │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │ GATE 1  │ │ GATE 2  │ │ OUT L   │ │ OUT R   │ │  CV 1   │           │
│  │ Voice 1 │ │ Voice 2 │ │ V1 Vel  │ │ V2 Vel  │ │   AUX   │           │
│  │  Trig   │ │  Trig   │ │  (S&H)  │ │  (S&H)  │ │         │           │
│  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘           │
│                                                                          │
│  CV_OUT_2 = LED (visual feedback)                                        │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Processing Flow Per Bar

1. **Compute hit budgets** from ENERGY (SHAPE modulates anchor/shimmer ratio)
2. **Generate SHAPE-blended weights** (stable ↔ syncopated ↔ wild)
3. **Apply AXIS biasing** (X = beat position, Y = intricacy)
4. **Select hits via Gumbel Top-K** sampling
5. **Apply COMPLEMENT** for Voice 2 (gap-filling with DRIFT variation)
6. **Compute velocities** from ACCENT + metric position
7. **Store hit masks** for the bar
8. **On each step**: apply SWING timing, output triggers + velocities

---

[Next: Hardware Interface →](04-hardware.md)
