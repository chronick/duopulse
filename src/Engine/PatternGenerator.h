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
     * @brief Get the trigger states based on opinionated Style and Density controls.
     * 
     * @param style Style parameter (0.0 - 1.0) maps to coordinate path
     * @param step Current step index
     * @param lowDensity Density for Low/Kick channel (0.0 - 1.0)
     * @param highDensity Density for High/Snare/HH channels (0.0 - 1.0)
     * @param triggers Output array [Kick, Snare, HH]
     */
    void GetTriggers(float style, int step, float lowDensity, float highDensity, bool* triggers);

    /**
     * @brief (Legacy/Low-Level) Get the trigger states for the current step and coordinates.
     *
     * @param x X coordinate (0.0 - 1.0)
     * @param y Y coordinate (0.0 - 1.0)
     * @param step Current step index (0-31)
     * @param density Global density/fill (0.0 - 1.0)
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

