#pragma once

#include <Lite/Core/Base.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Lite {

	class LITE_API FileSystem
	{
	public:
		[[nodiscard]] static std::filesystem::path ExecutableDirectory();
		[[nodiscard]] static std::filesystem::path Locate(const std::filesystem::path& relative);
		[[nodiscard]] static bool Exists(const std::filesystem::path& path);
		[[nodiscard]] static std::filesystem::path Unused(const std::filesystem::path& parent, const std::string& stem, std::string_view extension);
		[[nodiscard]] static std::vector<uint8_t> ReadBinary(const std::filesystem::path& path);
	};

}
