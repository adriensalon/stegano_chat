#pragma once

void install_fonts();
void install_theme();

struct regular_icon_font_scoped {
    regular_icon_font_scoped();
    ~regular_icon_font_scoped();
};

struct bold_icon_font_scoped {
    bold_icon_font_scoped();
    ~bold_icon_font_scoped();
};

struct light_emoji_font_scoped {
    light_emoji_font_scoped();
    ~light_emoji_font_scoped();
};
