# Lite to-do

Work left on the engine, the editor, and Sandbox. Finished items are listed first so they are not rebuilt.

An item is done when the behavior below is true in the running program. A missing file, a failed load, or a bad edit must stay a console message. The scene keeps playing.

Do not add a second graphics API. Vulkan is the only backend until a game needs another one.

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
- **Scripts.** The scene panel sets the project script module. The path stays relative to the project file. Play loads that DLL and stop unloads it. A missing file or a failed load is a console error, and the scene still plays. A Script component stores a class name. On play, a class start function runs when it exists. Each frame, an update function receives the timestep. A collision begin calls a class that provided one, with the other entity's id. Contact end uses the same hook. Sandbox and the editor load the same module. Sandbox ships `scripts/SandboxScripts.dll` with `Bounce` on the crate, `Patrol` on the carrier, and `Sensor` on the trigger.
- **Planes in the editor.** Add Plane creates a plane in front of the highest draw order. The name is edited on the row, and two planes cannot share a name regardless of capitalization. Deleting an empty plane removes it. Deleting a plane that still has entities moves those entities to World and says so in the panel. World stays while it has entities, and the scene keeps at least one plane. The list stays in document order. The number on the row is still the draw order. The name and the number sit in separate columns.
- **Selection while playing.** A viewport click selects the frontmost entity during play and pause, using the same pick as edit mode. Del, Ctrl+D, and removing a component stay available and still update physics.
- **Undo.** The open scene has one undo stack. Ctrl+Z undoes and Ctrl+Y redoes an inspector edit, a gizmo drag, a delete, or an added component. A drag or a field edit becomes one step when it ends. Opening another scene clears the stack. Play and stop are not steps.
- **Asset paths.** The asset registry turns a texture path into the form that is saved. A file inside the project asset folder is stored relative to that folder. An absolute path outside the project is kept. Loading accepts either form, including an older `assets/...` path. The material field shows the stored path. The same file is loaded once.
- **Materials.** A material stores the shader name, color, opacity, roughness, metallic, emission, vertex colors, tiling, offset, and an optional texture. Roughness 1 with metallic 0 and emission 0 is the matte look. Lower roughness adds a highlight. Metallic pulls the surface toward its color. Emission adds glow. File > Save Material, the inspector button, or the scene panel context menu writes the selected material to `assets/materials` as `*.material` YAML. Dragging that file onto an entity copies those fields onto the same component. Sandbox includes `Red.material` and `Blue.material`.
- **Physics callbacks.** Contact end is logged the same way contact begin is, and it calls the collision script. A trigger stays out of the blocking collision and still reports the overlap, including when that overlap ends.
- **Entity parenting.** An entity stores a parent id. The scene panel lists children under that parent, and dragging one entity onto another parents it there. Local position, rotation, and scale stay on the transform. Drawing, picking, the camera, and the physics body use the world transform. Deleting a parent deletes the children. Detach Children and Delete keeps them, and the panel says which. Gizmos edit the local transform. The handles are drawn from the world transform.
- **Inspector layout.** Transform, Mesh, and Material start open. The other sections start closed. Color and opacity stay on the material. Surface holds roughness, metallic, and emission. More holds vertex colors, tiling, offset, the texture, and Save Material. A rigidbody shows the body type and freeze rotation first. Mass, gravity, and velocity are under Motion. Camera near and far are under Clipping. The color control is a swatch. Clicking it opens the picker.
- **Editor chrome.** The scene list is entity names. Children sit under their parent. The project path and the script module are under Project. Play, pause, and reset stay off the File, Edit, and View menus. The viewport status sits on a dark badge. Explorer actions and the folder path wrap instead of drawing on top of each other.
- **Sandbox scene.** `assets/scenes/Sandbox.scene` shows one use of each current feature: three planes, a sprite, a quad, and a triangle, an orthographic camera and a perspective camera that is not primary, a static ground, a dynamic crate and ball, a kinematic spinner, a trigger, `Bounce`, `Patrol`, and `Sensor`, a parent with two children, and prefab links on the ground, crate, and ball. The crate and the ball start above the ground.
- **Scene check.** `SceneCheck` loads `Sandbox/assets/scenes/Sandbox.scene` with no window. Background is behind World, and World is behind Foreground. Before play, the bottom of the crate and the bottom of the ball are above the top of the ground. `scripts\build.bat --check` builds, runs that program, and does not open the editor. A failure prints one line and exits non-zero.
- **Viewport target.** The editor draws the scene into a color target the size of the Viewport panel and shows that image in the panel. Picking and gizmos use that rectangle and the same view projection. Resizing the panel resizes the target. A zero-size panel draws nothing. Play uses the same image. Sandbox still draws into its window.
- **Batch flush.** Vertex and index memory stay mapped. A flush copies only the vertices and indices written since the last flush. A texture slot is written when that slot changes, and an unchanged frame does not call `vkUpdateDescriptorSets`. More than 16 textures flush and continue on another descriptor set, so the next texture is drawn.

