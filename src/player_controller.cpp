#include "player_controller.h"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include <cmath>

#include "pickup_item.h"

using namespace godot;

namespace {
constexpr double PITCH_LIMIT = 1.45; // ~83 degrees
constexpr uint32_t WORLD_LAYER = 1u << 0;

double approach(double p_from, double p_to, double p_step) {
	if (Math::abs(p_to - p_from) <= p_step) {
		return p_to;
	}
	return p_from + (p_to > p_from ? p_step : -p_step);
}

double angle_lerp(double p_from, double p_to, double p_weight) {
	constexpr double TAU = 6.283185307179586;
	double diff = std::fmod(p_to - p_from, TAU);
	diff = std::fmod(2.0 * diff, TAU) - diff; // shortest signed difference
	return p_from + diff * p_weight;
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

void clear_children(Node *p_node) {
	if (!p_node) {
		return;
	}
	for (int i = p_node->get_child_count() - 1; i >= 0; i--) {
		Node *child = p_node->get_child(i);
		p_node->remove_child(child);
		child->queue_free();
	}
}
} // namespace

#define BIND_PROP_HINT(m_type, m_name, m_hint, m_hint_text)                                              \
	ClassDB::bind_method(D_METHOD("set_" #m_name, "value"), &PlayerController::set_##m_name);           \
	ClassDB::bind_method(D_METHOD("get_" #m_name), &PlayerController::get_##m_name);                    \
	ADD_PROPERTY(PropertyInfo(m_type, #m_name, m_hint, m_hint_text), "set_" #m_name, "get_" #m_name);
#define BIND_PROP(m_type, m_name) BIND_PROP_HINT(m_type, m_name, PROPERTY_HINT_NONE, "")

void PlayerController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("toggle_view"), &PlayerController::toggle_view);
	ClassDB::bind_method(D_METHOD("interact"), &PlayerController::interact);
	ClassDB::bind_method(D_METHOD("drop_from_slot", "slot"), &PlayerController::drop_from_slot);
	ClassDB::bind_method(D_METHOD("drop_equipped", "gear_slot"), &PlayerController::drop_equipped);
	ClassDB::bind_method(D_METHOD("spawn_pickup", "item", "count"), &PlayerController::spawn_pickup);
	ClassDB::bind_method(D_METHOD("set_input_locked", "locked"), &PlayerController::set_input_locked);
	ClassDB::bind_method(D_METHOD("is_input_locked"), &PlayerController::is_input_locked);
	ClassDB::bind_method(D_METHOD("get_interaction_prompt"), &PlayerController::get_interaction_prompt);
	ClassDB::bind_method(D_METHOD("get_inventory"), &PlayerController::get_inventory);
	ClassDB::bind_method(D_METHOD("get_current_animation"), &PlayerController::get_current_animation);
	ClassDB::bind_method(D_METHOD("get_body_yaw"), &PlayerController::get_body_yaw);

	BIND_PROP_HINT(Variant::INT, view_mode, PROPERTY_HINT_ENUM, "First Person,Third Person")
	ADD_GROUP("Movement", "");
	BIND_PROP_HINT(Variant::FLOAT, walk_speed, PROPERTY_HINT_RANGE, "0,20,0.1")
	BIND_PROP_HINT(Variant::FLOAT, sprint_speed, PROPERTY_HINT_RANGE, "0,30,0.1")
	BIND_PROP_HINT(Variant::FLOAT, jump_velocity, PROPERTY_HINT_RANGE, "0,20,0.1")
	BIND_PROP_HINT(Variant::FLOAT, acceleration, PROPERTY_HINT_RANGE, "0,100,0.5")
	BIND_PROP_HINT(Variant::FLOAT, turn_speed, PROPERTY_HINT_RANGE, "0,40,0.5")
	ADD_GROUP("Camera", "");
	BIND_PROP_HINT(Variant::FLOAT, mouse_sensitivity, PROPERTY_HINT_RANGE, "0.0001,0.02,0.0001")
	BIND_PROP_HINT(Variant::FLOAT, third_person_distance, PROPERTY_HINT_RANGE, "1,10,0.1")
	ADD_GROUP("Interaction", "");
	BIND_PROP_HINT(Variant::FLOAT, interact_range, PROPERTY_HINT_RANGE, "0.5,10,0.1")
	ADD_GROUP("Animation", "anim_");
	BIND_PROP(Variant::STRING, anim_idle)
	BIND_PROP(Variant::STRING, anim_walk)
	BIND_PROP(Variant::STRING, anim_run)
	BIND_PROP_HINT(Variant::FLOAT, anim_blend, PROPERTY_HINT_RANGE, "0,1,0.01")

	ADD_SIGNAL(MethodInfo("view_mode_changed", PropertyInfo(Variant::INT, "mode")));
	ADD_SIGNAL(MethodInfo("item_picked_up", PropertyInfo(Variant::STRING, "name"), PropertyInfo(Variant::INT, "count")));
	ADD_SIGNAL(MethodInfo("inventory_full", PropertyInfo(Variant::STRING, "name")));
}

#undef BIND_PROP
#undef BIND_PROP_HINT

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
	add_key_action(map, "interact", KEY_E);
	add_key_action(map, "inventory", KEY_TAB);
	const Key digits[Inventory::HOTBAR_SIZE] = { KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8 };
	for (int i = 0; i < Inventory::HOTBAR_SIZE; i++) {
		add_key_action(map, StringName("hotbar_" + String::num_int64(i + 1)), digits[i]);
	}
}

void PlayerController::_ready() {
	// GDExtension classes also run inside the editor; only drive gameplay at runtime.
	if (Engine::get_singleton()->is_editor_hint()) {
		set_process(false);
		set_physics_process(false);
		set_process_unhandled_input(false);
		return;
	}

	ensure_input_actions();
	add_to_group("player");
	gravity = double(ProjectSettings::get_singleton()->get_setting("physics/3d/default_gravity", 9.8));

	body = get_node<Node3D>("Body");
	head = get_node<Node3D>("Head");
	fp_camera = get_node<Camera3D>("Head/FirstPersonCamera");
	spring_arm = get_node<SpringArm3D>("Head/SpringArm3D");
	tp_camera = get_node<Camera3D>("Head/SpringArm3D/ThirdPersonCamera");
	inventory = get_node<Inventory>("Inventory");

	ERR_FAIL_NULL_MSG(head, "PlayerController needs a 'Head' Node3D child.");
	ERR_FAIL_NULL_MSG(fp_camera, "PlayerController needs 'Head/FirstPersonCamera'.");
	ERR_FAIL_NULL_MSG(spring_arm, "PlayerController needs 'Head/SpringArm3D'.");
	ERR_FAIL_NULL_MSG(tp_camera, "PlayerController needs 'Head/SpringArm3D/ThirdPersonCamera'.");
	ERR_FAIL_NULL_MSG(inventory, "PlayerController needs an 'Inventory' child.");

	// Keep the spring arm from colliding with our own capsule.
	spring_arm->add_excluded_object(get_rid());
	spring_arm->set_length(third_person_distance);

	setup_sockets();
	inventory->connect("inventory_changed", callable_mp(this, &PlayerController::refresh_equipment_visuals));
	inventory->connect("hotbar_selected", callable_mp(this, &PlayerController::on_hotbar_selected));

	Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
	apply_view_mode();
	setup_animation();
}

// --- input ------------------------------------------------------------------

void PlayerController::_unhandled_input(const Ref<InputEvent> &p_event) {
	if (input_locked) {
		return;
	}
	Input *input = Input::get_singleton();
	const bool captured = input->get_mouse_mode() == Input::MOUSE_MODE_CAPTURED;

	const InputEventMouseMotion *motion = Object::cast_to<InputEventMouseMotion>(p_event.ptr());
	if (motion && captured) {
		const Vector2 rel = motion->get_relative();
		rotate_y(-rel.x * mouse_sensitivity);
		// Keep the third-person body pointing the same way in the world while the camera turns.
		body_yaw += rel.x * mouse_sensitivity;
		pitch = Math::clamp(pitch - rel.y * mouse_sensitivity, -PITCH_LIMIT, PITCH_LIMIT);
		if (head) {
			head->set_rotation(Vector3(pitch, 0.0, 0.0));
		}
		return;
	}

	const InputEventMouseButton *button = Object::cast_to<InputEventMouseButton>(p_event.ptr());
	if (button && button->is_pressed()) {
		if (!captured) {
			input->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
			return;
		}
		if (inventory && button->get_button_index() == MOUSE_BUTTON_WHEEL_UP) {
			inventory->select_hotbar(inventory->get_selected_hotbar() - 1);
			return;
		}
		if (inventory && button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN) {
			inventory->select_hotbar(inventory->get_selected_hotbar() + 1);
			return;
		}
	}

	if (p_event->is_action_pressed("release_mouse")) {
		input->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
	} else if (p_event->is_action_pressed("toggle_view")) {
		toggle_view();
	} else if (p_event->is_action_pressed("interact")) {
		interact();
	} else if (inventory) {
		for (int i = 0; i < Inventory::HOTBAR_SIZE; i++) {
			if (p_event->is_action_pressed(StringName("hotbar_" + String::num_int64(i + 1)))) {
				inventory->select_hotbar(i);
				break;
			}
		}
	}
}

void PlayerController::set_input_locked(bool p_locked) {
	input_locked = p_locked;
	Input::get_singleton()->set_mouse_mode(p_locked ? Input::MOUSE_MODE_VISIBLE : Input::MOUSE_MODE_CAPTURED);
	if (p_locked) {
		focused_pickup = ObjectID();
		interaction_prompt = String();
	}
}

bool PlayerController::is_input_locked() const { return input_locked; }

// --- movement ---------------------------------------------------------------

void PlayerController::_physics_process(double p_delta) {
	Input *input = Input::get_singleton();
	Vector3 velocity = get_velocity();

	if (!is_on_floor()) {
		velocity.y -= gravity * p_delta;
	} else if (!input_locked && input->is_action_just_pressed("jump")) {
		velocity.y = jump_velocity;
	}

	Vector2 move;
	bool sprinting = false;
	if (!input_locked) {
		move = input->get_vector("move_left", "move_right", "move_forward", "move_back");
		sprinting = input->is_action_pressed("sprint");
	}
	const Vector3 local_move(move.x, 0.0, move.y);
	Vector3 direction = get_global_transform().basis.xform(local_move);
	direction.y = 0.0;
	if (direction.length_squared() > 0.0) {
		direction = direction.normalized();
	}

	const double speed = sprinting ? sprint_speed : walk_speed;
	const double step = acceleration * speed * p_delta;
	velocity.x = approach(velocity.x, direction.x * speed, step);
	velocity.z = approach(velocity.z, direction.z * speed, step);

	set_velocity(velocity);
	move_and_slide();

	update_body_facing(local_move, p_delta);
	update_animation(sprinting);
	if (!input_locked) {
		update_interaction();
	}
}

void PlayerController::update_body_facing(const Vector3 &p_local_move, double p_delta) {
	if (!body) {
		return;
	}
	if (view_mode == VIEW_FIRST_PERSON) {
		body_yaw = 0.0; // first person: body always faces where we look
	} else if (p_local_move.length_squared() > 0.01) {
		// Model forward is local -Z; turn it toward the input direction (relative to the camera yaw).
		const double target = std::atan2(-p_local_move.x, -p_local_move.z);
		body_yaw = angle_lerp(body_yaw, target, 1.0 - std::exp(-turn_speed * p_delta));
	}
	body->set_rotation(Vector3(0.0, body_yaw, 0.0));
}

// --- interaction ------------------------------------------------------------

Camera3D *PlayerController::get_active_camera() const {
	return view_mode == VIEW_FIRST_PERSON ? fp_camera : tp_camera;
}

PickupItem *PlayerController::get_focused_pickup() const {
	if (focused_pickup.is_null()) {
		return nullptr;
	}
	return Object::cast_to<PickupItem>(ObjectDB::get_instance(focused_pickup));
}

void PlayerController::update_interaction() {
	Camera3D *cam = get_active_camera();
	if (!cam || !head) {
		return;
	}
	const Vector3 from = cam->get_global_position();
	const Vector3 forward = -cam->get_global_transform().basis.get_column(2).normalized();
	const double camera_offset = from.distance_to(head->get_global_position());
	const Vector3 to = from + forward * (interact_range + camera_offset);

	TypedArray<RID> exclude;
	exclude.push_back(get_rid());
	Ref<PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(from, to, WORLD_LAYER | PickupItem::INTERACT_LAYER, exclude);
	query->set_collide_with_areas(true);
	const Dictionary hit = get_world_3d()->get_direct_space_state()->intersect_ray(query);

	PickupItem *pickup = nullptr;
	if (!hit.is_empty()) {
		Object *collider = hit["collider"];
		pickup = Object::cast_to<PickupItem>(collider);
		if (pickup && head->get_global_position().distance_to(Vector3(hit["position"])) > interact_range + 0.5) {
			pickup = nullptr;
		}
	}
	focused_pickup = pickup ? ObjectID(pickup->get_instance_id()) : ObjectID();
	interaction_prompt = pickup ? pickup->get_prompt() : String();
}

void PlayerController::interact() {
	PickupItem *pickup = get_focused_pickup();
	if (!pickup || !inventory || pickup->get_item().is_null()) {
		return;
	}
	const Ref<ItemData> item = pickup->get_item();
	const int count = pickup->get_count();
	const int left = inventory->add_item(item, count);
	if (left < count) {
		emit_signal("item_picked_up", item->get_display_name(), count - left);
	}
	if (left == 0) {
		pickup->queue_free();
		focused_pickup = ObjectID();
		interaction_prompt = String();
	} else {
		pickup->set_count(left);
		emit_signal("inventory_full", item->get_display_name());
	}
}

void PlayerController::drop_from_slot(int p_slot) {
	if (!inventory) {
		return;
	}
	const Ref<ItemData> item = inventory->get_item(p_slot);
	const int count = inventory->get_count(p_slot);
	if (item.is_null() || count <= 0) {
		return;
	}
	inventory->remove_from_slot(p_slot, count);
	spawn_pickup(item, count);
}

void PlayerController::drop_equipped(int p_gear_slot) {
	if (!inventory) {
		return;
	}
	const Ref<ItemData> item = inventory->take_equipped(p_gear_slot);
	if (item.is_valid()) {
		spawn_pickup(item, 1);
	}
}

PickupItem *PlayerController::spawn_pickup(const Ref<ItemData> &p_item, int p_count) {
	Node *parent = get_parent();
	ERR_FAIL_NULL_V(parent, nullptr);
	PickupItem *pickup = memnew(PickupItem);
	pickup->set_item(p_item);
	pickup->set_count(p_count);

	// In front of the player, snapped down onto whatever is below.
	const Vector3 forward = -get_global_transform().basis.get_column(2).normalized();
	Vector3 pos = get_global_position() + forward * 1.1 + Vector3(0, 1.0, 0);
	TypedArray<RID> exclude;
	exclude.push_back(get_rid());
	Ref<PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(pos, pos + Vector3(0, -4, 0), WORLD_LAYER, exclude);
	const Dictionary hit = get_world_3d()->get_direct_space_state()->intersect_ray(query);
	if (!hit.is_empty()) {
		pos = hit["position"];
	}
	parent->add_child(pickup);
	pickup->set_global_position(pos);
	pickup->set_rotation(Vector3(0, get_rotation().y, 0));
	return pickup;
}

String PlayerController::get_interaction_prompt() const { return interaction_prompt; }
Inventory *PlayerController::get_inventory() const { return inventory; }

// --- equipment visuals ------------------------------------------------------

void PlayerController::setup_sockets() {
	if (!body) {
		return;
	}
	TypedArray<Node> skeletons = body->find_children("*", "Skeleton3D", true, false);
	if (!skeletons.is_empty()) {
		skeleton = Object::cast_to<Skeleton3D>(skeletons[0]);
		hand_bone = skeleton->find_bone(hand_bone_name);
		back_bone = skeleton->find_bone(back_bone_name);
	}
	hand_socket = memnew(Node3D);
	hand_socket->set_name("HandSocket");
	body->add_child(hand_socket);
	back_socket = memnew(Node3D);
	back_socket->set_name("BackSocket");
	body->add_child(back_socket);
	view_socket = memnew(Node3D);
	view_socket->set_name("ViewSocket");
	fp_camera->add_child(view_socket);
}

void PlayerController::_process(double p_delta) {
	update_sockets();
}

void PlayerController::update_sockets() {
	if (!skeleton) {
		return;
	}
	const Transform3D skel_xf = skeleton->get_global_transform();
	if (hand_bone >= 0 && hand_socket) {
		hand_socket->set_global_transform((skel_xf * skeleton->get_bone_global_pose(hand_bone)).orthonormalized());
	}
	if (back_bone >= 0 && back_socket) {
		back_socket->set_global_transform((skel_xf * skeleton->get_bone_global_pose(back_bone)).orthonormalized());
	}
}

void PlayerController::on_hotbar_selected(int p_index) {
	refresh_equipment_visuals();
}

void PlayerController::refresh_equipment_visuals() {
	if (!inventory) {
		return;
	}
	const Ref<ItemData> hand_item = inventory->get_held_item();
	if (hand_item != shown_hand_item) {
		shown_hand_item = hand_item;
		clear_children(hand_socket);
		clear_children(view_socket);
		if (hand_item.is_valid()) {
			Node3D *held = hand_item->instantiate_model();
			if (held) {
				Node3D *pivot = memnew(Node3D);
				pivot->set_transform(hand_item->get_hold_transform());
				pivot->add_child(held);
				hand_socket->add_child(pivot);
			}
			Node3D *viewmodel = hand_item->instantiate_model();
			if (viewmodel) {
				Node3D *pivot = memnew(Node3D);
				pivot->set_transform(hand_item->get_view_transform());
				pivot->add_child(viewmodel);
				view_socket->add_child(pivot);
			}
		}
	}

	const Ref<ItemData> back_item = inventory->get_equipped(ItemData::SLOT_BACK);
	if (back_item != shown_back_item) {
		shown_back_item = back_item;
		clear_children(back_socket);
		if (back_item.is_valid()) {
			Node3D *worn = back_item->instantiate_model();
			if (worn) {
				Node3D *pivot = memnew(Node3D);
				pivot->set_transform(back_item->get_hold_transform());
				pivot->add_child(worn);
				back_socket->add_child(pivot);
			}
		}
	}
	apply_view_visibility(); // new meshes need the shadow-only / viewmodel state of the current view
}

// --- view mode ----------------------------------------------------------------

void PlayerController::toggle_view() {
	set_view_mode(view_mode == VIEW_FIRST_PERSON ? VIEW_THIRD_PERSON : VIEW_FIRST_PERSON);
}

void PlayerController::apply_view_mode() {
	if (!fp_camera || !tp_camera) {
		return;
	}
	if (view_mode == VIEW_FIRST_PERSON) {
		fp_camera->make_current();
	} else {
		tp_camera->make_current();
	}
	apply_view_visibility();
}

void PlayerController::apply_view_visibility() {
	const bool first = view_mode == VIEW_FIRST_PERSON;
	if (body) {
		set_body_shadow_only(body, first);
	}
	if (view_socket) {
		view_socket->set_visible(first);
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

// --- animation ----------------------------------------------------------------

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

// --- properties -----------------------------------------------------------------

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
void PlayerController::set_turn_speed(double p_value) { turn_speed = p_value; }
double PlayerController::get_turn_speed() const { return turn_speed; }
void PlayerController::set_mouse_sensitivity(double p_value) { mouse_sensitivity = p_value; }
double PlayerController::get_mouse_sensitivity() const { return mouse_sensitivity; }

void PlayerController::set_third_person_distance(double p_value) {
	third_person_distance = p_value;
	if (spring_arm) {
		spring_arm->set_length(third_person_distance);
	}
}
double PlayerController::get_third_person_distance() const { return third_person_distance; }
void PlayerController::set_interact_range(double p_value) { interact_range = p_value; }
double PlayerController::get_interact_range() const { return interact_range; }

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
double PlayerController::get_body_yaw() const { return body_yaw; }
