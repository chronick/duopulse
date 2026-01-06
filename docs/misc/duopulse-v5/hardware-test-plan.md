# DuoPulse V5 Hardware Test Plan

**Version**: 5.0
**Date**: 2026-01-05
**Status**: Ready for Testing
**Branch**: `feature/duopulse-v5`

---

## Overview

This document provides a comprehensive hardware validation plan for the V5 control interface redesign. V5 introduces significant changes:

- **Zero shift layers** in both Performance and Config modes
- **New SHAPE parameter** with 3-way zone blending
- **AXIS X/Y bidirectional biasing** (renamed from FIELD X/Y)
- **Voice COMPLEMENT relationship** with DRIFT variation
- **Hat Burst** pattern-aware fill triggers
- **Hold+Switch gesture** for AUX mode selection
- **5-layer LED feedback system**
- **ACCENT parameter** for musical weight velocity

---

## Hardware Reference

### Patch.Init Panel Layout

```
┌─────────────────────────────────────────────────────┐
│                    PATCH.INIT                       │
├─────────────────────────────────────────────────────┤
│                                                     │
│        ┌───┐  ┌───┐                                │
│        │K1 │  │K2 │       ← TOP ROW KNOBS          │
│        └───┘  └───┘                                │
│       ENERGY SHAPE                                  │
│                                                     │
│        ┌───┐  ┌───┐                                │
│        │K3 │  │K4 │       ← BOTTOM ROW KNOBS       │
│        └───┘  └───┘                                │
│       AXIS X  AXIS Y                                │
│                                                     │
│   ○ CV1  ○ CV2  ○ CV3  ○ CV4   ← CV INPUTS        │
│                                                     │
│   ○ GATE1  ○ GATE2             ← GATE INPUTS       │
│   (Clock)  (Reset)                                  │
│                                                     │
│   ● GATE1  ● GATE2             ← GATE OUTPUTS      │
│   (Voice 1) (Voice 2)                              │
│                                                     │
│   ○ AUD L  ○ AUD R             ← AUDIO INPUTS      │
│   (Fill)   (unused)                                 │
│                                                     │
│   ● AUD L  ● AUD R             ← AUDIO OUTPUTS     │
│   (V1 Vel) (V2 Vel)                                │
│                                                     │
│   [BTN]  [SW]                  ← BUTTON/SWITCH     │
│   Fill   Mode                                       │
│   Reseed                                            │
│                                                     │
│   ● CV1   ● CV2                ← CV OUTPUTS        │
│   (AUX)   (LED)                                     │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### V5 Control Modes

```
           ┌─────────────────────────────────────────┐
           │         MODE SWITCH                     │
           ├───────────────────┬─────────────────────┤
           │    UP = PERF      │    DOWN = CONFIG    │
           ├───────────────────┼─────────────────────┤
           │ K1: ENERGY        │ K1: CLOCK DIV       │
           │ K2: SHAPE         │ K2: SWING           │
           │ K3: AXIS X        │ K3: DRIFT           │
           │ K4: AXIS Y        │ K4: ACCENT          │
           └───────────────────┴─────────────────────┘

           NO SHIFT LAYERS - Button gestures only!
```

---

## Test Sequence

```
Test 1: Basic Boot & Clock → Test 2: Performance Mode
                                      ↓
Test 3: SHAPE 3-Zone Blending ← Test 4: AXIS Biasing
                                      ↓
Test 5: Voice COMPLEMENT → Test 6: Config Mode
                                      ↓
Test 7: Button Gestures → Test 8: AUX Mode (Hat Burst)
                                      ↓
