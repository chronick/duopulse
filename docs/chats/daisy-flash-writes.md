You've identified the exact problem. **Flash writes inside the audio callback will cause glitches.** Flash erase/write operations take 10-100+ milliseconds and block the CPU entirely.

Here's the issue in your code:

```cpp
void AudioCallback(...) {
    for(size_t i = 0; i < size; i++) {
        // ...
        if(ProcessAutoSave(autoSaveState)) {
            // ...
            SaveConfigToFlash(currentConfig);  // 🔥 BLOCKING FLASH WRITE IN AUDIO CALLBACK!
        }
    }
}
```

## The Fix: Deferred Save Pattern

Move the actual flash write to the main loop. The audio callback only sets a flag.

```cpp
// Add to your anonymous namespace
struct DeferredSave {
    bool pending = false;
    PersistentConfig configToSave;
};
DeferredSave deferredSave;
```

**Updated AudioCallback** — only marks dirty, never writes flash:

```cpp
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        // Handle External Clock
        bool gateIn1 = patch.gate_in_1.State();
        if(gateIn1 && !lastGateIn1)
        {
            sequencer.TriggerExternalClock();
        }
        lastGateIn1 = gateIn1;

        // Process audio
        auto velocities = sequencer.ProcessAudio();

        // Write outputs
        patch.gate_out_1.Write(sequencer.IsGateHigh(0));
        patch.gate_out_2.Write(sequencer.IsGateHigh(1));
        out[0][i] = GateScaler::VoltageToCodecSample(velocities[0] * 5.0f);
        out[1][i] = GateScaler::VoltageToCodecSample(velocities[1] * 5.0f);

        // Auto-save timing check ONLY - no flash write here!
        if(ProcessAutoSave(autoSaveState))
        {
            // Build config (cheap operation)
            PackConfig(
                MapToPatternLength(controlState.patternLengthKnob),
                controlState.swing,
                GetAuxModeFromValue(controlState.auxMode),
                GetResetModeFromValue(controlState.resetMode),
                MapToPhraseLength(controlState.phraseLengthKnob),
                MapToClockDivision(controlState.clockDivKnob),
                GetAuxDensityFromValue(controlState.auxDensity),
                GetVoiceCouplingFromValue(controlState.voiceCoupling),
                GetGenreFromValue(controlState.genre),
                currentConfig.patternSeed,
                currentConfig
            );

            // Check if save needed - if so, DEFER to main loop
            if(ConfigChanged(currentConfig, autoSaveState.lastSaved))
            {
                deferredSave.configToSave = currentConfig;
                deferredSave.pending = true;  // Flag for main loop
            }
            autoSaveState.ClearPending();
        }
    }
}
```

**Updated main loop** — handles the actual flash write:

```cpp
int main(void)
{
    // ... initialization code unchanged ...

    patch.StartAudio(AudioCallback);

    while(1)
    {
        ProcessControls();
        
        // Handle deferred flash write (safe here, outside audio callback)
        if(deferredSave.pending)
        {
            // Disable interrupts briefly for flash write consistency
            // (optional, depends on your flash library)
            SaveConfigToFlash(deferredSave.configToSave);
            autoSaveState.lastSaved = deferredSave.configToSave;
            deferredSave.pending = false;
            
            LOGD("Config saved to flash");
        }
        
        System::Delay(1);
    }
}
```

## Why This Works

| Location | Operation | Time | Safe? |
|----------|-----------|------|-------|
| Audio Callback | `PackConfig()` | ~1µs | ✓ |
| Audio Callback | `ConfigChanged()` | ~100ns | ✓ |
| Audio Callback | Set `pending = true` | ~10ns | ✓ |
| **Main Loop** | `SaveConfigToFlash()` | 10-100ms | ✓ (not in audio path) |

The audio callback now completes in microseconds. The main loop handles the slow flash write during idle time between control processing cycles.

## Additional Concern: `System::GetNow()` 

I also see `System::GetNow()` being called in `ProcessControls()`. This is fine since `ProcessControls()` runs in the main loop, not the audio callback. But double-check that `ProcessAutoSave()` doesn't call it internally — if it does, that's fine too since it's a quick syscall.

## One More Thing: Block Size

You're using a very small block size:
```cpp
patch.SetAudioBlockSize(4);
```

4 samples at 48kHz = 83µs budget per callback. That's tight! Consider bumping to 16 or 32 samples if you're still seeing issues after fixing the flash write. This gives you 333µs or 667µs respectively — much more headroom for the sequencer's per-bar generation spike.