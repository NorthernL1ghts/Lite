cmake_minimum_required(VERSION 3.25)

# Use the newest Windows SDK installed on this machine. These must be set
# before project() so CMake does not fall back to the host OS version.
file(GLOB _windows_sdk_dirs LIST_DIRECTORIES true
    "C:/Program Files (x86)/Windows Kits/10/Include/[0-9]*")
set(_latest_windows_sdk "")
foreach(_sdk_dir IN LISTS _windows_sdk_dirs)
    cmake_path(GET _sdk_dir FILENAME _sdk_version)
    if(_sdk_version VERSION_GREATER _latest_windows_sdk)
        set(_latest_windows_sdk "${_sdk_version}")
    endif()
endforeach()
if(_latest_windows_sdk)
    set(CMAKE_SYSTEM_VERSION "${_latest_windows_sdk}" CACHE STRING "Windows SDK version" FORCE)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "${_latest_windows_sdk}" CACHE STRING "Windows SDK version" FORCE)
endif()

project(LiteEngine
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Lite game engine"
)

if(NOT MSVC)
    message(FATAL_ERROR "Lite requires the MSVC compiler.")
endif()

if(_latest_windows_sdk)
    message(STATUS "Using latest Windows SDK ${_latest_windows_sdk}")
endif()

# MSVC parallel PDB writes and file-level parallelism.
add_compile_options(/FS /MP)

# Keep each config in the same bin folder instead of appending Debug/Release.
function(lite_bin_dir target folder)
    set(output_dir "${CMAKE_BINARY_DIR}/bin/${folder}")
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${output_dir}"
        LIBRARY_OUTPUT_DIRECTORY "${output_dir}"
        ARCHIVE_OUTPUT_DIRECTORY "${output_dir}"
    )
    foreach(config IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${config}" config_upper)
        set_target_properties(${target} PROPERTIES
            "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${output_dir}"
            "LIBRARY_OUTPUT_DIRECTORY_${config_upper}" "${output_dir}"
            "ARCHIVE_OUTPUT_DIRECTORY_${config_upper}" "${output_dir}"
        )
    endforeach()
endfunction()

# Collect every source under the calling project's src/ directory.
function(lite_sources target)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    )
    list(FILTER sources EXCLUDE REGEX "[/\\\\]vendor[/\\\\]")
    list(FILTER sources EXCLUDE REGEX "[/\\\\]pch\\.h$")
    target_sources(${target} PRIVATE ${sources})
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${sources})
endfunction()

# C++23 via /std:c++latest. Do not also set CMAKE_CXX_STANDARD.
function(lite_compile_flags target)
    target_compile_options(${target} PRIVATE
        /std:c++latest
        /W4
        /wd4251
        /permissive-
        /Zc:__cplusplus
        /Zc:preprocessor
        /EHsc
        /utf-8
    )
endfunction()

function(lite_require_glslc)
    if(EXISTS "${Vulkan_GLSLC_EXECUTABLE}")
        return()
    endif()

    find_package(Vulkan QUIET)
    if(NOT EXISTS "${Vulkan_GLSLC_EXECUTABLE}")
        find_program(Vulkan_GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/Bin")
    endif()
    if(NOT EXISTS "${Vulkan_GLSLC_EXECUTABLE}")
        message(FATAL_ERROR "glslc was not found. Install the Vulkan SDK.")
    endif()
endfunction()

function(lite_compile_shaders target output_dir)
    lite_require_glslc()

    set(commands)
    foreach(shader IN LISTS ARGN)
        set(shader_path "${shader}")
        cmake_path(ABSOLUTE_PATH shader_path BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" NORMALIZE)
        cmake_path(GET shader FILENAME shader_name)
        list(APPEND commands
            COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${shader_path}" -o "${output_dir}/${shader_name}.spv"
        )
    endforeach()

    add_custom_command(
        TARGET ${target}
        POST_BUILD
        ${commands}
        COMMENT "Compiling shaders"
        VERBATIM
    )
endfunction()

add_subdirectory(Lite)
add_subdirectory(Sandbox)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)
