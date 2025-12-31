# C++ Coding Standards

Project uses C++14 for embedded STM32H7 target. Follow these standards for consistency.

## Naming Conventions

### Classes and Structs
**PascalCase** - capitalize first letter of each word, no underscores.

```cpp
// ✅ Correct
class PatternField { };
struct ArchetypeDNA { };
class VelocityCompute { };

// ❌ Wrong
class pattern_field { };      // snake_case
class patternField { };        // camelCase
class PATTERN_FIELD { };       // SCREAMING_SNAKE_CASE
```

### Functions and Methods
**camelCase** - lowercase first word, capitalize subsequent words.

```cpp
// ✅ Correct
void processAudio();
float computeVelocity(int step);
bool isGateHigh(int voice);

// ❌ Wrong
void ProcessAudio();           // PascalCase
void process_audio();          // snake_case
void PROCESS_AUDIO();          // SCREAMING_SNAKE_CASE
```

### Variables and Parameters
**camelCase** - same as functions.

```cpp
// ✅ Correct
float sampleRate;
int patternLength;
bool isAccented;

// ❌ Wrong
float SampleRate;              // PascalCase
int pattern_length;            // snake_case
```

### Constants
**UPPER_SNAKE_CASE** - all caps with underscores.

```cpp
// ✅ Correct
constexpr int MAX_PATTERN_LENGTH = 64;
constexpr float DEFAULT_SWING = 0.50f;
#define SAMPLE_RATE 48000.0f

// ❌ Wrong
constexpr int maxPatternLength = 64;   // camelCase
constexpr float DefaultSwing = 0.50f;  // PascalCase
```

### Private Members
**camelCase with trailing underscore**.

```cpp
class Sequencer {
private:
    float sampleRate_;        // ✅ Trailing underscore
    int currentStep_;
    bool isPlaying_;

public:
    void Init(float sampleRate) {
        sampleRate_ = sampleRate;
        currentStep_ = 0;
        isPlaying_ = false;
    }
};
```

### Namespaces
**snake_case** - all lowercase with underscores.

```cpp
// ✅ Correct
namespace daisysp_idm_grids {
    class PatternField { };
}

// ❌ Wrong
namespace DaisyspIdmGrids { }   // PascalCase
namespace DAISYSP_IDM_GRIDS { } // SCREAMING_SNAKE_CASE
```

## File Organization

### Header Files (.h)
```cpp
#pragma once  // ✅ Use pragma once, not include guards

#include <cstdint>  // System headers first
#include "DuoPulseTypes.h"  // Project headers second

namespace daisysp_idm_grids
{

/**
 * Brief description of class purpose
 *
 * Longer description if needed. Reference spec sections:
 * Reference: docs/specs/main.md section X.Y
 */
class MyClass
{
public:
    /** Brief method description */
    void Init(float sampleRate);

    /**
     * More detailed description
     *
     * @param param1 Description of param1
     * @param param2 Description of param2
     * @return Description of return value
     */
    float Process(float input);

private:
    float sampleRate_;
};

} // namespace daisysp_idm_grids
```

### Implementation Files (.cpp)
```cpp
#include "MyClass.h"

#include <cmath>  // System headers
#include "OtherClass.h"  // Other project headers

namespace daisysp_idm_grids
{

void MyClass::Init(float sampleRate)
{
    sampleRate_ = sampleRate;
    // Implementation...
}

float MyClass::Process(float input)
{
    // Implementation...
    return input * 0.5f;
}

} // namespace daisysp_idm_grids
```

## Documentation Comments

### Class Documentation
```cpp
/**
 * VelocityCompute: Velocity computation for DuoPulse v4
 *
 * Velocity is controlled by the PUNCH parameter, which sets the dynamic
 * contrast between accented and ghost hits. BUILD adds phrase-arc
 * modulation, and accent masks determine which steps get emphasized.
 *
 * Reference: docs/specs/main.md section 7.2
 */
class VelocityCompute { };
```

