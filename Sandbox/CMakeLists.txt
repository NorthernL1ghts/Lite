add_executable(Sandbox)

lite_sources(Sandbox)

target_link_libraries(Sandbox PRIVATE Lite)

target_compile_definitions(Sandbox
    PRIVATE
        Sandbox=Sandbox
)

lite_bin_dir(Sandbox sandbox)

add_custom_command(
    TARGET Sandbox
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Lite>"
        "${CMAKE_BINARY_DIR}/bin/sandbox"
    COMMENT "Copying Lite DLL from build/bin/lite to build/bin/sandbox"
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
