#include "game_hud.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "inventory.h"
#include "player_controller.h"
#include "ui_common.h"

using namespace godot;

namespace {
constexpr double TOAST_SECONDS = 2.5;
constexpr double HELD_LABEL_SECONDS = 1.6;

Label *make_outlined_label(int p_size) {
	Label *label = memnew(Label);
	label->add_theme_font_size_override("font_size", p_size);
	label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9));
	label->add_theme_constant_override("outline_size", 5);
	label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return label;
}
} // namespace

void GameHUD::_bind_methods() {}

void GameHUD::_ready() {
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	set_mouse_filter(MOUSE_FILTER_IGNORE);
	set_theme(ui::make_game_theme());
	if (Engine::get_singleton()->is_editor_hint()) {
		set_process(false);
		return;
	}
	build();
}

void GameHUD::build() {
	// Crosshair dot
	crosshair = memnew(ColorRect);
	crosshair->set_color(Color(1, 1, 1, 0.85));
	crosshair->set_anchors_preset(PRESET_CENTER);
	crosshair->set_offset(SIDE_LEFT, -2);
	crosshair->set_offset(SIDE_TOP, -2);
	crosshair->set_offset(SIDE_RIGHT, 2);
	crosshair->set_offset(SIDE_BOTTOM, 2);
	crosshair->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(crosshair);

	// Interaction prompt under the crosshair
	prompt_label = make_outlined_label(18);
	prompt_label->set_anchors_preset(PRESET_CENTER);
	prompt_label->set_offset(SIDE_LEFT, -300);
	prompt_label->set_offset(SIDE_RIGHT, 300);
	prompt_label->set_offset(SIDE_TOP, 26);
	prompt_label->set_offset(SIDE_BOTTOM, 56);
	prompt_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	add_child(prompt_label);

	// Quick bar, bottom center
	HBoxContainer *bar = memnew(HBoxContainer);
	bar->add_theme_constant_override("separation", 6);
	bar->set_mouse_filter(MOUSE_FILTER_IGNORE);
	const float bar_width = Inventory::HOTBAR_SIZE * 64.0f + (Inventory::HOTBAR_SIZE - 1) * 6.0f;
	bar->set_anchors_preset(PRESET_CENTER_BOTTOM);
	bar->set_offset(SIDE_LEFT, -bar_width / 2);
	bar->set_offset(SIDE_RIGHT, bar_width / 2);
	bar->set_offset(SIDE_TOP, -84);
	bar->set_offset(SIDE_BOTTOM, -20);
	add_child(bar);
	hotbar = bar;
	for (int i = 0; i < Inventory::HOTBAR_SIZE; i++) {
		InventorySlot *slot = memnew(InventorySlot);
		slot->setup(InventorySlot::KIND_INVENTORY, i, nullptr, false, String::num_int64(i + 1), "");
		hotbar_slots.push_back(slot);
		bar->add_child(slot);
	}

	// Name of the selected quick slot item, shown briefly above the bar
	held_label = make_outlined_label(17);
	held_label->set_anchors_preset(PRESET_CENTER_BOTTOM);
	held_label->set_offset(SIDE_LEFT, -300);
	held_label->set_offset(SIDE_RIGHT, 300);
	held_label->set_offset(SIDE_TOP, -118);
	held_label->set_offset(SIDE_BOTTOM, -90);
	held_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	add_child(held_label);

	// Pickup toasts, bottom right
	toast_box = memnew(VBoxContainer);
	toast_box->set_anchors_preset(PRESET_BOTTOM_RIGHT);
	toast_box->set_offset(SIDE_LEFT, -320);
	toast_box->set_offset(SIDE_RIGHT, -24);
	toast_box->set_offset(SIDE_TOP, -260);
	toast_box->set_offset(SIDE_BOTTOM, -100);
	toast_box->set_alignment(BoxContainer::ALIGNMENT_END);
	toast_box->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(toast_box);
}

bool GameHUD::bind_player() {
	if (player) {
		return true;
	}
	player = Object::cast_to<PlayerController>(get_tree()->get_first_node_in_group("player"));
	if (!player || !player->get_inventory()) {
		player = nullptr;
		return false;
	}
	inventory = player->get_inventory();
	inventory->connect("inventory_changed", callable_mp(this, &GameHUD::refresh_hotbar));
	inventory->connect("hotbar_selected", callable_mp(this, &GameHUD::on_hotbar_selected));
	player->connect("item_picked_up", callable_mp(this, &GameHUD::on_item_picked_up));
	player->connect("inventory_full", callable_mp(this, &GameHUD::on_inventory_full));
	refresh_hotbar();
	return true;
}

void GameHUD::_process(double p_delta) {
	if (!bind_player()) {
		return;
	}

	const bool locked = player->is_input_locked();
	crosshair->set_visible(!locked);
	hotbar->set_visible(!locked); // the inventory window has its own quick slot row
	held_label->set_visible(!locked);
	const String prompt = player->get_interaction_prompt();
	prompt_label->set_text(prompt.is_empty() ? String() : String::utf8("[E] 줍기   ") + prompt);

	if (held_label_time > 0.0) {
		held_label_time -= p_delta;
		held_label->set_modulate(Color(1, 1, 1, MIN(1.0, held_label_time / 0.4)));
	} else {
		held_label->set_text("");
	}

	for (int i = (int)toasts.size() - 1; i >= 0; i--) {
		Toast &t = toasts[i];
		t.time_left -= p_delta;
		t.label->set_modulate(Color(1, 1, 1, MIN(1.0, t.time_left / 0.5)));
		if (t.time_left <= 0.0) {
			t.label->queue_free();
			toasts.erase(toasts.begin() + i);
		}
	}
}

void GameHUD::refresh_hotbar() {
	if (!inventory) {
		return;
	}
	for (int i = 0; i < (int)hotbar_slots.size(); i++) {
		hotbar_slots[i]->set_content(inventory->get_item(i), inventory->get_count(i));
		hotbar_slots[i]->set_selected(i == inventory->get_selected_hotbar());
	}
}

void GameHUD::on_hotbar_selected(int p_index) {
	refresh_hotbar();
	const Ref<ItemData> item = inventory->get_item(p_index);
	held_label->set_text(item.is_valid() ? item->get_display_name() : String());
	held_label_time = item.is_valid() ? HELD_LABEL_SECONDS : 0.0;
}

void GameHUD::on_item_picked_up(const String &p_name, int p_count) {
	String text = "+ " + p_name;
	if (p_count > 1) {
		text += " x" + String::num_int64(p_count);
	}
	push_toast(text, Color(1, 1, 1, 1));
}

void GameHUD::on_inventory_full(const String &p_name) {
	push_toast(String::utf8("가방이 가득 찼어요 (") + p_name + ")", Color(1, 0.55, 0.45, 1));
}

void GameHUD::push_toast(const String &p_text, const Color &p_color) {
	Label *label = make_outlined_label(16);
	label->set_text(p_text);
	label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	label->add_theme_color_override("font_color", p_color);
	toast_box->add_child(label);
	toasts.push_back({ label, TOAST_SECONDS });
	if (toasts.size() > 5) {
		toasts.front().label->queue_free();
		toasts.erase(toasts.begin());
	}
}
