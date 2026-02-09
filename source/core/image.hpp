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
    std::ostream& image_stream, 
    const chat_image& image);

void load_image(
    std::istream& image_stream, 
    chat_image& image);
