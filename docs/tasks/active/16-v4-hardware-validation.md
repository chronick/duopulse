# Task 16: DuoPulse v4 Hardware Validation & Debug

**Status**: IN PROGRESS  
**Branch**: `feature/duopulse-v4`  
**Spec Reference**: docs/specs/main.md sections 4-11

---

## 🎛️ Hardware Reference

### Patch.Init Panel Layout

```
┌─────────────────────────────────────────────────────┐
│                    PATCH.INIT                       │
├─────────────────────────────────────────────────────┤
│                                                     │
│        ┌───┐  ┌───┐                                │
│        │K1 │  │K2 │       ← TOP ROW KNOBS          │
│        └───┘  └───┘                                │
│       ENERGY  BUILD                                 │
│                                                     │
│        ┌───┐  ┌───┐                                │
│        │K3 │  │K4 │       ← BOTTOM ROW KNOBS       │
│        └───┘  └───┘                                │
│       FIELD   FIELD                                 │
│         X       Y                                   │
│                                                     │
│   ○ CV1  ○ CV2  ○ CV3  ○ CV4   ← CV INPUTS        │
│                                                     │
│   ○ GATE1  ○ GATE2             ← GATE INPUTS       │
│   (Clock)  (Reset)                                  │
│                                                     │
│   ● GATE1  ● GATE2             ← GATE OUTPUTS      │
│   (Anchor) (Shimmer)                                │
│                                                     │
│   ○ AUD L  ○ AUD R             ← AUDIO INPUTS      │
│   (Fill)   (Flavor)                                 │
│                                                     │
│   ● AUD L  ● AUD R             ← AUDIO OUTPUTS     │
│   (Anchor  (Shimmer                                 │
│    Vel)    Vel)                                     │
│                                                     │
│   [B7]  [B8]                   ← BUTTONS           │
│   Shift  Mode                                       │
│   Layer  Switch                                     │
│                                                     │
│   ● CV1   ● CV2                ← CV OUTPUTS        │
│   (AUX)   (LED)                                     │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### Knob Layout (2×2 Grid)

```
        ┌─────────────────────┐
        │   ┌───┐     ┌───┐   │
        │   │K1 │     │K2 │   │  ← Top Row
        │   └───┘     └───┘   │
        │  ENERGY     BUILD   │
        │                     │
        │   ┌───┐     ┌───┐   │
        │   │K3 │     │K4 │   │  ← Bottom Row
        │   └───┘     └───┘   │
        │  FIELD X   FIELD Y  │
        └─────────────────────┘
```

### Control Modes

```
           ┌─────────────────────────────────────────┐
           │         MODE SWITCH (B8)                │
           ├───────────────────┬─────────────────────┤
           │    DOWN = PERF    │     UP = CONFIG     │
           ├───────────────────┼─────────────────────┤
 Normal    │ K1: ENERGY        │ K1: Pattern Length  │
           │ K2: BUILD         │ K2: Swing           │
           │ K3: FIELD X       │ K3: AUX Mode        │
           │ K4: FIELD Y       │ K4: Reset Mode      │
           ├───────────────────┼─────────────────────┤
 +Shift    │ K1: PUNCH         │ K1: Phrase Length   │
 (hold B7) │ K2: GENRE         │ K2: Clock Div       │
           │ K3: DRIFT         │ K3: AUX Density     │
           │ K4: BALANCE       │ K4: Voice Coupling  │
           └───────────────────┴─────────────────────┘
```

---

## Problem Statement

Initial hardware testing revealed triggers feel "irregular and slow" with unpredictable control response.

### Root Causes Identified

```mermaid
graph TD
    A[Irregular/Slow Triggers] --> B[Default Field 0,0]
    A --> C[Conservative Hit Budget]
    A --> D[Restrictive Eligibility]
    A --> E[Spacing Constraints]
    A --> F[No Flavor CV]
    
    B --> B1[MINIMAL archetype<br/>= sparsest pattern]
    C --> C1[Only 4 hits per<br/>32-step bar]
    D --> D1[Quarter notes only<br/>at mid-energy]
    E --> E1[2-step minimum<br/>at GROOVE zone]
    F --> F1[Zero swing/jitter<br/>= robotic feel]

    classDef problemStyle fill:#fae5e5,stroke:#586e75,color:#002b36
    classDef defaultStyle fill:#e6f2ff,stroke:#586e75,color:#002b36
    classDef errorStyle fill:#f2cccc,stroke:#586e75,color:#002b36
    
    class A problemStyle
    class B,C,D,E,F defaultStyle
    class B1,C1,D1,E1,F1 errorStyle
