# DuoPulse Control Map Reference

## Quick Reference Visual

### Performance Mode (Switch DOWN) - Button Released

```
┌──────────────────────────────────────────────────────────────────┐
│  PERFORMANCE MODE (Primary Controls)                              │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  K1: ANCHOR DENSITY         CV5: Always modulates K1             │
│      Sparse ← → Busy              (even in Config mode)          │
│                                                                   │
│  K2: SHIMMER DENSITY        CV6: Always modulates K2             │
│      Sparse ← → Busy              (even in Config mode)          │
│                                                                   │
│  K3: FLUX                   CV7: Always modulates K3             │
│      Steady ← → Chaotic           (even in Config mode)          │
│                                                                   │
│  K4: FUSE                   CV8: Always modulates K4             │
│      Anchor ← → Shimmer           (even in Config mode)          │
│                                                                   │
│  [B7 Button: HOLD = Shift Controls]                              │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

### Performance Mode (Switch DOWN) - Button Held (Shift)

```
┌──────────────────────────────────────────────────────────────────┐
│  PERFORMANCE MODE (Shift Controls)                                │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  K1+Shift: ANCHOR ACCENT                                         │
│            Subtle ← → Hard                                       │
│                                                                   │
│  K2+Shift: SHIMMER ACCENT                                        │
│            Subtle ← → Hard                                       │
│                                                                   │
│  K3+Shift: ORBIT (Voice Relationship)                            │
│            Interlock ← Free ← → Shadow                           │
│                                                                   │
│  K4+Shift: CONTOUR (CV Shape)                                    │
│            Velocity | Decay | Pitch | Random                     │
│                                                                   │
│  [B7 Button: Release to return to Primary Controls]              │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

### Config Mode (Switch UP) - Button Released

```
┌──────────────────────────────────────────────────────────────────┐
│  CONFIG MODE (Primary Controls)                                   │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  K1: TERRAIN (Genre Character)                                   │
│      Techno | Tribal | Trip-Hop | IDM                            │
│                                                                   │
│  K2: LENGTH (Loop Bars)                                          │
│      1 | 2 | 4 | 8 | 16                                          │
│                                                                   │
│  K3: GRID (Pattern Select)                                       │
│      1 ← → 16                                                    │
│                                                                   │
│  NOTE: Gate In 1 = Clock, Gate In 2 = Reset                      │
│                                                                   │
│  K4: TEMPO (Internal BPM)                                        │
│      90 ← → 160                                                  │
│                                                                   │
│  [B7 Button: Hold for Shift Controls]                            │
│                                                                   │
│  NOTE: CV 5-8 still modulate Performance parameters!             │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

### Config Mode (Switch UP) - Button Held (Shift)

```
┌──────────────────────────────────────────────────────────────────┐
│  CONFIG MODE (Shift Controls)                                     │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  K1+Shift: SWING TASTE                                           │
│            Low (within genre) ← → High (within genre)            │
│                                                                   │
│  K2+Shift: GATE TIME                                             │
│            5ms ← → 50ms                                          │
│                                                                   │
│  K3+Shift: HUMANIZE                                              │
│            None ← → Loose (±10ms jitter)                         │
│                                                                   │
│  K4+Shift: CLOCK DIV                                             │
│            ÷4 | ÷2 | ×1 | ×2 | ×4                                │
│                                                                   │
│  [B7 Button: Release to return to Primary Controls]              │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

---

## Control Flow Diagram

```
                        ┌─────────────────┐
                        │   Mode Switch   │
                        │   (B8)          │
                        └────────┬────────┘
                                 │
                    ┌────────────┴────────────┐
                    │                         │
             Switch DOWN              Switch UP
            (Performance)              (Config)
                    │                         │
        ┌───────────┴───────────┐   ┌────────┴──────────┐
        │                       │   │                   │
   Button OFF            Button HELD   Button OFF       Button HELD
    (Primary)             (Shift)       (Primary)        (Shift)
        │                       │   │                   │
        │                       │   │                   │
K1: Anchor Dens      K1: Anchor Acc  K1: Terrain    K1: Swing Taste
K2: Shimmer Dens     K2: Shimmer Acc K2: Length     K2: Gate Time
K3: Flux             K3: Orbit       K3: Grid       K3: Humanize
K4: Fuse             K4: Contour     K4: Tempo      K4: Clock Div
```

---

## CV Input Routing (Always Active)

