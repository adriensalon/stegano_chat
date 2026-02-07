#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <png.h>

#include <core/image.hpp>

void load_image(const std::filesystem::path& path, chat_image& image)
{
    FILE* _file_ptr = fopen(path.string().c_str(), "rb");
    if (!_file_ptr) {
        throw std::runtime_error("Failed to open image");
    }
    png_structp _png_read = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!_png_read) {
        throw std::runtime_error("png_create_read_struct failed");
    }
    png_infop _png_info = png_create_info_struct(_png_read);
    if (!_png_info) {
        throw std::runtime_error("png_create_info_struct failed");
    }
    if (setjmp(png_jmpbuf(_png_read))) {
        throw std::runtime_error("libpng read error");
    }
    png_init_io(_png_read, _file_ptr);
    png_read_info(_png_read, _png_info);
    const int _width = png_get_image_width(_png_read, _png_info);
    const int _height = png_get_image_height(_png_read, _png_info);
    const int _color_type = png_get_color_type(_png_read, _png_info);
    const int _bit_depth = png_get_bit_depth(_png_read, _png_info);
    if (_bit_depth == 16) {
        png_set_strip_16(_png_read);
    }
    if (_color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(_png_read);
    }
    if (_color_type == PNG_COLOR_TYPE_GRAY || _color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(_png_read);
    }
    if (png_get_valid(_png_read, _png_info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(_png_read);
    }
    png_set_strip_alpha(_png_read);
    png_read_update_info(_png_read, _png_info);
    image.width = _width;
    image.height = _height;
    image.rgb.resize(_width * _height * 3);
    std::vector<png_bytep> rows(_height);
    for (int _row_index = 0; _row_index < _height; ++_row_index) {
        rows[_row_index] = image.rgb.data() + _row_index * _width * 3;
    }
    png_read_image(_png_read, rows.data());
    fclose(_file_ptr);
    png_destroy_read_struct(&_png_read, &_png_info, nullptr);
}

void save_image(const std::filesystem::path& path, const chat_image& image)
{
    FILE* _file_ptr = fopen(path.string().c_str(), "wb");
    if (!_file_ptr) {
        throw std::runtime_error("failed to open output file");
    }
    png_structp _png_write = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!_png_write) {
        throw std::runtime_error("png_create_write_struct failed");
    }
    png_infop _png_info = png_create_info_struct(_png_write);
    if (!_png_info) {
        throw std::runtime_error("png_create_info_struct failed");
    }
    if (setjmp(png_jmpbuf(_png_write))) {
        throw std::runtime_error("libpng write error");
    }
    png_init_io(_png_write, _file_ptr);
    png_set_IHDR(_png_write, _png_info, static_cast<png_uint_32>(image.width), static_cast<png_uint_32>(image.height), 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(_png_write, _png_info);
    std::vector<png_bytep> _rows(image.height);
    for (int _row_index = 0; _row_index < image.height; ++_row_index) {
        _rows[_row_index] = (png_bytep)(image.rgb.data() + _row_index * image.width * 3);
    }
    png_write_image(_png_write, _rows.data());
    png_write_end(_png_write, nullptr);
    fclose(_file_ptr);
    png_destroy_write_struct(&_png_write, &_png_info);
}