```

### Fixes Applied

1. Updated default control values in `main.cpp`:
   - `fieldX = 0.5, fieldY = 0.33` (Groovy archetype instead of Minimal)
   - `energy = 0.6` (Higher in GROOVE zone for more hits)

2. Added debug compile flags in `config.h`:
   - `DEBUG_FEATURE_LEVEL` (0-5) - Progressive feature enablement (primary flag)
   - `DEBUG_BASELINE_MODE` - Forces known-good knob values
   - `DEBUG_FIXED_SEED` - Reproducible patterns for debugging

3. Code cleanup (2024-12-25):
   - Consolidated `DEBUG_SIMPLE_TRIGGERS` into `DEBUG_FEATURE_LEVEL 0`
   - Refactored feature level checks to use `>=` pattern for clarity
   - Improved config.h documentation with per-level descriptions

4. **Deferred Flash Write Pattern (2024-12-26)**:
   - **Problem**: `SaveConfigToFlash()` was called inside the audio callback, causing 10-100ms blocking operations that glitch audio
   - **Solution**: Implemented deferred save pattern
     - Audio callback only sets `deferredSave.pending = true` flag (non-blocking)
     - Main loop handles actual flash write during idle time
     - Eliminates audio glitches from flash operations
   - **Files Modified**: `src/main.cpp`
     - Added `DeferredSave` struct in anonymous namespace (lines 71-76)
     - Modified `AudioCallback()` to defer save instead of blocking (lines 229-254)
     - Added flash write handler in main loop (lines 694-702)
   - **Reference**: `docs/chats/daisy-flash-writes.md`

---

## 🎚️ Debug Flag Reference

### DEBUG_FEATURE_LEVEL Quick Reference

| Level | Name | What It Tests | Knobs Active |
|-------|------|---------------|--------------|
| 0 | CLOCK | Audio callback, gates, LED, tempo | None (fixed pattern) |
| 1 | ARCHETYPE | Knob reading, archetype blending | K3 (Field X), K4 (Field Y) |
| 2 | SAMPLING | Hit budget, density scaling | K1 (Energy), K3, K4 |
| 3 | GUARD RAILS | Voice coupling, beat-1 anchor | K1, K3, K4 |
| 4 | TIMING | Swing/jitter from FLAVOR CV | K1, K3, K4 + Audio In R |
| 5 | PRODUCTION | Full system (same as no flag) | All knobs |

### Recommended Flag Combinations

```
# Level 0: Clock test - minimal config
#define DEBUG_FEATURE_LEVEL 0
// No other flags needed

# Level 1: Archetype test - add fixed seed for reproducibility
#define DEBUG_FEATURE_LEVEL 1
// #define DEBUG_FIXED_SEED 1  // Optional: same pattern each time

# Level 2-3: Generation test - add baseline mode to isolate sampling
#define DEBUG_FEATURE_LEVEL 2  // or 3
// #define DEBUG_BASELINE_MODE 1  // Optional: known control values
// #define DEBUG_FIXED_SEED 1     // Optional: reproducible patterns

# Level 4: Timing test
#define DEBUG_FEATURE_LEVEL 4
// No extra flags needed - test FLAVOR CV input

