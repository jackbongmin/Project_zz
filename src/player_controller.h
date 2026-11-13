#pragma once

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/spring_arm3d.hpp>

namespace godot {

// First/third person player controller.
// Expected children (see scenes/player/player.tscn):
//   Body                              - visual model (hidden from camera in first person, keeps shadow)
//   Head                              - pitch pivot at eye height
//   Head/FirstPersonCamera
//   Head/SpringArm3D/ThirdPersonCamera
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
	double mouse_sensitivity = 0.0025;
	double third_person_distance = 3.5;
	int view_mode = VIEW_FIRST_PERSON;

	// Runtime state
	double pitch = 0.0;
	double gravity = 9.8;

	Node3D *body = nullptr;
	Node3D *head = nullptr;
	Camera3D *fp_camera = nullptr;
	SpringArm3D *spring_arm = nullptr;
	Camera3D *tp_camera = nullptr;

	static void ensure_input_actions();
	void apply_view_mode();
	void set_body_shadow_only(Node *p_node, bool p_shadow_only);

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _physics_process(double p_delta) override;
	void _unhandled_input(const Ref<InputEvent> &p_event) override;

	void toggle_view();

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
	void set_mouse_sensitivity(double p_value);
	double get_mouse_sensitivity() const;
	void set_third_person_distance(double p_value);
	double get_third_person_distance() const;
};

} // namespace godot
