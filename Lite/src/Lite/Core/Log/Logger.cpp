#include <Lite/Core/Log/Logger.h>

#include <Lite/Scene/Console.h>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>

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

	Lite::Ref<spdlog::logger> MakeLogger(const char* name, const spdlog::sink_ptr& console)
	{
		auto output = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		output->set_pattern("%^[%T] %n: %v%$");
		Lite::Ref<spdlog::logger> logger = std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list { output, console });
		logger->set_level(spdlog::level::trace);
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
		s_CoreLogger = MakeLogger("LITE", console);
		s_ClientLogger = MakeLogger("APP", console);
		spdlog::register_logger(s_CoreLogger);
		spdlog::register_logger(s_ClientLogger);
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
