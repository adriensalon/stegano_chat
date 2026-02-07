#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include <stegano/lsb.hpp>
#include <stegano/stc.hpp>
#include <stegano/wow.hpp>
#include <stegano/ycbcr.hpp>

namespace {

static constexpr std::size_t _payload_length_bit_count = 32;

static constexpr std::int32_t _daubechies_length = 16;

static const std::array<float, _daubechies_length> _daubechies_lowpass = {
    -0.00011747678400228192f,
    0.00067544940599855677f,
    -0.00039174037299597711f,
    -0.0048703529930106603f,
    0.0087460940470156547f,
    0.013981027917015516f,
    -0.044088253931064719f,
    -0.017369301002022108f,
    0.12874742662018601f,
    0.00047248457399797254f,
    -0.28401554296242809f,
    -0.015829105256023893f,
    0.58535468365486909f,
    0.67563073629801285f,
    0.31287159091446592f,
    0.054415842243081609f
};

static const std::array<float, _daubechies_length> _daubechies_highpass = {
    -0.054415842243081609f,
    0.31287159091446592f,
    -0.67563073629801285f,
    0.58535468365486909f,
    0.015829105256023893f,
    -0.28401554296242809f,
    -0.00047248457399797254f,
    0.12874742662018601f,
    0.017369301002022108f,
    -0.044088253931064719f,
    -0.013981027917015516f,
    0.0087460940470156547f,
    0.0048703529930106603f,
    -0.00039174037299597711f,
    -0.00067544940599855677f,
    -0.00011747678400228192f
};

struct _steganography_rng {
    std::uint64_t state;

    _steganography_rng(const std::array<std::uint8_t, 32>& key)
    {
        state = 0x9e3779b97f4a7c15ULL;
        for (const std::uint8_t _char : key) {
            state ^= static_cast<std::uint64_t>(_char) + 0x9e3779b97f4a7c15ULL + (state << 6) + (state >> 2);
        }
        if (state == 0) {
            state = 0xdeadbeefcafebabeULL;
        }
    }

