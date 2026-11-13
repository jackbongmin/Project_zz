#include "player_controller.h"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace {
constexpr double PITCH_LIMIT = 1.45; // ~83 degrees

double approach(double p_from, double p_to, double p_step) {
	if (Math::abs(p_to - p_from) <= p_step) {
		return p_to;
	}
	return p_from + (p_to > p_from ? p_step : -p_step);
}

void add_key_action(InputMap *p_map, const StringName &p_action, Key p_key) {
	if (p_map->has_action(p_action)) {
		return; // already defined in project settings - respect user rebinding
	}
	p_map->add_action(p_action);
	Ref<InputEventKey> ev;
	ev.instantiate();
	ev->set_physical_keycode(p_key);
	p_map->action_add_event(p_action, ev);
}
} // namespace

void PlayerController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("toggle_view"), &PlayerController::toggle_view);

	ClassDB::bind_method(D_METHOD("set_view_mode", "mode"), &PlayerController::set_view_mode);
	ClassDB::bind_method(D_METHOD("get_view_mode"), &PlayerController::get_view_mode);
	ClassDB::bind_method(D_METHOD("set_walk_speed", "value"), &PlayerController::set_walk_speed);
	ClassDB::bind_method(D_METHOD("get_walk_speed"), &PlayerController::get_walk_speed);
	ClassDB::bind_method(D_METHOD("set_sprint_speed", "value"), &PlayerController::set_sprint_speed);
	ClassDB::bind_method(D_METHOD("get_sprint_speed"), &PlayerController::get_sprint_speed);
	ClassDB::bind_method(D_METHOD("set_jump_velocity", "value"), &PlayerController::set_jump_velocity);
	ClassDB::bind_method(D_METHOD("get_jump_velocity"), &PlayerController::get_jump_velocity);
	ClassDB::bind_method(D_METHOD("set_acceleration", "value"), &PlayerController::set_acceleration);
	ClassDB::bind_method(D_METHOD("get_acceleration"), &PlayerController::get_acceleration);
	ClassDB::bind_method(D_METHOD("set_mouse_sensitivity", "value"), &PlayerController::set_mouse_sensitivity);
	ClassDB::bind_method(D_METHOD("get_mouse_sensitivity"), &PlayerController::get_mouse_sensitivity);
	ClassDB::bind_method(D_METHOD("set_third_person_distance", "value"), &PlayerController::set_third_person_distance);
	ClassDB::bind_method(D_METHOD("get_third_person_distance"), &PlayerController::get_third_person_distance);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "view_mode", PROPERTY_HINT_ENUM, "First Person,Third Person"), "set_view_mode", "get_view_mode");
	ADD_GROUP("Movement", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "walk_speed", PROPERTY_HINT_RANGE, "0,20,0.1"), "set_walk_speed", "get_walk_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sprint_speed", PROPERTY_HINT_RANGE, "0,30,0.1"), "set_sprint_speed", "get_sprint_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "jump_velocity", PROPERTY_HINT_RANGE, "0,20,0.1"), "set_jump_velocity", "get_jump_velocity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "acceleration", PROPERTY_HINT_RANGE, "0,100,0.5"), "set_acceleration", "get_acceleration");
	ADD_GROUP("Camera", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mouse_sensitivity", PROPERTY_HINT_RANGE, "0.0001,0.02,0.0001"), "set_mouse_sensitivity", "get_mouse_sensitivity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "third_person_distance", PROPERTY_HINT_RANGE, "1,10,0.1"), "set_third_person_distance", "get_third_person_distance");

	ClassDB::bind_method(D_METHOD("set_anim_idle", "value"), &PlayerController::set_anim_idle);
	ClassDB::bind_method(D_METHOD("get_anim_idle"), &PlayerController::get_anim_idle);
	ClassDB::bind_method(D_METHOD("set_anim_walk", "value"), &PlayerController::set_anim_walk);
	ClassDB::bind_method(D_METHOD("get_anim_walk"), &PlayerController::get_anim_walk);
	ClassDB::bind_method(D_METHOD("set_anim_run", "value"), &PlayerController::set_anim_run);
	ClassDB::bind_method(D_METHOD("get_anim_run"), &PlayerController::get_anim_run);
	ClassDB::bind_method(D_METHOD("set_anim_blend", "value"), &PlayerController::set_anim_blend);
	ClassDB::bind_method(D_METHOD("get_anim_blend"), &PlayerController::get_anim_blend);
	ClassDB::bind_method(D_METHOD("get_current_animation"), &PlayerController::get_current_animation);

	ADD_GROUP("Animation", "anim_");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "anim_idle"), "set_anim_idle", "get_anim_idle");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "anim_walk"), "set_anim_walk", "get_anim_walk");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "anim_run"), "set_anim_run", "get_anim_run");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "anim_blend", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_anim_blend", "get_anim_blend");

	ADD_SIGNAL(MethodInfo("view_mode_changed", PropertyInfo(Variant::INT, "mode")));
}

