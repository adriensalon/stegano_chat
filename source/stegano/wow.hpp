#pragma once

#include <array>
#include <cstdint>
#include <vector>

bool embed_wow(
    const std::vector<std::uint8_t>& rgb,
    const std::size_t width,
    const std::size_t height,
    const std::array<std::uint8_t, 32> stegano_key,
    const std::uint32_t constraint_height,
    const std::vector<std::uint8_t>& payload_bits,
    std::vector<std::uint8_t>& rgb_embedded,
    double& cost_embedded);

void extract_wow(
    const std::vector<std::uint8_t>& rgb_stegano,
    const std::size_t width,
    const std::size_t height,
    const std::array<std::uint8_t, 32> stegano_key,
    const std::uint32_t constraint_height,
    const std::size_t payload_bit_count,
    std::vector<std::uint8_t>& payload_bits);
