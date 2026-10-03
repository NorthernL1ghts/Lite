# Lite to-do

Work left on the engine, the editor, and the player. Items already finished are listed first so they are not rebuilt.

## Done

- **Delete and duplicate.** The scene panel can duplicate and delete the selection. Ctrl+D copies onto the same plane. Del removes the selection, and holding Del keeps removing the next entity. A copied camera is not primary. The physics body is removed with the entity.
- **Remove a component.** Every inspector section has a Remove button. Physics rebuilds the body immediately, including while playing.
- **Viewport gizmos.** Drag the selected entity to move it. Corner squares scale it. The ring rotates it. The inspector fields still type and slide.
- **Explorer drag.** A texture dropped on a material is assigned. The first image dropped while the Background plane has no mesh fills the view. Later drops are editable sprites. Opacity fades a sprite over whatever is behind it. A scene file dropped on the viewport or the scene panel opens that scene.
- **Projects.** A new project is written to disk at once. A new scene is saved under `assets/scenes`. Projects already contain `scenes`, `scripts`, `prefabs`, `materials`, and `textures`.
- **Sandbox.** The player opens `Sandbox/Sandbox.lite` and plays that project's start scene.
- **Planes.** Each plane has a draw order. A higher order renders in front. Entity sorting still orders the entities inside one plane. Picking uses the same order.
- **Shared scene calls.** Screen and world conversion, polygon hits, gizmo math, sprite placement, and texture assignment live in Lite. The editor draws the panels and the handles.
- **Draw path.** A sprite rotates its two axes once per shape, then places every corner from those axes. The scene looks up each drawable entity once, then submits it.
- **Core.** `FileSystem::Contains` is the path-inside-folder check. The layer stack walks forward and backward through one pair of helpers. The two loggers share one setup. The application passes the native window as an untyped handle.
- **Prefabs.** File > Save Prefab, or the scene panel context menu, writes the selection to `assets/prefabs` as the same component YAML a scene entity uses. The plane name is only a hint. Double-click a prefab, or drag it into the viewport, to place a normal copy with a fresh name, its own physics body, and no second primary camera. The inspector shows the prefab path. Later edits to the file do not rewrite copies already in the scene.
- **Scripts.** The scene panel sets the project script module. The path stays relative to the project file. Play loads that DLL and stop unloads it. A missing file or a failed load is a console error, and the scene still plays. A Script component stores a class name. On play, a class start function runs when it exists. Each frame, an update function receives the timestep. A collision begin calls a class that provided one, with the other entity's id. Sandbox and the editor load the same module. Sandbox ships `scripts/SandboxScripts.dll` with `Bounce` on the crate, `Patrol` on the carrier, and `Sensor` on the trigger.
- **Planes in the editor.** Add Plane creates a plane in front of the highest draw order. The name is edited on the row, and two planes cannot share a name regardless of capitalization. Deleting an empty plane removes it. Deleting a plane that still has entities moves those entities to World and says so in the panel. World stays while it has entities, and the scene keeps at least one plane. The list stays in document order. The number on the row is still the draw order.
- **Selection while playing.** A viewport click selects the frontmost entity during play and pause, using the same pick as edit mode. Del, Ctrl+D, and removing a component stay available and still update physics.
- **Undo.** The open scene has one undo stack. Ctrl+Z undoes and Ctrl+Y redoes an inspector edit, a gizmo drag, a delete, or an added component. A drag or a field edit becomes one step when it ends. Opening another scene clears the stack. Play and stop are not steps.
- **Asset paths.** The asset registry turns a texture path into the form that is saved. A file inside the project asset folder is stored relative to that folder. An absolute path outside the project is kept. Loading accepts either form, including an older `assets/...` path. The material field shows the stored path. The same file is loaded once.
- **Materials as files.** File > Save Material, or the scene panel context menu, writes the selected `MaterialComponent` to `assets/materials` as `*.material` YAML. The file stores the shader, color, vertex colors, tiling, offset, roughness, metallic, emission, opacity, and texture path. Dragging that file onto an entity's material copies those fields onto the same component. Sandbox includes `Red.material`.
- **Physics callbacks.** Contact end is logged the same way contact begin is, and it calls the collision script. A trigger stays out of the blocking collision and still reports the overlap, including when that overlap ends. Those reports use the same script hook.
- **Entity parenting.** An entity stores a parent id. The scene panel lists children under that parent, and dragging one entity onto another parents it there. Local position, rotation, and scale stay on the transform. Drawing, picking, the camera, and the physics body use the world transform. Deleting a parent deletes the children. Detach Children and Delete keeps them, and the panel says which. Gizmos edit the local transform. The handles are drawn from the world transform.

## Next

### 10. Tests

There is no automated check for scene load, draw order, or physics.

- A small console program, or a Sandbox mode, that loads `assets/scenes/Sandbox.scene`, checks plane order, and checks that the dynamic body is above the ground before play.
- Run it from the existing build script with a flag, without opening the editor.

## Later

These are real gaps. They are not the next editing session.

- **Audio.** No listener, clip, or play call.
- **Animation.** No timeline for transform, color, or sprite frames.
- **3D meshes.** Drawing is quads, triangles, and sprites in one 2D batch.
- **Tile maps.** A background is one sprite. There is no grid.
- **Build a player.** Sandbox is the player for the example project. Shipping another project still means a new executable that loads a `*.lite` file.
- **Render device.** Vulkan is the only backend. Do not add a second API until a game needs one.
