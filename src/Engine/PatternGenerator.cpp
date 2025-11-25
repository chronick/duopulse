#include "PatternGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "GridsData.h"

namespace daisysp_idm_grids
{

namespace
{
constexpr float kCoordScale = 255.0f;
constexpr float kCellSize = 64.0f;

template <typename T>
inline T Clamp(T value, T minValue, T maxValue)
{
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

int WrapStepIndex(int step)
{
    int wrapped = step % PatternGenerator::kPatternLength;
    if(wrapped < 0)
    {
        wrapped += PatternGenerator::kPatternLength;
    }
    return wrapped;
}
} // namespace

void PatternGenerator::Init()
{
}

void PatternGenerator::GetTriggers(float style, int step, float lowDensity, float highDensity, bool* triggers)
{
    // Opinionated mapping: Style traverses the X axis of the map, Y is fixed at center.
    // This hits a good variety of patterns (breakbeat at center, etc.)
    // TODO: Consider a more complex path (e.g. diagonal) if more variety is needed.
    float x = style;
    float y = 0.5f;

    x = Clamp(x, 0.0f, 1.0f);
    y = Clamp(y, 0.0f, 1.0f);
    lowDensity = Clamp(lowDensity, 0.0f, 1.0f);
    highDensity = Clamp(highDensity, 0.0f, 1.0f);
    int wrappedStep = WrapStepIndex(step);

    // Calculate thresholds
    int lowThreshold = static_cast<int>(std::round((1.0f - lowDensity) * 255.0f));
    lowThreshold = Clamp(lowThreshold, 0, 255);
    
    int highThreshold = static_cast<int>(std::round((1.0f - highDensity) * 255.0f));
    highThreshold = Clamp(highThreshold, 0, 255);

    // Kick (Channel 0) -> Low Density
    uint8_t kickVal = ReadMap(x, y, 0, wrappedStep);
    triggers[0] = kickVal >= lowThreshold;

    // Snare (Channel 1) -> High Density
    uint8_t snareVal = ReadMap(x, y, 1, wrappedStep);
    triggers[1] = snareVal >= highThreshold;

    // HH (Channel 2) -> High Density
    uint8_t hhVal = ReadMap(x, y, 2, wrappedStep);
    triggers[2] = hhVal >= highThreshold;
}

void PatternGenerator::GetTriggers(float x, float y, int step, float density, bool* triggers)
{
    x = Clamp(x, 0.0f, 1.0f);
    y = Clamp(y, 0.0f, 1.0f);
    density = Clamp(density, 0.0f, 1.0f);
    int wrappedStep = WrapStepIndex(step);

    int threshold = static_cast<int>(std::round((1.0f - density) * 255.0f));
    threshold = Clamp(threshold, 0, 255);

    for(int ch = 0; ch < kNumChannels; ++ch)
    {
        uint8_t value = ReadMap(x, y, ch, wrappedStep);
        triggers[ch] = value >= threshold;
    }
}

uint8_t PatternGenerator::GetLevel(float x, float y, int channel, int step) const
{
    return ReadMap(x, y, channel, step);
}

uint8_t PatternGenerator::ReadMap(float x, float y, int channel, int step) const
{
    x = Clamp(x, 0.0f, 1.0f);
    y = Clamp(y, 0.0f, 1.0f);
    channel = Clamp(channel, 0, kNumChannels - 1);
    int wrappedStep = WrapStepIndex(step);

    const float xAddress = x * kCoordScale;
    const float yAddress = y * kCoordScale;

    const int maxNodeIndex = static_cast<int>(grids_data::kDrumMapSide) - 1;
    const int maxCellIndex = maxNodeIndex - 1;

    int cellX = Clamp(static_cast<int>(xAddress) >> 6, 0, maxCellIndex);
    int cellY = Clamp(static_cast<int>(yAddress) >> 6, 0, maxCellIndex);
    int cellX1 = std::min(cellX + 1, maxNodeIndex);
    int cellY1 = std::min(cellY + 1, maxNodeIndex);

    float tx = (xAddress - static_cast<float>(cellX << 6)) / kCellSize;
    float ty = (yAddress - static_cast<float>(cellY << 6)) / kCellSize;
    tx = Clamp(tx, 0.0f, 1.0f);
    ty = Clamp(ty, 0.0f, 1.0f);

    auto sample = [&](int mapX, int mapY) -> float {
        mapX = Clamp(mapX, 0, maxNodeIndex);
        mapY = Clamp(mapY, 0, maxNodeIndex);
        const uint8_t nodeIndex = grids_data::kDrumMap[mapX][mapY];
        const auto& node = grids_data::kNodeData[nodeIndex];
        std::size_t idx = static_cast<std::size_t>(channel) * kPatternLength + wrappedStep;
        return static_cast<float>(node[idx]);
    };

    float a = sample(cellX, cellY);
    float b = sample(cellX1, cellY);
    float c = sample(cellX, cellY1);
    float d = sample(cellX1, cellY1);

    float top = a + (b - a) * tx;
    float bottom = c + (d - c) * tx;
    float value = top + (bottom - top) * ty;
    value = Clamp(value, 0.0f, 255.0f);

    return static_cast<uint8_t>(std::round(value));
}

} // namespace daisysp_idm_grids

