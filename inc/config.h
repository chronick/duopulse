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

