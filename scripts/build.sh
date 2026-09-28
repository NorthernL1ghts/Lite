#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

config="${1:-Debug}"

case "$config" in
    Debug|Release|RelWithDebInfo|MinSizeRel) ;;
    *)
        echo "Usage: ./scripts/build.sh [Debug|Release|RelWithDebInfo|MinSizeRel]" >&2
        exit 1
        ;;
esac

candidates=()
if command -v vswhere.exe >/dev/null 2>&1; then
    candidates+=("$(command -v vswhere.exe)")
fi
candidates+=(
    "/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    "/mnt/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
)

vswhere=""
for candidate in "${candidates[@]}"; do
    if [[ -f "$candidate" ]]; then
        vswhere="$candidate"
        break
    fi
done

if [[ -z "$vswhere" ]]; then
    echo "vswhere.exe was not found. Install Visual Studio with the C++ toolset." >&2
    exit 1
fi

version="$("$vswhere" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion | tr -d '\r')"
major="${version%%.*}"

case "$major" in
    18) generator="Visual Studio 18 2026" ;;
    17) generator="Visual Studio 17 2022" ;;
    16) generator="Visual Studio 16 2019" ;;
    *)
        echo "Visual Studio ${version:-unknown} is installed, but this script does not know its CMake generator." >&2
        exit 1
        ;;
esac

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake was not found on PATH." >&2
    exit 1
fi

echo "Configuring with ${generator} into build/ ..."
cmake -S . -B build -G "$generator" -A x64

echo "Building ${config} ..."
cmake --build build --config "$config" --parallel

echo "Opening Sandbox in a new window..."
root="$(pwd)"
if command -v cygpath >/dev/null 2>&1; then
    root="$(cygpath -w "$root")"
fi
MSYS_NO_PATHCONV=1 cmd.exe /c start "Sandbox" /D "$root" cmd /k "build\\bin\\sandbox\\Sandbox.exe"
