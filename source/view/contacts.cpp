#include <optional>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <core/user.hpp>
#include <view/theme.hpp>

extern std::optional<chat_user> _memory_user;
extern std::optional<std::uint32_t> _memory_contact_index;

namespace {

static std::string _truncate_with_ellipsis(const std::string& text, float max_width)
{
    std::string out = text;

    // Measure text until it fits
    while (!out.empty() && ImGui::CalcTextSize(out.c_str()).x > max_width) {
        out.pop_back();
    }

    if (out.size() < text.size()) {
        if (out.size() >= 3) {
            out.resize(out.size() - 3);
        }
        out += "...";
    }

    return out;
}
}

void draw_contacts()
{
    std::size_t _contact_index = 0;
    for (const chat_contact& _contact : _memory_user->contacts) {

        ImGui::PushID((int)_contact_index);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;
        float h = 75.0f;
        ImVec2 card_min = p;
        ImVec2 card_max = ImVec2(p.x + w, p.y + h);

        // 1) Invisible button = clickable + hover detection
        if (ImGui::InvisibleButton("contact_card", ImVec2(w, h))) {
            _memory_contact_index = (int)_contact_index; // clicked
        }

        bool hovered = ImGui::IsItemHovered();
        bool selected = (_memory_contact_index && _memory_contact_index.value() == (int)_contact_index);

        // 2) Choose colors depending on state
        ImU32 col_bg;
        if (selected) {
            col_bg = IM_COL32(230, 230, 230, 255);
        } else if (hovered) {
            col_bg = IM_COL32(245, 245, 245, 255);
        } else {
            col_bg = IM_COL32(255, 255, 255, 0);
        }

        float rounding = 6.0f;

        // 3) Draw rounded card
        dl->AddRectFilled(card_min, card_max, col_bg);

        // 4) Draw text inside the card
        ImGui::SetCursorScreenPos(card_min + ImGui::GetStyle().WindowPadding * 2);
        ImGui::TextUnformatted(_contact.display.c_str());

        ImGui::SetCursorScreenPos(card_min + ImGui::GetStyle().WindowPadding * 2 + ImVec2(0, 28));
        {
            light_emoji_font_scoped _font;

            const std::string* src;
            std::string placeholder;

            if (!_contact.messages.empty()) {
                src = &_contact.messages.back().plaintext;
            } else {
                placeholder = "say hi to " + _contact.display;
                src = &placeholder;
            }

            float max_text_width = w - (ImGui::GetStyle().WindowPadding.x * 4);

            std::string truncated = _truncate_with_ellipsis(*src, max_text_width);

            ImGui::TextUnformatted(truncated.c_str());
        }

        // 5) Move cursor under the card for next one
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h));

        ImGui::PopID();

        _contact_index++;
    }
}