#include <cmath>
#include <stdexcept>

#include <stegano/lsb.hpp>

void encode_lsb(
    const std::vector<std::uint8_t>& y,
    std::vector<std::uint8_t>& y_lsb)
{
    const std::size_t _pixels_count = y.size();
    y_lsb.resize(_pixels_count);
    for (std::size_t _pixel_index = 0; _pixel_index < _pixels_count; ++_pixel_index) {
        y_lsb[_pixel_index] = y[_pixel_index] & 1u;
    }
}

void decode_lsb(
    const std::vector<std::uint8_t>& y,
    const std::vector<std::uint8_t>& y_lsb,
    std::vector<std::uint8_t>& y_embedded)
{
    if (y.size() != y_lsb.size()) {
        throw std::runtime_error("y.size() must be equal to y_lsb.size()");
    }

    const std::size_t _pixels_count = y_lsb.size();
    y_embedded.resize(_pixels_count);
    for (std::size_t _pixel_index = 0; _pixel_index < _pixels_count; ++_pixel_index) {
        y_embedded[_pixel_index] = (y[_pixel_index] & ~1u) | (y_lsb[_pixel_index] & 1u);
    }
}
