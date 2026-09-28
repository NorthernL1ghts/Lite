#pragma once

#include <Lite/Core/Base.h>

#include <spdlog/spdlog.h>

namespace Lite {

	class LITE_API Logger
	{
	public:
		static void Init();
		static void Shutdown();

		static Ref<spdlog::logger>& GetCoreLogger();
		static Ref<spdlog::logger>& GetClientLogger();
	};

}

#define LITE_TRACE(...)    ::Lite::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define LITE_INFO(...)     ::Lite::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define LITE_WARN(...)     ::Lite::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define LITE_ERROR(...)    ::Lite::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define LITE_CRITICAL(...) ::Lite::Logger::GetCoreLogger()->critical(__VA_ARGS__)

#define LITE_CLIENT_TRACE(...)    ::Lite::Logger::GetClientLogger()->trace(__VA_ARGS__)
#define LITE_CLIENT_INFO(...)     ::Lite::Logger::GetClientLogger()->info(__VA_ARGS__)
#define LITE_CLIENT_WARN(...)     ::Lite::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define LITE_CLIENT_ERROR(...)    ::Lite::Logger::GetClientLogger()->error(__VA_ARGS__)
#define LITE_CLIENT_CRITICAL(...) ::Lite::Logger::GetClientLogger()->critical(__VA_ARGS__)
