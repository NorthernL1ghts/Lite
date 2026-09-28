add_library(Lite SHARED)

set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_SYSTEM_INCLUDES ON CACHE BOOL "" FORCE)
add_subdirectory(src/vendor/spdlog)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(src/vendor/glfw)

lite_sources(Lite)

target_include_directories(Lite
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Lite
    PUBLIC
        spdlog::spdlog
    PRIVATE
        glfw
        opengl32
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
        /std:c++latest
        /W4
        /wd4251
        /permissive-
        /Zc:__cplusplus
        /EHsc
        /utf-8
)

target_precompile_headers(Lite PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/Lite/Core/pch.h>"
)
