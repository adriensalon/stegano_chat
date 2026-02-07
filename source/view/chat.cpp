#include <optional>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <core/user.hpp>

extern std::optional<chat_user> _memory_user;
extern std::optional<std::uint32_t> _memory_contact_index;
extern std::uint32_t _memory_chat_lines;
extern std::string _memory_chat_text;

namespace {

static void _draw_message_bubble(const chat_message& msg, float wrapWidth)
{
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + wrapWidth);

    ImVec2 textSize = ImGui::CalcTextSize(msg.plaintext.c_str(), nullptr, false, wrapWidth);
    const float pad = 8.0f;
    ImVec2 bubbleSize = textSize + ImVec2(pad * 2.0f, pad * 2.0f);

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float fullWidth = ImGui::GetContentRegionAvail().x;

    ImVec2 pMin = cursor;
    if (msg.direction == chat_message_direction::sent) {
        // right aligned
        pMin.x = cursor.x + fullWidth - bubbleSize.x;
    }
    ImVec2 pMax = pMin + bubbleSize;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 colBubble;
    ImU32 colText;

    if (msg.direction == chat_message_direction::sent) {
        colBubble = ImGui::GetColorU32(ImVec4(0.18f, 0.55f, 0.98f, 1.0f)); // blue-ish
        colText = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
    } else {
        colBubble = ImGui::GetColorU32(ImVec4(0.23f, 0.23f, 0.23f, 1.0f)); // dark grey
        colText = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
    }

    float rounding = 12.0f;
    dl->AddRectFilled(pMin, pMax, colBubble, rounding);

    // Add text
    ImGui::SetCursorScreenPos(pMin + ImVec2(pad, pad));
    ImGui::PushStyleColor(ImGuiCol_Text, colText);
    ImGui::TextWrapped("%s", msg.plaintext.c_str());
    ImGui::PopStyleColor();

    // move cursor to next line under the bubble
    ImGui::SetCursorScreenPos(ImVec2(cursor.x, pMax.y + 4.0f));
    ImGui::Dummy(ImVec2(0, 0)); // keeps layout happy

    ImGui::PopTextWrapPos();
}

}

void draw_chat()
{
    if (_memory_contact_index) {
        const std::list<chat_contact>::iterator _iterator = std::next(_memory_user->contacts.begin(), _memory_contact_index.value());
        for (const chat_message& _message : _iterator->messages) {
            _draw_message_bubble(_message, 0.5f * ImGui::GetContentRegionAvail().x);
        }
    }
}