## Next

Do these in order.

### 1. Matte shader path

The fragment shader always builds the highlight. A material with roughness 1, metallic 0, and emission 0 is the common case, and that highlight is zero. Vertex UVs already include tiling, and the flush sets the material tiling uniform to 1.

- That matte case samples the texture, tints it, and writes premultiplied alpha. It does not normalize a light vector.
- Roughness below 1, metallic above 0, and emission above 0 keep the current highlight, metal tint, and glow.
- The unused tiling multiply is removed, or the uniform stops pretending to tile. Existing scenes do not shift their UVs.

### 2. Scripted motion

A script runs before the physics step. The step then copies velocity from Box2D back onto the component. Writing `LinearVelocity` during update does not move a kinematic body. `Patrol` works by moving a parent that has no body, and a static child collider is teleported after the step.

- A kinematic body's velocity, set by a script during update, is the velocity used for that step.
- A dynamic body's scripted velocity is applied the same way when the script sets it.
- `Patrol` can drive a kinematic platform directly. Bodies resting on it are a separate problem and stay out of this item.
- The collision script can tell a begin from an end. `Sensor` brightens on begin and fades on end.

## Later

Real gaps. They are not the next session. Each block says what exists today so the work is not started from a blank guess.

### Core

The loop polls events, begins a frame, updates layers, renders layers, flushes, draws ImGui, presents, and limits the frame rate. Input can ask if a key or mouse button is down and where the cursor is. Q and E rotate the editor camera while the scene is stopped. The scroll wheel zooms it.

- **Fixed step.** A long frame is clamped to 0.05 seconds inside the physics step. There is no accumulator. Play should step physics at a fixed rate and pass the leftover time as interpolation. Rendering still uses the real frame.
- **Script input.** A script cannot read the keyboard or the mouse. Add a small query for key, mouse button, and cursor position in world units of the active camera. Sandbox can use it. The editor camera keeps Q, E, and the wheel.
- **One log in the console.** Engine logs and `Console::Log` are separate. Play errors should appear in the editor console without a second setup.
- **Headless update.** `SceneCheck` loads the scene and stops. The same program should also step a scene for a fixed number of frames and report where the crate ended. Still no window.

### Rendering

Drawing is one 2D batch. Quads, triangles, and sprites share it. Blend is premultiplied. The batch holds 4096 quads and 16 textures. `Scene::Render` builds a draw list, sorts by plane then sorting order, and submits every drawable. `WorldTransform` walks parents for each one. There is no frustum test. Textures are uploaded as a single level.

- **Draw list.** Keep the sorted list and rebuild it when an entity, a plane, a parent, or a material changes. A frame that only moves a body updates transforms, not the whole sort from scratch.
- **Cull.** A drawable whose world bounds miss the camera is not submitted. A sprite larger than the view, such as the checkerboard, still draws when it covers the camera.
- **Mips.** A texture larger than its on-screen size keeps a mip chain. The checkerboard, scaled down, does not shimmer. A 1x1 white texture stays one level.
- **Unused shader files.** `Triangle` and `Quad` shader sources are compiled and are not the batch. Delete them when nothing references them, and leave `Batch` as the only sprite shader.

