# Legacy Design Documentation

Historical design documentation from previous DuoPulse versions. Preserved for reference and understanding the evolution of the project.

## Version History

| Version | Status | Key Features |
|---------|--------|--------------|
| V5 (current) | Active | SHAPE-based generation, COMPLEMENT relationship, simplified 8-knob interface |
| V4 | Superseded | 2D pattern field, archetype grid, 12+ parameters |
| V3 | Superseded | Pulse field, BROKEN/DRIFT controls |
| V2 | Superseded | Genre-aware swing, phrase structure |
| V1 | Superseded | Basic Grids port |

## Contents

### [design-versions-summary.md](design-versions-summary.md)

Comprehensive summary of all design versions and their evolution. Good starting point for understanding the project history.

### [v4/](v4/)

Complete V4 specification and architecture documentation:
- `duopulse-v4-spec_1.md` - Full V4 specification
- `duopulse-v4-architecture.md` - System architecture
- `duopulse-v4-detailed.md` - Detailed implementation notes
- `duopulse-v4-visual-guide.md` - Visual reference guide

V4 introduced the 2D pattern field with 3x3 archetype grid and complex parameter relationships. Superseded by V5's simpler SHAPE-based approach.

### [v3/](v3/)

V3 documentation is primarily captured in task files:
- [Task 13: Pulse Field V3](../../tasks/completed/13-pulse-field-v3.md)
- [Task 14: V3 Ratchet Fixes](../../tasks/completed/14-v3-ratchet-fixes.md)

V3 introduced the weighted pulse field concept and BROKEN/DRIFT controls.

### [early-concepts/](early-concepts/)

Early brainstorming and alternative design explorations from before the current architecture was established. Includes contributions from multiple AI models during ideation.

## Key Transitions

### V4 → V5 (Tasks 27-45)

Major simplification:
- Removed: GENRE, BALANCE, BUILD, PUNCH parameters
- Added: SHAPE (3-zone blending), ACCENT (velocity dynamics)
- Changed: Archetype lookup → procedural generation
- Changed: Multiple coupling modes → COMPLEMENT only

### V3 → V4 (Task 15)

Architecture overhaul:
- Added: 2D pattern field with X/Y navigation
- Added: Gumbel sampling for hit selection
- Added: Hit budget system
- Removed: Direct pattern manipulation

## See Also

- [Current V5 Specification](../../specs/)
- [V5 Design Process](../v5-ideation/)
