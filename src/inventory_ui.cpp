#include "inventory_ui.h"

#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/grid_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "player_controller.h"
#include "ui_common.h"

using namespace godot;

namespace {
constexpr int GRID_COLUMNS = 5;
constexpr int NEARBY_COLUMNS = 3;
constexpr int NEARBY_SLOTS = 12;
constexpr double NEARBY_SCAN_SECONDS = 0.25;

Label *make_label(const String &p_text, int p_size, const Color &p_color = Color(1, 1, 1, 1)) {
	Label *label = memnew(Label);
	label->set_text(p_text);
	label->add_theme_font_size_override("font_size", p_size);
	label->add_theme_color_override("font_color", p_color);
	label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return label;
}

String u8(const char *p_text) { return String::utf8(p_text); }
} // namespace

void InventoryUI::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_open", "open"), &InventoryUI::set_open);
	ClassDB::bind_method(D_METHOD("is_open"), &InventoryUI::is_open);
	ClassDB::bind_method(D_METHOD("handle_drop", "src_kind", "src_index", "dst_kind", "dst_index"), &InventoryUI::handle_drop);
	ClassDB::bind_method(D_METHOD("handle_secondary", "kind", "index"), &InventoryUI::handle_secondary);
}

void InventoryUI::_ready() {
	set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	set_theme(ui::make_game_theme());
	set_mouse_filter(MOUSE_FILTER_STOP); // the dim background is the "drop into world" target
	if (Engine::get_singleton()->is_editor_hint()) {
		set_process(false);
		set_process_input(false);
		return;
	}
	set_visible(false);
}

bool InventoryUI::bind_player() {
	if (inventory) {
		return true;
	}
	player = Object::cast_to<PlayerController>(get_tree()->get_first_node_in_group("player"));
	if (!player || !player->get_inventory()) {
		return false;
	}
	inventory = player->get_inventory();
	build();
	inventory->connect("inventory_changed", callable_mp(this, &InventoryUI::refresh));
	inventory->connect("hotbar_selected", callable_mp(this, &InventoryUI::refresh).unbind(1));
	refresh();
	return true;
}

void InventoryUI::_process(double p_delta) {
	if (!inventory) {
		bind_player();
		return;
	}
	if (open) {
		nearby_scan_timer -= p_delta;
		if (nearby_scan_timer <= 0.0) {
			refresh_nearby();
		}
	}
}

PickupItem *InventoryUI::get_nearby_pickup(int p_index) const {
	if (p_index < 0 || p_index >= (int)nearby_pickups.size() || nearby_pickups[p_index].is_null()) {
		return nullptr;
	}
	PickupItem *pickup = Object::cast_to<PickupItem>(ObjectDB::get_instance(nearby_pickups[p_index]));
	if (!pickup || pickup->is_queued_for_deletion()) {
		return nullptr;
	}
	return pickup;
}

void InventoryUI::refresh_nearby() {
	nearby_scan_timer = NEARBY_SCAN_SECONDS;
	if (!player || nearby_slots.empty()) {
		return;
	}
	const TypedArray<PickupItem> found = player->get_nearby_pickups();
	for (int i = 0; i < (int)nearby_slots.size(); i++) {
		PickupItem *pickup = i < found.size() ? Object::cast_to<PickupItem>(found[i]) : nullptr;
		nearby_pickups[i] = pickup ? ObjectID(pickup->get_instance_id()) : ObjectID();
		nearby_slots[i]->set_content(pickup ? pickup->get_item() : Ref<ItemData>(), pickup ? pickup->get_count() : 0);
	}
	if (nearby_title) {
		String title = u8("주변 ") + String::num(player->get_nearby_radius(), 0) + "m";
		if (found.size() > (int)nearby_slots.size()) {
			title += "  (+" + String::num_int64(found.size() - nearby_slots.size()) + ")";
		}
		nearby_title->set_text(title);
	}
}

InventorySlot *InventoryUI::make_slot(int p_kind, int p_index, const String &p_key, const String &p_placeholder) {
	InventorySlot *slot = memnew(InventorySlot);
	slot->setup(p_kind, p_index, this, true, p_key, p_placeholder);
	return slot;
}

