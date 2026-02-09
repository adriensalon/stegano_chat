#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <png.h>

#include <core/image.hpp>

namespace {

void _png_ostream_write(png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::ostream* stream = reinterpret_cast<std::ostream*>(png_get_io_ptr(png_ptr));
    if (!stream->write(reinterpret_cast<char*>(data), length)) {
        png_error(png_ptr, "Write error to std::ostream");
    }
}

void _png_istream_read(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
{
    std::istream* stream = reinterpret_cast<std::istream*>(png_get_io_ptr(png_ptr));
    if (!stream->read(reinterpret_cast<char*>(outBytes), byteCountToRead)) {
        png_error(png_ptr, "Read error from std::istream");
    }
}

}

void save_image(
    std::ostream& image_stream,
    const chat_image& image)
{
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
    png_set_write_fn(_png_write, &image_stream, _png_ostream_write, nullptr);
    png_set_IHDR(_png_write, _png_info, static_cast<png_uint_32>(image.width), static_cast<png_uint_32>(image.height), 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // METADATA WIP
    std::vector<png_text> _png_texts;
    for (const std::pair<const std::string, std::string>& _metadata_pair : image.metadata) {
        png_text _png_text;
        _png_text.compression = PNG_TEXT_COMPRESSION_NONE;
        _png_text.key = const_cast<char*>(_metadata_pair.first.c_str());
        _png_text.text = const_cast<char*>(_metadata_pair.second.c_str());
        _png_text.text_length = static_cast<int>(_metadata_pair.second.size());
        _png_texts.push_back(_png_text);
    }
    if (!_png_texts.empty()) {
        png_set_text(_png_write, _png_info, _png_texts.data(), static_cast<int>(_png_texts.size()));
    }
    // METADATA WIP

    png_write_info(_png_write, _png_info);
    std::vector<png_bytep> _rows(image.height);
    for (int _row_index = 0; _row_index < image.height; ++_row_index) {
        _rows[_row_index] = (png_bytep)(image.rgb.data() + _row_index * image.width * 3);
    }
    png_write_image(_png_write, _rows.data());
    png_write_end(_png_write, nullptr);
    png_destroy_write_struct(&_png_write, &_png_info);
}

void load_image(
    std::istream& image_stream,
    chat_image& image)
{
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
    png_set_read_fn(_png_read, &image_stream, _png_istream_read);
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
    std::vector<png_bytep> _rows(_height);
    for (int _row_index = 0; _row_index < _height; ++_row_index) {
        _rows[_row_index] = image.rgb.data() + _row_index * _width * 3;
    }
    png_read_image(_png_read, _rows.data());

    // METADATA WIP
    png_textp _png_text;
    int _png_text_count;
    if (png_get_text(_png_read, _png_info, &_png_text, &_png_text_count) > 0) {
        for (int _text_index = 0; _text_index < _png_text_count; ++_text_index) {
            const std::string _key(_png_text[_text_index].key);
            const std::string _value(_png_text[_text_index].text);
            image.metadata[_key] = _value;

            // DEBUG
            std::cout << _key << " - " << _value << std::endl;
        }
    }
    // METADATA WIP

    png_destroy_read_struct(&_png_read, &_png_info, nullptr);
}