Test 9: LED Feedback → Test 10: Integration
```

---

## Test 1: Basic Boot & Clock

**Goal**: Verify module boots correctly and clock system works.

### 1.1 Boot Sequence

- [ ] Module powers on without errors
- [ ] LED provides boot indication
- [ ] Internal clock starts (120 BPM default)
- [ ] Triggers fire at expected tempo

### 1.2 Internal Clock

- [ ] Gate Out 1 (Voice 1) fires triggers
- [ ] Gate Out 2 (Voice 2) fires triggers
- [ ] Audio Out L holds velocity after Voice 1 trigger
- [ ] Audio Out R holds velocity after Voice 2 trigger
- [ ] Tempo is approximately 120 BPM

### 1.3 External Clock

- [ ] Patch clock to Gate In 1 → internal clock disables
- [ ] Steps advance only on rising edges
- [ ] Unplug clock → internal clock restores immediately
- [ ] No timeout behavior (exclusive mode)

### 1.4 Reset

- [ ] Gate In 2 rising edge → pattern restarts from step 0
- [ ] Reset works with internal clock
- [ ] Reset works with external clock

### Test 1 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 2: Performance Mode Basics

**Goal**: Verify all 4 performance knobs respond correctly.

**Setup**: Switch UP (Performance Mode)

### 2.1 ENERGY (K1)

| K1 Position | Expected Behavior |
|-------------|-------------------|
| CCW (0%) | Sparse pattern, few hits |
| Noon (50%) | Medium density, balanced |
| CW (100%) | Dense pattern, many hits |

- [ ] Pattern density increases clockwise
- [ ] Smooth transitions (no jarring changes)
- [ ] Both voices scale proportionally

### 2.2 SHAPE (K2) - Basic Response

| K2 Position | Expected Character |
|-------------|-------------------|
| CCW (0-30%) | Stable, techno feel |
| Noon (50%) | Syncopated, funky |
| CW (70-100%) | Wild, IDM chaos |

- [ ] Pattern character changes across range
- [ ] Audible difference between zones
- [ ] No abrupt discontinuities at zone boundaries

### 2.3 AXIS X (K3)

| K3 Position | Expected Behavior |
|-------------|-------------------|
| CCW (0%) | Grounded, downbeat-heavy |
| Noon (50%) | Neutral |
| CW (100%) | Floating, offbeat-heavy |

- [ ] Pattern shifts from downbeats to offbeats
- [ ] Bidirectional effect (suppress AND boost)
- [ ] Full range feels musical

### 2.4 AXIS Y (K4)

| K4 Position | Expected Behavior |
|-------------|-------------------|
| CCW (0%) | Simple, sparse decoration |
| Noon (50%) | Moderate complexity |
| CW (100%) | Intricate, busy fills |

- [ ] Complexity increases clockwise
- [ ] Weak positions get boosted at high Y
- [ ] Pattern remains coherent at extremes

### Test 2 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 3: SHAPE 3-Zone Blending

**Goal**: Verify SHAPE parameter zones and crossfade transitions.

**Setup**: Switch UP, ENERGY at 50%, AXIS X/Y at noon

### Zone Map

```
SHAPE Knob (K2) Position

  CCW        30%        50%        70%        CW
   │          │          │          │          │
   ▼          ▼          ▼          ▼          ▼
┌──────────────────────────────────────────────────┐
│  STABLE   │← →│ SYNCOPATED │← →│    WILD    │
│ Euclidean │FADE│  Tension   │FADE│  Weighted  │
│ humanized │    │  offbeats  │    │   chaos    │
└──────────────────────────────────────────────────┘
            28-32%            68-72%
            (crossfade)       (crossfade)
