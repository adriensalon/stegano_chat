#pragma once

#include <cstdint>
#include <vector>

void encode_y(
    const std::vector<std::uint8_t>& rgb,
    std::vector<std::uint8_t>& y);

bool decode_y(
    const std::vector<std::uint8_t>& rgb,
    const std::vector<std::uint8_t>& y,
    std::vector<std::uint8_t>& rgb_embedded);