```
┌─────────────────────────────────────────────────────────────────┐
│                     CV INPUT ROUTING                             │
│                  (Independent of Mode)                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   CV 5 (0-5V) ────────────────────┐                             │
│                                    │                             │
│   CV 6 (0-5V) ───────────────┐     │                             │
│                               │     │                             │
│   CV 7 (0-5V) ──────────┐     │     │                             │
│                          │     │     │                             │
│   CV 8 (0-5V) ─────┐     │     │     │                             │
│                     │     │     │     │                             │
│                     ▼     ▼     ▼     ▼                             │
│               ┌─────────────────────────┐                        │
│               │   PERFORMANCE ENGINE    │                        │
│               │                         │                        │
│               │  FUSE ◄── CV8 (additive)│                        │
│               │  FLUX ◄── CV7 (additive)│                        │
│               │  SHIM ◄── CV6 (additive)│                        │
│               │  ANCH ◄── CV5 (additive)│                        │
│               │                         │                        │
│               └─────────────────────────┘                        │
│                                                                  │
│   NOTE: Knob values + CV = Final performance parameters          │
│         This works regardless of Performance/Config mode         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Output Mapping

```
┌─────────────────────────────────────────────────────────────────┐
│                        OUTPUTS                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  GATE OUT 1 ────► Anchor Trigger (0/5V digital)                 │
│                                                                  │
│  GATE OUT 2 ────► Shimmer Trigger (0/5V digital)                │
│                                                                  │
│  AUDIO OUT 1 ───► Anchor CV (0-5V, CONTOUR-dependent)           │
│                   Default: Velocity                              │
│                                                                  │
│  AUDIO OUT 2 ───► Shimmer CV (0-5V, CONTOUR-dependent)          │
│                   Default: Velocity                              │
│                                                                  │
│  CV OUT 1 ──────► Clock Output (respects SWING timing!)         │
│                                                                  │
│  CV OUT 2 ──────► LED / Visual Feedback                         │
│                   - Performance: pulses on Anchor                │
│                   - Config: solid ON                             │
│                   - Knob turn: brightness = value                │
│                   - Fill active: rapid flash                     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## TERRAIN Genre Reference

```
┌─────────────────────────────────────────────────────────────────┐
│                     TERRAIN (K1 Config)                          │
├───────────┬──────────────┬────────────────┬─────────────────────┤
│   Range   │    Genre     │   Swing Base   │     Character       │
├───────────┼──────────────┼────────────────┼─────────────────────┤
│   0-25%   │   TECHNO     │    52-57%      │ Straight, driving   │
│           │              │                │ Minimal swing       │
├───────────┼──────────────┼────────────────┼─────────────────────┤
│  25-50%   │   TRIBAL     │    56-62%      │ Circular groove     │
│           │              │                │ Percussive feel     │
├───────────┼──────────────┼────────────────┼─────────────────────┤
│  50-75%   │  TRIP-HOP    │    60-68%      │ Lazy, behind-beat   │
│           │              │                │ Heavy swing         │
├───────────┼──────────────┼────────────────┼─────────────────────┤
│  75-100%  │    IDM       │  54-65%+jitter │ Broken, glitchy     │
│           │              │                │ Micro-timing chaos  │
└───────────┴──────────────┴────────────────┴─────────────────────┘
```

---

## ORBIT Voice Relationship Reference

```
┌─────────────────────────────────────────────────────────────────┐
│                    ORBIT (K3+Shift Perf)                         │
├───────────┬──────────────┬──────────────────────────────────────┤
│   Range   │     Mode     │            Behavior                  │
├───────────┼──────────────┼──────────────────────────────────────┤
│   0-33%   │  INTERLOCK   │ Shimmer fills gaps in Anchor         │
│           │              │ Call-and-response feel               │
│           │              │ Shimmer hit prob ↑ when Anchor silent│
├───────────┼──────────────┼──────────────────────────────────────┤
│  33-67%   │    FREE      │ Independent patterns                 │
│           │              │ No collision logic                   │
│           │              │ Can create polyrhythmic feel         │
├───────────┼──────────────┼──────────────────────────────────────┤
│  67-100%  │   SHADOW     │ Shimmer echoes Anchor with delay     │
│           │              │ Creates doubling/echo effect         │
│           │              │ Delay = 1 step (or swing-based)      │
└───────────┴──────────────┴──────────────────────────────────────┘
```

---

## CONTOUR CV Mode Reference

```
┌─────────────────────────────────────────────────────────────────┐
│                   CONTOUR (K4+Shift Perf)                        │
├───────────┬──────────────┬──────────────────────────────────────┤
│   Range   │     Mode     │           CV Output                  │
├───────────┼──────────────┼──────────────────────────────────────┤
│   0-25%   │   VELOCITY   │ 0-5V proportional to hit intensity   │
│           │              │ Accent = high voltage                │
│           │              │ Ghost = low voltage                  │
├───────────┼──────────────┼──────────────────────────────────────┤
│  25-50%   │    DECAY     │ CV hints decay time to downstream    │
│           │              │ Accent = long decay                  │
│           │              │ Ghost = short decay                  │
├───────────┼──────────────┼──────────────────────────────────────┤
│  50-75%   │    PITCH     │ CV = pitch offset per hit            │
│           │              │ Random within scaled range           │
│           │              │ For melodic percussion               │
├───────────┼──────────────┼──────────────────────────────────────┤
│  75-100%  │   RANDOM     │ Sample & Hold random voltage         │
│           │              │ New value each trigger               │
│           │              │ For modulation/chaos                 │
└───────────┴──────────────┴──────────────────────────────────────┘
```