void InventoryUI::build() {
	ColorRect *dim = memnew(ColorRect);
	dim->set_color(Color(0, 0, 0, 0.45));
	dim->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	dim->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(dim);

	CenterContainer *center = memnew(CenterContainer);
	center->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	center->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(center);

	PanelContainer *panel = memnew(PanelContainer);
	panel->add_theme_stylebox_override("panel", ui::make_box(ui::PANEL_BG, Color(0.3, 0.32, 0.26, 1), 2, 10, 18));
	panel->set_mouse_filter(MOUSE_FILTER_STOP); // dropping onto the window itself does nothing
	center->add_child(panel);

	VBoxContainer *root = memnew(VBoxContainer);
	root->add_theme_constant_override("separation", 12);
	panel->add_child(root);

	// Header
	HBoxContainer *header = memnew(HBoxContainer);
	root->add_child(header);
	Label *title = make_label(u8("인벤토리"), 22, ui::ACCENT);
	title->set_h_size_flags(SIZE_EXPAND_FILL);
	header->add_child(title);
	weight_label = make_label("", 15, ui::TEXT_DIM);
	header->add_child(weight_label);
	root->add_child(memnew(HSeparator));

	// Gear + backpack
	HBoxContainer *body = memnew(HBoxContainer);
	body->add_theme_constant_override("separation", 28);
	root->add_child(body);

	VBoxContainer *gear_col = memnew(VBoxContainer);
	gear_col->add_theme_constant_override("separation", 8);
	body->add_child(gear_col);
	gear_col->add_child(make_label(u8("장착"), 16, ui::TEXT_DIM));
	const int gear_order[] = { ItemData::SLOT_HEAD, ItemData::SLOT_BODY, ItemData::SLOT_BACK };
	for (int gear : gear_order) {
		HBoxContainer *row = memnew(HBoxContainer);
		row->add_theme_constant_override("separation", 10);
		gear_col->add_child(row);
		InventorySlot *slot = make_slot(InventorySlot::KIND_GEAR, gear, "", ItemData::get_slot_name(gear));
		gear_slots[gear] = slot;
		row->add_child(slot);
		Label *name = make_label(ItemData::get_slot_name(gear), 15);
		name->set_v_size_flags(SIZE_SHRINK_CENTER);
		row->add_child(name);
	}

	VBoxContainer *bag_col = memnew(VBoxContainer);
	bag_col->add_theme_constant_override("separation", 8);
	body->add_child(bag_col);
	bag_col->add_child(make_label(u8("가방"), 16, ui::TEXT_DIM));
	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(GRID_COLUMNS);
	grid->add_theme_constant_override("h_separation", 6);
	grid->add_theme_constant_override("v_separation", 6);
	bag_col->add_child(grid);

	inventory_slots.assign(inventory->get_capacity(), nullptr);
	for (int i = Inventory::HOTBAR_SIZE; i < inventory->get_capacity(); i++) {
		InventorySlot *slot = make_slot(InventorySlot::KIND_INVENTORY, i, "", "");
		inventory_slots[i] = slot;
		grid->add_child(slot);
	}

	// Nearby pickups (items lying within the player's nearby radius)
	VBoxContainer *near_col = memnew(VBoxContainer);
	near_col->add_theme_constant_override("separation", 8);
	body->add_child(near_col);
	nearby_title = make_label(u8("주변"), 16, ui::TEXT_DIM);
	near_col->add_child(nearby_title);
	PanelContainer *near_panel = memnew(PanelContainer);
	near_panel->add_theme_stylebox_override("panel", ui::make_box(Color(0.2, 0.26, 0.14, 0.35), Color(0.4, 0.55, 0.28, 0.6), 1, 8, 6));
	near_panel->set_mouse_filter(MOUSE_FILTER_PASS);
	near_col->add_child(near_panel);
	GridContainer *near_grid = memnew(GridContainer);
	near_grid->set_columns(NEARBY_COLUMNS);
	near_grid->add_theme_constant_override("h_separation", 6);
	near_grid->add_theme_constant_override("v_separation", 6);
	near_panel->add_child(near_grid);
	for (int i = 0; i < NEARBY_SLOTS; i++) {
		InventorySlot *slot = make_slot(InventorySlot::KIND_NEARBY, i, "", "");
		nearby_slots.push_back(slot);
		near_grid->add_child(slot);
	}
	nearby_pickups.assign(NEARBY_SLOTS, ObjectID());

	// Quick slots
	root->add_child(make_label(u8("퀵슬롯"), 16, ui::TEXT_DIM));
	HBoxContainer *hotbar = memnew(HBoxContainer);
	hotbar->add_theme_constant_override("separation", 6);
	root->add_child(hotbar);
	for (int i = 0; i < Inventory::HOTBAR_SIZE; i++) {
		InventorySlot *slot = make_slot(InventorySlot::KIND_INVENTORY, i, String::num_int64(i + 1), "");
		inventory_slots[i] = slot;
		hotbar->add_child(slot);
	}

	// Info box
	root->add_child(memnew(HSeparator));
	VBoxContainer *info = memnew(VBoxContainer);
	info->set_custom_minimum_size(Vector2(0, 86));
	root->add_child(info);
	info_name = make_label("", 18);
	info->add_child(info_name);
	info_meta = make_label("", 14, ui::ACCENT);
	info->add_child(info_meta);
	info_desc = make_label("", 14, ui::TEXT_DIM);
	info_desc->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	info_desc->set_custom_minimum_size(Vector2(560, 0));
	info->add_child(info_desc);

	root->add_child(make_label(u8("드래그: 옮기기 · 주변 칸에서 끌어오면 줍기   우클릭: 장착/이동/줍기   창 밖이나 주변 칸으로 끌기: 버리기   Tab/Esc: 닫기"), 13, ui::TEXT_DIM));
	show_item_info(Ref<ItemData>());
}

