#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

lite_config="${LITE_CONFIG:-Debug}"
lite_arch="${LITE_ARCH:-x64}"
lite_build_dir="${LITE_BUILD_DIR:-build}"
lite_generator="${LITE_GENERATOR:-}"
lite_sdk="${LITE_WINDOWS_SDK:-}"
lite_run="${LITE_RUN:-1}"

usage() {
    cat <<'EOF'
Usage: ./scripts/build.sh [Debug|Release|RelWithDebInfo|MinSizeRel] [options]
  --config <config>       Build configuration. Default: Debug, or LITE_CONFIG.
  --arch <arch>           CMake architecture. Default: x64, or LITE_ARCH.
  --build-dir <dir>       Build directory. Default: build, or LITE_BUILD_DIR.
  --generator <name>      CMake generator. Default: detected Visual Studio, or LITE_GENERATOR.
  --sdk <version>         Windows SDK version. Default: newest installed, or LITE_WINDOWS_SDK.
  --no-run                Build without opening Sandbox. LITE_RUN=0 does the same.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h)
            usage
            exit 0
            ;;
        --no-run)
            lite_run=0
            shift
            ;;
        --config)
            lite_config="${2:?--config needs a value}"
            shift 2
            ;;
        --arch)
            lite_arch="${2:?--arch needs a value}"
            shift 2
            ;;
        --build-dir)
            lite_build_dir="${2:?--build-dir needs a value}"
            shift 2
            ;;
        --generator)
            lite_generator="${2:?--generator needs a value}"
            shift 2
            ;;
        --sdk)
            lite_sdk="${2:?--sdk needs a value}"
            shift 2
            ;;
        Debug|Release|RelWithDebInfo|MinSizeRel)
            lite_config="$1"
            shift
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

case "$lite_config" in
    Debug|Release|RelWithDebInfo|MinSizeRel) ;;
    *)
        echo "Config must be Debug, Release, RelWithDebInfo, or MinSizeRel." >&2
        exit 1
        ;;
esac

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake was not found on PATH." >&2
    exit 1
fi

vswhere_property() {
    local property="$1"
    if [[ -n "${LITE_VSWHERE:-}" ]]; then
        MSYS_NO_PATHCONV=1 "$LITE_VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property "$property" | tr -d '\r'
        return
    fi
    if command -v vswhere.exe >/dev/null 2>&1; then
        vswhere.exe -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property "$property" | tr -d '\r'
        return
    fi
    cmd.exe /d /c "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property ${property}" | tr -d '\r'
}

if [[ -z "$lite_generator" ]]; then
    version="$(vswhere_property installationVersion || true)"
    major="${version%%.*}"
    case "$major" in
        18) lite_generator="Visual Studio 18 2026" ;;
        17) lite_generator="Visual Studio 17 2022" ;;
        16) lite_generator="Visual Studio 16 2019" ;;
        *)
            echo "Visual Studio ${version:-was not found}. Pass --generator with the matching CMake generator." >&2
            exit 1
            ;;
    esac
fi

cmake_args=(-S . -B "$lite_build_dir" -G "$lite_generator" -A "$lite_arch")
if [[ -n "$lite_sdk" ]]; then
    cmake_args+=("-DLITE_WINDOWS_SDK=${lite_sdk}")
fi

echo "Configuring with ${lite_generator} (${lite_arch}) into ${lite_build_dir}/ ..."
cmake "${cmake_args[@]}"

echo "Building ${lite_config} ..."
cmake --build "$lite_build_dir" --config "$lite_config" --parallel

if [[ "$lite_run" == "0" ]]; then
    exit 0
fi

echo "Opening Sandbox in a new window..."
root="$(pwd)"
if command -v cygpath >/dev/null 2>&1; then
    root="$(cygpath -w "$root")"
fi
MSYS_NO_PATHCONV=1 cmd.exe /c start "Sandbox" /D "$root" cmd /k "${lite_build_dir}\\bin\\sandbox\\Sandbox.exe"
