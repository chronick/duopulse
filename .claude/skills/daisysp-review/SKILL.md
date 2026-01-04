# DaisySP/STM32 Code Review Skill

## Purpose

This skill provides project-specific code review criteria for DaisySP firmware projects targeting STM32H7 microcontrollers. It should be activated when reviewing C++ code that runs in real-time audio contexts.

## When to Activate

Activate this skill when:
- Reviewing C++ code in `src/Engine/` or similar audio paths
- Code touches audio callback, DSP processing, or hardware interfaces
- Changes involve timing-critical operations
- Reviewing pattern generation, voice/trigger logic, or CV handling

## Critical Review Criteria

### 1. Real-Time Audio Safety (HIGHEST PRIORITY)

**FORBIDDEN in audio callback path:**
- `new`, `delete`, `malloc`, `free` - heap allocation
- `std::vector::push_back`, `std::string` concatenation - hidden allocations
- File I/O, network, any syscalls
- `mutex`, `lock_guard`, semaphores - blocking
- `printf`, `cout`, logging functions
- Unbounded loops without iteration limits

**REQUIRED:**
- All buffers pre-allocated in `Init()` methods
- Fixed-size arrays, not dynamic containers
- Audio callback must complete in <20.8μs at 48kHz (block size dependent)
- All DaisySP modules `Init()`'d before first `Process()` call

**Check pattern:**
```cpp
// BAD - allocation in process
void Process() {
    std::vector<float> temp;  // HEAP ALLOCATION!
    temp.push_back(value);
}

// GOOD - pre-allocated
void Init() {
    buffer_.fill(0.0f);  // Fixed array
}
void Process() {
    buffer_[idx_] = value;  // No allocation
}
```

### 2. Historical Bug Patterns

These bugs have occurred in this codebase before. Check explicitly:

**CV Modulation:**
- CV must be ADDITIVE (0V = no effect, not 0.5V baseline)
- Watch for double-application of CV (parameter + direct)
- Verify correct getter for each output mode

**Config Parameters:**
- Parameters must be USED, not just defined
- Check that getter returns the right signal for mode
- Verify detent positions match spec (e.g., ×1 at 50%)

**Invariants:**
- DENSITY=0 MUST produce zero hits (no exceptions)
- DRIFT=0 MUST prevent evolution (no exceptions)
- These are HARD requirements, test explicitly

**Array Bounds:**
- Loops must use variables, not magic numbers
- Check off-by-one at boundaries
- Verify index calculations

### 3. Hardware Specifics (Patch.Init)

**CV Outputs:** 0-5V unipolar (NOT 0-10V)
**Audio Outputs:** ±9V bipolar, DC-coupled, INVERTED polarity
**CV Inputs:** Unpatched reads as 0V (NOT 0.5V or 2.5V)

**Common mistakes:**
```cpp
// BAD - assumes 0.5V baseline
float cvMod = hw.GetCV(0) - 0.5f;

// GOOD - 0V baseline
float cvMod = hw.GetCV(0);  // 0V = no modulation
```

### 4. Naming Conventions

- Classes: `PascalCase` (e.g., `PatternField`)
- Functions: `camelCase` (e.g., `processAudio()`)
- Variables: `camelCase` (e.g., `sampleRate`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)
- Private members: `camelCase_` with trailing underscore

### 5. Testing Requirements

New code should have tests that verify:
- Parameters are actually USED (not just stored)
- Invariants hold (DENSITY=0 → 0 hits in 100 trials)
- Boundary conditions (0%, 25%, 50%, 75%, 100%)
- Floats use `Approx()` with appropriate margin
- Determinism with fixed RNG seeds

## Review Output Format

When using this skill, structure findings as:

```
### Real-Time Safety
- [CRITICAL/OK] <finding>

### Historical Patterns
- [WARNING/OK] <finding>

### Hardware Interface
- [WARNING/OK] <finding>

### Code Quality
- [SUGGESTION/OK] <finding>
```

## References

For full details, see:
- `docs/code-review/realtime-audio-safety.md`
- `docs/code-review/common-pitfalls.md`
- `docs/code-review/dsp-gotchas.md`
- `docs/code-review/testing-standards.md`
- `docs/code-review/cpp-standards.md`
