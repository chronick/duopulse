# V5 Design Iteration 1: Control Simplification

**Date**: 2026-01-03
**Focus**: Reducing control complexity while maintaining expressiveness

---

## Analysis: V4 Control System Pain Points

### Current V4 Performance Mode (8 controls via shift)

| Knob | Primary | +Shift |
|------|---------|--------|
| K1 | ENERGY | PUNCH |
| K2 | BUILD | GENRE |
| K3 | FIELD X | DRIFT |
| K4 | FIELD Y | BALANCE |

### Problems Identified (applying eurorack-ux principles)

1. **Violates "maximum 1 shift layer" guideline**: Config mode ALSO has shift functions, creating 2 layers of hidden controls

2. **Shift functions aren't coherent modifications**:
   - K1: ENERGY → PUNCH ✅ (intensity domain, coherent)
   - K2: BUILD → GENRE ❌ (phrase arc → style bank, unrelated domains)
   - K3: FIELD X → DRIFT ❌ (navigation → evolution, unrelated)
   - K4: FIELD Y → BALANCE ❌ (navigation → voice ratio, unrelated)

3. **GENRE forces genre thinking**: User wants first-principles/algorithm-focused, not genre-specific patterns

4. **DRIFT buried in shift**: Evolution rate is actually a performance parameter users might CV-modulate

5. **Button is overloaded**: Shift + Fill + Reseed on one button

6. **Voice coupling (BALANCE) isn't valued**: User explicitly says it's not important

---

## User's Proposed Structure

### Performance (4 controls, no shift)
1. **Energy** - hit density
2. **Regular ↔ Irregular** - euclidean vs musically-random morphing
3. **Navigation A** - within algorithm paradigm
4. **Navigation B** - within algorithm paradigm

### Config (4 controls, shift OK)
- Pattern length
- Aux mode
- (2 free for other config)

### Freed Button
- Manual fills or performance toggle

---

## Design Option A: "Algorithm Focus"

**Philosophy**: Replace genre/field with a single "regularity" axis

```
Performance Mode (no shift needed):
┌─────────────────────────────────────────────────────────────┐
│  K1: ENERGY      K2: SHAPE        K3: SEEK X     K4: SEEK Y │
│  (density)       (regular↔chaos)  (navigate)     (navigate) │
└─────────────────────────────────────────────────────────────┘

Config Mode (shift for secondary):
┌─────────────────────────────────────────────────────────────┐
│  K1: LENGTH      K2: SWING        K3: AUX MODE   K4: (free) │
│  +S: (free)      +S: CLOCK DIV    +S: AUX DENS   +S: (free) │
└─────────────────────────────────────────────────────────────┘

Button: TAP = Fill, HOLD = Reseed
```

### SHAPE Parameter (K2)

Morphs between:
- **0% = Euclidean**: Mathematically even hit distribution
- **100% = Irregular**: Musically-weighted random with hit budgets

```
SHAPE 0%:    ●···●···●···●···  (perfect 4-on-floor)
SHAPE 50%:   ●···●·●·●···●···  (light syncopation)
SHAPE 100%:  ●·●···●·····●·●·  (irregular but musical)
```

### SEEK X/Y (K3, K4)

Instead of navigating a genre grid, navigate a **musical intent space**:

- **SEEK X**: Downbeat emphasis ↔ Offbeat emphasis
- **SEEK Y**: Sparse ↔ Dense (within ENERGY's budget)

This is genre-agnostic but produces genre-adjacent results:
- X=0, Y=0 → Four-on-floor feel (techno-adjacent)
- X=1, Y=0 → Offbeat emphasis (tribal-adjacent)
- X=1, Y=1 → Busy offbeats (IDM-adjacent)

### Trade-offs

**Pro**:
- No shift in performance mode
- Algorithm-focused, not genre-focused
- SHAPE is the core innovation user requested

**Con**:
- Loses PUNCH (velocity dynamics) from performance
- Loses BUILD (phrase arc) entirely
- SEEK X/Y may duplicate ENERGY's function

---

## Design Option B: "Merged Navigation"

**Philosophy**: Reduce 4 performance params to 3 + keep one shift pair

```
Performance Mode:
┌─────────────────────────────────────────────────────────────┐
│  K1: ENERGY      K2: SHAPE        K3: MORPH      K4: BUILD  │
│  +S: PUNCH       (regular↔chaos)  (navigation)   +S: DRIFT  │
└─────────────────────────────────────────────────────────────┘
```

### MORPH (K3) - Merged Navigation

Combines FIELD X and FIELD Y into a single knob that morphs through the pattern space:

```
MORPH 0%:   [0,0] minimal, straight
MORPH 25%:  [1,0] steady, syncopated
MORPH 50%:  [1,1] groovy, broken
MORPH 75%:  [2,1] driving, broken
MORPH 100%: [2,2] chaotic
```

The 2D field is traversed along a diagonal path, trading one dimension for simplicity.

### BUILD + DRIFT as Coherent Pair

K4: BUILD (phrase arc) + Shift: DRIFT (pattern evolution)

Both relate to "how the pattern changes over time" - coherent domain pairing.

### Trade-offs

**Pro**:
- Only 2 shift functions (PUNCH, DRIFT)
- BUILD preserved (phrase arc is valuable)
- SHAPE+MORPH captures user's intent
- DRIFT logically pairs with BUILD (temporal domain)

**Con**:
- Loses 2D navigation granularity
- MORPH traversal path is opinionated
- Still has shift layer in performance mode

---

## Design Option C: "Performance Purity"

**Philosophy**: Zero shift in performance mode, all complexity in config

```
Performance Mode (ZERO shift):
┌─────────────────────────────────────────────────────────────┐
│  K1: ENERGY      K2: SHAPE        K3: MORPH      K4: PUNCH  │
│  (density)       (regular↔chaos)  (navigation)   (dynamics) │
└─────────────────────────────────────────────────────────────┘

Config Mode (shift for secondary):
┌─────────────────────────────────────────────────────────────┐
│  K1: LENGTH      K2: SWING        K3: AUX MODE   K4: DRIFT  │
│  +S: PHRASE LEN  +S: CLOCK DIV    +S: AUX DENS   +S: BUILD  │
└─────────────────────────────────────────────────────────────┘

Button: TAP = Fill, DOUBLE-TAP = Reseed
```

### Key Insight

**Move BUILD and DRIFT to config mode**. Rationale:
- BUILD defines phrase *structure* (how long, how much arc) - rarely tweaked mid-performance
- DRIFT defines pattern *evolution* - set once, let it run
- These are "set and forget" more than performance controls

PUNCH stays in performance mode because velocity dynamics ARE tweaked constantly.

### Trade-offs

**Pro**:
- Zero cognitive load in performance mode
- All 4 performance knobs are CV-able, frequently tweaked
- Config mode is coherent "set and forget" territory
- Button is freed for fills (tap) and reseed (double-tap)

**Con**:
- BUILD moved to config means no CV modulation of phrase arc
- Loses field X/Y 2D navigation entirely (MORPH is 1D)
- Major paradigm shift from v4

---

## Recommendation: Option C with Refinements

### Why Option C

1. **Matches user's "4 performance + 4 config" request exactly**
2. **Zero shift in performance mode** - maximum simplicity
3. **Frees button** for fills/reseed as user requested
4. **SHAPE parameter** directly addresses euclidean↔irregular morphing
5. **Performance controls are the ones you actually tweak live**

### Refinements

1. **MORPH traversal should be customizable in config**:
   - Config could set "morph path" (diagonal, X-first, Y-first)
   - Default is diagonal through pattern space

2. **Consider keeping BUILD CV-able**:
   - Alternative: Put BUILD on performance K4, move PUNCH to config
   - BUILD + LFO is incredibly useful (automatic phrase dynamics)
   - Counter: PUNCH on hardware during live set is arguably more important

3. **Aux modes simplified**:
   - Just HAT + FILL BAR as user requested
   - Remove PHRASE_CV and EVENT modes (rarely used)

### Open Questions for User

1. **BUILD vs PUNCH**: Which deserves the performance slot?
   - BUILD = set phrase dynamics, let it run (→ config)
   - PUNCH = velocity contrast for groove (→ performance)
   - Or flip: BUILD is CV-modulated with LFO, PUNCH is set once

2. **MORPH path**: Single 1D knob loses 2D control. Acceptable?
   - Could offer a "MORPH MODE" in config that sets traversal strategy
   - Or expose Y-axis as CV-only (CV4 = complexity override)

3. **Reseed trigger**: Double-tap or dedicated?
   - If button is TAP=fill, could HOLD=reseed
   - Or CV input could serve as reseed trigger

---

## Next Steps

1. Get user feedback on Option C direction
2. If approved, detail the SHAPE algorithm (euclidean↔irregular blend)
3. Define MORPH traversal paths
4. Specify config mode simplifications
5. Consider running design-critic agent on this iteration

---

## Appendix: Eurorack-UX Checklist Applied

| Principle | V4 Status | Option C Status |
|-----------|-----------|-----------------|
| Domain mapping (musical intent) | ✅ Mostly | ✅ Improved (SHAPE = regular↔chaos) |
| Ergonomic pairing | ⚠️ Weak (GENRE unrelated to BUILD) | ✅ Strong (all performance = live tweaks) |
| 3-second rule | ⚠️ Shift breaks it | ✅ No shift in performance |
| CV = additive to knob | ✅ | ✅ |
| Maximum 1 shift layer | ❌ 2 layers | ✅ 1 layer (config only) |
| LED feedback | ✅ | ✅ (unchanged) |
| Defaults musically useful | ✅ | ✅ |
| Knob endpoints meaningful | ✅ | ✅ |
