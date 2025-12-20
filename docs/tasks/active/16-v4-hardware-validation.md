# Task 16: DuoPulse v4 Hardware Validation & Debug

**Status**: IN PROGRESS  
**Branch**: `feature/duopulse-v4`  
**Spec Reference**: docs/specs/main.md sections 4-11

## Problem Statement

Initial hardware testing revealed triggers feel "irregular and slow" with unpredictable control response.

### Root Causes Identified

1. **Default Field Position (0,0) = MINIMAL Archetype** - Sparsest pattern in the grid
2. **Hit Budget at GROOVE Zone is Very Conservative** - Only 4 hits per 32-step bar
3. **Eligibility Masks Too Restrictive** - Quarter notes only at mid-energy
4. **Spacing Constraints Compound Restrictions** - 2-step minimum at GROOVE
5. **No FLAVOR CV = No Timing Variation** - Patterns are perfectly quantized
6. **BUILD=0 = No Phrase Development** - Patterns stay flat

### Fixes Applied

1. Updated default control values in `main.cpp`:
   - `fieldX = 0.5, fieldY = 0.33` (Groovy archetype instead of Minimal)
   - `energy = 0.6` (Higher in GROOVE zone for more hits)

2. Added debug compile flags in `config.h`:
   - `DEBUG_BASELINE_MODE` - Forces known-good values
   - `DEBUG_SIMPLE_TRIGGERS` - Bypasses generation, simple 4-on-floor
   - `DEBUG_FIXED_SEED` - Reproducible patterns
   - `DEBUG_FEATURE_LEVEL` (0-5) - Progressive feature enablement

---

## Part 1: Hardware Testing Checklist

### Test 1: Basic Clock & Trigger Verification (Level 0)

**Setup:**
```cpp
// In inc/config.h, uncomment:
#define DEBUG_SIMPLE_TRIGGERS 1
// Or set:
#define DEBUG_FEATURE_LEVEL 0
```

**Build & Flash:**
```bash
make clean && make && make program-dfu
```

**Verify:**
- [ ] LED blinks at regular tempo (~120 BPM = 2 Hz blinks for quarter notes)
- [ ] Gate Out 1 (Anchor) fires on beats 1, 2, 3, 4 (every 0.5 seconds)
- [ ] Gate Out 2 (Shimmer) fires on beats 2, 4 only (every 1 second)
- [ ] Audio Out L outputs steady 5V when Anchor fires
- [ ] Audio Out R outputs steady 5V when Shimmer fires
- [ ] Tap tempo button (B7) changes tempo

**Expected Result:** Simple, predictable 4-on-floor pattern with backbeat.

---

### Test 2: Tempo Control (Level 0)

**Same config as Test 1**

**Verify:**
- [ ] Tap B7 twice ~0.5s apart → tempo increases to ~120 BPM
- [ ] Tap B7 twice ~1.0s apart → tempo slows to ~60 BPM
- [ ] External clock on Gate In 1 syncs sequencer
- [ ] Reset on Gate In 2 resets to step 0

---

### Test 3: Archetype Pattern Direct (Level 1)

**Setup:**
```cpp
// In inc/config.h:
#define DEBUG_FEATURE_LEVEL 1
```

**Build & Flash.**

**Verify:**
- [ ] K3 (FIELD X) changes pattern character when turned
  - Left = more downbeats, Right = more offbeats
- [ ] K4 (FIELD Y) changes pattern density
  - Down = sparse, Up = busy
- [ ] Pattern changes are immediate (at next bar)
- [ ] At center position (K3=50%, K4=50%), pattern feels "groovy"

---

### Test 4: Full Generation (Level 2-3)

**Setup:**
```cpp
#define DEBUG_FEATURE_LEVEL 2  // First: no guard rails
// Then try:
#define DEBUG_FEATURE_LEVEL 3  // With guard rails
```

**Verify:**
- [ ] K1 (ENERGY) affects pattern density
  - 0-20% = very sparse (1-2 hits/bar)
  - 20-50% = moderate (4-6 hits/bar)
  - 50-75% = active (6-8 hits/bar)
  - 75-100% = dense (8+ hits/bar)
- [ ] Patterns change at bar boundaries, not mid-bar
- [ ] Guard rails (Level 3): Anchor always fires on beat 1
- [ ] Guard rails (Level 3): No more than 4 steps silence

