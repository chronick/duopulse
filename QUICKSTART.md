# Quick Start Guide

Get up and running with the DaisySP Patch.Init firmware in minutes.

## Prerequisites Check

```bash
# Check for ARM toolchain
arm-none-eabi-gcc --version

# Check for make
make --version

# Check for dfu-util (optional, for flashing)
dfu-util --version
```

## Setup (5 minutes)

```bash
# 1. Clone the repository
git clone --recursive https://github.com/yourusername/daisysp-idm-grids.git
cd daisysp-idm-grids

# 2. Run setup script
./setup.sh

# 3. Build firmware
make
```

## Customize Your Firmware

Edit `src/main.cpp` to add your DSP processing:

```cpp
// Add DSP objects
Oscillator osc;
Adsr env;

// Initialize in main()
osc.Init(sample_rate);
osc.SetFreq(440.0f);

// Process in AudioCallback()
float osc_out = osc.Process();
float env_out = env.Process();
out[0][i] = osc_out * env_out;
```

## Build and Test

```bash
# Build firmware
make

# Run unit tests (requires Catch2)
make test

# Flash to hardware (requires DFU mode)
make program
```

## Common Commands

```bash
make              # Build firmware
make clean        # Clean build artifacts
make rebuild      # Clean and rebuild
make test         # Run unit tests
make program      # Flash firmware
make daisy-update # Update DaisySP library
make help         # Show all targets
```

## Project Structure

- `src/main.cpp` - Main firmware code (edit this!)
- `inc/config.h` - Configuration constants
- `tests/` - Unit tests
- `Makefile` - Build configuration
- `.cursor/rules` - Coding standards

## Next Steps

1. Read the full [README.md](README.md) for detailed documentation
2. Check `.cursor/rules` for coding standards
3. Explore DaisySP documentation: https://github.com/electro-smith/DaisySP
4. Join the Daisy community: https://forum.electro-smith.com/

## Troubleshooting

**Build fails:**
- Ensure DaisySP submodule is initialized: `git submodule update --init --recursive`
- Build DaisySP first: `cd DaisySP && make`

**Tests fail:**
- Install Catch2: See README.md for installation instructions
- Set CATCH2_INC environment variable if Catch2 is in non-standard location

**Flash fails:**
- Ensure module is in DFU mode (hold BOOT button while powering on)
- Check USB connection
- Try: `dfu-util --list` to verify device is detected

