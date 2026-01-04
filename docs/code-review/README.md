# Code Review Guidelines

This directory contains code review standards and common pitfall documentation for the DuoPulse v4 firmware project.

## Quick Reference

| Document | Purpose | When to Use |
|----------|---------|-------------|
| [realtime-audio-safety.md](realtime-audio-safety.md) | Audio callback safety rules | Reviewing any audio-path code |
| [common-pitfalls.md](common-pitfalls.md) | Historical bugs and fixes | Learning from past mistakes |
| [dsp-gotchas.md](dsp-gotchas.md) | Hardware-specific quirks | Working with Patch.Init/DaisySP |
| [testing-standards.md](testing-standards.md) | Test writing guidelines | Writing or reviewing tests |
| [cpp-standards.md](cpp-standards.md) | C++ coding style | General code review |

## For Reviewers

When reviewing code, check these documents in order:

1. **Start here**: [realtime-audio-safety.md](realtime-audio-safety.md)
   - Critical for audio callback changes
   - Prevents glitches and crashes

2. **Check against**: [common-pitfalls.md](common-pitfalls.md)
   - See if similar bugs happened before
   - Verify invariants aren't violated

3. **Hardware concerns**: [dsp-gotchas.md](dsp-gotchas.md)
   - CV/audio voltage ranges
   - DaisySP initialization patterns
   - Unpatched input assumptions

4. **Test coverage**: [testing-standards.md](testing-standards.md)
   - Verify tests are added for new features
   - Check for regression tests on bug fixes

5. **Style check**: [cpp-standards.md](cpp-standards.md)
   - Naming conventions
   - Documentation quality

## For Implementers

Before starting a task:

1. Read the relevant section of `docs/specs/main.md`
2. Check [common-pitfalls.md](common-pitfalls.md) for similar past issues
3. Review [dsp-gotchas.md](dsp-gotchas.md) if touching hardware interface
4. Follow [realtime-audio-safety.md](realtime-audio-safety.md) for audio-path code
5. Write tests per [testing-standards.md](testing-standards.md)
6. Format code per [cpp-standards.md](cpp-standards.md)

## Critical Invariants

From [common-pitfalls.md](common-pitfalls.md):

- **DENSITY=0** → Zero hits (hard stop)
- **DRIFT=0** → No pattern evolution
- **No heap allocation** in audio callback
- **No flash writes** in audio callback (use deferred save)
- **CV inputs**: Unpatched = 0V, not 0.5V
- **All DSP modules**: Must call `Init()` before `Process()`

## Bug Prevention Checklist

Use this checklist for every code review:

### Real-Time Safety
- [ ] No `new`/`delete`/`malloc`/`free` in audio path
- [ ] No flash writes in audio callback
- [ ] All buffers pre-allocated in `Init()`
- [ ] All DaisySP objects `Init()`'d before use

### Correctness
- [ ] Config parameters actually used (not ignored)
- [ ] Correct signal/getter used for each output mode
- [ ] Invariants preserved (DENSITY=0, DRIFT=0)
- [ ] Loop bounds use variables, not magic numbers

### Hardware Interface
- [ ] CV modulation is additive (0V = no effect)
- [ ] CV outputs are 0-5V, not 0-10V
- [ ] Audio outputs handle ±9V range (inverted!)
- [ ] No assumptions about unpatched CV voltage

### Testing
- [ ] Tests added for new features
- [ ] Regression test added for bug fixes
- [ ] Invariants tested exhaustively
- [ ] Float comparisons use `Approx()`

### Style
- [ ] Naming conventions followed
- [ ] Functions documented with spec references
- [ ] Comments explain WHY, not WHAT

## Recent Bug Fixes

See [common-pitfalls.md](common-pitfalls.md) for details:

| Bug | Commit | Category |
|-----|--------|----------|
| Flash write glitches | 1ccd75f | Real-time safety |
| CV double-application | 969aa30 | CV modulation |
| Unpatched CV baseline | 96e206c | CV modulation |
| Swing config ignored | be7b58f | Config parameters |
| Clock mapping off | be7b58f | Hardware mapping |
| DENSITY=0 bypass | 3ddcfc6 | Invariants |
| Pattern length hardcoded | 1288d21 | Array bounds |

## Resources

- Main specification: `docs/specs/main.md`
- SDD workflow: `docs/SDD_WORKFLOW.md`
- Hardware chat docs: `docs/chats/` (flash writes, logging, audio vs CV)
- Example tests: `tests/test_timing.cpp`, `tests/test_generation.cpp`
- DaisySP docs: https://github.com/electro-smith/DaisySP
- Patch.Init info: https://www.electro-smith.com/daisy/patch

## Contributing to These Docs

Found a new pattern or pitfall? Add it:

- **Bug fix**: Document in `common-pitfalls.md` with commit hash
- **Hardware quirk**: Add to `dsp-gotchas.md`
- **Test pattern**: Document in `testing-standards.md`
- **Style rule**: Clarify in `cpp-standards.md`

Keep examples concrete and reference actual commits where possible.
