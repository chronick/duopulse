# DaisySP Patch.Init Eurorack Firmware

Custom firmware for the Patch.Init Eurorack module using DaisySP DSP library.

## Overview

This project provides a custom firmware implementation for the Electro-Smith Patch.Init Eurorack module. The Patch.Init is a versatile eurorack module featuring:

- 2x CV inputs (±5V)
- 2x CV outputs (±5V)
- 2x audio inputs
- 2x audio outputs
- Multiple GPIO pins for buttons, LEDs, and encoders
- STM32H7 microcontroller with floating-point unit

## DuoPulse v3: 2-Voice Percussive Sequencer

DuoPulse is an opinionated 2-voice percussion sequencer with a focus on electronic music from club-ready techno to experimental IDM. Version 3 introduces an **algorithmic pulse field** that replaces discrete pattern lookup with continuous variation.

### Core Concept

Two simple axes control everything:

| Axis | Control | Effect |
|------|---------|--------|
| **BROKEN** | K3 | Pattern regularity (0%=4/4 techno → 100%=IDM chaos) |
| **DRIFT** | K4 | Pattern evolution (0%=locked → 100%=generative) |

Genre character (swing, jitter, feel) **emerges** from the BROKEN parameter—no explicit genre selection needed.

### Control Layout

#### Performance Mode (Switch DOWN)

| Knob | Primary | +Shift |
|------|---------|--------|
| K1 | Anchor Density | FUSE (voice balance) |
| K2 | Shimmer Density | LENGTH (1-16 bars) |
| K3 | BROKEN | COUPLE (voice interlock) |
| K4 | DRIFT | RATCHET (fill intensity) |

#### Config Mode (Switch UP)

| Knob | Primary | +Shift |
|------|---------|--------|
| K1 | Anchor Accent | Swing Taste |
| K2 | Shimmer Accent | Gate Time |
| K3 | Contour | Humanize |
| K4 | Tempo / Clock Div* | Clock Div / Aux Mode* |

> *K4 is context-aware: behavior changes when external clock is patched.

### BROKEN × DRIFT Interaction

| BROKEN | DRIFT | Result |
|--------|-------|--------|
| Low | Low | Solid, repeating groove (DJ tool) |
| Low | High | Stable groove with living details |
| High | Low | Broken but consistent each loop |
| High | High | Maximum generative chaos (experimental) |

### RATCHET: Fill Intensity Control

**RATCHET** (K4+Shift) controls fill intensity during phrase transitions. Works together with DRIFT:

- **DRIFT** controls fill **probability** (when fills occur)
- **RATCHET** controls fill **intensity** (how intense fills are)
- At DRIFT=0, RATCHET has no effect (no fills occur)
- At RATCHET > 50%, ratcheting (32nd note subdivisions) can occur in fill zones

| RATCHET Level | Behavior |
|---------------|----------|
| 0% | Subtle fills — slight density boost only |
| 25% | Moderate fills — noticeable energy increase |
| 50% | Standard fills — clear rhythmic intensification |
| 75% | Aggressive fills — ratcheted 16ths, strong build |
| 100% | Maximum intensity — rapid-fire hits, peak energy |

### CV Inputs (Always Performance Parameters)

| CV | Modulates |
|----|-----------|
| CV 5 | Anchor Density |
| CV 6 | Shimmer Density |
| CV 7 | BROKEN |
| CV 8 | DRIFT |

### v3 vs v2 Mode

The firmware supports both v3 (Pulse Field) and v2 (Pattern Skeleton) algorithms. Toggle in `inc/config.h`:

```cpp
#define USE_PULSE_FIELD_V3 1   // v3: Algorithmic pulse field
// #define USE_PULSE_FIELD_V3 1  // Comment out for v2: Pattern skeletons
```

See `docs/specs/main.md` for the complete specification.

## Prerequisites

- **Hardware**: Electro-Smith Patch.Init Eurorack module
- **Software**:
  - ARM GCC toolchain (for STM32)
  - `arm-none-eabi-gcc` compiler
  - `make` build system
  - `dfu-util` for firmware deployment (or STM32CubeProgrammer)
  - **For testing**: Catch2 testing framework (optional)
