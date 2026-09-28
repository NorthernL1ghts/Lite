add_executable(Sandbox)

lite_sources(Sandbox)

find_package(Vulkan REQUIRED)
if(NOT Vulkan_GLSLC_EXECUTABLE)
    find_program(Vulkan_GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/Bin")
endif()
if(NOT Vulkan_GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc was not found. Install the Vulkan SDK.")
endif()

set(SANDBOX_SHADER_SOURCES
    assets/shaders/Triangle.vert
    assets/shaders/Triangle.frag
    assets/shaders/Quad.vert
    assets/shaders/Quad.frag
)
target_sources(Sandbox PRIVATE ${SANDBOX_SHADER_SOURCES})
set_source_files_properties(${SANDBOX_SHADER_SOURCES} PROPERTIES
    HEADER_FILE_ONLY ON
)

target_link_libraries(Sandbox PRIVATE Lite)

target_compile_definitions(Sandbox
    PRIVATE
        Sandbox=Sandbox
)

lite_bin_dir(Sandbox sandbox)

set(SANDBOX_ASSET_DIR "${CMAKE_BINARY_DIR}/bin/sandbox/assets")
add_custom_command(
    TARGET Sandbox
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Lite>"
        "${CMAKE_BINARY_DIR}/bin/sandbox"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SANDBOX_ASSET_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/Checkerboard.png"
        "${SANDBOX_ASSET_DIR}/Checkerboard.png"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Triangle.vert" -o "${SANDBOX_ASSET_DIR}/shaders/Triangle.vert.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Triangle.frag" -o "${SANDBOX_ASSET_DIR}/shaders/Triangle.frag.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Quad.vert" -o "${SANDBOX_ASSET_DIR}/shaders/Quad.vert.spv"
    COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/Quad.frag" -o "${SANDBOX_ASSET_DIR}/shaders/Quad.frag.spv"
    COMMENT "Copying Lite DLL and Sandbox assets"
    VERBATIM
)

target_compile_options(Sandbox
    PRIVATE
        /std:c++latest
        /W4
        /wd4251
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /utf-8
)

set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/sandbox"
)
