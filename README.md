# Lite

Lite is a C++ game engine shared library. Sandbox is an executable that links to it.

## Build

Windows, Visual Studio with MSVC, and CMake. The build uses the compiler's latest C++ mode.

```bat
build.bat
```

```sh
./build.sh
```

`Debug` is the default. Pass `Release` for a release build.

`Lite.dll` is built in `build/bin/lite`. After Sandbox links, CMake copies it into `build/bin/sandbox`.

## License

MIT. See [LICENSE](LICENSE).
