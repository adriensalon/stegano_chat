#include <fstream>
#include <optional>
#include <sstream>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

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
            if (std::filesystem::exists(_userprofile_path)) {
                std::ifstream _fstream(_userprofile_path, std::ios::binary);
                chat_user _user;
                if (load_user(_fstream, _user, _memory_userpass)) {
                    _memory_user = _user;
                } else {
                    _is_notify_login_failed = true;
                }
            } else {
                _is_notify_login_failed = true;
            }
        }

        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        if (ImGui::Button("create local userprofile", ImVec2(_login_width, 0))) {
            const std::filesystem::path _userprofile_path = _memory_username + ".userprofile";
            if (!std::filesystem::exists(_userprofile_path)) {
                std::ofstream _fstream(_userprofile_path, std::ios::binary);
                _memory_user = chat_user();

                // DEMO
                // stchat_contact& _lol = _memory_user->contacts.emplace_back();
                // _lol.display = "myfriend";
                // _lol.messages.push_back({ stchat_message_direction::received, "Vous avez été énorme . C’était pas une interview de plateau télé mais un tribunal de police. Bravo !" });
                // _lol.messages.push_back({ stchat_message_direction::received, "Quel courage! J'ai 50 ans et je n'ai jamais vu un responsable politique attaqué aussi fort. Ces gens ont une trouille tellement enorme et c'est de bonne augure. Merci Mme Guetté, vous etes seule sur ces plateaux... tres seule mais nous sommes tres tres nombreux derriere vous n'en doutez pas." });
                // _lol.messages.push_back({ stchat_message_direction::sent, "Wauquiez c est le manuel valls de la droite😂" });
                // _lol.messages.push_back({ stchat_message_direction::received, "Vous avez été énorme . C’était pas une interview de plateau télé mais un tribunal de police. Bravo !" });
                // _lol.messages.push_back({ stchat_message_direction::received, "Quel courage! J'ai 50 ans et je n'ai jamais vu un responsable politique attaqué aussi fort. Ces gens ont une trouille tellement enorme et c'est de bonne augure. Merci Mme Guetté, vous etes seule sur ces plateaux... tres seule mais nous sommes tres tres nombreux derriere vous n'en doutez pas." });
                // stchat_contact& _lol2 = _memory_user->contacts.emplace_back();
                // _lol2.display = "myfriend2";
                // _lol2.messages.push_back({ stchat_message_direction::received, "Vous avez été énorme . C’était pas une interview de plateau télé mais un tribunal de police. Bravo !" });
                // _lol2.messages.push_back({ stchat_message_direction::received, "Quel courage! J'ai 50 ans et je n'ai jamais vu un responsable politique attaqué aussi fort. Ces gens ont une trouille tellement enorme et c'est de bonne augure. Merci Mme Guetté, vous etes seule sur ces plateaux... tres seule mais nous sommes tres tres nombreux derriere vous n'en doutez pas." });
                // _lol2.messages.push_back({ stchat_message_direction::sent, "Wauquiez c est le manuel valls de la droite😂" });
                // _lol2.messages.push_back({ stchat_message_direction::received, "Vous avez été énorme . C’était pas une interview de plateau télé mais un tribunal de police. Bravo !" });

                create_keys(_memory_user->public_key, _memory_user->private_key);
                save_user(_fstream, _memory_user.value(), _memory_userpass);
            } else {
                _is_notify_login_failed = true;
            }
        }

        ImGui::SetCursorPosX(0.5f * (_display_size.x - _login_width));
        if (ImGui::Button("import userprofile from file", ImVec2(_login_width, 0))) {
            // open load file dialog
            std::istringstream _sstream("");
            chat_user _user;
            if (load_user(_sstream, _user, _memory_userpass)) {
                _memory_user = _user;
                // save
            } else {
                _is_notify_login_failed = true;
            }
        }

        if (_memory_user && !_memory_user->contacts.empty()) {
            _memory_contact_index = 0;
        }
    }
}