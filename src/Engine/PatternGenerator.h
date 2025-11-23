#pragma once

#include <cstdint>

namespace daisysp_idm_grids
{

class PatternGenerator
{
public:
    PatternGenerator() {}
    ~PatternGenerator() {}

    void Init();

    /**
     * @brief Get the trigger states for the current step and coordinates.
     *
     * @param x X coordinate (0.0 - 1.0)
     * @param y Y coordinate (0.0 - 1.0)
     * @param step Current step index (0-31)
     * @param density Global density/fill (0.0 - 1.0) - Optional, maps to randomness/chaos or just fill
     * @param triggers Output array for trigger states [Kick, Snare, HH]
     */
    void GetTriggers(float x, float y, int step, float density, bool* triggers);

    /**
     * @brief Returns the raw map value (0-255) for a channel/step at the given coordinates.
     */
    uint8_t GetLevel(float x, float y, int channel, int step) const;

    static constexpr int kNumChannels = 3;
    static constexpr int kPatternLength = 32;

private:
    uint8_t ReadMap(float x, float y, int channel, int step) const;
};

} // namespace daisysp_idm_grids