void InventoryUI::refresh() {
	if (!inventory) {
		return;
	}
	for (int i = 0; i < (int)inventory_slots.size(); i++) {
		if (inventory_slots[i]) {
			inventory_slots[i]->set_content(inventory->get_item(i), inventory->get_count(i));
			inventory_slots[i]->set_selected(i == inventory->get_selected_hotbar());
		}
	}
	for (int g = ItemData::SLOT_HEAD; g < ItemData::SLOT_COUNT; g++) {
		if (gear_slots[g]) {
			gear_slots[g]->set_content(inventory->get_equipped(g), 1);
		}
	}
	if (weight_label) {
		weight_label->set_text(String::utf8("무게 ") + String::num(inventory->get_total_weight(), 1) + " kg");
	}
}

void InventoryUI::_input(const Ref<InputEvent> &p_event) {
	if (!inventory && !bind_player()) {
		return;
	}
	if (p_event->is_action_pressed("inventory") || (open && p_event->is_action_pressed("release_mouse"))) {
		set_open(!open);
		get_viewport()->set_input_as_handled();
	}
}

void InventoryUI::set_open(bool p_open) {
	if (!inventory && !bind_player()) {
		return;
	}
	open = p_open;
	set_visible(open);
	player->set_input_locked(open);
	if (open) {
		refresh();
		refresh_nearby();
	}
}

bool InventoryUI::is_open() const { return open; }

void InventoryUI::show_item_info(const Ref<ItemData> &p_item) {
	if (!info_name) {
		return;
	}
	if (p_item.is_null()) {
		info_name->set_text(u8("아이템에 마우스를 올려보세요"));
		info_meta->set_text("");
		info_desc->set_text("");
		return;
	}
	info_name->set_text(p_item->get_display_name());
	String meta = ItemData::get_category_name(p_item->get_category());
	if (p_item->get_use_type() != ItemData::USE_OTHER) {
		meta += u8("  ·  ") + ItemData::get_use_type_name(p_item->get_use_type()) + u8(" 공격력 ") + String::num(p_item->get_damage(), 0);
	}
	if (p_item->get_equip_slot() == ItemData::SLOT_HAND) {
		meta += u8("  ·  손에 들기 (퀵슬롯)");
	} else if (p_item->is_equippable()) {
		meta += u8("  ·  장착: ") + ItemData::get_slot_name(p_item->get_equip_slot());
	}
	meta += u8("  ·  ") + String::num(p_item->get_weight(), 2) + " kg";
	if (p_item->get_max_stack() > 1) {
		meta += u8("  ·  최대 ") + String::num_int64(p_item->get_max_stack()) + u8("개");
	}
	info_meta->set_text(meta);
	info_desc->set_text(p_item->get_description());
}

// --- drag & drop ----------------------------------------------------------------

bool InventoryUI::can_drop(int p_src_kind, int p_src_index, int p_dst_kind, int p_dst_index) const {
	if (!inventory) {
		return false;
	}
	if (p_src_kind == InventorySlot::KIND_INVENTORY && p_dst_kind == InventorySlot::KIND_INVENTORY) {
		return true;
	}
	if (p_src_kind == InventorySlot::KIND_INVENTORY && p_dst_kind == InventorySlot::KIND_GEAR) {
		const Ref<ItemData> item = inventory->get_item(p_src_index);
		return item.is_valid() && item->get_equip_slot() == p_dst_index;
	}
	if (p_src_kind == InventorySlot::KIND_GEAR && p_dst_kind == InventorySlot::KIND_INVENTORY) {
		return true;
	}
	// Nearby (ground) <-> inventory
	if (p_src_kind == InventorySlot::KIND_NEARBY && p_dst_kind == InventorySlot::KIND_INVENTORY) {
		return get_nearby_pickup(p_src_index) != nullptr;
	}
	if (p_src_kind == InventorySlot::KIND_NEARBY && p_dst_kind == InventorySlot::KIND_GEAR) {
		const PickupItem *pickup = get_nearby_pickup(p_src_index);
		return pickup && pickup->get_item()->get_equip_slot() == p_dst_index && inventory->get_equipped(p_dst_index).is_null();
	}
	if (p_dst_kind == InventorySlot::KIND_NEARBY) {
		return p_src_kind != InventorySlot::KIND_NEARBY; // drop onto the ground
	}
	return false;
}

