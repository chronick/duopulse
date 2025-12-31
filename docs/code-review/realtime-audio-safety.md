# Real-Time Audio Safety Checklist

**Critical**: Audio callbacks on STM32H7 run at high priority. Any blocking operation will cause audio glitches (clicks, pops, dropouts).

## 🔴 NEVER in Audio Callback

### Memory Allocation
- ❌ `new` / `delete`
- ❌ `malloc` / `free`
- ❌ `std::vector` growth (push_back, resize, reserve)
- ❌ `std::string` operations
- ❌ Any STL container that may allocate

### Blocking I/O
- ❌ Flash writes/erases (**10-100ms blocking!**)
- ❌ Serial logging (use ring buffer + deferred logging)
- ❌ File system operations
- ❌ USB communication

### System Calls
- ❌ `System::Delay()`
- ❌ Mutex locks (potential blocking)
- ❌ Semaphore waits
- ❌ Long-running calculations (>microseconds)

## ✅ Safe in Audio Callback

### Memory Access
- ✅ Stack variables (local primitives, small structs)
- ✅ Pre-allocated buffers (sized in Init())
- ✅ Fixed-size arrays
- ✅ Reading/writing class members

### Audio Processing
- ✅ DSP math (add, multiply, filters)
- ✅ DaisySP `Process()` calls (if Init()'d)
- ✅ Simple conditionals and loops
- ✅ Deterministic table lookups

### State Updates
- ✅ Setting flags (for main loop to read)
- ✅ Writing to ring buffers (if lock-free)
- ✅ Atomic operations (if needed)

## Time Budget

At 48kHz with block size 4:
- **Per-block budget**: 83 microseconds
- **Per-sample budget**: ~20 microseconds

At 48kHz with block size 32 (recommended):
- **Per-block budget**: 667 microseconds
- **Per-sample budget**: ~20 microseconds

**Rule of thumb**: If an operation takes >10µs, reconsider. If it takes >50µs, move to main loop.

## Common Violations in This Project

### Fixed: Flash Write in Audio Callback (Commit 1ccd75f)
```cpp
// ❌ BEFORE (caused glitches)
void AudioCallback(...) {
    if (shouldSave) {
        SaveConfigToFlash(config);  // 10-100ms blocking!
    }
}

// ✅ AFTER (deferred save pattern)
void AudioCallback(...) {
    if (shouldSave) {
        deferredSave.configToSave = config;
        deferredSave.pending = true;  // Flag for main loop
    }
}

// Main loop handles the actual write
while(1) {
    if (deferredSave.pending) {
        SaveConfigToFlash(deferredSave.configToSave);  // Safe here!
        deferredSave.pending = false;
    }
}
```

### Pattern: Deferred Logging
```cpp
// Audio callback: write to ring buffer
void AudioCallback(...) {
    if (eventOccurred) {
        logBuffer.Add(timestamp, eventData);  // Lock-free, fast
    }
}

// Main loop: flush ring buffer to serial
while(1) {
    logBuffer.FlushToSerial();
}
```

## Initialization Pattern

**All buffers MUST be pre-allocated in Init():**

```cpp
class MyProcessor {
private:
    float buffer_[MAX_SIZE];  // Fixed size, no allocation needed
    daisysp::Oscillator osc_;

public:
    void Init(float sampleRate) {
        // Initialize DSP modules ONCE
        osc_.Init(sampleRate);
        osc_.SetFreq(440.0f);

        // Pre-fill buffers if needed
        for (int i = 0; i < MAX_SIZE; i++) {
            buffer_[i] = 0.0f;
        }
    }

    float Process() {
        // Audio-rate processing - no allocation!
        return osc_.Process();
    }
};
```

## Checklist for Code Review

When reviewing audio callback code, verify:

- [ ] No `new`/`delete`/`malloc`/`free` anywhere in call tree
- [ ] All buffers pre-allocated in `Init()`
- [ ] All DaisySP objects `Init()`'d before use
- [ ] No flash writes (use deferred save pattern)
- [ ] No serial logging (use ring buffer)
- [ ] No system delays or sleeps
- [ ] No unbounded loops (all loops have fixed iteration count)
- [ ] No recursive calls (risk of stack overflow)
- [ ] Per-block processing time estimated <500µs

## References

- Flash write bug fix: `docs/chats/daisy-flash-writes.md`
- Logging strategy: `docs/chats/daisy-logging.md`
- Audio vs CV outputs: `docs/chats/daisy-audio-vs-cv.md`
