---
id: 52
slug: release-builds
title: "CI Release Builds on Version Tags"
status: pending
created_date: 2026-01-13
updated_date: 2026-01-13
branch: null
spec_refs: []
---

# Task 52: CI Release Builds on Version Tags

## Objective

Create a GitHub Actions workflow that builds firmware binaries and creates GitHub releases when version tags (e.g., `v5.0.0`) are pushed.

## Background

### Current CI State

The project currently has two workflows:
- `test.yml`: Runs unit tests on push/PR (uses native `g++`)
- `pages.yml`: Builds pattern-viz and deploys GitHub Pages site

**Important Note (2026-01-13):** PR #2 removed `gcc-arm-none-eabi` from `pages.yml` because:
- `pattern-viz` uses `HOST_CXX := g++` (native compiler), not ARM cross-compiler
- Tests also use native `g++`
- The ARM toolchain was unnecessary for those builds

However, **release builds WILL require the ARM toolchain** since firmware compilation targets the STM32H7 (Cortex-M7).

### ARM Toolchain Options

1. **apt-get install**: `sudo apt-get install gcc-arm-none-eabi` (~500MB, slow)
2. **GitHub Action**: `carlosperate/arm-none-eabi-gcc-action@v1` (used by libDaisy)
3. **Pre-built Docker image**: Custom GCR image with toolchain pre-installed (fastest, requires maintenance)

## Subtasks

### Workflow Implementation
- [ ] Create `.github/workflows/release.yml` triggered on `v*` tags
- [ ] Install ARM toolchain (evaluate options above)
- [ ] Checkout with submodules (`recursive`)
- [ ] Build firmware: `make all`
- [ ] Create GitHub Release with:
  - Binary files: `.bin`, `.hex`, `.elf`
  - Auto-generated changelog from commits
  - Version from tag

### Build Artifacts
- [ ] Upload `build/patch-init-firmware.bin` as release asset
- [ ] Upload `build/patch-init-firmware.hex` as release asset
- [ ] Consider adding build info (git sha, timestamp) to binary

### Documentation
- [ ] Document release process in README
- [ ] Add versioning strategy notes

## Acceptance Criteria

- [ ] Pushing `v*` tag triggers release workflow
- [ ] Firmware builds successfully in CI
- [ ] GitHub Release created with downloadable binaries
- [ ] Release notes auto-generated from commits

## Implementation Notes

### Recommended Workflow Structure

```yaml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Setup ARM toolchain
        uses: carlosperate/arm-none-eabi-gcc-action@v1
        # OR: apt-get install gcc-arm-none-eabi
        # OR: use pre-built Docker image

      - name: Build firmware
        run: make all

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            build/*.bin
            build/*.hex
          generate_release_notes: true
```

### Future Optimization

If CI time becomes a concern, consider:
- Publishing a Docker image to GCR with ARM toolchain pre-installed
- Using GitHub Actions cache for toolchain
- Matrix builds for multiple firmware variants
