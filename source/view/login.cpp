#include <fstream>
#include <optional>
#include <sstream>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <core/dialog.hpp>
#include <core/transport.hpp>
#include <core/user.hpp>
#include <view/theme.hpp>

extern std::string _memory_username;
extern std::string _memory_userpass;
extern std::optional<chat_user> _memory_user;
extern std::optional<std::uint32_t> _memory_contact_index;

static bool _is_notify_login_failed = false;

void draw_login()
{
    if (_memory_user) {
        return;
    }

    const ImVec2 _display_size = ImGui::GetIO().DisplaySize;

    if (_display_size.x > _display_size.y) {

        // Landscape

    } else {

        // Portrait
    }

    const float _login_width = 400.f;

    ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
    ImGui::SetCursorPosY(0.5f * (_display_size.y - 380));
    {
        bold_icon_font_scoped _font;
        ImGui::TextUnformatted("steganography");
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 3);
    {
        ImGui::TextUnformatted("messenger");

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        ImGui::TextUnformatted("local username");
        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        ImGui::SetNextItemWidth(_login_width);
        {

            light_emoji_font_scoped _font;
            ImGui::InputText("###login_local_username_edit", &_memory_username);
        }
        ImGui::Spacing();
        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        ImGui::TextUnformatted("password");
        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        ImGui::SetNextItemWidth(_login_width);
        {
            light_emoji_font_scoped _font;
            ImGui::InputText("###login_password_edit", &_memory_userpass);
        }
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        if (ImGui::Button("login to local userprofile", ImVec2(_login_width, 0))) {
            const std::filesystem::path _userprofile_path = _memory_username + ".userprofile";
            // const std::filesystem::path _userprofile_path = "data/" + _memory_username + ".userprofile";
            chat_user _user;
            bool _is_userprofile_loaded = false;
            load_stream([&](std::istream& _stream) {
                if (load_user(_stream, _user, _memory_userpass)) {
                    _memory_user = _user;
                    _is_userprofile_loaded = true;
                }
            },
                _userprofile_path);
            if (!_is_userprofile_loaded) {
                _is_notify_login_failed = true;
            }
        }

        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        if (ImGui::Button("create local userprofile", ImVec2(_login_width, 0))) {
            const std::filesystem::path _userprofile_path = _memory_username + ".userprofile";
            // const std::filesystem::path _userprofile_path = "data/" + _memory_username + ".userprofile";
            bool _is_userprofile_present = false;
            load_stream([&](std::istream& _stream) {
                (void)_stream;
                _is_userprofile_present = true;
            },
                _userprofile_path);

            if (!_is_userprofile_present) {
                _memory_user = chat_user();
                create_keys(_memory_user->public_key, _memory_user->private_key);
                save_stream([&](std::ostream& _stream) {
                    save_user(_stream, _memory_user.value(), _memory_userpass);
                },
                    _userprofile_path);
            } else {
                _is_notify_login_failed = true;
            }
        }

        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        if (ImGui::Button("import userprofile from file", ImVec2(_login_width, 0))) {
            // open load file dialog
            // std::istringstream _sstream("");
            // chat_user _user;
            // if (load_user(_sstream, _user, _memory_userpass)) {
            //     _memory_user = _user;
            //     // save
            // } else {
            //     _is_notify_login_failed = true;
            // }
        }

        if (_memory_user && !_memory_user->contacts.empty()) {
            _memory_contact_index = 0;
        }
    }
}