# Level 5 / Production: Remove all debug flags
// #define DEBUG_FEATURE_LEVEL 5  // Or just comment out
```

---

## 🔄 Iterative Testing Methodology

**This document is a living test plan.** Hardware testing often reveals issues that require immediate code changes. The workflow is:

1. **Run test** → Observe behavior
2. **Document findings** → Note what works, what doesn't
3. **Modify code** → Fix issues as you discover them
4. **Re-test** → Verify the fix works
5. **Update this document** → Record changes made and new expected behavior

### Important Notes

- **Expect to modify code during testing** - this is normal and expected
- **Always rebuild and reflash** after code changes: `make clean && make && make program-dfu`
- **Keep notes in the Test Log** section at the bottom
- Tests build on each other - don't advance to Level N+1 until Level N passes

---

## 🛠️ Test 0: Pre-Test Modifications

This section documents simplifications made to reduce complexity before hardware testing.

### Modification 0.1: Remove Tap Tempo (2024-12-20)

**Problem**: Button B7 was handling tap tempo, shift layer, and double-tap reseed. This complex state machine made debugging difficult and button behavior felt erratic.

**Changes Made**:
- **Removed tap tempo** - Internal clock runs at fixed 120 BPM (external clock via Gate In 1 still works)
- **Removed double-tap reseed** - Simplifies button logic
- **B7 is now shift-only** - Hold >100ms to activate shift layer, release to deactivate

**Files Modified**:
- `src/main.cpp` - Simplified button handling logic
- `inc/config.h` - DEBUG_FEATURE_LEVEL set to 0

**Expected Behavior After This Change**:
- B7 button: Hold to access shift layer, release returns to normal
- No tap tempo - tempo is fixed at 120 BPM internal clock
- External clock (Gate In 1) still works for tempo sync

**Build and Flash**:
```bash
make clean && make && make program-dfu
```

---

### Modification 0.2: Simplify LED Feedback (2024-12-20)

**Problem**: LED system was complex with multiple modes, making it hard to debug basic sequencer behavior.

**Changes Made**:
- **Simplified LED logic** - Removed complex LedIndicator state machine
- **Config mode**: LED solid on (100% brightness)
- **Anchor trigger (Gate Out 1)**: LED 50% brightness
- **Shimmer trigger (Gate Out 2)**: LED 30% brightness
- **Otherwise**: LED off

**Files Modified**:
- `src/main.cpp` - Replaced LedIndicator with simple brightness logic

**Expected Behavior After This Change**:
- Flip B8 switch UP → LED solid on (config mode)
- Flip B8 switch DOWN → LED blinks with triggers (performance mode)
- Gate Out 1 fires → LED at 50% brightness
- Gate Out 2 fires → LED at 30% brightness

**Switch Direction Confirmed**:
- **B8 UP** = Config mode (LED solid on)
- **B8 DOWN** = Performance mode (LED blinks with triggers)

**Build and Flash**:
```bash
make clean && make && make program-dfu
```

---

### Modification 0.3: Exclusive External Clock Mode (2025-12-27)

**Problem**: Clock system used timeout-based fallback (2-second timeout), causing unpredictable behavior. When external clock was patched, internal Metro continued running in parallel, making it hard to test and debug clock behavior.

**Changes Made (Task 19)**:
- **Removed timeout logic** - No more 2-second fallback to internal clock
- **Implemented exclusive mode** - When external clock is patched, internal Metro is completely disabled
- **Simplified clock state** - Replaced 3 complex variables (`usingExternalClock_`, `externalClockTimeout_`, `mustTick_`) with 2 simple flags (`externalClockActive_`, `externalClockTick_`)
- ~~**Added unpatch detection** - When Gate In 1 low for 1 second, restores internal clock immediately~~ **REMOVED** (see Update 2)

**Update 1 - Hardware Testing Feedback (2025-12-27)**:
Initial implementation had 1-second unpatch timeout, which broke slow/irregular external clocks.

**Update 2 - Removed Unpatch Timeout (2025-12-27)**:
- **Removed 1-second low state detection** - External clock stays active indefinitely
- **Allows slow, irregular, or paused clocks** - No automatic revert to internal clock
- External clock remains active until mode switch or power cycle

**Update 3 - Clock Division/Multiplication (2025-12-27)**:
- **Implemented full clock division/multiplication**: ÷8, ÷4, ÷2, ×1, ×2, ×4, ×8
- **Applies to BOTH internal and external clock** (not just internal)
- **Division**: Counter-based pulse counting
- **Multiplication**:
  - Internal: Metro frequency multiplied
  - External: Interval measurement + subdivision generation
- **Updated knob mapping**: 7 positions across full range (Config+Shift K2)

**Files Modified**:
- `src/Engine/Sequencer.h` - Clock state variables, clock division counters
- `src/Engine/Sequencer.cpp` - Clock logic in `ProcessAudio()`, `TriggerExternalClock()`, `SetClockDivision()`, `SetBpm()`
- `src/main.cpp` - Removed unpatch timeout, updated clock division knob mapping
- `src/Engine/Persistence.cpp` - Handle negative values (multiplication) in flash
- `docs/specs/main.md` - Updated CLOCK DIV spec (line 367)

**Expected Behavior After This Change**:
- **No external clock patched**: Internal 120 BPM clock drives sequencer (respects clock div/mult)
- **External clock patched**: Steps advance ONLY on rising edges (respects clock div/mult), internal Metro completely disabled
- **External clock paused/slow**: Sequencer waits indefinitely, no timeout
- **Clock division (÷2, ÷4, ÷8)**: Steps advance every N pulses
- **Clock multiplication (×2, ×4, ×8)**: Steps advance N times per pulse

**Spec Reference**: `docs/specs/main.md` section 3.4 [exclusive-external-clock], section 4.5 (K2 CLOCK DIV)

**Build and Flash**:
```bash
make clean && make && make program-dfu
```

**Build Status**: 122664 B / 128 KB (93.59% flash usage)

---

## 🧪 Part 1: Hardware Testing

### Test Flow Overview

```mermaid
flowchart LR
    L0[Level 0<br/>Clock Only] --> L1[Level 1<br/>Archetypes]
    L1 --> L2[Level 2<br/>Sampling]
    L2 --> L3[Level 3<br/>Guard Rails]
    L3 --> L4[Level 4<br/>Timing]
    L4 --> L5[Level 5<br/>Production]
    
    L0 -.->|FAIL| F0[Check:<br/>Audio callback<br/>Gate outputs]
    L1 -.->|FAIL| F1[Check:<br/>PatternField<br/>Soft knobs]
    L2 -.->|FAIL| F2[Check:<br/>HitBudget<br/>GumbelSampler]
    L3 -.->|FAIL| F3[Check:<br/>GuardRails<br/>VoiceRelation]
    L4 -.->|FAIL| F4[Check:<br/>BrokenEffects<br/>Flavor CV]

    classDef defaultStyle fill:#e6f2ff,stroke:#586e75,color:#002b36
    classDef warningStyle fill:#f5f0e0,stroke:#586e75,color:#002b36
    classDef errorStyle fill:#f2cccc,stroke:#586e75,color:#002b36
    
    class L0,L1,L2,L3,L4 defaultStyle
    class L5 warningStyle
    class F0,F1,F2,F3,F4 errorStyle
