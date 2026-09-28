# Lite

Lite is a C++ game engine shared library. Sandbox is an executable that links to it.

## Build

Windows, Visual Studio with MSVC, and CMake. The build uses the compiler's latest C++ mode.

```bat
scripts\build.bat
```

```sh
./scripts/build.sh
```

`Debug` is the default. Pass `Release` for a release build. The script opens Sandbox in its own window when the build succeeds.

`Lite.dll` is built in `build/bin/lite`. After Sandbox links, CMake copies it into `build/bin/sandbox`.

spdlog and GLFW live in `Lite/src/vendor` as git submodules. `LITE_WARN` logs from the engine. `LITE_CLIENT_WARN` logs from the application. Sandbox opens a GLFW window.

## License

MIT. See [LICENSE](LICENSE).
