# Testing Standards and Best Practices

Project uses Catch2 for host-compiled unit tests. Tests run on development machine, not on hardware.

## Test Organization

### File Naming
- Pattern: `test_<module>.cpp`
- Examples: `test_timing.cpp`, `test_pattern_field.cpp`, `test_sequencer.cpp`

### Test Location
```
tests/
├── test_main.cpp          # Catch2 main()
├── test_timing.cpp        # BrokenEffects timing tests
├── test_generation.cpp    # Pattern generation tests
├── test_sequencer.cpp     # Full sequencer integration
└── ...
```

## Test Structure

### Use Descriptive Names and Tags
```cpp
TEST_CASE("ComputeSwing multiplies archetype base by config", "[timing][swing][config]")
{
    // Test body
}

TEST_CASE("DENSITY=0 guarantees zero hits", "[generation][invariants][regression]")
{
    // Test body
}
```

**Tags** help filter test runs:
- `[timing]` - timing/swing/jitter tests
- `[generation]` - pattern generation
- `[invariants]` - critical design rules
- `[regression]` - documents a bug fix
- `[config]` - config parameter behavior

### Test What the Code Does, Not What It Is
```cpp
// ❌ WEAK: Only checks output type/range
TEST_CASE("ComputeSwing returns a float") {
    float result = ComputeSwing(0.5f, 0.5f, EnergyZone::PEAK);
    REQUIRE(result >= 0.5f);
    REQUIRE(result <= 0.7f);
    // ✅ Passes even if swing parameter ignored!
}

// ✅ STRONG: Verifies parameter actually affects output
TEST_CASE("ComputeSwing uses swing config parameter", "[swing][config][regression]") {
    float low  = ComputeSwing(0.0f, 0.5f, EnergyZone::PEAK);
    float high = ComputeSwing(1.0f, 0.5f, EnergyZone::PEAK);

    REQUIRE(low < high);  // ✅ Fails if parameter ignored!
    REQUIRE(high == Approx(0.70f).margin(0.01f));  // ✅ Checks actual value
}
```

## Float Comparisons

### Always Use Approx()
```cpp
#include <catch2/catch_all.hpp>
using Catch::Approx;

// ❌ WRONG: Exact float comparison (will fail!)
REQUIRE(result == 0.5f);

// ✅ CORRECT: Use Approx with margin
REQUIRE(result == Approx(0.5f).margin(0.01f));

// ✅ ALTERNATIVE: Use epsilon for relative tolerance
REQUIRE(result == Approx(0.5f).epsilon(0.001));  // 0.1% tolerance
```

### Choosing Margins
- **DSP calculations**: `margin(0.01f)` or `epsilon(0.001)` (0.1%)
- **Timing offsets**: `margin(0.001f)` (tighter for sample-accurate timing)
- **Control values**: `margin(0.05f)` (looser for UI mappings)

## Testing Invariants

### Critical Design Rules Must Be Tested
**Invariants** are rules that must ALWAYS hold. Test them exhaustively.

```cpp
TEST_CASE("DENSITY=0 guarantees zero hits", "[generation][invariants]") {
    for (int trial = 0; trial < 100; trial++) {
        uint32_t seed = 0x1000 + trial;

        auto pattern = GeneratePattern(
            /* energy */ 0.0f,  // DENSITY=0
            /* fieldX */ 0.5f,
            /* fieldY */ 0.5f,
            /* seed */ seed
        );

        int hitCount = CountHits(pattern);
        REQUIRE(hitCount == 0);  // ✅ HARD invariant - must be zero!
    }
}

TEST_CASE("DRIFT=0 prevents pattern evolution", "[generation][invariants]") {
    uint32_t seed = 0x12345678;

    auto pattern1 = GeneratePattern(/* drift */ 0.0f, seed);
    auto pattern2 = GeneratePattern(/* drift */ 0.0f, seed);

    REQUIRE(pattern1 == pattern2);  // ✅ Same seed + drift=0 = identical
}
```

**From spec**: DENSITY=0 and DRIFT=0 are **critical invariants** (see bug commits 3ddcfc6, 0289e38).

## Regression Tests

### Document Bug Fixes with Tests
**Every bug fix should add a regression test** to prevent re-introduction.

