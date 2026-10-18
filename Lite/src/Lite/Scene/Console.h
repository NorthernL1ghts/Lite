#pragma once

#include <Lite/Core/Base.h>

#include <string>
#include <string_view>
#include <vector>

namespace Lite {

	class LITE_API Console
	{
	public:
		static void Log(std::string_view message);
		static void Append(std::string_view line);
		static const std::vector<std::string>& GetLines();
		static void Clear();
	};

}
