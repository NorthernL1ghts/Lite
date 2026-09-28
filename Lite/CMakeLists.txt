add_library(Lite SHARED)

lite_sources(Lite)

target_include_directories(Lite
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(Lite
    PUBLIC
        LITE_PLATFORM_WINDOWS
        Lite=Lite
    PRIVATE
        BUILD_DLL
        LITE_BUILD_DLL
)

lite_bin_dir(Lite lite)

target_compile_options(Lite
    PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /utf-8
)
