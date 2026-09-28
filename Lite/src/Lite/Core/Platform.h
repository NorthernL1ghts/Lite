#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#ifndef LITE_PLATFORM_WINDOWS
		#define LITE_PLATFORM_WINDOWS
	#endif
#else
	#error Lite only supports Windows.
#endif
