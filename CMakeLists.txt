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

add_subdirectory(Lite)
add_subdirectory(Sandbox)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)