### Function Documentation
```cpp
/**
 * Compute velocity for a step based on PUNCH, BUILD, and accent status.
 *
 * The velocity computation pipeline:
 * 1. Determine base velocity from velocityFloor
 * 2. If accented, add accentBoost
 * 3. Apply BUILD modifiers (fill intensity, phrase position)
 * 4. Add random variation (from velocityVariation)
 * 5. Clamp to valid range
 *
 * @param punchParams Computed PUNCH parameters
 * @param buildMods Computed BUILD modifiers
 * @param isAccent Whether this step should be accented
 * @param seed Seed for deterministic randomness
 * @param step Step index for per-step variation
 * @return Computed velocity (0.0-1.0)
 */
float ComputeVelocity(const PunchParams& punchParams,
                      const BuildModifiers& buildMods,
                      bool isAccent,
                      uint32_t seed,
                      int step);
```

### Inline Comments
Use for complex logic, not obvious statements.

```cpp
// ✅ Good: Explains WHY
// Swing only affects offbeats (odd steps)
if (step % 2 == 1) {
    offset += swingAmount * samplesPerStep;
}

// ❌ Bad: States the obvious
// Set offset to swing amount times samples per step
offset = swingAmount * samplesPerStep;
```

## Code Style

### Braces
**Always use braces**, even for single-line blocks. Opening brace on same line.

```cpp
// ✅ Correct
if (condition) {
    doSomething();
}

// ❌ Wrong: No braces
if (condition)
    doSomething();

// ❌ Wrong: Brace on new line
if (condition)
{
    doSomething();
}
```

### Indentation
**4 spaces** (no tabs).

```cpp
class MyClass {
public:
    void Init() {
        if (condition) {
            for (int i = 0; i < 10; i++) {
                process(i);
            }
        }
    }
};
```

### Line Length
**Prefer <100 characters** per line. Break long lines at logical points.

```cpp
// ✅ Good: Readable line breaks
void ComputeVelocity(const PunchParams& punchParams,
                     const BuildModifiers& buildMods,
                     bool isAccent,
                     uint32_t seed,
                     int step);

// ❌ Bad: Too long
void ComputeVelocity(const PunchParams& punchParams, const BuildModifiers& buildMods, bool isAccent, uint32_t seed, int step);
```

### Whitespace
- Space after keywords: `if (`, `for (`, `while (`
- No space before function call parenthesis: `process()`
- Space around operators: `a + b`, `x == y`

```cpp
// ✅ Correct
if (x > 0) {
    result = compute(x + y);
}

// ❌ Wrong
if(x>0){
    result=compute (x+y);
}
```

## Modern C++ Practices

### Prefer constexpr for Compile-Time Constants
```cpp
// ✅ Good: constexpr (type-safe, scoped)
constexpr int MAX_VOICES = 2;
constexpr float DEFAULT_SWING = 0.50f;

// ❌ Avoid: #define (not type-safe)
#define MAX_VOICES 2
```

**Exception**: Config macros in `inc/config.h` use `#define` for compatibility.

### Use auto for Complex Types
```cpp
// ✅ Good: auto improves readability
auto archetype = GetBlendedArchetype(field, fieldX, fieldY);

// ❌ Verbose: Full type
ArchetypeDNA archetype = GetBlendedArchetype(field, fieldX, fieldY);
```

**But**: Don't obscure obvious types.

```cpp
// ❌ Bad: Unnecessary auto
auto x = 0;       // Just write int x = 0;
auto y = 0.5f;    // Just write float y = 0.5f;
```

### Prefer nullptr over NULL
```cpp
// ✅ Correct
float* ptr = nullptr;

// ❌ Wrong (C-style)
float* ptr = NULL;
```

### Use Range-Based For Loops
```cpp
// ✅ Good: Range-based for
for (const auto& hit : pattern.anchorHits) {
    processHit(hit);
}

// ❌ Verbose: Index-based
for (size_t i = 0; i < pattern.anchorHits.size(); i++) {
    processHit(pattern.anchorHits[i]);
}
```

## Embedded-Specific Guidelines

### No Exceptions
**Avoid exceptions** (disabled for embedded targets).

```cpp
// ❌ Don't use try/catch
try {
    processAudio();
} catch (...) {
    handleError();
}

// ✅ Use return codes or asserts
bool processAudio() {
    if (!initialized) {
        return false;  // Error condition
    }
    // Process...
    return true;
}
```