---

### Test 5: Timing Effects (Level 4)

**Setup:**
```cpp
#define DEBUG_FEATURE_LEVEL 4
```

**Verify (requires scope or trained ear):**
- [ ] Patch 5V to Audio In R (FLAVOR CV)
  - At 0V: Pattern is perfectly quantized
  - At 2.5V: Slight swing/shuffle
  - At 5V: Maximum swing + jitter
- [ ] Swing affects odd steps (e, a subdivisions)

---

### Test 6: Full Production Mode (Level 5)

**Setup:**
```cpp
#define DEBUG_FEATURE_LEVEL 5  // (default)
// Remove all DEBUG_ flags
```

**Full Control Test Matrix:**

| Knob | Normal Mode | Shift Mode (hold B7) |
|------|-------------|---------------------|
| K1   | ENERGY (density) | PUNCH (velocity dynamics) |
| K2   | BUILD (phrase arc) | GENRE (Techno/Tribal/IDM) |
| K3   | FIELD X (syncopation) | DRIFT (pattern evolution) |
| K4   | FIELD Y (complexity) | BALANCE (voice ratio) |

**Verify each control responds correctly.**

---

## Part 2: Progressive Rebuild Method

### Phase A: Core Clock (Tests 1-2)
```cpp
#define DEBUG_FEATURE_LEVEL 0
```
- Confirms: Audio callback runs, triggers fire, tempo works
- If fails: Issue is in clock/output layer

### Phase B: Pattern Field (Test 3)
```cpp
#define DEBUG_FEATURE_LEVEL 1
```
- Confirms: Archetype loading, blending, FIELD X/Y work
- If fails: Issue is in PatternField/ArchetypeData

### Phase C: Hit Budget & Gumbel (Test 4)
```cpp
#define DEBUG_FEATURE_LEVEL 2
```
- Confirms: Hit budget, eligibility, Gumbel sampling work
- If fails: Issue is in HitBudget or GumbelSampler

### Phase D: Guard Rails (Test 4 continued)
```cpp
#define DEBUG_FEATURE_LEVEL 3
```
- Confirms: Soft repair, hard guard rails, voice relationship work
- If fails: Issue is in GuardRails or VoiceRelation

### Phase E: Timing (Test 5)
```cpp
#define DEBUG_FEATURE_LEVEL 4
```
- Confirms: Swing, jitter, displacement work
- If fails: Issue is in BrokenEffects

### Phase F: Full System (Test 6)
```cpp
#define DEBUG_FEATURE_LEVEL 5
```
- Full integration test

---

## Part 3: Issue Triage Flow

```
ISSUE: Triggers feel irregular/slow

├── Test 1 FAILS (no triggers at all)
│   └── Check: Audio callback, gate outputs, wiring
│
├── Test 1 PASSES, Test 2 FAILS (tempo issues)
│   └── Check: Metro init, tap tempo, external clock
│
├── Tests 1-2 PASS, Test 3 FAILS (pattern doesn't change)
│   └── Check: Soft knobs, control mapping, PatternField
│
├── Tests 1-3 PASS, Test 4 FAILS (patterns sparse/weird)
│   └── Check: HitBudget params, eligibility masks, energy zones
│
├── Tests 1-4 PASS, Test 5 FAILS (no swing/timing)
│   └── Check: FLAVOR CV input, BrokenEffects
│
└── All tests PASS but still feels wrong
    └── Check: Archetype weight tuning (Phase 12)
```

---

## Completed Checklist

- [x] Add DEBUG_FEATURE_LEVEL compile flag
- [x] Add DEBUG_SIMPLE_TRIGGERS bypass
- [x] Update default control values to musical center
- [x] Verify build compiles with all debug levels
- [x] Verify tests pass
- [ ] Hardware Test Level 0: Basic clock
- [ ] Hardware Test Level 1: Direct archetype
- [ ] Hardware Test Level 2-3: Full generation
- [ ] Hardware Test Level 4: Timing effects
- [ ] Hardware Test Level 5: Production mode
- [ ] Document any issues found
- [ ] Fix issues and iterate

---

## Quick Start Commands

```bash
# Build with Level 0 (simple triggers)
make clean && make && make program-dfu

# After each level change in config.h:
make clean && make && make program-dfu

# Run tests after code changes:
make test
```
