#pragma once

#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include <vector>

#include "inventory_slot.h"

namespace godot {

class PlayerController;
class Inventory;

// In-game overlay: crosshair, "[E] pick up" prompt, pickup toasts, quick bar (1-8) at the bottom center.
class GameHUD : public Control {
	GDCLASS(GameHUD, Control)

private:
	struct Toast {
		Label *label = nullptr;
		double time_left = 0.0;
	};

	PlayerController *player = nullptr;
	Inventory *inventory = nullptr;

	ColorRect *crosshair = nullptr;
	Label *prompt_label = nullptr;
	Label *held_label = nullptr;
	Control *hotbar = nullptr;
	Control *hit_marker = nullptr;
	double hit_marker_time = 0.0;
	double held_label_time = 0.0;
	VBoxContainer *toast_box = nullptr;
	std::vector<Toast> toasts;
	std::vector<InventorySlot *> hotbar_slots;

	void build();
	bool bind_player();
	void refresh_hotbar();
	void on_hotbar_selected(int p_index);
	void on_item_picked_up(const String &p_name, int p_count);
	void on_inventory_full(const String &p_name);
	void on_attack_hit(const Vector3 &p_position, Object *p_collider);
	void push_toast(const String &p_text, const Color &p_color);

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double p_delta) override;
};

} // namespace godot