    [[nodiscard]] std::size_t next(const std::size_t bound)
    {
        std::uint64_t _x = state;
        _x ^= _x >> 12;
        _x ^= _x << 25;
        _x ^= _x >> 27;
        state = _x;
        const std::uint64_t _result = _x * 2685821657736338717ULL;
        return static_cast<std::size_t>(_result % bound);
    }
};

void _abs_inplace(std::vector<float>& values)
{
    for (float& value : values) {
        value = std::fabs(value);
    }
}

[[nodiscard]] inline std::uint8_t _clamp_u8(float value)
{
    if (value < 0.f) {
        value = 0.f;
    }
    if (value > 255.f) {
        value = 255.f;
    }
    return static_cast<std::uint8_t>(std::lround(value));
}

[[nodiscard]] static std::int32_t _mirror_index(std::int32_t index, const std::int32_t limit)
{
    if (limit <= 1) {
        return 0;
    }
    while (index < 0 || index >= limit) {
        if (index < 0) {
            index = -index - 1;
        } else if (index >= limit) {
            index = 2 * limit - index - 1;
        }
    }
    return index;
}

[[nodiscard]] static float _mirror_pixel(
    const std::vector<std::uint8_t>& input,
    const std::size_t width,
    const std::size_t height,
    const std::int32_t x,
    const std::int32_t y)
{
    const std::int32_t _mirrored_x = _mirror_index(x, static_cast<std::int32_t>(width));
    const std::int32_t _mirrored_y = _mirror_index(y, static_cast<std::int32_t>(height));
    return static_cast<float>(input[static_cast<std::size_t>(_mirrored_y) * width + static_cast<std::size_t>(_mirrored_x)]);
}

[[nodiscard]] static float _mirror_pixel(
    const std::vector<float>& input,
    const std::size_t width,
    const std::size_t height,
    const std::int32_t x,
    const std::int32_t y)
{
    const std::int32_t _mirrored_x = _mirror_index(x, static_cast<std::int32_t>(width));
    const std::int32_t _mirrored_y = _mirror_index(y, static_cast<std::int32_t>(height));
    return input[static_cast<std::size_t>(_mirrored_y) * width + static_cast<std::size_t>(_mirrored_x)];
}

static void _separable_convolution(
    const std::vector<std::uint8_t>& input,
    const std::size_t width,
    const std::size_t height,
    const float* vertical_filter,
    const float* horizontal_filter,
    const std::int32_t filters_length,
    std::vector<float>& output)
{
    const std::int32_t _half_filters_length = filters_length / 2;
    std::vector<float> _temp(width * height);
    for (std::size_t _y_index = 0; _y_index < height; ++_y_index) {
        for (std::size_t _x_index = 0; _x_index < width; ++_x_index) {
            float _accumulate = 0.f;
            for (std::int32_t _filter_index = 0; _filter_index < filters_length; ++_filter_index) {
                const std::int32_t _yy = static_cast<std::int32_t>(_y_index) + _filter_index - _half_filters_length;
                _accumulate += vertical_filter[_filter_index] * _mirror_pixel(input, width, height, static_cast<std::int32_t>(_x_index), _yy);
            }
            _temp[_y_index * width + _x_index] = _accumulate;
        }
    }

    output.resize(width * height);
    for (std::size_t _y_index = 0; _y_index < height; ++_y_index) {
        for (std::size_t _x_index = 0; _x_index < width; ++_x_index) {
            float _accumulate = 0.f;
            for (std::int32_t _filter_index = 0; _filter_index < filters_length; ++_filter_index) {
                const std::int32_t _xx = static_cast<std::int32_t>(_x_index) + _filter_index - _half_filters_length;
                const std::int32_t _mx = _mirror_index(_xx, static_cast<std::int32_t>(width));
                _accumulate += horizontal_filter[_filter_index] * _temp[_y_index * width + static_cast<std::size_t>(_mx)];
            }
            output[_y_index * width + _x_index] = _accumulate;
        }
    }
}

static void _separable_convolution(
    const std::vector<float>& input,
    const std::size_t width,
    const std::size_t height,
    const float* vertical_filter,
    const float* horizontal_filter,
    const std::int32_t filters_length,
    std::vector<float>& output)
{
    const std::int32_t _half_filters_length = filters_length / 2;
    std::vector<float> _temp(width * height);
    for (std::size_t _y_index = 0; _y_index < height; ++_y_index) {
        for (std::size_t _x_index = 0; _x_index < width; ++_x_index) {
            float _accumulate = 0.f;
            for (std::int32_t _filter_index = 0; _filter_index < filters_length; ++_filter_index) {
                const std::int32_t _yy = static_cast<std::int32_t>(_y_index) + _filter_index - _half_filters_length;
                _accumulate += vertical_filter[_filter_index] * _mirror_pixel(input, width, height, static_cast<std::int32_t>(_x_index), _yy);
            }
            _temp[_y_index * width + _x_index] = _accumulate;
        }
    }

    output.resize(width * height);
    for (std::size_t _y_index = 0; _y_index < height; ++_y_index) {
        for (std::size_t _x_index = 0; _x_index < width; ++_x_index) {
            float _accumulate = 0.f;
            for (std::int32_t _filter_index = 0; _filter_index < filters_length; ++_filter_index) {
                const std::int32_t _xx = static_cast<std::int32_t>(_x_index) + _filter_index - _half_filters_length;
                const std::int32_t _mx = _mirror_index(_xx, static_cast<std::int32_t>(width));
                _accumulate += horizontal_filter[_filter_index] * _temp[_y_index * width + static_cast<std::size_t>(_mx)];
            }
            output[_y_index * width + _x_index] = _accumulate;
        }
    }
}

static void _compute_wow_rho(
    const std::vector<std::uint8_t>& input,
    std::size_t width,
    std::size_t height,
    std::vector<float>& rho)
{
    const std::size_t _rho_count = width * height;
    rho.resize(_rho_count);

    std::vector<float> _rho_lh, _rho_hl, _rho_hh;
    _separable_convolution(input, width, height, _daubechies_lowpass.data(), _daubechies_highpass.data(), _daubechies_length, _rho_lh);
    _separable_convolution(input, width, height, _daubechies_highpass.data(), _daubechies_lowpass.data(), _daubechies_length, _rho_hl);
    _separable_convolution(input, width, height, _daubechies_highpass.data(), _daubechies_highpass.data(), _daubechies_length, _rho_hh);
    _abs_inplace(_rho_lh);
    _abs_inplace(_rho_hl);
    _abs_inplace(_rho_hh);

    std::vector<float> _xi_lh, _xi_hl, _xi_hh;
    float _abs_l[_daubechies_length], _abs_h[_daubechies_length];
    for (std::int32_t _filter_index = 0; _filter_index < _daubechies_length; ++_filter_index) {
        _abs_l[_filter_index] = std::fabs(_daubechies_lowpass[_filter_index]);
        _abs_h[_filter_index] = std::fabs(_daubechies_highpass[_filter_index]);
    }
    _separable_convolution(_rho_lh, width, height, _abs_l, _abs_h, _daubechies_length, _xi_lh);
    _separable_convolution(_rho_hl, width, height, _abs_h, _abs_l, _daubechies_length, _xi_hl);
    _separable_convolution(_rho_hh, width, height, _abs_h, _abs_h, _daubechies_length, _xi_hh);

    const float _pow_value = -1.f;
    const float _wet_cost = 1e10f;
    float _rho_max = 0.f;
    for (std::size_t _rho_index = 0; _rho_index < _rho_count; ++_rho_index) {
        const float _pow_lh = std::pow(_xi_lh[_rho_index], _pow_value);
        const float _pow_hl = std::pow(_xi_hl[_rho_index], _pow_value);
        const float _pow_hh = std::pow(_xi_hh[_rho_index], _pow_value);
        float _pow_sum = _pow_lh + _pow_hl + _pow_hh;
        if (_pow_sum <= 0.f) {
            _pow_sum = 1e-12f;
        }
        float _rho_value = std::pow(_pow_sum, -1.f / _pow_value);
        if (_rho_value > _wet_cost) {
            _rho_value = _wet_cost;
        }
        if (input[_rho_index] == 0 || input[_rho_index] == 255) {
            _rho_value = _wet_cost;
        }
        rho[_rho_index] = _rho_value;
        if (_rho_value > _rho_max && _rho_value < _wet_cost * 0.5f) {
            _rho_max = _rho_value;
        }
    }

    if (_rho_max > 0.f) {
        for (std::size_t _rho_index = 0; _rho_index < _rho_count; ++_rho_index) {
            if (rho[_rho_index] >= _wet_cost * 0.5f) {
                continue;
            }
            rho[_rho_index] = rho[_rho_index] * (255.f / _rho_max);
        }
    }
}

void _make_permutation(
    const std::array<std::uint8_t, 32>& steganography_key,
    const std::size_t first,
    const std::size_t count,
    std::vector<std::size_t>& indices_out)
{
    indices_out.resize(count);
    for (std::size_t _pixel_index = 0; _pixel_index < count; ++_pixel_index) {
        indices_out[_pixel_index] = first + _pixel_index;
    }
    _steganography_rng _rng(steganography_key);
    for (std::size_t _pixel_index = count; _pixel_index > 1; --_pixel_index) {
        const std::size_t _shuffle_index = _rng.next(_pixel_index);
        std::swap(indices_out[_pixel_index - 1], indices_out[_shuffle_index]);
    }
}
}

