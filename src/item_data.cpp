#include "item_data.h"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

namespace {
Basis basis_from_degrees(const Vector3 &p_deg) {
	return Basis::from_euler(Vector3(Math::deg_to_rad(p_deg.x), Math::deg_to_rad(p_deg.y), Math::deg_to_rad(p_deg.z)));
}
} // namespace

#define BIND_PROP_HINT(m_type, m_name, m_hint, m_hint_text)                                         \
	ClassDB::bind_method(D_METHOD("set_" #m_name, "value"), &ItemData::set_##m_name);                \
	ClassDB::bind_method(D_METHOD("get_" #m_name), &ItemData::get_##m_name);                         \
	ADD_PROPERTY(PropertyInfo(m_type, #m_name, m_hint, m_hint_text), "set_" #m_name, "get_" #m_name);
#define BIND_PROP(m_type, m_name) BIND_PROP_HINT(m_type, m_name, PROPERTY_HINT_NONE, "")

void ItemData::_bind_methods() {
	BIND_PROP(Variant::STRING_NAME, id)
	BIND_PROP(Variant::STRING, display_name)
	BIND_PROP_HINT(Variant::STRING, description, PROPERTY_HINT_MULTILINE_TEXT, "")
	BIND_PROP_HINT(Variant::INT, category, PROPERTY_HINT_ENUM, "Misc,Food,Water,Medical,Tool,Material,Gear")
	BIND_PROP_HINT(Variant::INT, max_stack, PROPERTY_HINT_RANGE, "1,999,1")
	BIND_PROP_HINT(Variant::FLOAT, weight, PROPERTY_HINT_RANGE, "0,100,0.01")
	BIND_PROP_HINT(Variant::INT, equip_slot, PROPERTY_HINT_ENUM, "None,Hand,Head,Body,Back")
	BIND_PROP_HINT(Variant::OBJECT, icon, PROPERTY_HINT_RESOURCE_TYPE, "Texture2D")
	ADD_GROUP("Combat", "");
	BIND_PROP_HINT(Variant::INT, use_type, PROPERTY_HINT_ENUM, "Other,Melee,Ranged")
	BIND_PROP_HINT(Variant::FLOAT, damage, PROPERTY_HINT_RANGE, "0,500,0.5")
	BIND_PROP_HINT(Variant::FLOAT, attack_range, PROPERTY_HINT_RANGE, "0.5,100,0.1")
	BIND_PROP_HINT(Variant::FLOAT, attack_cooldown, PROPERTY_HINT_RANGE, "0.1,5,0.05")
	BIND_PROP(Variant::STRING, attack_animation)
	ADD_GROUP("Model", "");
	BIND_PROP_HINT(Variant::OBJECT, model, PROPERTY_HINT_RESOURCE_TYPE, "PackedScene")
	BIND_PROP_HINT(Variant::FLOAT, model_scale, PROPERTY_HINT_RANGE, "0.001,10,0.001")
	BIND_PROP(Variant::VECTOR3, ground_rotation_degrees)
	BIND_PROP(Variant::VECTOR3, hold_position)
	BIND_PROP(Variant::VECTOR3, hold_rotation_degrees)
	BIND_PROP(Variant::VECTOR3, view_position)
	BIND_PROP(Variant::VECTOR3, view_rotation_degrees)

	ClassDB::bind_method(D_METHOD("is_equippable"), &ItemData::is_equippable);
	ClassDB::bind_static_method("ItemData", D_METHOD("get_category_name", "category"), &ItemData::get_category_name);
	ClassDB::bind_static_method("ItemData", D_METHOD("get_slot_name", "slot"), &ItemData::get_slot_name);
	ClassDB::bind_static_method("ItemData", D_METHOD("get_use_type_name", "use_type"), &ItemData::get_use_type_name);
}

String ItemData::get_use_type_name(int p_use_type) {
	switch (p_use_type) {
		case USE_MELEE: return String::utf8("근거리");
		case USE_RANGED: return String::utf8("원거리");
		default: return String::utf8("기타");
	}
}

#undef BIND_PROP
#undef BIND_PROP_HINT

String ItemData::get_category_name(int p_category) {
	switch (p_category) {
		case CAT_FOOD: return String::utf8("식량");
		case CAT_WATER: return String::utf8("물");
		case CAT_MEDICAL: return String::utf8("의약품");
		case CAT_TOOL: return String::utf8("도구");
		case CAT_MATERIAL: return String::utf8("자재");
		case CAT_GEAR: return String::utf8("장비");
		default: return String::utf8("잡동사니");
	}
}

String ItemData::get_slot_name(int p_slot) {
	switch (p_slot) {
		case SLOT_HAND: return String::utf8("손");
		case SLOT_HEAD: return String::utf8("머리");
		case SLOT_BODY: return String::utf8("몸");
		case SLOT_BACK: return String::utf8("등");
		default: return String();
	}
}

Node3D *ItemData::instantiate_model() const {
	if (model.is_null()) {
		return nullptr;
	}
	Node3D *node = Object::cast_to<Node3D>(model->instantiate());
	if (node) {
		node->set_scale(Vector3(1, 1, 1) * model_scale);
	}
	return node;
}

Transform3D ItemData::get_hold_transform() const {
	return Transform3D(basis_from_degrees(hold_rotation_degrees), hold_position);
}

Transform3D ItemData::get_view_transform() const {
	return Transform3D(basis_from_degrees(view_rotation_degrees), view_position);
}

Ref<Texture2D> ItemData::get_icon() const {
	if (icon.is_valid()) {
		return icon;
	}
	if (cached_icon.is_null() && !String(id).is_empty()) {
		const String path = "res://assets/icons/" + String(id) + ".png";
		if (ResourceLoader::get_singleton()->exists(path)) {
			cached_icon = ResourceLoader::get_singleton()->load(path);
		}
	}
	return cached_icon;
}

void ItemData::set_id(const StringName &p_value) { id = p_value; cached_icon.unref(); }
StringName ItemData::get_id() const { return id; }
void ItemData::set_display_name(const String &p_value) { display_name = p_value; }
String ItemData::get_display_name() const { return display_name; }
void ItemData::set_description(const String &p_value) { description = p_value; }
String ItemData::get_description() const { return description; }
void ItemData::set_category(int p_value) { category = p_value; }
int ItemData::get_category() const { return category; }
void ItemData::set_max_stack(int p_value) { max_stack = MAX(1, p_value); }
int ItemData::get_max_stack() const { return max_stack; }
void ItemData::set_weight(double p_value) { weight = p_value; }
double ItemData::get_weight() const { return weight; }
void ItemData::set_equip_slot(int p_value) { equip_slot = CLAMP(p_value, 0, SLOT_COUNT - 1); }
int ItemData::get_equip_slot() const { return equip_slot; }
void ItemData::set_use_type(int p_value) { use_type = CLAMP(p_value, 0, 2); }
int ItemData::get_use_type() const { return use_type; }
void ItemData::set_damage(double p_value) { damage = p_value; }
double ItemData::get_damage() const { return damage; }
void ItemData::set_attack_range(double p_value) { attack_range = p_value; }
double ItemData::get_attack_range() const { return attack_range; }
void ItemData::set_attack_cooldown(double p_value) { attack_cooldown = MAX(0.1, p_value); }
double ItemData::get_attack_cooldown() const { return attack_cooldown; }
void ItemData::set_attack_animation(const String &p_value) { attack_animation = p_value; }
String ItemData::get_attack_animation() const { return attack_animation; }
void ItemData::set_icon(const Ref<Texture2D> &p_value) { icon = p_value; }
void ItemData::set_model(const Ref<PackedScene> &p_value) { model = p_value; }
Ref<PackedScene> ItemData::get_model() const { return model; }
void ItemData::set_model_scale(double p_value) { model_scale = p_value; }
double ItemData::get_model_scale() const { return model_scale; }
void ItemData::set_ground_rotation_degrees(const Vector3 &p_value) { ground_rotation_degrees = p_value; }
Vector3 ItemData::get_ground_rotation_degrees() const { return ground_rotation_degrees; }
void ItemData::set_hold_position(const Vector3 &p_value) { hold_position = p_value; }
Vector3 ItemData::get_hold_position() const { return hold_position; }
void ItemData::set_hold_rotation_degrees(const Vector3 &p_value) { hold_rotation_degrees = p_value; }
Vector3 ItemData::get_hold_rotation_degrees() const { return hold_rotation_degrees; }
void ItemData::set_view_position(const Vector3 &p_value) { view_position = p_value; }
Vector3 ItemData::get_view_position() const { return view_position; }
void ItemData::set_view_rotation_degrees(const Vector3 &p_value) { view_rotation_degrees = p_value; }
Vector3 ItemData::get_view_rotation_degrees() const { return view_rotation_degrees; }
