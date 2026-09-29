#include <Lite/Core/Log/Logger.h>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace {

	Lite::Ref<spdlog::logger> s_CoreLogger;
	Lite::Ref<spdlog::logger> s_ClientLogger;

	Lite::Ref<spdlog::logger> MakeLogger(const char* name)
	{
		Lite::Ref<spdlog::logger> logger = spdlog::stdout_color_mt(name);
		logger->set_level(spdlog::level::trace);
		return logger;
	}

}

namespace Lite {

	void Logger::Init()
	{
		if (s_CoreLogger)
			return;

		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = MakeLogger("LITE");
		s_ClientLogger = MakeLogger("APP");
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
