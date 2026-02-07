#include <view/diagram.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

void draw_diagram(const std::array<std::uint8_t, 32>& public_key, const float size)
{
    ImDrawList* _drawlist = ImGui::GetWindowDrawList();
    const ImVec2 _cursor_pos = ImGui::GetCursorScreenPos();
    const float _pixel_size = size / 16.f;
    const ImColor _color(212, 212, 212, 212);
    for (std::uint8_t _y_index = 0; _y_index < 16; ++_y_index) {
        for (std::uint8_t _x_index = 0; _x_index < 16; ++_x_index) {
            const int _bit_index = _y_index * 16 + _x_index;
            const int _byte_index = _bit_index / 8;
            const int _bit_in_byte = 7 - (_bit_index % 8);
            const bool _bit = (public_key[_byte_index] >> _bit_in_byte) & 1;
            if (_bit) {
                const ImVec2 _start_pos = { _cursor_pos.x + _x_index * _pixel_size, _cursor_pos.y + _y_index * _pixel_size };
                const ImVec2 _end_pos = { _start_pos.x + _pixel_size, _start_pos.y + _pixel_size };
                _drawlist->AddRectFilled(_start_pos, _end_pos, _color);
            }
        }
    }
    ImGui::Dummy({ 16 * _pixel_size, 16 * _pixel_size });
}