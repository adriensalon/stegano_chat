#include <cstddef>
#include <limits>
#include <stdexcept>

#include <stegano/stc.hpp>

// TODO real trellis-based STC
// https://staff.emu.edu.tr/alexanderchefranov/Documents/CMSE492/Spring2019/FillerIEEETIFS2011%20Minimizing%20Additive%20Distortion%20in%20Steganography.pdf

namespace {

inline std::uint8_t _bit_from_symbol(std::int8_t v)
{
    return static_cast<std::uint8_t>(v & 1);
}

inline std::uint8_t _bit_from_symbol(std::uint8_t v)
{
    return v & 1u;
}

}

double encode_stc(
    const std::vector<std::uint8_t>& cover_symbols,
    const std::vector<std::uint8_t>& syndrome_bits,
    const std::vector<std::uint8_t>& pricevector,
    const std::uint32_t constraint_height,
    std::vector<std::uint8_t>& stego_symbols)
{
    (void)constraint_height; // reserved for a real trellis-based STC later

    const std::size_t _payload_bit_count = cover_symbols.size();
    const std::size_t _stegano_bit_count = syndrome_bits.size();

    if (_stegano_bit_count > _payload_bit_count) {
        throw std::runtime_error("payload length cannot exceed cover length in this implementation");
    }
    if (pricevector.size() != _payload_bit_count) {
        throw std::runtime_error("pricevector size must match cover_symbols size");
    }
    if (_stegano_bit_count == 0) {
        stego_symbols.assign(_payload_bit_count, 0);
        for (std::size_t _bit_index = 0; _bit_index < _payload_bit_count; ++_bit_index) {
            stego_symbols[_bit_index] = _bit_from_symbol(cover_symbols[_bit_index]);
        }
        return 0;
    }

    const std::size_t _base_block_size = _payload_bit_count / _stegano_bit_count;
    const std::size_t _remainder = _payload_bit_count % _stegano_bit_count;
    stego_symbols.resize(_payload_bit_count);
    for (std::size_t _bit_index = 0; _bit_index < _payload_bit_count; ++_bit_index) {
        stego_symbols[_bit_index] = _bit_from_symbol(cover_symbols[_bit_index]);
    }

    std::size_t _index = 0;
    double _total_price = 0;
    for (std::size_t _bit_index = 0; _bit_index < _stegano_bit_count; ++_bit_index) {
        const std::size_t _block_size = _base_block_size + (_bit_index < _remainder ? 1 : 0);
        const std::size_t _start = _index;
        const std::size_t _end = _index + _block_size;
        _index = _end;
        if (_start >= _end) {
            break;
        }
        const std::uint8_t _target_bit = syndrome_bits[_bit_index] & 1u;
        std::uint8_t _parity = 0;
        for (std::size_t _parity_index = _start; _parity_index < _end; ++_parity_index) {
            _parity ^= (stego_symbols[_parity_index] & 1u);
        }
        if (_parity == _target_bit) {
            continue;
        }

        float _best_cost = std::numeric_limits<float>::infinity();
        std::size_t _best_index = _end;
        for (std::size_t _cost_index = _start; _cost_index < _end; ++_cost_index) {
            const float _cost = static_cast<float>(pricevector[_cost_index]);
            if (_cost < _best_cost) {
                _best_cost = _cost;
                _best_index = _cost_index;
            }
        }
        if (_best_index == _end) {
            throw std::runtime_error("no valid position found in block");
        }
        stego_symbols[_best_index] ^= 1u;
        _total_price += static_cast<double>(pricevector[_best_index]);
    }

    return _total_price;
}

void decode_stc(
    const std::vector<std::uint8_t>& stego_symbols,
    const std::uint32_t constraint_height,
    const std::size_t payload_bit_count,
    std::vector<std::uint8_t>& syndrome_bits_out)
{
    (void)constraint_height; // reserved for a real trellis-based STC later

    const std::size_t _stego_bit_count = stego_symbols.size();
    if (payload_bit_count == 0) {
        syndrome_bits_out.clear();
        return;
    }
    if (payload_bit_count > _stego_bit_count) {
        throw std::runtime_error("payload_bit_count cannot exceed stego length");
    }

    const std::size_t _base_block_size = _stego_bit_count / payload_bit_count;
    const std::size_t _remainder = _stego_bit_count % payload_bit_count;
    syndrome_bits_out.clear();
    syndrome_bits_out.reserve(payload_bit_count);
    std::size_t _index = 0;
    for (std::size_t _bit_index = 0; _bit_index < payload_bit_count; ++_bit_index) {
        const std::size_t _block_size = _base_block_size + (_bit_index < _remainder ? 1 : 0);
        const std::size_t _start = _index;
        const std::size_t _end = _index + _block_size;
        _index = _end;
        if (_start >= _end) {
            syndrome_bits_out.push_back(0);
            continue;
        }
        std::uint8_t _parity = 0;
        for (std::size_t i = _start; i < _end; ++i) {
            _parity ^= _bit_from_symbol(stego_symbols[i]);
        }
        syndrome_bits_out.push_back(_parity & 1u);
    }
}
