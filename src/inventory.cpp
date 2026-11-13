#include "inventory.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void Inventory::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_item", "item", "count"), &Inventory::add_item);
	ClassDB::bind_method(D_METHOD("remove_from_slot", "slot", "count"), &Inventory::remove_from_slot);
	ClassDB::bind_method(D_METHOD("move_slot", "from", "to"), &Inventory::move_slot);
	ClassDB::bind_method(D_METHOD("count_item", "id"), &Inventory::count_item);
	ClassDB::bind_method(D_METHOD("equip_from_slot", "slot"), &Inventory::equip_from_slot);
	ClassDB::bind_method(D_METHOD("unequip", "gear_slot", "to_slot"), &Inventory::unequip, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_equipped", "gear_slot"), &Inventory::get_equipped);
	ClassDB::bind_method(D_METHOD("take_equipped", "gear_slot"), &Inventory::take_equipped);
	ClassDB::bind_method(D_METHOD("select_hotbar", "index"), &Inventory::select_hotbar);
	ClassDB::bind_method(D_METHOD("get_selected_hotbar"), &Inventory::get_selected_hotbar);
	ClassDB::bind_method(D_METHOD("get_held_item"), &Inventory::get_held_item);
	ClassDB::bind_method(D_METHOD("get_item", "slot"), &Inventory::get_item);
	ClassDB::bind_method(D_METHOD("get_count", "slot"), &Inventory::get_count);
	ClassDB::bind_method(D_METHOD("get_capacity"), &Inventory::get_capacity);
	ClassDB::bind_method(D_METHOD("get_hotbar_size"), &Inventory::get_hotbar_size);
	ClassDB::bind_method(D_METHOD("find_first_empty", "from"), &Inventory::find_first_empty, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_total_weight"), &Inventory::get_total_weight);

	ADD_SIGNAL(MethodInfo("inventory_changed"));
	ADD_SIGNAL(MethodInfo("hotbar_selected", PropertyInfo(Variant::INT, "index")));
}

Inventory::Inventory() {
	slots.resize(capacity);
}

void Inventory::notify_changed() {
	emit_signal("inventory_changed");
}

int Inventory::add_item(const Ref<ItemData> &p_item, int p_count) {
	if (p_item.is_null() || p_count <= 0) {
		return p_count;
	}
	int left = p_count;
	const int max_stack = p_item->get_max_stack();

	// 1) top up existing stacks of the same item
	for (Stack &s : slots) {
		if (left == 0) {
			break;
		}
		if (!s.is_empty() && s.item == p_item && s.count < max_stack) {
			const int moved = MIN(left, max_stack - s.count);
			s.count += moved;
			left -= moved;
		}
	}
	// 2) empty slots (hotbar first, then backpack)
	for (Stack &s : slots) {
		if (left == 0) {
			break;
		}
		if (s.is_empty()) {
			const int moved = MIN(left, max_stack);
			s.item = p_item;
			s.count = moved;
			left -= moved;
		}
	}
	if (left != p_count) {
		notify_changed();
	}
	return left;
}

int Inventory::remove_from_slot(int p_slot, int p_count) {
	ERR_FAIL_COND_V(!valid_slot(p_slot), 0);
	Stack &s = slots[p_slot];
	if (s.is_empty() || p_count <= 0) {
		return 0;
	}
	const int removed = MIN(p_count, s.count);
	s.count -= removed;
	if (s.count <= 0) {
		s.clear();
	}
	notify_changed();
	return removed;
}

void Inventory::move_slot(int p_from, int p_to) {
	ERR_FAIL_COND(!valid_slot(p_from) || !valid_slot(p_to));
	if (p_from == p_to) {
		return;
	}
	Stack &a = slots[p_from];
	Stack &b = slots[p_to];
	if (a.is_empty()) {
		return;
	}
	if (!b.is_empty() && b.item == a.item && b.count < b.item->get_max_stack()) {
		const int moved = MIN(a.count, b.item->get_max_stack() - b.count);
		b.count += moved;
		a.count -= moved;
		if (a.count <= 0) {
			a.clear();
		}
	} else {
		std::swap(a, b);
	}
	notify_changed();
}

int Inventory::count_item(const StringName &p_id) const {
	int total = 0;
	for (const Stack &s : slots) {
		if (!s.is_empty() && s.item->get_id() == p_id) {
			total += s.count;
		}
	}
	return total;
}

bool Inventory::equip_from_slot(int p_slot) {
	ERR_FAIL_COND_V(!valid_slot(p_slot), false);
	Stack &s = slots[p_slot];
	if (s.is_empty() || !valid_gear_slot(s.item->get_equip_slot())) {
		return false;
	}
	const int gear = s.item->get_equip_slot();
	Stack worn;
	worn.item = s.item;
	worn.count = 1;

	if (s.count > 1) {
		// Equip one from a stack; the previously worn piece needs a free slot.
		if (!equipment[gear].is_empty()) {
			const int free_slot = find_first_empty(0);
			if (free_slot < 0) {
				return false;
			}
			slots[free_slot] = equipment[gear];
		}
		s.count -= 1;
	} else {
		// Single item: swap with whatever was worn.
		s = equipment[gear];
	}
	equipment[gear] = worn;
	notify_changed();
	return true;
}

bool Inventory::unequip(int p_gear_slot, int p_to_slot) {
	ERR_FAIL_COND_V(!valid_gear_slot(p_gear_slot), false);
	Stack &worn = equipment[p_gear_slot];
	if (worn.is_empty()) {
		return false;
	}
	int target = p_to_slot;
	if (valid_slot(target) && !slots[target].is_empty()) {
		Stack &t = slots[target];
		if (t.item->get_equip_slot() == p_gear_slot && t.count == 1) {
			std::swap(worn, t); // swap gear of the same kind
			notify_changed();
			return true;
		}
		target = -1;
	}
	if (!valid_slot(target)) {
		target = find_first_empty(Inventory::HOTBAR_SIZE);
		if (target < 0) {
			target = find_first_empty(0);
		}
	}
	if (target < 0) {
		return false; // inventory full
	}
	slots[target] = worn;
	worn.clear();
	notify_changed();
	return true;
}

Ref<ItemData> Inventory::get_equipped(int p_gear_slot) const {
	if (!valid_gear_slot(p_gear_slot)) {
		return Ref<ItemData>();
	}
	return equipment[p_gear_slot].item;
}

Ref<ItemData> Inventory::take_equipped(int p_gear_slot) {
	ERR_FAIL_COND_V(!valid_gear_slot(p_gear_slot), Ref<ItemData>());
	Ref<ItemData> item = equipment[p_gear_slot].item;
	if (item.is_valid()) {
		equipment[p_gear_slot].clear();
		notify_changed();
	}
	return item;
}

void Inventory::select_hotbar(int p_index) {
	const int index = ((p_index % HOTBAR_SIZE) + HOTBAR_SIZE) % HOTBAR_SIZE;
	if (index == selected_hotbar) {
		return;
	}
	selected_hotbar = index;
	emit_signal("hotbar_selected", selected_hotbar);
}

int Inventory::get_selected_hotbar() const { return selected_hotbar; }

Ref<ItemData> Inventory::get_held_item() const {
	const Stack &s = slots[selected_hotbar];
	if (s.is_empty() || s.item->get_equip_slot() != ItemData::SLOT_HAND) {
		return Ref<ItemData>();
	}
	return s.item;
}

Ref<ItemData> Inventory::get_item(int p_slot) const {
	return valid_slot(p_slot) ? slots[p_slot].item : Ref<ItemData>();
}

int Inventory::get_count(int p_slot) const {
	return valid_slot(p_slot) ? slots[p_slot].count : 0;
}

int Inventory::get_capacity() const { return (int)slots.size(); }

int Inventory::find_first_empty(int p_from) const {
	for (int i = MAX(0, p_from); i < (int)slots.size(); i++) {
		if (slots[i].is_empty()) {
			return i;
		}
	}
	return -1;
}

double Inventory::get_total_weight() const {
	double total = 0.0;
	for (const Stack &s : slots) {
		if (!s.is_empty()) {
			total += s.item->get_weight() * s.count;
		}
	}
	for (int i = 0; i < ItemData::SLOT_COUNT; i++) {
		if (!equipment[i].is_empty()) {
			total += equipment[i].item->get_weight();
		}
	}
	return total;
}