```

---

### Test 1: Basic Clock & Triggers (Level 0)

**Goal**: Verify the audio callback runs and triggers fire at the correct tempo.

**What you'll hear**: A simple "four-on-floor" kick pattern with snare on 2 and 4.

#### Setup

Open `inc/config.h` and find the `DEBUG_FEATURE_LEVEL` line. Set it to 0:

```cpp
#define DEBUG_FEATURE_LEVEL 0
```

Then build and flash:

```bash
make clean && make && make program-dfu
```

#### Expected Pattern

```
Step:    1 2 3 4 5 6 7 8 9 ...16  17...24  25...32
         ┌─────────────────────────────────────────
Anchor:  ● . . . . . . . ● . . .  ● . . .  ● . . .
Shimmer: . . . . . . . . ● . . .  . . . .  ● . . .
Aux:     ● . ● . ● . ● . ● . ● .  ● . ● .  ● . ● .
         └─────────────────────────────────────────
         ↑beat1   ↑beat2   ↑beat3   ↑beat4
```

#### Knob Positions (ignored at Level 0)

At Level 0, the knobs don't affect the pattern. The simple 4-on-floor runs regardless of knob positions.

![Knobs - Any Position](assets/knobs-any-position.svg)

```
        ┌─────────────────────┐
        │   ┌───┐     ┌───┐   │
        │   │ · │     │ · │   │  Any position
        │   └───┘     └───┘   │  is fine!
        │    K1        K2     │
        │                     │
        │   ┌───┐     ┌───┐   │
        │   │ · │     │ · │   │  Knobs are
        │   └───┘     └───┘   │  ignored at
        │    K3        K4     │  Level 0
        └─────────────────────┘