---

## CV-Driven Fills

Fills emerge naturally from high FLUX values—no dedicated trigger needed.

```
┌─────────────────────────────────────────────────────────────────┐
│                    CV-DRIVEN FILL BEHAVIOR                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  FLUX (K3 Performance) controls variation/fills:                │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                                                              ││
│  │  0-20%:   Clean, minimal pattern                            ││
│  │  20-50%:  Some ghost notes, subtle variation                ││
│  │  50-70%:  Active fills, velocity swells                     ││
│  │  70-90%:  Busy, lots of ghost notes                         ││
│  │  90-100%: Maximum chaos, fill on every opportunity          ││
│  │                                                              ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  CV 7 modulates FLUX (additive):                                │
│  • Patch pressure plate → CV 7 → Press harder = more fills     │
│  • Patch slow LFO → CV 7 → Periodic intensity swells           │
│  • Patch envelope follower → CV 7 → Audio peaks = fills        │
│  • Patch random/S&H → CV 7 → Occasional random fills           │
│                                                                  │
│  GATE INPUTS:                                                    │
│  • Gate In 1 = Clock (external clock input)                     │
│  • Gate In 2 = Reset (return to step 0)                         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Quick Performance Recipes

### Recipe 1: Minimal Techno

```
TERRAIN:      0% (Techno)
ANCHOR DENS:  20-30% (sparse kicks)
SHIMMER DENS: 40-50% (steady hats)
FLUX:         10-20% (minimal variation)
FUSE:         40% (slight anchor emphasis)
SWING TASTE:  Low (nearly straight)
```

### Recipe 2: Rolling Tribal

```
TERRAIN:      35% (Tribal)
ANCHOR DENS:  40% (driving pulse)
SHIMMER DENS: 60% (busy percussion)
FLUX:         40% (some fills)
FUSE:         50% (balanced)
ORBIT:        20% (interlock)
SWING TASTE:  High (pronounced swing)
```

### Recipe 3: Lazy Trip-Hop

```
TERRAIN:      60% (Trip-Hop)
ANCHOR DENS:  25% (sparse, heavy)
SHIMMER DENS: 30% (sparse snare/hat)
FLUX:         30% (subtle ghosts)
FUSE:         45% (anchor-biased)
SWING TASTE:  High (very drunk)
ORBIT:        40% (free)
```

### Recipe 4: Glitchy IDM

```
TERRAIN:      90% (IDM)
ANCHOR DENS:  50% (irregular kicks)
SHIMMER DENS: 70% (scattered hits)
FLUX:         70-90% (high chaos)
FUSE:         55% (shimmer-biased)
ORBIT:        50% (free/random)
SWING TASTE:  Mid (with micro-jitter)
```

---

## Phrase Structure Reference

Longer patterns have automatic phrase-aware composition:

### 4-Bar Phrase Structure

```
┌─── Bar 1 ───┬─── Bar 2 ───┬─── Bar 3 ───┬─── Bar 4 ───┐
│             │             │             │    FILL     │
│  ANCHOR     │   STEADY    │   BUILD     │    ZONE     │
│  DOWNBEAT   │             │             │             │
└─────────────┴─────────────┴─────────────┴─────────────┘

Fill Prob:    ░░░░░░░░░░░░░░░░░░░░░░░░░░▓▓▓▓▓▓▓▓████████
Ghost Notes:  ░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓██████████████████
Syncopation:  ░░░░░░░░░░░░░░░░▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓

░ = minimal  ▒ = low  ▓ = medium  █ = high
```

### Zone Lengths by Pattern Length

| LENGTH | Fill Zone | Build Zone | Notes |
|--------|-----------|------------|-------|
| 1 bar | Steps 12-15 | Steps 8-15 | Every bar has mini-arc |
| 2 bars | Last 4 steps | Last 8 steps | Quick build/release |
| 4 bars | Last 8 steps | Last 16 steps | Standard phrase |
| 8 bars | Last 16 steps | Last 32 steps | Long-form tension |
| 16 bars | Last 32 steps | Last 64 steps | Epic builds, DJ-friendly |

### Genre Phrase Intensity

| TERRAIN | Phrase Modulation | Character |
|---------|-------------------|-----------|
| Techno | 50% (subtle) | Minimal build, focused fill |
| Tribal | 120% (pronounced) | Cascading, rolling momentum |
| Trip-Hop | 70% (sparse) | Dropout-based tension |
| IDM | 150% (extreme) | Chaotic builds, jarring resets |

