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
// DuoPulse v3: Algorithmic Pulse Field
// =============================================================================
// v3 is now the default implementation. v2 PatternSkeleton system has been removed.
// The codebase uses the weighted pulse field algorithm with BROKEN/DRIFT controls.

// =============================================================================
// Debug Modes for Hardware Testing
// =============================================================================
// Uncomment ONE of these to enable a specific debug mode:

// DEBUG_BASELINE_SIMPLE: 4-on-the-floor kick + backbeat snare
// Perfect for verifying clock, gates, and basic timing
// #define DEBUG_BASELINE_SIMPLE 1

// DEBUG_BASELINE_LIVELY: Full-featured defaults for expressive patterns
// Density=0.7, Broken=0.3, Drift=0.2, Tempo=120 BPM
// #define DEBUG_BASELINE_LIVELY 1

// DEBUG_FEATURE_LEVEL: Progressive feature enablement (0-5)
// 0 = Simple clock + 4otf, 1 = Add shimmer, 2 = Add density, etc.
// #define DEBUG_FEATURE_LEVEL 0

