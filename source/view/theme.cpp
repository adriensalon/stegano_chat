#include <iostream>

#include <imgui.h>

#include <view/icon.hpp>
#include <view/theme.hpp>

extern unsigned char light_ttf[];
extern unsigned int light_ttf_len;

extern unsigned char emoji_ttf[];
extern unsigned int emoji_ttf_len;

extern unsigned char bold_ttf[];
extern unsigned int bold_ttf_len;

extern unsigned char regular_ttf[];
extern unsigned int regular_ttf_len;

extern unsigned char icon_ttf[];
extern unsigned int icon_ttf_len;

namespace {
static ImFont* _regular_icon_font = nullptr;
static ImFont* _bold_icon_font = nullptr;
static ImFont* _light_emoji_font = nullptr;

static const ImWchar _emoji_ranges[] = {
    0x2190, 0x21FF,
    0x2600, 0x26FF,
    0x2700, 0x27BF,
    (ImWchar)0x1F300, (ImWchar)0x1F5FF,
    (ImWchar)0x1F600, (ImWchar)0x1F64F,
    (ImWchar)0x1F680, (ImWchar)0x1F6FF,
    0
};

const ImWchar _icon_ranges[] = {
    ICON_MIN_FA,
    ICON_MAX_FA,
    0
};
}

void install_fonts()
{
    ImGuiIO& _io = ImGui::GetIO();
    
    const float _font_size = 15.f;

    ImFontConfig _merge_config;
    _merge_config.PixelSnapH = true;
    _merge_config.MergeMode = true;

    // regular + icon (ui)
    _regular_icon_font = _io.Fonts->AddFontFromMemoryTTF(regular_ttf, regular_ttf_len, _font_size);
    _io.Fonts->AddFontFromMemoryTTF(icon_ttf, icon_ttf_len, _font_size, &_merge_config, _icon_ranges);

    // bold + icon (ui)
    _bold_icon_font = _io.Fonts->AddFontFromMemoryTTF(bold_ttf, bold_ttf_len, _font_size);
    _io.Fonts->AddFontFromMemoryTTF(icon_ttf, icon_ttf_len, _font_size, &_merge_config, _icon_ranges);

    // light + emoji (text)
    _light_emoji_font = _io.Fonts->AddFontFromMemoryTTF(light_ttf, light_ttf_len, _font_size);
    _io.Fonts->AddFontFromMemoryTTF(emoji_ttf, emoji_ttf_len, _font_size, &_merge_config, _emoji_ranges);

    if (!_regular_icon_font) {
        std::cout << "Failed to load TTF font regular.ttf" << std::endl;
        std::terminate();
    }
    if (!_bold_icon_font) {
        std::cout << "Failed to load TTF font bold.ttf" << std::endl;
        std::terminate();
    }
    if (!_light_emoji_font) {
        std::cout << "Failed to load TTF font light.ttf or emoji.ttf" << std::endl;
        std::terminate();
    }
}

void install_theme()
{
    ImGuiStyle& _style = ImGui::GetStyle();
    _style.FramePadding = ImVec2(8, 8);
    _style.ItemSpacing = ImVec2(7, 8);
    _style.FrameRounding = 4;
    _style.WindowRounding = 4;
    _style.WindowBorderSize = 0;

    _style.Colors[ImGuiCol_Button] = ImVec4(0.60f, 0.60f, 0.60f, 0.40f);
    _style.Colors[ImGuiCol_WindowBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
}

regular_icon_font_scoped::regular_icon_font_scoped()
{
    ImGui::PushFont(_regular_icon_font);
}

regular_icon_font_scoped::~regular_icon_font_scoped()
{
    ImGui::PopFont();
}

bold_icon_font_scoped::bold_icon_font_scoped()
{
    ImGui::PushFont(_bold_icon_font);
}

bold_icon_font_scoped::~bold_icon_font_scoped()
{
    ImGui::PopFont();
}

light_emoji_font_scoped::light_emoji_font_scoped()
{
    ImGui::PushFont(_light_emoji_font);
}

light_emoji_font_scoped::~light_emoji_font_scoped()
{
    ImGui::PopFont();
}
