add_executable(Sandbox)

lite_sources(Sandbox)
lite_compile_flags(Sandbox)
lite_bin_dir(Sandbox sandbox)

set(SANDBOX_SHADER_SOURCES
    assets/shaders/Triangle.vert
    assets/shaders/Triangle.frag
    assets/shaders/Quad.vert
    assets/shaders/Quad.frag
    assets/shaders/Batch.vert
    assets/shaders/Batch.frag
)
target_sources(Sandbox PRIVATE ${SANDBOX_SHADER_SOURCES})
set_source_files_properties(${SANDBOX_SHADER_SOURCES} PROPERTIES
    HEADER_FILE_ONLY ON
)

target_include_directories(Sandbox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(Sandbox PRIVATE Lite)

target_compile_definitions(Sandbox
    PRIVATE
        Sandbox=Sandbox
)

set(SANDBOX_ASSET_DIR "${CMAKE_BINARY_DIR}/bin/sandbox/assets")
add_custom_command(
    TARGET Sandbox
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Lite>"
        "${CMAKE_BINARY_DIR}/bin/sandbox"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SANDBOX_ASSET_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SANDBOX_ASSET_DIR}/scenes"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/Checkerboard.png"
        "${SANDBOX_ASSET_DIR}/Checkerboard.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes/Sandbox.scene"
        "${SANDBOX_ASSET_DIR}/scenes/Sandbox.scene"
    COMMENT "Copying Lite DLL and Sandbox assets"
    VERBATIM
)

lite_compile_shaders(Sandbox "${SANDBOX_ASSET_DIR}/shaders" ${SANDBOX_SHADER_SOURCES})

set_target_properties(Sandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/sandbox"
)
