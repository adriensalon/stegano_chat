#pragma once

#include <cstdint>
#include <vector>

void encode_lsb(
    const std::vector<std::uint8_t>& y,
    std::vector<std::uint8_t>& y_lsb);

void decode_lsb(
    const std::vector<std::uint8_t>& y,
    const std::vector<std::uint8_t>& y_lsb,
    std::vector<std::uint8_t>& y_embedded);