### Scene

A scene is planes of entities. Components are transform, camera, mesh, material, spin, rigidbody, box collider, circle collider, sorting, and script. YAML load and save round-trip those fields. One undo stack covers inspector edits, gizmos, delete, and added components.

- **Dirty scene.** The window or the scene panel shows that the open scene has unsaved edits. Save clears it. Play and stop do not mark it dirty.
- **Runtime scene change.** A call loads another scene from the project by the path stored in the project, the way the editor opens a scene. Scripts can ask for it. The editor Stop button still returns to the scene that was open when play began.
- **Tags.** An entity can store a short tag. The scene panel can filter by it. Physics does not use it until collision layers exist.

### Physics

Box2D starts on play. Bodies are static, kinematic, or dynamic. Shapes are boxes and circles, solid or trigger. Begin and end are logged and sent to scripts. Gravity is down at 9.81, with a per-body gravity scale. Rotation can be frozen. A collider with no rigidbody is static.

- **Queries.** A script can raycast and ask for the first solid hit, with the entity id and the point. A miss returns no hit. Triggers are skipped by that query.
- **Layers.** A body stores a layer and a mask. Two bodies collide only when each mask includes the other's layer. The default layer collides with everything, so current scenes do not change. The inspector edits both fields.
- **Joints.** One joint type is enough to start: a distance joint between two entities. It is saved on one of them. Deleting either entity removes the joint.

### Scripts and assets

One DLL per project exports `LiteRegisterScripts`. A class is a name plus start, update, and collision function pointers. The editor and Sandbox load that file on play and free it on stop. Materials and prefabs placed in a scene are copies. Editing the file later does not change copies already in the scene. The asset registry loads a texture once.

- **Collision phase.** Covered by scripted motion. Do not add a second callback.
- **Reload while stopped.** Replacing the DLL on disk and pressing play loads the new file. If the file is locked, the console says so and the old behavior is not half-applied.
- **Watched textures.** Changing a texture file on disk while the editor is open reloads that texture. The scene does not need a restart. A failed reload keeps the previous image and logs the path.

### Editor

The dock is Scene, Viewport, Inspector, Console, and Explorer. Playback is F5, F6, and F7. Undo is Ctrl+Z and Ctrl+Y. The explorer creates folders, imports files, and opens scenes, prefabs, and projects.

- **Multi-select.** Shift-click and a box drag in the viewport add to the selection. Gizmos move the group. Delete removes the group. Duplicate copies the group onto the same planes. A single click returns to one entity.
- **Snap.** Holding Ctrl while dragging a gizmo snaps position to a grid. The grid size is one field in the viewport, remembered for the session.
- **Console.** The panel can clear, and it can hide lines that do not contain a typed word. Physics contact spam can be muted without muting script errors.
- **Material preview.** The inspector swatch shows the material's color. A later pass can show roughness and emission on that swatch. Do not block the swatch on that.

### Sandbox and shipping

Sandbox is a player. It locates `Sandbox/Sandbox.lite`, opens the start scene, and calls play. The scroll wheel zooms the player camera. Key presses are traced. There is no gameplay input on the crate.

- **Player input.** After script input exists, the crate or a new body moves from the keyboard. The scene stays playable with no keys held.
- **Project argument.** Starting Sandbox with a path to a `*.lite` file plays that project. No argument still plays `Sandbox/Sandbox.lite`. A missing file logs the path and exits.
- **One player binary.** Shipping a project is that executable plus the project folder. It is not a new executable per project.

### Out of scope until a game needs them

- **Audio.** No listener, clip, or play call.
- **Animation.** No timeline for transform, color, or sprite frames.
- **3D meshes.** Drawing stays quads, triangles, and sprites.
- **Tile maps.** A background is one sprite. There is no grid.
- **A second render API.** Do not start one.
