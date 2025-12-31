# DSP Gotchas: Patch.Init / DaisySP

Hardware-specific quirks and DSP patterns for Electro-Smith Patch.Init.

## Hardware Voltage Ranges

### CV Outputs: 0-5V Unipolar
- **Spec**: 12-bit DAC, 0-5V unipolar
- **Access**: `patch.WriteCvOut(channel, voltage)` or similar API
- **Normalization**: `0.0f` → 0V, `1.0f` → 5V
- **Use cases**: 1V/oct pitch CV, gates, envelopes, "clean" modulation

```cpp
// ✅ Correct CV output usage
float normalizedValue = 0.8f;  // 0.0-1.0 range
float voltage = normalizedValue * 5.0f;  // 0-5V
patch.WriteCvOut(0, voltage);
```

### Audio Outputs: ±9V Bipolar (INVERTED!)
- **Spec**: 24-bit codec, ~±9V (18Vpp), DC-coupled
- **Polarity**: **INVERTED** - positive float → negative voltage!
- **Normalization**: `-1.0f` → +9V, `+1.0f` → -9V
- **Use cases**: Audio, bipolar CV/LFO, "hot" modulation

```cpp
// ⚠️  INVERTED POLARITY!
out[0][i] = 0.5f;   // Outputs -4.5V (not +4.5V!)
out[0][i] = -0.5f;  // Outputs +4.5V (not -4.5V!)
```

**Common mistake**: Expecting audio outs to be "normal" polarity.

### CV Inputs: Expect 0V When Unpatched
**Critical**: Unpatched CV inputs read **0V**, not 2.5V!

```cpp
// ❌ WRONG: Assumes 0.5V baseline (2.5V on 0-5V scale)
float cv = patch.cv_in[0].Process();  // Returns 0.0f when unpatched
float modulated = knob + (cv - 0.5f);  // -0.5 offset breaks everything!

// ✅ CORRECT: Additive modulation (0V = no effect)
float cv = patch.cv_in[0].Process();
float modulated = Clamp(knob + cv, 0.0f, 1.0f);  // 0V leaves knob unchanged
```

**Why this matters**: See bug commit 96e206c.

## DaisySP Patterns

### Always Init() Before Use
**All DaisySP modules MUST be initialized before processing.**

```cpp
class MyProcessor {
private:
    daisysp::Oscillator osc_;
    daisysp::Svf filter_;

public:
    void Init(float sampleRate) {
        // ✅ REQUIRED: Init all DSP objects
        osc_.Init(sampleRate);
        osc_.SetFreq(440.0f);
        osc_.SetAmp(0.5f);

        filter_.Init(sampleRate);
        filter_.SetFreq(1000.0f);
        filter_.SetRes(0.7f);
    }

    float Process() {
        float sig = osc_.Process();
        filter_.Process(sig);
        return filter_.Low();  // ✅ Safe - Init() was called
    }
};
```

**Uninitialized modules will produce garbage or crash.**

### Process() Patterns

Most DaisySP modules follow one of these patterns:

#### Pattern 1: Single Sample In/Out
```cpp
daisysp::Svf filter;
filter.Init(sampleRate);

// In audio callback:
for (size_t i = 0; i < size; i++) {
    filter.Process(in[0][i]);
    out[0][i] = filter.Low();  // Get output after Process()
}
```

#### Pattern 2: Generate (No Input)
```cpp
daisysp::Oscillator osc;
osc.Init(sampleRate);

// In audio callback:
for (size_t i = 0; i < size; i++) {
    out[0][i] = osc.Process();  // No input, returns output
}
```

#### Pattern 3: Block Processing
```cpp
daisysp::ReverbSc reverb;
reverb.Init(sampleRate);

// In audio callback:
float outL, outR;
reverb.Process(in[0][i], in[1][i], &outL, &outR);
out[0][i] = outL;
out[1][i] = outR;
```

**Check DaisySP docs for each module's specific pattern!**

## Sample Rate and Block Size

### Current Configuration
```cpp
#define SAMPLE_RATE 48000.0f
#define BLOCK_SIZE 4  // Very small! Consider 16-32 for more headroom
```

### Time Budget
- **Block size 4** at 48kHz = 83µs per callback (tight!)
- **Block size 32** at 48kHz = 667µs per callback (recommended)

**Recommendation**: Use block size 16-32 for complex DSP. Size 4 is aggressive.

### Audio Callback Signature
```cpp
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // in[channel][sample]
    // out[channel][sample]
    // size = BLOCK_SIZE

    for (size_t i = 0; i < size; i++) {
        // Process sample-by-sample
        out[0][i] = ProcessLeft(in[0][i]);
        out[1][i] = ProcessRight(in[1][i]);
    }
}
```

## Conversion Macros

### Defined in config.h
```cpp
// CV voltage ↔ normalized (0-5V ↔ 0-1)
#define CV_TO_NORMALIZED(cv) ((cv) / CV_VOLTAGE_RANGE)  // 5V → 1.0
#define NORMALIZED_TO_CV(norm) ((norm) * CV_VOLTAGE_RANGE)  // 1.0 → 5V

// Frequency to radians (for oscillators)
#define FREQ_TO_RAD(freq, sr) ((freq) * 2.0f * 3.14159265359f / (sr))
```

### Usage
```cpp
// Reading CV input (already normalized by libDaisy)
float cv = patch.cv_in[0].Process();  // Returns 0.0-1.0

// Writing CV output
float voltage = NORMALIZED_TO_CV(0.8f);  // 4.0V
patch.WriteCvOut(0, voltage);

// Audio output is codec samples (±1.0 normalized)
out[0][i] = 0.5f;  // Codec handles conversion to voltage
```

