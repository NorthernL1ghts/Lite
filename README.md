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

- **Scene** lists planes, then the entities on each plane, then the components on each entity.
- **Viewport** shows the scene. While stopped or paused, click an entity here to select it. The frontmost object wins.
- **Inspector** edits the selected entity. Number fields slide when dragged and take a typed value when clicked. While the scene is playing, fields stay locked until you pause.
- **Console** shows scene, physics, and selection messages.

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

Project files are YAML documents named `*.lite`. A project stores its name, start scene, asset directory, and script module path. The start scene and asset directory are relative to the project file. Open and Save use the same file browser, filtered to `*.lite`.

Open and Save As use a file browser. Save Scene writes the scene that is already open. A new scene with no path opens Save As. Scene files are YAML documents named `*.scene`. The document has a `name` and a `planes` list. Each plane has a `name` and an `entities` list. Each entity has a `name` and one map per component, such as `transform`, `camera`, `mesh`, and `rigidbody2d`.

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

`Get<T>()`, `Has<T>()`, and `Remove<T>()` use the same pattern. The inspector has an Add control for the same set.

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

The example scene has a Background plane for the checkerboard and a World plane for the camera, triangle, dynamic quad, and static ground. Press play and the quad falls onto the ground. The console reports `Box2D started` and the collision.

## Sandbox

`build/bin/sandbox/Sandbox.exe` loads the same example scene and plays it immediately. It is the scene without the editor around it.

## License

MIT. See [LICENSE](LICENSE).