bool embed_wow(
    const std::vector<std::uint8_t>& rgb,
    const std::size_t width,
    const std::size_t height,
    const std::array<std::uint8_t, 32> stegano_key,
    const std::uint32_t constraint_height,
    const std::vector<std::uint8_t>& payload_bits,
    std::vector<std::uint8_t>& rgb_embedded,
    double& cost_embedded)
{
    if (rgb.size() != 3 * width * height) {
        throw std::runtime_error("rgb.size() must be equal to 3 * width * height");
    }
    const std::size_t _pixels_count = width * height;
    if (_pixels_count <= _payload_length_bit_count) {
        throw std::runtime_error("image too small to store length prefix");
    }

    const std::size_t _available_for_payload_count = _pixels_count - _payload_length_bit_count;
    std::vector<std::uint8_t> _y;
    encode_y(rgb, _y);
    if (_y.size() != _pixels_count) {
        throw std::runtime_error("encode_y produced unexpected Y size");
    }
    std::vector<std::uint8_t> _cover_symbols;
    encode_lsb(_y, _cover_symbols);
    if (_cover_symbols.size() != _pixels_count) {
        throw std::runtime_error("encode_lsb produced unexpected symbol count");
    }

    std::vector<float> _rho;
    _compute_wow_rho(_y, width, height, _rho);
    std::vector<std::size_t> _permutation_indices;
    _make_permutation(stegano_key, _payload_length_bit_count, _available_for_payload_count, _permutation_indices);
    std::vector<std::uint8_t> _price(_pixels_count);
    float _rho_max = 0.f;
    for (float _rho_value : _rho) { // replace by algorithm
        if (_rho_value > _rho_max) {
            _rho_max = _rho_value;
        }
    }
    const float _rho_scale = (_rho_max > 0.f) ? (255.f / _rho_max) : 1.f;
    for (std::size_t _rho_index = 0; _rho_index < _pixels_count; ++_rho_index) {
        const float _rho_value = _rho[_rho_index] * _rho_scale;
        _price[_rho_index] = _clamp_u8(_rho_value);
    }

    std::vector<std::uint8_t> _stc_cover_symbols(_available_for_payload_count);
    std::vector<std::uint8_t> _stc_price(_available_for_payload_count);
    for (std::size_t _stc_index = 0; _stc_index < _available_for_payload_count; ++_stc_index) {
        const std::size_t _permutation_index = _permutation_indices[_stc_index];
        _stc_cover_symbols[_stc_index] = _cover_symbols[_permutation_index];
        _stc_price[_stc_index] = _price[_permutation_index];
    }
    std::vector<std::uint8_t> _stc_symbols;
    encode_stc(_stc_cover_symbols, payload_bits, _stc_price, constraint_height, _stc_symbols);
    if (_stc_symbols.size() != _available_for_payload_count) {
        throw std::runtime_error("encode_stc returned wrong symbol count");
    }

    std::vector<std::uint8_t> _stegano_symbols(_pixels_count);
    const std::size_t _payload_bit_count = payload_bits.size();
    for (std::size_t _length_index = 0; _length_index < _payload_length_bit_count; ++_length_index) {
        const std::size_t _shift = (_payload_length_bit_count - 1) - _length_index;
        _stegano_symbols[_length_index] = static_cast<std::uint8_t>((_payload_bit_count >> _shift) & 0x1u);
    }
    for (std::size_t _symbol_index = 0; _symbol_index < _available_for_payload_count; ++_symbol_index) {
        const std::size_t _pixel_index = _permutation_indices[_symbol_index];
        _stegano_symbols[_pixel_index] = _stc_symbols[_symbol_index];
    }
    std::vector<std::uint8_t> _stegano_y;
    decode_lsb(_y, _stegano_symbols, _stegano_y);
    return decode_y(rgb, _stegano_y, rgb_embedded);
}