```

#### Checklist

**Switch & LED Tests**:
- [x] B8 switch UP → LED solid on (config mode)
- [x] B8 switch DOWN → LED blinks with triggers (performance mode)

**Performance Mode (B8 DOWN)**:
- [x] LED blinks at regular tempo (~120 BPM = 2 Hz for quarter notes)
- [x] LED at 50% brightness when Gate Out 1 fires
- [x] LED at 30% brightness when Gate Out 2 fires
- [x] Gate Out 1 fires on every beat (4 times per bar)
- [x] Gate Out 2 fires on beats 2 and 4 only
- [x] Audio Out L holds ~5V after each anchor trigger
- [x] Audio Out R holds ~5V after each shimmer trigger

**Button Test**:
- [x] B7 button: Hold to activate shift layer (no tap tempo)

#### Hardware Testing Feedback
- Mode switching appears to work correctly based on logs
- LED not blinking at regular 120 Hz
- LED brightness fluctuates but does not seem to correlate with timing of each hit
- Gates do fire (albeit not correct timing)
- Drum pattern seems erratic: shimmer hits when anchor does not, not detecting regular pattern.
- Aux output (CV_OUT_1) affected by Config + K3 while gate_in_1 (clock in) unpatched (should reflect clock only regardless of setting)

Recommendations:
- remove clock-based LED blinking, only blink on hits (Gate 1 and 2)
- Add logging for each gate out event with timestamp
- Add logging for config mode value changes

---

### Test 2: External Clock & Reset (Level 0)

**Goal**: Verify exclusive external clock behavior and reset functionality.

Same config as Test 1 (Level 0).

**Note**: The clock system uses **exclusive** mode — when external clock is patched, internal Metro is completely disabled. No timeout, no parallel operation.

#### External Clock Test: Exclusive Mode

**Spec Reference**: Section 3.4 [exclusive-external-clock]

**Setup**:
1. Start with no cable in Gate In 1 (internal clock active)
2. Verify internal 120 BPM clock is running (LED blinks every 500ms)

**Test A: External Clock Engagement**
1. Patch a clock source (16th notes, any tempo) to Gate In 1
2. Module should sync to external clock **immediately** (within one audio callback cycle)
3. Steps should advance **only** on rising edges of external clock
4. Internal Metro should be **completely disabled** (no parallel operation)

**Test B: Clock Source Exclusivity**
1. With external clock still patched and running
2. Observe that step timing follows **only** the external clock
3. Pattern should maintain 4-on-floor at whatever tempo the external clock provides
4. No drift or timeout behavior

**Test C: Immediate Internal Clock Restoration**
1. Unplug the external clock cable from Gate In 1
2. Internal 120 BPM clock should resume **immediately** (no delay)
3. Pattern should continue seamlessly with internal timing
4. ✅ **No 2-second timeout** (old behavior removed)

**Test D: Clock Stability at Various Tempos**
1. Patch external clock at ~60 BPM (slow)
2. Verify pattern tracks correctly
3. Change to ~200 BPM (fast)
4. Verify pattern tracks correctly
5. Try irregular clock (e.g., humanized/swung)
6. Verify steps advance only on rising edges

#### Reset Test

**Spec Reference**: Section 3.4 — Reset behavior identical for both clock sources

**Test E: Reset with Internal Clock**
1. Unplug external clock (internal 120 BPM active)
2. Patch a trigger/gate to Gate In 2
3. When gate goes high → pattern restarts from step 0
4. Verify reset happens on rising edge

**Test F: Reset with External Clock**
1. Patch external clock to Gate In 1
2. Trigger reset via Gate In 2
3. Pattern should restart from step 0 (same behavior as internal)
4. Reset should work identically regardless of clock source

#### Shift Button Test

1. Hold B7 for >100ms → shift layer should activate (LED may change behavior)
2. Release B7 → returns to normal mode
3. Short taps on B7 do nothing (tap tempo removed)

#### Acceptance Criteria

From spec section 3.4:

**Clock Source**:
- [ ] External clock patched → internal Metro completely disabled
- [ ] Steps advance only on rising edges at Gate In 1
- [ ] No timeout-based fallback while external clock patched
- [ ] Unplugging external clock restores internal immediately
- [ ] Clock source switching is deterministic and predictable

**Reset**:
- [ ] Reset detects rising edges reliably
- [ ] Reset behavior identical with internal or external clock
- [ ] Reset works at step 0 (PHRASE mode)

---

### Test 3: Archetype Patterns (Level 1)

**Goal**: Verify the Pattern Field responds to FIELD X and FIELD Y knobs (K3 and K4).

#### Setup

```cpp
#define DEBUG_FEATURE_LEVEL 1
```

Build and flash. Now the archetype patterns are active, but no hit budget/sampling—patterns come directly from archetype weight tables.

#### The Pattern Grid

The bottom two knobs (K3 and K4) navigate a 3×3 grid of pattern archetypes:

```
        FIELD Y (K4 - Complexity)
            ↑
        2   ┌───────┬───────┬───────┐
            │ Busy  │ Poly  │ Chaos │
            │       │rhythm │       │
        1   ├───────┼───────┼───────┤
            │Driving│ GROOVY│ Broken│  ← Sweet spot
            │       │  ★    │       │
        0   ├───────┼───────┼───────┤
            │Minimal│Steady │Displcd│
            │       │       │       │
            └───────┴───────┴───────┘
            0       1       2       → FIELD X (K3 - Syncopation)
