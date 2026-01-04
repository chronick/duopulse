---
id: 18
slug: level0-debug-fixes
title: "Level 0 Debug Fixes"
status: completed
created_date: 2025-12-26
updated_date: 2025-12-27
completed_date: 2025-12-27
branch: testing/hardware-simplifications
parent_task: 16
spec_refs:
  - "Test 1: Basic Clock & Triggers"
---

# Task 18: Level 0 Debug Fixes

---

## Problem Statement

Hardware testing of DEBUG_FEATURE_LEVEL 0 revealed several issues:

1. **LED not blinking at regular 120 BPM** - Brightness fluctuates without correlation to hit timing
2. **Drum pattern erratic** - Shimmer fires when anchor doesn't; not detecting regular 4-on-floor
3. **Gates fire but not at correct timing** - Pattern doesn't match expected 4-on-floor
4. **Aux output affected by Config + K3** - Should ignore config at Level 0

---

## CRITICAL BUG: Sample Rate is Zero (2025-12-26)

### Symptom
After flashing, the sequencer never advances. Pressing reset triggers gates once, then nothing.

### Evidence from Logs
```
[7563] [INFO] main.cpp:657 Sample rate:  Hz, Block size: 32
```
The sample rate prints as **blank/zero**!

### Root Cause
In `main.cpp`, `patch.AudioSampleRate()` is called BEFORE `patch.StartAudio()`:

```cpp
patch.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_32KHZ);
float sampleRate = patch.AudioSampleRate();  // Returns 0 before StartAudio!
// ... later ...
sequencer.Init(sampleRate);  // Sequencer initialized with sampleRate = 0
// ... later ...
patch.StartAudio(AudioCallback);
```

The Daisy SDK's `AudioSampleRate()` only returns a valid value AFTER `StartAudio()` configures the SAI peripheral.

### Impact
- `sequencer.Init(0)` initializes metro with frequency 8.0f and sample rate 0.0f
- Metro never generates clock ticks (division by zero in period calculation)
- `ProcessAudio()` never advances steps
- No pattern playback, no gate outputs

### Fix
Use hardcoded sample rate constant instead of querying before audio starts:

```cpp
// BEFORE (broken):
float sampleRate = patch.AudioSampleRate();

// AFTER (fixed):
constexpr float sampleRate = 32000.0f;  // SAI_32KHZ = 32000 Hz
```

---

## Root Cause Analysis

### Issue 1: Wrong Pattern Masks

The Level 0 pattern masks in `Sequencer.cpp:196-200` are incorrect:

```cpp
// CURRENT (WRONG):
state_.sequencer.anchorMask = 0x01010101;    // Steps 0, 8, 16, 24 = every 8 steps
state_.sequencer.shimmerMask = 0x01000100;   // Steps 8, 24 = beats 3, 7
```

**Problem**: At 16th-note resolution (4 steps per beat), the pattern length is 32 steps = 8 beats.

- `0x01010101` = bits 0, 8, 16, 24 = steps 0, 8, 16, 24 = every 8 steps = **beats 1, 3, 5, 7**
- This is NOT four-on-floor (which should be every beat)

**Correct masks**:
```cpp
// CORRECT:
state_.sequencer.anchorMask = 0x11111111;    // Steps 0, 4, 8, 12, 16, 20, 24, 28 = every beat
state_.sequencer.shimmerMask = 0x10101010;   // Steps 4, 12, 20, 28 = beats 2, 4, 6, 8 (backbeat)
```

**Bit math verification**:
- `0x11111111` = 0b00010001000100010001000100010001 = bits 0, 4, 8, 12, 16, 20, 24, 28 ✓
- `0x10101010` = 0b00010000000100000001000000010000 = bits 4, 12, 20, 28 ✓

### Issue 2: LED Not Correlating with Hits

The LED logic in `main.cpp:483-515` reads gate state from `sequencer.IsGateHigh()`, which only returns true during the 10ms trigger duration. The `ProcessControls()` runs in the main loop with `System::Delay(1)`.

**Potential issues**:
- No logging to verify when gates actually fire
- LED update frequency may miss short gates
- Need timestamp logging to correlate

### Issue 3: Aux Output Affected by Config

At Level 0, the aux output uses `controlState.auxMode` which is affected by K3 in config mode:

```cpp
AuxMode currentAuxMode = GetAuxModeFromValue(controlState.auxMode);
switch(currentAuxMode) { ... }
```

**Fix**: At Level 0, aux should use fixed clock output, ignoring config settings.

---

## Proposed Fixes

### Fix 1: Correct Pattern Masks (Sequencer.cpp)

