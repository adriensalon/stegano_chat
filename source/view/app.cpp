#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <core/transport.hpp>
#include <core/user.hpp>
#include <view/chat.hpp>
#include <view/contacts.hpp>
#include <view/dialog.hpp>
#include <view/icon.hpp>
#include <view/login.hpp>
#include <view/theme.hpp>

std::string _memory_username = "";
std::string _memory_userpass = "";
std::optional<chat_user> _memory_user = std::nullopt;
std::optional<std::uint32_t> _memory_contact_index = std::nullopt;
std::uint32_t _memory_chat_lines = 1;
std::string _memory_chat_text = "";

namespace {

void _draw_public_key(const std::array<std::uint8_t, 32>& public_key)
{
    ImDrawList* _drawlist = ImGui::GetWindowDrawList();
    const ImVec2 _cursor_pos = ImGui::GetCursorScreenPos();
    const float _pixel_size = ImGui::GetContentRegionAvail().x / 16.f;
    const ImColor color0(212, 212, 212, 212);
    for (std::uint8_t y = 0; y < 16; ++y) {
        for (std::uint8_t x = 0; x < 16; ++x) {
            int bitIndex = y * 16 + x;
            int byteIndex = bitIndex / 8;
            int bitInByte = 7 - (bitIndex % 8);

            bool bit = (public_key[byteIndex] >> bitInByte) & 1;

            ImVec2 p0 = { _cursor_pos.x + x * _pixel_size, _cursor_pos.y + y * _pixel_size };
            ImVec2 p1 = { p0.x + _pixel_size, p0.y + _pixel_size };

            if (bit) {
                _drawlist->AddRectFilled(p0, p1, color0);
            }
        }
    }
    ImGui::Dummy({ 16 * _pixel_size, 16 * _pixel_size }); // reserve space
}

void _draw_user_settings()
{
    const float _modal_width = 400.f;
    const ImVec2 _viewport_center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(_viewport_center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(_modal_width, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(_modal_width, 0.f), ImVec2(_modal_width, FLT_MAX));
    if (ImGui::BeginPopupModal("userprofile settings###user_settings_modal", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        const float _available_width = ImGui::GetContentRegionAvail().x;
        _draw_public_key(_memory_user->public_key);
        {
            light_emoji_font_scoped _font;
            if (ImGui::Button("copy public key", ImVec2(_available_width, 0))) {
                // TODO
            }
            if (ImGui::Button("export public key...", ImVec2(_available_width, 0))) {
                std::filesystem::path _file_path;
                if (export_file(_file_path, "", { { "Text", "*.txt" } })) {
                    std::ofstream _fstream(_file_path, std::ios::binary);
                    _fstream.write(reinterpret_cast<const char*>(_memory_user->public_key.data()), 32);
                }
            }
            if (ImGui::Button("close", ImVec2(_available_width, 0))) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

void _draw_add_contact()
{
    const float _modal_width = 400.f;
    const ImVec2 _viewport_center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(_viewport_center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(_modal_width, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(_modal_width, 0.f), ImVec2(_modal_width, FLT_MAX));
    if (ImGui::BeginPopupModal("add contact###user_add_modal", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        const float _available_width = ImGui::GetContentRegionAvail().x;
        {
            light_emoji_font_scoped _font;
            static std::string _add_user_display = "";
            ImGui::SetNextItemWidth(_available_width);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
            ImGui::InputText("###user_add_input", &_add_user_display);
            ImGui::PopStyleVar();
            if (ImGui::Button("paste public key", ImVec2(_available_width, 0))) {
                // TODO
            }
            if (ImGui::Button("import public key...", ImVec2(_available_width, 0))) {
                std::filesystem::path _file_path;
                if (import_file(_file_path, "", { { "Text", "*.txt" } })) {
                    std::ifstream _fstream(_file_path, std::ios::binary);
                    std::array<std::uint8_t, 32> _read_public_key;
                    _fstream.read(reinterpret_cast<char*>(_read_public_key.data()), 32);
                    _memory_user->contacts.emplace_back().contact_public_key = _read_public_key;
                    _memory_user->contacts.back().display = _add_user_display;
                    // save_user()
                    _add_user_display.clear();
                    ImGui::CloseCurrentPopup();
                }
            }
            if (ImGui::Button("close", ImVec2(_available_width, 0))) {
                _add_user_display.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}
}

void draw_app()
{
    const ImVec2 _display_size = ImGui::GetIO().DisplaySize;

    constexpr ImGuiWindowFlags _fullscreen_window_flags = ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(_display_size);
    ImGui::Begin("fullscreen desktop", 0, _fullscreen_window_flags);

    draw_login();

    if (_memory_user) {
        ImDrawList* _drawlist = ImGui::GetWindowDrawList();
        const float _screen_width = ImGui::GetIO().DisplaySize.x;
        const float _screen_height = ImGui::GetIO().DisplaySize.y;
        const float _contacts_width = (float)(int)(0.25 * _screen_width);
        const ImU32 _line_color = IM_COL32(229, 229, 229, 255); // #E5E5E5
        const ImVec2 _style_window_padding = ImGui::GetStyle().WindowPadding;

        {
            bold_icon_font_scoped _font;
            ImGui::SetCursorPos(ImGui::GetStyle().WindowPadding * 2 + ImVec2(0, 30));
            ImGui::TextUnformatted(_memory_username.c_str());

            // DEMO
            ImGui::SameLine();
            ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, ImGui::GetStyle().FramePadding.y));
            if (ImGui::Button("settings###user_settings", ImVec2(200, 0))) {
                ImGui::OpenPopup("userprofile settings###user_settings_modal");
            }

            if (_memory_contact_index) {
                ImGui::SetCursorPos(ImVec2(_contacts_width, 0) + ImGui::GetStyle().WindowPadding * 2 + ImVec2(0, 30));
                const std::list<chat_contact>::iterator _iterator = std::next(_memory_user->contacts.begin(), _memory_contact_index.value());
                ImGui::TextUnformatted(_iterator->display.c_str());

                // DEMO
                ImGui::SameLine();
                ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, ImGui::GetStyle().FramePadding.y));
                if (ImGui::Button("settings###contact_settings", ImVec2(200, 0))) {
                    // ImGui::OpenPopup("contact settings###contact_settings_modal");
                }
            }
        }

        _drawlist->AddLine(ImVec2(0, 90), ImVec2(_screen_width, 90), _line_color, 1.f);
        _drawlist->AddLine(ImVec2(_contacts_width, 0), ImVec2(_contacts_width, _screen_height), _line_color, 1.f);

        ImGui::SetCursorPos(ImVec2(0, 91));
        ImGui::BeginChild("###contacts_child", ImVec2(_contacts_width, _screen_height - 91 - ImGui::GetFrameHeight() - 2 * _style_window_padding.y));
        draw_contacts();
        ImGui::EndChild();
        if (ImGui::Button("add contact###contact_add_button", ImVec2(_contacts_width - _style_window_padding.x * 2, 0))) {
            ImGui::OpenPopup("add contact###user_add_modal");
        }

        ImGui::SetCursorPos(_style_window_padding + ImVec2(_contacts_width, 91));
        ImGui::BeginChild("###chat_child", ImVec2(_screen_width - _contacts_width, _screen_height - 91 - _memory_chat_lines * ImGui::GetFrameHeight() - _style_window_padding.y) - _style_window_padding * 2);
        draw_chat();
        ImGui::EndChild();

        const float _button_width = 100.f;
        const float _item_spacing_x = ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(_style_window_padding.x + _contacts_width);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
        ImGui::PushTextWrapPos(0);
        {
            light_emoji_font_scoped _font;
            ImGui::InputTextMultiline("###chat_input", &_memory_chat_text, ImVec2(_screen_width - _contacts_width - _style_window_padding.x * 2 - _button_width * 2 - _item_spacing_x * 2, _memory_chat_lines * ImGui::GetFrameHeight()), ImGuiInputTextFlags_NoHorizontalScroll);
        }
        ImGui::PopTextWrapPos();
        ImGui::PopStyleVar();
        ImGui::SameLine();
        regular_icon_font_scoped _font;
        const std::string _send_button_id = std::string(ICON_FA_ENVELOPE_O) + "  send###chat_send_button";
        if (ImGui::Button(_send_button_id.c_str(), ImVec2(_button_width, 0))) {
            // TODO
            chat_image _image, _result_image;
            load_image("input.png", _image);
            const std::list<chat_contact>::iterator _iterator = std::next(_memory_user->contacts.begin(), _memory_contact_index.value());
            if (send_message(_image, _memory_user->public_key, _memory_user->private_key, _iterator->contact_public_key, _memory_chat_text, _result_image)) {
                std::filesystem::path _file_path;
                if (export_file(_file_path, "", { { "Image", "*.png" } })) {
                    save_image(_file_path, _result_image);
                    _iterator->messages.emplace_back().direction = chat_message_direction::sent;
                    _iterator->messages.back().plaintext = _memory_chat_text;
                    _memory_chat_text.clear();
                }
            }
        }
        ImGui::SameLine();
        const std::string _receive_button_id = std::string(ICON_FA_ENVELOPE_O) + "  receive###chat_receive_button";
        if (ImGui::Button(_receive_button_id.c_str(), ImVec2(_button_width, 0))) {
            // TODO
            std::filesystem::path _file_path;
            if (import_file(_file_path, "", { { "Image", "*.png" } })) {
                chat_image _image;
                load_image(_file_path, _image);
                const std::list<chat_contact>::iterator _iterator = std::next(_memory_user->contacts.begin(), _memory_contact_index.value());
                std::optional<std::string> _message;
                if (receive_message(_image, _memory_user->public_key, _memory_user->private_key, _iterator->contact_public_key, _message)) {
                    _iterator->messages.emplace_back().direction = chat_message_direction::received;
                    _iterator->messages.back().plaintext = _message.value();
                }
            }
        }
        _draw_user_settings();
        _draw_add_contact();
    }

    if (_display_size.x > _display_size.y) {

        // Landscape

    } else {

        // Portrait
    }

    ImGui::End();
}