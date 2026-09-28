#pragma once

#include "Platform.h"
#include "Memory.h"

#define LITE_BIT(x) (1 << (x))

#ifdef LITE_PLATFORM_WINDOWS
    #if defined(BUILD_DLL) || defined(LITE_BUILD_DLL)
        #define LITE_API __declspec(dllexport)
    #else
        #define LITE_API __declspec(dllimport)
    #endif
#else
    #error Lite only supports Windows with MSVC.
#endif
