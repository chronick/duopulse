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

void PatternGenerator::GetTriggers(float x, float y, int step, float density, bool* triggers)
{
    x = std::clamp(x, 0.0f, 1.0f);
    y = std::clamp(y, 0.0f, 1.0f);
    density = std::clamp(density, 0.0f, 1.0f);
    int wrappedStep = WrapStepIndex(step);

    int threshold = static_cast<int>(std::round((1.0f - density) * 255.0f));
    threshold = std::clamp(threshold, 0, 255);

    for(int ch = 0; ch < kNumChannels; ++ch)
    {
        uint8_t value = ReadMap(x, y, ch, wrappedStep);
        triggers[ch] = value >= threshold;
    }
}

uint8_t PatternGenerator::ReadMap(float x, float y, int channel, int step) const
{
    x = std::clamp(x, 0.0f, 1.0f);
    y = std::clamp(y, 0.0f, 1.0f);
    channel = std::clamp(channel, 0, kNumChannels - 1);
    int wrappedStep = WrapStepIndex(step);

    const float xAddress = x * kCoordScale;
    const float yAddress = y * kCoordScale;

    const int maxNodeIndex = static_cast<int>(grids_data::kDrumMapSide) - 1;
    const int maxCellIndex = maxNodeIndex - 1;

    int cellX = std::clamp(static_cast<int>(xAddress) >> 6, 0, maxCellIndex);
    int cellY = std::clamp(static_cast<int>(yAddress) >> 6, 0, maxCellIndex);
    int cellX1 = std::min(cellX + 1, maxNodeIndex);
    int cellY1 = std::min(cellY + 1, maxNodeIndex);

    float tx = (xAddress - static_cast<float>(cellX << 6)) / kCellSize;
    float ty = (yAddress - static_cast<float>(cellY << 6)) / kCellSize;
    tx = std::clamp(tx, 0.0f, 1.0f);
    ty = std::clamp(ty, 0.0f, 1.0f);

    auto sample = [&](int mapX, int mapY) -> float {
        mapX = std::clamp(mapX, 0, maxNodeIndex);
        mapY = std::clamp(mapY, 0, maxNodeIndex);
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
    value = std::clamp(value, 0.0f, 255.0f);

    return static_cast<uint8_t>(std::round(value));
}

} // namespace daisysp_idm_grids

