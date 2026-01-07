---
id: 42
slug: 42-v4-dead-code-cleanup
title: "V4 Dead Code Cleanup: Remove Deprecated Archetype System"
status: completed
created_date: 2026-01-06
updated_date: 2026-01-07
completed_date: 2026-01-07
branch: feature/42-v4-dead-code-cleanup
commits:
  - d9154f8
spec_refs:
  - "V5 specification (archetype system deprecated)"
---

# Task 42: V4 Dead Code Cleanup

## Objective

Remove ~2,551 lines of dead V4 archetype code that was deprecated by Task 40's V5 pattern generator implementation.

## Background

Task 40 replaced the V4 archetype blending system with V5's procedural SHAPE-based generation. The old archetype files remain in the codebase but are never called. This cleanup will:

- Reduce binary size
- Simplify codebase maintenance
- Remove misleading v4 references in comments

## Subtasks

### Delete Unused Files (~1,452 lines)

- [ ] Delete `src/Engine/ArchetypeData.h` (~1,001 lines)
- [ ] Delete `src/Engine/ArchetypeData.cpp` (~243 lines)
- [ ] Delete `src/Engine/ArchetypeDNA.h` (~208 lines)

### Remove Unused Functions from PatternField (~185 lines)

- [ ] Remove `BlendArchetypes()` (PatternField.cpp:173-230)
- [ ] Remove `GetBlendedArchetype()` (PatternField.cpp:242-265)
- [ ] Remove `InitializeGenreFields()` (PatternField.cpp:271-302)
- [ ] Remove `GetGenreField()` (PatternField.cpp:304-319)
- [ ] Remove `AreGenreFieldsInitialized()` (PatternField.cpp:321-324)
- [ ] Remove `InterpolateStepWeight()` (PatternField.cpp:140-170)
- [ ] Remove corresponding declarations from PatternField.h
- [ ] Remove helper functions: `FindDominantArchetype`, `SoftmaxWithTemperature`, `ComputeGridWeights`

### Remove Unused State Members

- [ ] Remove `GenreField currentField;` from DuoPulseState.h:40
- [ ] Remove `ArchetypeDNA blendedArchetype;` from DuoPulseState.h:43
- [ ] Remove their Init() calls from DuoPulseState.h:87-88
- [ ] Remove `#include "ArchetypeDNA.h"` from DuoPulseState.h

### Remove Unused Sequencer Code

- [ ] Remove `BlendArchetype()` declaration from Sequencer.h:300
- [ ] Remove `BlendArchetype()` implementation from Sequencer.cpp:1131-1134
- [ ] Remove `GetBlendedAnchorWeight()` debug method from Sequencer.h:264-266

### Update Comments from V4 to V5

- [ ] main.cpp:2 - Update file header to "DuoPulse v5"
- [ ] main.cpp:161 - Update K3 comment to describe SHAPE zones (not archetypes)
- [ ] main.cpp:402 - Update "DuoPulse v4 Control Layout" comment
- [ ] main.cpp:638 - Update "Apply all DuoPulse v4 parameters" comment
- [ ] main.cpp:773 - Update LOGI message to "DuoPulse v5 boot"
- [ ] main.cpp:868 - Update control layout comment

### Clean Up DuoPulseTypes.h

- [ ] Remove `kArchetypesPerGenre` constant (line 32)
- [ ] Update/remove archetype-related comments (lines 45-47)
- [ ] Remove unused `GetDefaultGenre()` function (lines 213-218)

### Final Verification

- [ ] Build succeeds (`make clean && make`)
- [ ] All tests pass (`make test`)
- [ ] No new compiler warnings
- [ ] Verify flash usage decreased

## Acceptance Criteria

- [ ] All archetype-related files deleted (ArchetypeData.h/cpp, ArchetypeDNA.h)
- [ ] No references to "archetype" remain in active code paths
- [ ] All "v4" version references updated to "v5" in comments
- [ ] Build succeeds with no warnings
- [ ] All 362+ tests pass
- [ ] Flash usage reduced (expect ~2-5KB savings)

## Implementation Notes

### Files to Delete

| File | Lines | Purpose (deprecated) |
|------|-------|---------------------|
| `src/Engine/ArchetypeData.h` | ~1,001 | V4 archetype weight tables |
| `src/Engine/ArchetypeData.cpp` | ~243 | V4 archetype loader |
| `src/Engine/ArchetypeDNA.h` | ~208 | V4 archetype data structure |

### Files to Modify

| File | Changes |
|------|---------|
| `src/Engine/PatternField.h` | Remove ~8 function declarations |
| `src/Engine/PatternField.cpp` | Remove ~185 lines of blending functions |
| `src/Engine/DuoPulseState.h` | Remove 2 members, 1 include |
| `src/Engine/Sequencer.h` | Remove 2 method declarations |
| `src/Engine/Sequencer.cpp` | Remove ~5 lines |
| `src/Engine/DuoPulseTypes.h` | Remove constants, clean comments |
| `src/main.cpp` | Update 6 comment locations |

### Verification Commands

```bash
# Check for remaining archetype references
grep -r "archetype" src/ --include="*.cpp" --include="*.h" | grep -v "DEPRECATED"

# Check flash usage before/after
make clean && make 2>&1 | grep "FLASH"
```

### Constraints

- Keep the `Genre` enum (still used by EuclideanGen)
- Keep any archetype references in test files if tests still validate behavior
- Don't break any currently passing tests

### Risks

- May need to update Makefile if it explicitly lists deleted files
- Some test files might reference archetype structures
