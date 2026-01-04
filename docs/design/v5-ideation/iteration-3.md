# V5 Design Iteration 3: Parameter Domain Refinement

**Date**: 2026-01-03
**Focus**: Clean parameter separation and config mode restructuring

---

## Parameter Domains (Clarified)

Each parameter owns a distinct musical dimension:

| Parameter | Domain | Range | What it controls |
|-----------|--------|-------|------------------|
| **ENERGY** | Density | Sparse ↔ Busy | How many hits per bar |
| **SHAPE** | Regularity | Stable/on-grid ↔ Unstable/off-grid | How predictable the placement |
| **SEEK X** | Character | Grounded ↔ Floating | Downbeat vs offbeat emphasis |
| **SEEK Y** | Intricacy | Simple ↔ Complex | Pattern sophistication |

### Domain Boundaries (What Each Does NOT Control)

| Parameter | Explicitly NOT |
|-----------|----------------|
| ENERGY | Pattern character (that's SEEK) |
| SHAPE | Hit count (that's ENERGY) |
| SEEK X | Density (that's ENERGY) |
| SEEK Y | Density (that's ENERGY), Regularity (that's SHAPE) |

---

## V5 Performance Mode (Final)

```
┌─────────────────────────────────────────────────────────────┐
│  K1: ENERGY      K2: SHAPE        K3: SEEK X     K4: SEEK Y │
│  CV1             CV2              CV3            CV4        │
│                                                             │
│  Sparse ↔ Busy   Stable ↔ Wild    Grounded ↔     Simple ↔   │
│  (density)       (regularity)     Floating       Complex    │
└─────────────────────────────────────────────────────────────┘
```

**No shift layer. CV 1-4 map directly.**

### Visual: How Parameters Interact

```
                              SHAPE
                    Stable              Wild
                    (on-grid)           (off-grid)
                       │                    │
ENERGY   ┌─────────────┼────────────────────┼─────────────┐
Low      │  ●···●···   │   ●·····●··        │  Few hits,  │
(sparse) │  (euclidean)│   (displaced)      │  varies by  │
         │             │                    │  SHAPE      │
         ├─────────────┼────────────────────┤             │
High     │  ●●●●●●●●   │   ●●·●●·●●●·       │  Many hits, │
(busy)   │  (dense)    │   (chaotic)        │  varies by  │
         └─────────────┴────────────────────┴─────────────┘
                              ↓
                    SEEK X/Y navigate within this space
```

### SHAPE Algorithm: Humanized Euclidean

Even at SHAPE=0%, patterns are NOT robotic:

```cpp
// SHAPE = 0% (Humanized Euclidean)
// - Mathematical spacing as foundation
// - Subtle timing variation (±2ms jitter)
// - Slight velocity variation (±5%)
// - Result: Precise but alive

// SHAPE = 50% (Hybrid)
// - Mix of euclidean and weighted placement
// - Moderate timing looseness (±6ms)
// - Some hits displaced by ±1 step

// SHAPE = 100% (Wild)
// - Weighted probability placement
// - Significant timing chaos (±12ms)
// - Hits can displace ±2 steps
// - Still respects hit budget (not random noise)
```

### SEEK X: Grounded ↔ Floating

Controls which beat positions are emphasized:

```
SEEK X = 0% (Grounded):
├── Downbeats emphasized (beats 1, 2, 3, 4)
├── Backbeats secondary
├── Offbeats rare
└── Feel: Solid, foundational, "in the pocket"

SEEK X = 100% (Floating):
├── Offbeats emphasized
├── Downbeats de-emphasized
├── Syncopation maximized
└── Feel: Airy, displaced, "around the beat"
```

### SEEK Y: Simple ↔ Complex

Controls pattern intricacy (NOT density):

```
SEEK Y = 0% (Simple):
├── Straightforward rhythms
├── Clear pulse, easy to follow
├── Quarter/8th note focus
└── Feel: Accessible, danceable

SEEK Y = 100% (Complex):
├── Intricate rhythms
├── Polyrhythmic hints
├── Unusual subdivisions
└── Feel: Sophisticated, cerebral
```

**Key distinction**: At the same ENERGY level:
- Simple = `●···●···●···●···` (4 hits, obvious pattern)
- Complex = `●·····●·●·····●·` (4 hits, unusual pattern)

---

## V5 Config Mode (Restructured)

With SWING and AUX MODE less frequently changed, they move to shift:

```
┌─────────────────────────────────────────────────────────────┐
│  K1: LENGTH      K2: BUILD        K3: DRIFT      K4: CLOCK  │
│  (no shift)      +S: SWING        +S: AUX MODE   (no shift) │
└─────────────────────────────────────────────────────────────┘
```

| Knob | Primary | +Shift | Notes |
|------|---------|--------|-------|
| K1 | LENGTH (16/24/32/64) | - | Pattern length in steps |
| K2 | BUILD (phrase arc) | SWING (0-100%) | Drama + timing domain |
| K3 | DRIFT (evolution) | AUX MODE (Hat/Fill) | Evolution + output domain |
| K4 | CLOCK DIV (÷4 to ×4) | - | Clock division |

### Config Control Details

**LENGTH** (K1): Pattern length
- 16 steps = 1 bar (techno-friendly)
- 24 steps = 1.5 bars (shuffle-friendly)
- 32 steps = 2 bars (standard)
- 64 steps = 4 bars (long form)

**BUILD** (K2): Phrase arc intensity
- 0% = flat throughout (no builds)
- 50% = subtle crescendo toward phrase end
- 100% = dramatic build with fills

**DRIFT** (K3): Pattern evolution
- 0% = locked (identical every loop)
- 50% = subtle variation
- 100% = significant evolution per phrase

**CLOCK DIV** (K4): Clock division/multiplication
- ÷4, ÷2, ×1, ×2, ×4

**SWING** (K2+Shift): Timing feel
- 0% = straight (no swing)
- 50% = subtle shuffle
- 100% = heavy triplet feel

**AUX MODE** (K3+Shift): Aux output function
- HAT: Third voice trigger (hi-hat pattern)
- FILL: Gate high during fill zones

---

## Button Behavior (Simplified)

```
┌─────────────────────────────────────────────────────────────┐
│  TAP:        Trigger fill (on/off, not velocity sensitive) │
│  HOLD:       (reserved / could be reseed)                  │
│  DOUBLE-TAP: Reseed pattern (optional, could remove)       │
└─────────────────────────────────────────────────────────────┘
```

Given button is on/off only:
- **TAP = Fill** is the primary function
- Hold or double-tap could trigger reseed
- Or reseed could be triggered by external CV/gate

---

## AUX Output Behavior (Unchanged)

| Condition | AUX Output (CV Out 1) |
|-----------|----------------------|
| **Clock unpatched** | Internal clock output |
| **Clock patched, AUX=HAT** | Third voice trigger |
| **Clock patched, AUX=FILL** | Gate high during fills |

---

## Summary: V5 vs V4

### Performance Mode

| Aspect | v4 | v5 |
|--------|----|----|
| Parameters | ENERGY, BUILD, FIELD X, FIELD Y | ENERGY, SHAPE, SEEK X, SEEK Y |
| Shift functions | 4 | **0** |
| Paradigm | Genre-based grid | **Algorithm-based (regular↔wild)** |
| Density control | ENERGY + FIELD Y overlap | **ENERGY only** |
| Regularity control | Implicit in GENRE | **Explicit SHAPE parameter** |

### Config Mode

| Aspect | v4 | v5 |
|--------|----|----|
| Shift functions | 4 | **2** |
| Primary controls | LENGTH, SWING, AUX MODE, RESET MODE | LENGTH, BUILD, DRIFT, CLOCK DIV |
| Removed | - | GENRE, PUNCH, BALANCE, VOICE COUPLING, RESET MODE (hardcoded), AUX DENSITY |

### Removed Parameters (v4 → v5)

| Parameter | Reason |
|-----------|--------|
| GENRE | Algorithm-focused, not genre-specific |
| PUNCH | External VCAs handle dynamics |
| BALANCE | Voice coupling not valued |
| VOICE COUPLING | Not valued |
| AUX DENSITY | Over-complication |
| RESET MODE | Hardcoded to STEP (from v4.2) |

---

## Open Questions for Critic

1. **SHAPE naming**: Is "SHAPE" the right name for regularity/stability? Alternatives: CHAOS, GRID, ORDER
2. **SEEK naming**: Is "SEEK" the right verb? Alternatives: BIAS, TILT, SHIFT
3. **Config layout**: Is the K2/K3 shift pairing logical? BUILD+SWING, DRIFT+AUX MODE
4. **Reseed mechanism**: Double-tap, hold, or external CV?
5. **Euclidean humanization**: How much jitter at SHAPE=0%? ±2ms or more?
