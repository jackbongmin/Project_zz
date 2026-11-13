#include "pickup_item.h"

#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/classes/visual_instance3d.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace {
// Bounds of all visuals below p_node, in p_node's parent space.
bool merged_aabb(Node *p_node, const Transform3D &p_xf, AABB &r_out, bool p_first) {
	Transform3D xf = p_xf;
	Node3D *n3d = Object::cast_to<Node3D>(p_node);
	if (n3d) {
		xf = p_xf * n3d->get_transform();
	}
	VisualInstance3D *vis = Object::cast_to<VisualInstance3D>(p_node);
	if (vis) {
		const AABB box = xf.xform(vis->get_aabb());
		r_out = p_first ? box : r_out.merge(box);
		p_first = false;
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		p_first = merged_aabb(p_node->get_child(i), xf, r_out, p_first);
	}
	return p_first;
}
} // namespace

void PickupItem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_item", "item"), &PickupItem::set_item);
	ClassDB::bind_method(D_METHOD("get_item"), &PickupItem::get_item);
	ClassDB::bind_method(D_METHOD("set_count", "count"), &PickupItem::set_count);
	ClassDB::bind_method(D_METHOD("get_count"), &PickupItem::get_count);
	ClassDB::bind_method(D_METHOD("get_prompt"), &PickupItem::get_prompt);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "item", PROPERTY_HINT_RESOURCE_TYPE, "ItemData"), "set_item", "get_item");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "count", PROPERTY_HINT_RANGE, "1,999,1"), "set_count", "get_count");
}

void PickupItem::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		set_collision_layer(INTERACT_LAYER);
		set_collision_mask(0);
		set_monitoring(false);
		rebuild();
	}
}

void PickupItem::rebuild() {
	if (!is_inside_tree()) {
		return;
	}
	// Generated children are not owned, so they are never saved into the scene file.
	for (int i = get_child_count() - 1; i >= 0; i--) {
		Node *child = get_child(i);
		if (child->has_meta("pickup_generated")) {
			remove_child(child);
			child->queue_free();
		}
	}
	visual = nullptr;
	if (item.is_null()) {
		return;
	}

	visual = item->instantiate_model();
	if (!visual) {
		return;
	}
	visual->set_meta("pickup_generated", true);
	visual->set_rotation_degrees(item->get_ground_rotation_degrees());
	add_child(visual);

	// Rest the model on the pickup origin and size the interaction sphere to it.
	AABB box;
	merged_aabb(visual, Transform3D(), box, true);
	// merged_aabb started from the visual's own transform, so box is in this node's space.
	visual->set_position(visual->get_position() + Vector3(-box.get_center().x, -box.position.y, -box.get_center().z));

	Ref<SphereShape3D> sphere;
	sphere.instantiate();
	const double radius = MAX(0.3, box.size[box.get_longest_axis_index()] * 0.5);
	sphere->set_radius(radius);
	CollisionShape3D *shape = memnew(CollisionShape3D);
	shape->set_shape(sphere);
	shape->set_position(Vector3(0, box.size.y * 0.5, 0));
	shape->set_meta("pickup_generated", true);
	add_child(shape);
}

String PickupItem::get_prompt() const {
	if (item.is_null()) {
		return String();
	}
	String text = item->get_display_name();
	if (count > 1) {
		text += " x" + String::num_int64(count);
	}
	return text;
}

void PickupItem::set_item(const Ref<ItemData> &p_item) {
	item = p_item;
	rebuild();
}
Ref<ItemData> PickupItem::get_item() const { return item; }
void PickupItem::set_count(int p_count) { count = MAX(1, p_count); }
int PickupItem::get_count() const { return count; }
