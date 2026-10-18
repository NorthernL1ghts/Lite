#include <Lite/Scene/Console.h>

#include <Lite/Core/Log/Logger.h>

namespace Lite {

	namespace {

		std::vector<std::string> s_Lines;

	}

	void Console::Log(std::string_view message)
	{
		LITE_INFO("{}", message);
	}

	void Console::Append(std::string_view line)
	{
		s_Lines.emplace_back(line);
	}

	const std::vector<std::string>& Console::GetLines()
	{
		return s_Lines;
	}

	void Console::Clear()
	{
		s_Lines.clear();
	}

}
