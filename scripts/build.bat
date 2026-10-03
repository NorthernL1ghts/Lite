@echo off
setlocal EnableExtensions

cd /d "%~dp0.."

if not defined LITE_CONFIG set "LITE_CONFIG=Debug"
if not defined LITE_ARCH set "LITE_ARCH=x64"
if not defined LITE_BUILD_DIR set "LITE_BUILD_DIR=build"
if not defined LITE_RUN set "LITE_RUN=1"
if not defined LITE_CHECK set "LITE_CHECK=0"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--help" goto help
if /I "%~1"=="-h" goto help
if /I "%~1"=="--no-run" (
    set "LITE_RUN=0"
    shift
    goto parse_args
)
if /I "%~1"=="--check" (
    set "LITE_CHECK=1"
    set "LITE_RUN=0"
    shift
    goto parse_args
)
if /I "%~1"=="--config" (
    set "LITE_CONFIG=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--arch" (
    set "LITE_ARCH=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--build-dir" (
    set "LITE_BUILD_DIR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--generator" (
    set "LITE_GENERATOR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--sdk" (
    set "LITE_WINDOWS_SDK=%~2"
    shift
    shift
    goto parse_args
)
echo %~1 | findstr /R /I "^-" >nul
if not errorlevel 1 (
    echo Unknown option: %~1
    goto usage
)
set "LITE_CONFIG=%~1"
shift
goto parse_args

:args_done
if "%LITE_CHECK%"=="1" set "LITE_RUN=0"

if /I not "%LITE_CONFIG%"=="Debug" if /I not "%LITE_CONFIG%"=="Release" if /I not "%LITE_CONFIG%"=="RelWithDebInfo" if /I not "%LITE_CONFIG%"=="MinSizeRel" (
    echo Config must be Debug, Release, RelWithDebInfo, or MinSizeRel.
    exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo cmake was not found on PATH.
    exit /b 1
)

if not defined LITE_GENERATOR call :detect_generator
if errorlevel 1 exit /b 1

echo Configuring with %LITE_GENERATOR% (%LITE_ARCH%) into %LITE_BUILD_DIR%\ ...
if defined LITE_WINDOWS_SDK (
    cmake -S . -B "%LITE_BUILD_DIR%" -G "%LITE_GENERATOR%" -A "%LITE_ARCH%" -DLITE_WINDOWS_SDK=%LITE_WINDOWS_SDK%
) else (
    cmake -S . -B "%LITE_BUILD_DIR%" -G "%LITE_GENERATOR%" -A "%LITE_ARCH%"
)
if errorlevel 1 exit /b 1

echo Building %LITE_CONFIG% ...
cmake --build "%LITE_BUILD_DIR%" --config %LITE_CONFIG% --parallel
if errorlevel 1 exit /b 1

if "%LITE_CHECK%"=="1" (
    echo Running scene check...
    "%LITE_BUILD_DIR%\bin\check\SceneCheck.exe"
    if errorlevel 1 exit /b 1
)

if "%LITE_RUN%"=="0" exit /b 0

echo Opening Editor in a new window...
start "Editor" /D "%cd%" cmd /k "%LITE_BUILD_DIR%\bin\editor\Editor.exe"
exit /b 0

:detect_generator
if not defined LITE_VSWHERE set "LITE_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%LITE_VSWHERE%" set "LITE_VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%LITE_VSWHERE%" (
    echo vswhere.exe was not found. Install Visual Studio with the C++ toolset, or pass --generator.
    exit /b 1
)

set "LITE_VSVER="
for /f "usebackq delims=" %%i in (`"%LITE_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do (
    set "LITE_VSVER=%%i"
)

if not defined LITE_VSVER (
    echo No Visual Studio installation with the MSVC toolset was found.
    exit /b 1
)

for /f "tokens=1 delims=." %%m in ("%LITE_VSVER%") do set "LITE_VSMAJOR=%%m"

if "%LITE_VSMAJOR%"=="18" set "LITE_GENERATOR=Visual Studio 18 2026"
if "%LITE_VSMAJOR%"=="17" set "LITE_GENERATOR=Visual Studio 17 2022"
if "%LITE_VSMAJOR%"=="16" set "LITE_GENERATOR=Visual Studio 16 2019"

if not defined LITE_GENERATOR (
    echo Visual Studio %LITE_VSVER% is installed. Pass --generator with the matching CMake generator.
    exit /b 1
)
exit /b 0

:help
call :usage
exit /b 0

:usage
echo Usage: scripts\build.bat [Debug^|Release^|RelWithDebInfo^|MinSizeRel] [options]
echo   --config ^<config^>       Build configuration. Default: Debug, or LITE_CONFIG.
echo   --arch ^<arch^>           CMake architecture. Default: x64, or LITE_ARCH.
echo   --build-dir ^<dir^>       Build directory. Default: build, or LITE_BUILD_DIR.
echo   --generator ^<name^>      CMake generator. Default: detected Visual Studio, or LITE_GENERATOR.
echo   --sdk ^<version^>         Windows SDK version. Default: newest installed, or LITE_WINDOWS_SDK.
echo   --no-run                Build without opening Editor. LITE_RUN=0 does the same.
echo   --check                 Build, run the scene check, and do not open Editor. LITE_CHECK=1 does the same.
exit /b 1
