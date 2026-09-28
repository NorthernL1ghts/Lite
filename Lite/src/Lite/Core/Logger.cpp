#include "Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace {

	std::shared_ptr<spdlog::logger> s_CoreLogger;
	std::shared_ptr<spdlog::logger> s_ClientLogger;

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

	std::shared_ptr<spdlog::logger>& Logger::GetCoreLogger()
	{
		return s_CoreLogger;
	}

	std::shared_ptr<spdlog::logger>& Logger::GetClientLogger()
	{
		return s_ClientLogger;
	}

}
