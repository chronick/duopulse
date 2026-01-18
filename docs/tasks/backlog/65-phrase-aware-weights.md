---
id: 65
slug: phrase-aware-weights
title: "Phrase-Aware Weight Modulation"
status: backlog
created_date: 2026-01-18
updated_date: 2026-01-18
branch: feature/phrase-aware-weights
spec_refs:
  - "docs/specs/main.md#5-shape-algorithm"
depends_on:
  - 56  # Weight-based blending (foundation)
  - 55  # Iteration command (for testing)
---

# Task 65: Phrase-Aware Weight Modulation

## Objective

Add per-section weight variation to the pattern generation algorithm, allowing different weight profiles for intro/verse/chorus sections to create more musical phrase structure.

## Context

### Background

This task was split from Task 56 during design review (2026-01-18). The original Task 56 included "per-section variation" which is a major architectural change requiring API modifications to `PatternParams`. It's more appropriate as a follow-up task after the core weight system is proven.

### Current State (After Task 56)

- Algorithm weights explicitly computed from SHAPE parameter
- Euclidean/syncopation/random blend controlled by config
- No awareness of phrase position or section type
- `PatternParams` has no section/phrase field

### Target State

- `PatternParams` extended with `section` field (intro/verse/chorus/fill)
- Weight profiles vary by section:
  - Intro: Sparse euclidean, minimal syncopation
  - Verse: Standard weights (current behavior)
  - Chorus: Boosted density, more syncopation
  - Fill: Maximum syncopation, high activity
- Section detection from phrase position or explicit input

## Design Considerations

### Option A: Phrase Position Inference

Automatically detect section from current position in phrase:
- Bars 1-2: Intro
- Bars 3-6: Verse
- Bars 7-8: Chorus

**Pros**: No API changes to user-facing controls
**Cons**: Rigid structure, may not match actual music

### Option B: Explicit Section Input

Add a CV input or control for section:
- AUX CV repurposed for section in certain modes
- Or: New digital input

**Pros**: User control over transitions
**Cons**: More complex control surface

### Option C: Automatic + Override

Use phrase position inference by default, but allow CV override when connected.

**Recommended**: Option C provides best of both worlds.

## Subtasks (Preliminary)

### API Changes
- [ ] Add `Section` enum (intro, verse, chorus, fill)
- [ ] Add `section` field to `PatternParams`
- [ ] Update pattern generator to read section

### Section Detection
- [ ] Implement phrase position → section mapping
- [ ] Add configurable phrase structure
- [ ] Handle section transitions smoothly

### Weight Profiles
- [ ] Define weight profile per section
- [ ] Implement profile interpolation at transitions
- [ ] Make profiles configurable

### CV Override (Optional)
- [ ] Add section CV input handling
- [ ] Map CV voltage to section
- [ ] Priority: CV override > auto-detect

### Tests
- [ ] Test section detection
- [ ] Test weight profile application
- [ ] Test smooth transitions
- [ ] All tests pass

## Acceptance Criteria (Preliminary)

- [ ] Patterns vary by phrase position
- [ ] Intro/verse/chorus have distinct character
- [ ] Transitions between sections are smooth
- [ ] Section behavior configurable
- [ ] Optional: CV control for section

## Why This Is Backlogged

1. **Prerequisite**: Task 56 must complete first (core weight system)
2. **Validation**: Need iteration system (Task 55) to tune section profiles
3. **Complexity**: API changes affect firmware, pattern_viz, and evals
4. **Low Urgency**: Current algorithm produces good patterns without sections

## Estimated Effort

4-6 hours (requires API changes and testing across tools)

## Notes

This is a "nice to have" enhancement. The core algorithm works well without phrase awareness. Consider implementing only after the hill-climbing iteration system is proven and stable.