void InventoryUI::handle_drop(int p_src_kind, int p_src_index, int p_dst_kind, int p_dst_index) {
	if (!can_drop(p_src_kind, p_src_index, p_dst_kind, p_dst_index)) {
		return;
	}
	if (p_src_kind == InventorySlot::KIND_INVENTORY && p_dst_kind == InventorySlot::KIND_INVENTORY) {
		inventory->move_slot(p_src_index, p_dst_index);
	} else if (p_src_kind == InventorySlot::KIND_INVENTORY && p_dst_kind == InventorySlot::KIND_GEAR) {
		inventory->equip_from_slot(p_src_index);
	} else if (p_src_kind == InventorySlot::KIND_GEAR && p_dst_kind == InventorySlot::KIND_INVENTORY) {
		inventory->unequip(p_src_index, p_dst_index);
	} else if (p_src_kind == InventorySlot::KIND_NEARBY && p_dst_kind == InventorySlot::KIND_INVENTORY) {
		player->take_pickup(get_nearby_pickup(p_src_index), p_dst_index);
	} else if (p_src_kind == InventorySlot::KIND_NEARBY && p_dst_kind == InventorySlot::KIND_GEAR) {
		const int slot = inventory->find_first_empty(Inventory::HOTBAR_SIZE);
		if (slot >= 0 && player->take_pickup(get_nearby_pickup(p_src_index), slot) > 0) {
			inventory->equip_from_slot(slot);
		}
	} else if (p_dst_kind == InventorySlot::KIND_NEARBY) {
		if (p_src_kind == InventorySlot::KIND_GEAR) {
			player->drop_equipped(p_src_index);
		} else {
			player->drop_from_slot(p_src_index);
		}
	}
	refresh_nearby();
}

void InventoryUI::handle_secondary(int p_kind, int p_index) {
	if (!inventory) {
		return;
	}
	if (p_kind == InventorySlot::KIND_GEAR) {
		inventory->unequip(p_index);
		return;
	}
	if (p_kind == InventorySlot::KIND_NEARBY) {
		player->take_pickup(get_nearby_pickup(p_index), -1); // right-click: take it
		refresh_nearby();
		return;
	}
	const Ref<ItemData> item = inventory->get_item(p_index);
	if (item.is_null()) {
		return;
	}
	if (item->is_equippable() && item->get_equip_slot() != ItemData::SLOT_HAND) {
		inventory->equip_from_slot(p_index);
		return;
	}
	// Otherwise shuttle the stack between the backpack and the quick slots.
	if (p_index >= Inventory::HOTBAR_SIZE) {
		int target = -1;
		for (int i = 0; i < Inventory::HOTBAR_SIZE && target < 0; i++) {
			if (inventory->get_item(i).is_null()) {
				target = i;
			}
		}
		if (target < 0) {
			target = inventory->get_selected_hotbar(); // swap with the selected quick slot
		}
		inventory->move_slot(p_index, target);
	} else {
		const int target = inventory->find_first_empty(Inventory::HOTBAR_SIZE);
		if (target >= 0) {
			inventory->move_slot(p_index, target);
		}
	}
}

bool InventoryUI::_can_drop_data(const Vector2 &p_at_position, const Variant &p_data) const {
	if (p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	const Dictionary data = p_data;
	return String(data.get("type", "")) == "inventory_item" && int(data["kind"]) != InventorySlot::KIND_NEARBY;
}

void InventoryUI::_drop_data(const Vector2 &p_at_position, const Variant &p_data) {
	// Dropped on the dim background (outside the window): throw it into the world.
	const Dictionary data = p_data;
	if (!player) {
		return;
	}
	if (int(data["kind"]) == InventorySlot::KIND_GEAR) {
		player->drop_equipped(int(data["index"]));
	} else {
		player->drop_from_slot(int(data["index"]));
	}
	refresh_nearby();
}
