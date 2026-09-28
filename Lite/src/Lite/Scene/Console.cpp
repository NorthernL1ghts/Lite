#include "Console.h"

#include "Lite/Core/Logger.h"

namespace Lite {

	namespace {

		std::vector<std::string> s_Lines;

	}

	void Console::Log(std::string_view message)
	{
		s_Lines.emplace_back(message);
		LITE_INFO("{}", message);
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
