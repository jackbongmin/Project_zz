#pragma once

#include <godot_cpp/classes/node.hpp>

#include <vector>

#include "item_data.h"

namespace godot {

// Item storage for the player.
//   slots [0, HOTBAR_SIZE)            : quick slots (keys 1-8), the selected one is held in hand
//   slots [HOTBAR_SIZE, capacity)     : backpack grid
//   equipment[ItemData::SLOT_HEAD..]  : worn gear (head / body / back)
class Inventory : public Node {
	GDCLASS(Inventory, Node)

public:
	static constexpr int HOTBAR_SIZE = 8;

private:
	struct Stack {
		Ref<ItemData> item;
		int count = 0;
		bool is_empty() const { return item.is_null() || count <= 0; }
		void clear() {
			item.unref();
			count = 0;
		}
	};

	int capacity = HOTBAR_SIZE + 20;
	std::vector<Stack> slots;
	Stack equipment[ItemData::SLOT_COUNT];
	int selected_hotbar = 0;

	bool valid_slot(int p_slot) const { return p_slot >= 0 && p_slot < (int)slots.size(); }
	bool valid_gear_slot(int p_slot) const { return p_slot > ItemData::SLOT_HAND && p_slot < ItemData::SLOT_COUNT; }
	void notify_changed();

protected:
	static void _bind_methods();

public:
	Inventory();

	// Adds as much as fits (fills existing stacks, then hotbar, then backpack). Returns the leftover count.
	int add_item(const Ref<ItemData> &p_item, int p_count);
	// Removes up to p_count from a slot and returns how many were removed.
	int remove_from_slot(int p_slot, int p_count);
	// Drag & drop between slots: merge same items, otherwise swap.
	void move_slot(int p_from, int p_to);
	int count_item(const StringName &p_id) const;

	// Gear (head / body / back)
	bool equip_from_slot(int p_slot);
	bool unequip(int p_gear_slot, int p_to_slot = -1);
	Ref<ItemData> get_equipped(int p_gear_slot) const;
	// Takes the gear out entirely (for dropping it into the world).
	Ref<ItemData> take_equipped(int p_gear_slot);

	// Quick slots
	void select_hotbar(int p_index);
	int get_selected_hotbar() const;
	Ref<ItemData> get_held_item() const; // selected quick slot item if it is a hand item

	Ref<ItemData> get_item(int p_slot) const;
	int get_count(int p_slot) const;
	int get_capacity() const;
	int get_hotbar_size() const { return HOTBAR_SIZE; }
	int find_first_empty(int p_from = 0) const;
	double get_total_weight() const;
};

} // namespace godot
