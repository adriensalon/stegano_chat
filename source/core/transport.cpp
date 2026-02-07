#include <iomanip>
#include <iostream>
#include <stdexcept>

#include <sodium.h>

#include <core/transport.hpp>
#include <stegano/wow.hpp>

namespace {

enum struct _stchat_algorithm : std::uint8_t {
    WOW = 1,
};

struct _chat_header {
    std::uint8_t magic[4];
    _stchat_algorithm algorithm;
    std::array<std::uint8_t, crypto_box_NONCEBYTES> nonce;
    std::uint32_t ciphertext_size;
};

struct _chat_payload {
    _chat_header header;
    std::vector<std::uint8_t> ciphertext;
};

struct _chat_connexion {
    std::array<std::uint8_t, crypto_box_BEFORENMBYTES> shared_key;
    std::array<std::uint8_t, 32> steganography_key;
};

void _dump_hex(const char* label, const unsigned char* data, const std::size_t length)
{
    std::cerr << label;
    for (std::size_t i = 0; i < length; ++i) {
        std::cerr << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    std::cerr << std::dec << std::endl;
}

void _create_connexion(const std::array<std::uint8_t, crypto_box_SECRETKEYBYTES> user_private_key, const std::array<std::uint8_t, crypto_box_PUBLICKEYBYTES>& contact_public_key, _chat_connexion& connexion)
{
    if (sodium_init() < 0) {
        throw std::runtime_error("sodium_init failed");
    }

    if (crypto_box_beforenm(
            connexion.shared_key.data(),
            contact_public_key.data(),
            user_private_key.data())
        != 0) {
        throw std::runtime_error("crypto_box_beforenm failed");
    }

    if (crypto_generichash(
            connexion.steganography_key.data(),
            connexion.steganography_key.size(),
            connexion.shared_key.data(),
            connexion.shared_key.size(),
            reinterpret_cast<const unsigned char*>("STEGKCTX"), 8)
        != 0) {
        throw std::runtime_error("crypto_generichash failed");
    }
}

void _create_payload(const _chat_connexion& connexion, const std::array<std::uint8_t, crypto_box_PUBLICKEYBYTES> user_public_key, const std::array<std::uint8_t, crypto_box_PUBLICKEYBYTES> contact_public_key, const std::string& message, _chat_payload& result_payload)
{
    if (sodium_init() < 0) {
        throw std::runtime_error("sodium_init failed");
    }

    result_payload.header.magic[0] = 'S';
    result_payload.header.magic[1] = 'T';
    result_payload.header.magic[2] = 'C';
    result_payload.header.magic[3] = 'H';
    result_payload.header.algorithm = _stchat_algorithm::WOW;
    randombytes_buf(result_payload.header.nonce.data(), result_payload.header.nonce.size());

    const std::uint8_t* _message_ptr = reinterpret_cast<const std::uint8_t*>(message.data());
    const unsigned long long _message_length = static_cast<unsigned long long>(message.size());
    result_payload.ciphertext.resize(_message_length + crypto_box_MACBYTES);
    if (crypto_box_easy_afternm(
            result_payload.ciphertext.data(),
            _message_ptr, _message_length,
            result_payload.header.nonce.data(),
            connexion.shared_key.data())
        != 0) {
        throw std::runtime_error("crypto_box_easy_afternm failed");
    }
    result_payload.header.ciphertext_size = static_cast<std::uint32_t>(result_payload.ciphertext.size());
}

void _store_payload(const _chat_payload& payload, std::vector<std::uint8_t>& result_bytes)
{
    result_bytes.clear();
    result_bytes.reserve(4 + 1 + crypto_box_NONCEBYTES + 4 + payload.ciphertext.size());
    result_bytes.push_back(static_cast<std::uint8_t>(payload.header.magic[0]));
    result_bytes.push_back(static_cast<std::uint8_t>(payload.header.magic[1]));
    result_bytes.push_back(static_cast<std::uint8_t>(payload.header.magic[2]));
    result_bytes.push_back(static_cast<std::uint8_t>(payload.header.magic[3]));
    result_bytes.push_back(static_cast<std::uint8_t>(payload.header.algorithm));
    result_bytes.insert(result_bytes.end(), payload.header.nonce.begin(), payload.header.nonce.end());
    result_bytes.push_back((payload.header.ciphertext_size >> 24) & 0xFF);
    result_bytes.push_back((payload.header.ciphertext_size >> 16) & 0xFF);
    result_bytes.push_back((payload.header.ciphertext_size >> 8) & 0xFF);
    result_bytes.push_back((payload.header.ciphertext_size) & 0xFF);
    result_bytes.insert(result_bytes.end(), payload.ciphertext.begin(), payload.ciphertext.end());
}

bool _load_payload(const std::vector<std::uint8_t>& bytes, _chat_payload& result_payload)
{
    const std::size_t _minimum_header_size = 4 + 1 + crypto_box_NONCEBYTES + 4;
    if (bytes.size() < _minimum_header_size) {
        return false;
    }

    _chat_header _header;
    std::size_t _payload_index = 0;
    _header.magic[0] = static_cast<char>(bytes[_payload_index++]);
    _header.magic[1] = static_cast<char>(bytes[_payload_index++]);
    _header.magic[2] = static_cast<char>(bytes[_payload_index++]);
    _header.magic[3] = static_cast<char>(bytes[_payload_index++]);
    if (_header.magic[0] != 'S' || _header.magic[1] != 'T' || _header.magic[2] != 'C' || _header.magic[3] != 'H') {
        return false;
    }

    _header.algorithm = static_cast<_stchat_algorithm>(bytes[_payload_index++]);
    for (std::size_t _nonce_index = 0; _nonce_index < crypto_box_NONCEBYTES; ++_nonce_index) {
        _header.nonce[_nonce_index] = bytes[_payload_index++];
    }

    std::uint32_t _ciphertext_size = 0;
    _ciphertext_size |= static_cast<std::uint32_t>(bytes[_payload_index++]) << 24;
    _ciphertext_size |= static_cast<std::uint32_t>(bytes[_payload_index++]) << 16;
    _ciphertext_size |= static_cast<std::uint32_t>(bytes[_payload_index++]) << 8;
    _ciphertext_size |= static_cast<std::uint32_t>(bytes[_payload_index++]);
    _header.ciphertext_size = _ciphertext_size;
    if (bytes.size() < _payload_index + _ciphertext_size || _ciphertext_size < crypto_box_MACBYTES) {
        return false;
    }

    result_payload.header = std::move(_header);
    result_payload.ciphertext.assign(bytes.begin() + _payload_index, bytes.end());
    return true;
}

void _bytes_to_bits(const std::vector<std::uint8_t>& bytes, std::vector<std::uint8_t>& result_bits)
{
    if (bytes.empty()) {
        throw std::runtime_error("bytes was empty");
    }

    result_bits.clear();
    result_bits.reserve(bytes.size() * 8);
    for (std::uint8_t _byte : bytes) {
        for (std::int32_t _index = 7; _index >= 0; --_index) {
            const std::uint8_t _bit = (_byte >> _index) & 0x1u;
            result_bits.push_back(_bit);
        }
    }
}

void _bits_to_bytes(const std::vector<std::uint8_t>& bits, std::vector<std::uint8_t>& result_bytes)
{
    if (bits.empty()) {
        throw std::runtime_error("bits was empty");
    }

    std::size_t _bit_index = 0;
    const std::size_t _bytes_count = (bits.size() + 7) / 8;
    result_bytes.resize(_bytes_count, 0);
    for (std::size_t _byte_index = 0; _byte_index < _bytes_count; ++_byte_index) {
        std::uint8_t _byte = 0;
        for (std::int32_t _bit_position = 7; _bit_position >= 0; --_bit_position) {
            if (_bit_index < bits.size()) {
                _byte |= (bits[_bit_index] & 0x1u) << _bit_position;
                ++_bit_index;
            } else {
                break;
            }
        }
        result_bytes[_byte_index] = _byte;
    }
}

}

void create_keys(
    std::array<std::uint8_t, 32>& public_key,
    std::array<std::uint8_t, 32>& private_key)
{
    if (sodium_init() < 0) {
        throw std::runtime_error("sodium_init failed");
    }

    if (crypto_box_keypair(
            public_key.data(),
            private_key.data())
        != 0) {
        throw std::runtime_error("crypto_box_keypair failed");
    }
}

bool send_message(
    const chat_image& image,
    const std::array<std::uint8_t, 32> user_public_key,
    const std::array<std::uint8_t, 32> user_private_key,
    const std::array<std::uint8_t, 32>& contact_public_key,
    const std::string& message,
    chat_image& result_image)
{
    _chat_connexion _connexion;
    _create_connexion(user_private_key, contact_public_key, _connexion);
    _chat_payload _payload;
    _create_payload(_connexion, user_public_key, contact_public_key, message, _payload);

    // DEBUG
    _dump_hex("TX shared: ", _connexion.shared_key.data(), _connexion.shared_key.size());
    _dump_hex("TX nonce:  ", _payload.header.nonce.data(), _payload.header.nonce.size());
    _dump_hex("TX ciph:   ", _payload.ciphertext.data(), _payload.ciphertext.size());

    std::vector<std::uint8_t> _payload_bytes;
    _store_payload(_payload, _payload_bytes);
    std::vector<std::uint8_t> _payload_bits;
    _bytes_to_bits(_payload_bytes, _payload_bits);
    double _cost_stegano;
    std::vector<std::uint8_t> _rgb_stegano;
    if (!embed_wow(image.rgb, image.width, image.height, _connexion.steganography_key, 3, _payload_bits, _rgb_stegano, _cost_stegano)) {
        return false;
    }
    result_image.width = image.width;
    result_image.height = image.height;
    result_image.rgb = std::move(_rgb_stegano);
    return true;
}

bool receive_message(
    const chat_image& image,
    const std::array<std::uint8_t, 32> user_public_key,
    const std::array<std::uint8_t, 32> user_private_key,
    const std::array<std::uint8_t, 32>& contact_public_key,
    std::optional<std::string>& result_message)
{
    result_message = std::nullopt;
    if (sodium_init() < 0) {
        throw std::runtime_error("sodium_init failed");
    }

    _chat_connexion _connexion;
    _create_connexion(user_private_key, contact_public_key, _connexion);
    const std::size_t _max_bits = image.width * image.height;
    std::vector<std::uint8_t> _payload_bits;
    extract_wow(image.rgb, image.width, image.height, _connexion.steganography_key, /*constraint_height*/ 3, _max_bits, _payload_bits);
    if (_payload_bits.empty()) {
        return false;
    }
    std::vector<std::uint8_t> _payload_bytes;
    _bits_to_bytes(_payload_bits, _payload_bytes);
    _chat_payload _payload;
    if (!_load_payload(_payload_bytes, _payload)) {
        return false;
    }
    if (_payload.ciphertext.size() < crypto_box_MACBYTES) {
        return false;
    }

    // DEBUG
    _dump_hex("RX nonce:  ", _payload.header.nonce.data(), _payload.header.nonce.size());
    _dump_hex("RX ciph:   ", _payload.ciphertext.data(), _payload.ciphertext.size());
    _dump_hex("RX shared: ", _connexion.shared_key.data(), _connexion.shared_key.size());

    const std::size_t _message_length = _payload.ciphertext.size() - crypto_box_MACBYTES;
    std::vector<std::uint8_t> message(_message_length);
    if (crypto_box_open_easy_afternm(
            message.data(),
            _payload.ciphertext.data(), _payload.ciphertext.size(),
            _payload.header.nonce.data(),
            _connexion.shared_key.data())
        != 0) {
        throw std::runtime_error("crypto_box_open_easy_afternm failed");
    }
    result_message = std::string(reinterpret_cast<const char*>(message.data()), message.size());
    return true;
}