```cpp
#if DEBUG_FEATURE_LEVEL < 1
    // Level 0: Simple 4-on-floor pattern, bypass entire generation pipeline
    // 32 steps = 8 beats at 4 steps per beat
    // Anchor: every beat (steps 0, 4, 8, 12, 16, 20, 24, 28)
    // Shimmer: backbeat (steps 4, 12, 20, 28 = beats 2, 4, 6, 8)
    // Aux: 8th notes (every 2 steps)
    state_.sequencer.anchorMask = 0x11111111;     // Every beat
    state_.sequencer.shimmerMask = 0x10101010;    // Backbeat
    state_.sequencer.auxMask = 0x55555555;        // 8th notes (unchanged)
    state_.sequencer.anchorAccentMask = 0x01010101;  // Accent on 1, 3, 5, 7
    state_.sequencer.shimmerAccentMask = 0x10101010; // Accent all backbeats
    return;
#endif
```

### Fix 2: Add Gate Event Logging (main.cpp)

Add DEBUG-level logging when gates fire:

```cpp
// In ProcessControls(), after reading gate states:
static bool lastAnchorGate = false;
static bool lastShimmerGate = false;

bool anchorGateHigh = sequencer.IsGateHigh(0);
bool shimmerGateHigh = sequencer.IsGateHigh(1);

// Log gate transitions
if (anchorGateHigh && !lastAnchorGate) {
    LOGD("GATE1 (Anchor) ON @ step %d", sequencer.GetCurrentStep());
}
if (shimmerGateHigh && !lastShimmerGate) {
    LOGD("GATE2 (Shimmer) ON @ step %d", sequencer.GetCurrentStep());
}
lastAnchorGate = anchorGateHigh;
lastShimmerGate = shimmerGateHigh;
```

### Fix 3: Simplify LED to Hit-Only (main.cpp)

Remove clock-based LED blinking, only blink on actual gate events:

```cpp
// === LED Feedback System (Level 0 Simplified) ===
// Only blink when gates fire - no clock-based blinking
// Config mode: solid on
// Gate 1 (Anchor): 50% brightness
// Gate 2 (Shimmer): 30% brightness

float ledBrightness = 0.0f;

if (controlState.configMode) {
    ledBrightness = 1.0f;
} else if (anchorGateHigh) {
    ledBrightness = 0.5f;
} else if (shimmerGateHigh) {
    ledBrightness = 0.3f;
}
// Note: LED is OFF when no gates are active (not clock-following)
```

### Fix 4: Force Fixed Aux at Level 0 (main.cpp)

```cpp
// === AUX Output (CV_OUT_1) ===
#if DEBUG_FEATURE_LEVEL < 1
    // Level 0: Fixed clock output, ignore config mode
    auxVoltage = sequencer.IsClockHigh() ? 5.0f : 0.0f;
#else
    // Level 1+: Use configured aux mode
    // ... existing switch statement ...
#endif
```

### Fix 5: Add Config Mode Change Logging

```cpp
// In ProcessControls(), when mode changes:
if (currentMode != previousMode) {
    LOGD("Mode: %s", modeName);

    // Also log current config values for debugging
    if (currentMode == ControlMode::ConfigPrimary || currentMode == ControlMode::ConfigShift) {
        LOGD("Config: AuxMode=%.2f, ResetMode=%.2f",
             controlState.auxMode, controlState.resetMode);
    }
}
```

### Fix 6 (Optional): Add Tempo Verification Logging

To confirm the clock is running at the correct speed, add one-time logging at startup:

```cpp
// In main(), after sequencer.Init():
LOGI("Clock: 120 BPM, 8 Hz (16th notes), Pattern: 32 steps = 8 beats = 4 sec/loop");
LOGI("Sample rate: %.0f Hz, Block size: %d", sampleRate, 32);
```

**Expected behavior:**
- Kick drum should fire every **0.5 seconds** (2 Hz = 120 BPM)
- Full 32-step pattern loops every **4 seconds**
- With corrected masks, you should hear regular 4-on-floor at 120 BPM

---

## Git History Investigation (2025-12-26)

Issues were reported after implementing "make listen" behavior. Relevant commits:

| Commit | Description | Potential Impact |
|--------|-------------|------------------|
| `a56031a` | Block size 4→32, Sample rate 48kHz→32kHz | **HIGH** - Changed audio timing |
| `303eb30` | Add debug build make commands | Low - Build system only |
| `1ccd75f` | Defer flash writes to main loop | Medium - Timing changes |

### Key Finding: Block Size Change

The commit `a56031a` changed:
- Block size: 4 → 32 samples
- Sample rate: 48kHz → 32kHz

This shouldn't break the Metro (which processes sample-by-sample), but combined with
the sample rate bug (AudioSampleRate() returning 0 before StartAudio), it may have
exposed a latent initialization order issue

