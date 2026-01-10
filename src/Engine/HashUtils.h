#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * Hash Utilities for Deterministic Pattern Generation (Task 28)
 *
 * Provides simple, fast hash functions for generating deterministic
 * pseudo-random values from seed + step combinations. These are designed
 * for real-time audio use (no allocations, O(1) operations).
 *
 * Reference: docs/specs/main.md section 4.4 (SHAPE parameter)
 */

// =============================================================================
// Hash Functions
// =============================================================================

/**
 * Convert a seed + step combination to a deterministic float in [0.0, 1.0]
 *
 * Uses a multiplicative hash with the golden ratio constant for good
 * distribution. Same seed + step always produces the same output.
 *
 * @param seed Base seed value (e.g., from pattern seed)
 * @param step Step index within pattern
 * @return Deterministic float in range [0.0, 1.0]
 *
 * Properties:
 * - Deterministic: HashToFloat(S, i) always returns the same value
 * - Well-distributed: Passes basic randomness tests for our use case
 * - Fast: Only integer arithmetic and bit operations
 */
inline float HashToFloat(uint32_t seed, int step)
{
    // Golden ratio fractional part as 32-bit integer (2^32 / phi)
    constexpr uint32_t kGoldenRatio = 0x9E3779B9;

    // Combine seed with step using XOR and multiply
    uint32_t hash = seed ^ (static_cast<uint32_t>(step) * kGoldenRatio);

    // Mix bits using xorshift-style operations
    hash ^= hash >> 16;
    hash *= 0x85EBCA6B;
    hash ^= hash >> 13;

    // Convert low 16 bits to float in [0.0, 1.0]
    return static_cast<float>(hash & 0xFFFF) / 65535.0f;
}

/**
 * Generate a deterministic integer from seed + offset
 *
 * Useful for selecting discrete choices (e.g., which step to modify)
 *
 * @param seed Base seed value
 * @param offset Additional offset for variation
 * @return Deterministic 32-bit integer
 */
inline uint32_t HashToInt(uint32_t seed, int offset)
{
    constexpr uint32_t kGoldenRatio = 0x9E3779B9;

    uint32_t hash = seed ^ (static_cast<uint32_t>(offset) * kGoldenRatio);
    hash ^= hash >> 16;
    hash *= 0x85EBCA6B;
    hash ^= hash >> 13;

    return hash;
}

} // namespace daisysp_idm_grids