```cpp
// Bug: Swing config was ignored (used flavorCV instead)
// Fixed in: commit be7b58f
TEST_CASE("ComputeSwing uses swing config, not flavorCV", "[timing][swing][config][regression]")
{
    // This test documents the bug fix from Modification 0.6:
    // ComputeSwing was incorrectly using flavorCV instead of swing config.

    const float archetypeBase = 0.50f;

    // Config at 0% should give minimal swing
    REQUIRE(ComputeSwing(0.0f, archetypeBase, EnergyZone::PEAK)
            == Approx(0.50f).margin(0.001f));

    // Config at 100% should give maximum swing
    REQUIRE(ComputeSwing(1.0f, archetypeBase, EnergyZone::PEAK)
            == Approx(0.70f).margin(0.001f));
}
```

**Pattern**:
1. Name the bug in comments
2. Reference commit hash
3. Tag as `[regression]`
4. Test the specific failure case

## Boundary Testing

### Test at Key Thresholds
```cpp
TEST_CASE("Clock division mapping centers ×1 at 50%", "[controls][config]") {
    // Test boundaries around ×1 detent (42-58% per spec)

    // Just below ×1 range → ÷2
    REQUIRE(MapToClockDivision(0.41f) == 4);

    // Within ×1 range → ×1
    REQUIRE(MapToClockDivision(0.42f) == 8);
    REQUIRE(MapToClockDivision(0.50f) == 8);  // ✅ 12 o'clock!
    REQUIRE(MapToClockDivision(0.58f) == 8);

    // Just above ×1 range → ×2
    REQUIRE(MapToClockDivision(0.59f) == 16);
}
```

**Test** at: 0%, 25%, 50%, 75%, 100%, and threshold boundaries.

## Deterministic Testing

### Use Fixed Seeds for RNG
```cpp
TEST_CASE("Pattern generation is deterministic", "[generation]") {
    const uint32_t seed = 0x12345678;

    auto pattern1 = GeneratePattern(seed);
    auto pattern2 = GeneratePattern(seed);

    REQUIRE(pattern1 == pattern2);  // Same seed = same output
}

TEST_CASE("Different seeds produce different patterns", "[generation]") {
    auto pattern1 = GeneratePattern(0x1000);
    auto pattern2 = GeneratePattern(0x2000);

    REQUIRE(pattern1 != pattern2);  // Different seeds = variation
}
```

**Why**: Makes failures reproducible. No flaky tests.

## Integration Testing

### Test Full Pipelines, Not Just Units
```cpp
TEST_CASE("Full generation pipeline produces valid pattern", "[sequencer][integration]") {
    Sequencer seq;
    seq.Init(48000.0f);

    // Set realistic control values
    seq.SetEnergy(0.7f);
    seq.SetBuild(0.5f);
    seq.SetFieldX(0.6f);
    seq.SetFieldY(0.4f);
    seq.SetGenre(Genre::TECHNO);

    // Trigger generation
    seq.OnBarStart();

    // Verify output is valid
    auto pattern = seq.GetCurrentPattern();
    REQUIRE(pattern.anchorHitCount > 0);
    REQUIRE(pattern.shimmerHitCount > 0);

    // Verify first hit is valid
    REQUIRE(pattern.anchorHits[0].step >= 0);
    REQUIRE(pattern.anchorHits[0].step < 32);
    REQUIRE(pattern.anchorHits[0].velocity > 0.0f);
    REQUIRE(pattern.anchorHits[0].velocity <= 1.0f);
}
```

## Performance Testing (Optional)

### Time-Critical Paths
For audio-rate code, consider timing tests:

```cpp
TEST_CASE("Audio callback completes within time budget", "[performance]") {
    Sequencer seq;
    seq.Init(48000.0f);

    const size_t blockSize = 32;
    const size_t iterations = 1000;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; i++) {
        for (size_t j = 0; j < blockSize; j++) {
            seq.ProcessAudio();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 32 samples at 48kHz = 667µs budget per block
    // With 1000 iterations, average should be well under budget
    double avgBlockTime = duration.count() / (double)iterations;

    INFO("Average block time: " << avgBlockTime << "µs");
    REQUIRE(avgBlockTime < 500.0);  // Comfortable margin under 667µs
}
```

