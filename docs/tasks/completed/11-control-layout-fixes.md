---
id: chronick/daisysp-idm-grids-11
title: "Control Layout Fixes & Bug Fixes"
status: "completed"
created_date: "2025-12-03"
last_updated: "2025-12-16"
owner: "user/ai"
---

# Feature: Control Layout Fixes [control-layout-fixes]

## Context

Three issues need addressing in DuoPulse v2:

1. **Mode Switching Bug**: Switching between Config and Performance modes appears to alter parameter values. Parameters should persist independently per mode/shift combination, with soft pickup engaging when returning to a mode.

2. **FLUX/FUSE Investigation**: Users report insufficient variation when adjusting FLUX and FUSE knobs. Need to verify these parameters are actually affecting pattern generation as expected.

3. **Control Layout Reorganization**: Move less-frequently-adjusted parameters to Config mode, and move structural/performance parameters to Performance shift layer for better live playability.

## Tasks

### Phase 1: Bug Fixes

- [x] **Investigate mode switching parameter persistence** — Debug why switching modes alters parameters. Each of the 16 slots (4 modes × 4 knobs) should maintain independent state. Check SoftKnob state management, mode detection logic, and parameter loading on mode switch. Reference: `docs/specs/main.md` section "Mode Switching Behavior [control-layout-fixes]".
  - *Completed 2025-12-03*: Found bug in SoftKnob.cpp - interpolation was running continuously even when knob stationary, causing parameter drift on mode switch.

- [x] **Fix soft pickup on mode return** — Ensure returning to a mode correctly restores soft pickup state. The knob should not immediately take over—it should wait for cross-detection or gradual interpolation. Reference: `docs/specs/main.md` section "Soft Takeover [duopulse-soft-pickup]".
  - *Completed 2025-12-03*: Fixed in SoftKnob.cpp - interpolation now only occurs when knob is actively moved (delta > threshold). Tests updated.

- [x] **Investigate FLUX parameter effectiveness** — Trace FLUX parameter through the code path: main.cpp → Sequencer::SetFlux() → pattern generation. Verify ghost note probability, fill chance, and velocity jitter are actually being applied. Add debug output if needed. Reference: `docs/specs/main.md` section "FLUX Application".
  - *Completed 2025-12-03*: Code is correct. FLUX provides: ±35% random density bias via ChaosModulator, ghost note triggers for ghost-level steps, fills at FLUX>=50% (up to 30% prob), velocity jitter up to ±20%. Effects ARE being applied - may need hardware verification for perceptual tuning.

- [x] **Investigate FUSE parameter effectiveness** — Trace FUSE parameter through the code path. Verify fuseBias is being calculated and applied to anchor/shimmer density. The ±15% tilt should be audible. Reference: `docs/specs/main.md` section "FUSE Application".
  - *Completed 2025-12-03*: Code is correct. FUSE applies ±15% density tilt (CCW boosts anchor, CW boosts shimmer). Effect should be audible but depends on pattern intensity levels - high-intensity steps (11-15) fire regardless of density.

### Phase 2: Control Layout Reorganization

- [x] **Update control mapping in main.cpp** — Swap control assignments per new layout:
  - Performance Shift: K1=TERRAIN, K2=LENGTH, K3=GRID, K4=ORBIT
  - Config Primary: K1=ANCHOR ACCENT, K2=SHIMMER ACCENT, K3=CONTOUR, K4=TEMPO
  Reference: `docs/specs/main.md` section "Control System [duopulse-controls]".
  - *Completed 2025-12-03*: Updated GetParameterPtr() and SoftKnob initialization in main.cpp. Header comments updated.

- [x] **Update SoftKnob slot indices** — Ensure the 16 SoftKnob slots map correctly to the new parameter assignments. Slot numbering must match new layout.
  - *Completed 2025-12-03*: SoftKnob array indices 4-7 now init with terrain/length/grid/orbit, indices 8-11 now init with anchorAccent/shimmerAccent/contour/tempo.

- [x] **Update LED feedback if needed** — Verify LED behavior still makes sense with new control layout (e.g., parameter value display for 1 second on knob turn).
  - *Completed 2025-12-03*: LED logic unchanged - shows parameter value for 1s on any knob turn, regardless of which parameter.

### Phase 3: Testing & Verification

- [ ] **Manual test: Mode switching persistence** — Switch between modes repeatedly, verify no parameter jumps or value changes. Reference: `docs/tasks/completed/10-duopulse-v2.md` Test 12.

- [ ] **Manual test: FLUX range** — Sweep FLUX from 0-100% and verify audible variation at each 25% increment. Ghost notes and fills should emerge progressively.

- [ ] **Manual test: FUSE range** — Sweep FUSE from 0-100% and verify energy tilt. CCW should sound anchor-heavy, CW should sound shimmer-heavy.

- [ ] **Manual test: New control layout** — Verify all 16 parameters accessible in correct modes per updated spec.

- [x] **Update task file test checklist** — Update `docs/tasks/completed/10-duopulse-v2.md` test tables to reflect new control layout. *Completed 2025-12-03 during spec updates.*

## New Control Layout Summary

### Performance Mode (Switch DOWN)

|    | Primary | +Shift |
|----|---------|--------|
| K1 | ANCHOR DENSITY | TERRAIN |
| K2 | SHIMMER DENSITY | LENGTH |
| K3 | FLUX | GRID |
| K4 | FUSE | ORBIT |

### Config Mode (Switch UP)

|    | Primary | +Shift |
|----|---------|--------|
| K1 | ANCHOR ACCENT | SWING TASTE |
| K2 | SHIMMER ACCENT | GATE TIME |
| K3 | CONTOUR | HUMANIZE |
| K4 | TEMPO | CLOCK DIV |

## Rationale

- **Terrain, Length, Grid** moved to Performance Shift because these are structural parameters you'd want to change during a live set (genre, phrase length, pattern selection).
- **Anchor/Shimmer Accent, Contour** moved to Config because these are "set and forget" expression settings—you dial in your dynamic feel once and leave it.
- **ORBIT** stays in Shift layer (now Performance Shift) as voice relationship is a performance choice.

## Files to Modify

- `src/main.cpp` — Control routing, SoftKnob slot assignments
- `src/Engine/Sequencer.h/cpp` — Verify FLUX/FUSE implementation
- `docs/specs/main.md` — Already updated
- `docs/tasks/completed/10-duopulse-v2.md` — Test tables already updated

## Estimated Effort

| Task | Hours |
|------|-------|
| Bug investigation & fixes | 2-4 |
| Control layout reorganization | 1-2 |
| Testing & verification | 1-2 |
| **Total** | **4-8 hours** |

## Comments
- 2025-12-03: Created task from user bug reports and control layout change request.
- 2025-12-03: Phase 1 & 2 complete. SoftKnob bug fixed (now only interpolates when knob moved). Control layout updated. FLUX/FUSE code verified correct - may need hardware tuning for perceptual impact. Phase 3 manual testing remains.
- 2025-12-03: **CV modulation bug fix** — Audio Out CV wasn't working because CV modulation used `(cv - 0.5f)` offset, assuming unpatched inputs read 0.5V. When inputs read 0V instead, density became 0 and no triggers fired. Fixed by using `MixControl()` (additive, 0V = no effect) to match main branch behavior.
- 2025-12-16: **Task completed** — All implementation work done. Spec updated with 32/34 acceptance criteria checked off (94% complete). Two remaining TODOs for future enhancement: (1) Differential swing (Anchor 70%, Shimmer 100%), (2) SetLength step wrapping. Manual hardware tests remain but don't block completion.

