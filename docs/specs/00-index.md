# DuoPulse v5: Algorithmic Drum Sequencer Specification

**Target Platform**: Daisy Patch.init() (Electro-Smith)
**Version**: 5.0
**Status**: Complete
**Last Updated**: 2026-01-09

---

## Specification Index

| Section | File | Description |
|---------|------|-------------|
| 01 | [Quick Reference](01-quick-reference.md) | I/O summary, control layouts, CV mapping |
| 02 | [Core Concepts](02-core-concepts.md) | Design philosophy, v4→v5 changes, knob pairing |
| 03 | [Architecture](03-architecture.md) | Pipeline overview, processing flow |
| 04 | [Hardware Interface](04-hardware.md) | Patch.init() hardware map, CV behavior |
| 05 | [Control System](05-controls.md) | Modes, parameters, button gestures |
| 06 | [SHAPE Algorithm](06-shape.md) | 3-zone blending, hit budget modulation |
| 07 | [AXIS Biasing](07-axis.md) | Beat position, intricacy, broken mode |
| 08 | [Voice Relationship](08-complement.md) | COMPLEMENT gap-filling, DRIFT placement |
| 09 | [ACCENT Velocity](09-accent.md) | Metric weight velocity, dynamic range |
| 10 | [Fill System](10-fill.md) | Triggers, modifiers, density curves |
| 11 | [AUX Output](11-aux.md) | Fill gate, hat burst generation |
| 12 | [LED Feedback](12-led.md) | 5-layer priority system |
| 13 | [Boot Behavior](13-boot.md) | Defaults, AUX mode gesture, knob reading |
| 14 | [Clock and Reset](14-clock.md) | Clock source selection, reset behavior |
| 15 | [Testing](15-testing.md) | Unit tests, integration tests, validation |
| A1 | [Style Presets](A1-presets.md) | Genre preset maps |
| A2 | [Glossary](A2-glossary.md) | Term definitions |

---

## Pending Changes (Active Tasks)

| Task | Spec Section | Change |
|------|--------------|--------|
| **Task 46** | SHAPE Algorithm | V5.5 noise formula fix |
| **Task 47** | ACCENT Velocity | V5.5 velocity variation (ghost notes) |
| **Task 48** | Generation Pipeline | V5.5 micro-displacement |
| **Task 49** | AUX Output | V5.5 AUX style zones, beat 1 enforcement |
| **Task 50** | Generation Pipeline | V5.5 two-pass generation (conditional) |

## Recent Changes

| Task | Date | Changes |
|------|------|---------|
| **Task 45** | 2026-01-08 | Pattern generator extraction (shared firmware/viz module) |
| **Task 44** | 2026-01-07 | Anchor seed variation fix |
| **Task 43** | 2026-01-07 | Shimmer seed variation fix |
| **Task 42** | 2026-01-07 | V4 dead code cleanup (~2,630 lines removed) |
| **Task 41** | 2026-01-06 | SHAPE budget zone boundaries fix |
| **Task 40** | 2026-01-06 | PatternGenerator class extraction |
| **Task 39** | 2026-01-06 | SHAPE-modulated hit budget for pattern variation |
| **Task 38** | 2026-01-06 | C++ pattern visualization test tool |
| **Task 37** | 2026-01-04 | Code review critical fixes (bounds, null checks) |
| **Task 35** | 2026-01-04 | ACCENT parameter with metric weight velocity |
| **Task 34** | 2026-01-04 | 5-layer priority LED feedback system |
| **Task 33** | 2026-01-04 | Boot-time AUX mode selection gesture |
| **Task 32** | 2026-01-04 | Hold+Switch gesture for runtime AUX mode |
| **Task 31** | 2026-01-04 | Hat burst pattern-aware fill triggers |
| **Task 30** | 2026-01-04 | Voice COMPLEMENT relationship with DRIFT |
| **Task 29** | 2026-01-04 | AXIS X/Y bidirectional biasing |
| **Task 28** | 2026-01-04 | SHAPE 3-way blending algorithm |
| **Task 27** | 2026-01-04 | Control renaming, zero shift layers |

---

*For the complete specification in a single file, see [main.md](main.md) (legacy).*
