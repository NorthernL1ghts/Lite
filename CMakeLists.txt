cmake_minimum_required(VERSION 3.25)

include(cmake/WindowsSDK.cmake)

project(LiteEngine
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "Lite game engine"
)

if(NOT MSVC)
    message(FATAL_ERROR "Lite requires the MSVC compiler.")
endif()

if(LITE_WINDOWS_SDK)
    message(STATUS "Using Windows SDK ${LITE_WINDOWS_SDK}")
endif()

include(cmake/Lite.cmake)
lite_apply_global_flags()

add_subdirectory(Lite)
add_subdirectory(Sandbox)
add_subdirectory(Editor)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)
