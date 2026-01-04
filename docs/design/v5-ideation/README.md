# Design Session: DuoPulse V5 Control Simplification

**Created**: 2026-01-03
**Status**: Active
**Focus**: Reducing control complexity while maintaining expressiveness

## Problem Statement

From user feedback (initial-feedback.md):

1. **Controls are unwieldy** - Config mode + shift for both modes increases complexity
2. **Too many modes/layers** - Need to simplify to just 4 performance + 4 config controls
3. **Voice coupling doesn't feel important** - Can be deprioritized or removed
4. **Button is occupied** - Want to free it for manual fills or performance toggle

## Key Design Goals

1. **Performance controls (4 total)**:
   - 1 for energy
   - 1 for regular/irregular morphing (euclidean ↔ musically-random)
   - 2 for navigation within algorithm paradigm

2. **Config controls (4 total)**:
   - Pattern length (1-bar vs song structure)
   - Aux mode selection

3. **Aux modes**: Simplified to hat + fill bar for now

4. **First-principles approach**: Not genre-specific, musically universal

## Constraints

- 4 knobs (K1-K4) hardware limit
- 1 button available
- 2 CV inputs
- Must remain performable without menu-diving
- Existing genre system can be modified/removed

## Success Criteria

- Performer can access all common operations without shift
- Config mode is rarely needed during performance
- Controls map intuitively to musical outcomes
- Complexity reduced without sacrificing core expressiveness

## Reference Documents

- `docs/misc/design-versions-summary.md` - v2/v3/v4 evolution
- `docs/design/v5-ideation/initial-feedback.md` - User feedback driving this redesign
