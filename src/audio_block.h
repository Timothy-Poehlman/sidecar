#pragma once

#include <array>
#include <cstddef>

constexpr std::size_t FRAMES_PER_BLOCK = 512;

struct AudioBlock
{
    std::array<float, FRAMES_PER_BLOCK> samples{};
    std::size_t frameCount{};
};