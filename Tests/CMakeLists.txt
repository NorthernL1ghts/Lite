add_executable(SceneCheck)

lite_sources(SceneCheck)
lite_compile_flags(SceneCheck)
lite_bin_dir(SceneCheck check)

target_link_libraries(SceneCheck PRIVATE Lite)

add_custom_command(
    TARGET SceneCheck
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Lite>"
        "$<TARGET_FILE_DIR:SceneCheck>"
    COMMENT "Copying Lite DLL"
    VERBATIM
)