---

## Race Condition Fixes (2025-12-27)

### Issue: Trigger Events Missed by Main Loop

**Symptom**: Gate events appeared in logs inconsistently, with ~4 second gaps and missing triggers. Drum modules showed non-deterministic behavior - sometimes alternating, sometimes simultaneous.

**Root Cause 1: Trigger Pulse Fits in One Audio Block**
- Audio block size: 32 samples
- Trigger duration: 32 samples (1ms at 32kHz)
- Trigger pulse completes **entirely within one audio block**
- Main loop runs after audio callback → gate already LOW → edge missed

**Fix 1: Event Latch System** (`src/Engine/OutputState.h:28-85`)
- Added `eventPending` flag to TriggerState
- Set on `Fire()`, cleared by `AcknowledgeEvent()`
- Main loop uses `HasPendingTrigger()` instead of `IsGateHigh()`
- Latch persists even after pulse ends → reliable edge detection

**Root Cause 2: Trigger Pulse Too Short for Drum Modules**
- V4 used 1ms pulses: `sampleRate / 1000.0f`
- Main branch used 10ms pulses: `sampleRate * 0.01f`
- Many Eurorack drum modules need 2-5ms minimum to trigger reliably
- 1ms pulses caused intermittent detection

**Fix 2: Increase Pulse Duration** (`src/Engine/OutputState.h:443`)
- Changed from `sampleRate / 1000.0f` (1ms) to `sampleRate * 0.01f` (10ms)
- Matches main branch behavior
- Ensures reliable triggering on all Eurorack modules

**Root Cause 3: UART Logging Blocking Main Loop**
- Logger switches to blocking mode after a few packets (logger.h:111-113)
- `TransmitSync()` spins until USB buffer clears
- High-frequency gate logging (16 events/bar) saturates USB buffer
- Main loop blocked → can't detect new trigger events

**Fix 3: Non-Blocking Gate Event Logger** (`src/main.cpp:78-120`)
- Added 32-event ring buffer
- Gate events captured immediately with true timestamp
- Main loop flushes 1 event per iteration (rate-limited)
- Prevents USB blocking from affecting event capture

**Files Modified**:
- `src/Engine/OutputState.h` - Event latch, 10ms trigger duration
- `src/Engine/Sequencer.h` - HasPendingTrigger(), AcknowledgeTrigger() declarations
- `src/Engine/Sequencer.cpp` - Latch API implementation
- `src/main.cpp` - Ring buffer logger, latch-based detection

**Verification**:
- Serial logs show consistent 500ms spacing (correct 120 BPM)
- No more ~4 second gaps
- Drum modules reliably trigger on 4-on-floor pattern
- Both anchor and shimmer fire on same step when expected

---

## Implementation Checklist

- [x] **Fix 1**: Update pattern masks in `src/Engine/Sequencer.cpp:196-203`
- [x] **Fix 2**: Add gate event logging in `src/main.cpp` (ProcessControls)
- [x] **Fix 3**: Verify LED logic is hit-only (no clock-based blink)
- [x] **Fix 4**: Add Level 0 aux output bypass in `src/main.cpp`
- [x] **Fix 5**: Add config mode value logging
- [x] **Build**: Firmware compiles successfully (121KB/128KB flash, 92.7%)
- [x] **Fix 7**: Sample rate hardcoded to 32000.0f (was returning 0 from AudioSampleRate())
- [x] **Fix 8**: Add audio callback heartbeat logging (every 2 sec)
- [x] **Fix 9**: Add Metro tick diagnostics (first 10 ticks logged)
- [x] **Fix 10**: Add Sequencer::Init sample rate verification log
- [x] **Build**: Firmware recompiled with diagnostics (122KB/128KB flash, 93.2%)
- [x] **Fix 11**: Add event latch system to TriggerState (prevents race condition)
- [x] **Fix 12**: Increase trigger pulse duration from 1ms to 10ms (matches main branch)
- [x] **Fix 13**: Implement non-blocking gate event logger with ring buffer
- [x] **Fix 14**: Fix logging system blocking issues (non-blocking capture, rate-limited flush)
- [x] **Build**: Final firmware compiled (119KB/128KB flash, 93.0%)
- [x] **Flash**: Upload to hardware with `make program`
- [x] **Verify**: Correct 4-on-floor pattern at 120 BPM confirmed via drum modules
- [x] **Update**: Mark Test 1 Level 0 checkboxes in task 16

---

## Diagnostic Log Messages Added

The following diagnostic messages are now logged to help debug timing issues:

1. **Sequencer::Init** (`INFO` level):
   ```
   Sequencer::Init called with sampleRate=32000
   Metro initialized: freq=8.0 Hz, period=4000 samples
   ```

