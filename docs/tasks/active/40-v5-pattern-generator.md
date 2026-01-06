---
id: 40
slug: 40-v5-pattern-generator
title: "V5 Pattern Generator Extraction & Spec Alignment"
status: pending
created_date: 2026-01-06
updated_date: 2026-01-06
branch: feature/40-v5-pattern-generator
spec_refs:
  - "v5-design-final.md"
  - "algorithm-appendix-final.md"
depends_on:
  - 28
  - 29
  - 38
  - 39
---

# Task 40: V5 Pattern Generator Extraction & Spec Alignment

## Objective

Extract shared `PatternGenerator` module and align firmware with V5 spec's SHAPE-based generation algorithm, replacing the V4 archetype system.

## Context

### Problem Discovery

Task 38 created a pattern visualization tool that uses V5 spec's SHAPE-based generation (`ComputeShapeBlendedWeights` + `ApplyAxisBias`). However, the firmware still uses the V4 archetype system (`BlendArchetype` + `GenreField`).

**V5 Spec** (Appendix A.2) defines SHAPE-based procedural generation:
- Three-zone blending (stable/syncopated/wild)
- 7-zone crossfade with overlap windows
- No archetype/GenreField system

**Current State**:
| Component | Weight Source | V5 Compliant? |
|-----------|---------------|---------------|
| Viz Tool (`pattern_viz.cpp`) | `ComputeShapeBlendedWeights` | YES |
| Firmware (`Sequencer.cpp`) | `BlendArchetype` | NO |

The viz tool is **more spec-compliant** than the firmware. This task unifies both to use V5 spec algorithm.

## Subtasks

### Phase A: V5 Weight Generation (~3h)
- [ ] Modify `Sequencer::GenerateBar()` to use `ComputeShapeBlendedWeights` + `ApplyAxisBias`
- [ ] Replace archetype-based weight generation directly
- [ ] Verify patterns via `pattern_viz` tool
- [ ] Run existing tests, fix any regressions

### Phase B: Extract PatternGenerator Module (~3h)
- [ ] Create `src/Engine/PatternGenerator.h` with interface
- [ ] Create `src/Engine/PatternGenerator.cpp` with implementation
- [ ] Create `tests/test_pattern_generator.cpp` unit tests
- [ ] Verify determinism (same seed = identical output)

### Phase C: Integrate & Deprecate (~2h)
- [ ] Update `Sequencer::GenerateBar()` to call `GeneratePattern()`
- [ ] Update `tools/pattern_viz.cpp` to call `GeneratePattern()`
- [ ] Remove `BlendArchetype()` calls from Sequencer
- [ ] Remove `currentField` from `DuoPulseState`

### Phase D: Cleanup (~1h)
- [ ] Update/remove archetype-related tests
- [ ] Archive or delete unused archetype code
- [ ] Update documentation

## Acceptance Criteria

- [ ] Firmware uses V5 SHAPE-based weight generation
- [ ] `PatternGenerator` module exists with clean interface
- [ ] Both Sequencer and viz tool call shared module
- [ ] Same inputs produce identical output in both
- [ ] Tests pass (`make test`)
- [ ] Build succeeds (`make`)

## Interface Design

```cpp
// src/Engine/PatternGenerator.h

struct PatternGenParams {
    float energy;
    float shape;
    float axisX;
    float axisY;
    float drift;
    float balance;
    int patternLength;
    EnergyZone zone;
    uint32_t seed;
    // Fill modifiers
    float densityMultiplier;
    bool inFillZone;
    float fillIntensity;
};

struct PatternGenResult {
    uint64_t anchorMask;
    uint64_t shimmerMask;
    uint64_t auxMask;
    int anchorHits;
    int shimmerHits;
    int auxHits;
};

// Main entry point
void GeneratePattern(const PatternGenParams& params, PatternGenResult& out);
```

## Files to Modify

**Core changes:**
- `src/Engine/Sequencer.cpp:264-525` - Use V5 weight generation
- `src/Engine/Sequencer.h` - Remove blendedArchetype references
- `src/Engine/DuoPulseState.h` - Remove BlendedArchetype member

**New files:**
- `src/Engine/PatternGenerator.h`
- `src/Engine/PatternGenerator.cpp`
- `tests/test_pattern_generator.cpp`

**Viz tool:**
- `tools/pattern_viz.cpp:90-154` - Call shared PatternGenerator

**Potentially remove:**
- Parts of `ArchetypeDNA.h`, `PatternField.cpp` related to archetype blending

## Archetype System Usage (for cleanup reference)

From codebase grep:
- `Sequencer.cpp` - 4 call sites to BlendArchetype()
- `DuoPulseState.h` - stores `currentField: GenreField`
- `PatternField.cpp/h` - BlendArchetypes(), GetBlendedArchetype() implementation
- `ArchetypeDNA.h` - GenreField struct definition
- `test_pattern_field.cpp` - ~20 archetype-related tests
- `test_duopulse_types.cpp` - GenreField tests

## V5 Spec Reference

From `v5-design-final.md` Appendix A.2:
- Zone 1 (0-28%): Stable humanized euclidean
- Crossfade (28-32%): Blend to syncopation
- Zone 2a (32-48%): Stable -> syncopated interpolation
- Crossfade (48-52%): Peak syncopation
- Zone 2b (52-68%): Syncopated -> wild interpolation
- Crossfade (68-72%): Blend to wild
- Zone 3 (72-100%): Wild with chaos injection

From `algorithm-appendix-final.md:894`:
> "GenreField 3x3 grid structure - replaced by SHAPE zones"

## Risk Mitigation

- Tests updated incrementally
- Archetype code preserved in git history
- Pattern viz tool validates output before/after

## Related Tasks

- Task 28: V5 SHAPE Algorithm (implemented the algorithm)
- Task 29: V5 Axis Biasing (implemented bias system)
- Task 38: Pattern Viz Tool (uses V5 algorithm correctly)
- Task 39: SHAPE Hit Budget (modulates density by SHAPE)

## References

- Plan file: `~/.claude/plans/lucky-cuddling-kay.md`
- V5 Design: `docs/design/v5-ideation/v5-design-final.md`
- Algorithm Appendix: `docs/design/v5-ideation/algorithm-appendix-final.md`
