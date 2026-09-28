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

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(src/vendor/glm)

set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BOX2D_VALIDATE OFF CACHE BOOL "" FORCE)
add_subdirectory(src/vendor/box2d)

lite_sources(Lite)

target_include_directories(Lite
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui/backends
)

find_package(Vulkan REQUIRED)

set(LITE_IMGUI_SOURCES
    src/vendor/imgui/imgui.cpp
    src/vendor/imgui/imgui_draw.cpp
    src/vendor/imgui/imgui_tables.cpp
    src/vendor/imgui/imgui_widgets.cpp
    src/vendor/imgui/imgui_demo.cpp
    src/vendor/imgui/backends/imgui_impl_glfw.cpp
    src/vendor/imgui/backends/imgui_impl_vulkan.cpp
)

target_sources(Lite PRIVATE ${LITE_IMGUI_SOURCES})
set_source_files_properties(${LITE_IMGUI_SOURCES} PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
    COMPILE_OPTIONS "/w"
)

target_compile_definitions(Lite
    PUBLIC
        "IMGUI_USER_CONFIG=\"Lite/ImGui/ImGuiConfig.h\""
        LITE_PLATFORM_WINDOWS
        Lite=Lite
    PRIVATE
        BUILD_DLL
        LITE_BUILD_DLL
)

target_link_libraries(Lite
    PUBLIC
        spdlog::spdlog
        glm::glm
        Vulkan::Vulkan
    PRIVATE
        glfw
        box2d
)

lite_bin_dir(Lite lite)
lite_compile_flags(Lite)

target_precompile_headers(Lite PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/Lite/Core/pch.h>"
)
