# Lite

Lite is a Windows game engine. The engine itself is a shared library. Two programs sit on top of it:

- **Editor** is where scenes are created, edited, saved, and played.
- **Sandbox** is a small player for the example scene. It has no editor UI.

Rendering is Vulkan. 2D drawing goes through one batch renderer, so quads, triangles, and sprites share a draw. Physics is Box2D.

## Requirements

- Windows
- Visual Studio with MSVC
- CMake
- The Vulkan SDK, with `VULKAN_SDK` set by the installer

spdlog, GLFW, Dear ImGui (docking branch), GLM, Box2D, and yaml-cpp are git submodules. If `Lite/src/vendor` is empty after clone, run:

```bat
git submodule update --init --recursive
```

The build uses the compiler's latest C++ mode and the newest Windows SDK installed on the machine.

## Build

From the repository root:

```bat
scripts\build.bat
```

```sh
./scripts/build.sh
```

That configures, builds, and opens Editor. `Debug` and `x64` are the defaults.

| Option | Environment variable | What it does |
| --- | --- | --- |
| `Release` or `--config Release` | `LITE_CONFIG` | `Debug`, `Release`, `RelWithDebInfo`, or `MinSizeRel` |
| `--no-run` | `LITE_RUN=0` | Build without opening Editor |
| `--arch` | `LITE_ARCH` | Architecture, default `x64` |
| `--build-dir` | `LITE_BUILD_DIR` | Build directory, default `build` |
| `--generator` | `LITE_GENERATOR` | Visual Studio generator |
| `--sdk` | `LITE_WINDOWS_SDK` | Windows SDK version |

`Lite.dll` is written to `build/bin/lite`. Editor and Sandbox each get a copy next to their executable, along with the example scene and the checkerboard texture. Shaders are compiled to SPIR-V after the link.

`LITE_WARN` is the engine log level. `LITE_CLIENT_WARN` is the application log level.

## Editor

Editor opens `Sandbox/Sandbox.lite`. That project names the start scene and the asset folder. The start scene is `assets/scenes/Sandbox.scene`, resolved from the project directory. The project path and the scene path are shown in the Scene panel and in the console when they load.

The window is a dockspace:

- **Scene** lists planes in file order, then the entities on each plane, then the components on each entity. Add Plane creates a plane in front of the highest draw order. Rename a plane in the row. Two planes cannot share a name, even with different capitalization. Right-click a plane to delete it. An empty plane is removed. A plane that still has entities is removed only after those entities move to World, and the panel says so. World cannot be deleted while it still has entities, and the scene always keeps one plane. The number on the row is the draw order. Right-click an entity, or use the Duplicate and Delete buttons. Ctrl+D copies the selection onto the same plane. Del removes it, and holding Del keeps removing the next entity in the list. A copied camera is not primary.
- **Viewport** shows the scene. Click an entity here to select it while the scene is stopped, paused, or playing. The frontmost object wins. Drag the selected entity to move it. The corner squares scale it, and the ring around it rotates it. Del and Ctrl+D still delete and duplicate during play. Ctrl+Z undoes the last inspector edit, gizmo drag, delete, or added component. Ctrl+Y redoes it. The step is recorded when the drag or field edit ends. Opening another scene clears that history. Play and stop are not undo steps. Picking, the gizmo math, and placing a dropped texture are Lite calls. The editor draws the handles.
- **Inspector** edits the selected entity. Number fields slide when dragged and take a typed value when clicked. Physics checkboxes and body settings apply to the simulation immediately, including while the scene is playing.
- **Console** shows scene, physics, and selection messages.
- **Explorer** is the tab next to the console. A new project is saved immediately and a new scene is written into `assets/scenes`. Both already contain `scenes`, `scripts`, `prefabs`, `materials`, and `textures`. Those folders are shown as icons. Double-click a folder to open it, and double-click a scene to load it. Drag a texture onto a material to assign it. Drag a texture into the viewport to place it. The first image dropped while the Background plane is empty fills the view behind everything. Later drops are normal sprites at the cursor, in front, and selected so they can be moved and scaled. Opacity on the material fades a sprite over whatever is behind it. Drag a scene file onto the viewport or the scene panel to open it. File > Save Prefab, or the scene panel context menu, writes the selection to `assets/prefabs`. Double-click a prefab to place a copy, or drag it into the viewport to place it at the cursor. The inspector shows which prefab that copy came from. Sandbox includes `Crate`, `Ball`, and `Ground` under `assets/prefabs`. New Folder creates a folder in the current view. Import copies content files into that folder.

