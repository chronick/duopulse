---
id: 27
slug: 27-v5-control-renaming
title: "V5 Control Renaming and Zero Shift Layers"
status: pending
created_date: 2026-01-04
updated_date: 2026-01-04
branch: feature/27-v5-control-renaming
spec_refs:
  - "v5-design-final.md#control-layout"
  - "v5-design-final.md#button--switch-behavior"
---

# Task 27: V5 Control Renaming and Zero Shift Layers

## Objective

Rename V4 control parameters to V5 vocabulary and eliminate all shift layers, establishing the foundation for the V5 control interface.

## Context

V5 simplifies the control interface by:
1. Renaming parameters to match V5 design (FIELD X/Y → AXIS X/Y, BUILD → SHAPE, PUNCH → ACCENT, etc.)
2. Eliminating all shift layers - each mode has 4 direct knob mappings
3. Implementing knob pairing concept (related functions across Performance/Config modes)

### V5 Control Mapping

**Performance Mode**:
- K1: ENERGY (was: ENERGY) - unchanged semantics
- K2: SHAPE (was: BUILD) - new 3-way blending semantics (Task 28)
- K3: AXIS X (was: FIELD X) - renamed, new bidirectional semantics (Task 29)
- K4: AXIS Y (was: FIELD Y) - renamed, new intricacy semantics (Task 29)

**Config Mode**:
- K1: CLOCK DIV (was: shift+K2) - moved to primary
- K2: SWING (was: K2) - unchanged
- K3: DRIFT (was: shift+K3 perf) - moved from perf shift to config
- K4: ACCENT (was: PUNCH) - renamed, moved from perf shift

### Removed Controls (V4 → V5)
- GENRE (shift+K2 perf) - removed, algorithm handles style
- BALANCE (shift+K4 perf) - removed, BUILD replaces
- PATTERN LENGTH (config K1) - moved to boot-time (Task 33)
- PHRASE LENGTH (config shift+K1) - auto-derived
- AUX MODE (config K3) - moved to gesture (Task 32)
- AUX DENSITY (config shift+K3) - removed
- RESET MODE (config K4) - removed
- VOICE COUPLING (config shift+K4) - replaced by COMPLEMENT

## Subtasks

- [ ] Rename `fieldX`/`fieldY` to `axisX`/`axisY` in ControlState.h
- [ ] Rename `build` to `shape` in ControlState.h
- [ ] Rename `punch` to `accent` in ControlState.h (move from shift to config)
- [ ] Add `clockDiv` as config K1 (already exists as shift+K2)
- [ ] Move `drift` from perf shift to config K3
- [ ] Remove `genre` enum and all references (hardcode to TECHNO behavior)
- [ ] Remove `balance` parameter
- [ ] Remove `auxDensity` enum
- [ ] Remove `voiceCoupling` enum (replaced by COMPLEMENT in Task 30)
- [ ] Remove shift layer processing from ControlProcessor.cpp
- [ ] Update CV modulation to target performance params only (CV1-4 → ENERGY, SHAPE, AXIS X, AXIS Y)
- [ ] Simplify button behavior: Tap = Fill, Hold 3s = Reseed (no shift)
- [ ] Update boot defaults to V5 spec
- [ ] All tests pass (`make test`)

## Acceptance Criteria

- [ ] Zero shift layers - button hold no longer activates shift mode
- [ ] All 8 parameters accessible directly (4 perf + 4 config)
- [ ] CV1-4 modulate ENERGY, SHAPE, AXIS X, AXIS Y in both modes
- [ ] Build compiles without errors
- [ ] All existing tests pass (with parameter name updates)
- [ ] Button tap (20-500ms) triggers fill
- [ ] Button hold (3s) triggers reseed on release
- [ ] No new compiler warnings

## Implementation Notes

### Files to Modify

- `src/Engine/ControlState.h` - Rename parameters, update struct
- `src/Engine/DuoPulseTypes.h` - Remove Genre, AuxDensity, VoiceCoupling enums
- `src/Engine/ControlProcessor.h` - Remove shift knob arrays
- `src/Engine/ControlProcessor.cpp` - Simplify to 2 contexts (perf/config only)
- `src/Engine/PatternField.cpp` - Update parameter references
- `src/Engine/VelocityCompute.cpp` - Update punch → accent references
- `src/main.cpp` - Update control flow, remove shift handling
- `tests/*.cpp` - Update all test parameter names

### V5 Boot Defaults

| Parameter | Default | Rationale |
|-----------|---------|-----------|
| ENERGY | 50% | Neutral density |
| SHAPE | 30% | Humanized euclidean zone |
| AXIS X | 50% | Neutral beat position |
| AXIS Y | 50% | Moderate intricacy |
| CLOCK DIV | ×1 | No division |
| SWING | 50% | Neutral groove |
| DRIFT | 0% | Locked pattern |
| ACCENT | 50% | Moderate dynamics |

### CV Response

CV modulation is instant with pattern regeneration per-phrase (not per-sample). This matches V4 behavior.

### CLOCK DIV Range

CLOCK DIV (Config K1) maps knob position to tempo multiplier:
- 0-25%: ÷4 (quarter speed)
- 25-50%: ÷2 (half speed)
- 50-75%: ×1 (normal, default)
- 75-100%: ×4 (quadruple speed)

### Constraints

- Maintain real-time audio safety
- Keep backward compatibility where reasonable (internal names can change)
- This task establishes vocabulary for all subsequent V5 tasks

### Risks

- Large refactor touching many files
- May temporarily break functionality until related tasks (28-35) complete
- Tests will need significant updates

## Testing

```bash
make test  # All unit tests pass
make       # Firmware compiles
```

Manual validation:
- Knobs respond directly without shift
- CV1-4 modulate performance parameters
- Button tap triggers fill
- Button hold 3s triggers reseed
