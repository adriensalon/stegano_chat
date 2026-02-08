#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>
#include <list>

enum struct chat_message_direction : std::uint8_t {
    sent,
    received,
};

struct chat_message {
    chat_message_direction direction;
    std::string plaintext;
};

struct chat_contact {
    std::string display;
    std::array<std::uint8_t, 32> contact_public_key;
    std::vector<chat_message> messages;
};

struct chat_user {
    std::array<std::uint8_t, 32> public_key;
    std::array<std::uint8_t, 32> private_key;
    std::list<chat_contact> contacts;
};

void save_user(
    const std::filesystem::path& user_path,
    const chat_user& user,
    const std::string& password,
    const bool is_download = false);

bool load_user(
    const std::filesystem::path& user_path,
    chat_user& user,
    const std::string& password);