2. **AudioCallback first call** (`DEBUG` level):
   ```
   AudioCallback first call, size=32
   ```

3. **Audio heartbeat** (every 2 seconds, `DEBUG` level):
   ```
   Audio heartbeat: 64000 samples
   ```

4. **Metro tick diagnostics** (first 10 ticks, `DEBUG` level):
   ```
   Metro tick #1 at sample 4000 (step 0)
   Metro tick #2 at sample 8000 (step 1)
   ...
   ```

### Expected Log Output (Healthy System)

With sample rate = 32kHz and 8 Hz metro:
- Metro should tick every 4000 samples (32000 / 8 = 4000)
- First 10 ticks should appear within 1.25 seconds
- Audio heartbeat every 2 seconds confirms callback is running

### Failure Modes to Diagnose

| Log Pattern | Probable Cause |
|-------------|----------------|
| No "AudioCallback first call" | StartAudio() failed or callback not registered |
| "sampleRate=0" in Sequencer::Init | Sample rate fix not applied (rebuild required) |
| No Metro ticks logged | Metro not ticking (division by zero or inf period) |
| Audio heartbeat but no Metro ticks | Metro phase increment issue |
| Metro ticks but no gates | Gate output logic issue |

---

## Expected Behavior After Fixes

At DEBUG_FEATURE_LEVEL 0:

| Output | Expected Behavior |
|--------|-------------------|
| Gate Out 1 (Anchor) | Fires every beat (8 times per 32-step bar) |
| Gate Out 2 (Shimmer) | Fires on backbeats 2, 4, 6, 8 (4 times per bar) |
| LED (CV_OUT_2) | Blinks on each gate event, 50% for anchor, 30% for shimmer |
| AUX (CV_OUT_1) | Fixed clock output (ignores config K3) |
| Logs | Gate events with step numbers for verification |

---

## Test Procedure

### Step 1: Build and Flash
```bash
make clean && make build-debug   # Build with DEBUG logging
make program-dfu                 # Flash (device in DFU mode)
```

### Step 2: Monitor Serial Output
```bash
make listen                      # Opens screen session with logging
```

### Step 3: Verify Initialization Logs

After boot, you should see these messages **in order**:
```
[timestamp] [INFO] main.cpp:XXX DuoPulse v4 boot
[timestamp] [INFO] main.cpp:XXX Build: ...
[timestamp] [INFO] Sequencer.cpp:29 Sequencer::Init called with sampleRate=32000
[timestamp] [DEBUG] Sequencer.cpp:33 Metro initialized: freq=8.0 Hz, period=4000 samples
[timestamp] [INFO] main.cpp:XXX Clock: 120 BPM, 8 Hz (16th notes)...
[timestamp] [INFO] main.cpp:XXX Sample rate: 32000 Hz, Block size: 32
[timestamp] [INFO] main.cpp:XXX Initialization complete, starting audio
[timestamp] [DEBUG] main.cpp:210 AudioCallback first call, size=32
```

**If sampleRate shows as 0 or blank, the fix wasn't applied - rebuild required!**

### Step 4: Verify Metro Ticks

Within the first 2 seconds, you should see Metro tick logs:
```
[timestamp] [DEBUG] Sequencer.cpp:XXX Metro tick #1 at sample 4000 (step 0)
[timestamp] [DEBUG] Sequencer.cpp:XXX Metro tick #2 at sample 8000 (step 1)
...
[timestamp] [DEBUG] Sequencer.cpp:XXX Metro tick #10 at sample 40000 (step 9)
```

**If no Metro ticks appear, the clock is not running!**

### Step 5: Verify Audio Heartbeat

Every 2 seconds, you should see:
```
[timestamp] [DEBUG] main.cpp:XXX Audio heartbeat: 64000 samples
```

**If no heartbeat, the audio callback is not running!**

### Step 6: Verify Gate Outputs (Physical)

With B8 switch DOWN (performance mode):
- LED blinks 8 times per bar (every beat)
- Gate 1 fires 8 times per bar
- Gate 2 fires 4 times per bar (backbeats)

### Step 7: Config Mode Test

1. Switch B8 UP (config mode) - LED should be solid
2. Turn K3 - aux output should NOT change (Level 0 bypasses config)

---

## Files to Modify

| File | Changes |
|------|---------|
| `src/Engine/Sequencer.cpp` | Fix pattern masks (lines 196-203) |
| `src/main.cpp` | Add gate logging, fix aux Level 0 bypass |

---

## References

- Task 16: DuoPulse v4 Hardware Validation (parent task)
- Task 17: Runtime Logging System (logging infrastructure)
- `docs/misc/HARDWARE_VALIDATION_GUIDE.md` - Testing procedures
