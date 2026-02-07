#pragma once

#include <cstdint>
#include <vector>

double encode_stc(
    const std::vector<std::uint8_t>& cover_symbols,
    const std::vector<std::uint8_t>& syndrome_bits,
    const std::vector<std::uint8_t>& pricevector,
    const std::uint32_t constraint_height,
    std::vector<std::uint8_t>& stego_symbols);

void decode_stc(
    const std::vector<std::uint8_t>& stego_symbols,
    const std::uint32_t constraint_height,
    const std::size_t payload_bit_count,
    std::vector<std::uint8_t>& syndrome_bits_out);
