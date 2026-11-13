extends SceneTree
## Renders an inventory icon for every item in res://items/ into res://assets/icons/<id>.png
## Run (needs a window, not --headless):
##   Godot_v4.7.2-stable_win64_console.exe --path . -s res://tools/icon_baker.gd

const ITEMS_DIR := "res://items/"
const OUT_DIR := "res://assets/icons/"
const SIZE := 128

func _initialize() -> void:
	_bake.call_deferred()

func _bake() -> void:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(OUT_DIR))

	var vp := SubViewport.new()
	vp.size = Vector2i(SIZE, SIZE)
	vp.transparent_bg = true
	vp.own_world_3d = true
	vp.msaa_3d = Viewport.MSAA_4X
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(vp)

	var env := Environment.new()
	env.background_mode = Environment.BG_CLEAR_COLOR
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.75, 0.75, 0.72)
	env.ambient_light_energy = 0.9
	var world_env := WorldEnvironment.new()
	world_env.environment = env
	vp.add_child(world_env)

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-50, 35, 0)
	sun.light_energy = 1.2
	vp.add_child(sun)

	var cam := Camera3D.new()
	cam.projection = Camera3D.PROJECTION_ORTHOGONAL
	vp.add_child(cam)
	cam.make_current()

	var baked := 0
	for file in DirAccess.get_files_at(ITEMS_DIR):
		if not file.ends_with(".tres"):
			continue
		var item = load(ITEMS_DIR + file)
		if item == null or item.model == null:
			push_warning("skip " + file)
			continue
		var model: Node3D = item.model.instantiate()
		vp.add_child(model)

		var box := _aabb(model, Transform3D.IDENTITY)
		var center := box.get_center()
		var dir := Vector3(0.9, 0.75, 1.1).normalized()
		cam.size = box.size.length() * 0.92
		cam.near = 0.01
		cam.far = box.size.length() * 6.0 + 10.0
		cam.global_position = center + dir * box.size.length() * 2.5
		cam.look_at(center, Vector3.UP)

		for i in 3:
			await RenderingServer.frame_post_draw
		var img := vp.get_texture().get_image()
		var out := OUT_DIR + String(item.id) + ".png"
		img.save_png(ProjectSettings.globalize_path(out))
		print("baked ", out)
		baked += 1
		model.queue_free()
		await process_frame

	print("icon_baker: %d icons" % baked)
	quit()

func _aabb(n: Node, xf: Transform3D) -> AABB:
	var out := AABB(); var first := true; var t := xf
	if n is Node3D: t = xf * (n as Node3D).transform
	if n is VisualInstance3D:
		out = t * (n as VisualInstance3D).get_aabb(); first = false
	for c in n.get_children():
		var cb := _aabb(c, t)
		if cb.size != Vector3.ZERO:
			out = cb if first else out.merge(cb); first = false
	return out
