#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace Lite {

	[[nodiscard]] inline std::string_view Trim(std::string_view text)
	{
		while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
			text.remove_prefix(1);
		while (!text.empty() && (text.back() == ' ' || text.back() == '\t'))
			text.remove_suffix(1);
		return text;
	}

	[[nodiscard]] inline std::string ToLower(std::string_view text)
	{
		std::string result(text);
		for (char& character : result)
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

		return result;
	}

}
