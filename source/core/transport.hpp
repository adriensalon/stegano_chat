#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <core/image.hpp>

void create_keys(
    std::array<std::uint8_t, 32>& public_key,
    std::array<std::uint8_t, 32>& private_key);

bool send_message(
    const chat_image& image,
    const std::array<std::uint8_t, 32> user_public_key,
    const std::array<std::uint8_t, 32> user_private_key,
    const std::array<std::uint8_t, 32>& contact_public_key,
    const std::string& message,
    chat_image& result_image);

bool receive_message(
    const chat_image& image,
    const std::array<std::uint8_t, 32> user_public_key,
    const std::array<std::uint8_t, 32> user_private_key,
    const std::array<std::uint8_t, 32>& contact_public_key,
    std::optional<std::string>& result_message);