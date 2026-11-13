#pragma once

#include <godot_cpp/classes/area3d.hpp>

#include "item_data.h"

namespace godot {

// An item lying in the world. The player aims at it (center-screen ray) and presses E.
// Lives on physics layer 2 ("interactable") so it never blocks movement.
class PickupItem : public Area3D {
	GDCLASS(PickupItem, Area3D)

public:
	static constexpr uint32_t INTERACT_LAYER = 1u << 1;

private:
	Ref<ItemData> item;
	int count = 1;
	Node3D *visual = nullptr;

	void rebuild();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	String get_prompt() const;

	void set_item(const Ref<ItemData> &p_item);
	Ref<ItemData> get_item() const;
	void set_count(int p_count);
	int get_count() const;
};

} // namespace godot
