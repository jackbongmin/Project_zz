#include "inventory_slot.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "inventory_ui.h"
#include "ui_common.h"

using namespace godot;

namespace {
constexpr float SLOT_SIZE = 64.0f;
}

void InventorySlot::_bind_methods() {}

InventorySlot::InventorySlot() {
	set_custom_minimum_size(Vector2(SLOT_SIZE, SLOT_SIZE));
	set_focus_mode(FOCUS_NONE);

	style = ui::make_box(ui::SLOT_BG, ui::SLOT_BORDER, 2, 6);
	add_theme_stylebox_override("panel", style);

	placeholder_label = memnew(Label);
	placeholder_label->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	placeholder_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	placeholder_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	placeholder_label->add_theme_font_size_override("font_size", 13);
	placeholder_label->add_theme_color_override("font_color", Color(1, 1, 1, 0.22));
	placeholder_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(placeholder_label);

	icon_rect = memnew(TextureRect);
	icon_rect->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	icon_rect->set_offset(SIDE_LEFT, 6);
	icon_rect->set_offset(SIDE_TOP, 6);
	icon_rect->set_offset(SIDE_RIGHT, -6);
	icon_rect->set_offset(SIDE_BOTTOM, -6);
	icon_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	icon_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(icon_rect);

	fallback_label = memnew(Label);
	fallback_label->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	fallback_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	fallback_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	fallback_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	fallback_label->add_theme_font_size_override("font_size", 12);
	fallback_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(fallback_label);

	key_label = memnew(Label);
	key_label->set_position(Vector2(5, 1));
	key_label->add_theme_font_size_override("font_size", 12);
	key_label->add_theme_color_override("font_color", ui::TEXT_DIM);
	key_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(key_label);

	count_label = memnew(Label);
	count_label->set_anchors_and_offsets_preset(PRESET_BOTTOM_RIGHT);
	count_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	count_label->set_offset(SIDE_LEFT, -40);
	count_label->set_offset(SIDE_TOP, -22);
	count_label->set_offset(SIDE_RIGHT, -5);
	count_label->set_offset(SIDE_BOTTOM, -1);
	count_label->add_theme_font_size_override("font_size", 14);
	count_label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 1));
	count_label->add_theme_constant_override("outline_size", 4);
	count_label->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(count_label);

	connect("mouse_entered", callable_mp(this, &InventorySlot::on_mouse_entered));
	connect("mouse_exited", callable_mp(this, &InventorySlot::on_mouse_exited));
}

void InventorySlot::setup(int p_kind, int p_index, InventoryUI *p_ui, bool p_interactive, const String &p_key_text, const String &p_placeholder) {
	kind = p_kind;
	index = p_index;
	ui = p_ui;
	interactive = p_interactive;
	key_label->set_text(p_key_text);
	placeholder_label->set_text(p_placeholder);
	set_mouse_filter(interactive ? MOUSE_FILTER_STOP : MOUSE_FILTER_IGNORE);
}

void InventorySlot::set_content(const Ref<ItemData> &p_item, int p_count) {
	item = p_item;
	count = item.is_valid() ? p_count : 0;
	const Ref<Texture2D> icon = item.is_valid() ? item->get_icon() : Ref<Texture2D>();
	icon_rect->set_texture(icon);
	fallback_label->set_text(item.is_valid() && icon.is_null() ? item->get_display_name() : String());
	count_label->set_text(count > 1 ? String::num_int64(count) : String());
	placeholder_label->set_visible(item.is_null());
}

void InventorySlot::set_selected(bool p_selected) {
	selected = p_selected;
	update_style();
}

void InventorySlot::update_style() {
	if (selected) {
		style->set_border_color(ui::SLOT_SELECTED);
		style->set_border_width_all(3);
	} else if (hovered) {
		style->set_border_color(ui::SLOT_HOVER);
		style->set_border_width_all(2);
	} else {
		style->set_border_color(ui::SLOT_BORDER);
		style->set_border_width_all(2);
	}
}

void InventorySlot::on_mouse_entered() {
	hovered = true;
	update_style();
	if (ui) {
		ui->show_item_info(item);
	}
}

void InventorySlot::on_mouse_exited() {
	hovered = false;
	update_style();
}

Variant InventorySlot::_get_drag_data(const Vector2 &p_at_position) {
	if (!interactive || item.is_null()) {
		return Variant();
	}
	TextureRect *preview = memnew(TextureRect);
	preview->set_texture(item->get_icon());
	preview->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	preview->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	preview->set_size(Vector2(56, 56));
	preview->set_position(Vector2(-28, -28));
	preview->set_modulate(Color(1, 1, 1, 0.85));
	Control *holder = memnew(Control);
	holder->add_child(preview);
	set_drag_preview(holder);

	Dictionary data;
	data["type"] = "inventory_item";
	data["kind"] = kind;
	data["index"] = index;
	return data;
}

bool InventorySlot::_can_drop_data(const Vector2 &p_at_position, const Variant &p_data) const {
	if (!interactive || !ui || p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	const Dictionary data = p_data;
	if (String(data.get("type", "")) != "inventory_item") {
		return false;
	}
	return ui->can_drop(int(data["kind"]), int(data["index"]), kind, index);
}

void InventorySlot::_drop_data(const Vector2 &p_at_position, const Variant &p_data) {
	const Dictionary data = p_data;
	if (ui) {
		ui->handle_drop(int(data["kind"]), int(data["index"]), kind, index);
	}
}

void InventorySlot::_gui_input(const Ref<InputEvent> &p_event) {
	const InputEventMouseButton *mb = Object::cast_to<InputEventMouseButton>(p_event.ptr());
	if (interactive && ui && mb && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_RIGHT) {
		ui->handle_secondary(kind, index);
		accept_event();
	}
}
