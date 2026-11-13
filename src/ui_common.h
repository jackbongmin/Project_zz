#pragma once

#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/system_font.hpp>
#include <godot_cpp/classes/theme.hpp>

namespace godot {
namespace ui {

// Colors shared by HUD and inventory screens.
inline const Color PANEL_BG(0.08, 0.09, 0.08, 0.88);
inline const Color SLOT_BG(0.13, 0.14, 0.12, 0.85);
inline const Color SLOT_BORDER(0.32, 0.33, 0.28, 1.0);
inline const Color SLOT_SELECTED(0.92, 0.78, 0.32, 1.0);
inline const Color SLOT_HOVER(0.55, 0.85, 0.35, 1.0);
inline const Color TEXT_DIM(0.72, 0.72, 0.66, 1.0);
inline const Color ACCENT(0.55, 0.9, 0.35, 1.0); // radiation green

inline Ref<StyleBoxFlat> make_box(const Color &p_bg, const Color &p_border, int p_border_width, int p_radius, int p_margin = 0) {
	Ref<StyleBoxFlat> box;
	box.instantiate();
	box->set_bg_color(p_bg);
	box->set_border_color(p_border);
	box->set_border_width_all(p_border_width);
	box->set_corner_radius_all(p_radius);
	box->set_content_margin_all(p_margin);
	return box;
}

// Theme with a system font that has Hangul glyphs (the built-in default font does not).
inline Ref<Theme> make_game_theme() {
	Ref<Theme> theme;
	Ref<SystemFont> font;
	font.instantiate();
	PackedStringArray names;
	names.push_back("Malgun Gothic");
	names.push_back("Apple SD Gothic Neo");
	names.push_back("Noto Sans CJK KR");
	names.push_back("Noto Sans KR");
	names.push_back("sans-serif");
	font->set_font_names(names);
	font->set_font_weight(500);

	theme.instantiate();
	theme->set_default_font(font);
	theme->set_default_font_size(16);
	return theme;
}

} // namespace ui
} // namespace godot
