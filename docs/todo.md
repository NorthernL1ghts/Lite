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
- **Scripts.** The scene panel sets the project script module. The path stays relative to the project file. Play loads that DLL and stop unloads it. A missing file or a failed load is a console error, and the scene still plays. A Script component stores a class name. On play, a class start function runs when it exists. Each frame, an update function receives the timestep. A collision begin calls a class that provided one, with the other entity's id. Sandbox and the editor load the same module. Sandbox ships `scripts/SandboxScripts.dll` with a `Bounce` class on the scene quad.

## Next

### 3. Planes in the editor

A scene can create planes in code, and the panel can change a plane's draw order. The panel cannot add, rename, or remove a plane.

- Add a plane from the scene panel. It is created in front of the current highest order.
- Rename a plane in place. `FindPlane` already compares names without case, so two planes should not end up with the same name.
- Delete a plane only when it has no entities, or move those entities to World first and say so in the panel.
- Keep the sidebar in document order. Draw order stays the number on the row.

### 4. Selection while playing

Clicking the viewport does not select an entity while the scene is playing. A gizmo drag still works if something was selected before play.

- Allow a click during play and pause to select the frontmost entity, using the same pick as edit mode.
- Keep Del, Ctrl+D, and component removal available during play. They already update physics.

### 5. Undo

Inspector edits, gizmo drags, deletes, and component adds have no undo.

- One undo stack per open scene. Ctrl+Z undoes, Ctrl+Y redoes.
- Record a step when a drag or a field edit ends, not on every mouse move.
- Clear the stack when another scene is opened. Play and stop do not need to undo the simulation.

### 6. Asset paths

A texture dropped from the explorer is stored as a full path. A texture already in the example scene is stored relative to the executable, such as `assets/Checkerboard.png`.

- If the file is inside the project's asset directory, save the path relative to that directory.
- Load either form. Absolute paths keep working for a file outside the project.
- The material texture field should show the path that will be saved.

### 7. Materials as files

The Materials folder exists. A material is only data on an entity.

- Save a material component as `*.material` YAML: shader, color, vertex colors, tiling, opacity, and texture path.
- Drag that file onto an entity's material to copy those fields onto it.
- Do not invent a second material system beside `MaterialComponent`.

### 8. Physics callbacks

Box2D steps, writes transforms back, and logs the start of a contact. End of contact and sensors are not reported as their own events.

- Log contact end the same way contact begin is logged. Contact begin already calls a script that asked for collisions.
- Keep trigger overlaps out of the blocking collision, which they already are, and still report them.
- Contact end and sensor overlaps should call that same script hook.

### 9. Entity parenting

Every entity is a root. A child's transform is not relative to a parent.

- Store a parent id on the entity. The scene panel shows children under the parent.
- Local position, rotation, and scale stay on the transform. The draw and the physics body use the world transform.
- Deleting a parent deletes the children, or detaches them, and the panel says which.
- Gizmos edit the local transform. The handles are drawn from the world transform.

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
