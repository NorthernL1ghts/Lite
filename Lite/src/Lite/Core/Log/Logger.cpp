#include <Lite/Core/Log/Logger.h>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace {

	Lite::Ref<spdlog::logger> s_CoreLogger;
	Lite::Ref<spdlog::logger> s_ClientLogger;

}

namespace Lite {

	void Logger::Init()
	{
		if (s_CoreLogger)
			return;

		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = spdlog::stdout_color_mt("LITE");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	void Logger::Shutdown()
	{
		s_ClientLogger.reset();
		s_CoreLogger.reset();
		spdlog::shutdown();
	}

	Ref<spdlog::logger>& Logger::GetCoreLogger()
	{
		return s_CoreLogger;
	}

	Ref<spdlog::logger>& Logger::GetClientLogger()
	{
		return s_ClientLogger;
	}

}
