#include <fstream>
#include <sstream>
#include <stdexcept>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/list.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <sodium.h>

#include <core/user.hpp>

template <typename Archive>
void serialize(Archive& archive, chat_message& data)
{
    archive(CEREAL_NVP(data.direction));
    archive(CEREAL_NVP(data.plaintext));
}

template <typename Archive>
void serialize(Archive& archive, chat_contact& data)
{
    archive(CEREAL_NVP(data.display));
    archive(CEREAL_NVP(data.contact_public_key));
    archive(CEREAL_NVP(data.messages));
}

template <typename Archive>
void serialize(Archive& archive, chat_user& data)
{
    archive(CEREAL_NVP(data.public_key));
    archive(CEREAL_NVP(data.private_key));
    archive(CEREAL_NVP(data.contacts));
}

namespace {

void _derive_key_from_password(
    const std::string& password,
    const std::array<std::uint8_t, 16>& salt,
    std::array<std::uint8_t, crypto_secretbox_KEYBYTES>& key)
{
    if (crypto_pwhash(key.data(), key.size(),
            password.c_str(), password.size(),
            salt.data(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_DEFAULT)
        != 0) {
        throw std::runtime_error("crypto_pwhash failed");
    }
}

}

void save_user(
    std::ostream& user_stream,
    const chat_user& user,
    const std::string& password)
{
    std::ostringstream _osstream(std::ios::binary);
    {
        cereal::PortableBinaryOutputArchive _cereal_archive(_osstream);
        _cereal_archive(user);
    }
    const std::string _serialized = _osstream.str();

    std::array<std::uint8_t, 16> _salt {};
    randombytes_buf(_salt.data(), _salt.size());
    std::array<std::uint8_t, crypto_secretbox_KEYBYTES> _key {};
    _derive_key_from_password(password, _salt, _key);
    std::array<std::uint8_t, crypto_secretbox_NONCEBYTES> _nonce {};
    randombytes_buf(_nonce.data(), _nonce.size());
    std::vector<std::uint8_t> _ciphertext;
    _ciphertext.resize(_serialized.size() + crypto_secretbox_MACBYTES);
    if (crypto_secretbox_easy(_ciphertext.data(),
            reinterpret_cast<const std::uint8_t*>(_serialized.data()),
            _serialized.size(),
            _nonce.data(),
            _key.data())
        != 0) {
        throw std::runtime_error("crypto_secretbox_easy failed");
    }
    const char _magic[4] = { 'S', 'U', 'S', 'R' };
    user_stream.write(_magic, 4);
    const std::uint32_t _version = 1;
    user_stream.write(reinterpret_cast<const char*>(&_version), sizeof(_version));
    user_stream.write(reinterpret_cast<const char*>(_salt.data()), _salt.size());
    user_stream.write(reinterpret_cast<const char*>(_nonce.data()), _nonce.size());
    const std::uint32_t _ciphertext_count = static_cast<std::uint32_t>(_ciphertext.size());
    user_stream.write(reinterpret_cast<const char*>(&_ciphertext_count), sizeof(_ciphertext_count));
    user_stream.write(reinterpret_cast<const char*>(_ciphertext.data()), _ciphertext.size());
}

bool load_user(
    std::istream& user_stream,
    chat_user& user,
    const std::string& password)
{
    char _magic[4];
    user_stream.read(_magic, 4);
    if (user_stream.gcount() != 4 || _magic[0] != 'S' || _magic[1] != 'U' || _magic[2] != 'S' || _magic[3] != 'R') {
        throw std::runtime_error("invalid user file magic");
    }
    std::uint32_t _version = 0;
    user_stream.read(reinterpret_cast<char*>(&_version), sizeof(_version));
    if (_version != 1) {
        throw std::runtime_error("unsupported user file version");
    }

    std::array<std::uint8_t, 16> _salt {};
    user_stream.read(reinterpret_cast<char*>(_salt.data()), _salt.size());
    std::array<std::uint8_t, crypto_secretbox_NONCEBYTES> _nonce {};
    user_stream.read(reinterpret_cast<char*>(_nonce.data()), _nonce.size());
    std::uint32_t _ciphertext_count = 0;
    user_stream.read(reinterpret_cast<char*>(&_ciphertext_count), sizeof(_ciphertext_count));
    std::vector<std::uint8_t> _ciphertext(_ciphertext_count);
    user_stream.read(reinterpret_cast<char*>(_ciphertext.data()), _ciphertext_count);

    std::array<std::uint8_t, crypto_secretbox_KEYBYTES> _key {};
    _derive_key_from_password(password, _salt, _key);
    if (_ciphertext.size() < crypto_secretbox_MACBYTES) {
        throw std::runtime_error("ciphertext too small");
    }
    std::vector<std::uint8_t> _plaintext(_ciphertext.size() - crypto_secretbox_MACBYTES);
    if (crypto_secretbox_open_easy(_plaintext.data(),
            _ciphertext.data(), _ciphertext.size(),
            _nonce.data(), _key.data())
        != 0) {
        return false;
    }

    const std::string _serialized(reinterpret_cast<const char*>(_plaintext.data()), _plaintext.size());
    std::istringstream _isstream(_serialized, std::ios::binary);
    {
        cereal::PortableBinaryInputArchive _cereal_archive(_isstream);
        _cereal_archive(user);
    }
    return true;
}