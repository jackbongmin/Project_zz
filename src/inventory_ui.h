#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>

#include <vector>

#include "inventory.h"
#include "inventory_slot.h"

namespace godot {

class PlayerController;

// Full-screen inventory window (Tab). Layout:
//   [ gear: head/body/back ] [ backpack grid ]
//   [ quick slots 1-8                         ]
//   [ item info                               ]
// Drag & drop between slots, right-click to equip / move, drag outside the window to drop.
class InventoryUI : public Control {
	GDCLASS(InventoryUI, Control)

private:
	PlayerController *player = nullptr;
	Inventory *inventory = nullptr;
	bool open = false;

	std::vector<InventorySlot *> inventory_slots; // indexed by inventory slot
	InventorySlot *gear_slots[ItemData::SLOT_COUNT] = {};
	Label *weight_label = nullptr;
	Label *info_name = nullptr;
	Label *info_meta = nullptr;
	Label *info_desc = nullptr;

	void build();
	bool bind_player();
	void refresh();
	InventorySlot *make_slot(int p_kind, int p_index, const String &p_key, const String &p_placeholder);

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _input(const Ref<InputEvent> &p_event) override;
	bool _can_drop_data(const Vector2 &p_at_position, const Variant &p_data) const override;
	void _drop_data(const Vector2 &p_at_position, const Variant &p_data) override;

	void set_open(bool p_open);
	bool is_open() const;

	// Called by InventorySlot
	bool can_drop(int p_src_kind, int p_src_index, int p_dst_kind, int p_dst_index) const;
	void handle_drop(int p_src_kind, int p_src_index, int p_dst_kind, int p_dst_index);
	void handle_secondary(int p_kind, int p_index);
	void show_item_info(const Ref<ItemData> &p_item);
};

} // namespace godot
