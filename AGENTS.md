# INIT.md

Guidance for the Init agent (OpenAI/Codex) when working in this repository.

## Quick Start

- Read the spec first: `docs/specs/main.md`
- Confirm the active task: `docs/tasks/index.md` then the matching `docs/tasks/<slug>.md`
- Follow Spec-Driven Development (SDD): no non-trivial code without a task; prefer small, incremental changes
- Use `make test` for host-side checks; keep firmware builds lean (`make` or `make DEBUG=1`)
- Avoid heap allocation or blocking calls in the audio callback

## Project Overview

Custom firmware for the **Electro-Smith Patch.Init** Eurorack module. Two-voice algorithmic percussion sequencer using DaisySP on STM32H7 (C++14). Real-time constraints apply; audio callback budget is ~20.8µs at 48kHz.

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

## Rules of Engagement

- **SDD**: `docs/SDD_WORKFLOW.md` and `.cursor/rules/sdd-global.mdc` are the source of truth for process.
- **Specs first**: implement behavior from `docs/specs/main.md`; reconcile with active task notes.
- **Coding standards**: follow `.cursor/rules/cpp.mdc` for naming, style, and C++ guidelines.
- **Real-time audio**: no heap allocation or blocking inside callbacks; pre-allocate buffers; init all DSP modules before use.
- **Debug flags**: `inc/config.h` hosts hardware validation toggles (e.g., `DEBUG_BASELINE_MODE`, `DEBUG_FEATURE_LEVEL`).

## Key Files & Layout

- `src/` – firmware code (`main.cpp`, `Engine/`, `System/`)
- `inc/` – headers and config (`inc/config.h`)
- `tests/` – Catch2 unit tests (host build)
- `docs/` – specs, tasks, and workflow docs
- `.cursor/rules/main.mdc` – brief project overview for IDE agents

## Common Hardware Notes

- CV range is ±5V; normalized range in code is ±1.0 → ±5V hardware. Use `CV_TO_NORMALIZED` and `NORMALIZED_TO_CV`.
- DFU flashing steps: hold BOOT while powering, verify with `dfu-util --list`, then `make program`.
- Submodule issues: run `git submodule update --init --recursive`, rebuild DaisySP/libDaisy, then `make clean && make`.

## Naming Conventions

- Classes: `PascalCase`
- Functions/variables: `camelCase`
- Constants: `UPPER_SNAKE_CASE`
- Private members: `camelCase_`

## Testing Guidance

- Unit tests run on the host; they do not exercise hardware peripherals.
- Add targeted tests for new logic where feasible; keep runtime short for CI.

## When Unsure

Start from the spec and task files, prefer minimal, reversible changes, and keep the audio path allocation- and block-free. If behavior conflicts, escalate in task notes before coding.