`View > Instrumentation` (I) adds Profile, Draw, GPU, and Swapchain panels.

### Files

| Action | Shortcut |
| --- | --- |
| New Project | Ctrl+Shift+N |
| Open Project... | Ctrl+Shift+O |
| Save Project | Ctrl+Shift+S |
| New Scene | Ctrl+N |
| Open Scene... | Ctrl+O |
| Save Scene | Ctrl+S |
| Save Scene As... | — |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y |

Project files are YAML documents named `*.lite`. A project stores its name, start scene, asset directory, and script module path. The start scene and asset directory are relative to the project file. The script module path is relative to the project file too. The scene panel shows that path, and Browse picks a DLL. Open and Save use the same file browser, filtered to `*.lite`.

A script module is a DLL that exports `LiteRegisterScripts`. Play loads it and stop unloads it. Each entity script is a component with a class name. On play, Lite calls that class's start function when it has one, then calls update with the timestep each frame. A class that provides a collision function is called when a contact begins, with the other entity's id. The editor and Sandbox load the same file. Sandbox's module is `scripts/SandboxScripts.dll`, and the scene quad uses the `Bounce` class.

Open and Save As use a file browser. Save Scene writes the scene that is already open. A new scene with no path opens Save As. Scene files are YAML documents named `*.scene`. The document has a `name` and a `planes` list. Each plane has a `name`, an `order`, and an `entities` list. A higher plane order is drawn in front of a lower one. Sorting still orders the entities inside one plane. Each entity has a `name` and one map per component, such as `transform`, `camera`, `mesh`, and `rigidbody2d`.

### Playback

The symbols in the center of the menu bar control the scene.

| Symbol | Key | What it does |
| --- | --- | --- |
| Play | F5 | Starts the scene. The primary camera drives the view, and Box2D simulates rigidbodies and colliders. |
| Pause | F6 | Freezes the simulation. The inspector can be edited, and play continues from there. |
| Reset | F7 | Restores the scene to the moment play began, then starts it again. |

While the scene is stopped, Q and E rotate the editor camera and the scroll wheel zooms. During play, the primary scene camera owns the view.

## Scenes

A scene is a list of planes. Each plane holds entities. A new scene starts with a World plane. `CreateEntity` puts the entity on that plane and gives it a transform. Other components are added on the entity:

```cpp
Entity quad = scene->CreateEntity("Quad");
quad.Add<MeshComponent>().Type = MeshType::Quad;
quad.Add<MaterialComponent>().Color = { 0.86f, 0.16f, 0.18f, 1.0f };
quad.Add<Rigidbody2DComponent>().Type = BodyType::Dynamic;
quad.Add<BoxCollider2DComponent>();
```

`Get<T>()`, `Has<T>()`, and `Remove<T>()` use the same pattern. The inspector can add any of these, and each section has a Remove button. Removing a physics component updates the body immediately, including while playing.

| Component | What it stores |
| --- | --- |
| Transform | Position, rotation, and scale |
| Camera | Orthographic or perspective, and whether it is the primary camera |
| Mesh | Quad, triangle, or sprite |
| Material | Shader name, color, vertex colors, tiling, and an optional texture |
| Spin | A constant spin, in radians per second, applied when play starts |
| Rigidbody 2D | Static, kinematic, or dynamic, plus mass, gravity, velocity, and freeze rotation |
| Box Collider 2D | Size, offset, and trigger |
| Circle Collider 2D | Radius, offset, and trigger |
| Sorting | Draw order. Higher values are drawn in front |

A collider with no rigidbody is a static body. Trigger colliders overlap and do not block. Dynamic bodies fall. Kinematic bodies move with their velocity and are not pushed. Static bodies stay where they are.

The example scene has a Background plane at order 0 for the checkerboard and a World plane at order 1 for the camera, triangle, dynamic quad, and static ground. The World plane renders in front of the background. Press play and the quad falls onto the ground. The console reports `Box2D started` and the collision.

## Sandbox

`build/bin/sandbox/Sandbox.exe` opens `Sandbox/Sandbox.lite` and plays that project's start scene. It is the project without the editor around it.

## License

MIT. See [LICENSE](LICENSE).
