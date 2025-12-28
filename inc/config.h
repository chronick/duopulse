/**
 * Configuration header for Patch.Init firmware
 * 
 * Define hardware-specific constants and configuration here.
 */

#pragma once

// Audio Configuration
#define SAMPLE_RATE 48000.0f
#define BLOCK_SIZE 4

// CV Configuration
#define CV_INPUT_COUNT 2
#define CV_OUTPUT_COUNT 2
#define CV_VOLTAGE_RANGE 5.0f  // ±5V
#define CV_NORMALIZED_MAX 1.0f
#define CV_NORMALIZED_MIN -1.0f

// Audio Configuration
#define AUDIO_INPUT_COUNT 2
#define AUDIO_OUTPUT_COUNT 2
#define AUDIO_MAX_LEVEL 1.0f   // Normalized ±1.0
#define AUDIO_MIN_LEVEL -1.0f

// GPIO Pin Assignments (adjust based on your hardware)
// These are examples - refer to Patch.Init documentation for actual pins
#define GPIO_BUTTON_1 0
#define GPIO_BUTTON_2 1
#define GPIO_LED_1 2
#define GPIO_LED_2 3

// Frequency Ranges
#define FREQ_MIN 20.0f      // Minimum frequency (Hz)
#define FREQ_MAX 20000.0f   // Maximum frequency (Hz)

// Utility Macros
#define CV_TO_NORMALIZED(cv) ((cv) / CV_VOLTAGE_RANGE)
#define NORMALIZED_TO_CV(norm) ((norm) * CV_VOLTAGE_RANGE)
#define FREQ_TO_RAD(freq, sr) ((freq) * 2.0f * 3.14159265359f / (sr))

// =============================================================================
// DuoPulse v4: Full Architecture Overhaul
// =============================================================================
// v4 is a complete rewrite. v3 flag is disabled during migration.
// The v3 code is being removed and replaced with v4 architecture.
//
// v4 key changes:
//   - 2D Pattern Field with 3×3 archetype grid per genre
//   - Hit budget system with Gumbel Top-K sampling
//   - ENERGY/PUNCH, BUILD/GENRE, FIELD X/DRIFT, FIELD Y/BALANCE controls
//   - FLAVOR parameter (from Audio In R) controls BROKEN timing stack
//
// #define USE_PULSE_FIELD_V3 1  // DISABLED: v3 code removed for v4 migration

// =============================================================================
// Debug Configuration Flags for Hardware Validation
// =============================================================================
// See docs/tasks/active/16-v4-hardware-validation.md for full testing guide
//
// QUICK START:
//   1. Set DEBUG_FEATURE_LEVEL to the level you want to test
//   2. Build and flash: make clean && make && make program-dfu
//   3. Test at that level, then increment and repeat
// =============================================================================

// -----------------------------------------------------------------------------
// DEBUG_FEATURE_LEVEL: Progressive Feature Enablement (0-5)
// -----------------------------------------------------------------------------
// Each level adds features on top of previous levels.
// Test from Level 0 upward - don't skip levels!
//
// Level 0: CLOCK TEST
//   - Simple 4-on-floor pattern (Anchor on 1,2,3,4; Shimmer on 2,4)
//   - Knobs are IGNORED - pattern is fixed
//   - Tests: Audio callback runs, gates fire, LED blinks, outputs work
//
// Level 1: ARCHETYPE TEST
//   - Pattern responds to FIELD X/Y knobs (K3, K4)
//   - No hit budget sampling - weights converted directly to hits
//   - Tests: Knob reading works, archetype blending works
//
// Level 2: SAMPLING TEST
//   - Hit budget with Gumbel Top-K sampling enabled
//   - ENERGY knob (K1) now controls hit density
//   - Tests: Hit budget scales correctly, sampling produces varied patterns
//
// Level 3: GUARD RAILS TEST
//   - Voice relationships and guard rails enabled
//   - Beat 1 always has anchor, no excessive gaps
//   - Tests: Musical rules enforced, coupling works
//
// Level 4: TIMING TEST
//   - Swing and microtiming jitter based on GENRE + SWING config
//   - Tests: Swing audible, genre timing profiles working, jitter humanizes feel
//   - Note: Audio In R (FLAVOR CV) removed - timing is GENRE/config-based only
//
// Level 5: PRODUCTION MODE (default when flag removed)
//   - All features enabled, ready for performance
//   - Same as removing the DEBUG_FEATURE_LEVEL definition
//
#define DEBUG_FEATURE_LEVEL 3  // Level 3: guard rails (Level 2 verified)

// -----------------------------------------------------------------------------
// Other Debug Flags (Optional - use alongside DEBUG_FEATURE_LEVEL)
// -----------------------------------------------------------------------------

// DEBUG_BASELINE_MODE: Forces known-good control values on startup
// Bypasses knob readings and sets predictable "musical" defaults:
//   - ENERGY=0.75, FIELD X/Y=0.5 (center of grid = Groovy archetype)
// Recommended: Use with Level 2+ to test generation with known inputs
// #define DEBUG_BASELINE_MODE 1

// DEBUG_RESET_CONFIG: Ignores saved flash config and uses defaults
// Forces fresh config on every boot - useful when testing new features
// or when flash config is corrupted/incompatible with code changes
// Recommended: Enable when adding new config parameters or debugging clock issues
#define DEBUG_RESET_CONFIG 1

// DEBUG_FIXED_SEED: Uses constant seed for reproducible patterns
// Helps compare patterns across builds - same seed = same pattern
// Recommended: Use with Level 2+ when debugging sampling logic
// #define DEBUG_FIXED_SEED 1

// DEBUG_LOG_MASKS: Log pattern masks to velocity output for scope debugging
// Outputs anchor/shimmer masks as stepped voltages on Audio Out L/R
// Recommended: Use with Level 1+ to visualize patterns on oscilloscope
// #define DEBUG_LOG_MASKS 1