## Common Traps

### Trap 1: Forgetting Init()
```cpp
// ❌ WRONG: No Init() called
daisysp::Oscillator osc;
osc.SetFreq(440.0f);
float sample = osc.Process();  // 🔥 GARBAGE OUTPUT!

// ✅ CORRECT: Init() before use
daisysp::Oscillator osc;
osc.Init(48000.0f);
osc.SetFreq(440.0f);
float sample = osc.Process();  // ✅ Works correctly
```

### Trap 2: Modifying Freq/Res Every Sample
**Bad for performance - control rate updates are sufficient.**

```cpp
// ❌ WRONG: Updating filter freq every sample
for (size_t i = 0; i < size; i++) {
    float newFreq = 1000.0f + lfo.Process() * 500.0f;
    filter.SetFreq(newFreq);  // ❌ Expensive! Called 4-32 times per block!
    filter.Process(in[0][i]);
    out[0][i] = filter.Low();
}

// ✅ BETTER: Update once per block
float newFreq = 1000.0f + lfo.Process() * 500.0f;
filter.SetFreq(newFreq);  // ✅ Called once per block

for (size_t i = 0; i < size; i++) {
    filter.Process(in[0][i]);
    out[0][i] = filter.Low();
}
```

**Exception**: Fast FM/modulation may require per-sample updates. Profile first.

### Trap 3: Assuming 0-10V Eurorack Standard
**Patch.Init CV is 0-5V, not 0-10V!**

```cpp
// ❌ WRONG: Thinking CV is 0-10V
float cv = patch.cv_in[0].Process() * 10.0f;  // ❌ Max is 5V!

// ✅ CORRECT: CV is 0-5V
float cv = patch.cv_in[0].Process() * 5.0f;   // ✅ Correct scaling
```

### Trap 4: Relying on Global State
**Audio callback runs in interrupt context - avoid global mutable state.**

```cpp
// ❌ RISKY: Global state shared with main loop
float globalFrequency = 440.0f;

void ProcessControls() {
    globalFrequency = mapKnobToFreq(knob.Process());  // Main loop
}

void AudioCallback(...) {
    osc.SetFreq(globalFrequency);  // ⚠️  Race condition!
}

// ✅ BETTER: Atomic or single-writer pattern
std::atomic<float> atomicFrequency{440.0f};

void ProcessControls() {
    atomicFrequency.store(mapKnobToFreq(knob.Process()));
}

void AudioCallback(...) {
    osc.SetFreq(atomicFrequency.load());
}
```

## Audio vs CV: When to Use Which?

| Use Case | Recommended Output | Range | Notes |
|----------|-------------------|-------|-------|
| 1V/oct pitch CV | CV out | 0-5V | Calibrated, unipolar |
| Gate/trigger | CV out | 0/5V | Clean square wave |
| Envelope | CV out | 0-5V | "Normal" Eurorack range |
| Bipolar LFO | Audio out | ±5V scaled | Attenuate from ±9V |
| Audio signal | Audio out | ±1.0 codec | Standard use |
| "Hot" modulation | Audio out | ±9V | For wavefolders, FX |

**See** `docs/chats/daisy-audio-vs-cv.md` for detailed comparison.

## Voltage Scaling Example

### Sample & Hold Velocity Output
**Goal**: Output 0-5V velocity on audio jacks (spec requirement).

```cpp
// Velocity is 0.0-1.0 normalized
float velocity = 0.8f;

// Audio codec expects ±1.0, but we want 0-5V output
// Need to convert: 0.0-1.0 → 0-5V → codec sample

// Method 1: Use GateScaler helper (project-specific)
out[0][i] = GateScaler::VoltageToCodecSample(velocity * 5.0f);

// Method 2: Manual scaling
// 0-5V on a ±9V output requires scaling and offset
float voltage = velocity * 5.0f;  // 0-5V
float codecSample = (voltage / 9.0f) - 0.5f;  // Approximate
out[0][i] = codecSample;
```

**Project uses**: `GateScaler::VoltageToCodecSample()` for 0-5V outputs on audio jacks.

## Testing DSP Code

### Use Approx() for Float Comparisons
```cpp
#include <catch2/catch_all.hpp>
using Catch::Approx;

TEST_CASE("Oscillator produces expected frequency") {
    daisysp::Oscillator osc;
    osc.Init(48000.0f);
    osc.SetFreq(440.0f);

    float sample = osc.Process();

    // ❌ WRONG: Exact float comparison
    REQUIRE(sample == 0.5f);  // Will fail due to float precision!

    // ✅ CORRECT: Use Approx()
    REQUIRE(sample == Approx(0.5f).margin(0.01f));
}
```

### Deterministic Testing
**Use fixed seeds for RNG to make tests reproducible.**

```cpp
TEST_CASE("Pattern generation is deterministic") {
    const uint32_t seed = 0x12345678;

    auto pattern1 = GeneratePattern(seed);
    auto pattern2 = GeneratePattern(seed);

    REQUIRE(pattern1 == pattern2);  // Same seed = same output
}
```

## References

- DaisySP Documentation: https://github.com/electro-smith/DaisySP
- Patch.Init Spec: https://www.electro-smith.com/daisy/patch
- Audio vs CV guide: `docs/chats/daisy-audio-vs-cv.md`
- Real-time safety: `docs/code-review/realtime-audio-safety.md`