**Note**: Host timing != hardware timing, but gives rough order of magnitude.

## Test Output

### Use INFO() for Context
```cpp
TEST_CASE("Velocity scaling is correct", "[velocity]") {
    for (float punch = 0.0f; punch <= 1.0f; punch += 0.25f) {
        PunchParams params;
        ComputePunch(punch, params);

        INFO("Testing punch = " << punch);
        REQUIRE(params.accentBoost >= params.velocityFloor);
        // If this fails, INFO shows which punch value failed
    }
}
```

### Use SECTION() for Subtests
```cpp
TEST_CASE("Pattern field blending", "[pattern_field]") {
    SECTION("Corner positions use single archetype") {
        // Test (0,0), (0,1), (1,0), (1,1)
    }

    SECTION("Center position blends all four") {
        // Test (0.5, 0.5)
    }

    SECTION("Edge positions blend two archetypes") {
        // Test (0.5, 0), (0.5, 1), (0, 0.5), (1, 0.5)
    }
}
```

## Test Coverage Goals

### What to Test
- ✅ Public API functions
- ✅ Critical invariants (DENSITY=0, DRIFT=0)
- ✅ Boundary conditions (0%, 50%, 100%)
- ✅ Config parameter effects (verify they're used!)
- ✅ Regression cases (every bug fix)
- ✅ Integration paths (full pipeline)

### What NOT to Test
- ❌ Private implementation details (test public interface)
- ❌ DaisySP internals (trust the library)
- ❌ Exact DSP output (test ranges, not specific floats)

## Running Tests

### Build and Run
```bash
# Build tests
make test

# Run all tests
./build/test_runner

# Run specific tag
./build/test_runner "[timing]"

# Run with verbose output
./build/test_runner -s  # Show successful tests too
```

### CI Integration (Future)
Tests should pass on every commit. Consider GitHub Actions for automated testing.

## Example: Comprehensive Test Case

```cpp
TEST_CASE("Euclidean generation produces evenly-spaced hits", "[euclidean][generation]") {
    SECTION("3 hits in 8 steps") {
        auto pattern = GenerateEuclidean(3, 8);

        REQUIRE(pattern.size() == 8);

        // Count hits
        int hitCount = 0;
        for (bool hit : pattern) {
            if (hit) hitCount++;
        }
        REQUIRE(hitCount == 3);

        // Verify spacing (should be 0,3,5 or similar)
        std::vector<int> hitPositions;
        for (size_t i = 0; i < pattern.size(); i++) {
            if (pattern[i]) hitPositions.push_back(i);
        }

        INFO("Hit positions: " << hitPositions[0] << ", "
             << hitPositions[1] << ", " << hitPositions[2]);

        // Check they're spread out (no consecutive hits)
        for (size_t i = 1; i < hitPositions.size(); i++) {
            int gap = hitPositions[i] - hitPositions[i-1];
            REQUIRE(gap >= 2);  // Minimum spacing
        }
    }

    SECTION("Edge cases") {
        // 0 hits
        auto empty = GenerateEuclidean(0, 8);
        REQUIRE(std::count(empty.begin(), empty.end(), true) == 0);

        // Full pattern
        auto full = GenerateEuclidean(8, 8);
        REQUIRE(std::count(full.begin(), full.end(), true) == 8);
    }
}
```

## Quick Reference

| Do | Don't |
|----|-------|
| Use `Approx()` for floats | Exact float comparison (==) |
| Test parameter effects | Test only output types |
| Add regression tests for bugs | Fix bugs without tests |
| Use descriptive names + tags | Generic test names |
| Test at boundaries (0%, 50%, 100%) | Test only middle values |
| Use fixed seeds for RNG | Non-deterministic tests |
| Tag with `[regression]` for bug fixes | Leave bug fixes undocumented |
| Test invariants exhaustively | Assume invariants hold |

## References

- Catch2 docs: https://github.com/catchorg/Catch2
- Common pitfalls: `docs/code-review/common-pitfalls.md`
- Example tests: `tests/test_timing.cpp`, `tests/test_generation.cpp`
