# Two-Track Dual Sequencer Specification

## Overview

This directory contains the complete specification for the **Two-Track Dual Sequencer**, a redesigned drum sequencer that embraces the constraint of two trigger outputs and maximizes expressive control over two complementary rhythmic tracks.

## Problem Statement

The current sequencer attempts to map three internal voices (kick, snare, hihat) to two outputs, creating unpredictable and difficult-to-control patterns. This design solves that problem by:

1. **Embracing the 2-track constraint** as a creative strength
2. **Maximizing expressive control** with 32 distinct parameters
3. **Providing predictable, intuitive behavior** with clear track roles

## Documents

### Core Specification
- **[two-track-dual.md](./two-track-dual.md)** - Complete specification document
  - Design philosophy
  - Hardware control layout
  - Sequencer engine design
  - Output generation
  - Implementation notes

### Reference Materials
- **[control-map.md](./control-map.md)** - Quick reference for control layout
  - Visual control maps
  - Parameter interaction matrix
  - Performance workflow examples
  - Control priority guide

- **[comparison.md](./comparison.md)** - Comparison with current implementation
  - Feature comparison table
  - Use case examples
  - Migration considerations

### Implementation Guide
- **[implementation-roadmap.md](./implementation-roadmap.md)** - Step-by-step implementation plan
  - Phased implementation approach
  - Estimated effort
  - Key design decisions
  - Success criteria

## Key Features

### Control Architecture
- **16 controls per mode** (8 base + 8 shift)
- **32 total parameters** (16 per mode × 2 modes)
- **Shift button** (B7) accesses secondary controls
- **Mode switch** (B8) toggles Performance/Config modes

### Performance Mode Controls
**Base (Button Released)**:
- Track 1 Density
- Track 2 Density
- Track 1 Variation
- Track 2 Variation

**Shift (Button Held)**:
- Track 1 Velocity Range
- Track 2 Velocity Range
- Track 1 Timing Offset
- Track 2 Timing Offset

### Config Mode Controls
**Base (Button Released)**:
- Pattern Style
- Pattern Length
- Track Relationship
- Tempo

**Shift (Button Held)**:
- Swing Amount
- Accent Pattern
- Fill Probability
- Gate Length

### Core Innovations

1. **Algorithmic Pattern Generation**: Infinite pattern variations, smooth interpolation
2. **Track Relationship Control**: Complementary, Independent, or Contrasting modes
3. **Timing Offset**: Create polyrhythmic feels, micro-timing control
4. **Velocity Range**: Independent dynamic range control per track
5. **Swing & Accents**: Groove and emphasis controls

## Design Principles

1. **Embrace Constraints**: Work with 2-track limitation, not against it
2. **Maximize Expressiveness**: Deep control over every dimension
3. **Predictable Behavior**: Clear, intuitive track roles
4. **Performance-Optimized**: Quick access to core controls, deep access to expressive controls
5. **Musical Results**: Algorithmic patterns optimized for musical output

## Quick Start

1. **Read**: [two-track-dual.md](./two-track-dual.md) for complete specification
2. **Reference**: [control-map.md](./control-map.md) for control layout
3. **Compare**: [comparison.md](./comparison.md) to understand differences
4. **Implement**: [implementation-roadmap.md](./implementation-roadmap.md) for step-by-step guide

## Status

- ✅ Specification complete
- ✅ Control layout defined
- ✅ Implementation roadmap created
- ⏳ Implementation pending

## Questions & Feedback

This specification is designed to be:
- **Comprehensive**: All aspects covered
- **Practical**: Implementation-ready
- **Creative**: Maximizes expressive potential

For questions or feedback, refer to the main specification document or implementation roadmap.

