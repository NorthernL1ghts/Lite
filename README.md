# Lite

Lite is a C++ game engine shared library. Sandbox is an executable that links to it.

## Requirements

Windows, Visual Studio with MSVC, CMake, and the Vulkan SDK. The Vulkan installer sets `VULKAN_SDK`. spdlog, GLFW, Dear ImGui, and GLM are git submodules. If those folders are empty, run `git submodule update --init --recursive`.

## Build

```bat
scripts\build.bat
```

```sh
./scripts/build.sh
```

`Debug` is the default. `scripts\build.bat Release` and `scripts\build.bat --config Release` select another config. `--no-run` builds without opening Sandbox. `--arch`, `--build-dir`, `--generator`, and `--sdk` override the architecture, build directory, Visual Studio generator, and Windows SDK. The same settings are `LITE_CONFIG`, `LITE_ARCH`, `LITE_BUILD_DIR`, `LITE_GENERATOR`, `LITE_WINDOWS_SDK`, and `LITE_RUN`.

The build uses the compiler's latest C++ mode and the newest Windows SDK installed on the machine. `Lite.dll` is built in `build/bin/lite`. After Sandbox links, CMake copies it into `build/bin/sandbox`.

`LITE_WARN` logs from the engine. `LITE_CLIENT_WARN` logs from the application. Sandbox opens a GLFW window.

## License

MIT. See [LICENSE](LICENSE).
