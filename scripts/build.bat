@echo off
setlocal EnableExtensions

cd /d "%~dp0.."

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

if /I not "%CONFIG%"=="Debug" if /I not "%CONFIG%"=="Release" if /I not "%CONFIG%"=="RelWithDebInfo" if /I not "%CONFIG%"=="MinSizeRel" (
    echo Usage: scripts\build.bat [Debug^|Release^|RelWithDebInfo^|MinSizeRel]
    exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo vswhere.exe was not found. Install Visual Studio with the C++ toolset.
    exit /b 1
)

set "VSVER="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do (
    set "VSVER=%%i"
)

if not defined VSVER (
    echo No Visual Studio installation with the MSVC toolset was found.
    exit /b 1
)

for /f "tokens=1 delims=." %%m in ("%VSVER%") do set "VSMAJOR=%%m"

set "GENERATOR="
if "%VSMAJOR%"=="18" set "GENERATOR=Visual Studio 18 2026"
if "%VSMAJOR%"=="17" set "GENERATOR=Visual Studio 17 2022"
if "%VSMAJOR%"=="16" set "GENERATOR=Visual Studio 16 2019"

if not defined GENERATOR (
    echo Visual Studio %VSVER% is installed, but this script does not know its CMake generator.
    exit /b 1
)

echo Configuring with %GENERATOR% into build\ ...
cmake -S . -B build -G "%GENERATOR%" -A x64
if errorlevel 1 exit /b 1

echo Building %CONFIG% ...
cmake --build build --config %CONFIG%
if errorlevel 1 exit /b 1

echo Opening Sandbox in a new window...
start "Sandbox" /D "%cd%" cmd /k build\bin\sandbox\Sandbox.exe
exit /b 0