void PlayerController::ensure_input_actions() {
	InputMap *map = InputMap::get_singleton();
	add_key_action(map, "move_forward", KEY_W);
	add_key_action(map, "move_back", KEY_S);
	add_key_action(map, "move_left", KEY_A);
	add_key_action(map, "move_right", KEY_D);
	add_key_action(map, "jump", KEY_SPACE);
	add_key_action(map, "sprint", KEY_SHIFT);
	add_key_action(map, "toggle_view", KEY_V);
	add_key_action(map, "release_mouse", KEY_ESCAPE);
}

void PlayerController::_ready() {
	// GDExtension classes also run inside the editor; only drive gameplay at runtime.
	if (Engine::get_singleton()->is_editor_hint()) {
		set_physics_process(false);
		set_process_unhandled_input(false);
		return;
	}

	ensure_input_actions();
	gravity = double(ProjectSettings::get_singleton()->get_setting("physics/3d/default_gravity", 9.8));

	body = get_node<Node3D>("Body");
	head = get_node<Node3D>("Head");
	fp_camera = get_node<Camera3D>("Head/FirstPersonCamera");
	spring_arm = get_node<SpringArm3D>("Head/SpringArm3D");
	tp_camera = get_node<Camera3D>("Head/SpringArm3D/ThirdPersonCamera");

	ERR_FAIL_NULL_MSG(head, "PlayerController needs a 'Head' Node3D child.");
	ERR_FAIL_NULL_MSG(fp_camera, "PlayerController needs 'Head/FirstPersonCamera'.");
	ERR_FAIL_NULL_MSG(spring_arm, "PlayerController needs 'Head/SpringArm3D'.");
	ERR_FAIL_NULL_MSG(tp_camera, "PlayerController needs 'Head/SpringArm3D/ThirdPersonCamera'.");

	// Keep the spring arm from colliding with our own capsule.
	spring_arm->add_excluded_object(get_rid());
	spring_arm->set_length(third_person_distance);

	Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
	apply_view_mode();
	setup_animation();
}

void PlayerController::setup_animation() {
	if (!body) {
		return;
	}
	TypedArray<Node> found = body->find_children("*", "AnimationPlayer", true, false);
	if (found.is_empty()) {
		return; // placeholder body without animations is fine
	}
	anim_player = Object::cast_to<AnimationPlayer>(found[0]);
	if (!anim_player) {
		return;
	}
	// glTF clips import as one-shot; locomotion clips must loop.
	const String looping[] = { anim_idle, anim_walk, anim_run };
	for (const String &name : looping) {
		if (anim_player->has_animation(name)) {
			Ref<Animation> clip = anim_player->get_animation(name);
			clip->set_loop_mode(Animation::LOOP_LINEAR);
		} else {
			WARN_PRINT(String("PlayerController: animation '") + name + "' not found in Body model.");
		}
	}
	if (anim_player->has_animation(anim_idle)) {
		anim_player->play(anim_idle);
	}
}

void PlayerController::update_animation(bool p_sprinting) {
	if (!anim_player || !is_on_floor()) {
		return; // keep the current clip while airborne
	}
	const Vector3 v = get_velocity();
	const double horizontal_speed = Vector2(v.x, v.z).length();

	String target = anim_idle;
	if (horizontal_speed > 0.2) {
		target = (p_sprinting && horizontal_speed > walk_speed + 0.3) ? anim_run : anim_walk;
	}
	if (anim_player->get_current_animation() != target && anim_player->has_animation(target)) {
		anim_player->play(target, anim_blend);
	}
}

void PlayerController::_unhandled_input(const Ref<InputEvent> &p_event) {
	Input *input = Input::get_singleton();

	const InputEventMouseMotion *motion = Object::cast_to<InputEventMouseMotion>(p_event.ptr());
	if (motion && input->get_mouse_mode() == Input::MOUSE_MODE_CAPTURED) {
		const Vector2 rel = motion->get_relative();
		rotate_y(-rel.x * mouse_sensitivity);
		pitch = Math::clamp(pitch - rel.y * mouse_sensitivity, -PITCH_LIMIT, PITCH_LIMIT);
		if (head) {
			head->set_rotation(Vector3(pitch, 0.0, 0.0));
		}
		return;
	}

	const InputEventMouseButton *button = Object::cast_to<InputEventMouseButton>(p_event.ptr());
	if (button && button->is_pressed() && input->get_mouse_mode() != Input::MOUSE_MODE_CAPTURED) {
		input->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
		return;
	}

	if (p_event->is_action_pressed("release_mouse")) {
		input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
	} else if (p_event->is_action_pressed("toggle_view")) {
		toggle_view();
	}
}

