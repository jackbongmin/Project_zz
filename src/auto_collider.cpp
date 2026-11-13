#include "auto_collider.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void AutoCollider::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &AutoCollider::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &AutoCollider::get_mode);
	ClassDB::bind_method(D_METHOD("get_built_count"), &AutoCollider::get_built_count);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Trimesh (exact),Convex (fast)"), "set_mode", "get_mode");
}

void AutoCollider::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	build(this);
}

void AutoCollider::build(Node *p_node) {
	// Snapshot the child count first: creating collision adds a StaticBody3D child.
	const int child_count = p_node->get_child_count();

	MeshInstance3D *mesh = Object::cast_to<MeshInstance3D>(p_node);
	if (mesh && mesh->get_mesh().is_valid()) {
		if (mode == MODE_CONVEX) {
			mesh->create_convex_collision(true, true);
		} else {
			mesh->create_trimesh_collision();
		}
		built_count++;
	}

	for (int i = 0; i < child_count; i++) {
		build(p_node->get_child(i));
	}
}

void AutoCollider::set_mode(int p_mode) { mode = p_mode == MODE_CONVEX ? MODE_CONVEX : MODE_TRIMESH; }
int AutoCollider::get_mode() const { return mode; }
int AutoCollider::get_built_count() const { return built_count; }
