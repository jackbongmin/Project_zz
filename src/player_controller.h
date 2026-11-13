#pragma once

#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/spring_arm3d.hpp>

#include "inventory.h"

namespace godot {

class PickupItem;

// First/third person player controller.
// Expected children (see scenes/player/player.tscn):
//   Body                              - visual model (shadow only in first person, turns toward movement)
//   Head                              - pitch pivot at eye height
//   Head/FirstPersonCamera
//   Head/SpringArm3D/ThirdPersonCamera
//   Inventory
class PlayerController : public CharacterBody3D {
	GDCLASS(PlayerController, CharacterBody3D)

public:
	enum ViewMode {
		VIEW_FIRST_PERSON = 0,
		VIEW_THIRD_PERSON = 1,
	};

private:
	// Tunables (exposed to the inspector)
	double walk_speed = 4.0;
	double sprint_speed = 7.0;
	double jump_velocity = 4.5;
	double acceleration = 12.0;
	double turn_speed = 12.0;
	double mouse_sensitivity = 0.0025;
	double third_person_distance = 3.5;
	double interact_range = 2.5;
	int view_mode = VIEW_FIRST_PERSON;

	// Animation clip names inside the Body model's AnimationPlayer
	String anim_idle = "CharacterArmature|Idle";
	String anim_walk = "CharacterArmature|Walk";
	String anim_run = "CharacterArmature|Run";
	double anim_blend = 0.15;
	String hand_bone_name = "Wrist.R";
	String back_bone_name = "Chest";

	// Runtime state
	AnimationPlayer *anim_player = nullptr;
	double pitch = 0.0;
	double body_yaw = 0.0;
	double gravity = 9.8;
	bool input_locked = false;
	ObjectID focused_pickup;
	String interaction_prompt;

	Node3D *body = nullptr;
	Node3D *head = nullptr;
	Camera3D *fp_camera = nullptr;
	SpringArm3D *spring_arm = nullptr;
	Camera3D *tp_camera = nullptr;
	Inventory *inventory = nullptr;

	Skeleton3D *skeleton = nullptr;
	int hand_bone = -1;
	int back_bone = -1;
	Node3D *hand_socket = nullptr;
	Node3D *back_socket = nullptr;
	Node3D *view_socket = nullptr;
	Ref<ItemData> shown_hand_item;
	Ref<ItemData> shown_back_item;

	static void ensure_input_actions();
	void apply_view_mode();
	void apply_view_visibility();
	void set_body_shadow_only(Node *p_node, bool p_shadow_only);
	void setup_animation();
	void setup_sockets();
	void update_animation(bool p_sprinting);
	void update_body_facing(const Vector3 &p_local_move, double p_delta);
	void update_interaction();
	void update_sockets();
	void refresh_equipment_visuals();
	void on_hotbar_selected(int p_index);
	PickupItem *get_focused_pickup() const;
	Camera3D *get_active_camera() const;

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _physics_process(double p_delta) override;
	void _unhandled_input(const Ref<InputEvent> &p_event) override;

	void toggle_view();
	void interact();
	void drop_from_slot(int p_slot);
	void drop_equipped(int p_gear_slot);
	PickupItem *spawn_pickup(const Ref<ItemData> &p_item, int p_count);

	void set_input_locked(bool p_locked);
	bool is_input_locked() const;
	String get_interaction_prompt() const;
	Inventory *get_inventory() const;

	void set_view_mode(int p_mode);
	int get_view_mode() const;
	void set_walk_speed(double p_value);
	double get_walk_speed() const;
	void set_sprint_speed(double p_value);
	double get_sprint_speed() const;
	void set_jump_velocity(double p_value);
	double get_jump_velocity() const;
	void set_acceleration(double p_value);
	double get_acceleration() const;
	void set_turn_speed(double p_value);
	double get_turn_speed() const;
	void set_mouse_sensitivity(double p_value);
	double get_mouse_sensitivity() const;
	void set_third_person_distance(double p_value);
	double get_third_person_distance() const;
	void set_interact_range(double p_value);
	double get_interact_range() const;
	void set_anim_idle(const String &p_value);
	String get_anim_idle() const;
	void set_anim_walk(const String &p_value);
	String get_anim_walk() const;
	void set_anim_run(const String &p_value);
	String get_anim_run() const;
	void set_anim_blend(double p_value);
	double get_anim_blend() const;
	String get_current_animation() const;
	double get_body_yaw() const;
};

} // namespace godot
