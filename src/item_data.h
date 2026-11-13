#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot {

// Static definition of an item type (one .tres per item under res://items/).
class ItemData : public Resource {
	GDCLASS(ItemData, Resource)

public:
	enum EquipSlot {
		SLOT_NONE = 0,
		SLOT_HAND = 1,
		SLOT_HEAD = 2,
		SLOT_BODY = 3,
		SLOT_BACK = 4,
		SLOT_COUNT = 5,
	};

	enum UseType {
		USE_OTHER = 0, // 기타
		USE_MELEE = 1, // 근거리
		USE_RANGED = 2, // 원거리
	};

	enum Category {
		CAT_MISC = 0,
		CAT_FOOD = 1,
		CAT_WATER = 2,
		CAT_MEDICAL = 3,
		CAT_TOOL = 4,
		CAT_MATERIAL = 5,
		CAT_GEAR = 6,
	};

private:
	StringName id;
	String display_name;
	String description;
	int category = CAT_MISC;
	int max_stack = 1;
	double weight = 0.1;
	int equip_slot = SLOT_NONE;

	// Combat
	int use_type = USE_OTHER;
	double damage = 0.0;
	double attack_range = 2.0; // meters from the eyes
	double attack_cooldown = 0.7; // seconds per swing (also the swing animation length)
	String attack_animation = "CharacterArmature|Sword_Slash";

	Ref<Texture2D> icon; // optional; falls back to res://assets/icons/<id>.png
	mutable Ref<Texture2D> cached_icon;

	Ref<PackedScene> model;
	double model_scale = 1.0;
	// Rotation of the model when lying in the world as a pickup (e.g. lay long tools on their side).
	Vector3 ground_rotation_degrees;
	// Pose of the model when held/worn, relative to the bone socket (third person).
	// Default fits tools modeled along +Y with the grip at the origin (axe, knife, shovel, torch).
	Vector3 hold_position = Vector3(0, 0.08, 0);
	Vector3 hold_rotation_degrees = Vector3(210, 90, 0);
	// Pose of the model in front of the first person camera.
	Vector3 view_position = Vector3(0.38, -0.52, -0.78);
	Vector3 view_rotation_degrees = Vector3(-10, -80, 20);

protected:
	static void _bind_methods();

public:
	static String get_category_name(int p_category);
	static String get_slot_name(int p_slot);
	static String get_use_type_name(int p_use_type);

	Node3D *instantiate_model() const; // scaled model, caller owns it
	Transform3D get_hold_transform() const;
	Transform3D get_view_transform() const;
	bool is_equippable() const { return equip_slot != SLOT_NONE; }

	void set_id(const StringName &p_value);
	StringName get_id() const;
	void set_display_name(const String &p_value);
	String get_display_name() const;
	void set_description(const String &p_value);
	String get_description() const;
	void set_category(int p_value);
	int get_category() const;
	void set_max_stack(int p_value);
	int get_max_stack() const;
	void set_weight(double p_value);
	double get_weight() const;
	void set_equip_slot(int p_value);
	int get_equip_slot() const;
	void set_use_type(int p_value);
	int get_use_type() const;
	void set_damage(double p_value);
	double get_damage() const;
	void set_attack_range(double p_value);
	double get_attack_range() const;
	void set_attack_cooldown(double p_value);
	double get_attack_cooldown() const;
	void set_attack_animation(const String &p_value);
	String get_attack_animation() const;
	void set_icon(const Ref<Texture2D> &p_value);
	Ref<Texture2D> get_icon() const;
	void set_model(const Ref<PackedScene> &p_value);
	Ref<PackedScene> get_model() const;
	void set_model_scale(double p_value);
	double get_model_scale() const;
	void set_ground_rotation_degrees(const Vector3 &p_value);
	Vector3 get_ground_rotation_degrees() const;
	void set_hold_position(const Vector3 &p_value);
	Vector3 get_hold_position() const;
	void set_hold_rotation_degrees(const Vector3 &p_value);
	Vector3 get_hold_rotation_degrees() const;
	void set_view_position(const Vector3 &p_value);
	Vector3 get_view_position() const;
	void set_view_rotation_degrees(const Vector3 &p_value);
	Vector3 get_view_rotation_degrees() const;
};

} // namespace godot
