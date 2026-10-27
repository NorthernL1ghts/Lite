#include <Lite/Core/Log/Logger.h>

#include <Lite/Core/String.h>
#include <Lite/Scene/Console.h>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdlib>
#include <mutex>
#include <string>

namespace {

	Lite::Ref<spdlog::logger> s_CoreLogger;
	Lite::Ref<spdlog::logger> s_ClientLogger;

	template<typename Mutex>
	class ConsoleSink final : public spdlog::sinks::base_sink<Mutex>
	{
	protected:
		void sink_it_(const spdlog::details::log_msg& message) override
		{
			if (message.level < spdlog::level::info)
				return;

			spdlog::memory_buf_t formatted;
			this->formatter_->format(message, formatted);
			std::string line(formatted.data(), formatted.size());
			while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
				line.pop_back();
			Lite::Console::Append(line);
		}

		void flush_() override
		{
		}
	};

	std::string ReadEnv(const char* name)
	{
		char* value = nullptr;
		size_t length = 0;
		if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
			return {};

		std::string text(value);
		std::free(value);
		return text;
	}

	spdlog::level::level_enum LevelFromEnv(const char* name, std::string& raw, bool& known)
	{
		known = true;
		raw = ReadEnv(name);
		std::string text = Lite::ToLower(Lite::Trim(raw));
		if (text.empty())
			return spdlog::level::trace;

		if (text == "trace")
			return spdlog::level::trace;
		if (text == "debug")
			return spdlog::level::debug;
		if (text == "info")
			return spdlog::level::info;
		if (text == "warn" || text == "warning")
			return spdlog::level::warn;
		if (text == "error" || text == "err")
			return spdlog::level::err;
		if (text == "critical")
			return spdlog::level::critical;
		if (text == "off")
			return spdlog::level::off;

		known = false;
		return spdlog::level::trace;
	}

	Lite::Ref<spdlog::logger> MakeLogger(const char* name, const spdlog::sink_ptr& console, spdlog::level::level_enum level)
	{
		auto output = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		output->set_pattern("%^[%T] %n: %v%$");
		Lite::Ref<spdlog::logger> logger = std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list { output, console });
		logger->set_level(level);
		logger->flush_on(spdlog::level::warn);
		return logger;
	}

}

namespace Lite {

	void Logger::Init()
	{
		if (s_CoreLogger)
			return;

		auto console = std::make_shared<ConsoleSink<std::mutex>>();
		console->set_pattern("[%T] %n: %v");

		std::string coreLevel;
		std::string clientLevel;
		bool coreKnown = true;
		bool clientKnown = true;
		s_CoreLogger = MakeLogger("LITE", console, LevelFromEnv("LITE_WARN", coreLevel, coreKnown));
		s_ClientLogger = MakeLogger("APP", console, LevelFromEnv("LITE_CLIENT_WARN", clientLevel, clientKnown));
		spdlog::register_logger(s_CoreLogger);
		spdlog::register_logger(s_ClientLogger);
		if (!coreKnown)
			s_CoreLogger->warn("LITE_WARN '{}' is not a log level", coreLevel);
		if (!clientKnown)
			s_ClientLogger->warn("LITE_CLIENT_WARN '{}' is not a log level", clientLevel);
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
