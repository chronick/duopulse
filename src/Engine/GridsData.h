#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace daisysp_idm_grids::grids_data
{

constexpr std::size_t kNumParts = 3;
constexpr std::size_t kStepsPerPattern = 32;
constexpr std::size_t kNodeSize = kNumParts * kStepsPerPattern;
constexpr std::size_t kNumNodes = 25;
constexpr std::size_t kDrumMapSide = 5;

using NodeArray = std::array<uint8_t, kNodeSize>;

extern const std::array<NodeArray, kNumNodes> kNodeData;
extern const uint8_t kDrumMap[kDrumMapSide][kDrumMapSide];

} // namespace daisysp_idm_grids::grids_data


