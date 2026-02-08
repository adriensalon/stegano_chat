#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

struct chat_image {
    std::vector<std::uint8_t> rgb;
    std::size_t width;
    std::size_t height;
    std::unordered_map<std::string, std::string> metadata;
};

void save_image(
    const std::filesystem::path& image_path, 
    const chat_image& image);

void load_image(
    const std::filesystem::path& image_path, 
    chat_image& image);