void PlayerController::_physics_process(double p_delta) {
	Input *input = Input::get_singleton();
	Vector3 velocity = get_velocity();

	if (!is_on_floor()) {
		velocity.y -= gravity * p_delta;
	} else if (input->is_action_just_pressed("jump")) {
		velocity.y = jump_velocity;
	}

	const Vector2 move = input->get_vector("move_left", "move_right", "move_forward", "move_back");
	Vector3 direction = get_global_transform().basis.xform(Vector3(move.x, 0.0, move.y));
	direction.y = 0.0;
	if (direction.length_squared() > 0.0) {
		direction = direction.normalized();
	}

	const bool sprinting = input->is_action_pressed("sprint");
	const double speed = sprinting ? sprint_speed : walk_speed;
	const double step = acceleration * speed * p_delta;
	velocity.x = approach(velocity.x, direction.x * speed, step);
	velocity.z = approach(velocity.z, direction.z * speed, step);

	set_velocity(velocity);
	move_and_slide();
	update_animation(sprinting);
}

void PlayerController::toggle_view() {
	set_view_mode(view_mode == VIEW_FIRST_PERSON ? VIEW_THIRD_PERSON : VIEW_FIRST_PERSON);
}

void PlayerController::apply_view_mode() {
	if (!fp_camera || !tp_camera) {
		return;
	}
	const bool first = view_mode == VIEW_FIRST_PERSON;
	if (first) {
		fp_camera->make_current();
	} else {
		tp_camera->make_current();
	}
	if (body) {
		set_body_shadow_only(body, first);
	}
}

void PlayerController::set_body_shadow_only(Node *p_node, bool p_shadow_only) {
	GeometryInstance3D *geo = Object::cast_to<GeometryInstance3D>(p_node);
	if (geo) {
		geo->set_cast_shadows_setting(p_shadow_only
						? GeometryInstance3D::SHADOW_CASTING_SETTING_SHADOWS_ONLY
						: GeometryInstance3D::SHADOW_CASTING_SETTING_ON);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		set_body_shadow_only(p_node->get_child(i), p_shadow_only);
	}
}

// --- properties -----------------------------------------------------------

void PlayerController::set_view_mode(int p_mode) {
	const int clamped = p_mode == VIEW_THIRD_PERSON ? VIEW_THIRD_PERSON : VIEW_FIRST_PERSON;
	if (clamped == view_mode) {
		return;
	}
	view_mode = clamped;
	if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		apply_view_mode();
		emit_signal("view_mode_changed", view_mode);
	}
}
int PlayerController::get_view_mode() const { return view_mode; }

void PlayerController::set_walk_speed(double p_value) { walk_speed = p_value; }
double PlayerController::get_walk_speed() const { return walk_speed; }
void PlayerController::set_sprint_speed(double p_value) { sprint_speed = p_value; }
double PlayerController::get_sprint_speed() const { return sprint_speed; }
void PlayerController::set_jump_velocity(double p_value) { jump_velocity = p_value; }
double PlayerController::get_jump_velocity() const { return jump_velocity; }
void PlayerController::set_acceleration(double p_value) { acceleration = p_value; }
double PlayerController::get_acceleration() const { return acceleration; }
void PlayerController::set_mouse_sensitivity(double p_value) { mouse_sensitivity = p_value; }
double PlayerController::get_mouse_sensitivity() const { return mouse_sensitivity; }

void PlayerController::set_third_person_distance(double p_value) {
	third_person_distance = p_value;
	if (spring_arm) {
		spring_arm->set_length(third_person_distance);
	}
}
double PlayerController::get_third_person_distance() const { return third_person_distance; }

void PlayerController::set_anim_idle(const String &p_value) { anim_idle = p_value; }
String PlayerController::get_anim_idle() const { return anim_idle; }
void PlayerController::set_anim_walk(const String &p_value) { anim_walk = p_value; }
String PlayerController::get_anim_walk() const { return anim_walk; }
void PlayerController::set_anim_run(const String &p_value) { anim_run = p_value; }
String PlayerController::get_anim_run() const { return anim_run; }
void PlayerController::set_anim_blend(double p_value) { anim_blend = p_value; }
double PlayerController::get_anim_blend() const { return anim_blend; }

String PlayerController::get_current_animation() const {
	return anim_player ? String(anim_player->get_current_animation()) : String();
}
