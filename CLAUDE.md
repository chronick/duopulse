# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Custom firmware for the **Electro-Smith Patch.Init** Eurorack module. A 2-voice algorithmic percussion sequencer built with DaisySP DSP library targeting techno/IDM genres. Uses C++14 on STM32H7 with real-time audio constraints.

## Build Commands

```bash
# Build firmware (requires ARM toolchain)
make

# Build with debug symbols
make DEBUG=1

# Run unit tests (host-side, requires Catch2)
make test

# Flash to hardware (requires DFU mode)
make program

# Clean build artifacts
make clean

# Build DaisySP/libDaisy libraries
make daisy-build
make libdaisy-build

# Update submodules to latest
make daisy-update
make libdaisy-update
```

## Architecture

### Spec-Driven Development

This project follows **Spec-Driven Development** (SDD). Key principles:

1. **Spec is source of truth**: `docs/specs/main.md` defines all behavior
2. **Never write non-trivial code without a task**: Tasks are defined in `docs/tasks/*.md`
3. **Small incremental changes**: Prefer 1-2 hour tasks
4. **Feature branches**: Use `feature/<slug>` naming

**Workflow**: Read `docs/SDD_WORKFLOW.md` for complete details.

**Key files**:
- `docs/specs/main.md` - Main specification
- `docs/tasks/index.md` - Global task tracker
- `docs/tasks/<slug>.md` - Per-feature task files

### SDD Agent System (v2)

This project uses a multi-agent architecture for SDD workflow:

| Agent | Purpose | Tools |
|-------|---------|-------|
| `sdd-manager` | Orchestrates all SDD work (planning, implementation, verification) | Read, Grep, Glob, Bash, Task |
| `code-writer` | Implements code changes with surgical precision | Read, Grep, Glob, Edit, Write |
| `validator` | Runs tests and builds, reports results verbatim | Read, Bash |
| `code-reviewer` | Reviews code for quality, safety, spec compliance | Read, Grep, Glob, Bash |

**Key design principle**: Only `sdd-manager` can delegate. Workers cannot claim success without verification.

#### Commands

| Command | Purpose |
|---------|---------|
| `/sdd-feature <desc>` | Plan a new feature, create task files |
| `/sdd-task <id or slug>` | Implement a task from start to finish |
| `/sdd-maintain` | Audit and fix project consistency |
| `/wrap-session` | Clean wrap-up with verification and commit |

#### Skills

Domain knowledge is provided via skills:
- `sdd-workflow` - SDD methodology reference
- `daisysp-review` - Real-time audio review criteria

#### Auto-Verification Hooks

The system uses hooks to automatically show verification after subagent runs:
- After any subagent completes: `git diff --stat` and `git status`
- On session end: current status and last commit

### Code Organization

```
src/
├── main.cpp              # Entry point, hardware init, audio callback
├── Engine/               # Core sequencer logic (~48 files)
│   ├── ArchetypeData.*   # Pattern archetypes
│   ├── PatternField.*    # 2D field system
│   ├── GenerationPipeline.* # Hit generation
│   ├── BrokenEffects.*   # Timing displacement
│   └── OutputManager.*   # Trigger/velocity outputs
└── System/               # Hardware abstraction

inc/
└── config.h              # Build-time flags, debug modes

tests/                    # Host-compiled Catch2 unit tests
docs/
├── specs/main.md         # Main specification
├── tasks/                # Task tracking
└── SDD_WORKFLOW.md       # Development workflow
```

## C++ Standards

### Naming Conventions
- **Classes**: `PascalCase` (e.g., `PatternField`)
- **Functions**: `camelCase` (e.g., `processAudio()`)
- **Variables**: `camelCase` (e.g., `sampleRate`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)
- **Private members**: `camelCase_` with trailing underscore

Full coding standards in `.cursor/rules/cpp.mdc`.

### Real-Time Audio Rules
1. **No heap allocation** in audio callback (`new`/`delete`/`malloc`)
2. **No blocking operations** (I/O, locks, syscalls)
3. **Pre-allocate all buffers** in `Init()` methods
4. **Initialize DSP objects** before use (call `Init()` on all DaisySP modules)
5. Audio callback must complete within ~20.8μs at 48kHz

### DaisySP Pattern
```cpp
// Always initialize before use
void Init(float sample_rate) {
    osc_.Init(sample_rate);
    osc_.SetFreq(440.0f);
}

// Process in blocks
for (size_t i = 0; i < size; i++) {
    out[0][i] = processor.Process(in[0][i]);
}
```

## Common Issues

### Audio Levels
- Eurorack CV: **±5V** (not ±10V)
- Normalized in code: **±1.0** maps to ±5V hardware
- Use macros: `CV_TO_NORMALIZED(cv)`, `NORMALIZED_TO_CV(norm)`

### Submodule Errors
If you see linker errors:
```bash
git submodule update --init --recursive
cd DaisySP && make && cd ..
cd libDaisy && make && cd ..
make clean && make
```

### DFU Mode (Flashing)
1. Power off module
2. Hold BOOT button
3. Power on while holding BOOT
4. Verify: `dfu-util --list`

### Unit Tests
- Tests are **host-compiled** (not ARM)
- Require Catch2 installed system-wide
- Run on development machine, not hardware

## Debug Flags

Hardware validation flags in `inc/config.h`:

- `DEBUG_BASELINE_MODE` - Forces known-good defaults
- `DEBUG_SIMPLE_TRIGGERS` - 4-on-floor test pattern
- `DEBUG_FEATURE_LEVEL` - Progressive feature enablement (0-5)

See `docs/misc/HARDWARE_VALIDATION_GUIDE.md` for testing procedure.

## Key Resources

- **Spec**: `docs/specs/main.md` (read this first!)
- **SDD Workflow**: `docs/SDD_WORKFLOW.md`
- **C++ Rules**: `.cursor/rules/cpp.mdc`
- **SDD Rules**: `.cursor/rules/sdd-global.mdc`
- [DaisySP Documentation](https://github.com/electro-smith/DaisySP)
- [libDaisy Documentation](https://github.com/electro-smith/libDaisy)
- [Patch.Init Info](https://www.electro-smith.com/daisy/patch)
