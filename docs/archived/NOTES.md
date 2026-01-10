# Development Notes

## ARM Toolchain Version Compatibility

**Important**: The ARM GCC toolchain version 15.2.0 (installed via Homebrew) may have compatibility issues with libDaisy. If you encounter build errors related to `stdint.h` or other standard library headers, you may need to use an older version of the ARM toolchain.

### Recommended ARM Toolchain Version

For best compatibility with libDaisy, use ARM GCC version 10.3 or earlier. Version 10.3.1 is known to work well.

### Installing Older ARM Toolchain

**Option 1: Use ARM's official toolchain**
1. Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
2. Extract to a directory (e.g., `~/Developer/gcc-arm-none-eabi-10-2020-q4-major/`)
3. Add to PATH:
   ```bash
   export GCC_PATH=~/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin
   export PATH=$GCC_PATH:$PATH
   ```
4. Use in Makefile by setting `GCC_PATH` variable

**Option 2: Use Homebrew to install specific version**
```bash
brew install arm-none-eabi-gcc@10
```

### Workaround for GCC 15.2.0

If you must use GCC 15.2.0, you may need to:
1. Create symlinks for missing headers
2. Use libDaisy's build system directly
3. Consider using Docker or a VM with an older toolchain

## libDaisy Build Issues

If libDaisy fails to build:
1. Ensure all submodules are initialized: `cd libDaisy && git submodule update --init --recursive`
2. Check that the ARM toolchain version is compatible
3. Try building libDaisy separately: `cd libDaisy && make clean && make`

## Project Structure

- **DaisySP**: DSP library (already built)
- **libDaisy**: Hardware abstraction library (needs to be built)
- Both are included as git submodules

## Building Order

1. Build DaisySP: `make daisy-build` or `cd DaisySP && make`
2. Build libDaisy: `make libdaisy-build` or `cd libDaisy && make`
3. Build firmware: `make`

