#pragma once

#include <Lite/Core/Log/Logger.h>

#include <format>

#if !defined(NDEBUG)
	#define LITE_ENABLE_ASSERTS
#endif

#ifdef LITE_ENABLE_ASSERTS
	#define LITE_CORE_ASSERT(condition, ...) \
		do \
		{ \
			if (!(condition)) \
			{ \
				LITE_CRITICAL("Assertion failed ({}): {}", #condition, std::format(__VA_ARGS__)); \
				__debugbreak(); \
			} \
		} while (0)

	#define LITE_ASSERT(condition, ...) \
		do \
		{ \
			if (!(condition)) \
			{ \
				LITE_CLIENT_CRITICAL("Assertion failed ({}): {}", #condition, std::format(__VA_ARGS__)); \
				__debugbreak(); \
			} \
		} while (0)
#else
	#define LITE_CORE_ASSERT(condition, ...)
	#define LITE_ASSERT(condition, ...)
#endif
