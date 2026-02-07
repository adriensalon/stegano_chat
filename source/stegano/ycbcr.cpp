#include <cmath>
#include <stdexcept>

#include <stegano/ycbcr.hpp>

namespace {

static constexpr int _y_red_factor = 77; // 0.299 * 256
static constexpr int _y_green_factor = 150; // 0.587 * 256
static constexpr int _y_blue_factor = 29; // 0.114 * 256

[[nodiscard]] inline bool _ensure_range(const int value)
{
    if (value < 0) {
        return false;
    }
    if (value > 255) {
        return false;
    }
    return true;
}

}

void encode_y(
    const std::vector<std::uint8_t>& rgb,
    std::vector<std::uint8_t>& y)
{
    if (rgb.size() % 3 != 0) {
        throw std::runtime_error("rgb.size() must be a multiple of 3");
    }

    const std::size_t _pixel_count = rgb.size() / 3;
    y.resize(_pixel_count);
    for (std::size_t _pixel_index = 0; _pixel_index < _pixel_count; ++_pixel_index) {
        const std::uint8_t r = rgb[_pixel_index * 3 + 0];
        const std::uint8_t g = rgb[_pixel_index * 3 + 1];
        const std::uint8_t b = rgb[_pixel_index * 3 + 2];
        const int _y_value = (_y_red_factor * r + _y_green_factor * g + _y_blue_factor * b) >> 8;
        y[_pixel_index] = static_cast<std::uint8_t>(_y_value);
    }
}

bool decode_y(
    const std::vector<std::uint8_t>& rgb,
    const std::vector<std::uint8_t>& y,
    std::vector<std::uint8_t>& rgb_embedded)
{
    if (rgb.size() % 3 != 0) {
        throw std::runtime_error("rgb.size() must be a multiple of 3");
    }
    if (rgb.size() != 3 * y.size()) {
        throw std::runtime_error("rgb.size() must be equal to 3 * y.size()");
    }

    const std::size_t _pixels_count = rgb.size() / 3;
    rgb_embedded.resize(rgb.size());
    for (std::size_t i = 0; i < _pixels_count; ++i) {
        const int _original_red = rgb[3 * i + 0];
        const int _original_green = rgb[3 * i + 1];
        const int _original_blue = rgb[3 * i + 2];
        const int _original_y = (_y_red_factor * _original_red + _y_green_factor * _original_green + _y_blue_factor * _original_blue) >> 8;
        const int _delta_y = y[i] - _original_y;
        const int _new_red = _original_red + _delta_y;
        const int _new_green = _original_green + _delta_y;
        const int _new_blue = _original_blue + _delta_y;
        if (!_ensure_range(_new_red)) {
            return false;
        }
        if (!_ensure_range(_new_green)) {
            return false;
        }
        if (!_ensure_range(_new_blue)) {
            return false;
        }
        rgb_embedded[3 * i + 0] = static_cast<std::uint8_t>(_new_red);
        rgb_embedded[3 * i + 1] = static_cast<std::uint8_t>(_new_green);
        rgb_embedded[3 * i + 2] = static_cast<std::uint8_t>(_new_blue);
    }

    return true;
}