- **DaisySP**: Included as a git submodule

### Installing Dependencies

**macOS** (using Homebrew):
```bash
brew install arm-none-eabi-gcc dfu-util
```

**Linux** (Debian/Ubuntu):
```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi dfu-util make
```

**Catch2** (for unit testing):
```bash
# Option 1: Install system-wide (recommended)
git clone https://github.com/catchorg/Catch2.git
cd Catch2
cmake -Bbuild -H. -DBUILD_TESTING=OFF
sudo cmake --build build/ --target install

# Option 2: Use package manager
# macOS: brew install catch2
# Linux: sudo apt-get install catch2 (if available)
```

## Project Structure

```
.
├── DaisySP/              # DaisySP library (git submodule)
├── src/                  # Source files
│   ├── main.cpp         # Main firmware entry point
│   └── ...
├── inc/                  # Header files
│   └── ...
├── tests/                # Unit tests
│   ├── test_main.cpp    # Test runner
│   └── ...
├── Makefile             # Build configuration
├── setup.sh             # Setup script for initial configuration
├── .cursor/             # Cursor IDE rules
│   └── rules
└── README.md            # This file
```

## Build Instructions

### Initial Setup

**Quick Setup** (recommended):
```bash
# Clone the repository
git clone --recursive https://github.com/yourusername/daisysp-idm-grids.git
cd daisysp-idm-grids

# Run setup script (initializes submodules and builds DaisySP)
./setup.sh

# Build the firmware
make
```

**Manual Setup**:
1. **Clone the repository** (including submodules):
   ```bash
   git clone --recursive https://github.com/yourusername/daisysp-idm-grids.git
   cd daisysp-idm-grids
   ```

   If you've already cloned without submodules:
   ```bash
   git submodule update --init --recursive
   ```

2. **Build DaisySP library**:
   ```bash
   cd DaisySP
   make
   cd ..
   ```

3. **Build the firmware**:
   ```bash
   make
   ```

### Build Targets

- `make` or `make all` - Build the firmware
- `make clean` - Remove build artifacts
- `make rebuild` - Clean and rebuild
- `make test` - Build and run unit tests
- `make program` - Flash firmware to Patch.Init module (requires DFU mode)
- `make daisy-update` - Update DaisySP submodule to latest version

### Build Configuration

The Makefile supports the following variables:

- `DAISYSP_PATH` - Path to DaisySP (default: `./DaisySP`)
- `BUILD_DIR` - Build output directory (default: `build`)
- `TARGET` - Target board (default: `patch`)
- `DEBUG` - Enable debug symbols (set `DEBUG=1`)

Example:
```bash
make DEBUG=1 BUILD_DIR=debug_build
```

## Test Instructions

### Running Unit Tests

The project includes a unit test framework using Catch2:

1. **Run all tests**:
   ```bash
   make test
   ```

2. **Run specific test**:
   ```bash
   ./build/test_runner [test_name]
   ```

3. **Run tests with verbose output**:
   ```bash
   ./build/test_runner --success
   ```

### Writing Tests

Tests are located in the `tests/` directory. Each test file should:

- Include the Catch2 header: `#include <catch2/catch.hpp>`
- Use `TEST_CASE()` macro for test cases
- Use `REQUIRE()` or `CHECK()` for assertions

Example:
```cpp
#include <catch2/catch.hpp>
#include "../src/my_module.h"

TEST_CASE("MyModule processes audio correctly")
{
    MyModule module;
    module.Init(48000.0f);
    
    float input = 0.5f;
    float output = module.Process(input);
    
    REQUIRE(output >= -1.0f);
    REQUIRE(output <= 1.0f);
}
```

### Test Coverage

To generate test coverage reports:

```bash
make test-coverage
```

This requires `gcov` and `lcov` to be installed.

## Deploy Instructions

### Preparing the Module

1. **Enter DFU (Device Firmware Update) mode**:
   - Power off the module
   - Hold the BOOT button (if available) or use the DFU jumper
   - Power on the module while holding BOOT
   - Release BOOT button
   - The module should now be in DFU mode

2. **Verify DFU mode**:
   ```bash
   dfu-util --list
   ```
   You should see the STM32 device listed.

### Flashing Firmware

