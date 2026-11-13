#pragma once

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/texture_rect.hpp>

#include "item_data.h"

namespace godot {

class InventoryUI;

// One square slot showing an item icon + stack count.
// Interactive slots (inventory screen) support drag & drop and right-click;
// non-interactive slots are used by the HUD quick bar.
class InventorySlot : public Panel {
	GDCLASS(InventorySlot, Panel)

public:
	enum Kind {
		KIND_INVENTORY = 0, // index = inventory slot
		KIND_GEAR = 1, // index = ItemData::EquipSlot
	};

private:
	int kind = KIND_INVENTORY;
	int index = 0;
	bool interactive = true;
	bool selected = false;
	bool hovered = false;
	InventoryUI *ui = nullptr;

	Ref<ItemData> item;
	int count = 0;

	TextureRect *icon_rect = nullptr;
	Label *count_label = nullptr;
	Label *key_label = nullptr;
	Label *fallback_label = nullptr;
	Label *placeholder_label = nullptr;
	Ref<StyleBoxFlat> style;

	void update_style();
	void on_mouse_entered();
	void on_mouse_exited();

protected:
	static void _bind_methods();

public:
	InventorySlot();

	void setup(int p_kind, int p_index, InventoryUI *p_ui, bool p_interactive, const String &p_key_text, const String &p_placeholder);
	void set_content(const Ref<ItemData> &p_item, int p_count);
	void set_selected(bool p_selected);
	int get_kind() const { return kind; }
	int get_index() const { return index; }
	Ref<ItemData> get_item() const { return item; }

	Variant _get_drag_data(const Vector2 &p_at_position) override;
	bool _can_drop_data(const Vector2 &p_at_position, const Variant &p_data) const override;
	void _drop_data(const Vector2 &p_at_position, const Variant &p_data) override;
	void _gui_input(const Ref<InputEvent> &p_event) override;
};

} // namespace godot
