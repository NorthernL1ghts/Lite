# Select a Windows SDK before project(). Included from the root CMakeLists.

set(_lite_sdk "")

if(LITE_WINDOWS_SDK)
    set(_lite_sdk "${LITE_WINDOWS_SDK}")
elseif(DEFINED ENV{LITE_WINDOWS_SDK} AND NOT "$ENV{LITE_WINDOWS_SDK}" STREQUAL "")
    set(_lite_sdk "$ENV{LITE_WINDOWS_SDK}")
elseif(DEFINED ENV{WindowsSDKVersion} AND NOT "$ENV{WindowsSDKVersion}" STREQUAL "")
    set(_lite_sdk "$ENV{WindowsSDKVersion}")
else()
    set(_lite_kits_root "")
    if(DEFINED ENV{WindowsSdkDir} AND NOT "$ENV{WindowsSdkDir}" STREQUAL "")
        set(_lite_kits_root "$ENV{WindowsSdkDir}")
    else()
        foreach(_lite_reg IN ITEMS
            "HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots"
            "HKLM\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots")
            execute_process(
                COMMAND reg query "${_lite_reg}" /v KitsRoot10
                OUTPUT_VARIABLE _lite_reg_out
                RESULT_VARIABLE _lite_reg_rc
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(_lite_reg_rc EQUAL 0)
                string(REGEX MATCH "KitsRoot10[ \t]+REG_SZ[ \t]+([^\r\n]+)" _lite_kits_match "${_lite_reg_out}")
                if(CMAKE_MATCH_1)
                    string(STRIP "${CMAKE_MATCH_1}" _lite_kits_root)
                    break()
                endif()
            endif()
        endforeach()
    endif()

    if(_lite_kits_root)
        string(REGEX REPLACE "[\\/]+$" "" _lite_kits_root "${_lite_kits_root}")
        file(GLOB _lite_sdk_dirs LIST_DIRECTORIES true "${_lite_kits_root}/Include/[0-9]*")
        foreach(_lite_sdk_dir IN LISTS _lite_sdk_dirs)
            cmake_path(GET _lite_sdk_dir FILENAME _lite_sdk_version)
            if(_lite_sdk_version VERSION_GREATER _lite_sdk)
                set(_lite_sdk "${_lite_sdk_version}")
            endif()
        endforeach()
    endif()
endif()

string(REGEX REPLACE "[\\/]+$" "" _lite_sdk "${_lite_sdk}")

if(_lite_sdk)
    set(CMAKE_SYSTEM_VERSION "${_lite_sdk}" CACHE STRING "Windows SDK version" FORCE)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "${_lite_sdk}" CACHE STRING "Windows SDK version" FORCE)
    set(LITE_WINDOWS_SDK "${_lite_sdk}" CACHE STRING "Windows SDK version selected for Lite" FORCE)
endif()

unset(_lite_sdk)
unset(_lite_kits_root)
unset(_lite_reg)
unset(_lite_reg_out)
unset(_lite_reg_rc)
unset(_lite_kits_match)
unset(_lite_sdk_dirs)
unset(_lite_sdk_dir)
unset(_lite_sdk_version)