```

### 3.1 Zone 1: Stable (0-28%)

- [ ] Four-on-floor feel (techno)
- [ ] Downbeats prominent
- [ ] Subtle humanization (not robotic)
- [ ] Pattern feels "grounded"

### 3.2 Zone 2: Syncopated (32-68%)

- [ ] Increased tension
- [ ] Anticipation positions boosted
- [ ] Downbeats suppressed (but beat 1 protected)
- [ ] Funky/displaced feel

### 3.3 Zone 3: Wild (72-100%)

- [ ] Irregular patterns
- [ ] Chaos increases with knob position
- [ ] IDM/glitch character
- [ ] Still musically usable at 100%

### 3.4 Crossfade Transitions

- [ ] 28-32% crossfade: Stable → Syncopated (no clicks/pops)
- [ ] 68-72% crossfade: Syncopated → Wild (smooth)
- [ ] Sweep K2 slowly: no discontinuities

### Test 3 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 4: AXIS Bidirectional Biasing

**Goal**: Verify AXIS X/Y provide full bidirectional control.

**Setup**: Switch UP, ENERGY at 60%, SHAPE at 50%

### 4.1 AXIS X Full Range

| Test | K3 Position | Expected |
|------|-------------|----------|
| A | 0% (CCW) | Strong downbeats, suppressed offbeats |
| B | 25% | Mild downbeat emphasis |
| C | 50% (noon) | Neutral (no bias) |
| D | 75% | Mild offbeat emphasis |
| E | 100% (CW) | Strong offbeats, suppressed downbeats |

- [ ] Test A: Clear downbeat emphasis
- [ ] Test C: Neutral baseline
- [ ] Test E: Clear offbeat emphasis
- [ ] Sweep CCW→CW: Continuous transition

### 4.2 AXIS Y Full Range

| Test | K4 Position | Expected |
|------|-------------|----------|
| A | 0% (CCW) | Simple, only strong positions |
| B | 50% (noon) | Moderate complexity |
| C | 100% (CW) | Complex, weak positions active |

- [ ] Test A: Sparse decoration
- [ ] Test C: Rich, intricate fills
- [ ] Y boosts weak metric positions at high values
- [ ] Y suppresses weak positions at low values

### 4.3 Broken Mode Interaction

**Setup**: SHAPE > 60%, AXIS X > 70%

- [ ] "Broken" mode activates at high SHAPE + high AXIS X
- [ ] Downbeats randomly suppressed (not all, deterministic per seed)
- [ ] Creates IDM/broken beat feel
- [ ] Pattern still has groove (not random noise)

### Test 4 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 5: Voice COMPLEMENT Relationship

**Goal**: Verify shimmer voice fills gaps in anchor pattern.

**Setup**: Switch UP, moderate settings

### 5.1 Gap-Filling Behavior

- [ ] Shimmer hits primarily fall in anchor gaps
- [ ] No shimmer hits directly on anchor hits
- [ ] Adjacent positions (±1 step) may have shimmer

### 5.2 DRIFT Variation (Config Mode K3)

Switch DOWN, adjust K3:

| DRIFT Position | Expected Behavior |
|----------------|-------------------|
| 0-30% | Evenly spaced shimmer in gaps |
| 30-70% | Weight-based placement |
| 70-100% | Seed-varied placement |

- [ ] DRIFT=0%: Shimmer evenly distributed in gaps
- [ ] DRIFT=50%: Shimmer prefers weighted positions
- [ ] DRIFT=100%: Shimmer placement varies (seed-controlled)

### 5.3 Wrap-Around Handling

- [ ] Pattern end/start gap handled correctly
- [ ] No orphan hits at phrase boundaries

### Test 5 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 6: Config Mode

**Goal**: Verify all 4 config parameters work correctly.

**Setup**: Switch DOWN (Config Mode)

### 6.1 CLOCK DIV (K1)

| K1 Position | Division | Expected |
|-------------|----------|----------|
| 0-14% | ÷4 | 4× slower |
| 14-28% | ÷2 | 2× slower |
| 28-42% | ÷1 | Normal |
| 42-58% | ×1 | Normal (center) |
| 58-72% | ×2 | 2× faster |
| 72-86% | ×4 | 4× faster |

- [ ] CCW slows tempo
- [ ] Center is normal tempo
- [ ] CW speeds up tempo
- [ ] Works with internal clock
- [ ] Works with external clock

### 6.2 SWING (K2)

| K2 Position | Swing % | Expected |
|-------------|---------|----------|
| CCW (0%) | 50% | Straight/quantized |
| Noon (50%) | ~58% | Shuffle feel |
| CW (100%) | 66% | Heavy triplet |

- [ ] CCW: Robotic/quantized timing
- [ ] CW: Heavy swing/shuffle
- [ ] Off-beats shift later with more swing
- [ ] Audible timing difference

### 6.3 DRIFT (K3)

| K3 Position | Expected |
|-------------|----------|
| CCW (0%) | Pattern locked (same each phrase) |
| Noon (50%) | Moderate evolution |
| CW (100%) | Pattern varies each phrase |

- [ ] DRIFT=0%: Same pattern repeats
- [ ] DRIFT=100%: Pattern changes each phrase
- [ ] Variation is controlled (not random)

### 6.4 ACCENT (K4)

| K4 Position | Expected |
|-------------|----------|
| CCW (0%) | Flat velocity (all hits equal) |
| Noon (50%) | Normal dynamics |
| CW (100%) | Wide dynamics (ghost notes to accents) |

- [ ] CCW: All hits similar velocity
- [ ] CW: Strong accent/ghost contrast
- [ ] Velocity follows metric weight
- [ ] Range: 30% floor to 100% ceiling at max ACCENT

### Test 6 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 7: Button Gestures

**Goal**: Verify all button gestures work correctly.

### Gesture Reference

| Gesture | Action | Timing |
|---------|--------|--------|
| Tap (20-500ms) | Trigger Fill | Immediate |
| Hold 3s + release | Reseed pattern | On release |
| Hold + Switch UP | Set AUX to HAT | During hold |
| Hold + Switch DOWN | Set AUX to FILL GATE | During hold |

### 7.1 Tap Fill

- [ ] Tap button (20-500ms) → Fill triggers
- [ ] Fill affects density/velocity
- [ ] Fill has clear start/end
- [ ] Multiple quick taps queue fills

### 7.2 Hold Reseed (3 seconds)

- [ ] Hold button for 3s → LED builds pulse
- [ ] Release → Pattern reseeds
- [ ] LED confirms reseed (triple flash)
- [ ] New pattern is different from previous

### 7.3 AUX Mode Gesture (Hold + Switch)

**Test A: Set HAT Mode**
1. Hold button
2. Move switch UP
3. Release button

- [ ] AUX mode changes to HAT
- [ ] LED shows triple rising flash
- [ ] Mode change does NOT change Perf/Config mode
- [ ] HAT triggers appear on CV Out 1

**Test B: Set FILL GATE Mode**
1. Hold button
2. Move switch DOWN
3. Release button

- [ ] AUX mode changes to FILL GATE
- [ ] LED shows single fade
- [ ] CV Out 1 goes high during fills

### 7.4 Gesture Cancellation

- [ ] Hold button + switch → cancels pending fill
- [ ] Hold button + switch → cancels pending shift
- [ ] Button release after switch → no fill triggered

### Test 7 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 8: AUX Output Modes (Hat Burst)

**Goal**: Verify AUX output modes, especially new Hat Burst feature.

### 8.1 FILL GATE Mode (Default)

- [ ] CV Out 1 goes HIGH during fill duration
- [ ] Gate level is ~5V
- [ ] Gate duration matches fill length
- [ ] Works with fill button tap

### 8.2 HAT Mode (Secret "2.5 Pulse")

Activate via: Hold button + Switch UP

**Pattern-Aware Burst Triggers**:

| ENERGY | Expected Burst Density |
|--------|------------------------|
| Low (20%) | 2-4 triggers per fill |
| Medium (50%) | 5-7 triggers per fill |
| High (80%) | 8-12 triggers per fill |

- [ ] Burst triggers appear during fill
- [ ] Density follows ENERGY
- [ ] Regularity follows SHAPE
- [ ] Max 12 triggers per burst (pre-allocated)

**Velocity Ducking**:
- [ ] Hat triggers near main pattern hits: 30% velocity
- [ ] Hat triggers away from main hits: 65-100% velocity
- [ ] Ducking is audible (if using VCA)

### 8.3 Mode Persistence

- [ ] AUX mode survives power cycle (boot gesture)
- [ ] Boot with Hold + Switch UP → HAT mode persists
- [ ] Boot with Hold + Switch DOWN → FILL GATE persists

### Test 8 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 9: LED Feedback System

**Goal**: Verify 5-layer LED feedback system.

### Layer Reference

| Layer | State | Behavior | Priority |
|-------|-------|----------|----------|
| 1 | Idle (Perf) | Gentle breath synced to clock | Base |
| 2 | Clock sync | Subtle pulse on beats | Additive |
| 3 | Trigger activity | Pulse on hits, envelope decay | Maximum |
| 4 | Fill active | Accelerating strobe + trigger overlay | Maximum |
| 5 | Events | Mode switch, reseed, etc. | Replace |

### 9.1 Base Layer (Idle)

- [ ] Performance mode: Gentle breathing animation
- [ ] Config mode: Slower breathing
- [ ] Breath syncs to clock tempo

### 9.2 Clock Sync Layer

- [ ] Subtle pulse on downbeats
- [ ] Additive (doesn't replace base)

### 9.3 Trigger Activity Layer

- [ ] LED pulses on each trigger
- [ ] Envelope decay after pulse
- [ ] Uses maximum blend (shows peaks)

### 9.4 Fill Layer

- [ ] Fill active: Accelerating strobe
- [ ] Trigger overlay visible on top of strobe
- [ ] Clear visual indication of fill duration

### 9.5 Event Layer (Overrides)

| Event | LED Behavior |
|-------|--------------|
| Mode switch | Quick signature |
| Reseed progress | Building pulse (1-5Hz over 3s) |
| Reseed confirm | POP POP POP (3 flashes) |
| AUX mode HAT unlock | Triple rising flash |
| AUX mode FILL GATE | Single fade |

- [ ] Mode switch: Quick flash
- [ ] Reseed building: Pulse rate increases
- [ ] Reseed confirm: Triple flash
- [ ] HAT unlock: Triple rising flash
- [ ] FILL GATE: Single fade

### Test 9 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Test 10: Integration Tests

**Goal**: Verify complete system behavior with combined settings.

### 10.1 Style Preset Tests

Test these preset combinations and verify they produce expected results:

**Minimal Techno**:
```
ENERGY: 20%    SHAPE: 10%    AXIS X: 20%    AXIS Y: 20%
CLOCK: x1      SWING: 20%    DRIFT: 10%     ACCENT: 70%
```
- [ ] Sparse, driving, four-on-floor feel

**Groovy/Funk**:
```
ENERGY: 55%    SHAPE: 50%    AXIS X: 65%    AXIS Y: 55%
CLOCK: x1      SWING: 60%    DRIFT: 40%     ACCENT: 65%
```
- [ ] Syncopated, shuffled, funky

**IDM/Chaos**:
```
ENERGY: 85%    SHAPE: 90%    AXIS X: 85%    AXIS Y: 90%
CLOCK: varies  SWING: varies DRIFT: 80%     ACCENT: 85%
```
- [ ] Busy, irregular, broken

### 10.2 CV Modulation

**CV inputs always modulate Performance parameters**:

| CV Input | Modulates |
|----------|-----------|
| CV1 | ENERGY |
| CV2 | SHAPE |
| CV3 | AXIS X |
| CV4 | AXIS Y |

- [ ] CV1 modulates ENERGY in Performance mode
- [ ] CV1 modulates ENERGY even in Config mode (background)
- [ ] All 4 CVs respond to modulation
- [ ] CV modulation is bipolar (0V = no mod)

### 10.3 Extended Playability

Run module for 30+ minutes:
- [ ] No audio glitches
- [ ] No crashes
- [ ] LED remains responsive
- [ ] Clock stays locked (internal and external)

### 10.4 Transition Tests

**Smooth Transition Paths**:

1. Techno → Syncopation:
   - [ ] SHAPE 25% → 50%: Smooth transition
   - [ ] AXIS X 30% → 65%: Gradual shift
   - [ ] No jarring pattern changes

2. Syncopation → Chaos:
   - [ ] SHAPE 50% → 85%: Enters wild zone
   - [ ] AXIS X 65% → 80%+: Triggers broken mode
   - [ ] Remains musically coherent

### Test 10 Notes:
```
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________
```

---

## Issue Log

Use this section to document any issues found during testing:

| Date | Test | Severity | Description | Resolution |
|------|------|----------|-------------|------------|
| | | | | |
| | | | | |
| | | | | |

**Severity Levels**:
- **P0**: Blocker (crashes, no audio)
- **P1**: Major (wrong behavior, unusable feature)
- **P2**: Minor (cosmetic, workaround exists)
- **P3**: Enhancement (nice to have)

---

## Quick Commands

```bash
# Build and flash
make clean && make && make program-dfu

# Build with debug logging
make build-debug && make program-dfu

# Monitor serial output
make listen

# Run unit tests
make test
```

---

## Test Completion Checklist

- [ ] Test 1: Basic Boot & Clock
- [ ] Test 2: Performance Mode Basics
- [ ] Test 3: SHAPE 3-Zone Blending
- [ ] Test 4: AXIS Bidirectional Biasing
- [ ] Test 5: Voice COMPLEMENT Relationship
- [ ] Test 6: Config Mode
- [ ] Test 7: Button Gestures
- [ ] Test 8: AUX Output Modes (Hat Burst)
- [ ] Test 9: LED Feedback System
- [ ] Test 10: Integration Tests

**Sign-off**:
- Tester: _______________
- Date: _______________
- Firmware Version: _______________
- All Critical Tests Pass: [ ] Yes [ ] No

---

*End of V5 Hardware Test Plan*
