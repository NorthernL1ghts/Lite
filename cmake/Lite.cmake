set(LITE_GLOBAL_COMPILE_FLAGS
    /FS
    /MP
)

set(LITE_COMPILE_FLAGS
    /std:c++latest
    /W4
    /wd4251
    /permissive-
    /Zc:__cplusplus
    /Zc:preprocessor
    /EHsc
    /utf-8
)

function(lite_apply_global_flags)
    add_compile_options(${LITE_GLOBAL_COMPILE_FLAGS})
endfunction()

function(lite_compile_flags target)
    target_compile_options(${target} PRIVATE ${LITE_COMPILE_FLAGS})
endfunction()

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

function(lite_require_glslc)
    if(EXISTS "${Vulkan_GLSLC_EXECUTABLE}")
        return()
    endif()

    find_package(Vulkan QUIET)

    set(glslc_hints)
    if(DEFINED ENV{VULKAN_SDK} AND NOT "$ENV{VULKAN_SDK}" STREQUAL "")
        list(APPEND glslc_hints "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
    endif()

    if(NOT EXISTS "${Vulkan_GLSLC_EXECUTABLE}")
        find_program(Vulkan_GLSLC_EXECUTABLE glslc HINTS ${glslc_hints})
    endif()
    if(NOT EXISTS "${Vulkan_GLSLC_EXECUTABLE}")
        message(FATAL_ERROR "glslc was not found. Install the Vulkan SDK and set VULKAN_SDK to its root.")
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
