# Lite

Lite is the engine backend. Editor is the editor: scenes and viewports live there. Sandbox is the test scene the editor runs, and it can also be launched on its own.

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

`LITE_WARN` logs from the engine. `LITE_CLIENT_WARN` logs from the application. Editor opens with the Sandbox test scene in the Viewport. Press I for instrumentation.

## License

MIT. See [LICENSE](LICENSE).