### No RTTI
**Avoid `dynamic_cast` and `typeid`** (RTTI disabled).

```cpp
// ❌ Don't use dynamic_cast
BaseClass* base = getObject();
DerivedClass* derived = dynamic_cast<DerivedClass*>(base);

// ✅ Use static polymorphism or explicit type checks
```

### Minimize Dynamic Allocation
**Avoid heap allocation** in performance-critical paths.

```cpp
// ❌ Bad: Allocates in process loop
void process() {
    std::vector<float> buffer;  // Potential allocation!
    buffer.push_back(sample);
}

// ✅ Good: Pre-allocated buffer
class Processor {
private:
    float buffer_[MAX_SIZE];
    size_t bufferIndex_;

public:
    void process() {
        buffer_[bufferIndex_++] = sample;
    }
};
```

### Use Fixed-Size Arrays
```cpp
// ✅ Good: Stack allocation, fixed size
float buffer[64];
Hit hits[MAX_HITS];

// ❌ Risky: Heap allocation
std::vector<float> buffer;
std::vector<Hit> hits;
```

## Type Safety

### Use Enums for Discrete Values
```cpp
// ✅ Good: Type-safe enum
enum class Voice {
    ANCHOR = 0,
    SHIMMER = 1,
    AUX = 2
};

Voice voice = Voice::ANCHOR;

// ❌ Bad: Magic numbers
int voice = 0;  // What does 0 mean?
```

### Use Structs for Related Data
```cpp
// ✅ Good: Structured data
struct PunchParams {
    float velocityFloor;
    float accentBoost;
    float velocityVariation;
    float accentProbability;
};

// ❌ Bad: Unrelated parameters
void compute(float a, float b, float c, float d);  // What are these?
```

### Use `const` Liberally
```cpp
// ✅ Good: const correctness
void process(const float* input, float* output, size_t size) const {
    // Input buffer is read-only
    // Method doesn't modify object state
}

// Mark parameters const if not modified
float compute(const PunchParams& params);
```

## Error Handling

### Use Asserts for Invariants
```cpp
#include <cassert>

void setEnergy(float energy) {
    assert(energy >= 0.0f && energy <= 1.0f);  // Debug builds only
    energy_ = energy;
}
```

**Note**: Asserts are disabled in release builds. For critical checks, use explicit validation.

### Validate Inputs
```cpp
// ✅ Good: Explicit validation
void setPatternLength(int length) {
    if (length < 16 || length > 64) {
        // Handle error (clamp, return error code, etc.)
        length = 32;  // Safe default
    }
    patternLength_ = length;
}
```

## Formatting Quick Reference

```cpp
// Class definition
class MyClass
{
public:
    void publicMethod();

private:
    int privateData_;
};

// Function definition
void MyClass::publicMethod()
{
    if (condition) {
        for (int i = 0; i < count; i++) {
            process(i);
        }
    }
}

// Namespace
namespace daisysp_idm_grids
{
    // Content indented
    class MyClass { };
} // namespace daisysp_idm_grids
```

## File Headers

**Minimal headers** - no copyright/license boilerplate unless required.

```cpp
/**
 * Brief description of file purpose
 */

#pragma once

#include <cstdint>
// ...
```

## Summary Table

| Element | Style | Example |
|---------|-------|---------|
| Class/Struct | PascalCase | `PatternField` |
| Function/Method | camelCase | `processAudio()` |
| Variable/Parameter | camelCase | `sampleRate` |
| Constant | UPPER_SNAKE_CASE | `MAX_VOICES` |
| Private Member | camelCase_ | `sampleRate_` |
| Namespace | snake_case | `daisysp_idm_grids` |
| Enum | PascalCase | `Voice::ANCHOR` |
| File | PascalCase.h/.cpp | `PatternField.h` |

## Tools

Project doesn't use auto-formatting (clang-format, etc). Follow these standards manually.

## References

- C++14 standard (target for embedded)
- CLAUDE.md (project overview)
- docs/specs/main.md (specification)