**Method 1: Using Makefile** (recommended)
```bash
make program
```

**Method 2: Using dfu-util directly**
```bash
dfu-util -a 0 -s 0x08000000:leave -D build/patch-init-firmware.bin
```

**Method 3: Using STM32CubeProgrammer**
1. Open STM32CubeProgrammer
2. Select "USB" connection
3. Connect to the device
4. Load the `.bin` or `.hex` file from `build/`
5. Click "Download"

### Post-Flash

After flashing:
1. Power cycle the module (or use `:leave` flag with dfu-util)
2. The module should boot with the new firmware
3. Test audio and CV I/O to verify functionality

### Troubleshooting

- **Device not found**: Ensure module is in DFU mode and USB cable is connected
- **Permission denied**: Add udev rules for STM32 DFU devices (Linux) or run with sudo
- **Flash fails**: Verify module is in DFU mode and try resetting the module

## Upgrading DaisySP

### Update to Latest Version

1. **Navigate to DaisySP directory**:
   ```bash
   cd DaisySP
   ```

2. **Fetch and checkout latest version**:
   ```bash
   git fetch origin
   git checkout master  # or specific tag/commit
   git pull origin master
   ```

3. **Rebuild DaisySP**:
   ```bash
   make clean
   make
   ```

4. **Return to project root and rebuild**:
   ```bash
   cd ..
   make clean
   make
   ```

### Update to Specific Version/Tag

```bash
cd DaisySP
git fetch --tags
git checkout v1.0.0  # Replace with desired version
make clean
make
cd ..
make clean
make
```

### Update Submodule Reference

After updating DaisySP, commit the submodule reference:

```bash
git add DaisySP
git commit -m "Update DaisySP to version X.X.X"
```

### Checking Current Version

```bash
cd DaisySP
git describe --tags
cd ..
```

## Development Workflow

1. **Create a feature branch**:
   ```bash
   git checkout -b feature/my-feature
   ```

2. **Make changes and test**:
   ```bash
   make clean
   make
   make test
   ```

3. **Flash and test on hardware**:
   ```bash
   make program
   ```

4. **Commit changes**:
   ```bash
   git add .
   git commit -m "Description of changes"
   ```

5. **Push and create pull request**

## Hardware Configuration

### Patch.Init Pin Assignments

Refer to the Patch.Init documentation for specific pin assignments. Common configurations:

- **CV Inputs**: ADC channels 0-1
- **CV Outputs**: DAC channels 0-1
- **Audio Inputs**: I2S/SAI interface
- **Audio Outputs**: I2S/SAI interface
- **GPIO**: Configurable via `daisy::GPIO`

### Sample Rate

Default sample rate: **48kHz**

To change:
```cpp
patch.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_96KHZ);
```

### Block Size

Default block size: **4 samples**

To change:
```cpp
patch.SetAudioBlockSize(8);
```

## Contributing

1. Follow the coding standards in `.cursor/rules`
2. Write unit tests for new features
3. Update documentation as needed
4. Test on hardware before submitting PR

## License

[Specify your license here]

## Resources

- [DaisySP Documentation](https://github.com/electro-smith/DaisySP)
- [Daisy Hardware Documentation](https://github.com/electro-smith/DaisyWiki)
- [Patch.Init Module Info](https://www.electro-smith.com/daisy/patch)
- [STM32 Documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32h7-series.html)

## Troubleshooting

### Build Issues

- **Missing toolchain**: Install ARM GCC toolchain
- **DaisySP not found**: Run `git submodule update --init --recursive`
- **Linker errors**: Ensure DaisySP is built (`cd DaisySP && make`)

### Runtime Issues

- **No audio output**: Check audio callback registration and hardware connections
- **CV not working**: Verify ADC/DAC configuration and pin assignments
- **Module crashes**: Check for stack overflow, uninitialized variables, or division by zero

### Debugging

Enable debug output:
```bash
make DEBUG=1
```

Use a debugger (OpenOCD + GDB) for hardware debugging:
```bash
make debug
```

## Support

For issues and questions:
- Open an issue on GitHub
- Check the DaisySP documentation
- Visit the [Daisy Forum](https://forum.electro-smith.com/)