```

#### Test Position A: Minimal (Bottom-Left)

Turn K3 and K4 fully counter-clockwise (7 o'clock position).

![Knobs - Minimal](assets/knobs-minimal.svg)

```
        ┌─────────────────────┐
        │   ┌───┐     ┌───┐   │
        │   │   │     │   │   │  K1, K2: don't care
        │   └───┘     └───┘   │
        │    K1        K2     │
        │                     │
        │   ┌───┐     ┌───┐   │
        │   │◄  │     │◄  │   │  K3, K4: fully CCW
        │   └───┘     └───┘   │  (7 o'clock)
        │    K3        K4     │
        └─────────────────────┘
```

**Expected**: Very sparse pattern, mostly just downbeats. Few triggers per bar.

---

#### Test Position B: Groovy ★ (Center)

Turn K3 and K4 to the 12 o'clock (noon) position.

![Knobs - Groovy](assets/knobs-groovy.svg)

```
        ┌─────────────────────┐
        │   ┌───┐     ┌───┐   │
        │   │   │     │   │   │  K1, K2: don't care
        │   └───┘     └───┘   │
        │    K1        K2     │
        │                     │
        │   ┌───┐     ┌───┐   │
        │   │ ↑ │     │ ↑ │   │  K3, K4: noon
        │   └───┘     └───┘   │  (12 o'clock)
        │    K3        K4     │
        └─────────────────────┘
```

**Expected**: Classic groove with syncopation. This is the "sweet spot" - danceable and musical.

---

#### Test Position C: Chaos (Top-Right)

Turn K3 and K4 fully clockwise (5 o'clock position).

![Knobs - Chaos](assets/knobs-chaos.svg)

```
        ┌─────────────────────┐
        │   ┌───┐     ┌───┐   │
        │   │   │     │   │   │  K1, K2: don't care
        │   └───┘     └───┘   │
        │    K1        K2     │
        │                     │
        │   ┌───┐     ┌───┐   │
        │   │  ►│     │  ►│   │  K3, K4: fully CW
        │   └───┘     └───┘   │  (5 o'clock)
        │    K3        K4     │
        └─────────────────────┘
```

**Expected**: Dense, irregular, IDM-style pattern. Lots of activity, unpredictable feel.

#### Checklist

- [ ] K3 (FIELD X) changes pattern character left-to-right
- [ ] K4 (FIELD Y) changes pattern density bottom-to-top
- [ ] Pattern changes happen at bar boundaries (not mid-bar)
- [ ] Center position feels "groovy" and danceable

---

### Test 4: Hit Budget & Generation (Level 2-3)

**Goal**: Verify ENERGY (K1) controls hit density and guard rails enforce musical rules.

#### Setup (Level 2 first, then Level 3)

```cpp
#define DEBUG_FEATURE_LEVEL 2  // Without guard rails
```

Build, flash, and test. Then:

```cpp
#define DEBUG_FEATURE_LEVEL 3  // With guard rails
```

#### Energy Zone Visualization

The ENERGY knob (K1) controls overall pattern density:

```
ENERGY Knob (K1) Position → Hit Density

  CCW       ¼         ½         ¾        CW
   │         │         │         │         │
   ▼         ▼         ▼         ▼         ▼
┌──────────────────────────────────────────────┐
│ MINIMAL │  GROOVE  │  BUILD  │    PEAK      │
│ 1-2 hits│ 2-4 hits │ 4-6 hits│  6-10 hits   │
│ per bar │ per bar  │ per bar │  per bar     │
└──────────────────────────────────────────────┘
```

#### Test: Sweep ENERGY (K1)

Slowly turn K1 from fully CCW to fully CW while listening.

![Knobs - Energy Sweep](assets/knobs-energy-sweep.svg)

```
        ┌─────────────────────┐
        │   ┌───┐     ┌───┐   │
        │   │↺↻ │     │   │   │  Sweep K1 slowly
        │   └───┘     └───┘   │  while listening
        │   K1 ↕       K2     │
        │   SWEEP             │
        │   ┌───┐     ┌───┐   │
        │   │ ↑ │     │ ↑ │   │  K3, K4: at noon
        │   └───┘     └───┘   │  (center position)
        │    K3        K4     │
        └─────────────────────┘
```

- [ ] Pattern gets denser as K1 turns clockwise
- [ ] Pattern gets sparser as K1 turns counter-clockwise
- [ ] Transitions feel smooth (no jarring changes)

#### Level 3: Guard Rails

Update to Level 3 and reflash:

```cpp
#define DEBUG_FEATURE_LEVEL 3  // With guard rails
```

- [ ] Beat 1 always has an anchor hit (downbeat forced)
- [ ] Never more than 4 consecutive silent steps
- [ ] Shimmer doesn't fire 3+ times in a row without anchor

---

### Test 5: Timing Effects (Level 4)

**Goal**: Verify swing and microtiming work with FLAVOR CV.

#### Setup

```cpp
#define DEBUG_FEATURE_LEVEL 4
```

#### Patching for Flavor CV

The FLAVOR CV input is on **Audio In R** (right audio input jack):

```
                    ┌─────────────────┐
   [Any CV source]──┤ Audio In R      │
   (0-5V)           │ (FLAVOR CV)     │
                    └─────────────────┘
```

Try patching:
- A steady +5V for maximum swing
- An LFO for evolving timing feel
- Leave unpatched for perfectly quantized (clinical) patterns

#### Timing Effect Ranges

```
FLAVOR CV Level → Timing Effect

 0V          1.5V         3V          4.5V        5V
  │            │           │            │          │
  ▼            ▼           ▼            ▼          ▼
┌─────────────────────────────────────────────────────┐
│ Quantized │  Light    │  Medium   │  Heavy   │ Max │
│ (robotic) │  Swing    │  Swing    │  Jitter  │Chaos│
└─────────────────────────────────────────────────────┘
```

#### Checklist

- [ ] At 0V (unpatched): Pattern is perfectly on-grid (clinical)
- [ ] At ~2.5V: Audible shuffle/swing feel
- [ ] At 5V: Maximum humanization, some notes feel "drunk"
- [ ] Swing primarily affects off-beats (the "e" and "a" subdivisions)

---

### Test 6: Full Production Mode (Level 5)

**Goal**: Full system integration test with all features enabled.

#### Setup

```cpp
#define DEBUG_FEATURE_LEVEL 5
```

Or simply remove/comment out the `DEBUG_FEATURE_LEVEL` line entirely (5 is default).

#### Complete Control Matrix

Test each parameter responds as expected:

**Performance Mode (Switch B8 DOWN)**

| Knob | Normal | Expected Effect |
|------|--------|-----------------|
| K1 | ENERGY | More hits as you turn clockwise |
| K2 | BUILD | Density increases toward phrase end |
| K3 | FIELD X | More syncopation clockwise |
| K4 | FIELD Y | More complexity clockwise |

**Performance Mode + Shift (Hold B7)**

| Knob | Shift Mode | Expected Effect |
|------|------------|-----------------|
| K1 | PUNCH | More velocity contrast |
| K2 | GENRE | Techno→Tribal→IDM character |
| K3 | DRIFT | Pattern evolves over time |
| K4 | BALANCE | Anchor/Shimmer ratio |

---

## 🔧 Part 2: Progressive Rebuild Method

```mermaid
graph TB
    subgraph "Phase A: Core Clock"
        A1[Level 0] --> A2{Triggers<br/>fire?}
        A2 -->|Yes| B1
        A2 -->|No| AF[Fix: Audio callback<br/>or gate outputs]
    end
    
    subgraph "Phase B: Pattern Field"
        B1[Level 1] --> B2{Knobs change<br/>pattern?}
        B2 -->|Yes| C1
        B2 -->|No| BF[Fix: PatternField<br/>or soft knobs]
    end
    
    subgraph "Phase C: Hit Budget"
        C1[Level 2] --> C2{ENERGY scales<br/>density?}
        C2 -->|Yes| D1
        C2 -->|No| CF[Fix: HitBudget<br/>or GumbelSampler]
    end
    
    subgraph "Phase D: Guard Rails"
        D1[Level 3] --> D2{Beat 1 always<br/>has anchor?}
        D2 -->|Yes| E1
        D2 -->|No| DF[Fix: GuardRails<br/>or VoiceRelation]
    end
    
    subgraph "Phase E: Timing"
        E1[Level 4] --> E2{Flavor CV<br/>adds swing?}
        E2 -->|Yes| F1
        E2 -->|No| EF[Fix: BrokenEffects]
    end
    
    subgraph "Phase F: Production"
        F1[Level 5] --> F2{Musical and<br/>controllable?}
        F2 -->|Yes| DONE[✓ Ship it!]
        F2 -->|No| FF[Tune archetypes<br/>Phase 12]
    end
    
    classDef defaultStyle fill:#e6f2ff,stroke:#586e75,color:#002b36
    classDef errorStyle fill:#f2cccc,stroke:#586e75,color:#002b36
    classDef warningStyle fill:#f5f0e0,stroke:#586e75,color:#002b36
    classDef successStyle fill:#d4f1d4,stroke:#586e75,color:#002b36
    
    class A1,A2,B1,B2,C1,C2,D1,D2,E1,E2,F1,F2 defaultStyle
    class AF,BF,CF,DF,EF errorStyle
    class FF warningStyle
    class DONE successStyle
```

---

## 🔍 Part 3: Issue Triage Flow

```mermaid
flowchart TD
    START[Triggers feel<br/>irregular/slow] --> T1{Test 1<br/>passes?}
    
    T1 -->|No triggers| FIX1[Check: AudioCallback<br/>gate outputs, wiring]
    T1 -->|Yes| T2{Test 2<br/>passes?}
    
    T2 -->|Tempo wrong| FIX2[Check: Metro init<br/>tap tempo logic]
    T2 -->|Yes| T3{Test 3<br/>passes?}
    
    T3 -->|Pattern static| FIX3[Check: Soft knobs<br/>PatternField, blend]
    T3 -->|Yes| T4{Test 4<br/>passes?}
    
    T4 -->|Too sparse| FIX4[Check: HitBudget<br/>eligibility masks]
    T4 -->|Yes| T5{Test 5<br/>passes?}
    
    T5 -->|No swing| FIX5[Check: FLAVOR CV input<br/>BrokenEffects]
    T5 -->|Yes| T6{Test 6<br/>passes?}
    
    T6 -->|Feels wrong| FIX6[Tune archetype<br/>weights Phase 12]
    T6 -->|Yes| DONE[✓ All working!]

    classDef problemStyle fill:#fae5e5,stroke:#586e75,color:#002b36
    classDef defaultStyle fill:#e6f2ff,stroke:#586e75,color:#002b36
    classDef errorStyle fill:#f2cccc,stroke:#586e75,color:#002b36
    classDef warningStyle fill:#f5f0e0,stroke:#586e75,color:#002b36
    classDef successStyle fill:#d4f1d4,stroke:#586e75,color:#002b36
    
    class START problemStyle
    class T1,T2,T3,T4,T5,T6 defaultStyle
    class FIX1,FIX2,FIX3,FIX4,FIX5 errorStyle
    class FIX6 warningStyle
    class DONE successStyle
```

---

## ✅ Completed Checklist

- [x] Add DEBUG_FEATURE_LEVEL compile flag
- [x] ~~Add DEBUG_SIMPLE_TRIGGERS bypass~~ (consolidated into Level 0)
- [x] Update default control values to musical center
- [x] Verify build compiles with all debug levels
- [x] Verify unit tests pass
- [x] Remove tap tempo (Test 0 modification - 2024-12-20)
- [x] Simplify B7 to shift-only behavior
- [x] Simplify LED feedback (Test 0 modification - 2024-12-20)
- [x] Verify switch direction (UP=config, DOWN=perf)
- [x] Refactor DEBUG_FEATURE_LEVEL to use >= pattern (2024-12-25)
- [x] Add flag reference table to task doc (2024-12-25)
- [x] **Hardware Test Level 0**: Basic clock (2025-12-27 - ✅ PASS)
- [x] Implement exclusive external clock mode (Test 0 modification - 2025-12-27, Task 19)
- [ ] **Hardware Test Level 0 Repeat**: External clock behavior (exclusive mode)
- [ ] **Hardware Test Level 1**: Direct archetype
- [ ] **Hardware Test Level 2-3**: Full generation
- [ ] **Hardware Test Level 4**: Timing effects
- [ ] **Hardware Test Level 5**: Production mode
- [ ] Document any issues found
- [ ] Fix issues and iterate

---

## ⚡ Quick Start Commands

### Debug Build Commands (2024-12-26)

New Makefile targets for DEBUG-level logging:

```bash
# Build with DEBUG-level logging (LOG_COMPILETIME_LEVEL=1, LOG_DEFAULT_LEVEL=1)
make build-debug

# Build debug and flash to device in one command
make program-debug

# These commands automatically set:
#   - DEBUG_FLASH=1 (debug symbols -g3, optimize for debugging -Og)
#   - LOG_COMPILETIME_LEVEL=1 (compile DEBUG+ logs)
#   - LOG_DEFAULT_LEVEL=1 (output DEBUG+ logs at runtime)
#
# Note: Uses -Og instead of -O0 to fit in 128KB flash
```

### Manual Debug Level Testing

For testing specific feature levels, manually edit `inc/config.h`:

```bash
# 1. Edit config.h to set debug level
#    nano inc/config.h
#    Change: #define DEBUG_FEATURE_LEVEL 0

# 2. Build and flash
make clean && make && make program-dfu

# 3. Test on hardware, note results

# 4. Increment level, repeat from step 1

# Run unit tests after any code changes:
make test
```

**Note**: `make build-debug` sets log verbosity but doesn't affect `DEBUG_FEATURE_LEVEL`. Use it when you need verbose logging output alongside your chosen feature level.

### Serial Monitoring (2024-12-26)

New `make listen` command added for serial debugging:

```bash
# Monitor serial output from device and log to temp file
make listen

# List available USB serial ports
make ports

# Custom baud rate or log directory
make listen BAUD=9600 LOGDIR=logs
```

**Usage**:
- Logs are saved to `/tmp/daisy_YYYYMMDD_HHMMSS.log` by default
- Press `Ctrl-A` then `\` then `y` to quit
- Log file path is displayed on exit
- Automatically detects `/dev/cu.usbmodem*` devices
- Uses `screen + tee` for reliable capture (see `docs/chats/daisy-debugging.md`)

---

## 📝 Test Log

Use this section to record your findings:

| Date | Level | Result | Notes |
|------|-------|--------|-------|
| 2024-12-20 | Pre | MODIFY | Removed tap tempo, simplified B7 to shift-only. Button was erratic. |
| 2024-12-20 | Pre | MODIFY | Simplified LED: 50% on anchor, 30% on shimmer, solid on config. |
| 2024-12-20 | Pre | VERIFY | Switch UP=config (LED solid), DOWN=perf (LED blinks). |
| 2024-12-20 | 0 | TESTING | Initial test after simplifications... |
| 2024-12-25 | Pre | REFACTOR | Consolidated DEBUG_SIMPLE_TRIGGERS into Level 0. Refactored to >= pattern. |
| 2024-12-26 | Pre | ENHANCE | Added `make build-debug` and `make program-debug` commands for DEBUG-level logging. |
| 2024-12-26 | Pre | FIX | Renamed logging enum values (DEBUG→LOG_DEBUG, etc.) to avoid macro conflict with -DDEBUG. |
| 2025-12-27 | 0 | ✅ PASS | All Level 0 tests passing! Fixed 3 race conditions: (1) event latch system for trigger detection, (2) increased pulse 1ms→10ms for Eurorack compatibility, (3) non-blocking ring buffer logger. Verified 4-on-floor at 120 BPM working correctly. See task 18 for details. |
| 2025-12-27 | Pre | MODIFY | Exclusive external clock mode (Task 19): Removed timeout logic, implemented exclusive mode with unpatch detection. Internal Metro completely disabled when ext clock active. Replaced 3 complex variables with 2 simple flags. Build: 122KB/128KB (93.15%). |
|      |   1   |        |       |
|      |   2   |        |       |
|      |   3   |        |       |
|      |   4   |        |       |
|      |   5   |        |       |
