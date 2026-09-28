add_executable(Editor)

lite_sources(Editor)
lite_compile_flags(Editor)
lite_bin_dir(Editor editor)

set(EDITOR_SHADER_SOURCES
    ../Sandbox/assets/shaders/Triangle.vert
    ../Sandbox/assets/shaders/Triangle.frag
    ../Sandbox/assets/shaders/Quad.vert
    ../Sandbox/assets/shaders/Quad.frag
    ../Sandbox/assets/shaders/Batch.vert
    ../Sandbox/assets/shaders/Batch.frag
)
target_sources(Editor PRIVATE
    ${EDITOR_SHADER_SOURCES}
    "${CMAKE_SOURCE_DIR}/Sandbox/src/SandboxLayer.h"
    "${CMAKE_SOURCE_DIR}/Sandbox/src/SandboxLayer.cpp"
)
set_source_files_properties(${EDITOR_SHADER_SOURCES} PROPERTIES
    HEADER_FILE_ONLY ON
)

target_include_directories(Editor PRIVATE "${CMAKE_SOURCE_DIR}/Sandbox/src")
target_link_libraries(Editor PRIVATE Lite)

target_compile_definitions(Editor
    PRIVATE
        Editor=Editor
)

set(EDITOR_ASSET_DIR "${CMAKE_BINARY_DIR}/bin/editor/assets")
add_custom_command(
    TARGET Editor
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Lite>"
        "${CMAKE_BINARY_DIR}/bin/editor"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EDITOR_ASSET_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/Sandbox/assets/Checkerboard.png"
        "${EDITOR_ASSET_DIR}/Checkerboard.png"
    COMMENT "Copying Lite DLL and Sandbox assets"
    VERBATIM
)

lite_compile_shaders(Editor "${EDITOR_ASSET_DIR}/shaders" ${EDITOR_SHADER_SOURCES})

set_target_properties(Editor PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/editor"
)