void extract_wow(
    const std::vector<std::uint8_t>& rgb_stegano,
    const std::size_t width,
    const std::size_t height,
    const std::array<std::uint8_t, 32> steganography_key,
    const std::uint32_t constraint_height,
    const std::size_t max_payload_bit_count,
    std::vector<std::uint8_t>& payload_bits)
{
    if (rgb_stegano.size() != 3 * width * height) {
        throw std::runtime_error("rgb_stego.size() must be 3 * width * height");
    }
    const std::size_t _pixels_count = width * height;
    if (_pixels_count <= _payload_length_bit_count) {
        throw std::runtime_error("image too small to contain length prefix");
    }
    if (max_payload_bit_count > width * height) {
        throw std::runtime_error("max_payload_bit_count > number of pixels");
    }
    if (max_payload_bit_count == 0) {
        payload_bits.clear();
        return;
    }

    const std::size_t _available_for_payload_count = _pixels_count - _payload_length_bit_count;
    std::vector<std::uint8_t> _stegano_y;
    encode_y(rgb_stegano, _stegano_y);
    if (_stegano_y.size() != _pixels_count) {
        throw std::runtime_error("encode_y produced unexpected Y size");
    }
    std::vector<std::uint8_t> _stegano_symbols;
    encode_lsb(_stegano_y, _stegano_symbols);
    if (_stegano_symbols.size() != _pixels_count) {
        throw std::runtime_error("encode_lsb produced unexpected symbol count");
    }
    std::size_t _payload_bit_count = 0;
    for (std::size_t _payload_index = 0; _payload_index < _payload_length_bit_count; ++_payload_index) {
        _payload_bit_count = (_payload_bit_count << 1) | (_stegano_symbols[_payload_index] & 0x1u);
    }
    if (_payload_bit_count == 0) {
        payload_bits.clear();
        return;
    }
    if (_payload_bit_count > max_payload_bit_count) {
        throw std::runtime_error("encoded payload length exceeds user cap");
    }
    if (_payload_bit_count > _available_for_payload_count) {
        throw std::runtime_error("encoded payload length does not fit in image");
    }

    std::vector<std::size_t> _permutation_indices;
    _make_permutation(steganography_key, _payload_length_bit_count, _available_for_payload_count, _permutation_indices);
    std::vector<std::uint8_t> _stc_symbols(_available_for_payload_count);
    for (std::size_t _stc_index = 0; _stc_index < _available_for_payload_count; ++_stc_index) {
        const std::size_t _pixel_index = _permutation_indices[_stc_index];
        _stc_symbols[_stc_index] = _stegano_symbols[_pixel_index];
    }
    if (_stc_symbols.size() != _available_for_payload_count) {
        throw std::runtime_error("internal STC buffer has wrong size");
    }

    payload_bits.clear();
    decode_stc(_stc_symbols, constraint_height, _payload_bit_count, payload_bits);
}
