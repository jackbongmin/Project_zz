#pragma once

#include <godot_cpp/classes/node3d.hpp>

namespace godot {

// Adds static collision at runtime to every MeshInstance3D below this node.
// Use it as a parent for imported .glb models that have no collision of their own
// (kit walls -> TRIMESH for exact door/window holes, furniture/props -> CONVEX).
class AutoCollider : public Node3D {
	GDCLASS(AutoCollider, Node3D)

public:
	enum Mode {
		MODE_TRIMESH = 0,
		MODE_CONVEX = 1,
	};

private:
	int mode = MODE_TRIMESH;
	int built_count = 0;

	void build(Node *p_node);

protected:
	static void _bind_methods();

public:
	void _ready() override;

	void set_mode(int p_mode);
	int get_mode() const;
	int get_built_count() const;
};

} // namespace godot
