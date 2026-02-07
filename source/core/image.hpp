#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

struct chat_image {
    std::vector<std::uint8_t> rgb;
    std::size_t width;
    std::size_t height;
    // TODO add exif metadata
};

void load_image(
    const std::filesystem::path& path, 
    chat_image& image);

void save_image(
    const std::filesystem::path& path, 
    const chat_image& image);
