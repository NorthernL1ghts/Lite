#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace Lite {

	[[nodiscard]] inline std::string ToLower(std::string_view text)
	{
		std::string result(text);
		for (char& character : result)
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

		return result;
	}

}
