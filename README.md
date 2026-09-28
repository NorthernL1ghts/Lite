# Lite

Lite is the engine backend. Editor creates, saves, loads, and plays scenes. Sandbox is an example scene, and the Sandbox app plays that scene on its own.

## Requirements

Windows, Visual Studio with MSVC, CMake, and the Vulkan SDK. The Vulkan installer sets `VULKAN_SDK`. spdlog, GLFW, Dear ImGui, and GLM are git submodules. If those folders are empty, run `git submodule update --init --recursive`.

## Build

```bat
scripts\build.bat
```

```sh
./scripts/build.sh
```

`Debug` is the default. `scripts\build.bat Release` and `scripts\build.bat --config Release` select another config. `--no-run` builds without opening Editor. `--arch`, `--build-dir`, `--generator`, and `--sdk` override the architecture, build directory, Visual Studio generator, and Windows SDK. The same settings are `LITE_CONFIG`, `LITE_ARCH`, `LITE_BUILD_DIR`, `LITE_GENERATOR`, `LITE_WINDOWS_SDK`, and `LITE_RUN`.

The build uses the compiler's latest C++ mode and the newest Windows SDK installed on the machine. `Lite.dll` is built in `build/bin/lite`. After Sandbox and Editor link, CMake copies it into `build/bin/sandbox` and `build/bin/editor`.

`LITE_WARN` logs from the engine. `LITE_CLIENT_WARN` logs from the application. Editor opens the Sandbox example stopped. The File menu creates and opens scenes, and Save Scene writes any file name to any folder. The play and pause symbols on the menu bar run the open scene, and F5 and F6 do the same. While the scene is stopped or paused, click an object in the viewport to edit its name, shader, colors, and transform. Press I for instrumentation.

## License

MIT. See [LICENSE](LICENSE).
