#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

/**
 * Hat Burst: Pattern-Aware Fill Triggers (Task 31)
 *
 * Generates hat triggers during fill zones with:
 * - Density following ENERGY parameter (2-12 triggers)
 * - Regularity following SHAPE parameter:
 *   - Low SHAPE (0-30%): Even spacing
 *   - Medium SHAPE (30-70%): Euclidean with jitter
 *   - High SHAPE (70-100%): Random steps
 * - Collision detection to avoid overlapping triggers
 * - Velocity ducking near main pattern hits
 *
 * Reference: docs/specs/main.md section [hat-burst]
 */

// =============================================================================
// Constants
// =============================================================================

/// Maximum number of hat triggers in a burst
constexpr int kMaxHatBurstTriggers = 12;

/// Minimum number of hat triggers in a burst
constexpr int kMinHatBurstTriggers = 2;

/// Velocity ducking multiplier when near main pattern hit
constexpr float kVelocityDuckMultiplier = 0.30f;

/// Base velocity at minimum energy
constexpr float kBaseVelocityMin = 0.65f;

/// Velocity bonus at maximum energy
constexpr float kBaseVelocityBonus = 0.35f;

// =============================================================================
// HatBurst Struct
// =============================================================================

/**
 * Pre-allocated structure for hat burst trigger data.
 *
 * All triggers are stored in a fixed-size array to avoid heap allocation.
 * The 'count' field indicates how many triggers are actually in use.
 */
struct HatBurst
{
    /**
     * Single trigger within the burst
     */
    struct Trigger
    {
        uint8_t step;      ///< Step position within fill (0 to fillDuration-1)
        float velocity;    ///< Velocity (0.0-1.0)
    };

    Trigger triggers[kMaxHatBurstTriggers];  ///< Pre-allocated trigger storage
    uint8_t count;                            ///< Actual number of triggers (0-12)
    uint8_t fillStart;                        ///< Fill zone start step in pattern
    uint8_t fillDuration;                     ///< Fill zone length in steps
    uint8_t _pad;                             ///< Alignment padding

    /**
     * Initialize burst to empty state
     */
    void Init()
    {
        count = 0;
        fillStart = 0;
        fillDuration = 0;
        _pad = 0;
    }

    /**
     * Clear all triggers but keep fill zone info
     */
    void Clear()
    {
        count = 0;
    }
};

// =============================================================================
// Collision Detection
// =============================================================================

/**
 * Find the nearest empty step position.
 *
 * When a step is already used, this function finds the closest unoccupied
 * position by searching alternately left and right.
 *
 * @param step Target step to place near (0 to fillDuration-1)
 * @param fillDuration Total duration of fill zone in steps
 * @param usedSteps Bitmask of used steps (bit N set = step N is used)
 * @return Nearest empty step index, or -1 if no empty position exists
 *
 * Guarantees:
 * - Returns -1 only if all steps are used
 * - Bounded to fillDuration iterations (no infinite loops)
 */
int FindNearestEmpty(int step, int fillDuration, uint64_t usedSteps);

// =============================================================================
// Proximity Detection
// =============================================================================

/**
 * Check if a step is within proximity of a main pattern hit.
 *
 * Used for velocity ducking - when a hat trigger is close to a main
 * pattern hit, its velocity should be reduced to avoid masking.
 *
 * @param step Step position to check (within fill zone, 0 to fillDuration-1)
 * @param fillStart Start of fill zone in main pattern
 * @param mainPattern 64-bit pattern where bit N = hit on step N
 * @param proximityWindow Number of steps to check on each side (typically 1)
 * @param patternLength Length of pattern in steps (1-64, clamped to kMaxSteps)
 * @return true if a main pattern hit is within proximityWindow of step
 */
bool CheckProximity(int step, int fillStart, uint64_t mainPattern, int proximityWindow, int patternLength);

// =============================================================================
// Timing Distribution
// =============================================================================

/**
 * Compute a trigger position using Euclidean distribution with jitter.
 *
 * For medium SHAPE values, triggers are spaced using Euclidean rhythm
 * with seed-based jitter added for variation.
 *
 * @param triggerIndex Index of this trigger (0 to triggerCount-1)
 * @param triggerCount Total number of triggers
 * @param fillDuration Duration of fill zone in steps
 * @param shape SHAPE parameter (0.0-1.0)
 * @param seed Pattern seed for deterministic jitter
 * @return Step position within fill zone (0 to fillDuration-1)
 */
int EuclideanWithJitter(int triggerIndex, int triggerCount,
                        int fillDuration, float shape, uint32_t seed);

// =============================================================================
// Main Generation Function
// =============================================================================

/**
 * Generate a pattern-aware hat burst for a fill zone.
 *
 * Creates 2-12 hat triggers based on ENERGY and distributes them
 * according to SHAPE:
 *
 * ENERGY -> Trigger Count:
 *   count = 2 + floor(energy * 10)
 *   Ranges from 2 (energy=0) to 12 (energy=1)
 *
 * SHAPE -> Timing Distribution:
 *   [0.00 - 0.30): Even spacing (straight divisions)
 *   [0.30 - 0.70): Euclidean with jitter (structured variation)
 *   [0.70 - 1.00]: Random steps (chaotic distribution)
 *
 * Velocity:
 *   Base = 0.65 + 0.35 * energy (scales 0.65 to 1.0)
 *   Ducked to 30% when within 1 step of main pattern hit
 *
 * @param energy ENERGY parameter (0.0-1.0), controls density
 * @param shape SHAPE parameter (0.0-1.0), controls regularity
 * @param mainPattern 64-bit bitmask of main pattern hits
 * @param fillStart Start step of fill zone (0-63)
 * @param fillDuration Length of fill zone in steps (1-64)
 * @param patternLength Length of pattern in steps (1-64, clamped to kMaxSteps)
 * @param seed Pattern seed for deterministic randomness
 * @param burst Output HatBurst struct (will be initialized)
 *
 * Guarantees:
 * - burst.count is in range [2, 12]
 * - All trigger steps are in range [0, fillDuration-1]
 * - All velocities are in range [0.0, 1.0]
 * - Same inputs always produce identical outputs (deterministic)
 * - No heap allocations (RT audio safe)
 */
void GenerateHatBurst(float energy, float shape,
                      uint64_t mainPattern, int fillStart,
                      int fillDuration, int patternLength, uint32_t seed,
                      HatBurst& burst);

} // namespace daisysp_idm_grids
