# CLAUDE.md

## Project Overview

**DuoPulse v5** - Custom firmware for the Electro-Smith Patch.Init Eurorack module. A 2-voice algorithmic percussion sequencer for techno/IDM. C++14 on STM32H7 with real-time audio constraints.

Key concepts:
- **ENERGY** controls pattern density (sparse → busy)
- **SHAPE** controls generation method (euclidean → weighted random)
- **AXIS X/Y** bias pattern placement (downbeats/offbeats, simple/complex)
- **Anchor/Shimmer** - two complementary rhythm voices

## Commands

```bash
make            # Build firmware
make test       # Run unit tests (362 tests)
make program    # Flash to hardware (DFU mode)
make help       # Show all targets
```

## File Organization

```
src/Engine/           # Core sequencer (~25 files)
  Sequencer.*         # Main sequencer orchestration
  GenerationPipeline.*# Hit generation (SHAPE algorithm)
  PatternField.*      # AXIS X/Y biasing
  VelocityCompute.*   # Accent/velocity calculation

docs/
  specs/main.md       # Spec (source of truth) - READ FIRST
  specs/*.md          # Spec sections (00-index.md has TOC)
  tasks/              # SDD task tracking
  design/             # Design iterations and history
  design/legacy/      # V3/V4 historical docs

scripts/
  pattern-viz/        # Pattern visualization tools
  manual_generator/   # PDF manual generation

tests/                # Catch2 unit tests (host-compiled)
inc/config.h          # Build flags and debug modes
```

## SDD Workflow

This project uses Spec-Driven Development. Key commands:
- `/sdd-feature` - Plan a new feature
- `/sdd-task` - Implement a task
- `/sdd-maintain` - Audit project consistency

See `docs/SDD_WORKFLOW.md` for details.

## Real-Time Audio Rules

1. **No heap allocation** in audio callback
2. **No blocking operations** (I/O, locks, syscalls)
3. Audio callback must complete within ~20.8μs at 48kHz

## Quick Reference

- Eurorack CV: ±5V (normalized as ±1.0 in code)
- Submodule issues: `git submodule update --init --recursive`
- DFU mode: Hold BOOT while powering on
