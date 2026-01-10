# DuoPulse Second Iteration

## Overview

This directory contains the refined specification for **DuoPulse**, a 2-voice percussive sequencer synthesized from the best ideas in the initial brainstorming phase.

## Documents

| File | Description |
|------|-------------|
| **[specification.md](./specification.md)** | Complete specification with all parameters, behaviors, and rationale |
| **[control-map.md](./control-map.md)** | Visual control reference, quick recipes, parameter tables |
| **[implementation-roadmap.md](./implementation-roadmap.md)** | Phased implementation plan against current codebase |

## Key Features

### Abstract Terminology
- **Anchor** / **Shimmer** instead of Kick/Snare
- **Terrain** for genre character
- **Orbit** for voice relationships
- **Flux** for variation
- **Fuse** for energy balance

### Genre-Aware Swing
- Opinionated swing ranges per genre (Techno/Tribal/Trip-Hop/IDM)
- **Swing Taste** knob fine-tunes within the genre's valid range
- Clock output respects swing timing

### Performance-First Controls
- CV inputs **always** modulate performance parameters (regardless of mode)
- CV-driven fills: high FLUX values naturally create fill behavior
- Immediate parameter changes (no waiting for pattern boundaries)
- Gate In 1 = Clock, Gate In 2 = Reset (clean, dedicated functions)

### Phrase-Aware Pattern Composition
- Longer patterns (4, 8, 16 bars) have musical arc
- Fill probability increases toward phrase end
- Ghost notes and syncopation build through the phrase
- Genre-specific phrase behavior (techno: subtle, tribal: pronounced, trip-hop: sparse, IDM: chaotic)
- Fill zones and build zones scaled by pattern length

### Technical Specs
| Parameter | Value |
|-----------|-------|
| Tempo Range | 90-160 BPM |
| Pattern Length | 1, 2, 4, 8, 16 bars |
| Swing Range | 50-68% (genre-dependent) |
| Gate Time | 5-50ms |
| CV Range | 0-5V |

## Control Summary

### Performance Mode (Switch DOWN)

| Knob | Primary | +Shift |
|------|---------|--------|
| K1 | ANCHOR DENSITY | ANCHOR ACCENT |
| K2 | SHIMMER DENSITY | SHIMMER ACCENT |
| K3 | FLUX | ORBIT |
| K4 | FUSE | CONTOUR |

### Config Mode (Switch UP)

| Knob | Primary | +Shift |
|------|---------|--------|
| K1 | TERRAIN | SWING TASTE |
| K2 | LENGTH | GATE TIME |
| K3 | GRID | HUMANIZE |
| K4 | TEMPO | CLOCK DIV |

## Design Decisions

### From Initial Brainstorming

| Feature | Source | Rationale |
|---------|--------|-----------|
| Anchor/Shimmer naming | GPT-5 DuoPulse | Abstract terminology encourages modular thinking |
| Genre-aware swing | User requirement | Opinionated but adjustable—prevents bad musical choices |
| CV always performance | User requirement | Predictable CV behavior regardless of mode |
| CV-driven fills | Simplification | High FLUX = fills, no dedicated trigger needed |
| Orbit modes | Claude Opus 4 | Interlock/Free/Shadow gives voice interaction control |
| Contour CV modes | Claude Opus 4 | Velocity/Decay/Pitch/Random expression |
| Hybrid patterns | Multiple specs | Skeleton tables + algorithmic variation |
| Immediate changes | User requirement | Responsive feel, no waiting |

### Excluded Ideas

| Feature | Reason |
|---------|--------|
| Scene memory (4 slots) | Complexity—defer to future iteration |
| Subdivision control | Tempo range + pattern selection covers this |
| Ratchet mode | Can be achieved via CV + downstream modules |
| Pattern morphing | Deferred—immediate pattern switching is sufficient |

## Implementation Status

- ✅ Specification complete
- ✅ Control map defined
- ✅ Implementation roadmap created
- ⏳ Implementation pending

## Estimated Effort

~37-54 hours total implementation time (see [implementation-roadmap.md](./implementation-roadmap.md) for breakdown).

